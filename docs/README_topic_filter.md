# `TopicFilter`

## Overview

The `TopicFilter` class (`topic_filter.h`) provides a flexible mechanism for matching input strings (often referred to as "topics" or "keys," for example, from Redis notifications or messaging systems) against a collection of predefined filtering rules. It supports several types of matching logic:

-   **Exact Match:** Matches if the input key is identical to a rule pattern.
-   **Prefix Match:** Matches if the input key starts with a specified prefix. Supports `*` as a wildcard at the end of the prefix pattern.
-   **Range Match:** Matches keys of the format "PREFIX_NUMBER" where "NUMBER" falls within a specified numeric range (inclusive).
-   **Regular Expression Match:** Matches if the input key satisfies a given regular expression. This can be either a full match or a search for the pattern within the key.

Rules are evaluated in a specific order (typically exact, prefix, range, then regex) for performance, returning `true` on the first match.

## Features

-   **Multiple Rule Types:** Supports exact, prefix, numeric range, and regular expression matching.
-   **Flexible Regex:** Regex rules can specify full matching (`RegexMode::MATCH`) or substring searching (`RegexMode::SEARCH`), and can use standard `std::regex` syntax options and match flags.
-   **Performance Considerations:**
    -   Order of rule evaluation is designed to check faster rule types first.
    -   `std::unordered_set` for exact matches provides O(1) average lookup.
    -   `optimize()` method attempts to improve average performance by sorting prefix, range, and regex rules.
    -   `reserve()` method to pre-allocate memory for rules.
-   **Clear API:** Methods for adding different types of rules, checking for matches, clearing rules, and getting statistics.
-   **Error Handling:** Throws `std::invalid_argument` for invalid rule patterns (e.g., empty strings, invalid ranges, bad regex syntax).

## Core Components

### Rule Storage
-   `m_exactMatches`: `std::unordered_set<std::string>`
-   `m_prefixMatches`: `std::vector<std::string>`
-   `m_rangeMatches`: `std::vector<RangeRule>` (where `RangeRule { std::string prefix; long long start; long long end; }`)
-   `m_regexMatches`: `std::vector<RegexRule>` (for full matches)
-   `m_regexSearches`: `std::vector<RegexRule>` (for search/substring matches)
    (where `RegexRule { std::string pattern_str; std::unique_ptr<std::regex> regex; ... }`)

## Public Interface Highlights

### Adding Rules
-   **`void addExactMatch(const std::string& key_pattern)` / `(std::string&&)`**:
    Adds a rule for exact string matching.
-   **`void addPrefixMatch(const std::string& key_pattern)` / `(std::string&&)`**:
    Adds a prefix rule. If `key_pattern` ends with `*`, it's treated as a wildcard (e.g., "Eth*" matches "Ethernet0"). Otherwise, the full string is the prefix.
-   **`void addRangeMatch(const std::string& key_prefix, long long start, long long end)` / `(std::string&&, ...)`**:
    Adds a rule for keys like "PREFIX_NUMBER" (e.g., `key_prefix="VLAN"`, `start=1`, `end=100` matches "VLAN_1" through "VLAN_100"). An underscore `_` is automatically appended to `key_prefix` if not present.
-   **`void addRegexMatch(const std::string& pattern, RegexMode mode = RegexMode::MATCH, ...)` / `(std::string&&, ...)`**:
    Adds a regular expression rule.
    -   `mode`: `TopicFilter::RegexMode::MATCH` (default) for full string match, or `TopicFilter::RegexMode::SEARCH` for substring match.
    -   `syntax_opts`: e.g., `std::regex_constants::ECMAScript`, `std::regex_constants::icase`.
    -   `match_flags`: e.g., `std::regex_constants::match_default`.

### Matching
-   **`bool match(std::string_view key) const`**:
    Checks the input `key` against all configured rules in order: exact, prefix, range, regex-match, regex-search. Returns `true` on the first match, `false` otherwise.

### Management
-   **`void clear()`**: Removes all rules.
-   **`size_t size() const noexcept`**: Returns the total number of rules.
-   **`void reserve(size_t exact_count, size_t prefix_count, ...)`**: Pre-allocates space.
-   **`void optimize()`**: Sorts internal rule collections for potentially faster average matching. Call after adding a batch of rules.
-   **`Statistics getStatistics() const noexcept`**: Returns a struct with counts of each rule type.
    ```cpp
    struct Statistics {
        size_t exact_rules;
        size_t prefix_rules;
        size_t range_rules;
        size_t regex_match_rules;
        size_t regex_search_rules;
        size_t total_rules;
    };
    ```

## Usage Examples

(Based on `tests/topic_filter_test.cpp`)

### Basic Rule Types

```cpp
#include "topic_filter.h" // Adjust path as needed
#include <iostream>
#include <string>

int main() {
    TopicFilter filter;

    // Exact matches
    filter.addExactMatch("ALARM_SystemFailure");
    filter.addExactMatch("EVENT_UserLogin");

    // Prefix matches
    filter.addPrefixMatch("STATS_Port*"); // Matches STATS_Port1, STATS_PortChannel5, etc.
    filter.addPrefixMatch("CONFIG_ACL");  // Matches CONFIG_ACL_Ingress, CONFIG_ACL_Egress

    // Range matches (e.g., for VLAN IDs or interface numbers)
    filter.addRangeMatch("VLAN", 100, 199); // Matches VLAN_100, ..., VLAN_199
    filter.addRangeMatch("Ethernet", 1, 48);  // Matches Ethernet_1, ..., Ethernet_48

    // Regex matches
    // Match keys like "TEMP_Sensor_0" to "TEMP_Sensor_9"
    filter.addRegexMatch(R"(TEMP_Sensor_[0-9])", TopicFilter::RegexMode::MATCH);
    // Search for any key containing "Error" (case-insensitive)
    filter.addRegexMatch(R"(Error)", TopicFilter::RegexMode::SEARCH, std::regex_constants::icase);


    // Optimize after adding rules (optional, but recommended for performance)
    filter.optimize();

    // Test matching
    std::cout << std::boolalpha;
    std::cout << "Match 'ALARM_SystemFailure': " << filter.match("ALARM_SystemFailure") << std::endl; // true
    std::cout << "Match 'STATS_Port3/1': " << filter.match("STATS_Port3/1") << std::endl;         // true
    std::cout << "Match 'VLAN_150': " << filter.match("VLAN_150") << std::endl;                   // true
    std::cout << "Match 'VLAN_250': " << filter.match("VLAN_250") << std::endl;                   // false
    std::cout << "Match 'TEMP_Sensor_5': " << filter.match("TEMP_Sensor_5") << std::endl;         // true
    std::cout << "Match 'Application critical error log': "
              << filter.match("Application critical error log") << std::endl; // true (due to regex search)
    std::cout << "Match 'SOME_OTHER_KEY': " << filter.match("SOME_OTHER_KEY") << std::endl;       // false

    TopicFilter::Statistics stats = filter.getStatistics();
    std::cout << "Total rules: " << stats.total_rules << std::endl;
}
```

## Dependencies
- `<algorithm>`, `<cassert>`, `<charconv>`, `<memory>`, `<regex>`, `<stdexcept>`, `<string>`, `<string_view>`, `<unordered_set>`, `<vector>`, `<ostream>`, `<istream>`, `<cctype>`, `<sstream>`, `<limits>`, `<chrono>`, `<numeric>`, `<map>`, `<array>`, `<utility>`, `<cmath>`, `<optional>`. (Note: Some of these dependencies might be from the test file or broader includes in `tcam.h` rather than strictly `topic_filter.h` itself, but `std::regex`, `std::string`, `std::vector`, `std::unordered_set`, `std::string_view` are key.)

The `TopicFilter` class provides a comprehensive way to define and apply various types of matching rules to string-based keys or topics, making it suitable for event filtering, configuration dispatching, and similar tasks.

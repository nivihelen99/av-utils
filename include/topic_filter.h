#pragma once 

#include <algorithm>
#include <cassert>
#include <charconv>
#include <chrono>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace aos_utils
{

// Struct to hold the components of a range-based filtering rule
struct RangeRule
{
   std::string prefix;
   long long start;
   long long end;

   // Constructor for efficiency
   RangeRule(std::string p, long long s, long long e)
       : prefix(std::move(p))
       , start(s)
       , end(e)
   {
   }

   // Move constructor
   RangeRule(RangeRule&&) = default;
   RangeRule& operator=(RangeRule&&) = default;

   // Copy operations
   RangeRule(const RangeRule&) = default;
   RangeRule& operator=(const RangeRule&) = default;
};

// Struct to hold compiled regex rules for better performance
struct RegexRule
{
   std::string pattern_str; // Original pattern string
   std::unique_ptr<std::regex> regex; // Compiled regex
   std::regex_constants::match_flag_type flags;

   RegexRule(
      std::string pattern,
      std::regex_constants::syntax_option_type syntax_opts = std::regex_constants::ECMAScript,
      std::regex_constants::match_flag_type match_flags = std::regex_constants::match_default)
       : pattern_str(std::move(pattern))
       , regex(std::make_unique<std::regex>(pattern_str, syntax_opts))
       , flags(match_flags)
   {
   }

   // Move constructor
   RegexRule(RegexRule&&) = default;
   RegexRule& operator=(RegexRule&&) = default;

   // Copy constructor
   RegexRule(const RegexRule& other)
       : pattern_str(other.pattern_str)
       , regex(std::make_unique<std::regex>(*other.regex))
       , flags(other.flags)
   {
   }

   // Copy assignment
   RegexRule& operator=(const RegexRule& other)
   {
      if (this != &other)
      {
         pattern_str = other.pattern_str;
         regex = std::make_unique<std::regex>(*other.regex);
         flags = other.flags;
      }
      return *this;
   }

   bool matches(std::string_view key) const
   {
      // Convert string_view to string for regex matching
      std::string key_str(key);
      return std::regex_match(key_str, *regex, flags);
   }

   bool search(std::string_view key) const
   {
      std::string key_str(key);
      return std::regex_search(key_str, *regex, flags);
   }
};

class TopicFilter
{
   public:
   enum class RegexMode
   {
      MATCH, // Full string must match the regex pattern
      SEARCH // Regex pattern can match anywhere in the string
   };

   /**
     * @brief Adds a rule for an exact key match.
     * @param key_pattern The exact key to match (e.g., "VLAN_1000").
     */
   void addExactMatch(const std::string& key_pattern);
   void addExactMatch(std::string&& key_pattern);

   /**
     * @brief Adds a rule for a prefix match.
     * @param key_pattern The prefix to match. A trailing '*' is handled automatically.
     * (e.g., "Ethernet*", "PortChannel").
     */
   void addPrefixMatch(const std::string& key_pattern);
   void addPrefixMatch(std::string&& key_pattern);

   /**
     * @brief Adds a rule for a numeric range match for keys of format 'PREFIX_NUMBER'.
     * The range is inclusive.
     * @param key_prefix The non-numeric prefix of the key (e.g., "VLAN").
     * @param start The start of the numeric range.
     * @param end The end of the numeric range.
     */
   void addRangeMatch(const std::string& key_prefix, long long start, long long end);
   void addRangeMatch(std::string&& key_prefix, long long start, long long end);

   /**
     * @brief Adds a regex pattern rule.
     * @param pattern The regex pattern (e.g., "VLAN_[0-9]+", "Ethernet[0-9]+/[0-9]+/[0-9]+").
     * @param mode Whether to match the full string or search within the string.
     * @param syntax_opts Regex syntax options (default: ECMAScript).
     * @param match_flags Regex matching flags (default: match_default).
     */
   void addRegexMatch(
      const std::string& pattern,
      RegexMode mode = RegexMode::MATCH,
      std::regex_constants::syntax_option_type syntax_opts = std::regex_constants::ECMAScript,
      std::regex_constants::match_flag_type match_flags = std::regex_constants::match_default);

   void addRegexMatch(
      std::string&& pattern,
      RegexMode mode = RegexMode::MATCH,
      std::regex_constants::syntax_option_type syntax_opts = std::regex_constants::ECMAScript,
      std::regex_constants::match_flag_type match_flags = std::regex_constants::match_default);

   /**
     * @brief Checks if a given key matches any of the defined filtering rules.
     * Rules are checked in order of performance: exact -> prefix -> range -> regex.
     * @param key The key from the Redis notification.
     * @return True if the key matches any rule, false otherwise.
     */
   bool match(std::string_view key) const;

   /**
     * @brief Clears all rules.
     */
   void clear();

   /**
     * @brief Returns the total number of rules.
     */
   size_t size() const noexcept;

   /**
     * @brief Reserves space for rules to avoid reallocations.
     */
   void
   reserve(size_t exact_count, size_t prefix_count, size_t range_count, size_t regex_count = 0);

   /**
     * @brief Optimizes internal data structures after adding rules.
     * Call this after adding all rules for better performance.
     */
   void optimize();

   /**
     * @brief Returns statistics about the filter rules.
     */
   struct Statistics
   {
      size_t exact_rules;
      size_t prefix_rules;
      size_t range_rules;
      size_t regex_match_rules;
      size_t regex_search_rules;
      size_t total_rules;
   };

   Statistics getStatistics() const noexcept;

   private:
   std::unordered_set<std::string> m_exactMatches;
   std::vector<std::string> m_prefixMatches;
   std::vector<RangeRule> m_rangeMatches;
   std::vector<RegexRule> m_regexMatches; // Full string match patterns
   std::vector<RegexRule> m_regexSearches; // Search patterns

   // Helper function to parse number from string_view
   static bool parseNumber(std::string_view str, long long& result) noexcept;

   // Helper function to check if string starts with prefix
   static bool startsWith(std::string_view str, std::string_view prefix) noexcept;
};

// Implementation

void TopicFilter::addExactMatch(const std::string& key_pattern)
{
   if (key_pattern.empty())
   {
      throw std::invalid_argument("Empty key pattern not allowed");
   }
   m_exactMatches.insert(key_pattern);
}

void TopicFilter::addExactMatch(std::string&& key_pattern)
{
   if (key_pattern.empty())
   {
      throw std::invalid_argument("Empty key pattern not allowed");
   }
   m_exactMatches.insert(std::move(key_pattern));
}

void TopicFilter::addPrefixMatch(const std::string& key_pattern)
{
   if (key_pattern.empty())
   {
      throw std::invalid_argument("Empty key pattern not allowed");
   }

   if (key_pattern.back() == '*')
   {
      m_prefixMatches.emplace_back(key_pattern, 0, key_pattern.length() - 1);
   }
   else
   {
      m_prefixMatches.push_back(key_pattern);
   }
}

void TopicFilter::addPrefixMatch(std::string&& key_pattern)
{
   if (key_pattern.empty())
   {
      throw std::invalid_argument("Empty key pattern not allowed");
   }

   if (key_pattern.back() == '*')
   {
      key_pattern.pop_back();
   }
   m_prefixMatches.push_back(std::move(key_pattern));
}

void TopicFilter::addRangeMatch(const std::string& key_prefix, long long start, long long end)
{
   if (key_prefix.empty())
   {
      throw std::invalid_argument("Empty key prefix not allowed");
   }
   if (start > end)
   {
      throw std::invalid_argument("Invalid range: start must be <= end");
   }

   m_rangeMatches.emplace_back(key_prefix + "_", start, end);
}

void TopicFilter::addRangeMatch(std::string&& key_prefix, long long start, long long end)
{
   if (key_prefix.empty())
   {
      throw std::invalid_argument("Empty key prefix not allowed");
   }
   if (start > end)
   {
      throw std::invalid_argument("Invalid range: start must be <= end");
   }

   key_prefix += "_";
   m_rangeMatches.emplace_back(std::move(key_prefix), start, end);
}

void TopicFilter::addRegexMatch(const std::string& pattern,
                                RegexMode mode,
                                std::regex_constants::syntax_option_type syntax_opts,
                                std::regex_constants::match_flag_type match_flags)
{
   if (pattern.empty())
   {
      throw std::invalid_argument("Empty regex pattern not allowed");
   }

   try
   {
      if (mode == RegexMode::MATCH)
      {
         m_regexMatches.emplace_back(pattern, syntax_opts, match_flags);
      }
      else
      {
         m_regexSearches.emplace_back(pattern, syntax_opts, match_flags);
      }
   }
   catch (const std::regex_error& e)
   {
      throw std::invalid_argument("Invalid regex pattern: " + std::string(e.what()));
   }
}

void TopicFilter::addRegexMatch(std::string&& pattern,
                                RegexMode mode,
                                std::regex_constants::syntax_option_type syntax_opts,
                                std::regex_constants::match_flag_type match_flags)
{
   if (pattern.empty())
   {
      throw std::invalid_argument("Empty regex pattern not allowed");
   }

   try
   {
      if (mode == RegexMode::MATCH)
      {
         m_regexMatches.emplace_back(std::move(pattern), syntax_opts, match_flags);
      }
      else
      {
         m_regexSearches.emplace_back(std::move(pattern), syntax_opts, match_flags);
      }
   }
   catch (const std::regex_error& e)
   {
      throw std::invalid_argument("Invalid regex pattern: " + std::string(e.what()));
   }
}

bool TopicFilter::match(std::string_view key) const
{
   // 1. Check for exact match (O(1) average case) - fastest
   if (m_exactMatches.count(std::string(key)))
   {
      return true;
   }

   // 2. Check prefix matches (O(n*m)) - very fast
   for (const auto& prefix : m_prefixMatches)
   {
      if (startsWith(key, prefix))
      {
         return true;
      }
   }

   // 3. Check range matches - fast
   for (const auto& rule : m_rangeMatches)
   {
      if (startsWith(key, rule.prefix))
      {
         std::string_view num_part = key.substr(rule.prefix.length());
         if (num_part.empty())
            continue;

         long long num_value;
         if (parseNumber(num_part, num_value))
         {
            if (num_value >= rule.start && num_value <= rule.end)
            {
               return true;
            }
         }
      }
   }

   // 4. Check regex full matches - slower but more flexible
   for (const auto& regex_rule : m_regexMatches)
   {
      if (regex_rule.matches(key))
      {
         return true;
      }
   }

   // 5. Check regex search patterns - slowest but most flexible
   for (const auto& regex_rule : m_regexSearches)
   {
      if (regex_rule.search(key))
      {
         return true;
      }
   }

   return false;
}

void TopicFilter::clear()
{
   m_exactMatches.clear();
   m_prefixMatches.clear();
   m_rangeMatches.clear();
   m_regexMatches.clear();
   m_regexSearches.clear();
}

size_t TopicFilter::size() const noexcept
{
   return m_exactMatches.size() + m_prefixMatches.size() + m_rangeMatches.size() +
          m_regexMatches.size() + m_regexSearches.size();
}

void TopicFilter::reserve(size_t exact_count,
                          size_t prefix_count,
                          size_t range_count,
                          size_t regex_count)
{
   m_exactMatches.reserve(exact_count);
   m_prefixMatches.reserve(prefix_count);
   m_rangeMatches.reserve(range_count);
   m_regexMatches.reserve(regex_count / 2); // Estimate split between match and search
   m_regexSearches.reserve(regex_count / 2);
}

void TopicFilter::optimize()
{
   // Sort prefix matches by length (longer prefixes first) for potential early termination
   std::sort(m_prefixMatches.begin(),
             m_prefixMatches.end(),
             [](const std::string& a, const std::string& b) { return a.length() > b.length(); });

   // Sort range matches by prefix for potential cache locality
   std::sort(m_rangeMatches.begin(),
             m_rangeMatches.end(),
             [](const RangeRule& a, const RangeRule& b) { return a.prefix < b.prefix; });

   // Sort regex patterns by complexity (simpler patterns first for better average performance)
   auto complexity_comparator = [](const RegexRule& a, const RegexRule& b) {
      // Heuristic: shorter patterns are often simpler
      return a.pattern_str.length() < b.pattern_str.length();
   };

   std::sort(m_regexMatches.begin(), m_regexMatches.end(), complexity_comparator);
   std::sort(m_regexSearches.begin(), m_regexSearches.end(), complexity_comparator);
}

TopicFilter::Statistics TopicFilter::getStatistics() const noexcept
{
   return {m_exactMatches.size(),
           m_prefixMatches.size(),
           m_rangeMatches.size(),
           m_regexMatches.size(),
           m_regexSearches.size(),
           size()};
}

// Helper function implementations
bool TopicFilter::parseNumber(std::string_view str, long long& result) noexcept
{
   const char* begin = str.data();
   const char* end = str.data() + str.size();

   auto [ptr, ec] = std::from_chars(begin, end, result);
   return ec == std::errc {} && ptr == end;
}

bool TopicFilter::startsWith(std::string_view str, std::string_view prefix) noexcept
{
   return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

} // namespace aos_utils


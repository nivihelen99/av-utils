# `SplitView` — Zero-Copy String Tokenizer for C++17

## Overview

`SplitView` is a C++17 header-only library that provides a highly efficient, zero-copy utility for splitting strings (represented by `std::string_view`) based on a delimiter. It is inspired by the ease of use of Python's string splitting functions, offering a lazy iteration approach that is suitable for parsing large strings without upfront tokenization costs or memory allocations for the tokens themselves.

The tokenizer returns `std::string_view` tokens, which are slices of the original input string, making it very performant.

## Features

*   **Zero-copy**: Tokens are `std::string_view` objects that point to sections of the original string. No new strings are allocated for the tokens.
*   **Lazy Iteration**: Tokens are generated one by one as you iterate, not all at once. This is efficient for large inputs or when you only need the first few tokens.
*   **Delimiter Support**: Supports splitting by a single `char` or a `std::string_view` delimiter.
*   **C++17 Standard**: Uses only C++17 features and the Standard Library. No external dependencies.
*   **Header-Only**: Easy to integrate; just include `split_view.h`.
*   **STL Range-Style Iterator**: Compatible with range-based `for` loops and STL algorithms that work with input iterators.
*   **Handles Edge Cases**: Correctly processes empty strings, leading/trailing delimiters, and consecutive delimiters (which result in empty `std::string_view` tokens).

## How to Use

### 1. Include the Header

```cpp
#include "split_view.h" // Adjust path as necessary
```

### 2. Create a `SplitView` Object

You can construct a `SplitView` object by providing an input `std::string_view` and a delimiter. The delimiter can be a single character or a string view.

```cpp
std::string_view text = "apple,banana,orange";
char delimiter_char = ',';
std::string_view delimiter_sv = ",";

// Using a char delimiter
SplitView view_char(text, delimiter_char);

// Using a string_view delimiter
SplitView view_sv(text, delimiter_sv);

// Or with a string literal for the delimiter
SplitView view_literal(text, ",");
```

### 3. Iterate Over Tokens

Use a range-based `for` loop or standard iterator patterns to access the tokens.

```cpp
std::string data = "id:name:value";
SplitView parts(data, ':');

std::cout << "Tokens:\n";
for (std::string_view token : parts) {
    std::cout << "  [\"" << token << "\"]\n";
}
// Output:
// Tokens:
//   ["id"]
//   ["name"]
//   ["value"]
```

### Example: CSV Parsing

```cpp
#include "split_view.h"
#include <iostream>
#include <string_view>

int main() {
    std::string_view csv_line = "col1,col2,,col3"; // Note the empty token
    SplitView row(csv_line, ',');

    std::cout << "Parsing CSV line: \"" << csv_line << "\"\n";
    for (std::string_view token : row) {
        // Process each token
        std::cout << "Token: [" << token << "]\n";
    }
    return 0;
}
```

**Expected Output:**

```
Parsing CSV line: "col1,col2,,col3"
Token: [col1]
Token: [col2]
Token: []
Token: [col3]
```

### Example: Collecting Tokens into a Vector

```cpp
#include "split_view.h"
#include <iostream>
#include <vector>
#include <string_view>

int main() {
    std::string_view config_line = "feature_a;feature_b;feature_c";
    SplitView settings(config_line, ';');

    std::vector<std::string_view> setting_tokens;
    for (std::string_view token : settings) {
        setting_tokens.push_back(token);
    }
    // Alternatively, using iterator constructor:
    // std::vector<std::string_view> setting_tokens(settings.begin(), settings.end());


    std::cout << "Collected settings:\n";
    for (const auto& token : setting_tokens) {
        std::cout << " - " << token << "\n";
    }
    return 0;
}
```

## Behavior Details

*   **Empty Tokens**: `SplitView` generates empty `std::string_view` tokens for consecutive delimiters or for delimiters at the beginning or end of the input string. For example, `"a,,b"` split by `','` yields `"a"`, `""`, `"b"`. An input of `""` split by `','` yields `""`. An input of `","` split by `','` yields `""`, `""`.
*   **No Delimiter Found**: If the delimiter is not found in the remaining part of the string, the rest of the string is returned as the final token.
*   **Empty Input String**: If the input string is empty, `SplitView` yields a single empty `std::string_view` token.
*   **Whitespace**: `SplitView` does not trim whitespace from tokens by default. This should be handled by the user if needed.
*   **Empty Delimiter**: The behavior for an empty `std::string_view` delimiter is to treat it as if no delimiter is found, thus yielding the entire input string as a single token. (Note: This differs from some languages where an empty delimiter might split between each character or raise an error. `SplitView` takes a simpler approach for this specific edge case.)

## Performance

| Operation    | Time Complexity    | Memory                    |
|--------------|--------------------|---------------------------|
| `begin()`    | O(k) or O(1)*      | Zero allocations          |
| `operator++` | O(k) per delimiter | Zero allocations          |

*Note: `begin()` involves finding the first token. If the first token is at the beginning of the string (e.g. leading delimiter or very short first token), it's closer to O(length of delimiter). If the first delimiter is far into the string, it's O(distance to first delimiter + length of delimiter). `k` refers to the length of the delimiter or the distance scanned to find it.* The dominant factor for `operator++` is searching for the next delimiter.

## Constraints

*   **`std::string_view` Only**: Works only on `std::string_view`. It does not directly support `std::string`, `const char*` (though these can be easily converted to `std::string_view`), `wchar_t`, etc.
*   **No Escape Sequences**: Does not handle escape sequences (e.g., `\,` to escape a comma delimiter). For such advanced parsing, a more sophisticated tokenizer (like a shell-style lexer) would be needed.
*   **Lifetime Management**: The `SplitView` object and the `std::string_view` tokens it produces must not outlive the original string data they refer to. This is a standard characteristic of `std::string_view`.

## Real-World Value

*   Ideal for parsers, protocol decoders, log analysis, and configuration file readers where performance and low memory overhead are critical.
*   Lightweight and allocation-free tokenization makes it suitable for embedded systems, command-line tools, and high-performance data processing pipelines.
*   Offers an expressive and familiar API, particularly for developers accustomed to Python's string manipulation capabilities.

## Future Enhancements (Possible)

The current `SplitView` focuses on core delimiter-based splitting. Future enhancements could include:

*   `ShlexSplitView`: A shell-style quote and escape-aware tokenizer.
*   `SplitLines()`: A specialized and optimized version for splitting by newline characters.
*   `SplitView::map(fn)`: Ability to apply a transformation function to tokens lazily.
*   Optional behaviors like trimming whitespace or filtering empty tokens directly within the view.

---

This `SplitView` provides a fundamental building block for efficient string processing in C++.

# Suffix Array

## Overview

A Suffix Array is a data structure that stores all suffixes of a given text in lexicographical (sorted) order. More precisely, it's an integer array that stores the starting positions of these sorted suffixes. Suffix arrays are fundamental tools in string processing and bioinformatics, offering efficient ways to solve a variety of problems related to string matching and analysis.

They provide a more space-efficient alternative to Suffix Trees for many applications, though Suffix Trees can solve some problems more efficiently.

## Use Cases

Suffix Arrays are used for:

*   **Substring Searching:** Quickly find if a pattern string exists in a text and locate all its occurrences.
*   **Counting Substring Occurrences:** Efficiently count how many times a pattern appears.
*   **Longest Common Substring (LCS):** Finding the longest common substring between two or more strings (often requires an LCP array as well).
*   **Longest Repeated Substring:** Finding the longest substring that appears at least twice in the text.
*   **String Comparison and Similarity:** Various string metrics and comparisons.
*   **Data Compression and Bioinformatics:** Analyzing DNA sequences, text indexing, etc.

This implementation provides the core Suffix Array construction and searching capabilities. For more advanced applications like LCS, it would typically be augmented with an LCP (Longest Common Prefix) array.

## API

The `SuffixArray` class is defined in `include/SuffixArray.h`.

```cpp
#include "SuffixArray.h" // Located in include/
#include <string>
#include <vector>

class SuffixArray {
public:
    // Constructor
    // Builds the suffix array for the input 'text'.
    // Time Complexity: O(L log N) where N is text length and L is total length of strings compared during sort.
    // This often behaves as O(N log^2 N) or O(N log N * AvgSuffixLen) for typical inputs
    // with the current std::sort based approach. More advanced algorithms can achieve O(N log N) or O(N).
    // Space Complexity: O(N) for storing the suffix array and a copy of the text.
    SuffixArray(const std::string& text);

    // Returns a constant reference to the suffix array.
    // The suffix array is a vector of integers, where each integer is a starting
    // index of a suffix in the original text. These indices are sorted such that
    // the suffixes they point to are in lexicographical order.
    // Time Complexity: O(1)
    const std::vector<int>& get_array() const;

    // Finds all occurrences of 'pattern' in the original text.
    // Returns a vector of starting indices of all occurrences, sorted in ascending order.
    // Returns an empty vector if the pattern is empty or not found.
    // Time Complexity: O(M log N + K), where M is the length of the pattern,
    // N is the length of the text, and K is the number of occurrences.
    // O(M log N) for binary searching the range, O(K) to collect results.
    std::vector<int> find_occurrences(const std::string& pattern) const;

    // Counts the number of times 'pattern' occurs in the original text.
    // Returns 0 if the pattern is empty.
    // Time Complexity: O(M log N), where M is the length of the pattern and
    // N is the length of the text.
    size_t count_occurrences(const std::string& pattern) const;

    // Returns the length of the original text (number of characters).
    // Time Complexity: O(1)
    size_t size() const;

    // Returns true if the original text was empty, false otherwise.
    // Time Complexity: O(1)
    bool empty() const;
};
```

## Basic Usage Example

```cpp
#include <iostream>
#include <string>
#include <vector>
#include "SuffixArray.h" // Assuming SuffixArray.h is in include path set by build system

int main() {
    std::string text = "banana";
    SuffixArray sa(text);

    // Print all suffixes in lexicographical order
    std::cout << "Suffixes of \"" << text << "\" in sorted order:" << std::endl;
    const std::vector<int>& sorted_indices = sa.get_array();
    for (int index : sorted_indices) {
        std::cout << "\"" << text.substr(index) << "\" (starts at index " << index << ")" << std::endl;
    }
    // Expected Output (order of suffixes):
    // "a" (starts at index 5)
    // "ana" (starts at index 3)
    // "anana" (starts at index 1)
    // "banana" (starts at index 0)
    // "na" (starts at index 4)
    // "nana" (starts at index 2)

    std::string pattern = "ana";
    size_t count = sa.count_occurrences(pattern);
    std::cout << "\nPattern \"" << pattern << "\" occurs " << count << " times." << std::endl; // Output: 2

    std::vector<int> occurrences = sa.find_occurrences(pattern);
    std::cout << "Occurrences of \"" << pattern << "\" at indices: ";
    if (occurrences.empty()) {
        std::cout << "none";
    } else {
        for (size_t i = 0; i < occurrences.size(); ++i) {
            std::cout << occurrences[i] << (i == occurrences.size() - 1 ? "" : ", ");
        }
    }
    std::cout << std::endl; // Output: 1, 3 (sorted by index)

    // Example: Pattern not found
    std::string pattern_not_found = "xyz";
    size_t count_nf = sa.count_occurrences(pattern_not_found);
    std::cout << "\nPattern \"" << pattern_not_found << "\" occurs " << count_nf << " times." << std::endl; // Output: 0

    std::vector<int> occurrences_nf = sa.find_occurrences(pattern_not_found);
    std::cout << "Occurrences of \"" << pattern_not_found << "\" at indices: ";
     if (occurrences_nf.empty()) {
        std::cout << "none";
    } else {
        // this block should not be reached for "xyz"
    }
    std::cout << std::endl; // Output: none

    return 0;
}
```

## Notes

*   The current implementation uses a standard sorting approach for suffix array construction (`std::sort` with a custom comparator on string views/substrings). The complexity is O(L log N), where N is the text length and L is the total character comparisons performed by sort. This can be roughly O(N log<sup>2</sup> N) in many practical cases, or O(N log N * AverageComparedSuffixLength). For very large texts, specialized linear-time (O(N)) or more consistently O(N log N) suffix array construction algorithms (like SA-IS or the skew algorithm) would be more performant but are significantly more complex to implement.
*   The `find_occurrences` and `count_occurrences` methods use binary search (`std::lower_bound`, `std::upper_bound`) on the suffix array, leading to O(M log N) complexity for comparisons, where M is the pattern length.
*   The class stores its own copy of the input string to ensure its lifetime.
*   Searching for an empty pattern currently results in 0 occurrences.
```

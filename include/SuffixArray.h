#ifndef SUFFIX_ARRAY_H
#define SUFFIX_ARRAY_H

#include <string>
#include <vector>
#include <algorithm> // For std::sort, std::iota, std::min, std::lower_bound, std::upper_bound
#include <numeric>   // For std::iota (in C++17, otherwise in <algorithm>)

// Forward declaration for custom comparator if needed later
class SuffixArray;

namespace SuffixArrayDetail {
// Helper structure for initial suffix sorting if we don't store full strings
struct SuffixRef {
    int index;
    const std::string* text_ptr; // Pointer to the original text

    // Comparator for SuffixRef
    bool operator<(const SuffixRef& other) const {
        // Compare suffixes starting at index and other.index in *text_ptr
        return text_ptr->compare(index, std::string::npos, *other.text_ptr, other.index, std::string::npos) < 0;
    }
};
} // namespace SuffixArrayDetail


class SuffixArray {
public:
    // Constructs the suffix array for the given text.
    SuffixArray(const std::string& text) : text_ptr_(&text_storage_) {
        text_storage_ = text; // Store our own copy
        text_len_ = text_storage_.length();

        if (text_len_ == 0) {
            return;
        }

        sa_.resize(text_len_);
        std::iota(sa_.begin(), sa_.end(), 0); // Fill with 0, 1, 2, ...

        // Sort indices based on the suffixes they represent
        // This uses a lambda with string comparison, leading to O(N log N * L)
        // where L is average suffix comparison length (can be N in worst case for O(N^2 log N))
        // or O(N log^2 N) on average for string comparisons.
        std::sort(sa_.begin(), sa_.end(), [&](int a, int b) {
            return text_storage_.compare(a, std::string::npos, text_storage_, b, std::string::npos) < 0;
        });
    }

    // Returns the suffix array (indices of sorted suffixes).
    const std::vector<int>& get_array() const {
        return sa_;
    }

    // Finds all occurrences of the pattern in the text.
    // Returns a vector of starting indices of the occurrences, sorted.
    // Complexity: O(M log N + K) where M is pattern length, N is text length, K is number of occurrences.
    // M log N for binary searches, K for collecting results.
    std::vector<int> find_occurrences(const std::string& pattern) const {
        if (pattern.empty() || text_len_ == 0) {
            return {};
        }

        int pattern_len = pattern.length();

        // Find the first suffix that is >= pattern (lower_bound)
        auto first_it = std::lower_bound(sa_.begin(), sa_.end(), pattern,
            [&](int suffix_idx, const std::string& p) {
            return text_storage_.compare(suffix_idx, pattern_len, p) < 0;
        });

        if (first_it == sa_.end() || text_storage_.compare(*first_it, pattern_len, pattern) != 0) {
            return {}; // Pattern not found
        }

        // Find the first suffix that is > pattern (upper_bound)
        auto last_it = std::upper_bound(sa_.begin(), sa_.end(), pattern,
            [&](const std::string& p, int suffix_idx) {
            return p.compare(0, pattern_len, text_storage_, suffix_idx, pattern_len) < 0;
        });

        std::vector<int> occurrences;
        for (auto it = first_it; it != last_it; ++it) {
            occurrences.push_back(*it);
        }
        std::sort(occurrences.begin(), occurrences.end()); // Report in text order
        return occurrences;
    }

    // Counts occurrences of the pattern.
    // Complexity: O(M log N)
    size_t count_occurrences(const std::string& pattern) const {
        if (pattern.empty() || text_len_ == 0) {
            return 0;
        }
        int pattern_len = pattern.length();

        // Find the first suffix that is >= pattern (lower_bound)
        // Lambda returns true if suffix < pattern
        auto first_it = std::lower_bound(sa_.begin(), sa_.end(), pattern,
            [&](int suffix_idx, const std::string& p_param) {
                // Compare suffix starting at suffix_idx with p_param (pattern).
                // Use min_len to avoid reading past end of suffix if it's shorter than pattern.
                size_t len_to_compare_suffix = std::min((size_t)pattern_len, text_len_ - suffix_idx);
                return text_storage_.compare(suffix_idx, len_to_compare_suffix, p_param, 0, pattern_len) < 0;
            });

        if (first_it == sa_.end()) {
            return 0; // Pattern not found (or would be inserted at the end)
        }

        // Validate that the found suffix actually matches the pattern
        int current_sa_idx = *first_it;
        if (text_len_ - current_sa_idx < (size_t)pattern_len) {
            // Suffix is shorter than pattern, cannot be a match.
            return 0;
        }
        if (text_storage_.compare(current_sa_idx, pattern_len, pattern) != 0) {
            // Suffix does not start with the pattern.
            return 0;
        }

        // Find the first suffix that is > pattern (upper_bound)
        // Lambda returns true if pattern < suffix
        auto last_it = std::upper_bound(sa_.begin(), sa_.end(), pattern,
            [&](const std::string& p_param, int suffix_idx_param) {
                size_t len_to_compare_suffix = std::min((size_t)pattern_len, text_len_ - suffix_idx_param);
                // Compare pattern (p_param) with suffix starting at suffix_idx_param
                return p_param.compare(0, pattern_len, text_storage_, suffix_idx_param, len_to_compare_suffix) < 0;
            });

        return std::distance(first_it, last_it);
    }

    size_t size() const {
        return text_len_;
    }

    bool empty() const {
        return text_len_ == 0;
    }

private:
    std::string text_storage_;      // Making a copy of the text
    const std::string* text_ptr_;   // Pointer to text (could be external if API changes)
                                    // For now, points to text_storage_
    std::vector<int> sa_;           // The suffix array: stores indices
    size_t text_len_;
};

#endif // SUFFIX_ARRAY_H

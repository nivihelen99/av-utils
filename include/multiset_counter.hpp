/**
 * @file multiset_counter.hpp
 * @brief Defines the multiset_counter class for counting occurrences of multisets.
 */
#pragma once

#include <vector>
#include <map>
#include <algorithm> // For std::sort
#include <initializer_list> // For std::initializer_list

namespace cpp_collections {

/**
 * @brief A counter for multisets (collections of items where order doesn't matter but multiplicity does).
 *
 * Similar to Python's `collections.Counter` but for multisets of items.
 * It canonicalizes input collections (e.g., by sorting) to ensure that
 * different orderings of the same multiset are treated as identical.
 *
 * @tparam T The type of items within the multisets.
 * @tparam Compare The comparison function object type for sorting items within a multiset
 *                 to establish a canonical representation. Defaults to `std::less<T>`.
 */
template <typename T, typename Compare = std::less<T>>
class multiset_counter {
public:
    using item_type = T; ///< The type of individual items in a multiset.
    using multiset_type = std::vector<T>; ///< The type used to represent a multiset of items.
    using key_type = multiset_type; ///< The canonical representation type used as a key in the internal map (a sorted vector).
    using mapped_type = int; ///< The type used for counting (frequency of each multiset).
    using value_type = std::pair<const key_type, mapped_type>; ///< Type returned by iterators (pair of canonical multiset and its count).
    using size_type = std::size_t; ///< Unsigned integer type for sizes.

private:
    std::map<key_type, mapped_type, std::less<key_type>> counts_; ///< Internal storage for multiset counts. Keys are canonical (sorted) vectors.
    Compare item_comparator_; ///< Comparator for sorting items within a multiset.

    /**
     * @brief Canonicalizes a multiset by sorting its items.
     * @param items The multiset (vector of items) to canonicalize.
     * @return A new vector containing the sorted items, representing the canonical form.
     */
    key_type canonicalize(const multiset_type& items) const {
        key_type canonical_items = items;
        std::sort(canonical_items.begin(), canonical_items.end(), item_comparator_);
        return canonical_items;
    }

    /**
     * @brief Canonicalizes a multiset (rvalue overload) by sorting its items in place.
     * @param items The multiset (vector of items) to canonicalize (will be moved from).
     * @return The input vector, now sorted, representing the canonical form.
     */
    key_type canonicalize(multiset_type&& items) const {
        std::sort(items.begin(), items.end(), item_comparator_);
        return std::move(items);
    }


public:
    /**
     * @brief Default constructor. Creates an empty multiset_counter.
     */
    multiset_counter() = default;

    /**
     * @brief Constructor with a custom item comparator.
     * @param comp The comparator to use for sorting items within multisets.
     */
    explicit multiset_counter(const Compare& comp) : item_comparator_(comp) {}

    /**
     * @brief Constructor from an initializer list of multisets.
     * Each multiset in the list is added to the counter.
     * Example: `multiset_counter<std::string> mc = {{{ "a", "b"}, {"b", "a"}}, {{ "c" }}};`
     * This would count `{a,b}` (canonical form) twice, and `{c}` once.
     * @param init_list An initializer_list where each element is a multiset (e.g., `std::vector<T>`).
     * @param comp Optional custom comparator for items.
     */
    multiset_counter(std::initializer_list<multiset_type> init_list, const Compare& comp = Compare{})
        : item_comparator_(comp) {
        for (const auto& items : init_list) {
            add(items);
        }
    }

    // Iterators
    using iterator = typename std::map<key_type, mapped_type, std::less<key_type>>::iterator; ///< Iterator type.
    using const_iterator = typename std::map<key_type, mapped_type, std::less<key_type>>::const_iterator; ///< Const iterator type.

    /** @brief Returns an iterator to the beginning of the counter. */
    iterator begin() { return counts_.begin(); }
    /** @brief Returns a const iterator to the beginning of the counter. */
    const_iterator begin() const { return counts_.begin(); }
    /** @brief Returns a const iterator to the beginning of the counter. */
    const_iterator cbegin() const { return counts_.cbegin(); }

    /** @brief Returns an iterator to the end of the counter. */
    iterator end() { return counts_.end(); }
    /** @brief Returns a const iterator to the end of the counter. */
    const_iterator end() const { return counts_.end(); }
    /** @brief Returns a const iterator to the end of the counter. */
    const_iterator cend() const { return counts_.cend(); }

    // Core methods

    /**
     * @brief Adds a multiset to the counter or increments its count.
     * The input `items` will be canonicalized (sorted) before lookup or insertion.
     * @param items The multiset of items (as `std::vector<T>`) to add/increment.
     * @param num The number of times to add this multiset (defaults to 1).
     *            If `num` is negative, it effectively subtracts (or decrements count).
     *            If the count becomes zero or less after addition/subtraction, the multiset is removed from the counter.
     */
    void add(const multiset_type& items, int num = 1) {
        if (num == 0) return;
        key_type key = canonicalize(items);
        counts_[key] += num;
        if (counts_[key] <= 0) {
            counts_.erase(key);
        }
    }

    // Overload for rvalue items
    void add(multiset_type&& items, int num = 1) {
        if (num == 0) return;
        key_type key = canonicalize(std::move(items));
        counts_[key] += num;
        if (counts_[key] <= 0) {
            counts_.erase(key);
        }
    }

    /**
     * @brief Returns the count of a given multiset.
     * @param items The multiset of items to count.
     * @return The count of the multiset, or 0 if not present.
     */
    int count(const multiset_type& items) const {
        key_type key = canonicalize(items);
        auto it = counts_.find(key);
        if (it != counts_.end()) {
            return it->second;
        }
        return 0;
    }

    /**
     * @brief Accesses the count of a multiset (const). Alias for count().
     * @param items The multiset of items.
     * @return The count of the multiset, or 0 if not present.
     */
    int operator[](const multiset_type& items) const {
        return count(items);
    }

    /**
     * @brief Returns the number of unique multisets stored in the counter.
     * @return The number of unique multisets.
     */
    size_type size() const noexcept {
        return counts_.size();
    }

    /**
     * @brief Checks if the counter is empty (contains no multisets).
     * @return True if empty, false otherwise.
     */
    bool empty() const noexcept {
        return counts_.empty();
    }

    /**
     * @brief Removes all multisets and their counts from the counter.
     */
    void clear() noexcept {
        counts_.clear();
    }

    /**
     * @brief Checks if the counter contains a specific multiset.
     * @param items The multiset of items to check for.
     * @return True if the multiset has a count greater than 0, false otherwise.
     */
    bool contains(const multiset_type& items) const {
        return count(items) > 0;
    }

    // Additional utility methods

    /**
     * @brief Returns the sum of all counts in the counter.
     * This represents the total number of multisets added (respecting their multiplicities).
     * @return The total sum of counts.
     */
    int total() const noexcept {
        int total_sum = 0;
        for (const auto& pair : counts_) {
            total_sum += pair.second;
        }
        return total_sum;
    }

    /**
     * @brief Returns a vector of the n most common multisets and their counts.
     * If n is 0 or greater than or equal to the number of unique multisets,
     * all multisets are returned, sorted by count (most common first).
     * Ties in counts are broken by the natural sort order of the canonical multiset representation.
     * @param n The number of most common multisets to return.
     * @return A vector of `std::pair<key_type, mapped_type>`, where `key_type` is `std::vector<T>`.
     */
    std::vector<std::pair<key_type, mapped_type>> most_common(size_type n = 0) const {
        std::vector<std::pair<key_type, mapped_type>> sorted_items;
        sorted_items.reserve(counts_.size());
        for (const auto& pair : counts_) {
            sorted_items.push_back({pair.first, pair.second}); // Copy construct key and value
        }

        std::sort(sorted_items.begin(), sorted_items.end(),
                  [](const std::pair<key_type, mapped_type>& a, const std::pair<key_type, mapped_type>& b) {
                      if (a.second != b.second) {
                          return a.second > b.second; // Sort by count descending
                      }
                      // Tie-breaking: sort by key ascending (std::vector comparison)
                      return a.first < b.first;
                  });

        if (n > 0 && n < sorted_items.size()) {
            sorted_items.resize(n);
        }
        return sorted_items;
    }

    // Overloads for add and count to accept various container types

    /**
     * @brief Generic add overload for any container convertible to std::vector<T>.
     * @tparam Container Type of the input container (e.g., std::list<T>, std::multiset<T>).
     * @param items_container The container of items representing the multiset.
     * @param num The number of times to add this multiset.
     */
    template <typename Container>
    void add(const Container& items_container, int num = 1) {
        if (num == 0) return;
        // Convert to multiset_type (std::vector<T>) for canonicalization
        multiset_type items_vec(std::begin(items_container), std::end(items_container));
        add(std::move(items_vec), num); // Call the primary add method (rvalue overload)
    }

    /**
     * @brief Generic count overload for any container convertible to std::vector<T>.
     * @tparam Container Type of the input container.
     * @param items_container The container of items representing the multiset.
     * @return The count of the multiset.
     */
    template <typename Container>
    int count(const Container& items_container) const {
        multiset_type items_vec(std::begin(items_container), std::end(items_container));
        return count(items_vec); // Call the primary count method
    }

    /**
     * @brief Generic operator[] const overload for any container.
     */
    template <typename Container>
    int operator[](const Container& items_container) const {
        return count(items_container);
    }

};

} // namespace cpp_collections

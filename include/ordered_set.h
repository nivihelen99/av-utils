#ifndef ORDERED_SET_HPP
#define ORDERED_SET_HPP

#include <list>
#include <unordered_map>
#include <vector>
#include <initializer_list>
#include <iterator> // For std::reverse_iterator, etc.
#include <stdexcept> // For std::out_of_range
#include <algorithm> // For std::equal, std::copy
#include <utility>   // For std::move

/**
 * @brief An ordered set that preserves insertion order while maintaining uniqueness of elements.
 *
 * OrderedSet combines the properties of a std::list (to maintain insertion order)
 * and a std::unordered_map (to ensure uniqueness and provide fast lookups).
 *
 * @tparam T The type of elements to be stored. T must be Hashable and EqualityComparable.
 * @tparam Hash The hash function for T. Defaults to std::hash<T>.
 * @tparam KeyEqual The equality comparison function for T. Defaults to std::equal_to<T>.
 *
 * @note Iterators:
 * Non-const iterators (begin(), end(), rbegin(), rend()) allow modification of elements
 * if T itself is mutable. However, users must exercise extreme caution:
 * Modifying an element in a way that changes its hash value or its equality comparison
 * result with other elements will break the invariants of the OrderedSet and lead to
 * undefined behavior, as the underlying std::unordered_map will become corrupted.
 * It is generally recommended to treat elements as immutable once inserted, or to
 * erase and re-insert elements if modifications are needed that would affect hash/equality.
 * Methods like front() and back() return const references to prevent accidental modification.
 */
template <typename T,
          typename Hash = std::hash<T>,
          typename KeyEqual = std::equal_to<T>>
class OrderedSet {
private:
    using list_type = std::list<T>;
    using list_iterator = typename list_type::iterator;
    using const_list_iterator = typename list_type::const_iterator;

    list_type ordered_elements_;
    std::unordered_map<T, list_iterator, Hash, KeyEqual> element_index_;

public:
    // Type aliases
    using key_type = T;
    using value_type = T;
    using hasher = Hash;
    using key_equal = KeyEqual;
    // Note: 'reference' is typically T&. For key-based containers like this,
    // direct mutable access to elements (which are keys) via methods like front()/back()
    // can be dangerous as it might break map invariants if the key's hash/equality changes.
    // std::set::iterator are const_iterators.
    // We will ensure front()/back() return const_reference.
    using reference = const value_type&; // Changed to const value_type& for safety
    using const_reference = const value_type&;
    using pointer = value_type*; // Or const value_type*? Usually T*
    using const_pointer = const value_type*;
    using iterator = typename list_type::iterator;
    using const_iterator = typename list_type::const_iterator;
    using reverse_iterator = typename list_type::reverse_iterator;
    using const_reverse_iterator = typename list_type::const_reverse_iterator;
    using size_type = typename list_type::size_type;
    using difference_type = typename list_type::difference_type;

public:
    // Constructors
    OrderedSet() = default;

    OrderedSet(std::initializer_list<value_type> ilist) {
        for (const auto& item : ilist) {
            insert(item);
        }
    }

    OrderedSet(const OrderedSet& other) {
        // We cannot simply copy the list and map because the map's iterators
        // would point to the 'other' list's nodes, not our new list's nodes.
        // We must rebuild the map by iterating through the copied list.
        ordered_elements_ = other.ordered_elements_;
        for (auto it = ordered_elements_.begin(); it != ordered_elements_.end(); ++it) {
            element_index_[*it] = it;
        }
    }

    OrderedSet(OrderedSet&& other) noexcept
        : ordered_elements_(std::move(other.ordered_elements_)),
          element_index_(std::move(other.element_index_)) {
        // other is left in a valid but unspecified state (empty, typically)
    }

    OrderedSet& operator=(const OrderedSet& other) {
        if (this == &other) {
            return *this;
        }
        // Similar to copy constructor, need to rebuild.
        // A common approach: copy-and-swap or clear and rebuild.
        clear(); // Clear current state
        ordered_elements_ = other.ordered_elements_;
        for (auto it = ordered_elements_.begin(); it != ordered_elements_.end(); ++it) {
            element_index_[*it] = it;
        }
        return *this;
    }

    OrderedSet& operator=(OrderedSet&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        clear(); // Ensure current resources are released if any
        ordered_elements_ = std::move(other.ordered_elements_);
        element_index_ = std::move(other.element_index_);
        // other is left in a valid but unspecified state (empty, typically)
        return *this;
    }


    // Core operations
    std::pair<iterator, bool> insert(const value_type& value) {
        if (element_index_.count(value)) {
            return {element_index_.at(value), false};
        }
        // Strong exception safety:
        // 1. Add to list.
        // 2. Try to add to map.
        // 3. If map fails, remove from list.
        ordered_elements_.push_back(value); // Step 1
        list_iterator it = std::prev(ordered_elements_.end());
        try {
            element_index_.emplace(*it, it); // Step 2: Use *it as key, as value might be different if T is complex
                                          // emplace can be more efficient too.
        } catch (...) {
            ordered_elements_.pop_back(); // Step 3: Rollback
            throw;
        }
        return {it, true};
    }

    std::pair<iterator, bool> insert(value_type&& value) {
        // Need to check existence using 'value' before it's moved.
        auto map_find_it = element_index_.find(value);
        if (map_find_it != element_index_.end()) {
            return {map_find_it->second, false};
        }

        // Strong exception safety for move insert:
        ordered_elements_.push_back(std::move(value)); // value is moved into list
        list_iterator list_it = std::prev(ordered_elements_.end());
        try {
            // Key for the map is taken from the element now in the list (*list_it)
            element_index_.emplace(*list_it, list_it);
        } catch (...) {
            ordered_elements_.pop_back(); // Rollback list modification
            // Note: 'value' is already moved-from at this point. Nothing to do for 'value' itself.
            throw;
        }
        return {list_it, true};
    }

    size_type erase(const key_type& key) {
        auto it = element_index_.find(key);
        if (it != element_index_.end()) {
            ordered_elements_.erase(it->second);
            element_index_.erase(it);
            return 1;
        }
        return 0;
    }

    bool contains(const key_type& key) const {
        return element_index_.count(key);
    }

    void clear() noexcept {
        ordered_elements_.clear();
        element_index_.clear();
    }

    size_type size() const noexcept {
        return element_index_.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return element_index_.empty();
    }

    // Iterators
    iterator begin() noexcept {
        return ordered_elements_.begin();
    }

    const_iterator begin() const noexcept {
        return ordered_elements_.cbegin();
    }

    const_iterator cbegin() const noexcept {
        return ordered_elements_.cbegin();
    }

    iterator end() noexcept {
        return ordered_elements_.end();
    }

    const_iterator end() const noexcept {
        return ordered_elements_.cend();
    }

    const_iterator cend() const noexcept {
        return ordered_elements_.cend();
    }

    reverse_iterator rbegin() noexcept {
        return ordered_elements_.rbegin();
    }

    const_reverse_iterator rbegin() const noexcept {
        return ordered_elements_.crbegin();
    }

    const_reverse_iterator crbegin() const noexcept {
        return ordered_elements_.crbegin();
    }

    reverse_iterator rend() noexcept {
        return ordered_elements_.rend();
    }

    const_reverse_iterator rend() const noexcept {
        return ordered_elements_.crend();
    }

    const_reverse_iterator crend() const noexcept {
        return ordered_elements_.crend();
    }

    // Element access
    reference front() {
        if (empty()) {
            throw std::out_of_range("front() called on empty OrderedSet");
        }
        return ordered_elements_.front();
    }

    const_reference front() const {
        if (empty()) {
            throw std::out_of_range("front() called on empty OrderedSet");
        }
        return ordered_elements_.front();
    }

    reference back() {
        if (empty()) {
            throw std::out_of_range("back() called on empty OrderedSet");
        }
        return ordered_elements_.back();
    }

    const_reference back() const {
        if (empty()) {
            throw std::out_of_range("back() called on empty OrderedSet");
        }
        return ordered_elements_.back();
    }

    // Utilities
    std::vector<value_type> as_vector() const {
        return std::vector<value_type>(ordered_elements_.begin(), ordered_elements_.end());
    }

    // Optional APIs (Merge)
    void merge(const OrderedSet& other) {
        for (const auto& item : other) {
            insert(item); // insert handles uniqueness and order
        }
    }

    void merge(OrderedSet&& other) {
        // Optimized merge for rvalue reference:
        // Iterate through the 'other' set. For each element, try to insert it.
        // If an element from 'other' is successfully inserted (i.e., it wasn't already present),
        // it's moved from 'other.ordered_elements_'.
        // This is more complex than just iterating and calling insert(std::move(item))
        // because 'item' in 'for (auto&& item : other)' would be a reference to an element
        // within 'other.ordered_elements_'. Moving it directly would invalidate 'other's structure
        // during iteration.
        // A simpler approach for rvalue merge is to iterate and insert (copying if not present),
        // then clear 'other'. If we want to move elements, we'd have to extract them carefully.

        // Simpler approach: iterate and insert (copy/move handled by our insert methods)
        for (auto&& item : other) { // item is T& or T&& depending on other's iterator
            insert(std::forward<value_type>(item)); // Forward to appropriate insert
        }
        other.clear(); // As 'other' was passed by rvalue, we can clear it.
                       // This isn't strictly "moving" elements in O(1) from other to this,
                       // but rather efficiently inserting and then discarding 'other'.
    }

    // Optional APIs (Comparison)
    bool operator==(const OrderedSet& other) const {
        if (size() != other.size()) {
            return false;
        }
        // std::equal requires both sequences to have the same number of elements,
        // which is checked by size().
        return std::equal(ordered_elements_.begin(), ordered_elements_.end(), other.ordered_elements_.begin());
    }

    bool operator!=(const OrderedSet& other) const {
        return !(*this == other);
    }

};

// Non-member comparison operators (if needed, but typically member operators are sufficient for ==/!=)
// template <typename T, typename Hash, typename KeyEqual>
// bool operator==(const OrderedSet<T, Hash, KeyEqual>& lhs, const OrderedSet<T, Hash, KeyEqual>& rhs) {
//     if (lhs.size() != rhs.size()) {
//         return false;
//     }
//     auto it_lhs = lhs.begin();
//     auto it_rhs = rhs.begin();
//     while (it_lhs != lhs.end()) {
//         if (!(*it_lhs == *it_rhs)) { // Relies on T::operator==
//             return false;
//         }
//         ++it_lhs;
//         ++it_rhs;
//     }
//     return true;
// }

// template <typename T, typename Hash, typename KeyEqual>
// bool operator!=(const OrderedSet<T, Hash, KeyEqual>& lhs, const OrderedSet<T, Hash, KeyEqual>& rhs) {
//     return !(lhs == rhs);
// }

#endif // ORDERED_SET_HPP

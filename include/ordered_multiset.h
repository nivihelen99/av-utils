#pragma once

#include <list>
#include <vector>
#include <unordered_map>
#include <functional> // For std::hash, std::equal_to
#include <iterator>   // For std::reverse_iterator
#include <initializer_list>
#include <utility> // For std::move, std::forward

namespace cpp_utils {

template <
    typename T,
    typename Hash = std::hash<T>,
    typename KeyEqual = std::equal_to<T>
>
class OrderedMultiset {
public:
    // Type aliases
    using key_type = T;
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using hasher = Hash;
    using key_equal = KeyEqual;

private:
    using list_type = std::list<value_type>;
    using list_iterator_type = typename list_type::iterator;
    using const_list_iterator_type = typename list_type::const_iterator;

public:
    using reference = typename list_type::const_reference; // Elements should not be modifiable in place to change hash/equality
    using const_reference = typename list_type::const_reference;
    // Iterators iterate over elements_in_order_
    using iterator = typename list_type::iterator;
    using const_iterator = typename list_type::const_iterator;
    using reverse_iterator = typename list_type::reverse_iterator;
    using const_reverse_iterator = typename list_type::const_reverse_iterator;

private:
    list_type elements_in_order_;
    std::unordered_map<key_type, std::vector<list_iterator_type>, hasher, key_equal> element_positions_;

public:
    // --- Constructors ---
    OrderedMultiset() = default;

    OrderedMultiset(std::initializer_list<value_type> ilist) {
        for (const auto& value : ilist) {
            insert(value); // Relies on insert being implemented later
        }
    }

    OrderedMultiset(const OrderedMultiset& other) {
        // Deep copy elements_in_order_ first
        elements_in_order_ = other.elements_in_order_;
        // Rebuild element_positions_ because iterators from 'other' are invalid for 'this' list
        rebuild_element_positions();
    }

    OrderedMultiset(OrderedMultiset&& other) noexcept
        : elements_in_order_(std::move(other.elements_in_order_)),
          element_positions_(std::move(other.element_positions_)) {
        // 'other' is left in a valid but unspecified (likely empty) state.
    }

    OrderedMultiset& operator=(const OrderedMultiset& other) {
        if (this == &other) {
            return *this;
        }
        elements_in_order_ = other.elements_in_order_;
        // Clear and rebuild element_positions_
        element_positions_.clear();
        rebuild_element_positions();
        return *this;
    }

    OrderedMultiset& operator=(OrderedMultiset&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        clear(); // Clear current resources
        elements_in_order_ = std::move(other.elements_in_order_);
        element_positions_ = std::move(other.element_positions_);
        return *this;
    }

    // --- Capacity ---
    [[nodiscard]] bool empty() const noexcept {
        return elements_in_order_.empty();
    }

    size_type size() const noexcept {
        return elements_in_order_.size();
    }

    void clear() noexcept {
        elements_in_order_.clear();
        element_positions_.clear();
    }

    // --- Modifiers ---
    std::pair<iterator, bool> insert(const value_type& value) {
        elements_in_order_.push_back(value);
        list_iterator_type list_it = std::prev(elements_in_order_.end());
        element_positions_[value].push_back(list_it);
        return {list_it, true}; // bool indicates success, always true for multiset insert
    }

    std::pair<iterator, bool> insert(value_type&& value) {
        elements_in_order_.push_back(std::move(value));
        list_iterator_type list_it = std::prev(elements_in_order_.end());
        // Need to use the value from the list for map key if 'value' is moved from and potentially altered
        element_positions_[*list_it].push_back(list_it);
        return {list_it, true};
    }

    size_type erase(const key_type& key) {
        auto map_it = element_positions_.find(key);
        if (map_it == element_positions_.end() || map_it->second.empty()) {
            return 0; // Key not found or no iterators associated with it
        }

        // Get the iterator to the last occurrence of this key in the list
        list_iterator_type list_it_to_remove = map_it->second.back();

        // Erase from the list
        elements_in_order_.erase(list_it_to_remove);

        // Remove from the vector of iterators in the map
        map_it->second.pop_back();

        // If this was the last occurrence, remove the key from the map entirely
        if (map_it->second.empty()) {
            element_positions_.erase(map_it);
        }

        return 1;
    }

    size_type erase_all(const key_type& key) {
        auto map_it = element_positions_.find(key);
        if (map_it == element_positions_.end() || map_it->second.empty()) {
            return 0; // Key not found
        }

        size_type num_removed = 0;
        std::vector<list_iterator_type>& iterators_to_remove = map_it->second;

        for (list_iterator_type list_it : iterators_to_remove) {
            elements_in_order_.erase(list_it);
            num_removed++;
        }

        element_positions_.erase(map_it); // Remove the key from the map
        return num_removed;
    }

    // --- Lookup ---
    size_type count(const key_type& key) const {
        auto map_it = element_positions_.find(key);
        if (map_it == element_positions_.end()) {
            return 0;
        }
        return map_it->second.size();
    }

    bool contains(const key_type& key) const {
        return element_positions_.count(key) > 0;
        // Alternative: return count(key) > 0; but direct map check is slightly more efficient
    }

    // --- Swap ---
    void swap(OrderedMultiset& other) noexcept {
        using std::swap;
        swap(elements_in_order_, other.elements_in_order_);
        swap(element_positions_, other.element_positions_);
    }

private:
    // Helper function to rebuild element_positions_ from elements_in_order_
    // Used in copy constructor and copy assignment operator
    void rebuild_element_positions() {
        element_positions_.clear();
        for (auto it = elements_in_order_.begin(); it != elements_in_order_.end(); ++it) {
            element_positions_[*it].push_back(it);
        }
    }

public:
    // --- Iterators ---
    iterator begin() noexcept {
        return elements_in_order_.begin();
    }

    const_iterator begin() const noexcept {
        return elements_in_order_.cbegin();
    }

    const_iterator cbegin() const noexcept {
        return elements_in_order_.cbegin();
    }

    iterator end() noexcept {
        return elements_in_order_.end();
    }

    const_iterator end() const noexcept {
        return elements_in_order_.cend();
    }

    const_iterator cend() const noexcept {
        return elements_in_order_.cend();
    }

    reverse_iterator rbegin() noexcept {
        return elements_in_order_.rbegin();
    }

    const_reverse_iterator rbegin() const noexcept {
        return elements_in_order_.crbegin();
    }

    const_reverse_iterator crbegin() const noexcept {
        return elements_in_order_.crbegin();
    }

    reverse_iterator rend() noexcept {
        return elements_in_order_.rend();
    }

    const_reverse_iterator rend() const noexcept {
        return elements_in_order_.crend();
    }

    const_reverse_iterator crend() const noexcept {
        return elements_in_order_.crend();
    }

    // --- Comparison Operators ---
    bool operator==(const OrderedMultiset& other) const {
        // If sizes are different, they can't be equal
        if (elements_in_order_.size() != other.elements_in_order_.size()) {
            return false;
        }
        // Compare the lists directly; std::list::operator== checks for
        // element-wise equality in sequence.
        return elements_in_order_ == other.elements_in_order_;
    }

    bool operator!=(const OrderedMultiset& other) const {
        return !(*this == other);
    }

}; // class OrderedMultiset

// Non-member swap function already defined
template <typename T, typename Hash, typename KeyEqual>
void swap(OrderedMultiset<T, Hash, KeyEqual>& lhs, OrderedMultiset<T, Hash, KeyEqual>& rhs) noexcept {
    lhs.swap(rhs);
}

} // namespace cpp_utils

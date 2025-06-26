#pragma once

#include <vector>
#include <algorithm> // For std::sort, std::unique, std::lower_bound
#include <functional>  // For std::less, std::hash
#include <memory>      // For std::allocator
#include <initializer_list> // For std::initializer_list
#include <stdexcept>   // For std::out_of_range (though not strictly needed for FrozenSet's typical API)
#include <iterator>    // For std::iterator_traits

namespace cpp_collections {

template <
    typename Key,
    typename Compare = std::less<Key>,
    typename Allocator = std::allocator<Key>
>
class FrozenSet {
public:
    // Types
    using key_type = Key;
    using value_type = Key; // Set stores keys as values
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using key_compare = Compare;
    using value_compare = Compare;
    using allocator_type = Allocator;
    using reference = const Key&; // Always const reference
    using const_reference = const Key&;
    using pointer = typename std::allocator_traits<Allocator>::const_pointer; // Always const pointer
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

    // Use std::vector's const_iterator as FrozenSet's iterator
    using iterator = typename std::vector<Key, Allocator>::const_iterator;
    using const_iterator = typename std::vector<Key, Allocator>::const_iterator;

private:
    std::vector<Key, Allocator> data_;
    Compare comparator_;

    // Helper to sort and unique a vector
    template<typename InputIt>
    void build_from_range(InputIt first, InputIt last) {
        data_.assign(first, last);
        std::sort(data_.begin(), data_.end(), comparator_);
        data_.erase(std::unique(data_.begin(), data_.end(),
                                [this](const Key& a, const Key& b){ return !comparator_(a,b) && !comparator_(b,a); }),
                    data_.end());
        data_.shrink_to_fit(); // Minimize capacity as it won't change
    }

public:
    // Constructors
    FrozenSet(const Allocator& alloc = Allocator())
        : data_(alloc), comparator_(Compare()) {}

    FrozenSet(const Compare& comp, const Allocator& alloc = Allocator())
        : data_(alloc), comparator_(comp) {}

    template <typename InputIt>
    FrozenSet(InputIt first, InputIt last,
              const Compare& comp = Compare(),
              const Allocator& alloc = Allocator())
        : data_(alloc), comparator_(comp) {
        build_from_range(first, last);
    }

    FrozenSet(std::initializer_list<Key> ilist,
              const Compare& comp = Compare(),
              const Allocator& alloc = Allocator())
        : data_(alloc), comparator_(comp) {
        build_from_range(ilist.begin(), ilist.end());
    }

    // Copy constructor
    FrozenSet(const FrozenSet& other) = default;
    FrozenSet(const FrozenSet& other, const Allocator& alloc)
        : data_(other.data_, alloc), comparator_(other.comparator_) {}

    // Move constructor
    FrozenSet(FrozenSet&& other) noexcept = default;
    FrozenSet(FrozenSet&& other, const Allocator& alloc) noexcept
        : data_(std::move(other.data_), alloc), comparator_(std::move(other.comparator_)) {}

    // Destructor
    ~FrozenSet() = default;

    // Assignment operators (though less common for immutable types, can be useful for reassignment)
    // However, true immutability would mean no assignment after construction.
    // For now, let's stick to "frozen after construction" meaning the object itself can be reassigned.
    FrozenSet& operator=(const FrozenSet& other) = default;
    FrozenSet& operator=(FrozenSet&& other) noexcept = default;
    FrozenSet& operator=(std::initializer_list<Key> ilist) {
        build_from_range(ilist.begin(), ilist.end());
        return *this;
    }

    // Iterators
    iterator begin() const noexcept { return data_.begin(); }
    const_iterator cbegin() const noexcept { return data_.cbegin(); }
    iterator end() const noexcept { return data_.end(); }
    const_iterator cend() const noexcept { return data_.cend(); }

    // Capacity
    bool empty() const noexcept { return data_.empty(); }
    size_type size() const noexcept { return data_.size(); }
    size_type max_size() const noexcept { return data_.max_size(); }

    // Lookup
    size_type count(const Key& key) const {
        auto it = std::lower_bound(data_.begin(), data_.end(), key, comparator_);
        if (it != data_.end() && !comparator_(*it, key) && !comparator_(key, *it)) { // Found and equivalent
            return 1;
        }
        return 0;
    }

    bool contains(const Key& key) const {
        return count(key) > 0;
    }

    iterator find(const Key& key) const {
        auto it = std::lower_bound(data_.begin(), data_.end(), key, comparator_);
        if (it != data_.end() && !comparator_(*it, key) && !comparator_(key, *it)) { // Found and equivalent
            return it;
        }
        return data_.end();
    }

    // Comparators
    key_compare key_comp() const { return comparator_; }
    value_compare value_comp() const { return comparator_; } // Same as key_comp for sets

    // Allocator
    allocator_type get_allocator() const noexcept { return data_.get_allocator(); }

    // Comparison operators for FrozenSet objects
    friend bool operator==(const FrozenSet& lhs, const FrozenSet& rhs) {
        if (lhs.size() != rhs.size()) return false;
        // Relies on data_ being sorted and unique, and using the same comparator logic for equality.
        // std::equal uses lhs.comparator_ for element comparison by default if available.
        // However, we need to ensure both sets consider elements equal in the same way.
        // The most robust way is to check if their internal vectors are equal.
        return lhs.data_ == rhs.data_;
    }

    friend bool operator!=(const FrozenSet& lhs, const FrozenSet& rhs) {
        return !(lhs == rhs);
    }

    friend bool operator<(const FrozenSet& lhs, const FrozenSet& rhs) {
        // Lexicographical comparison
        return std::lexicographical_compare(lhs.begin(), lhs.end(),
                                            rhs.begin(), rhs.end(),
                                            lhs.comparator_);
    }

    friend bool operator<=(const FrozenSet& lhs, const FrozenSet& rhs) {
        return !(rhs < lhs);
    }

    friend bool operator>(const FrozenSet& lhs, const FrozenSet& rhs) {
        return rhs < lhs;
    }

    friend bool operator>=(const FrozenSet& lhs, const FrozenSet& rhs) {
        return !(lhs < rhs);
    }

    // Swap (member swap is often good practice, though less critical for immutable types if copy/move assign are deleted)
    void swap(FrozenSet& other) noexcept(
        std::allocator_traits<Allocator>::propagate_on_container_swap::value ||
        std::allocator_traits<Allocator>::is_always_equal::value) {
        std::swap(data_, other.data_);
        std::swap(comparator_, other.comparator_);
    }
};

// Non-member swap
template <typename Key, typename Compare, typename Allocator>
void swap(FrozenSet<Key, Compare, Allocator>& lhs,
          FrozenSet<Key, Compare, Allocator>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace cpp_collections

// Specialization of std::hash for FrozenSet
namespace std {
template <typename Key, typename Compare, typename Allocator>
struct hash<cpp_collections::FrozenSet<Key, Compare, Allocator>> {
    std::size_t operator()(const cpp_collections::FrozenSet<Key, Compare, Allocator>& fs) const {
        std::size_t seed = 0;
        // Combine hashes of elements. Order matters as the set is ordered.
        // A common way to combine hashes:
        std::hash<Key> hasher;
        for (const auto& elem : fs) {
            seed ^= hasher(elem) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};
} // namespace std

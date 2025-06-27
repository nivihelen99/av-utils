#pragma once

#include <vector>
#include <utility>      // For std::pair, std::move
#include <algorithm>    // For std::sort, std::lower_bound, std::unique
#include <stdexcept>    // For std::out_of_range, std::invalid_argument
#include <functional>   // For std::hash, std::equal_to, std::less
#include <memory>       // For std::allocator, std::allocator_traits
#include <initializer_list> // For std::initializer_list
#include <map>          // For std::map used in build_from_range

namespace cpp_collections {

// Forward declaration for std::hash specialization
template <
    typename Key,
    typename Value,
    typename HashFunc,
    typename KeyEqualFunc,
    typename CompareFunc, // Added for sorting keys
    typename Allocator
>
class FrozenDict;

} // namespace cpp_collections

namespace std {
// Forward declaration of std::hash specialization for FrozenDict
template <
    typename Key,
    typename Value,
    typename HashFunc,
    typename KeyEqualFunc,
    typename CompareFunc,
    typename Allocator
>
struct hash<cpp_collections::FrozenDict<Key, Value, HashFunc, KeyEqualFunc, CompareFunc, Allocator>>;
} // namespace std


namespace cpp_collections {

template <
    typename Key,
    typename Value,
    typename HashFunc = std::hash<Key>,
    typename KeyEqualFunc = std::equal_to<Key>,
    typename CompareFunc = std::less<Key>, // Used for sorting keys internally
    typename Allocator = std::allocator<std::pair<const Key, Value>>
>
class FrozenDict {
public:
    // Types
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>; // The type stored in the vector
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using hasher = HashFunc;
    using key_equal = KeyEqualFunc;
    using key_compare = CompareFunc; // For sorting keys
    using allocator_type = Allocator;

    // References to elements are const because the FrozenDict is immutable
    using reference = const value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::const_pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

private:
    // Internal storage: a sorted vector of key-value pairs
    using internal_storage_type = std::vector<value_type, Allocator>;
    internal_storage_type data_;
    key_compare key_comparator_; // Instance of the key comparator
    hasher key_hasher_;          // Instance of the key hasher

    // Helper to sort and unique data during construction.
    // Adopts "last one wins" policy for duplicate keys in the input range.
    template<typename InputIt>
    void build_from_range(InputIt first, InputIt last) {
        // Use a temporary map to handle "last one wins" for duplicate keys.
        // The map uses the FrozenDict's key_comparator_ for ordering keys.
        std::map<key_type, mapped_type, key_compare> temp_map(key_comparator_);
        for (InputIt it = first; it != last; ++it) {
            // Using insert_or_assign to achieve "last one wins" and avoid
            // default construction of Value if key is new, then assignment.
            // emplace could also work if we handle the return value to update if key exists.
            // insert_or_assign is cleaner here for "last wins".
            temp_map.insert_or_assign(it->first, it->second);
        }

        // Reserve space in data_ and copy from the ordered temp_map.
        data_.reserve(temp_map.size());
        for (const auto& pair : temp_map) {
            // Emplace directly, as key is const in value_type
            data_.emplace_back(pair.first, pair.second);
        }
        // data_ is now sorted by key and contains unique keys due to std::map properties.
        data_.shrink_to_fit(); // Optional: minimize capacity.
    }

public:
    // Iterators (const only, as FrozenDict is immutable)
    using iterator = typename internal_storage_type::const_iterator;
    using const_iterator = typename internal_storage_type::const_iterator;

    // Constructors
    FrozenDict(const Allocator& alloc = Allocator())
        : data_(alloc), key_comparator_(CompareFunc()), key_hasher_(HashFunc()) {}

    FrozenDict(const key_compare& comp, const Allocator& alloc = Allocator())
        : data_(alloc), key_comparator_(comp), key_hasher_(HashFunc()) {}

    FrozenDict(const key_compare& comp, const hasher& hash_fn, const Allocator& alloc = Allocator())
        : data_(alloc), key_comparator_(comp), key_hasher_(hash_fn) {}

    template <typename InputIt>
    FrozenDict(InputIt first, InputIt last,
               const key_compare& comp = CompareFunc(),
               const hasher& hash_fn = HashFunc(),
               const Allocator& alloc = Allocator())
        : data_(alloc), key_comparator_(comp), key_hasher_(hash_fn) {
        build_from_range(first, last);
    }

    FrozenDict(std::initializer_list<value_type> ilist,
               const key_compare& comp = CompareFunc(),
               const hasher& hash_fn = HashFunc(),
               const Allocator& alloc = Allocator())
        : data_(alloc), key_comparator_(comp), key_hasher_(hash_fn) {
        build_from_range(ilist.begin(), ilist.end());
    }

    // Copy constructor
    FrozenDict(const FrozenDict& other) = default; // Will copy key_hasher_ too
    FrozenDict(const FrozenDict& other, const Allocator& alloc)
        : data_(other.data_, alloc),
          key_comparator_(other.key_comparator_),
          key_hasher_(other.key_hasher_) {}

    // Move constructor
    FrozenDict(FrozenDict&& other) noexcept = default; // Will move key_hasher_ too
    FrozenDict(FrozenDict&& other, const Allocator& alloc) noexcept
        : data_(std::move(other.data_), alloc),
          key_comparator_(std::move(other.key_comparator_)),
          key_hasher_(std::move(other.key_hasher_)) {}

    // Destructor
    ~FrozenDict() = default;

    // Assignment operators (though less common for immutable types, can be useful for reassignment)
    // For true immutability, these would be deleted.
    // For now, let's assume "frozen after construction" means the object itself can be reassigned
    // to a new FrozenDict instance.
    FrozenDict& operator=(const FrozenDict& other) = default;
    FrozenDict& operator=(FrozenDict&& other) noexcept = default;

    FrozenDict& operator=(std::initializer_list<value_type> ilist) {
        // Rebuild from the initializer list
        // Need to ensure key_comparator_ is handled if it was set differently.
        // For simplicity, assume it remains.
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
    const_iterator find(const key_type& key) const {
        // Use a lambda for std::lower_bound with value_type
        auto it = std::lower_bound(data_.begin(), data_.end(), key,
            [this](const value_type& element, const key_type& k_val) {
                return key_comparator_(element.first, k_val);
            });
        // Check if found and key is actually equal (not just first element not less than key)
        if (it != data_.end() && !key_comparator_(key, it->first)) { // key >= it->first implies it->first <= key.
                                                                      // If also !(it->first < key), then key == it->first
            return it;
        }
        return data_.end();
    }

    size_type count(const key_type& key) const {
        return (find(key) != data_.end()) ? 1 : 0;
    }

    bool contains(const key_type& key) const {
        return count(key) > 0;
    }

    const mapped_type& at(const key_type& key) const {
        const_iterator it = find(key);
        if (it == data_.end()) {
            throw std::out_of_range("FrozenDict::at: key not found");
        }
        return it->second;
    }

    // operator[] for const access, throws if key not found (consistent with at())
    const mapped_type& operator[](const key_type& key) const {
        return at(key);
    }

    // Comparators
    key_compare key_comp() const { return key_comparator_; }
    // value_compare is not typical for maps, but if needed, it would compare pairs.

    // Allocator
    allocator_type get_allocator() const noexcept { return data_.get_allocator(); }

    // Comparison operators for FrozenDict objects
    friend bool operator==(const FrozenDict& lhs, const FrozenDict& rhs) {
        // Relies on data_ being sorted and unique by key.
        // And that key_comparator_ and value equality define overall FrozenDict equality.
        return lhs.data_ == rhs.data_; // std::vector::operator== does element-wise comparison
    }

    friend bool operator!=(const FrozenDict& lhs, const FrozenDict& rhs) {
        return !(lhs == rhs);
    }

    // Lexicographical comparison can be added if needed (operator<, etc.)
    // friend bool operator<(const FrozenDict& lhs, const FrozenDict& rhs) { ... }

    // Swap (member swap)
    void swap(FrozenDict& other) noexcept(
        std::allocator_traits<Allocator>::propagate_on_container_swap::value ||
        std::allocator_traits<Allocator>::is_always_equal::value) {
        using std::swap;
        swap(data_, other.data_);
        swap(key_comparator_, other.key_comparator_);
    }

    // Friend declaration for std::hash specialization
    friend struct std::hash<FrozenDict<Key, Value, HashFunc, KeyEqualFunc, CompareFunc, Allocator>>;

    // Helper method for std::hash to access the key_hasher_ instance
    // This is one way; another is to make std::hash a friend of this specific instantiation.
    const hasher& key_hasher_instance_for_std_hash() const {
        return key_hasher_;
    }
};

// Non-member swap
template <typename K, typename V, typename HF, typename KEF, typename CF, typename A>
void swap(FrozenDict<K, V, HF, KEF, CF, A>& lhs,
          FrozenDict<K, V, HF, KEF, CF, A>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace cpp_collections

// Specialization of std::hash for FrozenDict (definition will be in step 3)

namespace std {
template <typename Key, typename Value, typename HashFunc, typename KeyEqualFunc, typename CompareFunc, typename Allocator>
struct hash<cpp_collections::FrozenDict<Key, Value, HashFunc, KeyEqualFunc, CompareFunc, Allocator>> {
    std::size_t operator()(const cpp_collections::FrozenDict<Key, Value, HashFunc, KeyEqualFunc, CompareFunc, Allocator>& fd) const {
        // Use the HashFunc provided to FrozenDict for hashing keys,
        // and std::hash<Value> for hashing values.
        HashFunc key_hasher = fd.key_hasher_instance_for_std_hash(); // Need a way to get/construct this
        std::hash<Value> value_hasher;

        std::size_t seed = fd.data_.size(); // Start seed with size for empty dicts to have a consistent hash base

        for (const auto& pair : fd.data_) { // data_ is already sorted by key
            // Combine hash of key
            seed ^= key_hasher(pair.first) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            // Combine hash of value
            seed ^= value_hasher(pair.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};
} // namespace std


// Deduction guides (optional, for C++17+)
// Example:
// template<typename InputIt>
// FrozenDict(InputIt, InputIt) -> FrozenDict<typename std::iterator_traits<InputIt>::value_type::first_type,
//                                           typename std::iterator_traits<InputIt>::value_type::second_type>;
//
// template<typename Key, typename Value>
// FrozenDict(std::initializer_list<std::pair<const Key, Value>>) -> FrozenDict<Key, Value>;

// Note: Actual implementation of build_from_range for "last one wins" for duplicate keys
// would require more elaborate logic, e.g., by first putting elements into an unordered_map
// (which naturally handles last-one-wins) and then constructing the sorted vector from that map.
// Or, sorting a temporary vector of pair<original_index, value_type> to break ties.
// The current build_from_range will keep the first occurrence among equivalent keys after sorting.
// This is often acceptable for immutable structures where input is assumed to be well-formed
// or where "first wins" is the desired policy.

// If "last wins" is strictly needed for duplicate keys in input:
// Modify build_from_range:
// 1. Create a std::map<Key, Value, CompareFunc> (or unordered_map if order of insertion for duplicates doesn't matter before final sort).
// 2. Insert all pairs from [first, last) into this temporary map. This naturally handles "last one wins".
// 3. Create data_ from the contents of this temporary map. data_ will then be sorted.

// Example of "last-wins" for build_from_range:
/*
private:
    template<typename InputIt>
    void build_from_range_last_wins(InputIt first, InputIt last) {
        // Use a temporary map that respects the FrozenDict's key_comparator_ for ordering,
        // or an unordered_map if the order of insertion of duplicates doesn't matter before final sort.
        // For simplicity, let's use std::map which sorts by key.
        std::map<key_type, mapped_type, key_compare> temp_map(key_comparator_);
        for (InputIt it = first; it != last; ++it) {
            temp_map[it->first] = it->second; // operator[] ensures last one wins
        }
        // Reserve space in data_ and copy from temp_map
        data_.reserve(temp_map.size());
        for (const auto& pair : temp_map) {
            data_.emplace_back(pair.first, pair.second);
        }
        // data_ is already sorted by key due to std::map.
        // No duplicates by key due to std::map.
        data_.shrink_to_fit();
    }
// Then call this in constructors.
*/
// For now, the current build_from_range (keeps first unique key) will be used.
// This can be refined based on exact desired semantics for duplicate key handling during construction.

#endif // FROZEN_DICT_H

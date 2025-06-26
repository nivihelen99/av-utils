#pragma once

#include <functional> // For std::hash, std::equal_to
#include <utility>    // For std::pair
#include <unordered_map> // Underlying container for element counts

// Note: <vector> was removed as it's not directly used by this implementation.
// It might be re-added if custom iterators requiring it are developed later.

// The UnorderedMultiset class provides a hash-table based multiset,
// storing elements and their counts. It's similar in concept to
// std::unordered_multiset but implemented here potentially for specific
// library integration or API style.

namespace cpp_utils {

template <
    typename T,
    typename Hash = std::hash<T>,
    typename KeyEqual = std::equal_to<T>
>
class UnorderedMultiset {
public:
    using key_type = T;
    using value_type = T; // In a multiset, key and value are the same
    using hasher = Hash;
    using key_equal = KeyEqual;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    // For now, using a placeholder for iterator. This will be fleshed out.
    // It will likely iterate over unique elements, and count provides multiplicity.
    // Or, it could iterate over all elements including duplicates.
    // Let's define it to iterate over unique elements first.
    // The underlying map's iterator can serve this purpose.
    using underlying_container_type = std::unordered_map<T, size_type, Hash, KeyEqual>;
    using const_iterator = typename underlying_container_type::const_iterator; // Iterates unique keys
    // To iterate over all elements (including duplicates), a custom iterator would be needed.
    // This will be part of the implementation detail.

    // Constructors
    UnorderedMultiset() = default;
    explicit UnorderedMultiset(const Hash& hashfn, const KeyEqual& eq = KeyEqual())
        : map_(10, hashfn, eq) {} // Initial bucket count, hash, equal

    // Capacity
    bool empty() const noexcept {
        return total_elements_ == 0;
    }

    size_type size() const noexcept {
        return total_elements_;
    }

    // Modifiers
    void insert(const T& value) {
        map_[value]++;
        total_elements_++;
    }

    void insert(T&& value) {
        map_[std::move(value)]++;
        total_elements_++;
    }

    // Erases one instance of the value.
    // Returns the number of elements erased (0 or 1).
    size_type erase(const T& value) {
        auto it = map_.find(value);
        if (it != map_.end()) {
            if (it->second > 1) {
                it->second--;
            } else {
                map_.erase(it);
            }
            total_elements_--;
            return 1;
        }
        return 0;
    }

    // Erases all instances of the value.
    // Returns the number of elements erased.
    size_type erase_all(const T& value) {
        auto it = map_.find(value);
        if (it != map_.end()) {
            size_type num_erased = it->second;
            total_elements_ -= num_erased;
            map_.erase(it);
            return num_erased;
        }
        return 0;
    }

    void clear() noexcept {
        map_.clear();
        total_elements_ = 0;
    }

    void swap(UnorderedMultiset& other) noexcept {
        map_.swap(other.map_);
        std::swap(total_elements_, other.total_elements_);
    }

    // Lookup
    size_type count(const T& value) const {
        auto it = map_.find(value);
        if (it != map_.end()) {
            return it->second;
        }
        return 0;
    }

    bool contains(const T& value) const {
        return map_.count(value) > 0;
    }

    // Iterators for unique elements
    const_iterator begin() const noexcept {
        return map_.begin();
    }

    const_iterator end() const noexcept {
        return map_.end();
    }

    const_iterator cbegin() const noexcept {
        return map_.cbegin();
    }

    const_iterator cend() const noexcept {
        return map_.cend();
    }

    // Bucket interface (optional, but good for compatibility with std::unordered_*)
    // These could be added if desired, forwarding to the underlying map_.
    // size_type bucket_count() const noexcept { return map_.bucket_count(); }
    // float load_factor() const noexcept { return map_.load_factor(); }

private:
    underlying_container_type map_;
    size_type total_elements_ = 0;
};

// Non-member swap
template <typename T, typename Hash, typename KeyEqual>
void swap(UnorderedMultiset<T, Hash, KeyEqual>& lhs,
          UnorderedMultiset<T, Hash, KeyEqual>& rhs) noexcept {
    lhs.swap(rhs);
}

} // namespace cpp_utils

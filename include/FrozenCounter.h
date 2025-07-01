#pragma once

#include <vector>
#include <utility>      // For std::pair, std::move
#include <algorithm>    // For std::sort, std::lower_bound
#include <stdexcept>    // For std::out_of_range
#include <functional>   // For std::hash, std::equal_to, std::less
#include <memory>       // For std::allocator, std::allocator_traits
#include <initializer_list> // For std::initializer_list
#include <map>          // For std::map in build_from_range
#include <numeric>      // For std::accumulate (potentially for total)
#include "counter.h"    // For the constructor from cpp_collections::Counter

// Forward declaration for std::hash specialization
namespace cpp_collections {
template <
    typename Key,
    typename Compare,
    typename Allocator
>
class FrozenCounter;
} // namespace cpp_collections

namespace std {
// Forward declaration of std::hash specialization for FrozenCounter
template <
    typename Key,
    typename Compare,
    typename Allocator
>
struct hash<cpp_collections::FrozenCounter<Key, Compare, Allocator>>;
} // namespace std


namespace cpp_collections {

template <
    typename Key,
    typename Compare = std::less<Key>,
    typename Allocator = std::allocator<std::pair<const Key, int>>
>
class FrozenCounter {
public:
    // Types
    using key_type = Key;
    using mapped_type = int; // Counts are integers
    using value_type = std::pair<const Key, mapped_type>; // The type stored in the vector
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using key_compare = Compare;
    using allocator_type = Allocator;

    using reference = const value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::const_pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

private:
    using internal_storage_type = std::vector<value_type, Allocator>;
    internal_storage_type data_;
    key_compare key_comparator_;
    size_type total_count_;

    template<typename InputIt>
    void build_from_range(InputIt first, InputIt last) {
        std::map<key_type, mapped_type, key_compare> temp_map(key_comparator_);
        total_count_ = 0;

        for (InputIt it = first; it != last; ++it) {
            // Assuming it->first is key and it->second is count
            // This needs to be robust if InputIt points to just keys for some constructors
            // However, our constructors are designed to pass pairs {key, count}
            if (it->second > 0) {
                temp_map[it->first] += it->second;
            }
        }

        data_.clear();
        data_.reserve(temp_map.size());
        for (const auto& pair : temp_map) {
            if (pair.second > 0) {
                data_.emplace_back(pair.first, pair.second);
                total_count_ += static_cast<size_type>(pair.second);
            }
        }
        // data_ is sorted by key due to std::map.
    }

public:
    using iterator = typename internal_storage_type::const_iterator;
    using const_iterator = typename internal_storage_type::const_iterator;

    // Constructors
    FrozenCounter(const Allocator& alloc = Allocator())
        : data_(alloc), key_comparator_(Compare()), total_count_(0) {}

    FrozenCounter(const key_compare& comp, const Allocator& alloc = Allocator())
        : data_(alloc), key_comparator_(comp), total_count_(0) {}

    template <typename InputIt>
    FrozenCounter(InputIt first, InputIt last,
                  const key_compare& comp = Compare(),
                  const Allocator& alloc = Allocator())
        : data_(alloc), key_comparator_(comp), total_count_(0) {
        build_from_range(first, last);
    }

    FrozenCounter(std::initializer_list<std::pair<Key, mapped_type>> ilist,
                  const key_compare& comp = Compare(),
                  const Allocator& alloc = Allocator())
        : data_(alloc), key_comparator_(comp), total_count_(0) {
        build_from_range(ilist.begin(), ilist.end());
    }

    template <typename CounterHash, typename CounterKeyEqual>
    explicit FrozenCounter(const Counter<Key, CounterHash, CounterKeyEqual>& source_counter,
                           const key_compare& comp = Compare(),
                           const Allocator& alloc = Allocator())
        : data_(alloc), key_comparator_(comp), total_count_(0) {
        build_from_range(source_counter.begin(), source_counter.end());
    }

    FrozenCounter(const FrozenCounter& other)
        : data_(other.data_),
          key_comparator_(other.key_comparator_),
          total_count_(other.total_count_) {}

    FrozenCounter(const FrozenCounter& other, const Allocator& alloc)
        : data_(other.data_, alloc),
          key_comparator_(other.key_comparator_),
          total_count_(other.total_count_) {}

    FrozenCounter(FrozenCounter&& other) noexcept
        : data_(std::move(other.data_)),
          key_comparator_(std::move(other.key_comparator_)),
          total_count_(other.total_count_) {
        other.total_count_ = 0;
    }

    FrozenCounter(FrozenCounter&& other, const Allocator& alloc) noexcept
        : data_(std::move(other.data_), alloc),
          key_comparator_(std::move(other.key_comparator_)),
          total_count_(other.total_count_) {
        other.total_count_ = 0;
    }

    ~FrozenCounter() = default;

    FrozenCounter& operator=(const FrozenCounter& other) {
        if (this == &other) return *this;
        FrozenCounter temp(other);
        swap(temp);
        return *this;
    }

    FrozenCounter& operator=(FrozenCounter&& other) noexcept {
        if (this == &other) return *this;
        data_ = std::move(other.data_);
        key_comparator_ = std::move(other.key_comparator_);
        total_count_ = other.total_count_;
        other.total_count_ = 0;
        return *this;
    }

    FrozenCounter& operator=(std::initializer_list<std::pair<Key, mapped_type>> ilist) {
        FrozenCounter temp(ilist, key_comparator_, data_.get_allocator());
        swap(temp);
        return *this;
    }

    iterator begin() const noexcept { return data_.begin(); }
    const_iterator cbegin() const noexcept { return data_.cbegin(); }
    iterator end() const noexcept { return data_.end(); }
    const_iterator cend() const noexcept { return data_.cend(); }

    bool empty() const noexcept { return data_.empty(); }
    size_type size() const noexcept { return data_.size(); }
    size_type max_size() const noexcept { return data_.max_size(); }
    size_type total() const noexcept { return total_count_; }

    mapped_type count(const key_type& key) const {
        const_iterator it = find(key);
        return (it == data_.end()) ? 0 : it->second;
    }

    mapped_type operator[](const key_type& key) const {
        return count(key);
    }

    bool contains(const key_type& key) const {
        return find(key) != data_.end();
    }

    const_iterator find(const key_type& key) const {
        auto it = std::lower_bound(data_.begin(), data_.end(), key,
            [this](const value_type& element, const key_type& k_val) {
                return key_comparator_(element.first, k_val);
            });
        if (it != data_.end() && !key_comparator_(key, it->first)) {
            return it;
        }
        return data_.end();
    }

    key_compare key_comp() const { return key_comparator_; }
    allocator_type get_allocator() const noexcept { return data_.get_allocator(); }

    std::vector<std::pair<key_type, mapped_type>> most_common(size_type n = 0) const {
        std::vector<std::pair<key_type, mapped_type>> temp_elements;
        temp_elements.reserve(data_.size());
        for (const auto& p : data_) {
            temp_elements.emplace_back(p.first, p.second);
        }

        std::sort(temp_elements.begin(), temp_elements.end(),
            [this](const std::pair<key_type, mapped_type>& a, const std::pair<key_type, mapped_type>& b) {
            if (a.second != b.second) {
                return a.second > b.second;
            }
            return key_comparator_(a.first, b.first);
        });

        if (n > 0 && n < temp_elements.size()) {
            temp_elements.resize(n);
        }
        return temp_elements;
    }

    friend bool operator==(const FrozenCounter& lhs, const FrozenCounter& rhs) {
        if (lhs.total_count_ != rhs.total_count_ || lhs.data_.size() != rhs.data_.size()) {
            return false;
        }
        return lhs.data_ == rhs.data_;
    }

    friend bool operator!=(const FrozenCounter& lhs, const FrozenCounter& rhs) {
        return !(lhs == rhs);
    }

    void swap(FrozenCounter& other) noexcept(
        std::allocator_traits<Allocator>::propagate_on_container_swap::value ||
        std::allocator_traits<Allocator>::is_always_equal::value) {
        using std::swap;
        swap(data_, other.data_);
        swap(key_comparator_, other.key_comparator_);
        swap(total_count_, other.total_count_);
    }

    friend struct std::hash<FrozenCounter<Key, Compare, Allocator>>;
};

template <typename K, typename C, typename A>
void swap(FrozenCounter<K, C, A>& lhs, FrozenCounter<K, C, A>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace cpp_collections

namespace std {
template <typename Key, typename Compare, typename Allocator>
struct hash<cpp_collections::FrozenCounter<Key, Compare, Allocator>> {
    std::size_t operator()(const cpp_collections::FrozenCounter<Key, Compare, Allocator>& fc) const {
        std::size_t seed = 0;
        std::hash<typename cpp_collections::FrozenCounter<Key, Compare, Allocator>::size_type> size_hasher;
        seed ^= size_hasher(fc.total()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

        std::hash<Key> key_hasher;
        std::hash<int> count_hasher;

        for (const auto& elem : fc) {
            std::size_t key_h = key_hasher(elem.first);
            std::size_t count_h = count_hasher(elem.second);

            seed ^= key_h + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= count_h + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};
} // namespace std

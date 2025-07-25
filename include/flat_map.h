#ifndef FLAT_MAP_H
#define FLAT_MAP_H

#include <vector>
#include <algorithm>
#include <utility>
#include <stdexcept>
#include <functional>
#include <cstddef>
#include <initializer_list>

namespace cpp_collections {

template<
    typename Key,
    typename Value,
    typename Compare = std::less<Key>,
    typename Allocator = std::allocator<std::pair<Key, Value>>
>
class flat_map {
public:
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<Key, Value>;
    using key_compare = Compare;
    using allocator_type = Allocator;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
    using iterator = typename std::vector<value_type, Allocator>::iterator;
    using const_iterator = typename std::vector<value_type, Allocator>::const_iterator;
    using reverse_iterator = typename std::vector<value_type, Allocator>::reverse_iterator;
    using const_reverse_iterator = typename std::vector<value_type, Allocator>::const_reverse_iterator;
    using size_type = typename std::vector<value_type, Allocator>::size_type;
    using difference_type = typename std::vector<value_type, Allocator>::difference_type;

private:
    std::vector<value_type, Allocator> data_;
    Compare comp_;

public:
    flat_map() : data_(), comp_(Compare()) {}

    explicit flat_map(const Compare& comp, const Allocator& alloc = Allocator())
        : data_(alloc), comp_(comp) {}

    template<typename InputIt>
    flat_map(InputIt first, InputIt last, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
        : data_(alloc), comp_(comp) {
        insert(first, last);
    }

    flat_map(std::initializer_list<value_type> il, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
        : data_(alloc), comp_(comp) {
        insert(il);
    }

    iterator begin() { return data_.begin(); }
    const_iterator begin() const { return data_.begin(); }
    const_iterator cbegin() const { return data_.cbegin(); }
    iterator end() { return data_.end(); }
    const_iterator end() const { return data_.end(); }
    const_iterator cend() const { return data_.cend(); }
    reverse_iterator rbegin() { return data_.rbegin(); }
    const_reverse_iterator rbegin() const { return data_.rbegin(); }
    const_reverse_iterator crbegin() const { return data_.crbegin(); }
    reverse_iterator rend() { return data_.rend(); }
    const_reverse_iterator rend() const { return data_.rend(); }
    const_reverse_iterator crend() const { return data_.crend(); }

    bool empty() const { return data_.empty(); }
    size_type size() const { return data_.size(); }

    mapped_type& operator[](const key_type& key) {
        auto it = std::lower_bound(data_.begin(), data_.end(), key,
            [this](const value_type& a, const key_type& b) {
            return comp_(a.first, b);
        });
        if (it != data_.end() && !comp_(key, it->first)) {
            return it->second;
        }
        it = data_.insert(it, {key, {}});
        return it->second;
    }

    mapped_type& at(const key_type& key) {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("flat_map::at");
        }
        return it->second;
    }

    const mapped_type& at(const key_type& key) const {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("flat_map::at");
        }
        return it->second;
    }

    iterator find(const key_type& key) {
        auto it = std::lower_bound(data_.begin(), data_.end(), key,
            [this](const value_type& a, const key_type& b) {
            return comp_(a.first, b);
        });
        if (it != data_.end() && !comp_(key, it->first)) {
            return it;
        }
        return end();
    }

    const_iterator find(const key_type& key) const {
        auto it = std::lower_bound(data_.begin(), data_.end(), key,
            [this](const value_type& a, const key_type& b) {
            return comp_(a.first, b);
        });
        if (it != data_.end() && !comp_(key, it->first)) {
            return it;
        }
        return end();
    }

    std::pair<iterator, bool> insert(const value_type& value) {
        auto it = std::lower_bound(data_.begin(), data_.end(), value.first,
            [this](const value_type& a, const key_type& b) {
            return comp_(a.first, b);
        });
        if (it != data_.end() && !comp_(value.first, it->first)) {
            return {it, false};
        }
        it = data_.insert(it, value);
        return {it, true};
    }

    template<typename InputIt>
    void insert(InputIt first, InputIt last) {
        for (auto it = first; it != last; ++it) {
            insert(*it);
        }
    }

    void insert(std::initializer_list<value_type> il) {
        insert(il.begin(), il.end());
    }

    size_type erase(const key_type& key) {
        auto it = find(key);
        if (it == end()) {
            return 0;
        }
        data_.erase(it);
        return 1;
    }

    void clear() {
        data_.clear();
    }
};

} // namespace cpp_collections

#endif // FLAT_MAP_H

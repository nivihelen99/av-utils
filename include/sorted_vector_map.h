#pragma once

#include <vector>
#include <algorithm>
#include <stdexcept>

template<typename Key, typename Value, typename Compare = std::less<Key>>
class sorted_vector_map {
public:
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<Key, Value>;
    using key_compare = Compare;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = typename std::vector<value_type>::iterator;
    using const_iterator = typename std::vector<value_type>::const_iterator;
    using reverse_iterator = typename std::vector<value_type>::reverse_iterator;
    using const_reverse_iterator = typename std::vector<value_type>::const_reverse_iterator;
    using size_type = typename std::vector<value_type>::size_type;

private:
    std::vector<value_type> data_;
    key_compare comp_;

public:
    sorted_vector_map() = default;

    template<class InputIt>
    sorted_vector_map(InputIt first, InputIt last) {
        data_.assign(first, last);
        std::sort(data_.begin(), data_.end(), [this](const auto& a, const auto& b) {
            return comp_(a.first, b.first);
        });
    }

    iterator begin() noexcept {
        return data_.begin();
    }

    const_iterator begin() const noexcept {
        return data_.begin();
    }

    const_iterator cbegin() const noexcept {
        return data_.cbegin();
    }

    iterator end() noexcept {
        return data_.end();
    }

    const_iterator end() const noexcept {
        return data_.end();
    }

    const_iterator cend() const noexcept {
        return data_.cend();
    }

    reverse_iterator rbegin() noexcept {
        return data_.rbegin();
    }

    const_reverse_iterator rbegin() const noexcept {
        return data_.rbegin();
    }

    const_reverse_iterator crbegin() const noexcept {
        return data_.crbegin();
    }

    reverse_iterator rend() noexcept {
        return data_.rend();
    }

    const_reverse_iterator rend() const noexcept {
        return data_.rend();
    }

    const_reverse_iterator crend() const noexcept {
        return data_.crend();
    }

    bool empty() const noexcept {
        return data_.empty();
    }

    size_type size() const noexcept {
        return data_.size();
    }

    size_type max_size() const noexcept {
        return data_.max_size();
    }

    mapped_type& at(const key_type& key) {
        auto it = lower_bound(key);
        if (it == end() || comp_(key, it->first)) {
            throw std::out_of_range("sorted_vector_map::at");
        }
        return it->second;
    }

    const mapped_type& at(const key_type& key) const {
        auto it = lower_bound(key);
        if (it == end() || comp_(key, it->first)) {
            throw std::out_of_range("sorted_vector_map::at");
        }
        return it->second;
    }

    mapped_type& operator[](const key_type& key) {
        auto it = lower_bound(key);
        if (it == end() || comp_(key, it->first)) {
            return insert(it, {key, {}})->second;
        }
        return it->second;
    }

    mapped_type& operator[](key_type&& key) {
        auto it = lower_bound(key);
        if (it == end() || comp_(key, it->first)) {
            return insert(it, {std::move(key), {}})->second;
        }
        return it->second;
    }

    std::pair<iterator, bool> insert(const value_type& value) {
        auto it = lower_bound(value.first);
        if (it != end() && !comp_(value.first, it->first)) {
            return {it, false};
        }
        return {data_.insert(it, value), true};
    }

    iterator insert(const_iterator pos, const value_type& value) {
        (void)pos; // hint is ignored
        return insert(value).first;
    }

    template<class InputIt>
    void insert(InputIt first, InputIt last) {
        for (; first != last; ++first) {
            insert(*first);
        }
    }

    iterator erase(const_iterator pos) {
        return data_.erase(pos);
    }

    size_type erase(const key_type& key) {
        auto it = find(key);
        if (it == end()) {
            return 0;
        }
        erase(it);
        return 1;
    }

    void swap(sorted_vector_map& other) {
        data_.swap(other.data_);
        std::swap(comp_, other.comp_);
    }

    void clear() noexcept {
        data_.clear();
    }

    iterator find(const key_type& key) {
        auto it = lower_bound(key);
        if (it != end() && !comp_(key, it->first)) {
            return it;
        }
        return end();
    }

    const_iterator find(const key_type& key) const {
        auto it = lower_bound(key);
        if (it != end() && !comp_(key, it->first)) {
            return it;
        }
        return end();
    }

    size_type count(const key_type& key) const {
        return find(key) == end() ? 0 : 1;
    }

    iterator lower_bound(const key_type& key) {
        return std::lower_bound(begin(), end(), key, [this](const auto& elem, const auto& k) {
            return comp_(elem.first, k);
        });
    }

    const_iterator lower_bound(const key_type& key) const {
        return std::lower_bound(begin(), end(), key, [this](const auto& elem, const auto& k) {
            return comp_(elem.first, k);
        });
    }

    iterator upper_bound(const key_type& key) {
        return std::upper_bound(begin(), end(), key, [this](const auto& k, const auto& elem) {
            return comp_(k, elem.first);
        });
    }

    const_iterator upper_bound(const key_type& key) const {
        return std::upper_bound(begin(), end(), key, [this](const auto& k, const auto& elem) {
            return comp_(k, elem.first);
        });
    }

    std::pair<iterator, iterator> equal_range(const key_type& key) {
        return {lower_bound(key), upper_bound(key)};
    }

    std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        return {lower_bound(key), upper_bound(key)};
    }

private:
    // a helper function for insertion
    iterator insert_at(const_iterator pos, value_type&& value) {
        return data_.insert(pos, std::move(value));
    }
};

template<class Key, class T, class Compare>
void swap(sorted_vector_map<Key, T, Compare>& lhs, sorted_vector_map<Key, T, Compare>& rhs) {
    lhs.swap(rhs);
}

#pragma once

#include <list>
#include <unordered_map>
#include <utility>      // For std::pair, std::move
#include <stdexcept>    // For std::out_of_range
#include <optional>     // For std::optional
#include <functional>   // For std::hash, std::equal_to, std::reference_wrapper
#include <memory>       // For std::allocator, std::allocator_traits
#include <initializer_list> // For std::initializer_list
#include <algorithm>    // For std::equal, std::distance
#include <tuple>        // For std::piecewise_construct, std::forward_as_tuple

namespace cpp_collections {

template <typename LRUDictType, bool IsConst>
class LRUDictIterator;

template <
    typename Key,
    typename Value,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    typename Allocator = std::allocator<std::pair<const Key, Value>>
>
class LRUDict {
public:
    // Types
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using list_value_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<value_type>;
private:
    using list_type = std::list<value_type, list_value_allocator>;
public:
    using list_iterator_type = typename list_type::iterator;
    using const_list_iterator_type = typename list_type::const_iterator;

    using map_internal_value_type = std::pair<const key_type, list_iterator_type>;
    using map_value_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<map_internal_value_type>;

private:
    using map_type = std::unordered_map<key_type, list_iterator_type, hasher, key_equal, map_value_allocator>;

    list_type list_;
    map_type map_;
    size_type capacity_;

    void _move_to_front(list_iterator_type list_it);
    void _evict();

public:
    using iterator = LRUDictIterator<LRUDict<Key, Value, Hash, KeyEqual, Allocator>, false>;
    using const_iterator = LRUDictIterator<LRUDict<Key, Value, Hash, KeyEqual, Allocator>, true>;

    friend class LRUDictIterator<LRUDict<Key, Value, Hash, KeyEqual, Allocator>, false>;
    friend class LRUDictIterator<LRUDict<Key, Value, Hash, KeyEqual, Allocator>, true>;

    explicit LRUDict(size_type capacity,
                     const hasher& hash = hasher(),
                     const key_equal& equal = key_equal(),
                     const allocator_type& alloc = allocator_type());
    LRUDict(size_type capacity, const allocator_type& alloc);
    LRUDict(const LRUDict& other);
    LRUDict(const LRUDict& other, const allocator_type& alloc);
    LRUDict(LRUDict&& other) noexcept;
    LRUDict(LRUDict&& other, const allocator_type& alloc) noexcept;
    ~LRUDict() = default;

    LRUDict& operator=(const LRUDict& other);
    LRUDict& operator=(LRUDict&& other) noexcept;

    std::pair<iterator, bool> insert(const value_type& kv_pair);
    std::pair<iterator, bool> insert(value_type&& kv_pair);
    template<typename P>
    std::enable_if_t<std::is_constructible_v<value_type, P&&>, std::pair<iterator, bool>>
    insert(P&& pair_like);

    std::pair<iterator, bool> insert_or_assign(const key_type& k, const mapped_type& v);
    std::pair<iterator, bool> insert_or_assign(const key_type& k, mapped_type&& v);
    std::pair<iterator, bool> insert_or_assign(key_type&& k, const mapped_type& v);
    std::pair<iterator, bool> insert_or_assign(key_type&& k, mapped_type&& v);

    template <typename... Args>
    std::pair<iterator, bool> try_emplace(const key_type& k, Args&&... args);
    template <typename... Args>
    std::pair<iterator, bool> try_emplace(key_type&& k, Args&&... args);

    mapped_type& operator[](const key_type& key);
    mapped_type& operator[](key_type&& key);

    bool erase(const key_type& key);
    iterator erase(iterator pos);
    iterator erase(const_iterator pos);
    void clear() noexcept;

    mapped_type& at(const key_type& key);
    const mapped_type& at(const key_type& key) const;
    std::optional<std::reference_wrapper<mapped_type>> get(const key_type& key);
    std::optional<std::reference_wrapper<const mapped_type>> get(const key_type& key) const;
    std::optional<std::reference_wrapper<const mapped_type>> peek(const key_type& key) const;
    bool contains(const key_type& key) const;

    size_type capacity() const noexcept { return capacity_; }
    size_type size() const noexcept { return map_.size(); }
    bool empty() const noexcept { return map_.empty(); }
    bool is_full() const noexcept { return capacity_ > 0 && map_.size() >= capacity_; }

    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    const_iterator cbegin() const noexcept;
    iterator end() noexcept;
    const_iterator end() const noexcept;
    const_iterator cend() const noexcept;

    hasher hash_function() const { return map_.hash_function(); }
    key_equal key_eq() const { return map_.key_eq(); }
    allocator_type get_allocator() const;
    void swap(LRUDict& other) noexcept;

    bool operator==(const LRUDict& other) const;
    bool operator!=(const LRUDict& other) const { return !(*this == other); }
};

template <typename LRUDictType, bool IsConst>
class LRUDictIterator {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = typename LRUDictType::value_type;
    using difference_type = typename LRUDictType::difference_type;
    using internal_list_iterator_type = std::conditional_t<IsConst,
                                typename LRUDictType::const_list_iterator_type,
                                typename LRUDictType::list_iterator_type>;
    using pointer = std::conditional_t<IsConst, const value_type*, value_type*>;
    using reference = std::conditional_t<IsConst, const value_type&, value_type&>;
private:
    internal_list_iterator_type list_iter_;
    friend LRUDictType;
    friend class LRUDictIterator<LRUDictType, true>;
    LRUDictIterator(internal_list_iterator_type it) : list_iter_(it) {} // Non-explicit
public:
    LRUDictIterator() = default;
    LRUDictIterator(const LRUDictIterator<LRUDictType, false>& other) requires IsConst
        : list_iter_(other.list_iter_) {}
    reference operator*() const { return *list_iter_; }
    pointer operator->() const { return &(*list_iter_); }
    LRUDictIterator& operator++() { ++list_iter_; return *this; }
    LRUDictIterator operator++(int) { LRUDictIterator temp = *this; ++list_iter_; return temp; }
    LRUDictIterator& operator--() { --list_iter_; return *this; }
    LRUDictIterator operator--(int) { LRUDictIterator temp = *this; --list_iter_; return temp; }
    bool operator==(const LRUDictIterator& other) const { return list_iter_ == other.list_iter_; }
    bool operator!=(const LRUDictIterator& other) const { return list_iter_ != other.list_iter_; }
};

// Definitions of LRUDict member functions start here

template <typename K, typename V, typename H, typename KE, typename A>
void LRUDict<K, V, H, KE, A>::_move_to_front(list_iterator_type list_it) {
    if (list_it != list_.begin()) { list_.splice(list_.begin(), list_, list_it); }
}

template <typename K, typename V, typename H, typename KE, typename A>
void LRUDict<K, V, H, KE, A>::_evict() {
    if (capacity_ == 0 || list_.empty()) return;
    const key_type& key_to_evict = list_.back().first;
    map_.erase(key_to_evict);
    list_.pop_back();
}

template <typename K, typename V, typename H, typename KE, typename A>
LRUDict<K, V, H, KE, A>::LRUDict(size_type cap, const hasher& hash_fn, const key_equal& key_eq_fn, const allocator_type& alloc)
    : list_(alloc), map_((cap > 0 ? cap : 10), hash_fn, key_eq_fn, map_value_allocator(alloc)), capacity_(cap) {
    if (capacity_ > 0 && map_.max_load_factor() > 0) {
        map_.rehash(static_cast<size_t>(capacity_ / map_.max_load_factor()) + 1);
    } else if (capacity_ > 0) { map_.rehash(capacity_ * 2); }
}

template <typename K, typename V, typename H, typename KE, typename A>
LRUDict<K, V, H, KE, A>::LRUDict(size_type cap, const allocator_type& alloc)
    : LRUDict(cap, hasher(), key_equal(), alloc) {}

template <typename K, typename V, typename H, typename KE, typename A>
LRUDict<K, V, H, KE, A>::LRUDict(const LRUDict& other)
    : list_(std::allocator_traits<list_value_allocator>::select_on_container_copy_construction(other.list_.get_allocator())),
      map_(other.map_.bucket_count(), other.hash_function(), other.key_eq(),
           std::allocator_traits<map_value_allocator>::select_on_container_copy_construction(other.map_.get_allocator())),
      capacity_(other.capacity_) {
    if (capacity_ > 0 && map_.max_load_factor() > 0) {
         map_.rehash(static_cast<size_t>(capacity_ / map_.max_load_factor()) + 1);
    } else if (capacity_ > 0) { map_.rehash(capacity_ * 2); }
    for (auto it = other.list_.rbegin(); it != other.list_.rend(); ++it) {
        list_.push_front(*it); map_.emplace(list_.front().first, list_.begin());
    }
}

template <typename K, typename V, typename H, typename KE, typename A>
LRUDict<K, V, H, KE, A>::LRUDict(const LRUDict& other, const allocator_type& alloc)
    : list_(alloc), map_(other.map_.bucket_count(), other.hash_function(), other.key_eq(), map_value_allocator(alloc)), capacity_(other.capacity_) {
    if (capacity_ > 0 && map_.max_load_factor() > 0) {
        map_.rehash(static_cast<size_t>(capacity_ / map_.max_load_factor()) + 1);
    } else if (capacity_ > 0) { map_.rehash(capacity_ * 2); }
    for (auto it = other.list_.rbegin(); it != other.list_.rend(); ++it) {
        list_.push_front(*it); map_.emplace(list_.front().first, list_.begin());
    }
}

template <typename K, typename V, typename H, typename KE, typename A>
LRUDict<K, V, H, KE, A>::LRUDict(LRUDict&& other) noexcept
    : list_(std::move(other.list_)), map_(std::move(other.map_)), capacity_(other.capacity_) {
    other.capacity_ = 0;
}

template <typename K, typename V, typename H, typename KE, typename A>
LRUDict<K, V, H, KE, A>::LRUDict(LRUDict&& other, const allocator_type& alloc) noexcept
    : list_(std::move(other.list_), alloc), map_(std::move(other.map_)), capacity_(other.capacity_) {
    if (map_.get_allocator() != map_value_allocator(alloc)) {
        map_type temp_map(capacity_ > 0 ? capacity_ : 10, other.hash_function(), other.key_eq(), map_value_allocator(alloc));
        if (capacity_ > 0 && temp_map.max_load_factor() > 0) {
             temp_map.rehash(static_cast<size_t>(capacity_ / temp_map.max_load_factor()) + 1);
        } else if (capacity_ > 0) { temp_map.rehash(capacity_ * 2); }
        for(auto it = list_.begin(); it != list_.end(); ++it) { temp_map.emplace(it->first, it); }
        map_ = std::move(temp_map);
    }
    other.capacity_ = 0;
}

template <typename K, typename V, typename H, typename KE, typename A>
LRUDict<K, V, H, KE, A>& LRUDict<K, V, H, KE, A>::operator=(const LRUDict& other) {
    if (this == &other) return *this;
    LRUDict temp(other); swap(temp); return *this;
}

template <typename K, typename V, typename H, typename KE, typename A>
LRUDict<K, V, H, KE, A>& LRUDict<K, V, H, KE, A>::operator=(LRUDict&& other) noexcept {
    if (this == &other) return *this;
    LRUDict temp(std::move(other)); swap(temp); return *this;
}

template <typename K, typename V, typename H, typename KE, typename A>
std::pair<typename LRUDict<K, V, H, KE, A>::iterator, bool>
LRUDict<K, V, H, KE, A>::insert(const value_type& kv_pair) {
    if (capacity_ == 0) return std::make_pair(end(), false);
    auto map_it = map_.find(kv_pair.first);
    if (map_it != map_.end()) {
        map_it->second->second = kv_pair.second; _move_to_front(map_it->second);
        return std::make_pair(iterator(map_it->second), false);
    } else {
        if (is_full()) _evict();
        list_.push_front(kv_pair); map_.emplace(kv_pair.first, list_.begin());
        return std::make_pair(iterator(list_.begin()), true);
    }
}

template <typename K, typename V, typename H, typename KE, typename A>
std::pair<typename LRUDict<K, V, H, KE, A>::iterator, bool>
LRUDict<K, V, H, KE, A>::insert(value_type&& kv_pair) {
    if (capacity_ == 0) return std::make_pair(end(), false);
    auto map_it = map_.find(kv_pair.first);
    if (map_it != map_.end()) {
        map_it->second->second = std::move(kv_pair.second); _move_to_front(map_it->second);
        return std::make_pair(iterator(map_it->second), false);
    } else {
        if (is_full()) _evict();
        list_.push_front(std::move(kv_pair)); map_.emplace(list_.begin()->first, list_.begin());
        return std::make_pair(iterator(list_.begin()), true);
    }
}

template <typename K, typename V, typename H, typename KE, typename A>
template<typename P>
std::enable_if_t<std::is_constructible_v<typename LRUDict<K, V, H, KE, A>::value_type, P&&>, std::pair<typename LRUDict<K, V, H, KE, A>::iterator, bool>>
LRUDict<K, V, H, KE, A>::insert(P&& pair_like) {
    value_type val_to_insert(std::forward<P>(pair_like));
    return insert(std::move(val_to_insert));
}

template <typename K, typename V, typename H, typename KE, typename A>
std::pair<typename LRUDict<K, V, H, KE, A>::iterator, bool>
LRUDict<K, V, H, KE, A>::insert_or_assign(const key_type& k, const mapped_type& v) {
    auto map_it = map_.find(k);
    if (map_it != map_.end()) {
        map_it->second->second = v; _move_to_front(map_it->second);
        return std::make_pair(iterator(map_it->second), false);
    } else {
        if (capacity_ == 0) return std::make_pair(end(), false);
        if (is_full()) _evict();
        list_.emplace_front(k, v); map_.emplace(k, list_.begin());
        return std::make_pair(iterator(list_.begin()), true);
    }
}

template <typename K, typename V, typename H, typename KE, typename A>
std::pair<typename LRUDict<K, V, H, KE, A>::iterator, bool>
LRUDict<K, V, H, KE, A>::insert_or_assign(const key_type& k, mapped_type&& v) {
    auto map_it = map_.find(k);
    if (map_it != map_.end()) {
        map_it->second->second = std::move(v); _move_to_front(map_it->second);
        return std::make_pair(iterator(map_it->second), false);
    } else {
        if (capacity_ == 0) return std::make_pair(end(), false);
        if (is_full()) _evict();
        list_.emplace_front(k, std::move(v)); map_.emplace(k, list_.begin());
        return std::make_pair(iterator(list_.begin()), true);
    }
}

template <typename K, typename V, typename H, typename KE, typename A>
std::pair<typename LRUDict<K, V, H, KE, A>::iterator, bool>
LRUDict<K, V, H, KE, A>::insert_or_assign(key_type&& k, const mapped_type& v) {
    key_type temp_k = k;
    auto map_it = map_.find(temp_k);
    if (map_it != map_.end()) {
        map_it->second->second = v; _move_to_front(map_it->second);
        return std::make_pair(iterator(map_it->second), false);
    } else {
        if (capacity_ == 0) return std::make_pair(end(), false);
        if (is_full()) _evict();
        list_.emplace_front(std::move(k), v); map_.emplace(list_.begin()->first, list_.begin());
        return std::make_pair(iterator(list_.begin()), true);
    }
}

template <typename K, typename V, typename H, typename KE, typename A>
std::pair<typename LRUDict<K, V, H, KE, A>::iterator, bool>
LRUDict<K, V, H, KE, A>::insert_or_assign(key_type&& k, mapped_type&& v) {
    key_type temp_k = k;
    auto map_it = map_.find(temp_k);
    if (map_it != map_.end()) {
        map_it->second->second = std::move(v); _move_to_front(map_it->second);
        return std::make_pair(iterator(map_it->second), false);
    } else {
        if (capacity_ == 0) return std::make_pair(end(), false);
        if (is_full()) _evict();
        list_.emplace_front(std::move(k), std::move(v)); map_.emplace(list_.begin()->first, list_.begin());
        return std::make_pair(iterator(list_.begin()), true);
    }
}

template <typename K, typename V, typename H, typename KE, typename A>
template <typename... Args>
std::pair<typename LRUDict<K, V, H, KE, A>::iterator, bool>
LRUDict<K, V, H, KE, A>::try_emplace(const key_type& k, Args&&... args) {
    auto map_it = map_.find(k);
    if (map_it != map_.end()) {
        _move_to_front(map_it->second);
        return std::make_pair(iterator(map_it->second), false);
    } else {
        if (capacity_ == 0) return std::make_pair(end(), false);
        if (is_full()) _evict();
        list_.emplace_front(std::piecewise_construct, std::forward_as_tuple(k), std::forward_as_tuple(std::forward<Args>(args)...));
        map_.emplace(k, list_.begin());
        return std::make_pair(iterator(list_.begin()), true);
    }
}

template <typename K, typename V, typename H, typename KE, typename A>
template <typename... Args>
std::pair<typename LRUDict<K, V, H, KE, A>::iterator, bool>
LRUDict<K, V, H, KE, A>::try_emplace(key_type&& k, Args&&... args) {
    key_type temp_k = k;
    auto map_it = map_.find(temp_k);
    if (map_it != map_.end()) {
        _move_to_front(map_it->second);
        return std::make_pair(iterator(map_it->second), false);
    } else {
        if (capacity_ == 0) return std::make_pair(end(), false);
        if (is_full()) _evict();
        list_.emplace_front(std::piecewise_construct, std::forward_as_tuple(std::move(k)), std::forward_as_tuple(std::forward<Args>(args)...));
        map_.emplace(list_.begin()->first, list_.begin());
        return std::make_pair(iterator(list_.begin()), true);
    }
}

template <typename K, typename V, typename H, typename KE, typename A>
typename LRUDict<K, V, H, KE, A>::mapped_type&
LRUDict<K, V, H, KE, A>::operator[](const key_type& key) {
    auto map_it = map_.find(key);
    if (map_it != map_.end()) {
        _move_to_front(map_it->second); return map_it->second->second;
    } else {
        if (capacity_ == 0) throw std::out_of_range("LRUDict::operator[]: Cannot insert with zero capacity.");
        if (is_full()) _evict();
        list_.emplace_front(key, mapped_type{}); map_.emplace(key, list_.begin());
        return list_.begin()->second;
    }
}

template <typename K, typename V, typename H, typename KE, typename A>
typename LRUDict<K, V, H, KE, A>::mapped_type&
LRUDict<K, V, H, KE, A>::operator[](key_type&& key) {
    key_type temp_k = key;
    auto map_it = map_.find(temp_k);
    if (map_it != map_.end()) {
        _move_to_front(map_it->second); return map_it->second->second;
    } else {
        if (capacity_ == 0) throw std::out_of_range("LRUDict::operator[]: Cannot insert with zero capacity.");
        if (is_full()) _evict();
        list_.emplace_front(std::move(key), mapped_type{}); map_.emplace(list_.begin()->first, list_.begin());
        return list_.begin()->second;
    }
}

template <typename K, typename V, typename H, typename KE, typename A>
bool LRUDict<K, V, H, KE, A>::erase(const key_type& key) {
    auto map_it = map_.find(key);
    if (map_it != map_.end()) {
        list_.erase(map_it->second); map_.erase(map_it); return true;
    }
    return false;
}

template <typename K, typename V, typename H, typename KE, typename A>
typename LRUDict<K, V, H, KE, A>::iterator
LRUDict<K, V, H, KE, A>::erase(iterator pos) {
    if (pos == end() || pos.list_iter_ == list_.end()) return end();
    const key_type& key_to_erase = pos.list_iter_->first;
    auto next_list_iter = list_.erase(pos.list_iter_);
    map_.erase(key_to_erase); return iterator(next_list_iter);
}

template <typename K, typename V, typename H, typename KE, typename A>
typename LRUDict<K, V, H, KE, A>::iterator
LRUDict<K, V, H, KE, A>::erase(const_iterator pos) {
    if (pos == cend() || pos.list_iter_ == list_.cend()) return end();
    const key_type& key_to_erase = pos.list_iter_->first;
    auto next_list_iter = list_.erase(pos.list_iter_);
    map_.erase(key_to_erase); return iterator(next_list_iter);
}

template <typename K, typename V, typename H, typename KE, typename A>
void LRUDict<K, V, H, KE, A>::clear() noexcept { map_.clear(); list_.clear(); }

template <typename K, typename V, typename H, typename KE, typename A>
typename LRUDict<K, V, H, KE, A>::mapped_type&
LRUDict<K, V, H, KE, A>::at(const key_type& key) {
    auto map_it = map_.find(key);
    if (map_it != map_.end()) { _move_to_front(map_it->second); return map_it->second->second; }
    throw std::out_of_range("LRUDict::at: key not found");
}

template <typename K, typename V, typename H, typename KE, typename A>
const typename LRUDict<K, V, H, KE, A>::mapped_type&
LRUDict<K, V, H, KE, A>::at(const key_type& key) const {
    auto map_it = map_.find(key);
    if (map_it != map_.end()) return map_it->second->second;
    throw std::out_of_range("LRUDict::at const: key not found");
}

template <typename K, typename V, typename H, typename KE, typename A>
std::optional<std::reference_wrapper<typename LRUDict<K, V, H, KE, A>::mapped_type>>
LRUDict<K, V, H, KE, A>::get(const key_type& key) {
    auto map_it = map_.find(key);
    if (map_it != map_.end()) { _move_to_front(map_it->second); return std::ref(map_it->second->second); }
    return std::nullopt;
}

template <typename K, typename V, typename H, typename KE, typename A>
std::optional<std::reference_wrapper<const typename LRUDict<K, V, H, KE, A>::mapped_type>>
LRUDict<K, V, H, KE, A>::get(const key_type& key) const {
    auto map_it = map_.find(key);
    if (map_it != map_.end()) return std::cref(map_it->second->second);
    return std::nullopt;
}

template <typename K, typename V, typename H, typename KE, typename A>
std::optional<std::reference_wrapper<const typename LRUDict<K, V, H, KE, A>::mapped_type>>
LRUDict<K, V, H, KE, A>::peek(const key_type& key) const {
    auto map_it = map_.find(key);
    if (map_it != map_.end()) return std::cref(map_it->second->second);
    return std::nullopt;
}

template <typename K, typename V, typename H, typename KE, typename A>
bool LRUDict<K, V, H, KE, A>::contains(const key_type& key) const { return map_.count(key) > 0; }

template <typename K, typename V, typename H, typename KE, typename A>
typename LRUDict<K, V, H, KE, A>::iterator LRUDict<K, V, H, KE, A>::begin() noexcept { return iterator(list_.begin()); }
template <typename K, typename V, typename H, typename KE, typename A>
typename LRUDict<K, V, H, KE, A>::const_iterator LRUDict<K, V, H, KE, A>::begin() const noexcept { return const_iterator(list_.begin()); }
template <typename K, typename V, typename H, typename KE, typename A>
typename LRUDict<K, V, H, KE, A>::const_iterator LRUDict<K, V, H, KE, A>::cbegin() const noexcept { return const_iterator(list_.cbegin()); }
template <typename K, typename V, typename H, typename KE, typename A>
typename LRUDict<K, V, H, KE, A>::iterator LRUDict<K, V, H, KE, A>::end() noexcept { return iterator(list_.end()); }
template <typename K, typename V, typename H, typename KE, typename A>
typename LRUDict<K, V, H, KE, A>::const_iterator LRUDict<K, V, H, KE, A>::end() const noexcept { return const_iterator(list_.end()); }
template <typename K, typename V, typename H, typename KE, typename A>
typename LRUDict<K, V, H, KE, A>::const_iterator LRUDict<K, V, H, KE, A>::cend() const noexcept { return const_iterator(list_.cend()); }

template <typename K, typename V, typename H, typename KE, typename A>
typename LRUDict<K, V, H, KE, A>::allocator_type LRUDict<K, V, H, KE, A>::get_allocator() const { return list_.get_allocator(); }

template <typename K, typename V, typename H, typename KE, typename A>
void LRUDict<K, V, H, KE, A>::swap(LRUDict& other) noexcept {
    using std::swap; swap(list_, other.list_); swap(map_, other.map_); swap(capacity_, other.capacity_);
}

template <typename K, typename V, typename H, typename KE, typename A>
bool LRUDict<K, V, H, KE, A>::operator==(const LRUDict& other) const {
    if (this == &other) return true;
    if (capacity_ != other.capacity_) return false;
    if (size() != other.size()) return false;
    return list_ == other.list_;
}

template <typename K, typename V, typename H, typename KE, typename A>
void swap(LRUDict<K, V, H, KE, A>& lhs, LRUDict<K, V, H, KE, A>& rhs) noexcept { lhs.swap(rhs); }

} // namespace cpp_collections
// [end of include/LRUDict.h]  <- This was the problematic line, now removed from the actual content below.

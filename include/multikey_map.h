#pragma once

#include <functional> // For std::hash
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <utility> // For std::pair, std::move

// Forward declaration
template <typename Value, typename... KeyTypes>
class MultiKeyMap;

// Helper to combine hash values
// Inspired by boost::hash_combine
template <typename T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// Hash functor for std::tuple
template <typename... KeyTypes>
struct TupleHash {
    std::size_t operator()(const std::tuple<KeyTypes...>& t) const {
        std::size_t seed = 0;
        // Apply hash_combine for each element in the tuple
        std::apply([&seed](const auto&... args) {
            (hash_combine(seed, args), ...);
        }, t);
        return seed;
    }
};

// Equality functor for std::tuple (std::tuple already has operator==)
// Not strictly needed if std::tuple::operator== is used by default by unordered_map,
// but can be explicitly provided for clarity or if specific behavior is needed.
// For now, we rely on the default std::equal_to<std::tuple<KeyTypes...>> which uses operator==.

template <typename Value, typename... KeyTypes>
class MultiKeyMap {
public:
    using KeyTuple = std::tuple<KeyTypes...>;
    using InternalMap = std::unordered_map<KeyTuple, Value, TupleHash<KeyTypes...>>;
    using value_type = typename InternalMap::value_type; // std::pair<const KeyTuple, Value>
    using mapped_type = Value;
    using key_type = KeyTuple; // Exposing the tuple type as key_type
    using size_type = typename InternalMap::size_type;
    using difference_type = typename InternalMap::difference_type;
    using hasher = TupleHash<KeyTypes...>;
    using key_equal = typename InternalMap::key_equal; // std::equal_to<KeyTuple>
    using reference = typename InternalMap::reference;
    using const_reference = typename InternalMap::const_reference;
    using pointer = typename InternalMap::pointer;
    using const_pointer = typename InternalMap::const_pointer;
    using iterator = typename InternalMap::iterator;
    using const_iterator = typename InternalMap::const_iterator;

private:
    InternalMap map_;

public:
    // Constructors
    MultiKeyMap() = default;

    explicit MultiKeyMap(size_type bucket_count,
                         const hasher& hash = hasher(),
                         const key_equal& equal = key_equal())
        : map_(bucket_count, hash, equal) {}

    MultiKeyMap(std::initializer_list<std::pair<KeyTuple, Value>> init) {
        for (const auto& item : init) {
            // Directly use emplace which is more efficient if KeyTuple can be constructed from item.first
            // or if item.first is already a KeyTuple.
            // Since item is std::pair<KeyTuple, Value>, item.first is KeyTuple.
            map_.emplace(item.first, item.second);
        }
    }

    // Copy and Move constructors/assignments
    MultiKeyMap(const MultiKeyMap&) = default;
    MultiKeyMap(MultiKeyMap&&) = default;
    MultiKeyMap& operator=(const MultiKeyMap&) = default;
    MultiKeyMap& operator=(MultiKeyMap&&) = default;

    // Destructor
    ~MultiKeyMap() = default;

    // Iterators
    iterator begin() noexcept { return map_.begin(); }
    const_iterator begin() const noexcept { return map_.begin(); }
    const_iterator cbegin() const noexcept { return map_.cbegin(); }

    iterator end() noexcept { return map_.end(); }
    const_iterator end() const noexcept { return map_.end(); }
    const_iterator cend() const noexcept { return map_.cend(); }

    // Capacity
    bool empty() const noexcept { return map_.empty(); }
    size_type size() const noexcept { return map_.size(); }
    size_type max_size() const noexcept { return map_.max_size(); }

    // Modifiers
    void clear() noexcept { map_.clear(); }

    // Further methods (insert, emplace, find, at, erase, contains, operator[], etc.)
    // will be implemented in the next step.

    // --- Modifiers ---

    std::pair<iterator, bool> insert(const KeyTypes&... keys, const Value& value) {
        return map_.insert({std::make_tuple(keys...), value});
    }

    std::pair<iterator, bool> insert(const KeyTypes&... keys, Value&& value) {
        return map_.insert({std::make_tuple(keys...), std::move(value)});
    }

    // Overload for when keys are already in a tuple
    std::pair<iterator, bool> insert(const KeyTuple& key_tuple, const Value& value) {
        return map_.insert({key_tuple, value});
    }

    std::pair<iterator, bool> insert(const KeyTuple& key_tuple, Value&& value) {
        return map_.insert({key_tuple, std::move(value)});
    }

    std::pair<iterator, bool> insert(KeyTuple&& key_tuple, Value&& value) {
        return map_.insert({std::move(key_tuple), std::move(value)});
    }

    template <typename... ArgsValue>
    std::pair<iterator, bool> emplace(const KeyTypes&... keys, ArgsValue&&... args_value) {
        return map_.emplace(std::make_tuple(keys...), Value(std::forward<ArgsValue>(args_value)...));
    }

    // Emplace with KeyTuple directly
    template <typename... ArgsValue>
    std::pair<iterator, bool> emplace_tuple(const KeyTuple& key_tuple, ArgsValue&&... args_value) {
        return map_.emplace(key_tuple, Value(std::forward<ArgsValue>(args_value)...));
    }

    template <typename... ArgsValue>
    std::pair<iterator, bool> emplace_tuple(KeyTuple&& key_tuple, ArgsValue&&... args_value) {
        return map_.emplace(std::move(key_tuple), Value(std::forward<ArgsValue>(args_value)...));
    }

    // try_emplace equivalent for variadic keys
    template <typename... ArgsValue>
    std::pair<iterator, bool> try_emplace(const KeyTypes&... keys, ArgsValue&&... args_value) {
        return map_.try_emplace(std::make_tuple(keys...), std::forward<ArgsValue>(args_value)...);
    }

    // try_emplace for KeyTuple directly
    template <typename... ArgsValue>
    std::pair<iterator, bool> try_emplace_tuple(const KeyTuple& key_tuple, ArgsValue&&... args_value) {
        return map_.try_emplace(key_tuple, std::forward<ArgsValue>(args_value)...);
    }

    template <typename... ArgsValue>
    std::pair<iterator, bool> try_emplace_tuple(KeyTuple&& key_tuple, ArgsValue&&... args_value) {
        return map_.try_emplace(std::move(key_tuple), std::forward<ArgsValue>(args_value)...);
    }

    size_type erase(const KeyTypes&... keys) {
        return map_.erase(std::make_tuple(keys...));
    }

    size_type erase_tuple(const KeyTuple& key_tuple) {
        return map_.erase(key_tuple);
    }

    iterator erase(const_iterator pos) {
        return map_.erase(pos);
    }

    iterator erase(iterator pos) {
        return map_.erase(pos);
    }

    // --- Lookup ---

    iterator find(const KeyTypes&... keys) {
        return map_.find(std::make_tuple(keys...));
    }

    const_iterator find(const KeyTypes&... keys) const {
        return map_.find(std::make_tuple(keys...));
    }

    iterator find_tuple(const KeyTuple& key_tuple) {
        return map_.find(key_tuple);
    }

    const_iterator find_tuple(const KeyTuple& key_tuple) const {
        return map_.find(key_tuple);
    }

    Value& at(const KeyTypes&... keys) {
        return map_.at(std::make_tuple(keys...));
    }

    const Value& at(const KeyTypes&... keys) const {
        return map_.at(std::make_tuple(keys...));
    }

    Value& at_tuple(const KeyTuple& key_tuple) {
        return map_.at(key_tuple);
    }

    const Value& at_tuple(const KeyTuple& key_tuple) const {
        return map_.at(key_tuple);
    }

    // operator[] will take a KeyTuple for now.
    // A variadic operator[] returning a proxy is more complex.
    Value& operator[](const KeyTuple& key_tuple) {
        return map_[key_tuple];
    }

    Value& operator[](KeyTuple&& key_tuple) {
        return map_[std::move(key_tuple)];
    }

    // Variadic operator[] using a helper or direct construction.
    // This version will create a default Value if key doesn't exist.
    Value& operator()(const KeyTypes&... keys) {
        return map_[std::make_tuple(keys...)];
    }


    bool contains(const KeyTypes&... keys) const {
        return map_.count(std::make_tuple(keys...)) > 0;
    }

    bool contains_tuple(const KeyTuple& key_tuple) const {
        return map_.count(key_tuple) > 0;
    }

    // Bucket interface
    size_type bucket_count() const { return map_.bucket_count(); }
    size_type max_bucket_count() const { return map_.max_bucket_count(); }
    size_type bucket_size(size_type n) const { return map_.bucket_size(n); }
    size_type bucket(const KeyTypes&... keys) const {
        return map_.bucket(std::make_tuple(keys...));
    }
    size_type bucket_tuple(const KeyTuple& key_tuple) const {
        return map_.bucket(key_tuple);
    }

    // Hash policy
    float load_factor() const { return map_.load_factor(); }
    float max_load_factor() const { return map_.max_load_factor(); }
    void max_load_factor(float ml) { map_.max_load_factor(ml); }
    void rehash(size_type count) { map_.rehash(count); }
    void reserve(size_type count) { map_.reserve(count); }

    // Observers
    hasher hash_function() const { return map_.hash_function(); }
    key_equal key_eq() const { return map_.key_eq(); }

    // Equality
    bool operator==(const MultiKeyMap& other) const {
        return map_ == other.map_;
    }

    bool operator!=(const MultiKeyMap& other) const {
        return !(*this == other);
    }

    void swap(MultiKeyMap& other) noexcept {
        map_.swap(other.map_);
    }
};

// Non-member swap function
template <typename Value, typename... KeyTypes>
void swap(MultiKeyMap<Value, KeyTypes...>& lhs, MultiKeyMap<Value, KeyTypes...>& rhs) noexcept {
    lhs.swap(rhs);
}

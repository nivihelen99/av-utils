#ifndef INVERTED_INDEX_H
#define INVERTED_INDEX_H

#include <unordered_map>
#include <unordered_set>
#include <vector> // For potential future use or examples, not strictly required by current API
#include <string> // For potential future use or examples
#include <algorithm> // For std::remove_if if needed, or other algorithms
#include <utility> // For std::move

// Forward declaration for the iterators
// Not strictly necessary if iterators just wrap underlying map iterators directly
// but good practice if we want more complex iterator logic later.

template <
    typename Key,
    typename Value,
    typename HashKey = std::hash<Key>,
    typename EqualKey = std::equal_to<Key>,
    typename HashValue = std::hash<Value>,
    typename EqualValue = std::equal_to<Value>
>
class InvertedIndex {
public:
    // Types
    using KeySet = std::unordered_set<Key, HashKey, EqualKey>;
    using ValueSet = std::unordered_set<Value, HashValue, EqualValue>;

private:
    using ForwardMapType = std::unordered_map<Key, ValueSet, HashKey, EqualKey>;
    using ReverseMapType = std::unordered_map<Value, KeySet, HashValue, EqualValue>;

    ForwardMapType forward_map_;
    ReverseMapType reverse_map_;

public: // Made public for testing EMPTY_SET references
    // Static empty sets to return by reference for non-existent keys/values
    // This avoids dangling references or the need to return by value.
    static const KeySet EMPTY_KEY_SET;
    static const ValueSet EMPTY_VALUE_SET;

public:
    // Constructors
    InvertedIndex() = default;

    // Copy semantics
    InvertedIndex(const InvertedIndex& other)
        : forward_map_(other.forward_map_), reverse_map_(other.reverse_map_) {}

    InvertedIndex& operator=(const InvertedIndex& other) {
        if (this == &other) {
            return *this;
        }
        forward_map_ = other.forward_map_;
        reverse_map_ = other.reverse_map_;
        return *this;
    }

    // Move semantics
    InvertedIndex(InvertedIndex&& other) noexcept
        : forward_map_(std::move(other.forward_map_)), reverse_map_(std::move(other.reverse_map_)) {}

    InvertedIndex& operator=(InvertedIndex&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        forward_map_ = std::move(other.forward_map_);
        reverse_map_ = std::move(other.reverse_map_);
        return *this;
    }

    // Core methods
    void add(const Key& key, const Value& value) {
        forward_map_[key].insert(value);
        reverse_map_[value].insert(key);
    }

    void remove(const Key& key, const Value& value) {
        auto fwd_it = forward_map_.find(key);
        if (fwd_it != forward_map_.end()) {
            fwd_it->second.erase(value);
            if (fwd_it->second.empty()) {
                forward_map_.erase(fwd_it);
            }
        }

        auto rev_it = reverse_map_.find(value);
        if (rev_it != reverse_map_.end()) {
            rev_it->second.erase(key);
            if (rev_it->second.empty()) {
                reverse_map_.erase(rev_it);
            }
        }
    }

    void remove_key(const Key& key) {
        auto fwd_it = forward_map_.find(key);
        if (fwd_it != forward_map_.end()) {
            const ValueSet& values_to_remove = fwd_it->second;
            for (const auto& value : values_to_remove) {
                auto rev_it = reverse_map_.find(value);
                if (rev_it != reverse_map_.end()) {
                    rev_it->second.erase(key);
                    if (rev_it->second.empty()) {
                        reverse_map_.erase(rev_it);
                    }
                }
            }
            forward_map_.erase(fwd_it);
        }
    }

    void remove_value(const Value& value) {
        auto rev_it = reverse_map_.find(value);
        if (rev_it != reverse_map_.end()) {
            const KeySet& keys_to_remove = rev_it->second;
            for (const auto& key : keys_to_remove) {
                auto fwd_it = forward_map_.find(key);
                if (fwd_it != forward_map_.end()) {
                    fwd_it->second.erase(value);
                    if (fwd_it->second.empty()) {
                        forward_map_.erase(fwd_it);
                    }
                }
            }
            reverse_map_.erase(rev_it);
        }
    }

    const ValueSet& values_for(const Key& key) const {
        auto it = forward_map_.find(key);
        if (it != forward_map_.end()) {
            return it->second;
        }
        return EMPTY_VALUE_SET;
    }

    const KeySet& keys_for(const Value& value) const {
        auto it = reverse_map_.find(value);
        if (it != reverse_map_.end()) {
            return it->second;
        }
        return EMPTY_KEY_SET;
    }

    bool contains(const Key& key, const Value& value) const {
        auto it = forward_map_.find(key);
        if (it != forward_map_.end()) {
            const ValueSet& values = it->second;
            return values.count(value) > 0;
        }
        return false;
    }

    bool empty() const {
        return forward_map_.empty();
    }

    void clear() {
        forward_map_.clear();
        reverse_map_.clear();
    }

    // Iterator support (iterating over keys of the forward_map_)
    typename ForwardMapType::const_iterator begin() const {
        return forward_map_.begin();
    }

    typename ForwardMapType::const_iterator end() const {
        return forward_map_.end();
    }

    // Size information (optional, but common)
    size_t key_count() const { return forward_map_.size(); }
    // size_t value_count() const { return reverse_map_.size(); } // Unique values
    // size_t total_mappings() const; // Could be implemented if needed


};

// Initialize static empty sets
template <typename Key, typename Value, typename HashKey, typename EqualKey, typename HashValue, typename EqualValue>
const typename InvertedIndex<Key, Value, HashKey, EqualKey, HashValue, EqualValue>::KeySet InvertedIndex<Key, Value, HashKey, EqualKey, HashValue, EqualValue>::EMPTY_KEY_SET = {};

template <typename Key, typename Value, typename HashKey, typename EqualKey, typename HashValue, typename EqualValue>
const typename InvertedIndex<Key, Value, HashKey, EqualKey, HashValue, EqualValue>::ValueSet InvertedIndex<Key, Value, HashKey, EqualKey, HashValue, EqualValue>::EMPTY_VALUE_SET = {};

#endif // INVERTED_INDEX_H

#pragma once

#include <unordered_map>
#include <map>
#include <vector>
#include <optional>
#include <stdexcept>
#include <functional> // For std::hash, std::equal_to, std::less
#include <memory>     // For std::allocator
#include <algorithm>  // For std::max_element, std::prev
#include <utility>    // For std::pair, std::move

namespace cpp_collections {

template <
    typename Key,
    typename Value,
    typename VersionT = uint64_t,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    typename CompareVersions = std::less<VersionT>,
    typename Allocator = std::allocator<std::pair<const Key, std::map<VersionT, Value, CompareVersions>>>
>
class ValueVersionedMap {
public:
    using key_type = Key;
    using mapped_type = Value;
    using version_type = VersionT;
    using version_value_pair = std::pair<const VersionT, Value>;
    using version_map_type = std::map<VersionT, Value, CompareVersions>; // Consider allocator for this map's pairs if needed

    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

private:
    using internal_storage_value_type = version_map_type;
    using internal_map_type = std::unordered_map<Key, internal_storage_value_type, Hash, KeyEqual, Allocator>;
    internal_map_type data_;
    CompareVersions version_comparator_; // Store an instance if needed for operations outside map

public:
    // Constructors
    ValueVersionedMap(const CompareVersions& comp_versions = CompareVersions(),
                      const Hash& hash = Hash(),
                      const KeyEqual& key_eq = KeyEqual(),
                      const Allocator& alloc = Allocator())
        : data_(0, hash, key_eq, alloc), version_comparator_(comp_versions) {}

    explicit ValueVersionedMap(const Allocator& alloc)
        : data_(alloc), version_comparator_() {}

    // TODO: Add other constructors as needed (e.g., from iterators, initializer_list)

    // Modifiers
    void put(const Key& key, const Value& value, const VersionT& version) {
        data_[key].insert_or_assign(version, value);
    }

    void put(const Key& key, Value&& value, const VersionT& version) {
        data_[key].insert_or_assign(version, std::move(value));
    }

    void put(Key&& key, const Value& value, const VersionT& version) {
        data_[std::move(key)].insert_or_assign(version, value);
    }

    void put(Key&& key, Value&& value, const VersionT& version) {
        data_[std::move(key)].insert_or_assign(version, std::move(value));
    }

    // Lookup
    std::optional<std::reference_wrapper<const Value>> get(const Key& key, const VersionT& version) const {
        auto key_it = data_.find(key);
        if (key_it == data_.end()) {
            return std::nullopt;
        }

        const auto& versions = key_it->second;
        if (versions.empty()) {
            return std::nullopt;
        }

        // Find the latest version <= requested version
        auto version_it = versions.upper_bound(version);

        if (version_it == versions.begin()) {
            // All versions are > requested version, or versions.upper_bound(version) is begin()
            // meaning no version is <= requested_version
            return std::nullopt;
        }
        // upper_bound gives first element > version. We need the one before that.
        --version_it;
        return std::cref(version_it->second);
    }

    std::optional<std::reference_wrapper<Value>> get(const Key& key, const VersionT& version) {
        auto key_it = data_.find(key);
        if (key_it == data_.end()) {
            return std::nullopt;
        }

        auto& versions = key_it->second;
        if (versions.empty()) {
            return std::nullopt;
        }

        auto version_it = versions.upper_bound(version);
        if (version_it == versions.begin()) {
            return std::nullopt;
        }
        --version_it;
        return std::ref(version_it->second);
    }

    std::optional<std::reference_wrapper<const Value>> get_exact(const Key& key, const VersionT& version) const {
        auto key_it = data_.find(key);
        if (key_it == data_.end()) {
            return std::nullopt;
        }
        const auto& versions = key_it->second;
        auto version_it = versions.find(version);
        if (version_it == versions.end()) {
            return std::nullopt;
        }
        return std::cref(version_it->second);
    }

    std::optional<std::reference_wrapper<Value>> get_exact(const Key& key, const VersionT& version) {
        auto key_it = data_.find(key);
        if (key_it == data_.end()) {
            return std::nullopt;
        }
        auto& versions = key_it->second;
        auto version_it = versions.find(version);
        if (version_it == versions.end()) {
            return std::nullopt;
        }
        return std::ref(version_it->second);
    }

    std::optional<std::reference_wrapper<const Value>> get_latest(const Key& key) const {
        auto key_it = data_.find(key);
        if (key_it == data_.end() || key_it->second.empty()) {
            return std::nullopt;
        }
        return std::cref(key_it->second.rbegin()->second); // rbegin() gives the largest version
    }

    std::optional<std::reference_wrapper<Value>> get_latest(const Key& key) {
        auto key_it = data_.find(key);
        if (key_it == data_.end() || key_it->second.empty()) {
            return std::nullopt;
        }
        return std::ref(key_it->second.rbegin()->second);
    }

    std::optional<std::reference_wrapper<const version_map_type>> get_all_versions(const Key& key) const {
        auto key_it = data_.find(key);
        if (key_it == data_.end()) {
            return std::nullopt;
        }
        return std::cref(key_it->second);
    }

    std::optional<std::reference_wrapper<version_map_type>> get_all_versions(const Key& key) {
        auto key_it = data_.find(key);
        if (key_it == data_.end()) {
            return std::nullopt;
        }
        return std::ref(key_it->second);
    }

    // Removal
    bool remove_key(const Key& key) {
        return data_.erase(key) > 0;
    }

    bool remove_version(const Key& key, const VersionT& version) {
        auto key_it = data_.find(key);
        if (key_it == data_.end()) {
            return false;
        }
        bool erased = key_it->second.erase(version) > 0;
        if (erased && key_it->second.empty()) {
            data_.erase(key_it); // Remove key if no versions are left
        }
        return erased;
    }

    // Capacity
    bool empty() const noexcept {
        return data_.empty();
    }

    size_type size() const noexcept {
        return data_.size(); // Number of unique keys
    }

    size_type total_versions() const noexcept {
        size_type count = 0;
        for(const auto& pair : data_) {
            count += pair.second.size();
        }
        return count;
    }

    void clear() noexcept {
        data_.clear();
    }

    // Lookup
    bool contains_key(const Key& key) const {
        return data_.count(key) > 0;
    }

    bool contains_version(const Key& key, const VersionT& version) const {
        auto key_it = data_.find(key);
        if (key_it == data_.end()) {
            return false;
        }
        return key_it->second.count(version) > 0;
    }

    // Key and Version Iteration
    std::vector<Key> keys() const {
        std::vector<Key> key_list;
        key_list.reserve(data_.size());
        for (const auto& pair : data_) {
            key_list.push_back(pair.first);
        }
        return key_list;
    }

    std::optional<std::vector<VersionT>> versions(const Key& key) const {
        auto key_it = data_.find(key);
        if (key_it == data_.end()) {
            return std::nullopt;
        }
        std::vector<VersionT> version_list;
        version_list.reserve(key_it->second.size());
        for (const auto& pair : key_it->second) {
            version_list.push_back(pair.first);
        }
        return version_list;
    }

    // Observers
    hasher hash_function() const { return data_.hash_function(); }
    key_equal key_eq() const { return data_.key_eq(); }
    CompareVersions version_comp() const { return version_comparator_; }
    allocator_type get_allocator() const { return data_.get_allocator(); }

    // Iterators (basic pass-through to underlying map's iterators for keys and their version maps)
    // These iterators will yield std::pair<const Key, version_map_type>
    // Users can then iterate over version_map_type if needed.
    // A more complex iterator yielding Key, latest_Value could be implemented if desired.

    typename internal_map_type::iterator begin() noexcept { return data_.begin(); }
    typename internal_map_type::const_iterator begin() const noexcept { return data_.begin(); }
    typename internal_map_type::const_iterator cbegin() const noexcept { return data_.cbegin(); }

    typename internal_map_type::iterator end() noexcept { return data_.end(); }
    typename internal_map_type::const_iterator end() const noexcept { return data_.end(); }
    typename internal_map_type::const_iterator cend() const noexcept { return data_.cend(); }

    // Swap
    void swap(ValueVersionedMap& other) noexcept(
        std::allocator_traits<Allocator>::propagate_on_container_swap::value ||
        std::allocator_traits<Allocator>::is_always_equal::value)
    {
        using std::swap;
        swap(data_, other.data_);
        swap(version_comparator_, other.version_comparator_);
    }

    // Comparison operators
    // Equality compares if both maps have the same set of keys, and for each key,
    // the same set of (version, value) pairs.
    friend bool operator==(const ValueVersionedMap& lhs, const ValueVersionedMap& rhs) {
        if (lhs.data_.size() != rhs.data_.size()) {
            return false;
        }
        for (const auto& lhs_pair : lhs.data_) {
            auto rhs_it = rhs.data_.find(lhs_pair.first);
            if (rhs_it == rhs.data_.end()) {
                return false; // Key in lhs not in rhs
            }
            // Compare the map of versions (std::map has operator==)
            if (lhs_pair.second != rhs_it->second) {
                return false; // Version maps differ for the same key
            }
        }
        // Could optionally compare version_comparator_ if its state matters for equality
        return true;
    }

    friend bool operator!=(const ValueVersionedMap& lhs, const ValueVersionedMap& rhs) {
        return !(lhs == rhs);
    }
};

// Non-member swap
template <typename K, typename V, typename Ver, typename H, typename KE, typename CV, typename A>
void swap(ValueVersionedMap<K, V, Ver, H, KE, CV, A>& lhs,
          ValueVersionedMap<K, V, Ver, H, KE, CV, A>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace cpp_collections

#pragma once

#include <map>
#include <unordered_map>
#include <functional>
#include <type_traits>

namespace deltamap {

/**
 * @brief A utility class for computing differences between two map-like containers
 * 
 * DeltaMap compares two key-value mappings and categorizes the differences into:
 * - Added: entries present in new_map but not in old_map
 * - Removed: entries present in old_map but not in new_map  
 * - Changed: entries present in both maps but with different values
 * - Unchanged: entries present in both maps with identical values
 * 
 * @tparam K Key type
 * @tparam V Value type
 * @tparam MapType Container type (defaults to std::map<K,V>)
 * @tparam Equal Value equality comparator type
 */
template <typename K, typename V, 
          typename MapType = std::map<K, V>,
          typename Equal = std::equal_to<V>>
class DeltaMap {
public:
    using key_type = K;
    using mapped_type = V;
    using value_type = typename MapType::value_type;
    using map_type = MapType;
    using equal_type = Equal;

private:
    MapType added_;
    MapType removed_;
    MapType changed_;
    MapType unchanged_;
    Equal equal_;

    // Helper to detect if MapType is ordered (like std::map) vs unordered (like std::unordered_map)
    template<typename T>
    struct is_ordered_map {
        static constexpr bool value = std::is_same_v<T, std::map<K, V>> ||
                                     std::is_same_v<T, std::multimap<K, V>>;
    };

    // Optimized comparison for ordered maps (std::map)
    void compute_diff_ordered(const MapType& old_map, const MapType& new_map) {
        auto old_it = old_map.begin();
        auto new_it = new_map.begin();
        
        while (old_it != old_map.end() && new_it != new_map.end()) {
            if (old_it->first < new_it->first) {
                // Key only in old_map - removed
                removed_.emplace(*old_it);
                ++old_it;
            } else if (new_it->first < old_it->first) {
                // Key only in new_map - added
                added_.emplace(*new_it);
                ++new_it;
            } else {
                // Key in both maps - check values
                if (equal_(old_it->second, new_it->second)) {
                    unchanged_.emplace(*new_it);
                } else {
                    changed_.emplace(*new_it);
                }
                ++old_it;
                ++new_it;
            }
        }
        
        // Handle remaining elements
        while (old_it != old_map.end()) {
            removed_.emplace(*old_it);
            ++old_it;
        }
        
        while (new_it != new_map.end()) {
            added_.emplace(*new_it);
            ++new_it;
        }
    }

    // General comparison for unordered maps
    void compute_diff_unordered(const MapType& old_map, const MapType& new_map) {
        // First pass: find removed and changed/unchanged
        for (const auto& [key, old_value] : old_map) {
            auto new_it = new_map.find(key);
            if (new_it == new_map.end()) {
                // Key only in old_map - removed
                removed_.emplace(key, old_value);
            } else {
                // Key in both maps - check values
                if (equal_(old_value, new_it->second)) {
                    unchanged_.emplace(key, new_it->second);
                } else {
                    changed_.emplace(key, new_it->second);
                }
            }
        }
        
        // Second pass: find added
        for (const auto& [key, new_value] : new_map) {
            if (old_map.find(key) == old_map.end()) {
                // Key only in new_map - added
                added_.emplace(key, new_value);
            }
        }
    }

public:
    /**
     * @brief Construct a DeltaMap by comparing two maps
     * 
     * @param old_map The original/previous state map
     * @param new_map The new/current state map
     * @param equal Custom equality comparator for values (optional)
     */
    explicit DeltaMap(const MapType& old_map, 
                     const MapType& new_map,
                     Equal equal = Equal{})
        : equal_(std::move(equal)) {
        
        if constexpr (is_ordered_map<MapType>::value) {
            compute_diff_ordered(old_map, new_map);
        } else {
            compute_diff_unordered(old_map, new_map);
        }
    }

    /**
     * @brief Get entries that exist only in the new map
     * @return const reference to map containing added entries
     */
    const MapType& added() const noexcept {
        return added_;
    }

    /**
     * @brief Get entries that exist only in the old map
     * @return const reference to map containing removed entries
     */
    const MapType& removed() const noexcept {
        return removed_;
    }

    /**
     * @brief Get entries that exist in both maps but have different values
     * @return const reference to map containing changed entries (with new values)
     */
    const MapType& changed() const noexcept {
        return changed_;
    }

    /**
     * @brief Get entries that exist in both maps with identical values
     * @return const reference to map containing unchanged entries
     */
    const MapType& unchanged() const noexcept {
        return unchanged_;
    }

    /**
     * @brief Check if there are any differences between the maps
     * @return true if maps are identical, false otherwise
     */
    bool empty() const noexcept {
        return added_.empty() && removed_.empty() && changed_.empty();
    }

    /**
     * @brief Get the total number of differences
     * @return sum of added, removed, and changed entries
     */
    std::size_t size() const noexcept {
        return added_.size() + removed_.size() + changed_.size();
    }

    /**
     * @brief Check if a specific key was added
     * @param key The key to check
     * @return true if key was added, false otherwise
     */
    bool was_added(const K& key) const {
        return added_.find(key) != added_.end();
    }

    /**
     * @brief Check if a specific key was removed
     * @param key The key to check
     * @return true if key was removed, false otherwise
     */
    bool was_removed(const K& key) const {
        return removed_.find(key) != removed_.end();
    }

    /**
     * @brief Check if a specific key was changed
     * @param key The key to check
     * @return true if key was changed, false otherwise
     */
    bool was_changed(const K& key) const {
        return changed_.find(key) != changed_.end();
    }

    /**
     * @brief Check if a specific key was unchanged
     * @param key The key to check
     * @return true if key was unchanged, false otherwise
     */
    bool was_unchanged(const K& key) const {
        return unchanged_.find(key) != unchanged_.end();
    }

    /**
     * @brief Create an inverted delta (swap old and new perspectives)
     * @param old_map The original old map used in construction
     * @param new_map The original new map used in construction
     * @return DeltaMap with swapped perspective
     */
    DeltaMap invert(const MapType& old_map, const MapType& new_map) const {
        return DeltaMap(new_map, old_map, equal_);
    }

    /**
     * @brief Apply this delta to a map to produce the target state
     * @param base_map The map to apply changes to
     * @return New map with delta applied
     */
    MapType apply_to(MapType base_map) const {
        // Remove deleted entries
        for (const auto& [key, value] : removed_) {
            base_map.erase(key);
        }
        
        // Add new entries
        for (const auto& [key, value] : added_) {
            base_map[key] = value;
        }
        
        // Update changed entries
        for (const auto& [key, value] : changed_) {
            base_map[key] = value;
        }
        
        return base_map;
    }
};

// Type deduction helpers for C++17
template<typename K, typename V>
DeltaMap(const std::map<K, V>&, const std::map<K, V>&) 
    -> DeltaMap<K, V, std::map<K, V>>;

template<typename K, typename V>
DeltaMap(const std::unordered_map<K, V>&, const std::unordered_map<K, V>&) 
    -> DeltaMap<K, V, std::unordered_map<K, V>>;

template<typename K, typename V, typename Equal>
DeltaMap(const std::map<K, V>&, const std::map<K, V>&, Equal) 
    -> DeltaMap<K, V, std::map<K, V>, Equal>;

template<typename K, typename V, typename Equal>
DeltaMap(const std::unordered_map<K, V>&, const std::unordered_map<K, V>&, Equal) 
    -> DeltaMap<K, V, std::unordered_map<K, V>, Equal>;

} // namespace deltamap

#pragma once

#include <map>
#include <vector>
#include <type_traits> // For std::is_arithmetic_v, std::is_unsigned_v, std::is_floating_point_v
#include <utility>     // For std::pair, std::move
#include <algorithm>   // For std::max, std::min (not directly used here but good include for context)
#include <cmath>       // For std::abs
#include <limits>      // For std::numeric_limits

namespace cpp_utils {

template <typename KeyType, typename ValueType>
class MagnitudeMap {
public:
    static_assert(std::is_arithmetic_v<KeyType>, "MagnitudeMap KeyType must be an arithmetic type.");

    using PairType = std::pair<KeyType, ValueType>;
    using StorageType = std::map<KeyType, ValueType>;

    // Constructors
    MagnitudeMap() = default;

    // Modifiers
    void insert(const KeyType& key, const ValueType& value) {
        data_[key] = value;
    }

    void insert(const KeyType& key, ValueType&& value) {
        data_[key] = std::move(value);
    }

    void insert(KeyType&& key, const ValueType& value) {
        data_[std::move(key)] = value;
    }

    void insert(KeyType&& key, ValueType&& value) {
        data_[std::move(key)] = std::move(value);
    }

    bool remove(const KeyType& key) {
        return data_.erase(key) > 0;
    }

    // Accessors
    ValueType* get(const KeyType& key) {
        auto it = data_.find(key);
        if (it == data_.end()) {
            return nullptr;
        }
        return &(it->second);
    }

    const ValueType* get(const KeyType& key) const {
        auto it = data_.find(key);
        if (it == data_.end()) {
            return nullptr;
        }
        return &(it->second);
    }

    bool contains(const KeyType& key) const {
        return data_.count(key) > 0;
    }

    size_t size() const {
        return data_.size();
    }

    bool empty() const {
        return data_.empty();
    }

    std::vector<PairType> find_within_magnitude(const KeyType& query_key, const KeyType& magnitude_param) const {
        std::vector<PairType> result;
        if (data_.empty()) {
            return result;
        }

        KeyType actual_magnitude = magnitude_param;
        if constexpr (std::is_signed_v<KeyType> || std::is_floating_point_v<KeyType>) {
            if (magnitude_param < KeyType{0}) {
                actual_magnitude = KeyType{0};
            }
        } else { // Unsigned types are always >= 0
             // No change needed for actual_magnitude unless we want to disallow large magnitudes for unsigned
        }


        KeyType map_search_lower_key;
        KeyType map_search_upper_key;

        if constexpr (std::is_floating_point_v<KeyType>) {
            map_search_lower_key = query_key - actual_magnitude;
            map_search_upper_key = query_key + actual_magnitude;
        } else if constexpr (std::is_unsigned_v<KeyType>) {
            map_search_lower_key = (query_key > actual_magnitude) ? (query_key - actual_magnitude) : KeyType{0};
            map_search_upper_key = (std::numeric_limits<KeyType>::max() - actual_magnitude < query_key) ? std::numeric_limits<KeyType>::max() : (query_key + actual_magnitude);
        } else { // Signed integral types
            // Lower bound calculation
            if (query_key >= KeyType{0}) { // query_key is positive or zero
                if (actual_magnitude > KeyType{0} && query_key < std::numeric_limits<KeyType>::min() + actual_magnitude) {
                    map_search_lower_key = std::numeric_limits<KeyType>::min();
                } else {
                    map_search_lower_key = query_key - actual_magnitude;
                }
            } else { // query_key is negative
                if (actual_magnitude > KeyType{0} && query_key < std::numeric_limits<KeyType>::min() + actual_magnitude) {
                    map_search_lower_key = std::numeric_limits<KeyType>::min();
                } else {
                     // If actual_magnitude is 0, or query_key is not too close to min
                     // query_key - 0 = query_key
                     // query_key - positive_magnitude = smaller_number
                    map_search_lower_key = query_key - actual_magnitude;
                }
            }

            // Upper bound calculation
            if (query_key <= KeyType{0}) { // query_key is negative or zero
                 if (actual_magnitude > KeyType{0} && query_key > std::numeric_limits<KeyType>::max() - actual_magnitude) {
                    map_search_upper_key = std::numeric_limits<KeyType>::max();
                } else {
                    // query_key + 0 = query_key
                    // query_key + positive_magnitude = larger_number (closer to zero or positive)
                    map_search_upper_key = query_key + actual_magnitude;
                }
            } else { // query_key is positive
                if (actual_magnitude > KeyType{0} && query_key > std::numeric_limits<KeyType>::max() - actual_magnitude) {
                    map_search_upper_key = std::numeric_limits<KeyType>::max();
                } else {
                    map_search_upper_key = query_key + actual_magnitude;
                }
            }
        }

        auto it_low = data_.lower_bound(map_search_lower_key);
        auto it_high = data_.upper_bound(map_search_upper_key);

        for (auto it = it_low; it != it_high; ++it) {
            const KeyType& current_key = it->first;
            KeyType diff;

            // Calculate absolute difference: std::abs can be problematic with unsigned underflow
            // if current_key < query_key and KeyType is unsigned.
            if (current_key >= query_key) {
                diff = current_key - query_key;
            } else {
                diff = query_key - current_key;
            }

            if (diff <= actual_magnitude) {
                 result.push_back(*it);
            }
        }
        return result;
    }

private:
    StorageType data_;
};

} // namespace cpp_utils

#pragma once

#include <map>
#include <vector>
#include <string>
#include <random>
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <functional>
#include <limits> // Required for std::numeric_limits

namespace cpp_collections {

template <
    typename Key,
    typename WeightType = double,
    typename Compare = std::less<Key>
>
class WeightedSet {
public:
    using key_type = Key;
    using weight_type = WeightType;
    using key_compare = Compare;
    using value_type = std::pair<const Key, WeightType>; // For iterators over item_weights_
    using map_type = std::map<Key, WeightType, Compare>;
    using size_type = typename map_type::size_type;

private:
    map_type item_weights_;

    mutable std::vector<std::pair<Key, WeightType>> cumulative_items_;
    mutable WeightType total_weight_for_sampling_;
    mutable bool stale_sampling_data_;
    mutable std::mt19937 random_engine_;

    void rebuild_sampling_data_() const {
        cumulative_items_.clear();
        // Optimization: Only reserve if item_weights_ is not empty.
        if (!item_weights_.empty()) {
            cumulative_items_.reserve(item_weights_.size());
        }

        WeightType current_cumulative_weight = static_cast<WeightType>(0);
        for (const auto& pair : item_weights_) {
            // Only include items with strictly positive weight in the sampling distribution
            if (pair.second > static_cast<WeightType>(0)) {
                current_cumulative_weight += pair.second;
                cumulative_items_.emplace_back(pair.first, current_cumulative_weight);
            }
        }
        total_weight_for_sampling_ = current_cumulative_weight;
        stale_sampling_data_ = false;
    }

public:
    using iterator = typename map_type::iterator;
    using const_iterator = typename map_type::const_iterator;

    WeightedSet()
        : item_weights_(),
          total_weight_for_sampling_(static_cast<WeightType>(0)),
          stale_sampling_data_(true) {
        std::random_device rd;
        random_engine_.seed(rd());
    }

    explicit WeightedSet(const key_compare& comp)
        : item_weights_(comp),
          total_weight_for_sampling_(static_cast<WeightType>(0)),
          stale_sampling_data_(true) {
        std::random_device rd;
        random_engine_.seed(rd());
    }

    template<typename InputIt>
    WeightedSet(InputIt first, InputIt last, const key_compare& comp = key_compare())
        : item_weights_(comp),
          total_weight_for_sampling_(static_cast<WeightType>(0)),
          stale_sampling_data_(true) {
        std::random_device rd;
        random_engine_.seed(rd());
        for (InputIt it = first; it != last; ++it) {
            // Assuming InputIt dereferences to a pair compatible with add's arguments
            add(it->first, it->second);
        }
        // add() sets stale_sampling_data_, initial state is true anyway.
    }

    WeightedSet(std::initializer_list<value_type> ilist, const key_compare& comp = key_compare())
        : item_weights_(comp),
          total_weight_for_sampling_(static_cast<WeightType>(0)),
          stale_sampling_data_(true) {
        std::random_device rd;
        random_engine_.seed(rd());
        for (const auto& item : ilist) {
            add(item.first, item.second);
        }
    }

    WeightedSet(const WeightedSet& other)
        : item_weights_(other.item_weights_),
          total_weight_for_sampling_(static_cast<WeightType>(0)), // To be rebuilt
          stale_sampling_data_(true), // Mark as stale, will be rebuilt on sample
          random_engine_() { // Initialize new engine
        std::random_device rd;
        random_engine_.seed(rd());
    }

    WeightedSet(WeightedSet&& other) noexcept
        : item_weights_(std::move(other.item_weights_)),
          cumulative_items_(std::move(other.cumulative_items_)),
          total_weight_for_sampling_(other.total_weight_for_sampling_),
          stale_sampling_data_(other.stale_sampling_data_),
          random_engine_(std::move(other.random_engine_)) {
        other.stale_sampling_data_ = true; // Leave other in a valid but stale state
        other.total_weight_for_sampling_ = static_cast<WeightType>(0);
    }

    WeightedSet& operator=(const WeightedSet& other) {
        if (this == &other) return *this;
        item_weights_ = other.item_weights_;
        cumulative_items_.clear(); // Clear local cache
        total_weight_for_sampling_ = static_cast<WeightType>(0);
        stale_sampling_data_ = true;
        // random_engine_ can keep its current seed or be re-seeded
        // std::random_device rd; random_engine_.seed(rd());
        return *this;
    }

    WeightedSet& operator=(WeightedSet&& other) noexcept {
        if (this == &other) return *this;
        item_weights_ = std::move(other.item_weights_);
        cumulative_items_ = std::move(other.cumulative_items_);
        total_weight_for_sampling_ = other.total_weight_for_sampling_;
        stale_sampling_data_ = other.stale_sampling_data_;
        random_engine_ = std::move(other.random_engine_);

        other.stale_sampling_data_ = true;
        other.total_weight_for_sampling_ = static_cast<WeightType>(0);
        other.cumulative_items_.clear();
        return *this;
    }

    void add(const key_type& key, weight_type weight) {
        if (weight <= static_cast<weight_type>(0)) {
            // If adding with non-positive weight, key is effectively removed or not added.
            // Call remove to ensure stale_sampling_data_ is flagged if key existed.
            remove(key);
        } else {
            // Insert or update the weight
            auto result = item_weights_.insert_or_assign(key, weight);
            // No matter insert or assign, data is now stale for sampling.
            stale_sampling_data_ = true;
        }
    }

    void add(key_type&& key, weight_type weight) {
        if (weight <= static_cast<weight_type>(0)) {
            remove(std::move(key));
        } else {
            // Use item_weights_.insert_or_assign for rvalue key as well
            item_weights_.insert_or_assign(std::move(key), weight);
            stale_sampling_data_ = true;
        }
    }

    bool remove(const key_type& key) {
        if (item_weights_.erase(key) > 0) {
            stale_sampling_data_ = true;
            return true;
        }
        return false;
    }

    // Overload for rvalue key for remove, if map supports it (std::map::erase(K&&) is C++23)
    // For now, relying on const Key& version is fine.

    weight_type get_weight(const key_type& key) const {
        const_iterator it = item_weights_.find(key);
        if (it != item_weights_.end()) {
            return it->second; // Returns the stored weight
        }
        return static_cast<weight_type>(0); // Key not found or weight is not positive
    }

    bool contains(const key_type& key) const {
        // Check if key exists and has a positive weight considered by the structure.
        // However, `item_weights_` only stores items added with positive weight due to `add` logic.
        // So, a simple count is sufficient.
        return item_weights_.count(key) > 0;
    }

    const key_type& sample() const {
        if (item_weights_.empty()) {
            throw std::out_of_range("Cannot sample from an empty WeightedSet.");
        }
        if (stale_sampling_data_) {
            rebuild_sampling_data_();
        }
        // After rebuild, total_weight_for_sampling_ reflects sum of positive weights.
        if (total_weight_for_sampling_ <= static_cast<weight_type>(0)) {
            // This implies no items have positive weight
            throw std::out_of_range("Cannot sample when total positive weight is zero or negative.");
        }
        // cumulative_items_ should not be empty if total_weight_for_sampling_ > 0
        if (cumulative_items_.empty()) {
             throw std::logic_error("Internal error: cumulative_items_ empty despite positive total_weight_for_sampling_.");
        }

        // Always use double for the distribution to satisfy std::uniform_real_distribution requirements.
        // Cast total_weight_for_sampling_ to double for the upper bound.
        // The distribution is [min, max), so random_val will be < total_weight_for_sampling_ (as double).
        std::uniform_real_distribution<double> dist(
            0.0,
            static_cast<double>(total_weight_for_sampling_)
        );
        double random_val = dist(random_engine_);

        // Ensure random_val is not exactly total_weight_for_sampling_ if the distribution is [min, max)
        // and lower_bound would go past the end.
        // For std::uniform_real_distribution, it's [a, b). So random_val < total_weight_for_sampling_
        // (unless total_weight_for_sampling_ is 0, handled above).

        auto it = std::lower_bound(cumulative_items_.begin(), cumulative_items_.end(), random_val,
            [](const std::pair<Key, WeightType>& elem, WeightType val) {
                // Find first element where cumulative weight is >= val.
                // So, keep searching if elem.second < val.
                return elem.second < val;
            });

        // If random_val is exactly 0, lower_bound correctly returns begin().
        // If random_val is very close to total_weight_for_sampling_ (but less),
        // it should correctly point to the last element.
        if (it == cumulative_items_.end()) {
            // This should ideally not happen if total_weight_for_sampling_ > 0
            // and random_val is from a [0, total_weight_for_sampling_) range.
            // Could happen if all weights are extremely small, leading to precision issues,
            // or if random_val somehow equals total_weight_for_sampling_ (dist should be [0, T)).
            // Fallback to the last item if list is not empty.
            if (!cumulative_items_.empty()) {
                 return cumulative_items_.back().first;
            }
             // This indicates a logic flaw or extreme edge case with weights.
            throw std::logic_error("Internal error: sampling failed, lower_bound returned end unexpectedly.");
        }
        return it->first;
    }

    bool empty() const noexcept {
        return item_weights_.empty();
    }

    size_type size() const noexcept {
        return item_weights_.size();
    }

    weight_type total_weight() const noexcept {
        // Calculate sum of (positive) weights currently in item_weights_.
        // This is the true sum, distinct from total_weight_for_sampling_ cache.
        WeightType current_total = static_cast<WeightType>(0);
        for (const auto& pair : item_weights_) {
            // add() ensures only positive weights are stored.
            current_total += pair.second;
        }
        return current_total;
    }

    iterator begin() noexcept { return item_weights_.begin(); }
    const_iterator begin() const noexcept { return item_weights_.cbegin(); } // Corrected to cbegin
    const_iterator cbegin() const noexcept { return item_weights_.cbegin(); }

    iterator end() noexcept { return item_weights_.end(); }
    const_iterator end() const noexcept { return item_weights_.cend(); } // Corrected to cend
    const_iterator cend() const noexcept { return item_weights_.cend(); }

    key_compare key_comp() const {
        return item_weights_.key_comp();
    }

    void swap(WeightedSet& other) noexcept(
        /* std::is_nothrow_swappable_v<map_type> && // C++17
        std::is_nothrow_swappable_v<std::vector<std::pair<Key, WeightType>>> &&
        std::is_nothrow_swappable_v<WeightType> &&
        std::is_nothrow_swappable_v<bool> &&
        std::is_nothrow_swappable_v<std::mt19937> */
        // Simplified noexcept, map swap is complex. Rely on map's allocator propagation.
        std::allocator_traits<typename map_type::allocator_type>::propagate_on_container_swap::value ||
        std::allocator_traits<typename map_type::allocator_type>::is_always_equal::value
    ) {
        using std::swap;
        swap(item_weights_, other.item_weights_);
        swap(cumulative_items_, other.cumulative_items_);
        swap(total_weight_for_sampling_, other.total_weight_for_sampling_);
        swap(stale_sampling_data_, other.stale_sampling_data_);
        swap(random_engine_, other.random_engine_);
    }
};

template <typename K, typename W, typename C>
void swap(WeightedSet<K, W, C>& lhs, WeightedSet<K, W, C>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace cpp_collections

#pragma once

#include <vector>
#include <random>
#include <cmath>      // For std::pow, std::log, std::exp
#include <algorithm>  // For std::sort (potentially, if not using multiset), std::min_element
#include <set>        // For std::multiset
#include <functional> // For std::less
#include <stdexcept>  // For std::invalid_argument
#include <limits>     // For std::numeric_limits

namespace cpp_utils {

/**
 * @brief Implements weighted reservoir sampling using the A-ExpJ algorithm.
 *
 * This class allows for selecting a fixed-size sample of 'k' items from a stream
 * of items, where each item has an associated weight. The probability of an item
 * being included in the final sample is biased by its weight, with higher-weighted
 * items being more likely to be selected. This is particularly useful when the total
 * number of items in the stream is unknown or very large.
 *
 * The A-ExpJ algorithm works by assigning a random key to each incoming item, calculated as
 * `key = exp(log(u) / weight)`, where `u` is a random number uniformly drawn from (0,1).
 * The reservoir maintains the `k` items with the largest keys seen so far.
 *
 * @tparam T The type of items to be sampled.
 * @tparam WeightType The numerical type used for item weights (e.g., double, float, int). Defaults to double.
 *                     Weights must be positive. Non-positive weights cause items to be ignored.
 * @tparam URBG The type of the uniform random bit generator to be used. Defaults to std::mt19937.
 */
template<
    typename T,
    typename WeightType = double,
    typename URBG = std::mt19937
>
class WeightedReservoirSampler {
public:
    using item_type = T;
    using weight_type = WeightType;

private:
    struct ReservoirItem {
        T item;
        double key; // Using double for key as it results from u^(1/w) or exp(log(u)/w)

        // Need to store the original weight if we want to provide it later,
        // or if the key calculation needs it beyond initial generation.
        // For A-ExpJ, the key itself is sufficient for comparisons.

        // Comparator for std::multiset (sorts by key, ascending)
        // Friend declaration is one way, or make it a public nested struct.
        // Or define a standalone comparator struct.
        friend bool operator<(const ReservoirItem& a, const ReservoirItem& b) {
            if (a.key != b.key) {
                return a.key < b.key;
            }
            // Tie-breaking: arbitrary but consistent if needed.
            // For std::multiset, if keys are identical, elements are considered equivalent
            // for ordering but distinct instances can be stored.
            // If T is comparable, could use it, otherwise pointer comparison of item.
            // However, key collisions with doubles from random sources are extremely rare.
            // If T is not default constructible or easily comparable, this could be an issue.
            // Let's assume key uniqueness is sufficient for now.
            // If T is a pointer or complex type, &a.item < &b.item might not be stable
            // across different runs or if items are moved.
            // For simplicity, relying on key for ordering.
            return false; // Important for std::multiset to keep items with same key distinct.
                          // No, this is wrong. If a.key == b.key, they are equivalent for ordering.
                          // To allow multiple items with the same key, they are just inserted.
                          // The < operator should define a strict weak ordering.
                          // if a.key == b.key, then !(a < b) && !(b < a) must hold.
                          // The default std::less<ReservoirItem> would require ReservoirItem itself to have operator<.
                          // So, custom comparator is better.
        }
         // For heterogeneous lookup if needed (C++20)
        // using is_transparent = void;
    };

    struct ReservoirItemComparator {
        bool operator()(const ReservoirItem& a, const ReservoirItem& b) const {
            if (a.key != b.key) {
                return a.key < b.key;
            }
            // If keys are equal, provide a consistent tie-breaking mechanism
            // if we need to distinguish items that generated the exact same key.
            // This is highly unlikely for double keys from random sources.
            // If T is comparable: return a.item < b.item;
            // If not, could fall back to comparing addresses if items are stable.
            // For now, assume key collisions are negligible or unproblematic.
            // Standard behavior for multiset is fine if keys are "equal enough".
            return false; // Fallback for strict weak ordering if keys are equal
        }
    };


    size_t k_; // Reservoir capacity
    std::multiset<ReservoirItem, ReservoirItemComparator> reservoir_;

    URBG random_generator_;
    std::uniform_real_distribution<double> unit_distribution_;

    double generate_random_unit_value() {
        double u = 0.0;
        // Ensure u is in (0, 1) for log(u)
        // std::uniform_real_distribution generates in [min, max).
        // So, it could generate 0.0.
        do {
            u = unit_distribution_(random_generator_);
        } while (u == 0.0); // Extremely unlikely to loop more than once for double.
        return u;
    }

    double calculate_key(WeightType weight) {
        if (weight <= 0) {
            // This case should be handled by the caller (add method)
            // but as a safeguard:
            throw std::logic_error("Weight must be positive to calculate a key.");
        }
        double u = generate_random_unit_value();
        // Key k_i = u_i ^ (1/w_i) or exp(log(u_i) / w_i)
        // Using exp(log(u)/w) is generally more numerically stable.
        return std::exp(std::log(u) / static_cast<double>(weight));
    }

public:
    /**
     * @brief Constructs a WeightedReservoirSampler.
     * @param k The desired size of the sample reservoir. If k is 0, the sampler will always be empty.
     * @param seed The seed for the random number generator. Defaults to a seed from std::random_device.
     */
    explicit WeightedReservoirSampler(size_t k, unsigned int seed = std::random_device{}())
        : k_(k),
          random_generator_(seed),
          unit_distribution_(0.0, 1.0) // Generates in [0.0, 1.0)
    {
        if (k == 0) {
            // Sampler remains empty, operations are valid but will result in no sample.
        }
    }

    /**
     * @brief Processes an incoming item and its weight, potentially adding it to the reservoir.
     *
     * If the item's weight is not positive, it is ignored. Otherwise, a key is generated for the item.
     * If the reservoir has fewer than 'k' items, the new item is added.
     * If the reservoir is full and the new item's key is larger than the smallest key currently
     * in the reservoir, the item with the smallest key is replaced by the new item.
     *
     * @param item The item to consider for sampling (lvalue).
     * @param weight The weight associated with the item. Must be positive.
     */
    void add(const T& item, WeightType weight) {
        if (k_ == 0) return; // Nothing to do if reservoir capacity is 0

        if (weight <= 0) {
            // Non-positive weights are typically ignored in weighted sampling,
            // as they make key calculation problematic or meaningless.
            return; // Skip this item
        }

        double new_key = calculate_key(weight);
        ReservoirItem new_reservoir_item = {item, new_key};

        if (reservoir_.size() < k_) {
            reservoir_.insert(new_reservoir_item);
        } else {
            // Reservoir is full, compare with the smallest key item
            // multiset is sorted ascending by key, so begin() is smallest.
            if (new_key > reservoir_.begin()->key) {
                reservoir_.erase(reservoir_.begin()); // Remove item with smallest key
                reservoir_.insert(new_reservoir_item);   // Add new item
            }
        }
    }

    /**
     * @brief Processes an incoming item and its weight, potentially adding it to the reservoir (move version).
     *
     * Similar to the lvalue version of add, but the item is moved into the reservoir if selected.
     *
     * @param item The item to consider for sampling (rvalue).
     * @param weight The weight associated with the item. Must be positive.
     */
    void add(T&& item, WeightType weight) {
        if (k_ == 0) return;

        if (weight <= 0) {
            return;
        }

        double new_key = calculate_key(weight);
        // Construct ReservoirItem with moved item
        ReservoirItem new_reservoir_item = {std::move(item), new_key};


        if (reservoir_.size() < k_) {
            reservoir_.insert(std::move(new_reservoir_item));
        } else {
            if (new_key > reservoir_.begin()->key) {
                reservoir_.erase(reservoir_.begin());
                reservoir_.insert(std::move(new_reservoir_item));
            }
        }
    }

    /**
     * @brief Retrieves the current sample from the reservoir.
     * @return A std::vector containing the items currently in the sample.
     *         The order of items in the vector is determined by their keys (ascending).
     */
    std::vector<T> get_sample() const {
        std::vector<T> sample;
        sample.reserve(reservoir_.size());
        for (const auto& ri : reservoir_) {
            sample.push_back(ri.item);
        }
        return sample;
    }

    // Could also offer a way to get sample with keys or original weights if stored.
    // std::vector<std::pair<T, double>> get_sample_with_keys() const;

    /**
     * @brief Returns the current number of items in the sample reservoir.
     * @return The number of items currently stored. This will be less than or equal to 'k'.
     */
    size_t sample_size() const noexcept {
        return reservoir_.size();
    }

    /**
     * @brief Returns the maximum capacity of the sample reservoir.
     * @return The configured 'k' value.
     */
    size_t capacity() const noexcept {
        return k_;
    }

    /**
     * @brief Checks if the sample reservoir is currently empty.
     * @return True if no items are in the sample, false otherwise.
     */
    bool empty() const noexcept {
        return reservoir_.empty();
    }

    /**
     * @brief Clears all items from the sample reservoir.
     * The random number generator's state is not reset.
     */
    void clear() {
        reservoir_.clear();
        // Random generator state is not reset, which is typical.
        // If a full reset to construction state is needed, seed could be reset too.
    }
};

} // namespace cpp_utils

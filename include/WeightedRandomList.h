#pragma once

#include <vector>
#include <random>
#include <numeric> // for std::iota (potentially)
#include <optional>
#include <functional> // for std::reference_wrapper
#include <stdexcept> // for std::out_of_range
#include <type_traits> // for std::is_same_v, std::is_integral_v
#include <algorithm> // for std::lower_bound in binary search logic

#include "fenwick_tree.h" // Assuming it's in the same include directory

namespace cpp_utils {

template<typename T, typename WeightType = long long>
class WeightedRandomList {
public:
    static_assert(std::is_same_v<WeightType, long long>,
                  "WeightType must be long long for compatibility with the current FenwickTree implementation.");
    static_assert(std::is_integral_v<WeightType> && std::is_signed_v<WeightType>,
                  "WeightType must be a signed integral type."); // Weights can be 0, should not be negative.

    using value_type = T;
    using weight_type = WeightType;
    using size_type = std::size_t;

    // Constructors
    WeightedRandomList() : ft_(0), total_weight_(0), gen_(rd_()) {}

    explicit WeightedRandomList(size_type initial_capacity) : ft_(0), total_weight_(0), gen_(rd_()) {
        elements_.reserve(initial_capacity);
        weights_.reserve(initial_capacity);
        // Fenwick tree will be built when elements are added.
    }

    // TODO: Consider constructors from iterators or initializer_list
    // WeightedRandomList(std::initializer_list<std::pair<T, WeightType>> il);

    // Modifiers
    void push_back(const T& value, WeightType weight) {
        if (weight < 0) {
            throw std::invalid_argument("Weight cannot be negative.");
        }
        elements_.push_back(value);
        weights_.push_back(weight);
        total_weight_ += weight;
        rebuild_fenwick_tree();
    }

    void push_back(T&& value, WeightType weight) {
        if (weight < 0) {
            throw std::invalid_argument("Weight cannot be negative.");
        }
        elements_.push_back(std::move(value));
        weights_.push_back(weight);
        total_weight_ += weight;
        rebuild_fenwick_tree();
    }

    void update_weight(size_type index, WeightType new_weight) {
        if (index >= size()) {
            throw std::out_of_range("Index out of range in update_weight");
        }
        if (new_weight < 0) {
            throw std::invalid_argument("New weight cannot be negative.");
        }

        WeightType old_weight = weights_[index];
        weights_[index] = new_weight;

        total_weight_ -= old_weight;
        total_weight_ += new_weight;

        ft_.set(static_cast<int>(index), new_weight); // FenwickTree uses int for index
    }

    // Random Access
    std::optional<std::reference_wrapper<const T>> get_random() const {
        if (empty() || total_weight_ <= 0) {
            return std::nullopt;
        }

        std::uniform_int_distribution<WeightType> dist(0, total_weight_ - 1);
        WeightType random_target_sum = dist(gen_);

        size_type selected_index = find_index_for_cumulative_sum(random_target_sum);
        return std::cref(elements_[selected_index]);
    }

    // Mutable version of get_random
    std::optional<std::reference_wrapper<T>> get_random_mut() {
        if (empty() || total_weight_ <= 0) {
            return std::nullopt;
        }
        // Need mutable generator for non-const method
        std::uniform_int_distribution<WeightType> dist(0, total_weight_ - 1);
        WeightType random_target_sum = dist(gen_); // Ok, gen_ is mutable

        size_type selected_index = find_index_for_cumulative_sum(random_target_sum);
        return std::ref(elements_[selected_index]);
    }


    // Element Access
    const T& operator[](size_type index) const {
        return elements_[index];
    }

    T& operator[](size_type index) {
        return elements_[index];
    }

    const T& at(size_type index) const {
        if (index >= size()) {
            throw std::out_of_range("Index out of range in at()");
        }
        return elements_[index];
    }

    T& at(size_type index) {
        if (index >= size()) {
            throw std::out_of_range("Index out of range in at()");
        }
        return elements_[index];
    }

    // Returns a pair of const ref to element and its weight
    std::pair<const T&, WeightType> get_entry(size_type index) const {
        if (index >= size()) {
            throw std::out_of_range("Index out of range in get_entry");
        }
        return {elements_[index], weights_[index]};
    }

    // Capacity
    size_type size() const noexcept {
        return elements_.size();
    }

    bool empty() const noexcept {
        return elements_.empty();
    }

    void clear() noexcept {
        elements_.clear();
        weights_.clear();
        total_weight_ = 0;
        ft_ = FenwickTree(0); // Re-initialize Fenwick tree
    }

    void reserve(size_type new_cap) {
        elements_.reserve(new_cap);
        weights_.reserve(new_cap);
        // Fenwick tree is managed based on actual size, not capacity.
    }

    WeightType total_weight() const noexcept {
        return total_weight_;
    }

private:
    void rebuild_fenwick_tree() {
        if (weights_.empty()) {
            ft_ = FenwickTree(0);
        } else {
            // FenwickTree constructor from std::vector<long long>
            // Ensure weights_ is compatible or cast if necessary.
            // Since WeightType is asserted to be long long, this is fine.
            ft_ = FenwickTree(weights_);
        }
    }

    // Finds the first index `idx` such that prefixSum(idx) > target_sum.
    // target_sum is a value from [0, total_weight_ - 1].
    // This means we are looking for the item that "covers" this random value.
    size_type find_index_for_cumulative_sum(WeightType target_sum) const {
        // Binary search for the index
        size_type low = 0;
        size_type high = size() - 1;
        size_type ans_idx = size() -1; // Default to last element if something goes wrong, or if all but last have 0 weight

        while(low <= high) {
            size_type mid = low + (high - low) / 2;
            // ft_.prefixSum(i) is sum up to and including i.
            // if ft_.prefixSum(mid) > target_sum, it means mid *might* be our element,
            // or an element before it.
            // Example: weights [10,20,5], total 35. target_sum in [0,34]
            // sums: [10, 30, 35]
            // target_sum = 0: ft_.prefixSum(0)=10 > 0. ans=0. high=-1. Correct.
            // target_sum = 9: ft_.prefixSum(0)=10 > 9. ans=0. high=-1. Correct.
            // target_sum = 10: ft_.prefixSum(0)=10. Not > 10. low=1.
            //                  mid=1 (low=1, high=2). ft_.prefixSum(1)=30 > 10. ans=1. high=0.
            //                  Loop ends. ans_idx=1. Correct.
            // target_sum = 29: ft_.prefixSum(1)=30 > 29. ans=1. high=0. Correct.
            // target_sum = 30: ft_.prefixSum(1)=30. Not > 30. low=2.
            //                  mid=2 (low=2, high=2). ft_.prefixSum(2)=35 > 30. ans=2. high=1.
            //                  Loop ends. ans_idx=2. Correct.

            if (ft_.prefixSum(static_cast<int>(mid)) > target_sum) {
                ans_idx = mid;
                if (mid == 0) break; // Avoid underflow with mid-1
                high = mid - 1;
            } else {
                low = mid + 1;
            }
        }
        return ans_idx;
    }


    std::vector<T> elements_;
    std::vector<WeightType> weights_; // Raw weights
    FenwickTree ft_;                  // For cumulative sums and selection logic
    WeightType total_weight_;

    // Random number generation
    // mutable allows gen_ to be used in const get_random()
    // However, std::mt19937 is not thread-safe if shared and mutated.
    // For const correctness, if get_random() is const, it shouldn't change state.
    // But random number generator inherently changes its internal state.
    // Common practice is to mark generator `mutable` or pass generator by ref.
    // Let's make gen_ mutable for now.
    mutable std::random_device rd_; // Used once to seed gen_
    mutable std::mt19937 gen_;      // Standard mersenne_twister_engine
};

} // namespace cpp_utils

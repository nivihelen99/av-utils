#pragma once

#include <vector>
#include <functional> // For std::hash
#include <cmath>      // For std::log, std::pow, std::ceil
#include <limits>     // For std::numeric_limits
#include <cstdint>    // For uint8_t, uint32_t, uint64_t
#include <stdexcept>  // For std::invalid_argument
#include <string>     // For std::string in example hash, can be removed later

namespace cpp_utils {

// Basic hash functions (can be improved or made more flexible later)
// Using a simple combination for two hash values.
// For robust applications, consider more advanced hashing like MurmurHash3 or xxHash.
template <typename T>
struct DefaultHash {
    uint64_t operator()(const T& item, uint64_t seed) const {
        // A simple way to get two different hash values for an item.
        // This is a placeholder; a more robust hashing strategy is usually needed.
        std::hash<T> hasher;
        uint64_t h1 = hasher(item);
        // Combine with seed to get different hashes for different hash functions
        // This is a very basic approach, for illustration.
        // A common technique is h(i) = (h1 + i * h2) % m
        // where h1 and h2 are two independent hash functions.
        // For now, let's just use seed to perturb h1.
        return h1 ^ (seed * 0x9e3779b97f4a7c15ULL); // A large prime for mixing
    }
};


template <typename T,
          typename CounterType = uint8_t,
          typename Hasher = DefaultHash<T>>
class CountingBloomFilter {
public:
    CountingBloomFilter(size_t expected_insertions, double false_positive_rate)
        : hasher_() {
        if (expected_insertions == 0) {
            throw std::invalid_argument("Expected insertions must be greater than 0.");
        }
        if (false_positive_rate <= 0.0 || false_positive_rate >= 1.0) {
            throw std::invalid_argument("False positive rate must be between 0.0 and 1.0.");
        }

        // Optimal number of bits (m) = - (n * ln(p)) / (ln(2)^2)
        // where n = expected_insertions, p = false_positive_rate
        double m_optimal = - (static_cast<double>(expected_insertions) * std::log(false_positive_rate)) /
                             (std::log(2.0) * std::log(2.0));
        num_counters_ = static_cast<size_t>(std::ceil(m_optimal));
        if (num_counters_ == 0) num_counters_ = 1; // Ensure at least one counter

        // Optimal number of hash functions (k) = (m/n) * ln(2)
        double k_optimal = (static_cast<double>(num_counters_) / expected_insertions) * std::log(2.0);
        num_hash_functions_ = static_cast<size_t>(std::ceil(k_optimal));
        if (num_hash_functions_ == 0) num_hash_functions_ = 1; // Ensure at least one hash function

        counters_.resize(num_counters_, 0);
    }

    void add(const T& item) {
        for (size_t i = 0; i < num_hash_functions_; ++i) {
            size_t index = getHash(item, i);
            if (counters_[index] < std::numeric_limits<CounterType>::max()) {
                counters_[index]++;
            }
        }
    }

    // Might be present: true, Definitely not present: false
    bool contains(const T& item) const {
        for (size_t i = 0; i < num_hash_functions_; ++i) {
            size_t index = getHash(item, i);
            if (counters_[index] == 0) {
                return false; // Definitely not present
            }
        }
        return true; // Potentially present
    }

    // Returns true if item was potentially removed (all its counters were > 0)
    // Returns false if item was definitely not present (at least one counter was 0)
    bool remove(const T& item) {
        // First check if the item could even be in the filter
        // This avoids decrementing counters for items that were never added (or already fully removed)
        // which could incorrectly affect other items if a counter hits zero prematurely.
        if (!contains(item)) { // This check is important for correctness of subsequent 'contains'
            return false;
        }

        bool potentially_removed = true;
        for (size_t i = 0; i < num_hash_functions_; ++i) {
            size_t index = getHash(item, i);
            if (counters_[index] > 0) {
                counters_[index]--;
            } else {
                // This case should ideally not be hit if `contains` passed and logic is correct,
                // but as a safeguard for counter consistency.
                // If a counter is already zero, it means the item effectively wasn't fully "present"
                // according to this hash function, which is a bit of a state contradiction
                // if contains() returned true. This implies a potential issue or a very full filter.
                // For now, we'll just ensure it doesn't underflow.
                potentially_removed = false; // Or handle as an error/warning
            }
        }
        return potentially_removed; // More accurately, "attempted to remove"
    }

    size_t approxMemoryUsage() const {
        return num_counters_ * sizeof(CounterType) + sizeof(*this);
    }

    size_t numCounters() const { return num_counters_; }
    size_t numHashFunctions() const { return num_hash_functions_; }


private:
    std::vector<CounterType> counters_;
    size_t num_counters_;
    size_t num_hash_functions_;
    Hasher hasher_;

    // Generates the i-th hash value for the item.
    // Uses Kirsch-Mitzenmacher optimization: h_i(x) = (hash_A(x) + i * hash_B(x)) % num_counters_
    // For simplicity here, DefaultHash takes a seed. We use 'i' as part of the seed.
    size_t getHash(const T& item, size_t i) const {
        // Using 'i' as the seed for the DefaultHash to get distinct hash functions
        return static_cast<size_t>(hasher_(item, static_cast<uint64_t>(i))) % num_counters_;
    }
};

// Specialization of DefaultHash for std::string as an example
template <>
struct DefaultHash<std::string> {
    uint64_t operator()(const std::string& item, uint64_t seed) const {
        std::hash<std::string> str_hasher;
        uint64_t h1 = str_hasher(item);

        // Simple mixing with seed.
        // For more robust hashing, consider combining two independent string hashes.
        // e.g., h1 = primary_hash(item), h2 = secondary_hash(item)
        // then hash(i) = (h1 + i * h2) % m
        // Here, we use a simpler perturbation with seed.
        uint64_t seed_mixer = seed * 0x9e3779b97f4a7c15ULL; // Golden ratio prime
        return (h1 ^ seed_mixer) + seed; // Add seed to ensure different outputs for different i
    }
};


} // namespace cpp_utils

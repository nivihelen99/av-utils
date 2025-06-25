#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include <vector>
#include <string>
#include <cmath> // For std::log, std::pow, std::ceil
#include <functional> // For std::hash
#include <limits> // For std::numeric_limits
#include <stdexcept> // For std::invalid_argument

// Forward declaration for hashing
namespace detail {
template <typename T>
struct BloomHash {
    size_t operator()(const T& item, size_t seed) const {
        // A common way to generate multiple hashes is to use two hash functions h1 and h2
        // and compute g_i(x) = h1(x) + i * h2(x) + i*i * h1(x) mod m (less effective for small m)
        // Or, more simply, use std::hash with different seeds.
        // For this implementation, we'll use a combination of a base hash of the item
        // and a perturbation based on the seed.
        std::hash<T> primary_hasher;
        size_t item_hash = primary_hasher(item);

        // Apply mixing to item_hash to improve its bit distribution,
        // especially important if std::hash<T> is simple (e.g., identity for integers).
        // Using components from MurmurHash3's finalizer (fmix64 for size_t if 64-bit).
        // This sequence is for 64-bit size_t. For 32-bit, different constants/shifts would be better.
        // Assuming size_t is commonly 64-bit on test systems.
        size_t mixed_hash = item_hash;
        mixed_hash ^= mixed_hash >> 33;
        mixed_hash *= 0xff51afd7ed558ccdULL; //ULL for 64-bit constant
        mixed_hash ^= mixed_hash >> 33;
        mixed_hash *= 0xc4ceb9fe1a85ec53ULL;
        mixed_hash ^= mixed_hash >> 33;

        // Combine the well-mixed item_hash with the seed.
        // The Kirsch-Mitzenmacher optimization: hash(item) + seed * hash2(item)
        // Here, we use a variant: mixed_hash ^ (seed_perturbation)
        // seed_perturbation uses the seed and another derivative of mixed_hash.
        // (seed + C1 + (mixed_hash << S1) + (mixed_hash >> S2)) is a common way.
        return mixed_hash ^ (seed + 0x9e3779b9 + (mixed_hash << 6) + (mixed_hash >> 2));
    }
};

// Specialization for std::string
template <>
struct BloomHash<std::string> {
    size_t operator()(const std::string& item, size_t seed) const {
        std::hash<std::string> string_hasher;
        // Simple combination: hash the concatenation of seed (as string) and item
        // This is not ideal but works for demonstration.
        // A better approach would be to use a proper hashing library that supports seeding.
        // For this example, we'll use a sequence of hashes based on appending seed.
        // This is not cryptographically secure or statistically perfect but serves the purpose.

        // Using FNV-1a like approach for combining
        size_t h = 2166136261u; // FNV offset basis

        // Incorporate seed
        std::string seed_str = std::to_string(seed);
        for (char c : seed_str) {
            h ^= static_cast<size_t>(c);
            h *= 16777619u; // FNV prime
        }

        // Incorporate item
        for (char c : item) {
            h ^= static_cast<size_t>(c);
            h *= 16777619u; // FNV prime
        }
        return h;
    }
};

} // namespace detail

template <typename T>
class BloomFilter {
public:
    /**
     * @brief Constructs a Bloom filter.
     *
     * @param expected_items The expected number of items to be inserted.
     * @param false_positive_probability The desired false positive probability (e.g., 0.01 for 1%).
     *                                   Must be > 0.0 and < 1.0.
     */
    BloomFilter(size_t expected_items, double false_positive_probability)
        : num_expected_items_(expected_items),
          fp_prob_(false_positive_probability) {
        if (expected_items == 0) {
            // Handle degenerate case: if no items are expected,
            // make a minimal filter (e.g. 1 bit, 1 hash func).
            // `might_contain` will always return false if nothing is added.
            // `add` will set the bit.
            num_bits_ = 1;
            num_hashes_ = 1;
        } else if (false_positive_probability <= 0.0 || false_positive_probability >= 1.0) {
            throw std::invalid_argument("False positive probability must be between 0.0 and 1.0 (exclusive).");
        } else {
            num_bits_ = calculate_optimal_m(expected_items, false_positive_probability);
            if (num_bits_ == 0) num_bits_ = 1; // Ensure at least 1 bit
            num_hashes_ = calculate_optimal_k(expected_items, num_bits_);
            if (num_hashes_ == 0) num_hashes_ = 1; // Ensure at least 1 hash function
        }
        bits_.resize(num_bits_, false);
    }

    /**
     * @brief Adds an item to the Bloom filter.
     *
     * @param item The item to add.
     */
    void add(const T& item) {
        for (size_t i = 0; i < num_hashes_; ++i) {
            bits_[get_hash(item, i) % num_bits_] = true;
        }
        item_count_++;
    }

    /**
     * @brief Checks if an item might be in the Bloom filter.
     *
     * @param item The item to check.
     * @return true if the item might be in the filter (could be a false positive).
     * @return false if the item is definitely not in the filter.
     */
    bool might_contain(const T& item) const {
        if (num_expected_items_ == 0 && item_count_ == 0) { // Empty filter created for 0 expected items
             return false;
        }
        if (bits_.empty()) return false; // Should not happen if constructor is correct

        for (size_t i = 0; i < num_hashes_; ++i) {
            if (!bits_[get_hash(item, i) % num_bits_]) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Returns the current number of items added to the filter.
     * Note: This is the number of `add` operations, not unique items.
     */
    size_t approximate_item_count() const {
        return item_count_;
    }

    /**
     * @brief Returns the size of the bit array.
     */
    size_t bit_array_size() const {
        return num_bits_;
    }

    /**
     * @brief Returns the number of hash functions used.
     */
    size_t number_of_hash_functions() const {
        return num_hashes_;
    }

    /**
     * @brief Returns the configured expected number of items.
     */
    size_t expected_items_capacity() const {
        return num_expected_items_;
    }

    /**
     * @brief Returns the configured false positive probability.
     */
    double configured_fp_probability() const {
        return fp_prob_;
    }


private:
    size_t num_expected_items_;
    double fp_prob_;
    size_t num_bits_;    // m
    size_t num_hashes_;  // k
    std::vector<bool> bits_;
    size_t item_count_ = 0; // Count of items added

    detail::BloomHash<T> hasher_;

    /**
     * @brief Calculates the optimal size of the bit array (m).
     * m = - (n * ln(p)) / (ln(2)^2)
     * where n is the number of items, p is the false positive probability.
     */
    static size_t calculate_optimal_m(size_t n, double p) {
        if (n == 0) return 1; // Avoid log(0) or division by zero if p is extreme
        double m_double = - (static_cast<double>(n) * std::log(p)) / (std::log(2.0) * std::log(2.0));
        // Ensure m_double is not NaN or infinity due to extreme p values.
        if (!std::isfinite(m_double) || m_double <= 0) {
            // Fallback for extreme p (e.g., very close to 0 or 1) or n=0
            // If p is very small, m can be very large. std::numeric_limits<size_t>::max() might be too large for vector.
            // Let's cap it or handle it. For now, if it's non-finite, it means p was probably bad.
            // Or if n is huge.
            // A practical upper limit might be needed if vector allocation is a concern.
            // For now, we assume inputs are reasonable.
            // If result is non-positive, means inputs were tricky. Default to a small size.
            if (m_double <=0) return 1;
        }
        return static_cast<size_t>(std::ceil(m_double));
    }

    /**
     * @brief Calculates the optimal number of hash functions (k).
     * k = (m / n) * ln(2)
     * where m is the size of the bit array, n is the number of items.
     */
    static size_t calculate_optimal_k(size_t n, size_t m) {
        if (n == 0) return 1; // If no items, 1 hash function is fine.
        if (m == 0) return 1; // If no bits, 1 hash function (though filter is useless).
        double k_double = (static_cast<double>(m) / static_cast<double>(n)) * std::log(2.0);
         // Ensure k_double is not NaN or infinity.
        if (!std::isfinite(k_double) || k_double <= 0) {
            return 1; // Default to 1 hash function if calculation is problematic.
        }
        size_t k = static_cast<size_t>(std::ceil(k_double));
        return (k == 0) ? 1 : k; // Ensure at least one hash function
    }

    /**
     * @brief Generates the i-th hash for the item.
     * Uses the seed parameter of BloomHash to generate different hashes.
     */
    size_t get_hash(const T& item, size_t seed_index) const {
        // seed_index is 0, 1, ..., num_hashes_ - 1
        return hasher_(item, seed_index);
    }
};

#endif // BLOOM_FILTER_H

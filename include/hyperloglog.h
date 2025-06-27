#pragma once

#include <vector>
#include <cstdint>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <functional> // For std::hash
#include <algorithm> // For std::max
#include <string> // For std::string in hash specialization

// Forward declaration for a potential MurmurHash3 or other hash utility
// For now, we will rely on std::hash and provide a specialization for std::string.
// Users should ensure std::hash is well-defined for their custom types or provide their own hash function.

namespace cpp_collections {

// Helper function to count leading zeros (CLZ)
// Using __builtin_clz for GCC/Clang, _BitScanReverse for MSVC
// A generic fallback is provided but is less efficient.
inline uint8_t count_leading_zeros(uint32_t n) {
    if (n == 0) return 32;
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_clz(n);
#elif defined(_MSC_VER)
    unsigned long index;
    _BitScanReverse(&index, n);
    return 31 - index;
#else
    uint8_t count = 0;
    uint32_t mask = 1 << 31;
    while (mask && !(n & mask)) {
        count++;
        mask >>= 1;
    }
    return count;
#endif
}

inline uint8_t count_leading_zeros(uint64_t n) {
    if (n == 0) return 64;
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_clzll(n);
#elif defined(_MSC_VER)
    unsigned long index;
    _BitScanReverse64(&index, n);
    return 63 - index;
#else
    uint8_t count = 0;
    uint64_t mask = 1ULL << 63;
    while (mask && !(n & mask)) {
        count++;
        mask >>= 1;
    }
    return count;
#endif
}


template <typename T, typename Hash = std::hash<T>, size_t HashBits = 32>
class HyperLogLog {
public:
    // Precision p determines the number of registers m = 2^p.
    // Typical values for p are between 4 and 16 (or 18).
    // Higher precision means more accuracy but also more memory (m registers).
    explicit HyperLogLog(uint8_t precision) : p_(precision) {
        if (precision < 4 || precision > 18) { // Practical limits for p
            throw std::out_of_range("Precision p must be between 4 and 18.");
        }
        m_ = 1 << p_; // m = 2^p
        registers_.assign(m_, 0);
        alpha_ = calculate_alpha(m_);
    }

    void add(const T& item) {
        Hash hasher;
        uint64_t full_hash_val = hasher(item); // Use 64-bit hash internally

        // Adapt to HashBits (32 or 64)
        uint64_t hash_val;
        if constexpr (HashBits == 32) {
            hash_val = static_cast<uint32_t>(full_hash_val);
        } else {
            hash_val = full_hash_val;
        }

        uint32_t register_idx;
        uint64_t w_bits; // The bits from the hash used for rho calculation

        if constexpr (HashBits == 32) {
            uint32_t h_val32 = static_cast<uint32_t>(full_hash_val);
            if (p_ > 32) {
                 // This case should ideally be caught by a static_assert if p_ could be constexpr
                 // or a constructor check. Adding it here for runtime safety.
                throw std::logic_error("Precision p cannot exceed HashBits (32).");
            }
            register_idx = h_val32 >> (32 - p_);
            // The remaining (32-p_) bits are used for rho.
            // Create a mask for these bits. If (32-p_) is 0, mask is 0.
            uint32_t rho_mask = ( (1U << (32 - p_)) -1 );
            w_bits = h_val32 & rho_mask;
        } else { // HashBits == 64
            if (p_ > 64) {
                throw std::logic_error("Precision p cannot exceed HashBits (64).");
            }
            register_idx = static_cast<uint32_t>(full_hash_val >> (64 - p_));
            // The remaining (64-p_) bits are used for rho.
            uint64_t rho_mask = ( (1ULL << (64 - p_)) -1 );
            w_bits = full_hash_val & rho_mask;
        }

        uint8_t rank;
        const int num_rho_bits = HashBits - p_;

        if (num_rho_bits == 0) {
            // All hash bits were used for the index. The "w" part is conceptually empty.
            // rho(empty_string) is typically defined to yield rank 1.
            rank = 1;
        } else if (w_bits == 0) {
            // If the w part of the hash is all zeros, rho is num_rho_bits + 1.
            rank = num_rho_bits + 1;
        } else {
            // Count leading zeros in w_bits, considering it's a num_rho_bits quantity.
            uint8_t clz_in_w;
            if constexpr (HashBits == 32) { // num_rho_bits is implicitly <= 32
                // w_bits currently holds the value of the lowest (32-p_) bits.
                // e.g., p_=28 (4 rho_bits). If h=...ABCD, w_bits=ABCD.
                // count_leading_zeros needs to operate as if on a (32-p_)-bit number.
                // Example: if num_rho_bits = 4, w_bits = 0010 (value 2). CLZ is 2.
                // count_leading_zeros(static_cast<uint32_t>(w_bits)) would give e.g. 30 for value 2.
                // We need clz relative to the width num_rho_bits.
                // This is count_leading_zeros(w_bits) - (TotalBits - num_rho_bits)
                clz_in_w = count_leading_zeros(static_cast<uint32_t>(w_bits)) - p_;
            } else { // HashBits == 64, num_rho_bits implicitly <= 64
                clz_in_w = count_leading_zeros(w_bits) - p_;
            }
            rank = clz_in_w + 1;
        }

        // Ensure rank is at least 1, as per algorithm.
        // Max possible rank is num_rho_bits + 1. Stored value is uint8_t.
        // This check might be redundant if logic is perfect but good for safety.
        rank = std::max(static_cast<uint8_t>(1), rank);
        if (rank > (num_rho_bits + 1) && num_rho_bits +1 <= 255) { // Cap rank if it somehow exceeds theoretical max
             rank = num_rho_bits + 1;
        } else if (num_rho_bits + 1 > 255 && rank > 255) { // Unlikely for typical HLL
             rank = 255;
        }


        if (registers_[register_idx] < rank) {
            registers_[register_idx] = rank;
        }
    }

    double estimate() const {
        double sum_inv_power_2 = 0.0;
        for (uint8_t reg_val : registers_) {
            // Using 1.0 / (1ULL << reg_val) is more efficient for powers of 2.
            // (1ULL << reg_val) computes 2^reg_val.
            // Ensure reg_val is not too large to cause overflow for 1ULL if it can exceed 63.
            // Max rank is num_rho_bits + 1. If HashBits=32, p=4, num_rho_bits=28. Max rank = 29. 2^29 is fine.
            // If HashBits=64, p=4, num_rho_bits=60. Max rank = 61. 2^61 is fine for ULL.
            // Stored as uint8_t, so max reg_val is 255. 1ULL << 255 would overflow.
            // However, practical max rank is HashBits - p + 1, which is <= 64 for HashBits=64.
            // So, 1ULL << reg_val should be safe as long as reg_val < 64.
            // If reg_val can be higher (e.g. if HashBits was > 64, or due to some error), std::pow is safer.
            // Given rank is capped around HashBits-p+1 (max ~61 for p=4,HashBits=64), it's fine.
            if (reg_val == 0) { // 2^0 = 1
                sum_inv_power_2 += 1.0;
            } else if (reg_val < 64) { // Max value for 1ULL << reg_val without overflow
                 sum_inv_power_2 += 1.0 / static_cast<double>(1ULL << reg_val);
            } else { // Fallback for very large (and likely theoretical) register values
                 sum_inv_power_2 += std::pow(2.0, -static_cast<double>(reg_val));
            }
        }

        double raw_estimate = alpha_ * static_cast<double>(m_ * m_) / sum_inv_power_2;

        // Small range correction (HyperLogLog algorithm)
        int zero_registers = 0;
        for (uint8_t reg_val : registers_) {
            if (reg_val == 0) {
                zero_registers++;
            }
        }

        if (raw_estimate <= 2.5 * m_) {
            if (zero_registers > 0) { // LinearCounting
                return static_cast<double>(m_) * std::log(static_cast<double>(m_) / zero_registers);
            } else { // No zero registers, but still in small range, use HLL estimate
                return raw_estimate;
            }
        }

        // Large range correction (for 32-bit hashes)
        constexpr double pow_2_32 = 4294967296.0; // 2^32
        if constexpr (HashBits == 32) {
            if (raw_estimate > (1.0 / 30.0) * pow_2_32) {
                 return -pow_2_32 * std::log(1.0 - raw_estimate / pow_2_32);
            }
        }
        // No large range correction for 64-bit hashes specified in typical HLL papers,
        // as 2^64 is astronomically large.

        return raw_estimate;
    }

    void merge(const HyperLogLog& other) {
        if (p_ != other.p_ || m_ != other.m_) {
            throw std::invalid_argument("Cannot merge HyperLogLog instances with different precision/register counts.");
        }
        for (size_t i = 0; i < m_; ++i) {
            if (other.registers_[i] > registers_[i]) {
                registers_[i] = other.registers_[i];
            }
        }
    }

    void clear() {
        registers_.assign(m_, 0);
    }

    size_t num_registers() const { return m_; }
    uint8_t precision() const { return p_; }

    // Provides direct access to registers for advanced usage or serialization, const version
    const std::vector<uint8_t>& get_registers() const {
        return registers_;
    }

    // Merges from raw register values. Useful for distributed systems or deserialization.
    // Assumes the input `other_registers` has the same size `m_`.
    void merge_registers(const std::vector<uint8_t>& other_registers) {
        if (other_registers.size() != m_) {
            throw std::invalid_argument("Register vector size mismatch for merging.");
        }
        for (size_t i = 0; i < m_; ++i) {
            if (other_registers[i] > registers_[i]) {
                registers_[i] = other_registers[i];
            }
        }
    }


private:
    uint8_t p_; // precision
    uint32_t m_; // number of registers (m = 2^p)
    std::vector<uint8_t> registers_; // registers
    double alpha_; // correction constant

    static double calculate_alpha(uint32_t m) {
        if (m == 0) return 0.7213; // Should not happen with p_ checks
        switch (m) {
            case 16: return 0.673; // for p=4
            case 32: return 0.697; // for p=5
            case 64: return 0.709; // for p=6
            default: return 0.7213 / (1.0 + 1.079 / static_cast<double>(m));
        }
    }
};

} // namespace cpp_collections

// Specialization of std::hash for std::string if not already well-defined or for consistency.
// This is a basic example; for production, consider a more robust string hash if issues arise.
// Note: Most modern STL implementations have good std::hash<std::string>.
// This is here more as a placeholder or example if needed.
// namespace std {
// template <>
// struct hash<std::string> {
//     size_t operator()(const std::string& s) const {
//         // Basic FNV-1a hash as an example
//         size_t h = 14695981039346656037ULL; // FNV_offset_basis for 64-bit
//         for (char c : s) {
//             h ^= static_cast<size_t>(c);
//             h *= 1099511628211ULL; // FNV_prime for 64-bit
//         }
//         return h;
//     }
// };
// } // namespace std
// It's generally better to let users provide their own hash functions or ensure std::hash is specialized.
// Forcing a specific std::hash<std::string> can be problematic if the user expects the default.
// The class is templated on Hash, so users can pass their own.

#if (defined(__GNUC__) || defined(__clang__)) && !defined(_MSC_VER)
// For MSVC, these are often intrinsics available via <intrin.h>
// For GCC/Clang, <x86intrin.h> might be needed for _BitScanReverse if not using __builtin_clz
#elif defined(_MSC_VER)
#include <intrin.h> // For _BitScanReverse, _BitScanReverse64
#endif

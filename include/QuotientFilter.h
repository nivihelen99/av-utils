#ifndef QUOTIENT_FILTER_H
#define QUOTIENT_FILTER_H

#include <vector>
#include <string>
#include <cmath>      // For std::log2, std::ceil, std::pow
#include <functional> // For std::hash (though we use a custom one)
#include <limits>     // For std::numeric_limits
#include <stdexcept>  // For std::invalid_argument, std::runtime_error
#include <cstdint>    // For uintXX_t types
#include <numeric>    // For std::gcd, std::accumulate (potentially)
#include <algorithm>  // For std::max, std::min
#include <iostream>   // For debugging, remove later
#include <cstring>    // For std::strlen

// Hashing utilities from bloom_filter.h (adapted)
namespace detail {

// FNV-1a constants
#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__) || defined(__aarch64__)
constexpr size_t QF_FNV_PRIME = 1099511628211ULL;
constexpr size_t QF_FNV_OFFSET_BASIS = 14695981039346656037ULL;
#else // 32-bit
constexpr size_t QF_FNV_PRIME = 16777619U;
constexpr size_t QF_FNV_OFFSET_BASIS = 2166136261U;
#endif

// Helper to apply FNV-1a to a block of memory
inline size_t qf_fnv1a_hash_bytes(const unsigned char* data, size_t len, size_t basis, size_t prime) {
    size_t hash_val = basis;
    for (size_t i = 0; i < len; ++i) {
        hash_val ^= static_cast<size_t>(data[i]);
        hash_val *= prime;
    }
    return hash_val;
}

/**
 * @brief Generic hash functor for QuotientFilter.
 * @tparam T Type of the item to hash.
 */
template <typename T>
struct QuotientHash {
    uint64_t operator()(const T& item) const {
        static_assert(std::is_trivially_copyable<T>::value || std::is_standard_layout<T>::value,
                      "QuotientHash<T> default implementation relies on byte representation. Ensure T is suitable.");
        const unsigned char* item_bytes = reinterpret_cast<const unsigned char*>(&item);
        size_t item_len = sizeof(T);
        return qf_fnv1a_hash_bytes(item_bytes, item_len, QF_FNV_OFFSET_BASIS, QF_FNV_PRIME);
    }
};

/// @brief Specialization of QuotientHash for std::string.
template <>
struct QuotientHash<std::string> {
    uint64_t operator()(const std::string& item) const {
        const unsigned char* item_bytes = reinterpret_cast<const unsigned char*>(item.data());
        size_t item_len = item.length();
        return qf_fnv1a_hash_bytes(item_bytes, item_len, QF_FNV_OFFSET_BASIS, QF_FNV_PRIME);
    }
};

/// @brief Specialization of QuotientHash for const char*.
template <>
struct QuotientHash<const char*> {
    uint64_t operator()(const char* item) const {
        const unsigned char* item_bytes = reinterpret_cast<const unsigned char*>(item);
        size_t item_len = std::strlen(item);
        return qf_fnv1a_hash_bytes(item_bytes, item_len, QF_FNV_OFFSET_BASIS, QF_FNV_PRIME);
    }
};

} // namespace detail

/**
 * @brief A Quotient Filter implementation for approximate membership testing.
 *
 * @tparam T Type of the items to be stored.
 * @tparam Hasher Hash functor type for items of type T. Defaults to detail::QuotientHash<T>.
 *
 * @note This implementation is based on the general principles of Quotient Filters.
 *       There is a known issue: test_simple_add_lookup() fails, indicating a bug in
 *       the add() method related to metadata corruption or incorrect run structure
 *       modification when inserting a second element, causing a previously added element
 *       to become unfindable.
 */
template <typename T, typename Hasher = detail::QuotientHash<T>>
class QuotientFilter {
public:
    /// Type used for individual entries in the filter table.
    using entry_type = uint32_t;

private:
    // Metadata bit positions and masks
    uint8_t occupied_bit_shift_;
    uint8_t continuation_bit_shift_;
    uint8_t shifted_bit_shift_;

    entry_type remainder_mask_;
    entry_type occupied_mask_;
    entry_type continuation_mask_;
    entry_type shifted_mask_;

    // Filter parameters
    uint8_t q_bits_;      ///< Number of quotient bits.
    uint8_t r_bits_;      ///< Number of remainder bits.
    uint8_t fingerprint_bits_; ///< Total bits in fingerprint (q + r).

    size_t num_slots_;    ///< Number of slots in the table (2^q_bits_).
    std::vector<entry_type> table_; ///< The filter table storing entries.
    Hasher hasher_;       ///< Hash functor instance.
    size_t item_count_;   ///< Number of items currently in the filter.

    // Configuration storage
    size_t expected_items_config_; ///< Expected number of items filter was configured for.
    double fp_prob_config_;        ///< False positive probability filter was configured for.
    double target_load_factor_ = 0.90; ///< Target load factor for capacity calculation.

public:
    /**
     * @brief Constructs a Quotient Filter.
     * @param expected_items The expected number of items to be inserted. Must be > 0.
     * @param false_positive_probability The desired false positive probability (e.g., 0.01 for 1%).
     *                                   Must be > 0.0 and < 1.0.
     * @throws std::invalid_argument if parameters are out of bounds.
     * @throws std::runtime_error if calculated bit sizes are incompatible or memory allocation fails.
     */
    QuotientFilter(size_t expected_items, double false_positive_probability)
        : item_count_(0),
          expected_items_config_(expected_items),
          fp_prob_config_(false_positive_probability) {

        if (expected_items == 0) {
            throw std::invalid_argument("QuotientFilter: expected_items must be greater than 0.");
        }
        if (false_positive_probability <= 0.0 || false_positive_probability >= 1.0) {
            throw std::invalid_argument("QuotientFilter: false_positive_probability must be between 0.0 and 1.0 (exclusive).");
        }

        this->r_bits_ = static_cast<uint8_t>(std::max(1.0, std::ceil(-std::log2(false_positive_probability))));

        size_t effective_expected_items = expected_items;
        if (effective_expected_items < 2) effective_expected_items = 2; // Avoid log2(small_num) issues for q_bits_

        double min_slots_double = static_cast<double>(effective_expected_items) / this->target_load_factor_;
        this->q_bits_ = static_cast<uint8_t>(std::max(1.0, std::ceil(std::log2(min_slots_double))));

        this->num_slots_ = 1ULL << this->q_bits_;
        this->fingerprint_bits_ = this->q_bits_ + this->r_bits_;

        if (this->fingerprint_bits_ > 64) {
            throw std::runtime_error("QuotientFilter: Calculated q_bits + r_bits exceeds 64. Reduce expected_items or increase fp_probability.");
        }

        this->remainder_mask_ = (this->r_bits_ == (sizeof(entry_type) * 8)) ? static_cast<entry_type>(-1) : (static_cast<entry_type>(1) << this->r_bits_) - 1;

        this->occupied_bit_shift_ = this->r_bits_;
        this->continuation_bit_shift_ = this->r_bits_ + 1;
        this->shifted_bit_shift_ = this->r_bits_ + 2;

        if (this->shifted_bit_shift_ >= (sizeof(entry_type) * 8)) {
            throw std::runtime_error("QuotientFilter: r_bits is too large for entry_type. Max r_bits for uint32_t is 28 (needs 3 metadata bits).");
        }

        this->occupied_mask_ = static_cast<entry_type>(1) << this->occupied_bit_shift_;
        this->continuation_mask_ = static_cast<entry_type>(1) << this->continuation_bit_shift_;
        this->shifted_mask_ = static_cast<entry_type>(1) << this->shifted_bit_shift_;

        try {
            this->table_.resize(this->num_slots_, 0);
        } catch (const std::bad_alloc& e) {
            throw std::runtime_error(std::string("QuotientFilter: Failed to allocate table memory: ") + e.what());
        }
    }

    /**
     * @brief Adds an item to the Quotient Filter.
     * If the item (or an item with an identical fingerprint) might already be present,
     * this operation has no effect.
     * @param item The item to add.
     * @throws std::runtime_error if the filter is full or an internal error occurs during insertion.
     * @note There is a known bug where adding a second distinct item can make the first item unfindable.
     */
    void add(const T& item) {
        if (this->might_contain(item)) {
            return;
        }

        if (this->item_count_ >= static_cast<size_t>(this->num_slots_ * this->target_load_factor_)) {
             if (this->item_count_ >= this->num_slots_ ) {
                throw std::runtime_error("QuotientFilter is full - no physical slots left.");
             }
        }

        uint64_t fq, fr;
        this->get_fingerprint_parts(item, fq, fr);

        uint64_t cluster_start_b = this->find_cluster_start_idx(fq);
        uint64_t run_start_location = this->find_run_start_idx_for_quotient(fq, cluster_start_b);

        uint64_t insert_idx = run_start_location;
        bool new_element_is_continuation = false;
        bool spot_found = false;

        if (this->is_empty_slot(this->table_[run_start_location])) {
            insert_idx = run_start_location;
            new_element_is_continuation = false;
            spot_found = true;
        } else {
            uint64_t current_element_iter = run_start_location;
            for (uint64_t i = 0; i < this->num_slots_; ++i) {
                if (fr < this->get_remainder_from_entry(this->table_[current_element_iter])) {
                    insert_idx = current_element_iter;
                    new_element_is_continuation = (current_element_iter != run_start_location);
                    spot_found = true;
                    break;
                }
                if (!this->is_continuation(this->table_[current_element_iter]) && current_element_iter != run_start_location) {
                    insert_idx = current_element_iter;
                    new_element_is_continuation = true;
                    spot_found = true;
                    break;
                }
                uint64_t next_element_iter = (current_element_iter + 1) % this->num_slots_;
                if (!this->is_continuation(this->table_[next_element_iter])) {
                    insert_idx = next_element_iter;
                    new_element_is_continuation = true;
                    spot_found = true;
                    break;
                }
                if (next_element_iter == run_start_location) {
                    insert_idx = next_element_iter;
                    new_element_is_continuation = true;
                    spot_found = true;
                    break;
                }
                current_element_iter = next_element_iter;
            }
        }

        if (!spot_found) {
             throw std::logic_error("QuotientFilter::add - Failed to find insertion spot logically.");
        }

        uint64_t empty_slot_finder = insert_idx;
        uint64_t shift_scan_safety = this->num_slots_ + 1;
        while(!this->is_empty_slot(this->table_[empty_slot_finder])) {
            empty_slot_finder = (empty_slot_finder + 1) % this->num_slots_;
            if (--shift_scan_safety == 0) throw std::runtime_error("QuotientFilter::add - No empty slot found for shifting.");
        }

        uint64_t current_shift_src = (empty_slot_finder == 0) ? this->num_slots_ - 1 : empty_slot_finder - 1;
        while(true) {
            uint64_t dest_idx = (current_shift_src + 1) % this->num_slots_;
            this->table_[dest_idx] = this->table_[current_shift_src];
            if (!this->is_empty_slot(this->table_[dest_idx])) {
                 this->set_shifted(this->table_[dest_idx], true);
            }
            if (current_shift_src == insert_idx) break;
            current_shift_src = (current_shift_src == 0) ? this->num_slots_ - 1 : current_shift_src - 1;
        }

        this->clear_slot(this->table_[insert_idx]);
        this->set_remainder_in_entry(this->table_[insert_idx], fr);
        this->set_occupied(this->table_[fq], true);
        this->set_shifted(this->table_[insert_idx], insert_idx != fq);
        this->set_continuation(this->table_[insert_idx], new_element_is_continuation);

        if (!new_element_is_continuation) {
            uint64_t displaced_element_idx = (insert_idx + 1) % this->num_slots_;
            if (empty_slot_finder != insert_idx) {
                // Old logic: this->set_continuation(this->table_[displaced_element_idx], true);
                // This was suspected of corrupting metadata. The shifted element at
                // displaced_element_idx should carry its original continuation flag from when it was
                // at insert_idx (before being overwritten by the new element). The shift operation
                // (table_[dest] = table_[src]) already copies all metadata.
                // No explicit change to displaced_element_idx's continuation status might be needed here,
                // or it needs more careful conditional logic based on whether the new item splits an existing run.
                // For now, removing the unconditional set_continuation.
            }
        }
        this->item_count_++;
    }

    /**
     * @brief Checks if an item might be in the filter.
     * @param item The item to check.
     * @return True if the item might be in the filter (could be a false positive).
     * @return False if the item is definitely not in the filter.
     */
    bool might_contain(const T& item) const {
        if (this->item_count_ == 0) {
            return false;
        }

        uint64_t fq, fr;
        this->get_fingerprint_parts(item, fq, fr);

        if (!this->is_occupied(this->table_[fq])) {
            return false;
        }

        uint64_t cluster_start_actual_idx = this->find_cluster_start_idx(fq);
        uint64_t run_start_actual_idx = this->find_run_start_idx_for_quotient(fq, cluster_start_actual_idx);

        uint64_t run_iter = run_start_actual_idx;
        uint64_t element_scan_safety = this->num_slots_ + 1;

        while(true) {
            if (--element_scan_safety == 0) return false;

            if (this->get_remainder_from_entry(this->table_[run_iter]) == fr) {
                return true;
            }

            if (!this->is_continuation(this->table_[run_iter]) && run_iter != run_start_actual_idx) {
                return false;
            }

            if (this->is_empty_slot(this->table_[run_iter]) && run_iter != run_start_actual_idx) {
                return false;
            }
            run_iter = (run_iter + 1) % this->num_slots_;

            if (!this->is_continuation(this->table_[run_iter])) {
                return false;
            }

            if (run_iter == run_start_actual_idx) {
                 return false;
            }
        }
        return false;
    }

    /** @brief Returns the current number of items added to the filter. */
    size_t size() const {
        return this->item_count_;
    }

    /** @brief Checks if the filter is empty. */
    bool empty() const {
        return this->item_count_ == 0;
    }

    /** @brief Returns the approximate capacity based on configured load factor. */
    size_t capacity() const {
        return static_cast<size_t>(this->num_slots_ * this->target_load_factor_);
    }

    /** @brief Returns the total number of slots in the filter table (2^q). */
    size_t num_slots() const {
        return this->num_slots_;
    }

    /** @brief Returns the number of quotient bits (q) used. */
    uint8_t quotient_bits() const { return this->q_bits_; }
    /** @brief Returns the number of remainder bits (r) used. */
    uint8_t remainder_bits() const { return this->r_bits_; }
    /** @brief Returns the configured false positive probability. */
    double configured_fp_probability() const { return this->fp_prob_config_; }
    /** @brief Returns the configured expected number of items. */
    size_t expected_items_capacity_config() const { return this->expected_items_config_; }

private:
    // Helper methods for getting/setting parts of an entry
    inline uint64_t get_remainder_from_entry(entry_type entry) const {
        return entry & this->remainder_mask_;
    }

    inline bool is_occupied(entry_type entry) const {
        return entry & this->occupied_mask_;
    }

    inline bool is_continuation(entry_type entry) const {
        return entry & this->continuation_mask_;
    }

    inline bool is_shifted(entry_type entry) const {
        return entry & this->shifted_mask_;
    }

    inline bool is_empty_slot(entry_type entry) const {
        return entry == 0;
    }

    inline void set_remainder_in_entry(entry_type& entry, uint64_t remainder) const {
        entry = (entry & ~(this->remainder_mask_)) | (static_cast<entry_type>(remainder) & this->remainder_mask_);
    }

    inline void set_occupied(entry_type& entry, bool val) const {
        if (val) entry |= this->occupied_mask_;
        else entry &= ~(this->occupied_mask_);
    }

    inline void set_continuation(entry_type& entry, bool val) const {
        if (val) entry |= this->continuation_mask_;
        else entry &= ~(this->continuation_mask_);
    }

    inline void set_shifted(entry_type& entry, bool val) const {
        if (val) entry |= this->shifted_mask_;
        else entry &= ~(this->shifted_mask_);
    }

    inline void clear_slot(entry_type& entry) const {
        entry = 0;
    }

private:
    /**
     * @brief Finds the starting slot index of the cluster that quotient_idx belongs to.
     * A cluster starts at a slot 'b' that is not shifted.
     * @param quotient_idx The canonical slot index of the quotient.
     * @return The starting slot index of the cluster.
     * @throws std::logic_error if a cluster start cannot be found (indicates inconsistent state).
     */
    uint64_t find_cluster_start_idx(uint64_t quotient_idx) const {
        uint64_t b = quotient_idx;
        uint64_t scan_safety = this->num_slots_ + 1;
        while (this->is_shifted(this->table_[b])) {
            b = (b == 0) ? this->num_slots_ - 1 : b - 1;
            if (--scan_safety == 0) {
                throw std::logic_error("QuotientFilter - find_cluster_start_idx: Failed to find cluster start.");
            }
            if (b == quotient_idx && this->is_shifted(this->table_[b])) {
                 throw std::logic_error("QuotientFilter - find_cluster_start_idx: Cycled and still shifted.");
            }
        }
        return b;
    }

    /**
     * @brief Finds the actual starting slot index of a specific quotient's run.
     * @param quotient_idx The canonical slot index of the quotient.
     * @param cluster_start_idx The starting slot index of the cluster this quotient belongs to.
     * @return The starting slot index of the quotient's run.
     * @throws std::logic_error if the run start cannot be found (indicates inconsistent state).
     */
    uint64_t find_run_start_idx_for_quotient(uint64_t quotient_idx, uint64_t cluster_start_idx) const {
        int64_t rank = 0;
        uint64_t rank_calc_pos = cluster_start_idx;
        uint64_t rank_calc_safety = this->num_slots_ + 1;
        while (rank_calc_pos != quotient_idx) {
            if (this->is_occupied(this->table_[rank_calc_pos])) {
                rank++;
            }
            rank_calc_pos = (rank_calc_pos + 1) % this->num_slots_;
            if (--rank_calc_safety == 0) {
                throw std::logic_error("QuotientFilter - find_run_start_idx_for_quotient: Rank calculation failed.");
            }
        }

        uint64_t current_run_candidate = cluster_start_idx;
        uint64_t run_search_safety = this->num_slots_ + 1;
        int64_t runs_to_skip = rank;

        while (true) {
            if (--run_search_safety == 0) {
                 throw std::logic_error("QuotientFilter - find_run_start_idx_for_quotient: Exhausted scan for run start.");
            }
            if (!this->is_continuation(this->table_[current_run_candidate])) {
                if (runs_to_skip == 0) {
                    return current_run_candidate;
                }
                runs_to_skip--;
            }
            current_run_candidate = (current_run_candidate + 1) % this->num_slots_;
            if (current_run_candidate == cluster_start_idx && run_search_safety < this->num_slots_ + 1 ) {
                 throw std::logic_error("QuotientFilter - find_run_start_idx_for_quotient: Cycled during run start search.");
            }
        }
        throw std::logic_error("QuotientFilter - find_run_start_idx_for_quotient: Logic error, run start not returned.");
    }

    /**
     * @brief Truncates a full hash to the fingerprint size used by the filter.
     * @param full_hash The full 64-bit hash.
     * @return The truncated fingerprint.
     */
    inline uint64_t get_truncated_fingerprint(uint64_t full_hash) const {
        if (this->fingerprint_bits_ == 64) {
            return full_hash;
        }
        return full_hash & ((1ULL << this->fingerprint_bits_) - 1);
    }

    /**
     * @brief Hashes an item and splits its fingerprint into quotient and remainder.
     * @param item The item to process.
     * @param[out] quotient The resulting quotient.
     * @param[out] remainder The resulting remainder.
     */
    inline void get_fingerprint_parts(const T& item, uint64_t& quotient, uint64_t& remainder) const {
        uint64_t full_hash = this->hasher_(item);
        uint64_t fingerprint = this->get_truncated_fingerprint(full_hash);
        quotient = fingerprint >> this->r_bits_;
        remainder = fingerprint & this->remainder_mask_;
    }

    /**
     * @brief Splits an already computed (truncated) fingerprint into quotient and remainder.
     * @param fingerprint The truncated fingerprint.
     * @param[out] quotient The resulting quotient.
     * @param[out] remainder The resulting remainder.
     */
    inline void get_fingerprint_parts_from_fp(uint64_t fingerprint, uint64_t& quotient, uint64_t& remainder) const {
        quotient = fingerprint >> this->r_bits_;
        remainder = fingerprint & this->remainder_mask_;
    }
};

#endif // QUOTIENT_FILTER_H

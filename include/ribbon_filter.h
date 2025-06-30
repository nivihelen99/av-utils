#ifndef RIBBON_FILTER_H
#define RIBBON_FILTER_H

#include <vector>
#include <string>
#include <cstdint> // For uint16_t, uint32_t, uint64_t
#include <functional> // For std::hash (though we might not use it directly)
#include <stdexcept> // For std::invalid_argument, std::runtime_error
#include <array> // For std::array
#include <cmath> // For std::log, std::ceil
#include <algorithm> // For std::shuffle, std::find_if, std::reverse
#include <map>       // For construction phase
#include <limits>    // For std::numeric_limits
#include <cstring>   // For std::strlen (for const char* hasher)


// Forward declaration (minimal, if needed at all before full definition)
// template <typename T, typename FP, size_t KI, typename H> class RibbonFilter;
// For a self-contained header, often the main class template definition is sufficient without a preceding forward declaration.
// Let's remove the detailed forward declaration that caused the issue.

namespace detail {

// FNV-1a constants
#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__) || defined(__aarch64__)
constexpr uint64_t RF_FNV_PRIME_64 = 1099511628211ULL;
constexpr uint64_t RF_FNV_OFFSET_BASIS_64 = 14695981039346656037ULL;
constexpr size_t RF_FNV_PRIME = RF_FNV_PRIME_64;
constexpr size_t RF_FNV_OFFSET_BASIS = RF_FNV_OFFSET_BASIS_64;
#else // 32-bit systems
constexpr uint32_t RF_FNV_PRIME_32 = 16777619U;
constexpr uint32_t RF_FNV_OFFSET_BASIS_32 = 2166136261U;
constexpr size_t RF_FNV_PRIME = RF_FNV_PRIME_32;
constexpr size_t RF_FNV_OFFSET_BASIS = RF_FNV_OFFSET_BASIS_32;
#endif


// Helper to apply FNV-1a to a block of memory
inline size_t rf_fnv1a_hash_bytes(const unsigned char* data, size_t len) {
    size_t hash_val = RF_FNV_OFFSET_BASIS;
    for (size_t i = 0; i < len; ++i) {
        hash_val ^= static_cast<size_t>(data[i]);
        hash_val *= RF_FNV_PRIME;
    }
    return hash_val;
}

// Structure to generate fingerprint and k indices for Ribbon Filter
template <typename T, typename FingerprintType, size_t K_Indices_Param> // Renamed K_Indices to avoid conflict
struct RibbonHasher {
    // Generate a primary 64-bit hash first
    uint64_t get_primary_hash(const T& item) const {
        if constexpr (std::is_arithmetic_v<T> || (std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T> && !std::is_same_v<T, std::string> && !std::is_pointer_v<T>)) {
            const unsigned char* item_bytes = reinterpret_cast<const unsigned char*>(&item);
            return rf_fnv1a_hash_bytes(item_bytes, sizeof(T));
        } else {
             // This static_assert will trigger for types not specialized, e.g., std::string or const char* before their specializations.
             // It effectively means only arithmetic/trivial types pass here, others MUST use a specialization.
            static_assert(std::is_same_v<T, std::string> || std::is_convertible_v<T, std::string_view> || std::is_same_v<T, const char*>,
                          "RibbonHasher: Default works for arithmetic/trivial non-pointer types. Specialize for std::string, const char*, or other complex types.");
            return 0; // Should be overridden by specialization if static_assert condition is complex
        }
    }

    FingerprintType get_fingerprint(const T& item) const { // This now calls the potentially specialized get_primary_hash
        uint64_t primary_hash_val = get_primary_hash(item);
        FingerprintType fp = static_cast<FingerprintType>(primary_hash_val & std::numeric_limits<FingerprintType>::max());
        if (fp == 0) {
            fp = 1;
        }
        return fp;
    }

    FingerprintType get_fingerprint_from_hash(uint64_t primary_hash_val) const {
        FingerprintType fp = static_cast<FingerprintType>(primary_hash_val & std::numeric_limits<FingerprintType>::max());
        if (fp == 0) {
            fp = 1;
        }
        return fp;
    }


    void get_k_indices(uint64_t primary_hash, size_t block_size, std::array<size_t, K_Indices_Param>& indices) const {
        static_assert(K_Indices_Param == 3, "This RibbonHasher index generation is hardcoded for K=3");

        auto mix = [](uint64_t h) {
            h ^= h >> 30;
            h *= 0xbf58476d1ce4e5b9ULL;
            h ^= h >> 27;
            h *= 0x94d049bb133111ebULL;
            h ^= h >> 31;
            return h;
        };

        uint64_t h0 = mix(primary_hash);
        uint64_t h1 = mix(h0);
        uint64_t h2 = mix(h1);

        indices[0] = static_cast<size_t>(h0) % block_size;
        indices[1] = (static_cast<size_t>(h1) % block_size) + block_size;
        indices[2] = (static_cast<size_t>(h2) % block_size) + (2 * block_size);
    }
};

// Specialization for std::string
template <typename FingerprintType, size_t K_Indices_Param>
struct RibbonHasher<std::string, FingerprintType, K_Indices_Param> {
    uint64_t get_primary_hash(const std::string& item) const {
        const unsigned char* item_bytes = reinterpret_cast<const unsigned char*>(item.data());
        return rf_fnv1a_hash_bytes(item_bytes, item.length());
    }

    FingerprintType get_fingerprint(const std::string& item) const {
        uint64_t primary_hash_val = get_primary_hash(item);
        FingerprintType fp = static_cast<FingerprintType>(primary_hash_val & std::numeric_limits<FingerprintType>::max());
        if (fp == 0) {
            fp = 1;
        }
        return fp;
    }

    FingerprintType get_fingerprint_from_hash(uint64_t primary_hash_val) const {
        FingerprintType fp = static_cast<FingerprintType>(primary_hash_val & std::numeric_limits<FingerprintType>::max());
        if (fp == 0) {
            fp = 1;
        }
        return fp;
    }

    void get_k_indices(uint64_t primary_hash, size_t block_size, std::array<size_t, K_Indices_Param>& indices) const {
        static_assert(K_Indices_Param == 3, "This RibbonHasher index generation is hardcoded for K=3");
        auto mix = [](uint64_t h) {
            h ^= h >> 30;
            h *= 0xbf58476d1ce4e5b9ULL;
            h ^= h >> 27;
            h *= 0x94d049bb133111ebULL;
            h ^= h >> 31;
            return h;
        };
        uint64_t h0 = mix(primary_hash);
        uint64_t h1 = mix(h0);
        uint64_t h2 = mix(h1);

        indices[0] = static_cast<size_t>(h0) % block_size;
        indices[1] = (static_cast<size_t>(h1) % block_size) + block_size;
        indices[2] = (static_cast<size_t>(h2) % block_size) + (2 * block_size);
    }
};

// Specialization for const char*
template <typename FingerprintType, size_t K_Indices_Param>
struct RibbonHasher<const char*, FingerprintType, K_Indices_Param> {
    uint64_t get_primary_hash(const char* item) const {
        if (item == nullptr) return 0; // Or handle error appropriately
        const unsigned char* item_bytes = reinterpret_cast<const unsigned char*>(item);
        return rf_fnv1a_hash_bytes(item_bytes, std::strlen(item));
    }

    FingerprintType get_fingerprint(const char* item) const {
        uint64_t primary_hash_val = get_primary_hash(item);
        FingerprintType fp = static_cast<FingerprintType>(primary_hash_val & std::numeric_limits<FingerprintType>::max());
        if (fp == 0) {
            fp = 1;
        }
        return fp;
    }

    FingerprintType get_fingerprint_from_hash(uint64_t primary_hash_val) const {
        FingerprintType fp = static_cast<FingerprintType>(primary_hash_val & std::numeric_limits<FingerprintType>::max());
        if (fp == 0) {
            fp = 1;
        }
        return fp;
    }

    void get_k_indices(uint64_t primary_hash, size_t block_size, std::array<size_t, K_Indices_Param>& indices) const {
        static_assert(K_Indices_Param == 3, "This RibbonHasher index generation is hardcoded for K=3");
        auto mix = [](uint64_t h) {
            h ^= h >> 30;
            h *= 0xbf58476d1ce4e5b9ULL;
            h ^= h >> 27;
            h *= 0x94d049bb133111ebULL;
            h ^= h >> 31;
            return h;
        };
        uint64_t h0 = mix(primary_hash);
        uint64_t h1 = mix(h0);
        uint64_t h2 = mix(h1);

        indices[0] = static_cast<size_t>(h0) % block_size;
        indices[1] = (static_cast<size_t>(h1) % block_size) + block_size;
        indices[2] = (static_cast<size_t>(h2) % block_size) + (2 * block_size);
    }
};


} // namespace detail

template <
    typename T,
    typename FingerprintType = uint16_t,
    size_t K_Indices = 3,
    typename Hasher = detail::RibbonHasher<T, FingerprintType, K_Indices>
>
class RibbonFilter {
public:
    explicit RibbonFilter(size_t expected_items)
        : item_hashes_(),
          built_(false),
          hasher_() {
        if (expected_items == 0) {
            block_size_ = 1;
            array_size_ = K_Indices * block_size_;
            if (array_size_ == 0 && K_Indices > 0) block_size_ = 1;
            else if (array_size_ == 0 && K_Indices == 0) block_size_ = 0;
            array_size_ = K_Indices * block_size_;


            filter_array_.assign(array_size_, 0);
            num_items_ = 0;
            return;
        }

        double target_load_factor_for_sizing = 0.81;
        if (K_Indices == 4) {
            target_load_factor_for_sizing = 0.95;
        }

        size_t total_slots = static_cast<size_t>(std::ceil(static_cast<double>(expected_items) / target_load_factor_for_sizing));
        if (total_slots == 0) total_slots = K_Indices > 0 ? K_Indices : 1;

        block_size_ = static_cast<size_t>(std::ceil(static_cast<double>(total_slots) / K_Indices));
        if (block_size_ == 0 && K_Indices > 0) block_size_ = 1;
        else if (block_size_ == 0 && K_Indices == 0) block_size_ = 0;


        array_size_ = block_size_ * K_Indices;

        filter_array_.assign(array_size_, 0);
        item_hashes_.reserve(expected_items);
        num_items_ = 0;
    }

    void add(const T& item) {
        if (built_) {
            throw std::runtime_error("RibbonFilter: Cannot add items after build() has been called.");
        }
        uint64_t primary_hash = hasher_.get_primary_hash(item);
        FingerprintType fp = hasher_.get_fingerprint_from_hash(primary_hash);
        item_hashes_.push_back({primary_hash, fp});
    }

    bool build() {
        if (built_) {
            return true;
        }
        if (item_hashes_.empty()) {
            num_items_ = 0;
            if (array_size_ > 0 && filter_array_.empty()) {
                 filter_array_.assign(array_size_, 0);
            } else if (array_size_ == 0 && K_Indices > 0) {
                block_size_ = 1;
                array_size_ = K_Indices * block_size_;
                filter_array_.assign(array_size_,0);
            } else if (array_size_ == 0 && K_Indices == 0) {
                // No array to build.
            }

            built_ = true;
            return true;
        }

        if (K_Indices > 0 && array_size_ < K_Indices) {
            if (array_size_ == 0) {
                block_size_ = 1;
                array_size_ = K_Indices * block_size_;
                filter_array_.assign(array_size_, 0);
            } else {
                block_size_ = 1;
                array_size_ = K_Indices * block_size_;
                filter_array_.assign(array_size_, 0);
            }
        } else if (K_Indices == 0) {
            if (item_hashes_.empty()) {
                 num_items_ = 0; built_ = true; return true;
            } else {
                 num_items_ = 0; built_ = false; item_hashes_.clear(); return false;
            }
        }


        std::vector<FingerprintType> temp_filter_array(array_size_, 0);

        struct ItemPeelInfo {
            uint64_t original_primary_hash;
            FingerprintType fingerprint;
            std::array<size_t, K_Indices> indices;
            size_t id;
        };

        std::vector<ItemPeelInfo> peel_items(item_hashes_.size());
        // std::map<size_t, std::vector<size_t>> index_to_item_ids; // Not strictly needed for this version of peeling

        for (size_t i = 0; i < item_hashes_.size(); ++i) {
            peel_items[i].original_primary_hash = item_hashes_[i].first;
            peel_items[i].fingerprint = item_hashes_[i].second;
            hasher_.get_k_indices(item_hashes_[i].first, block_size_, peel_items[i].indices);
            peel_items[i].id = i;
            for (size_t slot_idx : peel_items[i].indices) {
                if (slot_idx >= array_size_) {
                    built_ = false; num_items_ = 0; item_hashes_.clear(); return false;
                }
                // index_to_item_ids[slot_idx].push_back(i); // Populated via G below
            }
        }

        std::vector<std::vector<size_t>> G(array_size_);
        // std::vector<size_t> C(item_hashes_.size()); // C is not used in this peeling variant directly for queue
        std::vector<std::pair<size_t, size_t>> H;
        H.reserve(item_hashes_.size());

        for(size_t item_id = 0; item_id < item_hashes_.size(); ++item_id) {
            // C[item_id] = K_Indices; // Not used
            for(size_t slot_idx : peel_items[item_id].indices) {
                G[slot_idx].push_back(item_id);
            }
        }

        std::vector<size_t> Q_slots;
        Q_slots.reserve(array_size_);
        for(size_t slot_idx = 0; slot_idx < array_size_; ++slot_idx) {
            if (G[slot_idx].size() == 1) {
                Q_slots.push_back(slot_idx);
            }
        }

        size_t processed_q_idx = 0;
        while(processed_q_idx < Q_slots.size()) {
            size_t slot_idx_u = Q_slots[processed_q_idx++];
            if (G[slot_idx_u].empty() || G[slot_idx_u].size() != 1) continue;

            size_t item_id_x = G[slot_idx_u][0];

            H.push_back({item_id_x, slot_idx_u});

            for (size_t slot_idx_v : peel_items[item_id_x].indices) {
                // G[slot_idx_u] is implicitly handled as item_id_x is "removed"
                if (slot_idx_v == slot_idx_u) continue;

                auto& items_in_slot_v = G[slot_idx_v];
                auto it = std::find(items_in_slot_v.begin(), items_in_slot_v.end(), item_id_x);
                if (it != items_in_slot_v.end()) {
                    items_in_slot_v.erase(it);
                    if (items_in_slot_v.size() == 1) {
                        // Before adding to Q_slots, check if the remaining item in slot_idx_v is already in H
                        // This prevents processing an item twice if it becomes peelable from multiple slots simultaneously.
                        // However, the standard algorithm relies on the fact that an item is processed once.
                        // The check `if (G[slot_idx_u].empty() || G[slot_idx_u].size() != 1) continue;` handles slots whose items might have been processed.
                        Q_slots.push_back(slot_idx_v);
                    }
                }
            }
        }

        if (H.size() != item_hashes_.size()) {
            item_hashes_.clear();
            built_ = false;
            num_items_ = 0;
            return false;
        }

        std::reverse(H.begin(), H.end());
        for (const auto& pair_xh : H) {
            size_t item_id = pair_xh.first;
            size_t slot_to_store_fp = pair_xh.second;

            FingerprintType current_fp_val = peel_items[item_id].fingerprint;
            for (size_t s_idx : peel_items[item_id].indices) {
                if (s_idx != slot_to_store_fp) {
                    current_fp_val ^= temp_filter_array[s_idx];
                }
            }
            temp_filter_array[slot_to_store_fp] = current_fp_val;
        }

        filter_array_ = std::move(temp_filter_array);
        num_items_ = item_hashes_.size();

        built_ = true;
        return true;
    }


    bool might_contain(const T& item) const {
        if (!built_ || K_Indices == 0) {
            return false;
        }
        if (num_items_ == 0) {
             return false;
        }
        if (array_size_ == 0 ) {
            return false;
        }


        uint64_t primary_hash = hasher_.get_primary_hash(item);
        FingerprintType fp_item = hasher_.get_fingerprint_from_hash(primary_hash);

        std::array<size_t, K_Indices> indices;
        hasher_.get_k_indices(primary_hash, block_size_, indices);

        FingerprintType xor_sum = 0;
        for (size_t idx : indices) {
            if (idx >= array_size_) {
                return false;
            }
            xor_sum ^= filter_array_[idx];
        }
        return xor_sum == fp_item;
    }

    size_t size() const {
        // num_items_ is updated at the end of a successful build, or cleared on failure.
        // If not built, it should reflect 0 items considered "in the filter structure".
        return built_ ? num_items_ : 0;
    }

    size_t capacity_slots() const {
        return array_size_;
    }

    bool is_built() const {
        return built_;
    }

private:
    std::vector<std::pair<uint64_t, FingerprintType>> item_hashes_;
    std::vector<FingerprintType> filter_array_;
    size_t block_size_;
    size_t array_size_;
    size_t num_items_ = 0;
    bool built_ = false; // Initialize to false

    Hasher hasher_;
};


#endif // RIBBON_FILTER_H

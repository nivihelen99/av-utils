#pragma once

#include <vector>
#include <cstdint> // For uint64_t
#include <stdexcept> // For std::out_of_range
#include <numeric>   // For std::accumulate, std::popcount (C++20)
#include <algorithm> // For std::fill, std::all_of, std::any_of, std::none_of
#include <limits>    // For std::numeric_limits

// Forward declarations
class DynamicBitset;

namespace detail {
// Definition of BitProxy needs to come before DynamicBitset if DynamicBitset returns it by value
// However, BitProxy methods use DynamicBitset methods.
// So, we declare BitProxy methods, define DynamicBitset, then define BitProxy methods.

class BitProxy {
public:
    // Constructor can be here
    BitProxy(DynamicBitset& bitset, size_t pos) : bitset_(bitset), pos_(pos) {}

    // Declare methods that depend on DynamicBitset's full definition
    BitProxy& operator=(bool value);
    BitProxy& operator=(const BitProxy& other);
    operator bool() const;
    BitProxy& flip();

private:
    DynamicBitset& bitset_; // Reference is fine with forward declaration
    size_t pos_;
};
} // namespace detail


class DynamicBitset {
public:
    using BlockType = uint64_t;
    static constexpr size_t kBitsPerBlock = std::numeric_limits<BlockType>::digits;

    // Constructors
    explicit DynamicBitset(size_t num_bits = 0, bool initial_value = false)
        : num_bits_(num_bits) {
        blocks_.assign(num_blocks_for_bits(num_bits_), 0); // All bits (including padding) are initially 0.

        if (initial_value && num_bits_ > 0) {
            size_t num_total_blocks_needed = num_blocks_for_bits(num_bits_);
            if (num_total_blocks_needed == 0) return; // Should not happen if num_bits_ > 0

            // Determine how many full blocks of 1s we need
            size_t count_full_blocks_of_ones = num_bits_ / kBitsPerBlock;
            for (size_t i = 0; i < count_full_blocks_of_ones; ++i) {
                blocks_[i] = ~BlockType(0);
            }

            // Handle the last block, which might be partially filled with 1s
            size_t remaining_bits = num_bits_ % kBitsPerBlock;
            if (remaining_bits > 0) {
                // The block index for these remaining bits is count_full_blocks_of_ones
                BlockType mask = (BlockType(1) << remaining_bits) - 1;
                blocks_[count_full_blocks_of_ones] = mask;
            } else if (num_bits_ > 0 && count_full_blocks_of_ones > 0 && num_total_blocks_needed == count_full_blocks_of_ones) {
                // This case means num_bits_ is a perfect multiple of kBitsPerBlock.
                // The loop above already set blocks_[count_full_blocks_of_ones - 1] to all 1s.
                // No further action needed for the last block if it was fully covered and set.
            }
        }
        // Any padding bits in the last block remain 0 due to the initial blocks_.assign.
    }

    // Capacity
    size_t size() const noexcept { return num_bits_; }
    bool empty() const noexcept { return num_bits_ == 0; }

    // Element access
    bool test(size_t pos) const {
        check_bounds(pos);
        return (blocks_[get_block_index(pos)] >> get_bit_offset(pos)) & 1;
    }

    detail::BitProxy operator[](size_t pos) {
        check_bounds(pos); // Check bounds before creating proxy
        return detail::BitProxy(*this, pos);
    }

    bool operator[](size_t pos) const { // For const access
        return test(pos);
    }

    // Modifiers
    void set(size_t pos, bool value = true) {
        check_bounds(pos);
        BlockType& block = blocks_[get_block_index(pos)];
        BlockType bit_mask = BlockType(1) << get_bit_offset(pos);
        if (value) {
            block |= bit_mask;
        } else {
            block &= ~bit_mask;
        }
    }

    void set() { // Sets all bits to true
        if (num_bits_ == 0) return;
        size_t num_total_blocks = num_blocks_for_bits(num_bits_);
        for (size_t i = 0; i < num_total_blocks; ++i) {
            blocks_[i] = ~BlockType(0);
        }
        // Ensure padding bits in the last block are 0
        if (num_bits_ % kBitsPerBlock != 0) {
            BlockType mask = (BlockType(1) << (num_bits_ % kBitsPerBlock)) - 1;
            blocks_.back() &= mask;
        }
    }

    void reset(size_t pos) {
        set(pos, false);
    }

    void reset() { // Clears all bits
        std::fill(blocks_.begin(), blocks_.end(), 0);
    }

    void flip(size_t pos) {
        check_bounds(pos);
        blocks_[get_block_index(pos)] ^= (BlockType(1) << get_bit_offset(pos));
    }

    void flip() { // Flips all bits
        if (num_bits_ == 0) return;
        size_t num_total_blocks = num_blocks_for_bits(num_bits_);
        for (size_t i = 0; i < num_total_blocks; ++i) {
            blocks_[i] = ~blocks_[i];
        }
        // Ensure padding bits in the last block are 0 after flipping the whole block
        if (num_bits_ % kBitsPerBlock != 0) {
            BlockType mask = (BlockType(1) << (num_bits_ % kBitsPerBlock)) - 1;
            blocks_.back() &= mask;
        }
    }

    // Queries
    // Definitions are now the declarations
    // size_t count() const noexcept;
    // bool all() const noexcept;
    // bool any() const noexcept;
    // bool none() const noexcept;

    // TODO: Resize
    // Bitwise operations
    // Definitions are now the declarations
    // DynamicBitset& operator&=(const DynamicBitset& other);
    // DynamicBitset& operator|=(const DynamicBitset& other);
    // DynamicBitset& operator^=(const DynamicBitset& other);
    // operator~() could also be added, returning a new bitset. For now, flip() modifies in-place.

    size_t count() const noexcept {
        if (num_bits_ == 0) return 0;
        size_t bit_count = 0;
        size_t num_full_blocks = num_bits_ / kBitsPerBlock;

        for (size_t i = 0; i < num_full_blocks; ++i) {
            bit_count += software_popcount(blocks_[i]);
        }

        size_t remaining_bits = num_bits_ % kBitsPerBlock;
        if (remaining_bits > 0) {
            BlockType last_block = blocks_[num_full_blocks];
            // Mask should not be needed here as padding bits are guaranteed to be 0
            // BlockType mask = (BlockType(1) << remaining_bits) - 1;
            // bit_count += software_popcount(last_block & mask);
            bit_count += software_popcount(last_block); // Padding bits are 0, won't affect popcount
        }
        return bit_count;
    }

    bool all() const noexcept {
        if (num_bits_ == 0) return true; // Or false, depending on convention. Typically true for empty sets.
                                       // std::bitset::all() is true if size is 0.

        size_t num_full_blocks = num_bits_ / kBitsPerBlock;
        for (size_t i = 0; i < num_full_blocks; ++i) {
            if (blocks_[i] != ~BlockType(0)) {
                return false;
            }
        }

        size_t remaining_bits = num_bits_ % kBitsPerBlock;
        if (remaining_bits > 0) {
            BlockType last_block_mask = (BlockType(1) << remaining_bits) - 1;
            if ((blocks_[num_full_blocks] & last_block_mask) != last_block_mask) {
                return false;
            }
        }
        return true;
    }

    bool any() const noexcept {
        if (num_bits_ == 0) return false;
        // Relies on padding bits being 0.
        for (BlockType block_val : blocks_) {
            if (block_val != 0) {
                return true;
            }
        }
        return false;
    }

    bool none() const noexcept {
        if (num_bits_ == 0) return true;
        // Relies on padding bits being 0.
        for (BlockType block_val : blocks_) {
            if (block_val != 0) {
                return false;
            }
        }
        return true; // Equivalent to !any()
    }

    DynamicBitset& operator&=(const DynamicBitset& other) {
        if (num_bits_ != other.num_bits_) {
            throw std::invalid_argument("DynamicBitset::operator&=: operands have different sizes");
        }
        for (size_t i = 0; i < blocks_.size(); ++i) {
            blocks_[i] &= other.blocks_[i];
        }
        // Padding bits remain 0 as other.blocks_[i] also has its padding bits as 0.
        return *this;
    }

    DynamicBitset& operator|=(const DynamicBitset& other) {
        if (num_bits_ != other.num_bits_) {
            throw std::invalid_argument("DynamicBitset::operator|=: operands have different sizes");
        }
        for (size_t i = 0; i < blocks_.size(); ++i) {
            blocks_[i] |= other.blocks_[i];
        }
        // Padding bits remain 0 as other.blocks_[i] also has its padding bits as 0.
        return *this;
    }

    DynamicBitset& operator^=(const DynamicBitset& other) {
        if (num_bits_ != other.num_bits_) {
            throw std::invalid_argument("DynamicBitset::operator^=: operands have different sizes");
        }
        for (size_t i = 0; i < blocks_.size(); ++i) {
            blocks_[i] ^= other.blocks_[i];
        }
        // Padding bits remain 0 as other.blocks_[i] also has its padding bits as 0.
        return *this;
    }

private:
    friend class detail::BitProxy;

    // Helper for popcount (can be replaced by std::popcount in C++20 or compiler intrinsics)
    static int software_popcount(BlockType n) {
        int count = 0;
        while (n > 0) {
            n &= (n - 1); // Brian Kernighan's algorithm
            count++;
        }
        return count;
    }

    size_t get_block_index(size_t pos) const {
        return pos / kBitsPerBlock;
    }

    size_t get_bit_offset(size_t pos) const {
        return pos % kBitsPerBlock;
    }

    void check_bounds(size_t pos) const {
        if (pos >= num_bits_) {
            throw std::out_of_range("DynamicBitset: position out of range");
        }
    }

    size_t num_blocks_for_bits(size_t num_bits) const {
        if (num_bits == 0) return 0;
        return (num_bits + kBitsPerBlock - 1) / kBitsPerBlock;
    }

    std::vector<BlockType> blocks_;
    size_t num_bits_;
};

// Definitions for detail::BitProxy methods that require DynamicBitset to be complete.
namespace detail {

inline BitProxy& BitProxy::operator=(bool value) {
    bitset_.set(pos_, value);
    return *this;
}

inline BitProxy& BitProxy::operator=(const BitProxy& other) {
    bitset_.set(pos_, static_cast<bool>(other)); // other.operator bool() will call bitset_.test(other.pos_)
    return *this;
}

inline BitProxy::operator bool() const {
    return bitset_.test(pos_);
}

inline BitProxy& BitProxy::flip() {
    bitset_.flip(pos_);
    return *this;
}

} // namespace detail

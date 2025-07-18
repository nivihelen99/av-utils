#pragma once

#include <cstdint>
#include <type_traits>

// A type to represent a field in the bitfield.
template<std::size_t Offset, std::size_t Bits, typename Type = bool>
struct BitfieldValue {
    static_assert(std::is_integral_v<Type> || std::is_enum_v<Type>, "Type must be an integral or enum type.");
    static_assert(Bits > 0, "Bits must be greater than 0.");
    static_assert(Bits <= (sizeof(Type) * 8), "Bits must be less than or equal to the number of bits in Type.");

    using type = Type;
    static constexpr std::size_t offset = Offset;
    static constexpr std::size_t bits = Bits;
};

// A type to represent a boolean flag in the bitfield.
template<std::size_t Offset>
using BitfieldFlag = BitfieldValue<Offset, 1, bool>;

template<typename Underlying, typename... Fields>
class Bitfield {
public:
    using underlying_type = Underlying;

    constexpr Bitfield() : data_(0) {}
    constexpr explicit Bitfield(Underlying data) : data_(data) {}

    template<typename Field>
    constexpr typename Field::type get() const {
        constexpr auto mask = (static_cast<Underlying>(1) << Field::bits) - 1;
        if constexpr (std::is_same_v<typename Field::type, bool>) {
            return (data_ >> Field::offset) & 1;
        } else {
            return static_cast<typename Field::type>((data_ >> Field::offset) & mask);
        }
    }

    template<typename Field>
    constexpr void set(typename Field::type value) {
        constexpr auto mask = (static_cast<Underlying>(1) << Field::bits) - 1;
        if constexpr (std::is_same_v<typename Field::type, bool>) {
            if (value) {
                data_ |= (static_cast<Underlying>(1) << Field::offset);
            } else {
                data_ &= ~(static_cast<Underlying>(1) << Field::offset);
            }
        } else {
            data_ = (data_ & ~(mask << Field::offset)) |
                    ((static_cast<Underlying>(value) & mask) << Field::offset);
        }
    }

    constexpr Underlying to_underlying() const {
        return data_;
    }

private:
    Underlying data_;
};

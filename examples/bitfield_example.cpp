#include "bitfield.h"
#include <iostream>

// Define the layout of our bitfield.
// This could represent a hardware register, a network packet header, etc.
using MyBitfield = Bitfield<uint32_t,
    BitfieldFlag<0>, // A boolean flag at bit 0
    BitfieldValue<1, 3, uint8_t>, // A 3-bit unsigned integer at bit 1
    BitfieldValue<4, 12, uint16_t> // A 12-bit unsigned integer at bit 4
>;

// Give meaningful names to the fields.
using IsEnabled = BitfieldFlag<0>;
using Mode = BitfieldValue<1, 3, uint8_t>;
using Value = BitfieldValue<4, 12, uint16_t>;

int main() {
    MyBitfield bf;

    // Set some values.
    bf.set<IsEnabled>(true);
    bf.set<Mode>(5);
    bf.set<Value>(1024);

    // Get the values back.
    std::cout << "IsEnabled: " << bf.get<IsEnabled>() << std::endl;
    std::cout << "Mode: " << static_cast<int>(bf.get<Mode>()) << std::endl;
    std::cout << "Value: " << bf.get<Value>() << std::endl;

    // Print the underlying integer value.
    std::cout << "Underlying value: " << bf.to_underlying() << std::endl;

    // Create a bitfield from an existing value.
    MyBitfield bf2(0b1010101010101010);

    std::cout << "\nFrom existing value:" << std::endl;
    std::cout << "IsEnabled: " << bf2.get<IsEnabled>() << std::endl;
    std::cout << "Mode: " << static_cast<int>(bf2.get<Mode>()) << std::endl;
    std::cout << "Value: " << bf2.get<Value>() << std::endl;

    return 0;
}

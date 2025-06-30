#include "DynamicBitset.h"
#include <iostream>
#include <vector>

void print_bitset_details(const DynamicBitset& bs, const std::string& name) {
    std::cout << "Bitset '" << name << "':\n";
    std::cout << "  Size: " << bs.size() << "\n";
    std::cout << "  Bits: ";
    if (bs.empty()) {
        std::cout << "(empty)";
    } else {
        for (size_t i = 0; i < bs.size(); ++i) {
            std::cout << (bs[i] ? '1' : '0');
        }
    }
    std::cout << "\n";
    std::cout << "  Count of set bits: " << bs.count() << "\n";
    std::cout << "  All bits set? " << (bs.all() ? "Yes" : "No") << "\n";
    std::cout << "  Any bit set? " << (bs.any() ? "Yes" : "No") << "\n";
    std::cout << "  No bits set? " << (bs.none() ? "Yes" : "No") << "\n";
    std::cout << "----------------------------------------\n";
}

int main() {
    std::cout << "DynamicBitset Examples\n";
    std::cout << "========================\n\n";

    // Example 1: Default constructor and basic operations
    DynamicBitset bs1(10); // 10 bits, initialized to false
    print_bitset_details(bs1, "bs1 (10 bits, default false)");

    bs1.set(1);
    bs1.set(3);
    bs1.set(5, true);
    bs1.set(7);
    print_bitset_details(bs1, "bs1 after setting bits 1, 3, 5, 7");

    std::cout << "bs1[3] is " << (bs1[3] ? "true" : "false") << std::endl;
    bs1[3] = false;
    std::cout << "bs1[3] after bs1[3] = false is " << (bs1[3] ? "true" : "false") << std::endl;
    print_bitset_details(bs1, "bs1 after bs1[3] = false");

    bs1.flip(0);
    bs1[9].flip(); // Using proxy flip
    print_bitset_details(bs1, "bs1 after flipping bits 0 and 9");

    bs1.reset(5);
    print_bitset_details(bs1, "bs1 after resetting bit 5");

    // Example 2: Constructor with initial value
    DynamicBitset bs2(17, true); // 17 bits, initialized to true
    print_bitset_details(bs2, "bs2 (17 bits, default true)");
    bs2.reset(); // Reset all to false
    print_bitset_details(bs2, "bs2 after global reset()");

    bs2.set(); // Set all to true
    print_bitset_details(bs2, "bs2 after global set()");

    bs2.flip(); // Flip all bits
    print_bitset_details(bs2, "bs2 after global flip()");


    // Example 3: Larger bitset (more than one block)
    // Assuming BlockType is uint64_t (64 bits)
    DynamicBitset bs3(70);
    bs3.set(0);
    bs3.set(63);
    bs3.set(64);
    bs3.set(69);
    print_bitset_details(bs3, "bs3 (70 bits) with a few bits set");

    // Example 4: Empty bitset
    DynamicBitset bs_empty;
    print_bitset_details(bs_empty, "bs_empty (default constructed)");
    DynamicBitset bs_empty2(0);
    print_bitset_details(bs_empty2, "bs_empty2 (constructed with 0 bits)");

    // Example 5: Bitwise operations
    DynamicBitset bsa(8, false);
    bsa.set(1); bsa.set(2); bsa.set(5); // 01100100
    // Bits:                                 01234567
    // bsa: 01100100

    DynamicBitset bsb(8, false);
    bsb.set(2); bsb.set(3); bsb.set(5); bsb.set(7); // 00110101
    // bsb: 00110101

    print_bitset_details(bsa, "bsa for bitwise ops");
    print_bitset_details(bsb, "bsb for bitwise ops");

    DynamicBitset bsa_and = bsa;
    bsa_and &= bsb; // Expected: 00100100
    print_bitset_details(bsa_and, "bsa_and (bsa &= bsb)");

    DynamicBitset bsa_or = bsa;
    bsa_or |= bsb;  // Expected: 01110101
    print_bitset_details(bsa_or, "bsa_or (bsa |= bsb)");

    DynamicBitset bsa_xor = bsa;
    bsa_xor ^= bsb; // Expected: 01010001
    print_bitset_details(bsa_xor, "bsa_xor (bsa ^= bsb)");

    std::cout << "\nTrying bitwise op with different sizes (should throw):\n";
    try {
        DynamicBitset bsc(7);
        bsa_and &= bsc;
    } catch (const std::exception& e) {
        std::cout << "  Caught exception: " << e.what() << std::endl;
    }
    std::cout << "----------------------------------------\n";

    std::cout << "\nEnd of DynamicBitset Examples\n";
    return 0;
}

#include "DynamicBitset.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>
#include <stdexcept> // For std::out_of_range, std::invalid_argument

// Helper to convert DynamicBitset to string for easy comparison
std::string to_string(const DynamicBitset& bs) {
    if (bs.empty()) {
        return "";
    }
    std::string s;
    s.reserve(bs.size());
    for (size_t i = 0; i < bs.size(); ++i) {
        s += (bs[i] ? '1' : '0');
    }
    return s;
}

TEST(DynamicBitsetTest, ConstructorDefault) {
    DynamicBitset bs;
    EXPECT_EQ(bs.size(), 0);
    EXPECT_TRUE(bs.empty());
    EXPECT_EQ(bs.count(), 0);
    EXPECT_TRUE(bs.all()); // Consistent with std::bitset
    EXPECT_FALSE(bs.any());
    EXPECT_TRUE(bs.none());
}

TEST(DynamicBitsetTest, ConstructorSizedDefaultValue) {
    DynamicBitset bs(10); // Default false
    EXPECT_EQ(bs.size(), 10);
    EXPECT_FALSE(bs.empty());
    EXPECT_EQ(bs.count(), 0);
    EXPECT_EQ(to_string(bs), "0000000000");
    EXPECT_FALSE(bs.all());
    EXPECT_FALSE(bs.any());
    EXPECT_TRUE(bs.none());

    DynamicBitset bs_large(100); // Default false
    EXPECT_EQ(bs_large.size(), 100);
    EXPECT_EQ(bs_large.count(), 0);
    EXPECT_FALSE(bs_large.all());
    EXPECT_TRUE(bs_large.none());
}

TEST(DynamicBitsetTest, ConstructorSizedSpecificValue) {
    DynamicBitset bs_false(10, false);
    EXPECT_EQ(bs_false.size(), 10);
    EXPECT_EQ(bs_false.count(), 0);
    EXPECT_EQ(to_string(bs_false), "0000000000");

    DynamicBitset bs_true(10, true);
    EXPECT_EQ(bs_true.size(), 10);
    EXPECT_EQ(bs_true.count(), 10);
    EXPECT_EQ(to_string(bs_true), "1111111111");
    EXPECT_TRUE(bs_true.all());
    EXPECT_TRUE(bs_true.any());
    EXPECT_FALSE(bs_true.none());

    DynamicBitset bs_large_true(130, true); // > 2 blocks
    EXPECT_EQ(bs_large_true.size(), 130);
    EXPECT_EQ(bs_large_true.count(), 130);
    EXPECT_TRUE(bs_large_true.all());
    for(size_t i=0; i < 130; ++i) EXPECT_TRUE(bs_large_true.test(i));
}


TEST(DynamicBitsetTest, SetAndTestIndividualBits) {
    DynamicBitset bs(20);
    bs.set(0);
    bs.set(5, true);
    bs.set(10, false); // should clear if already set, or remain clear
    bs.set(19);

    EXPECT_TRUE(bs.test(0));
    EXPECT_TRUE(bs[0]); // Test const operator[]
    EXPECT_TRUE(bs.test(5));
    EXPECT_FALSE(bs.test(10));
    EXPECT_TRUE(bs.test(19));

    EXPECT_FALSE(bs.test(1));
    EXPECT_FALSE(bs.test(18));

    EXPECT_EQ(bs.count(), 3);
    std::cout << "bs in SetAndTestIndividualBits: " << to_string(bs) << std::endl;
    EXPECT_EQ(to_string(bs), "10000100000000000001");
}

TEST(DynamicBitsetTest, OperatorBracketAssignment) {
    DynamicBitset bs(10);
    bs[0] = true;
    bs[3] = true;
    bs[5] = false;
    bs[3] = false;
    bs[7] = bs[0]; // Assign from another proxy

    EXPECT_TRUE(bs.test(0));
    EXPECT_FALSE(bs.test(3));
    EXPECT_FALSE(bs.test(5));
    EXPECT_TRUE(bs.test(7));
    EXPECT_EQ(to_string(bs), "1000000100");
}


TEST(DynamicBitsetTest, ResetIndividualBits) {
    DynamicBitset bs(10, true);
    bs.reset(0);
    bs.reset(5);
    bs.reset(9);

    EXPECT_FALSE(bs.test(0));
    EXPECT_TRUE(bs.test(1));
    EXPECT_FALSE(bs.test(5));
    EXPECT_TRUE(bs.test(8));
    EXPECT_FALSE(bs.test(9));
    EXPECT_EQ(bs.count(), 7);
    EXPECT_EQ(to_string(bs), "0111101110");
}

TEST(DynamicBitsetTest, FlipIndividualBits) {
    DynamicBitset bs(5); // 00000
    bs.flip(0); // 10000
    bs.flip(2); // 10100
    bs.flip(4); // 10101
    bs.flip(0); // 00101

    EXPECT_EQ(to_string(bs), "00101");
    EXPECT_EQ(bs.count(), 2);

    bs[1].flip(); // 01101
    EXPECT_EQ(to_string(bs), "01101");
}

TEST(DynamicBitsetTest, SetAllResetAllFlipAll) {
    DynamicBitset bs(70); // Test across block boundaries
    bs.set();
    EXPECT_EQ(bs.count(), 70);
    EXPECT_TRUE(bs.all());
    for(size_t i=0; i < 70; ++i) EXPECT_TRUE(bs.test(i));


    bs.reset();
    EXPECT_EQ(bs.count(), 0);
    EXPECT_TRUE(bs.none());
    for(size_t i=0; i < 70; ++i) EXPECT_FALSE(bs.test(i));

    bs.set(10);
    bs.set(20);
    bs.set(65); // 3 bits set

    bs.flip(); // All other 67 bits should be set
    EXPECT_EQ(bs.count(), 67);
    EXPECT_FALSE(bs.test(10));
    EXPECT_FALSE(bs.test(20));
    EXPECT_FALSE(bs.test(65));
    EXPECT_TRUE(bs.test(0));
    EXPECT_TRUE(bs.test(69));
}


TEST(DynamicBitsetTest, AllAnyNoneQueries) {
    DynamicBitset bs(5);
    EXPECT_TRUE(bs.none());
    EXPECT_FALSE(bs.any());
    EXPECT_FALSE(bs.all());

    bs.set(2); // 00100
    EXPECT_FALSE(bs.none());
    EXPECT_TRUE(bs.any());
    EXPECT_FALSE(bs.all());

    bs.set(); // 11111
    EXPECT_FALSE(bs.none());
    EXPECT_TRUE(bs.any());
    EXPECT_TRUE(bs.all());

    DynamicBitset bs_empty(0);
    EXPECT_TRUE(bs_empty.none()); // True for empty
    EXPECT_FALSE(bs_empty.any()); // False for empty
    EXPECT_TRUE(bs_empty.all());  // True for empty
}

TEST(DynamicBitsetTest, BoundsChecking) {
    DynamicBitset bs(10);
    EXPECT_THROW(bs.test(10), std::out_of_range);
    EXPECT_THROW(bs.set(10), std::out_of_range);
    EXPECT_THROW(bs.reset(10), std::out_of_range);
    EXPECT_THROW(bs.flip(10), std::out_of_range);
    EXPECT_THROW(bs[10], std::out_of_range); // const operator[]

    // Non-const operator[] check (access that would create proxy)
    EXPECT_THROW(bs[10] = true, std::out_of_range);


    // Valid accesses
    EXPECT_NO_THROW(bs.test(9));
    EXPECT_NO_THROW(bs.set(9));
    EXPECT_NO_THROW(bs[9]);
    EXPECT_NO_THROW(bs[9] = false);
}

TEST(DynamicBitsetTest, BitwiseOperations) {
    DynamicBitset bs1(68); // Across block boundary
    DynamicBitset bs2(68);

    bs1.set(1); bs1.set(30); bs1.set(65); // 0...1...0...1...010
    bs2.set(2); bs2.set(30); bs2.set(66); // 0..1..0...1...001

    DynamicBitset bs_and = bs1;
    bs_and &= bs2;
    EXPECT_TRUE(bs_and.test(30));
    EXPECT_FALSE(bs_and.test(1));
    EXPECT_FALSE(bs_and.test(2));
    EXPECT_FALSE(bs_and.test(65));
    EXPECT_FALSE(bs_and.test(66));
    EXPECT_EQ(bs_and.count(), 1);

    DynamicBitset bs_or = bs1;
    bs_or |= bs2;
    EXPECT_TRUE(bs_or.test(1));
    EXPECT_TRUE(bs_or.test(2));
    EXPECT_TRUE(bs_or.test(30));
    EXPECT_TRUE(bs_or.test(65));
    EXPECT_TRUE(bs_or.test(66));
    EXPECT_EQ(bs_or.count(), 5);

    DynamicBitset bs_xor = bs1;
    bs_xor ^= bs2;
    EXPECT_TRUE(bs_xor.test(1));
    EXPECT_TRUE(bs_xor.test(2));
    EXPECT_FALSE(bs_xor.test(30)); // 1^1 = 0
    EXPECT_TRUE(bs_xor.test(65));
    EXPECT_TRUE(bs_xor.test(66));
    EXPECT_EQ(bs_xor.count(), 4);
}

TEST(DynamicBitsetTest, BitwiseOperationsSizeMismatch) {
    DynamicBitset bs1(10);
    DynamicBitset bs2(12);
    EXPECT_THROW(bs1 &= bs2, std::invalid_argument);
    EXPECT_THROW(bs1 |= bs2, std::invalid_argument);
    EXPECT_THROW(bs1 ^= bs2, std::invalid_argument);
}

TEST(DynamicBitsetTest, StressTestCount) {
    DynamicBitset bs(256); // Exactly 4 blocks of 64 bits
    for (size_t i = 0; i < bs.size(); i += 2) {
        bs.set(i);
    }
    EXPECT_EQ(bs.count(), 128);

    bs.flip();
    EXPECT_EQ(bs.count(), 128);

    bs.reset();
    EXPECT_EQ(bs.count(), 0);

    bs.set();
    EXPECT_EQ(bs.count(), 256);
}

TEST(DynamicBitsetTest, ConstructorLargeValues) {
    const size_t size1 = 1000;
    DynamicBitset bs1_false(size1, false);
    EXPECT_EQ(bs1_false.size(), size1);
    EXPECT_EQ(bs1_false.count(), 0);
    EXPECT_TRUE(bs1_false.none());
    for(size_t i=0; i<size1; ++i) EXPECT_FALSE(bs1_false.test(i));

    DynamicBitset bs1_true(size1, true);
    EXPECT_EQ(bs1_true.size(), size1);
    EXPECT_EQ(bs1_true.count(), size1);
    EXPECT_TRUE(bs1_true.all());
    for(size_t i=0; i<size1; ++i) EXPECT_TRUE(bs1_true.test(i));
}

TEST(DynamicBitsetTest, PaddingBitsCorrectness) {
    // Test that padding bits in the last block are always zero
    // and do not affect operations like count, any, none, all.
    // Size 65 means 1 bit in the second block.
    DynamicBitset bs(65, false); // blocks_[0] = 0, blocks_[1] = 0
    bs.set(64); // blocks_[0] = 0, blocks_[1] = 1 (0...01)

    EXPECT_EQ(bs.count(), 1);
    EXPECT_TRUE(bs.any());
    EXPECT_FALSE(bs.none());
    EXPECT_FALSE(bs.all());
    EXPECT_EQ(to_string(bs).length(), 65);
    EXPECT_EQ(to_string(bs)[64], '1');


    // Flip all. blocks_[0] becomes all 1s. blocks_[1] becomes 1...1110
    // The relevant bit at pos 64 (offset 0 in block 1) flips to 0.
    // Padding bits in block 1 must remain 0.
    bs.flip();
    EXPECT_EQ(bs.count(), 64); // 64 bits are now 1, bit 64 is 0.
    EXPECT_FALSE(bs.test(64));
    EXPECT_TRUE(bs.test(0));
    EXPECT_TRUE(bs.test(63));

    // Set all. blocks_[0] = all 1s. blocks_[1] = 0...01 (bit 64 set, padding 0)
    bs.set();
    EXPECT_EQ(bs.count(), 65);
    EXPECT_TRUE(bs.all());
    EXPECT_TRUE(bs.test(64));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

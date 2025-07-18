#include "bitfield.h"
#include <gtest/gtest.h>

// Define a test bitfield layout.
using TestBitfield = Bitfield<uint32_t,
    BitfieldFlag<0>,
    BitfieldValue<1, 3, uint8_t>,
    BitfieldValue<4, 12, uint16_t>,
    BitfieldValue<16, 8, uint8_t>
>;

// Field definitions for the test bitfield.
using FlagField = BitfieldFlag<0>;
using ThreeBitField = BitfieldValue<1, 3, uint8_t>;
using TwelveBitField = BitfieldValue<4, 12, uint16_t>;
using EightBitField = BitfieldValue<16, 8, uint8_t>;

TEST(BitfieldTest, InitialState) {
    TestBitfield bf;
    EXPECT_EQ(bf.to_underlying(), 0);
    EXPECT_FALSE(bf.get<FlagField>());
    EXPECT_EQ(bf.get<ThreeBitField>(), 0);
    EXPECT_EQ(bf.get<TwelveBitField>(), 0);
    EXPECT_EQ(bf.get<EightBitField>(), 0);
}

TEST(BitfieldTest, SetAndGetFlag) {
    TestBitfield bf;
    bf.set<FlagField>(true);
    EXPECT_TRUE(bf.get<FlagField>());
    EXPECT_EQ(bf.to_underlying(), 1);

    bf.set<FlagField>(false);
    EXPECT_FALSE(bf.get<FlagField>());
    EXPECT_EQ(bf.to_underlying(), 0);
}

TEST(BitfieldTest, SetAndGetThreeBit) {
    TestBitfield bf;
    bf.set<ThreeBitField>(5); // 101 in binary
    EXPECT_EQ(bf.get<ThreeBitField>(), 5);
    EXPECT_EQ(bf.to_underlying(), 5 << 1);

    bf.set<ThreeBitField>(0);
    EXPECT_EQ(bf.get<ThreeBitField>(), 0);
    EXPECT_EQ(bf.to_underlying(), 0);
}

TEST(BitfieldTest, SetAndGetTwelveBit) {
    TestBitfield bf;
    bf.set<TwelveBitField>(2048); // 100000000000 in binary
    EXPECT_EQ(bf.get<TwelveBitField>(), 2048);
    EXPECT_EQ(bf.to_underlying(), 2048 << 4);

    bf.set<TwelveBitField>(0);
    EXPECT_EQ(bf.get<TwelveBitField>(), 0);
    EXPECT_EQ(bf.to_underlying(), 0);
}

TEST(BitfieldTest, SetAndGetEightBit) {
    TestBitfield bf;
    bf.set<EightBitField>(255); // 11111111 in binary
    EXPECT_EQ(bf.get<EightBitField>(), 255);
    EXPECT_EQ(bf.to_underlying(), 255 << 16);

    bf.set<EightBitField>(0);
    EXPECT_EQ(bf.get<EightBitField>(), 0);
    EXPECT_EQ(bf.to_underlying(), 0);
}

TEST(BitfieldTest, CombinedSetAndGet) {
    TestBitfield bf;
    bf.set<FlagField>(true);
    bf.set<ThreeBitField>(5);
    bf.set<TwelveBitField>(1024);
    bf.set<EightBitField>(128);

    EXPECT_TRUE(bf.get<FlagField>());
    EXPECT_EQ(bf.get<ThreeBitField>(), 5);
    EXPECT_EQ(bf.get<TwelveBitField>(), 1024);
    EXPECT_EQ(bf.get<EightBitField>(), 128);

    uint32_t expected = (1 << 0) | (5 << 1) | (1024 << 4) | (128 << 16);
    EXPECT_EQ(bf.to_underlying(), expected);
}

TEST(BitfieldTest, ConstructFromValue) {
    uint32_t value = (1 << 0) | (3 << 1) | (512 << 4) | (64 << 16);
    TestBitfield bf(value);

    EXPECT_TRUE(bf.get<FlagField>());
    EXPECT_EQ(bf.get<ThreeBitField>(), 3);
    EXPECT_EQ(bf.get<TwelveBitField>(), 512);
    EXPECT_EQ(bf.get<EightBitField>(), 64);
}

TEST(BitfieldTest, OverwriteField) {
    TestBitfield bf;
    bf.set<ThreeBitField>(7);
    bf.set<ThreeBitField>(1);
    EXPECT_EQ(bf.get<ThreeBitField>(), 1);
    EXPECT_EQ(bf.to_underlying(), 1 << 1);
}

TEST(BitfieldTest, MaxValues) {
    TestBitfield bf;
    bf.set<ThreeBitField>(7);
    EXPECT_EQ(bf.get<ThreeBitField>(), 7);

    bf.set<TwelveBitField>(4095);
    EXPECT_EQ(bf.get<TwelveBitField>(), 4095);
}

#include "gtest/gtest.h"
#include "hyperloglog.h" // Assuming hyperloglog.h is in include path
#include <string>
#include <vector>
#include <set> // To get actual unique counts for comparison
#include <cmath> // For std::abs
#include <iostream> // For debugging test failures

// A simple deterministic hash for testing purposes
struct SimpleStringHash {
    size_t operator()(const std::string& s) const {
        size_t h = 0;
        for (char c : s) {
            h = h * 31 + static_cast<size_t>(c);
        }
        return h;
    }
};

struct SimpleIntHash {
    size_t operator()(int val) const {
        // A very simple hash, just to make it different from identity if needed
        return static_cast<size_t>(val * 2654435761UL);
    }
};


TEST(HyperLogLogTest, Constructor) {
    cpp_collections::HyperLogLog<std::string> hll(10); // p=10, m=1024
    EXPECT_EQ(hll.precision(), 10);
    EXPECT_EQ(hll.num_registers(), 1024);
    EXPECT_NEAR(hll.estimate(), 0.0, 0.001);

    cpp_collections::HyperLogLog<int, SimpleIntHash, 32> hll_int(4); // p=4, m=16
    EXPECT_EQ(hll_int.precision(), 4);
    EXPECT_EQ(hll_int.num_registers(), 16);
    EXPECT_NEAR(hll_int.estimate(), 0.0, 0.001);

    // Test precision bounds
    EXPECT_THROW(cpp_collections::HyperLogLog<std::string>(3), std::out_of_range);
    EXPECT_THROW(cpp_collections::HyperLogLog<std::string>(19), std::out_of_range);
}

TEST(HyperLogLogTest, AddAndEstimateSmallCounts) {
    cpp_collections::HyperLogLog<std::string> hll(10); // p=10, m=1024
    hll.add("apple");
    hll.add("banana");
    EXPECT_NEAR(hll.estimate(), 2.0, 1.0);

    hll.add("apple");
    EXPECT_NEAR(hll.estimate(), 2.0, 1.0);

    cpp_collections::HyperLogLog<int, SimpleIntHash> hll_int(8); // p=8, m=256, USE SimpleIntHash
    hll_int.add(1);
    hll_int.add(2);
    hll_int.add(3);
    // With SimpleIntHash, this should be more accurate.
    // The tolerance can be tighter, but let's keep it relaxed for now.
    EXPECT_NEAR(hll_int.estimate(), 3.0, 1.0);

    hll_int.add(1);
    hll_int.add(2);
    EXPECT_NEAR(hll_int.estimate(), 3.0, 1.0);
}


TEST(HyperLogLogTest, EstimateEmpty) {
    cpp_collections::HyperLogLog<std::string> hll(10);
    EXPECT_NEAR(hll.estimate(), 0.0, 0.0001);
}

TEST(HyperLogLogTest, AddOneElement) {
    cpp_collections::HyperLogLog<std::string> hll(10);
    hll.add("test");
    // Estimate for one element might be slightly off but close to 1
    // The small range correction (LinearCounting) should make it fairly accurate.
    EXPECT_NEAR(hll.estimate(), 1.0, 0.5);
}

// Test with a specific hash to check register values (simplified)
// This test is more of a white-box test and depends heavily on implementation details
// of hashing and register indexing.

// Let's adjust FixedHash to be default constructible and use a static member for test control.
static uint64_t g_fixed_hash_value_hll_test = 0; // Renamed to avoid collision if other tests use similar
struct ControllableFixedHashHLL { // Renamed struct
    size_t operator()(const int& /*item*/) const {
        return g_fixed_hash_value_hll_test;
    }
};

TEST(HyperLogLogTest, SpecificHashToUpdateRegisterControllable) {
    // Use HashBits = 32 for easier reasoning about bits
    // p = 4, m = 16. HashBits = 32.
    cpp_collections::HyperLogLog<int, ControllableFixedHashHLL, 32> hll(4);

    g_fixed_hash_value_hll_test = 0; // Index 0 (0000), Rho part (w_bits) = 0 (28 zeros). Rank = 28+1 = 29.
    hll.add(1);
    const auto& regs = hll.get_registers();
    EXPECT_EQ(regs[0], 29);
    for (size_t i = 1; i < regs.size(); ++i) { EXPECT_EQ(regs[i], 0); }
    hll.clear();

    g_fixed_hash_value_hll_test = 0x10000000U; // Index 1 (0001), Rho part (w_bits) = 0 (28 zeros). Rank = 29.
    hll.add(2);
    const auto& regs2 = hll.get_registers();
    EXPECT_EQ(regs2[1], 29);
    EXPECT_EQ(regs2[0], 0);
    hll.clear();

    g_fixed_hash_value_hll_test = 0x00000001U; // Index 0 (0000), Rho part (w_bits) = 1 (27 zeros then 1). clz_in_w = 31-4=27. Rank = 28.
    hll.add(3);
    const auto& regs3 = hll.get_registers();
    EXPECT_EQ(regs3[0], 28);
    hll.clear();

    g_fixed_hash_value_hll_test = 0xF000000FU; // Index 15 (1111), Rho part (w_bits) = 0xF (24 zeros then 1111). clz_in_w = 28-4=24. Rank = 25.
    hll.add(4);
    const auto& regs4 = hll.get_registers();
    EXPECT_EQ(regs4[15], 25);
}

TEST(HyperLogLogTest, SpecificHash64BitControllable) {
    cpp_collections::HyperLogLog<int, ControllableFixedHashHLL, 64> hll(4); // p=4, m=16, HashBits=64

    g_fixed_hash_value_hll_test = 0ULL; // Index 0. Rho part (60 zeros). Rank = 60+1 = 61.
    hll.add(1);
    EXPECT_EQ(hll.get_registers()[0], 61);
    hll.clear();

    g_fixed_hash_value_hll_test = 0x1000000000000000ULL; // Index 1. Rho part (60 zeros). Rank = 61.
    hll.add(1);
    EXPECT_EQ(hll.get_registers()[1], 61);
    hll.clear();

    // Hash = 1 (0...01, 64-bit). Index = 0.
    // Rho part (w_bits) = 1 (0...01, 60 bits). clz_in_w = count_leading_zeros(1ULL) - p_ = 63 - 4 = 59. Rank = 60.
    g_fixed_hash_value_hll_test = 1ULL;
    hll.add(1);
    EXPECT_EQ(hll.get_registers()[0], 60);
}


TEST(HyperLogLogTest, Clear) {
    cpp_collections::HyperLogLog<std::string> hll(8);
    hll.add("a");
    hll.add("b");
    ASSERT_GT(hll.estimate(), 1.0);
    hll.clear();
    EXPECT_EQ(hll.precision(), 8);
    EXPECT_EQ(hll.num_registers(), 256);
    EXPECT_NEAR(hll.estimate(), 0.0, 0.0001);
    const auto& regs = hll.get_registers();
    for(uint8_t r_val : regs) {
        EXPECT_EQ(r_val, 0);
    }
    hll.add("c");
    EXPECT_GT(hll.estimate(), 0.5);
    EXPECT_LT(hll.estimate(), 1.5);
}

TEST(HyperLogLogTest, Merge) {
    cpp_collections::HyperLogLog<std::string> hll1(8); // p=8, m=256
    hll1.add("apple");
    hll1.add("banana");
    hll1.add("cherry");

    cpp_collections::HyperLogLog<std::string> hll2(8);
    hll2.add("banana");
    hll2.add("date");
    hll2.add("elderberry");

    cpp_collections::HyperLogLog<std::string> hll_diff_p(10);
    EXPECT_THROW(hll1.merge(hll_diff_p), std::invalid_argument);

    hll1.merge(hll2);
    // Expected unique: apple, banana, cherry, date, elderberry (5)
    double expected_error_rate = 1.04 / std::sqrt(256.0); // sqrt(256)=16, error ~6.5%
    double tolerance = 5.0 * expected_error_rate * 3.0; // 3 sigma
    EXPECT_NEAR(hll1.estimate(), 5.0, std::max(1.5, tolerance)); // Use 1.5 for small N, or stat error

    cpp_collections::HyperLogLog<std::string> hll_empty(8);
    double original_est = hll1.estimate();
    hll1.merge(hll_empty);
    EXPECT_NEAR(hll1.estimate(), original_est, 0.01);

    hll_empty.merge(hll2);
    EXPECT_NEAR(hll_empty.estimate(), hll2.estimate(), 0.5);
}

TEST(HyperLogLogTest, MergeRegisters) {
    cpp_collections::HyperLogLog<int, SimpleIntHash, 32> hll_manual1(4); // p=4, m=16
    std::vector<uint8_t> regs1(16,0);
    regs1[0] = 5; regs1[1] = 3;
    hll_manual1.merge_registers(regs1);

    cpp_collections::HyperLogLog<int, SimpleIntHash, 32> hll_manual2(4);
    std::vector<uint8_t> regs2(16,0);
    regs2[0] = 3; regs2[1] = 5; regs2[2] = 4;
    // Note: Can't use hll_manual2.merge_registers(regs2) if we want to merge hll_manual2 into hll_manual1
    // Instead, we'll merge regs2 directly into hll_manual1 after its initial state.
    // Or, better, create hll_manual2 with its registers and then merge hll_manual2 into hll_manual1.

    // Correct approach:
    cpp_collections::HyperLogLog<int, SimpleIntHash, 32> hll_m1(4);
    std::vector<uint8_t> r1 = {5, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    hll_m1.merge_registers(r1);

    cpp_collections::HyperLogLog<int, SimpleIntHash, 32> hll_m2(4);
    std::vector<uint8_t> r2 = {3, 5, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    hll_m2.merge_registers(r2);

    hll_m1.merge(hll_m2);
    const auto& merged_regs = hll_m1.get_registers();
    EXPECT_EQ(merged_regs[0], 5);
    EXPECT_EQ(merged_regs[1], 5);
    EXPECT_EQ(merged_regs[2], 4);

    std::vector<uint8_t> too_small_regs(10,0);
    EXPECT_THROW(hll_m1.merge_registers(too_small_regs), std::invalid_argument);
}

TEST(HyperLogLogTest, AccuracyLargeCounts) {
    const int num_unique_items = 10000;
    const int precision = 10; // m = 1024
    cpp_collections::HyperLogLog<int, SimpleIntHash> hll(precision); // Use SimpleIntHash
    std::set<int> unique_checker;

    for (int i = 0; i < num_unique_items; ++i) {
        // Using sequential integers with SimpleIntHash should give better distribution
        // than relying on std::hash<int> for potentially small i * 101 + 50 values.
        int val = i;
        hll.add(val);
        unique_checker.insert(val);
    }

    double estimate = hll.estimate();
    double actual_unique = static_cast<double>(unique_checker.size());
    ASSERT_EQ(actual_unique, num_unique_items);

    double expected_error_rate = 1.04 / std::sqrt(1 << precision);
    double tolerance = actual_unique * expected_error_rate * 3.0;

    EXPECT_NEAR(estimate, actual_unique, tolerance);

    const int more_items = num_unique_items / 2;
    for (int i = 0; i < more_items; ++i) {
        int val = i * 101 + 50; // Duplicates
        hll.add(val);
    }
    double estimate_after_duplicates = hll.estimate();
    EXPECT_NEAR(estimate_after_duplicates, actual_unique, tolerance);
}

TEST(HyperLogLogTest, AccuracyDifferentPrecisions) {
    const int num_unique_items = 5000;
    std::vector<int> items;
    for(int i=0; i<num_unique_items; ++i) items.push_back(i); // Simple sequence 0, 1, ..., N-1

    for (int p = 4; p <= 14; p += 2) {
        cpp_collections::HyperLogLog<int, SimpleIntHash> hll(p); // Use SimpleIntHash
        for(int item : items) {
            hll.add(item);
        }
        double estimate = hll.estimate();
        double m_val = static_cast<double>(1 << p);
        double expected_error_rate = 1.04 / std::sqrt(m_val);
        double tolerance = num_unique_items * expected_error_rate * 3.0; // 3 sigma

        EXPECT_NEAR(estimate, static_cast<double>(num_unique_items), tolerance);
    }
}

TEST(HyperLogLogTest, SmallRangeCorrection) {
    const int p = 10; // m = 1024
    cpp_collections::HyperLogLog<int, SimpleIntHash, 32> hll(p);

    int count = 0;
    for (int i = 0; i < 10; ++i) {
        hll.add(i * 100);
        count++;
    }
    double estimate = hll.estimate();
    EXPECT_NEAR(estimate, static_cast<double>(count), count * 0.2 + 1.0); // Wider margin for small N
}

TEST(HyperLogLogTest, Constructor64BitHash) {
    cpp_collections::HyperLogLog<std::string, std::hash<std::string>, 64> hll(10);
    EXPECT_EQ(hll.precision(), 10);
    EXPECT_EQ(hll.num_registers(), 1024);
    hll.add("test_64bit");
    EXPECT_GT(hll.estimate(), 0.5);
    EXPECT_LT(hll.estimate(), 1.5);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

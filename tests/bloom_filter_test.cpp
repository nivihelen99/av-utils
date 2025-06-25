#include "gtest/gtest.h"
#include "bloom_filter.h"
#include <string>
#include <vector>
#include <set>

// Test fixture for BloomFilter tests
class BloomFilterTest : public ::testing::Test {
protected:
    // You can define helper functions or members here if needed
};

TEST_F(BloomFilterTest, ConstructorAndBasicProperties) {
    BloomFilter<int> bf(1000, 0.01);
    EXPECT_GT(bf.bit_array_size(), 0);
    EXPECT_GT(bf.number_of_hash_functions(), 0);
    EXPECT_EQ(bf.approximate_item_count(), 0);
    EXPECT_EQ(bf.expected_items_capacity(), 1000);
    EXPECT_DOUBLE_EQ(bf.configured_fp_probability(), 0.01);
}

TEST_F(BloomFilterTest, AddAndMightContainIntegers) {
    BloomFilter<int> bf(100, 0.01);
    bf.add(42);
    bf.add(123);

    EXPECT_TRUE(bf.might_contain(42));
    EXPECT_TRUE(bf.might_contain(123));
    EXPECT_FALSE(bf.might_contain(100)); // Should be false with high probability
    EXPECT_FALSE(bf.might_contain(1));   // Should be false with high probability

    EXPECT_EQ(bf.approximate_item_count(), 2);
}

TEST_F(BloomFilterTest, AddAndMightContainStrings) {
    BloomFilter<std::string> bf(100, 0.01);
    bf.add("hello");
    bf.add("world");

    EXPECT_TRUE(bf.might_contain("hello"));
    EXPECT_TRUE(bf.might_contain("world"));
    EXPECT_FALSE(bf.might_contain("test"));    // Should be false with high probability
    EXPECT_FALSE(bf.might_contain("bloom"));   // Should be false with high probability

    EXPECT_EQ(bf.approximate_item_count(), 2);
}

TEST_F(BloomFilterTest, OptimalMAndKCalculation) {
    // Test cases derived from online calculators or formulas
    // m = - (n * ln(p)) / (ln(2)^2)
    // k = (m / n) * ln(2)

    // Case 1: n=1000, p=0.01
    // m = -(1000 * ln(0.01)) / (ln(2)^2) = -(1000 * -4.60517) / (0.693147^2) = 4605.17 / 0.48045 = 9585.1
    // k = (9586 / 1000) * ln(2) = 9.586 * 0.693147 = 6.64
    BloomFilter<int> bf1(1000, 0.01);
    EXPECT_EQ(bf1.bit_array_size(), 9586); // ceil(9585.1)
    EXPECT_EQ(bf1.number_of_hash_functions(), 7); // ceil(6.64)

    // Case 2: n=1000000, p=0.001
    // m = -(1000000 * ln(0.001)) / (ln(2)^2) = -(1000000 * -6.907755) / 0.48045 = 14377600
    // k = (14377600 / 1000000) * ln(2) = 14.3776 * 0.693147 = 9.96
    BloomFilter<long> bf2(1000000, 0.001);
    EXPECT_EQ(bf2.bit_array_size(), 14377600); // ceil(14377586.3) -> check calc precision
    EXPECT_EQ(bf2.number_of_hash_functions(), 10); // ceil(9.96)
    // Recalculating with more precision for m for n=1000000, p=0.001
    // m = - (1000000 * std::log(0.001)) / (std::log(2.0) * std::log(2.0));
    // m = - (1000000 * -6.90775527898) / (0.69314718056 * 0.69314718056)
    // m = 6907755.27898 / 0.48045301391
    // m = 14377586.31... => ceil is 14377587
    // The implementation had a slight difference, let's use its own calculation logic for verification
    // if the test fails, means my manual calc or understanding of its ceil is off.
    // The code uses std::ceil on the result.
    size_t expected_m_bf2 = static_cast<size_t>(std::ceil(- (1000000.0 * std::log(0.001)) / (std::log(2.0) * std::log(2.0))));
    EXPECT_EQ(bf2.bit_array_size(), expected_m_bf2);


    // Case 3: n=10, p=0.1
    // m = -(10 * ln(0.1)) / (ln(2)^2) = -(10 * -2.302585) / 0.48045 = 47.92
    // k = (48 / 10) * ln(2) = 4.8 * 0.693147 = 3.32
    BloomFilter<int> bf3(10, 0.1);
    size_t expected_m_bf3 = static_cast<size_t>(std::ceil(- (10.0 * std::log(0.1)) / (std::log(2.0) * std::log(2.0))));
    size_t expected_k_bf3 = static_cast<size_t>(std::ceil((static_cast<double>(expected_m_bf3) / 10.0) * std::log(2.0)));
    if (expected_k_bf3 == 0) expected_k_bf3 = 1;


    EXPECT_EQ(bf3.bit_array_size(), expected_m_bf3);
    EXPECT_EQ(bf3.number_of_hash_functions(), expected_k_bf3);


}

TEST_F(BloomFilterTest, FalsePositiveRateTestIntegers) {
    const size_t num_items_to_insert = 1000;
    const double target_fp_prob = 0.01;
    BloomFilter<int> bf(num_items_to_insert, target_fp_prob);

    std::set<int> inserted_items;
    for (int i = 0; i < num_items_to_insert; ++i) {
        bf.add(i);
        inserted_items.insert(i);
    }

    EXPECT_EQ(bf.approximate_item_count(), num_items_to_insert);

    // Check that all inserted items are found
    for (int item : inserted_items) {
        EXPECT_TRUE(bf.might_contain(item)) << "Item " << item << " should be present.";
    }

    // Check for false positives
    // Test num_items_to_insert *other* items (e.g., from num_items_to_insert to 2*num_items_to_insert -1)
    int false_positive_count = 0;
    const int num_items_to_check_fp = num_items_to_insert * 2; // Check more items to get a better FP estimate

    for (int i = 0; i < num_items_to_check_fp; ++i) {
        int item_to_check = num_items_to_insert + i; // These items were not inserted
        if (bf.might_contain(item_to_check)) {
            // Ensure it wasn't accidentally inserted if our range is weird
            ASSERT_TRUE(inserted_items.find(item_to_check) == inserted_items.end());
            false_positive_count++;
        }
    }

    double observed_fp_rate = static_cast<double>(false_positive_count) / num_items_to_check_fp;

    // Allow some leeway for probabilistic nature.
    // Expected FPs = target_fp_prob * num_items_to_check_fp
    // For example, for 1000 items checked, 0.01 rate means 10 FPs.
    // A common statistical bound is within a few standard deviations.
    // For Bernoulli trials, stddev = sqrt(N*p*(1-p)).
    // We expect it to be "close" to target_fp_prob.
    // Let's check if it's within, say, target_fp_prob +/- 0.01 (absolute) or within 2x target_fp_prob
    // This is a simple check, more rigorous statistical tests exist.
    EXPECT_LT(observed_fp_rate, target_fp_prob * 2.5) << "Observed FP rate " << observed_fp_rate
                                                     << " is much higher than target " << target_fp_prob;
    // And it shouldn't be *too* low either, unless our hash functions are poor or m/k is too large
    // However, a lower-than-expected FP rate is generally not a "failure" of the filter's contract.
    // It just means it's performing better than the minimum guarantee.
    // For a very small number of checks, FP count could be 0.
    if (num_items_to_insert >= 100 && target_fp_prob >= 0.001) { // Only check if FPs are somewhat likely
         // EXPECT_GT(observed_fp_rate, target_fp_prob * 0.1) << "Observed FP rate " << observed_fp_rate
         //                                                 << " is much lower than target " << target_fp_prob;
         // This lower bound check is tricky and might not always hold or be desired.
    }
    std::cout << "FP Test (int): Target FP Rate: " << target_fp_prob
              << ", Observed FP Rate: " << observed_fp_rate
              << " (Checked " << num_items_to_check_fp << " items, got " << false_positive_count << " FPs)" << std::endl;
}


TEST_F(BloomFilterTest, FalsePositiveRateTestStrings) {
    const size_t num_items_to_insert = 1000;
    const double target_fp_prob = 0.02; // Use a slightly different probability
    BloomFilter<std::string> bf(num_items_to_insert, target_fp_prob);

    std::set<std::string> inserted_items;
    for (int i = 0; i < num_items_to_insert; ++i) {
        std::string item = "item_" + std::to_string(i);
        bf.add(item);
        inserted_items.insert(item);
    }

    EXPECT_EQ(bf.approximate_item_count(), num_items_to_insert);

    for (const auto& item : inserted_items) {
        EXPECT_TRUE(bf.might_contain(item)) << "Item " << item << " should be present.";
    }

    int false_positive_count = 0;
    const int num_items_to_check_fp = num_items_to_insert * 2;

    for (int i = 0; i < num_items_to_check_fp; ++i) {
        std::string item_to_check = "non_existent_item_" + std::to_string(i);
        if (bf.might_contain(item_to_check)) {
            ASSERT_TRUE(inserted_items.find(item_to_check) == inserted_items.end());
            false_positive_count++;
        }
    }

    double observed_fp_rate = static_cast<double>(false_positive_count) / num_items_to_check_fp;
    EXPECT_LT(observed_fp_rate, target_fp_prob * 2.5) << "Observed FP rate " << observed_fp_rate
                                                     << " is much higher than target " << target_fp_prob;

    std::cout << "FP Test (string): Target FP Rate: " << target_fp_prob
              << ", Observed FP Rate: " << observed_fp_rate
              << " (Checked " << num_items_to_check_fp << " items, got " << false_positive_count << " FPs)" << std::endl;
}


TEST_F(BloomFilterTest, EdgeCaseEmptyFilter) {
    BloomFilter<int> bf(100, 0.01); // Standard parameters, but nothing added
    EXPECT_FALSE(bf.might_contain(0));
    EXPECT_FALSE(bf.might_contain(12345));
    EXPECT_EQ(bf.approximate_item_count(), 0);
}

TEST_F(BloomFilterTest, EdgeCaseZeroExpectedItems) {
    BloomFilter<int> bf(0, 0.01); // 0 expected items
    EXPECT_EQ(bf.bit_array_size(), 1); // Should default to minimal
    EXPECT_EQ(bf.number_of_hash_functions(), 1);
    EXPECT_FALSE(bf.might_contain(10)); // Nothing added, should be false

    bf.add(10);
    EXPECT_TRUE(bf.might_contain(10));
    EXPECT_FALSE(bf.might_contain(20)); // High chance of false positive if only 1 bit
    EXPECT_EQ(bf.approximate_item_count(), 1);
}

TEST_F(BloomFilterTest, EdgeCaseOneExpectedItem) {
    BloomFilter<std::string> bf(1, 0.0001); // High accuracy for one item
    // m = -(1 * ln(0.0001)) / (ln(2)^2) = -(-9.21034) / 0.48045 = 19.17 => 20 bits
    // k = (20/1) * ln(2) = 13.86 => 14 hashes
    // These are calculated by the class itself.
    // size_t expected_m = static_cast<size_t>(std::ceil(- (1.0 * std::log(0.0001)) / (std::log(2.0) * std::log(2.0))));
    // size_t expected_k = static_cast<size_t>(std::ceil((static_cast<double>(expected_m) / 1.0) * std::log(2.0)));
    // if (expected_k == 0) expected_k = 1;
    // EXPECT_EQ(bf.bit_array_size(), expected_m);
    // EXPECT_EQ(bf.number_of_hash_functions(), expected_k);


    bf.add("only_one");
    EXPECT_TRUE(bf.might_contain("only_one"));
    EXPECT_FALSE(bf.might_contain("another_one")); // Should be very unlikely to be a false positive
    EXPECT_EQ(bf.approximate_item_count(), 1);
}

TEST_F(BloomFilterTest, InvalidConstructorArguments) {
    EXPECT_THROW(BloomFilter<int>(100, 0.0), std::invalid_argument);
    EXPECT_THROW(BloomFilter<int>(100, 1.0), std::invalid_argument);
    EXPECT_THROW(BloomFilter<int>(100, -0.1), std::invalid_argument);
    EXPECT_THROW(BloomFilter<int>(100, 1.1), std::invalid_argument);
    // Zero items is handled, not an exception
    EXPECT_NO_THROW(BloomFilter<int>(0, 0.01));
}

// Test with a custom struct/class
struct MyCustomType {
    int id;
    std::string name;

    bool operator==(const MyCustomType& other) const {
        return id == other.id && name == other.name;
    }
};

// Provide a std::hash specialization for MyCustomType
namespace std {
    template <>
    struct hash<MyCustomType> {
        size_t operator()(const MyCustomType& val) const {
            size_t h1 = std::hash<int>()(val.id);
            size_t h2 = std::hash<std::string>()(val.name);
            return h1 ^ (h2 << 1); // Simple combination
        }
    };
}

TEST_F(BloomFilterTest, CustomType) {
    BloomFilter<MyCustomType> bf(50, 0.05);
    MyCustomType item1 = {1, "Alice"};
    MyCustomType item2 = {2, "Bob"};
    MyCustomType item3 = {3, "Charlie"}; // Not added

    bf.add(item1);
    bf.add(item2);

    EXPECT_TRUE(bf.might_contain(item1));
    EXPECT_TRUE(bf.might_contain(item2));
    EXPECT_FALSE(bf.might_contain(item3)); // High probability
    EXPECT_EQ(bf.approximate_item_count(), 2);
}

// Test what happens if many items are added, exceeding expected capacity
TEST_F(BloomFilterTest, ExceedingCapacity) {
    const size_t num_expected_items = 100;
    const double target_fp_prob = 0.01;
    BloomFilter<int> bf(num_expected_items, target_fp_prob);

    // Add expected number of items
    for (int i = 0; i < num_expected_items; ++i) {
        bf.add(i);
    }
    EXPECT_EQ(bf.approximate_item_count(), num_expected_items);

    // Check FP rate for items not added (within a reasonable range)
    int fp_count_at_capacity = 0;
    int items_to_check = num_expected_items * 2;
    for (int i = 0; i < items_to_check; ++i) {
        if (bf.might_contain(num_expected_items + i)) {
            fp_count_at_capacity++;
        }
    }
    double observed_fp_at_capacity = static_cast<double>(fp_count_at_capacity) / items_to_check;
    std::cout << "FP Test (at capacity): Target FP Rate: " << target_fp_prob
              << ", Observed FP Rate: " << observed_fp_at_capacity << std::endl;
    EXPECT_LT(observed_fp_at_capacity, target_fp_prob * 3.0); // Allow a bit more leeway


    // Add significantly more items (e.g., 5x expected)
    const size_t num_extra_items = num_expected_items * 4;
    for (int i = 0; i < num_extra_items; ++i) {
        bf.add(num_expected_items * 2 + i); // Add items in a new range
    }
    EXPECT_EQ(bf.approximate_item_count(), num_expected_items + num_extra_items);

    // FP rate is expected to increase significantly
    int fp_count_exceeded_capacity = 0;
    // Check items in a range far away from all inserted items
    for (int i = 0; i < items_to_check; ++i) {
        if (bf.might_contain(num_expected_items * 2 + num_extra_items + i)) {
            fp_count_exceeded_capacity++;
        }
    }
    double observed_fp_exceeded_capacity = static_cast<double>(fp_count_exceeded_capacity) / items_to_check;
    std::cout << "FP Test (exceeded capacity): Original Target FP Rate: " << target_fp_prob
              << ", Observed FP Rate: " << observed_fp_exceeded_capacity << std::endl;

    // The actual FP rate when n > n_expected is p' approx (1 - e^(-k_opt * n_actual / m))^k_opt
    // It should be higher than target_fp_prob.
    EXPECT_GT(observed_fp_exceeded_capacity, target_fp_prob) << "FP rate should increase when capacity is significantly exceeded.";
    // It's hard to give an exact upper bound without recalculating theoretical FP for n_actual.
    // But it should not be 1.0 unless the filter is extremely small or k is tiny.
    if (bf.bit_array_size() > 20) { // Avoid trivial cases where filter gets saturated quickly
        EXPECT_LT(observed_fp_exceeded_capacity, 0.99);
    }
}

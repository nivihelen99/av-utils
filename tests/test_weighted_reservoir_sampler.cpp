#include "gtest/gtest.h"
#include "WeightedReservoirSampler.h"
#include <string>
#include <vector>
#include <map>     // For frequency counting in statistical test
#include <algorithm> // For std::sort
#include <iostream>  // For debug prints if needed

// Test fixture for tests that might benefit from a common setup (optional here, but good practice)
class WeightedReservoirSamplerTest : public ::testing::Test {
protected:
    // Per-test set-up logic, if any, can go here.
    // void SetUp() override {}

    // Per-test tear-down logic, if any, can go here.
    // void TearDown() override {}
};

TEST_F(WeightedReservoirSamplerTest, BasicInitialization) {
    cpp_utils::WeightedReservoirSampler<int> sampler_empty(0);
    EXPECT_EQ(sampler_empty.capacity(), 0);
    EXPECT_EQ(sampler_empty.sample_size(), 0);
    ASSERT_TRUE(sampler_empty.get_sample().empty());

    cpp_utils::WeightedReservoirSampler<std::string> sampler_k5(5);
    EXPECT_EQ(sampler_k5.capacity(), 5);
    EXPECT_EQ(sampler_k5.sample_size(), 0);
    ASSERT_TRUE(sampler_k5.get_sample().empty());
    ASSERT_TRUE(sampler_k5.empty());
}

TEST_F(WeightedReservoirSamplerTest, AddingItemsBelowCapacity) {
    cpp_utils::WeightedReservoirSampler<int> sampler(3);
    sampler.add(10, 1.0);
    EXPECT_EQ(sampler.sample_size(), 1);
    ASSERT_EQ(sampler.get_sample().size(), 1);
    EXPECT_EQ(sampler.get_sample()[0], 10);

    sampler.add(20, 2.0);
    EXPECT_EQ(sampler.sample_size(), 2);
    ASSERT_EQ(sampler.get_sample().size(), 2);

    sampler.add(30, 0.5);
    EXPECT_EQ(sampler.sample_size(), 3);
    ASSERT_EQ(sampler.get_sample().size(), 3);
    ASSERT_FALSE(sampler.empty());

    std::vector<int> sample = sampler.get_sample();
    std::sort(sample.begin(), sample.end());
    ASSERT_EQ(sample.size(), 3); // Ensure size before indexing
    EXPECT_EQ(sample[0], 10);
    EXPECT_EQ(sample[1], 20);
    EXPECT_EQ(sample[2], 30);
}

TEST_F(WeightedReservoirSamplerTest, AddingItemsAtAndBeyondCapacity) {
    unsigned int fixed_seed = 123;
    cpp_utils::WeightedReservoirSampler<int, double> sampler(2, fixed_seed);

    sampler.add(1, 1.0);
    sampler.add(2, 10.0);
    EXPECT_EQ(sampler.sample_size(), 2);

    std::vector<int> s1 = sampler.get_sample();
    std::sort(s1.begin(), s1.end());
    ASSERT_EQ(s1.size(), 2);
    EXPECT_EQ(s1[0], 1);
    EXPECT_EQ(s1[1], 2);

    sampler.add(3, 0.1); // Low weight, should not replace
    EXPECT_EQ(sampler.sample_size(), 2);
    std::vector<int> s2 = sampler.get_sample();
    std::sort(s2.begin(), s2.end());
    ASSERT_EQ(s2.size(), 2);
    EXPECT_EQ(s2[0], 1);
    EXPECT_EQ(s2[1], 2);

    sampler.add(4, 100.0); // High weight, should replace item 1 (weight 1.0)
    EXPECT_EQ(sampler.sample_size(), 2);
    std::vector<int> s3 = sampler.get_sample();
    std::sort(s3.begin(), s3.end());
    ASSERT_EQ(s3.size(), 2);
    bool found2_s3 = std::find(s3.begin(), s3.end(), 2) != s3.end();
    bool found4_s3 = std::find(s3.begin(), s3.end(), 4) != s3.end();
    EXPECT_TRUE(found2_s3);
    EXPECT_TRUE(found4_s3);

    sampler.add(5, 20.0); // Should replace item 2 (weight 10.0)
    EXPECT_EQ(sampler.sample_size(), 2);
    std::vector<int> s4 = sampler.get_sample();
    std::sort(s4.begin(), s4.end());
    ASSERT_EQ(s4.size(), 2);
    bool found5_s4 = std::find(s4.begin(), s4.end(), 5) != s4.end();
    bool found4_s4 = std::find(s4.begin(), s4.end(), 4) != s4.end();
    EXPECT_TRUE(found5_s4);
    EXPECT_TRUE(found4_s4);
}

TEST_F(WeightedReservoirSamplerTest, HandlingOfNonPositiveWeights) {
    cpp_utils::WeightedReservoirSampler<int> sampler(2);
    sampler.add(1, 10.0);
    EXPECT_EQ(sampler.sample_size(), 1);

    sampler.add(2, 0.0); // Zero weight
    EXPECT_EQ(sampler.sample_size(), 1);
    ASSERT_EQ(sampler.get_sample().size(), 1);
    EXPECT_EQ(sampler.get_sample()[0], 1);

    sampler.add(3, -5.5); // Negative weight
    EXPECT_EQ(sampler.sample_size(), 1);
    ASSERT_EQ(sampler.get_sample().size(), 1);
    EXPECT_EQ(sampler.get_sample()[0], 1);

    sampler.add(4, 20.0);
    EXPECT_EQ(sampler.sample_size(), 2);

    std::vector<int> sample = sampler.get_sample();
    ASSERT_EQ(sample.size(), 2);
    bool found1 = std::find(sample.begin(), sample.end(), 1) != sample.end();
    bool found4 = std::find(sample.begin(), sample.end(), 4) != sample.end();
    bool found2 = std::find(sample.begin(), sample.end(), 2) != sample.end();
    bool found3 = std::find(sample.begin(), sample.end(), 3) != sample.end();

    EXPECT_TRUE(found1);
    EXPECT_TRUE(found4);
    EXPECT_FALSE(found2);
    EXPECT_FALSE(found3);
}

TEST_F(WeightedReservoirSamplerTest, ClearAndEmptyOperations) {
    cpp_utils::WeightedReservoirSampler<std::string> sampler(3);
    sampler.add("apple", 1.0);
    sampler.add("banana", 1.0);
    EXPECT_FALSE(sampler.empty());
    EXPECT_EQ(sampler.sample_size(), 2);

    sampler.clear();
    EXPECT_TRUE(sampler.empty());
    EXPECT_EQ(sampler.sample_size(), 0);
    ASSERT_TRUE(sampler.get_sample().empty());

    sampler.add("cherry", 2.0);
    EXPECT_FALSE(sampler.empty());
    EXPECT_EQ(sampler.sample_size(), 1);
    ASSERT_EQ(sampler.get_sample().size(), 1);
    EXPECT_EQ(sampler.get_sample()[0], "cherry");
}

TEST_F(WeightedReservoirSamplerTest, MoveSemanticsForItems) {
    cpp_utils::WeightedReservoirSampler<std::string> sampler(1);
    std::string s1 = "movable_string_1";
    std::string s2 = "movable_string_2";

    sampler.add(std::move(s1), 10.0);
    EXPECT_TRUE(s1.empty());
    EXPECT_EQ(sampler.sample_size(), 1);
    ASSERT_EQ(sampler.get_sample().size(), 1);
    EXPECT_EQ(sampler.get_sample()[0], "movable_string_1");

    sampler.add(std::move(s2), 100.0); // Higher weight
    EXPECT_TRUE(s2.empty());
    EXPECT_EQ(sampler.sample_size(), 1);
    ASSERT_EQ(sampler.get_sample().size(), 1);
    EXPECT_EQ(sampler.get_sample()[0], "movable_string_2");
}

TEST_F(WeightedReservoirSamplerTest, StatisticalDistributionTendency) {
    const int num_trials = 20000;
    const size_t k = 1;
    std::map<char, int> counts;
    double weightA = 90.0;
    double weightB = 9.0;
    double weightC = 1.0;

    for (int i = 0; i < num_trials; ++i) {
        cpp_utils::WeightedReservoirSampler<char> sampler(k, i);
        sampler.add('A', weightA);
        sampler.add('B', weightB);
        sampler.add('C', weightC);
        sampler.add('X', 0.01);
        sampler.add('Y', 0.001);

        std::vector<char> sample_vec = sampler.get_sample();
        if (!sample_vec.empty()) {
            ASSERT_EQ(sample_vec.size(), k);
            counts[sample_vec[0]]++;
        }
    }

    EXPECT_GT(counts['A'], counts['B']);
    EXPECT_GT(counts['B'], counts['C']);
    EXPECT_GT(counts['A'], num_trials * 0.70);
    EXPECT_LT(counts['C'], num_trials * 0.15);
    EXPECT_LT(counts['X'], num_trials * 0.05);
    EXPECT_LT(counts['Y'], num_trials * 0.05);

    // Test with k=2
    const size_t k2 = 2;
    std::map<char, int> counts_k2;
    for (int i = 0; i < num_trials; ++i) {
        cpp_utils::WeightedReservoirSampler<char> sampler_k2(k2, num_trials + i);
        sampler_k2.add('A', weightA);
        sampler_k2.add('B', weightB);
        sampler_k2.add('C', weightC);
        sampler_k2.add('D', 30.0);
        sampler_k2.add('E', 0.5);

        std::vector<char> sample_vec = sampler_k2.get_sample();
        if (sample_vec.size() == k2) {
            for (char c_item : sample_vec) {
                counts_k2[c_item]++;
            }
        }
    }

    EXPECT_GT(counts_k2['A'], num_trials * 0.85);
    EXPECT_GT(counts_k2['D'], num_trials * 0.50);
    EXPECT_GT(counts_k2['D'], counts_k2['B']);
    EXPECT_LT(counts_k2['C'], num_trials * 0.20);
    EXPECT_LT(counts_k2['E'], num_trials * 0.20);
}

TEST_F(WeightedReservoirSamplerTest, WithManyItems) {
    cpp_utils::WeightedReservoirSampler<int> sampler(10);
    for(int i = 0; i < 1000; ++i) {
        // Use a simple pseudo-random weight for consistency if rand() seed is not fixed per test run.
        // GTest typically doesn't fix global rand() seed unless done explicitly by user.
        // For this test, exact weights don't matter as much as just running through many items.
        sampler.add(i, static_cast<double>((i % 100) + 1));
    }
    EXPECT_EQ(sampler.sample_size(), 10);
    std::vector<int> sample_vec = sampler.get_sample();
    EXPECT_EQ(sample_vec.size(), 10);
}

// It's good practice to have a main function defined for GTest,
// but tests/CMakeLists.txt links against GTest::gtest_main, which provides it.
// So, no explicit main() is needed here.
/*
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
*/

#include "counting_bloom_filter.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>
#include <set>

// Using the cpp_utils namespace
using namespace cpp_utils;

// Test fixture for CountingBloomFilter
class CountingBloomFilterTest : public ::testing::Test {
protected:
    // Per-test set-up and tear-down can be done here if needed
};

TEST_F(CountingBloomFilterTest, ConstructorValidation) {
    EXPECT_THROW(CountingBloomFilter<int>(0, 0.1), std::invalid_argument);
    EXPECT_THROW(CountingBloomFilter<int>(100, 0.0), std::invalid_argument);
    EXPECT_THROW(CountingBloomFilter<int>(100, 1.0), std::invalid_argument);
    EXPECT_NO_THROW(CountingBloomFilter<int>(100, 0.01));
}

TEST_F(CountingBloomFilterTest, BasicAddContainsInt) {
    CountingBloomFilter<int> cbf(100, 0.01);
    cbf.add(42);
    EXPECT_TRUE(cbf.contains(42));
    EXPECT_FALSE(cbf.contains(43));

    cbf.add(100);
    EXPECT_TRUE(cbf.contains(100));
    EXPECT_TRUE(cbf.contains(42)); // Still there
    EXPECT_FALSE(cbf.contains(101));
}

TEST_F(CountingBloomFilterTest, BasicAddContainsString) {
    CountingBloomFilter<std::string> cbf(100, 0.01);
    cbf.add("hello");
    EXPECT_TRUE(cbf.contains("hello"));
    EXPECT_FALSE(cbf.contains("world"));

    cbf.add("world");
    EXPECT_TRUE(cbf.contains("world"));
    EXPECT_TRUE(cbf.contains("hello"));
    EXPECT_FALSE(cbf.contains("test"));
}

TEST_F(CountingBloomFilterTest, RemoveInt) {
    CountingBloomFilter<int> cbf(100, 0.01);
    cbf.add(42);
    cbf.add(100);

    EXPECT_TRUE(cbf.contains(42));
    EXPECT_TRUE(cbf.remove(42)); // Remove existing
    EXPECT_FALSE(cbf.contains(42)); // Should be gone (high probability for small test)

    EXPECT_TRUE(cbf.contains(100)); // Other item still there
    EXPECT_FALSE(cbf.remove(999));   // Try removing non-existent
    EXPECT_TRUE(cbf.contains(100)); // Still there
}

TEST_F(CountingBloomFilterTest, RemoveString) {
    CountingBloomFilter<std::string> cbf(100, 0.01);
    cbf.add("apple");
    cbf.add("banana");

    EXPECT_TRUE(cbf.contains("apple"));
    EXPECT_TRUE(cbf.remove("apple"));
    EXPECT_FALSE(cbf.contains("apple"));

    EXPECT_TRUE(cbf.contains("banana"));
    EXPECT_FALSE(cbf.remove("orange"));
    EXPECT_TRUE(cbf.contains("banana"));
}

TEST_F(CountingBloomFilterTest, MultipleAddsAndRemoves) {
    CountingBloomFilter<int> cbf(100, 0.01);
    cbf.add(10);
    cbf.add(10); // Add same item multiple times
    cbf.add(20);

    EXPECT_TRUE(cbf.contains(10));
    EXPECT_TRUE(cbf.contains(20));

    EXPECT_TRUE(cbf.remove(10)); // First remove
    EXPECT_TRUE(cbf.contains(10)); // Should still be "present" due to second add

    EXPECT_TRUE(cbf.remove(10)); // Second remove
    EXPECT_FALSE(cbf.contains(10)); // Now should be gone

    EXPECT_TRUE(cbf.contains(20));
    EXPECT_TRUE(cbf.remove(20));
    EXPECT_FALSE(cbf.contains(20));
}

TEST_F(CountingBloomFilterTest, CounterSaturation) {
    // Using uint8_t counters, max value 255
    CountingBloomFilter<int, uint8_t> cbf(10, 0.01); // Small capacity

    // Add item more than 255 times to saturate counters
    for (int i = 0; i < 300; ++i) {
        cbf.add(77);
    }
    EXPECT_TRUE(cbf.contains(77)) << "Item should be present after 300 adds.";

    // Remove 254 times. Item should still be present as counters go from 255 down to 1.
    for (int i = 0; i < 254; ++i) {
        EXPECT_TRUE(cbf.remove(77)) << "Remove #" << i + 1 << " should succeed.";
        EXPECT_TRUE(cbf.contains(77)) << "Contained after " << i + 1 << " removes.";
    }

    // The 255th remove. Counters for item 77 should go from 1 to 0.
    // remove() should still return true because it was contained before this remove.
    EXPECT_TRUE(cbf.remove(77)) << "255th remove should succeed.";
    // After this remove, the item should no longer be contained.
    EXPECT_FALSE(cbf.contains(77)) << "Not contained after 255 removes (counters are now 0).";

    // Attempting to remove again should return false (as item is no longer contained based on the initial check in remove())
    EXPECT_FALSE(cbf.remove(77)) << "256th remove should fail (item not present).";
    EXPECT_FALSE(cbf.contains(77)) << "Still not contained after attempting 256th remove.";
}


TEST_F(CountingBloomFilterTest, FalsePositiveRateSmokeTest) {
    // This is a smoke test, not a statistically rigorous FP rate test.
    // A true FP rate test would require many more items and queries.
    int num_insertions = 1000;
    double fp_rate = 0.01;
    CountingBloomFilter<int> cbf(num_insertions, fp_rate);

    std::set<int> added_items;
    for (int i = 0; i < num_insertions; ++i) {
        cbf.add(i);
        added_items.insert(i);
    }

    int false_positives = 0;
    int queries = 0;
    for (int i = num_insertions; i < num_insertions * 2; ++i) {
        if (added_items.find(i) == added_items.end()) { // Ensure it wasn't actually added
            queries++;
            if (cbf.contains(i)) {
                false_positives++;
            }
        }
    }

    // For a small number of queries, the actual FP might deviate.
    // We expect it to be roughly around fp_rate * queries.
    // This test primarily ensures it's not excessively high.
    // Allow some leeway, e.g., up to 2x or 3x the expected for a smoke test.
    double observed_fp_rate = static_cast<double>(false_positives) / queries;
    EXPECT_LT(observed_fp_rate, fp_rate * 3.0 + 0.05) // +0.05 for small query sets
        << "Observed FP rate " << observed_fp_rate
        << " vs expected " << fp_rate
        << " (False Positives: " << false_positives << "/" << queries << ")";
    // Also check it's not zero if fp_rate > 0, which would be suspicious for a Bloom filter
    if (fp_rate > 0 && num_insertions > 100) { // Only if FPs are actually expected
         // It's possible, though unlikely with 1000 queries, to get 0 FPs if k or m is large.
         // This is a loose check.
         // EXPECT_GT(false_positives, 0) << "Expected some false positives with this configuration.";
    }
    std::cout << "FP Smoke Test: " << false_positives << " FPs in " << queries << " queries for items not added. (Rate: " << observed_fp_rate << ")" << std::endl;
}

TEST_F(CountingBloomFilterTest, DifferentCounterType) {
    CountingBloomFilter<std::string, uint16_t> cbf(50, 0.05);
    cbf.add("test_item");
    EXPECT_TRUE(cbf.contains("test_item"));
    cbf.remove("test_item");
    EXPECT_FALSE(cbf.contains("test_item"));
}

TEST_F(CountingBloomFilterTest, RemoveCorrectnessAfterMultipleAdds) {
    CountingBloomFilter<int> cbf(100, 0.01);
    cbf.add(1);
    cbf.add(1);
    cbf.add(2);
    cbf.add(1); // Item 1 added 3 times

    EXPECT_TRUE(cbf.contains(1));
    EXPECT_TRUE(cbf.contains(2));

    cbf.remove(1); // Remove 1 (1st time)
    EXPECT_TRUE(cbf.contains(1)); // Still there

    cbf.remove(2); // Remove 2 (1st time)
    EXPECT_FALSE(cbf.contains(2)); // Should be gone

    cbf.remove(1); // Remove 1 (2nd time)
    EXPECT_TRUE(cbf.contains(1)); // Still there

    cbf.remove(1); // Remove 1 (3rd time)
    EXPECT_FALSE(cbf.contains(1)); // Should be gone now

    EXPECT_FALSE(cbf.remove(1)); // Try removing again
    EXPECT_FALSE(cbf.remove(2)); // Try removing again
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

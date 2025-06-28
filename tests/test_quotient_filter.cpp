#include "QuotientFilter.h"
#include "gtest/gtest.h"
#include <vector>
#include <string>
#include <iomanip> // For std::fixed, std::setprecision
#include <unordered_set> // For FPR test
#include <stdexcept> // For std::invalid_argument, std::runtime_error

// Test fixture for QuotientFilter tests
class QuotientFilterTest : public ::testing::Test {
protected:
    // You can define helper functions or setup/teardown logic here if needed
};

TEST_F(QuotientFilterTest, Construction) {
    QuotientFilter<int> qf(1000, 0.01);
    EXPECT_TRUE(qf.empty());
    EXPECT_EQ(qf.size(), 0);

    QuotientFilter<std::string> qf_str(500, 0.001);
    EXPECT_TRUE(qf_str.empty());
    EXPECT_EQ(qf_str.size(), 0);

    // Outputting details for manual inspection (optional in unit tests)
    std::cout << "QF(1000, 0.01): q_bits=" << (int)qf.quotient_bits()
              << ", r_bits=" << (int)qf.remainder_bits()
              << ", num_slots=" << qf.num_slots()
              << ", capacity_approx=" << qf.capacity() << std::endl;

    EXPECT_THROW(QuotientFilter<int> qf_invalid(0, 0.01), std::invalid_argument);
    EXPECT_THROW(QuotientFilter<int> qf_invalid(100, 0.0), std::invalid_argument);
    EXPECT_THROW(QuotientFilter<int> qf_invalid(100, 1.0), std::invalid_argument);
}

TEST_F(QuotientFilterTest, SimpleAddLookup) {
    QuotientFilter<int> qf(100, 0.01);

    qf.add(42);
    EXPECT_EQ(qf.size(), 1);
    EXPECT_TRUE(qf.might_contain(42));
    EXPECT_FALSE(qf.might_contain(100));

    qf.add(42); // Adding same item again
    EXPECT_EQ(qf.size(), 1); // Size should ideally remain 1 if duplicates are not counted or handled.
                             // Current implementation's `add` first checks `might_contain`.
                             // If it's true, it doesn't add again, so size doesn't change.

    qf.add(123);
    EXPECT_EQ(qf.size(), 2);
    EXPECT_TRUE(qf.might_contain(123));
    EXPECT_TRUE(qf.might_contain(42)); // Should still contain the old item
}

TEST_F(QuotientFilterTest, MultipleItems) {
    QuotientFilter<int> qf(200, 0.01);
    std::vector<int> items_to_add;
    for(int i=0; i<100; ++i) {
        items_to_add.push_back(i * 10); // 0, 10, 20, ... 990
    }

    for(int item : items_to_add) {
        qf.add(item);
    }
    EXPECT_EQ(qf.size(), 100);

    for(int item : items_to_add) {
        EXPECT_TRUE(qf.might_contain(item)) << "Item " << item << " not found after adding.";
    }

    // The check for non-added items is implicitly covered by the FPR test,
    // as it checks items known not to be in the filter.
    // A direct check here would look like:
    // EXPECT_FALSE(qf.might_contain(1)); // Or some other value not added
    // But this can be a false positive, so not asserting it strictly here.
}

TEST_F(QuotientFilterTest, StringItems) {
    QuotientFilter<std::string> qf(100, 0.01);

    qf.add("hello");
    qf.add("world");
    qf.add("quotient");
    qf.add("filter");

    EXPECT_EQ(qf.size(), 4);

    EXPECT_TRUE(qf.might_contain("hello"));
    EXPECT_TRUE(qf.might_contain("world"));
    EXPECT_TRUE(qf.might_contain("quotient"));
    EXPECT_TRUE(qf.might_contain("filter"));

    EXPECT_FALSE(qf.might_contain("test"));
    EXPECT_FALSE(qf.might_contain("")); // Empty string
}

TEST_F(QuotientFilterTest, FullBehavior) {
    // This test is heuristic, aiming to fill the filter close to its capacity.
    // For QF(5, 0.1): num_slots should be 8. Capacity approx 8 * 0.9 = 7.2.
    QuotientFilter<int> qf(5, 0.1);
    std::cout << "QF(5, 0.1): q_bits=" << (int)qf.quotient_bits()
              << ", r_bits=" << (int)qf.remainder_bits()
              << ", num_slots=" << qf.num_slots()
              << ", capacity_approx=" << qf.capacity() << std::endl;

    ASSERT_GT(qf.num_slots(), 0) << "Filter configured with 0 slots, cannot test full behavior.";

    size_t num_to_add = qf.num_slots(); // Try to fill up to physical slots

    for (size_t i = 0; i < num_to_add; ++i) {
        // Using ASSERT_NO_THROW for each add to ensure no unexpected errors during filling.
        ASSERT_NO_THROW(qf.add(static_cast<int>(i * 101 + 1))) << "Failed to add item " << i;
    }
    EXPECT_EQ(qf.size(), num_to_add);

    // Try to add one more item, expecting a "full" exception.
    // The QuotientFilter::add method throws std::runtime_error when full.
    EXPECT_THROW(qf.add(static_cast<int>(num_to_add * 101 + 1 + 1)), std::runtime_error);
}


TEST_F(QuotientFilterTest, FalsePositiveRate) {
    size_t num_insertions = 10000;
    double target_fp_rate = 0.01; // 1%
    QuotientFilter<int> qf(num_insertions, target_fp_rate);

    std::unordered_set<int> inserted_elements;
    for (size_t i = 0; i < num_insertions; ++i) {
        int val = static_cast<int>((i * 0x9E3779B9 + 0x61C88647) % 0xFFFFFFFF);
        // Ensure uniqueness for insertion if the values can collide, though for this large range, it's unlikely for 10k items.
        // However, if val was already added (due to collision in generation or by chance),
        // qf.add might not increment size. We want to ensure `num_insertions` unique items are processed.
        // A simple way is to add to set first, then add to QF, and check QF size against set size.
        if (inserted_elements.find(val) == inserted_elements.end()) {
            qf.add(val);
            inserted_elements.insert(val);
        }
    }
    // Re-check size based on actual unique insertions if the loop above changes
    EXPECT_EQ(qf.size(), inserted_elements.size()) << "Size after insertions should match unique inserted elements.";


    size_t num_lookups = 100000; // Number of items to check that were NOT inserted
    size_t false_positives = 0;
    size_t true_negatives_tested = 0;

    for (size_t i = 0; i < num_lookups; ++i) {
        int val_to_check = static_cast<int>(((i + num_insertions) * 0x1B873593 + 0x91E10DE5) % 0xFFFFFFFF);

        int attempts = 0;
        while(inserted_elements.count(val_to_check) && attempts < 100) {
            val_to_check++; // Simple way to get a new number not in the inserted set
            attempts++;
        }
        if(inserted_elements.count(val_to_check)) {
            // If after 100 attempts we still hit an inserted item, something is odd or sets are too dense.
            // For this test, we'll just skip this lookup to avoid skewing FPR.
            std::cerr << "Warning: Could not find a non-inserted test value easily for FPR, skipping one lookup." << std::endl;
            continue;
        }

        true_negatives_tested++;
        if (qf.might_contain(val_to_check)) {
            false_positives++;
        }
    }

    ASSERT_GT(true_negatives_tested, 0) << "No non-inserted items were tested for FPR.";

    double actual_fp_rate = static_cast<double>(false_positives) / true_negatives_tested;

    std::cout << std::fixed << std::setprecision(5);
    std::cout << "[INFO] Target FP Rate: " << target_fp_rate << std::endl;
    std::cout << "[INFO] Actual FP Rate: " << actual_fp_rate
              << " (FP: " << false_positives << " / TN_Tested: " << true_negatives_tested << ")" << std::endl;

    // Allow some leeway for probabilistic data structures.
    // For example, actual FPR can be up to 2.5x the target, or a small absolute value if target is very small.
    // This is a heuristic common in bloom filter tests.
    bool fpr_within_bounds = actual_fp_rate < (target_fp_rate * 2.5) || (actual_fp_rate < 0.001 && target_fp_rate < 0.001);
    if (num_insertions < 1000) { // More leeway for smaller N as variance is higher
        fpr_within_bounds = actual_fp_rate < (target_fp_rate * 5.0);
    }

    EXPECT_TRUE(fpr_within_bounds)
        << "Actual FPR (" << actual_fp_rate << ") exceeds acceptable bound for target (" << target_fp_rate << ").";
}

// No main() function is needed; gtest_main will provide one.

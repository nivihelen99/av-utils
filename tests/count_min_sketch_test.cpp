#include "gtest/gtest.h"
#include "count_min_sketch.h" // Assuming it's in the include path
#include <string>
#include <vector>
#include <cmath> // For std::exp, std::log, std::ceil

// Test fixture for CountMinSketch tests
class CountMinSketchTest : public ::testing::Test {
protected:
    // You can define helper functions or setup/teardown logic here if needed
};

// Test constructor with valid parameters and check calculated width/depth
TEST_F(CountMinSketchTest, ConstructorValidParameters) {
    double epsilon = 0.01; // Error factor
    double delta = 0.01;   // Error probability

    CountMinSketch<int> sketch(epsilon, delta);

    size_t expected_width = static_cast<size_t>(std::ceil(std::exp(1.0) / epsilon));
    size_t expected_depth = static_cast<size_t>(std::ceil(std::log(1.0 / delta)));

    EXPECT_EQ(sketch.get_width(), expected_width);
    EXPECT_EQ(sketch.get_depth(), expected_depth);
    EXPECT_EQ(sketch.get_error_factor_epsilon(), epsilon);
    EXPECT_EQ(sketch.get_error_probability_delta(), delta);
}

// Test constructor with invalid epsilon
TEST_F(CountMinSketchTest, ConstructorInvalidEpsilon) {
    EXPECT_THROW(CountMinSketch<int>(0.0, 0.1), std::invalid_argument);
    EXPECT_THROW(CountMinSketch<int>(1.0, 0.1), std::invalid_argument);
    EXPECT_THROW(CountMinSketch<int>(-0.1, 0.1), std::invalid_argument);
    EXPECT_THROW(CountMinSketch<int>(1.1, 0.1), std::invalid_argument);
}

// Test constructor with invalid delta
TEST_F(CountMinSketchTest, ConstructorInvalidDelta) {
    EXPECT_THROW(CountMinSketch<int>(0.1, 0.0), std::invalid_argument);
    EXPECT_THROW(CountMinSketch<int>(0.1, 1.0), std::invalid_argument);
    EXPECT_THROW(CountMinSketch<int>(0.1, -0.1), std::invalid_argument);
    EXPECT_THROW(CountMinSketch<int>(0.1, 1.1), std::invalid_argument);
}

// Test adding a single item and estimating its count
TEST_F(CountMinSketchTest, AddAndEstimateSingleItemInt) {
    CountMinSketch<int> sketch(0.01, 0.01);
    sketch.add(123, 5);
    EXPECT_GE(sketch.estimate(123), 5); // Estimate must be >= true count
}

TEST_F(CountMinSketchTest, AddAndEstimateSingleItemString) {
    CountMinSketch<std::string> sketch(0.01, 0.01);
    std::string item = "test_string";
    sketch.add(item, 10);
    EXPECT_GE(sketch.estimate(item), 10);
}

// Test estimating an item not added
TEST_F(CountMinSketchTest, EstimateItemNotAdded) {
    CountMinSketch<int> sketch(0.01, 0.01);
    sketch.add(123, 5);
    // Item 456 was not added. Its estimate could be > 0 due to collisions,
    // but it's often 0 for a sparse sketch.
    // We can't guarantee 0, but we know it shouldn't be related to item 123's count.
    // For a very high epsilon (e.g., 0.5, width ~5), collisions are more likely.
    // For low epsilon (e.g., 0.001, width ~2718), collisions for non-added items are less likely to be high.
    EXPECT_GE(sketch.estimate(456), 0); // Must be non-negative
}

// Test adding the same item multiple times
TEST_F(CountMinSketchTest, AddSameItemMultipleTimes) {
    CountMinSketch<int> sketch(0.01, 0.01);
    sketch.add(789, 3);
    sketch.add(789, 4);
    sketch.add(789, 2); // Total count = 9
    EXPECT_GE(sketch.estimate(789), 9);
}

// Test adding multiple distinct items
TEST_F(CountMinSketchTest, AddMultipleDistinctItems) {
    CountMinSketch<std::string> sketch(0.001, 0.001); // More precision
    std::string item_apple = "apple";
    std::string item_banana = "banana";
    std::string item_cherry = "cherry";

    sketch.add(item_apple, 100);
    sketch.add(item_banana, 200);
    sketch.add(item_cherry, 50);

    EXPECT_GE(sketch.estimate(item_apple), 100);
    EXPECT_GE(sketch.estimate(item_banana), 200);
    EXPECT_GE(sketch.estimate(item_cherry), 50);

    // Check that estimates are not wildly off due to collisions (probabilistic)
    // This is harder to assert definitively without knowing total sum of counts.
    // We expect estimate(X) >= count(X)
    // And estimate(X) <= count(X) + epsilon * TotalCountsInSketch
    // For now, just checking lower bound.
}

// Test with default count value for add method
TEST_F(CountMinSketchTest, AddWithDefaultCount) {
    CountMinSketch<int> sketch(0.01, 0.01);
    sketch.add(111); // Default count is 1
    sketch.add(111);
    EXPECT_GE(sketch.estimate(111), 2);
}

// Test adding with count 0 (should have no effect)
TEST_F(CountMinSketchTest, AddWithZeroCount) {
    CountMinSketch<int> sketch(0.01, 0.01);
    sketch.add(222, 5);
    sketch.add(222, 0); // Adding 0
    EXPECT_GE(sketch.estimate(222), 5);
    // It should ideally be exactly 5 if no other items cause collisions on these specific hashes.
    // To be more precise, we can check if it's less than 5 + some error margin, but that's complex.
    // For now, ensuring it's at least 5 and not excessively more is a basic check.
    // A more direct test: estimate, add 0, estimate again.
    unsigned int estimate_before = sketch.estimate(222);
    sketch.add(222, 0);
    unsigned int estimate_after = sketch.estimate(222);
    EXPECT_EQ(estimate_before, estimate_after);
}

// Test counter overflow capping
TEST_F(CountMinSketchTest, CounterOverflow) {
    CountMinSketch<int> sketch(0.1, 0.1); // Smaller sketch to make it easier to hit same cells
    unsigned int max_val = std::numeric_limits<unsigned int>::max();

    // Pick an item. Its hashes will hit certain cells.
    int item_to_overflow = 12345;

    // Add a large value close to max_val to one item.
    // This requires knowing which cells it hashes to, or adding it many times.
    // A simpler way: add max_val, then add 1. It should be capped at max_val.
    sketch.add(item_to_overflow, max_val - 10);
    sketch.add(item_to_overflow, 5); // Now at max_val - 5
    EXPECT_GE(sketch.estimate(item_to_overflow), max_val - 10); // Should be at least original

    sketch.add(item_to_overflow, 20); // This should cause overflow and cap at max_val
    EXPECT_EQ(sketch.estimate(item_to_overflow), max_val);

    // Add more, should remain capped
    sketch.add(item_to_overflow, 100);
    EXPECT_EQ(sketch.estimate(item_to_overflow), max_val);
}

// Test with minimal width/depth (epsilon/delta close to 1)
TEST_F(CountMinSketchTest, MinimalSketchParameters) {
    // Epsilon very high (e.g., 0.99) -> width should be ceil(e/0.99) approx ceil(2.718/0.99) = 3
    // Delta very high (e.g., 0.99) -> depth should be ceil(ln(1/0.99)) approx ceil(ln(1.01)) = 1
    CountMinSketch<int> sketch(0.9, 0.9); // Should result in small w, d

    size_t expected_width = static_cast<size_t>(std::ceil(std::exp(1.0) / 0.9)); // approx 3
    size_t expected_depth = static_cast<size_t>(std::ceil(std::log(1.0 / 0.9))); // approx 1

    EXPECT_EQ(sketch.get_width(), expected_width);
    EXPECT_EQ(sketch.get_depth(), expected_depth);

    sketch.add(1, 10);
    sketch.add(2, 20); // High chance of collision in such a small sketch

    EXPECT_GE(sketch.estimate(1), 10);
    EXPECT_GE(sketch.estimate(2), 20);
    // With depth 1, estimate(1) will be exactly counter[0][h(1)%width]
    // estimate(2) will be exactly counter[0][h(2)%width]
    // If h(1)%width == h(2)%width, then estimate(1) would be sum of counts.
}


// Test with custom struct (needs is_trivially_copyable or is_standard_layout for default hash)
struct MyStruct {
    int id;
    double value;

    // Equality operator for potential use in tests, not strictly needed by sketch itself if not used as key in map
    bool operator==(const MyStruct& other) const {
        return id == other.id && value == other.value;
    }
};

// Ensure MyStruct is suitable for the default hasher
static_assert(std::is_trivially_copyable<MyStruct>::value || std::is_standard_layout<MyStruct>::value,
              "MyStruct must be trivially copyable or standard layout for default CountMinSketchHash.");


TEST_F(CountMinSketchTest, CustomStructBasic) {
    CountMinSketch<MyStruct> sketch(0.01, 0.01);
    MyStruct s1 = {1, 10.5};
    MyStruct s2 = {2, 20.5};

    sketch.add(s1, 5);
    sketch.add(s2, 8);

    EXPECT_GE(sketch.estimate(s1), 5);
    EXPECT_GE(sketch.estimate(s2), 8);

    MyStruct s3 = {1, 10.5}; // Same as s1
    EXPECT_GE(sketch.estimate(s3), 5);

    MyStruct s4 = {3, 30.5}; // Not added
    EXPECT_GE(sketch.estimate(s4), 0);
}

// A more involved test to observe error bounds (qualitatively)
// This is hard to assert strictly without knowing total counts and specific hash values
TEST_F(CountMinSketchTest, ErrorBoundObservation) {
    double epsilon = 0.1; // 10% error factor relative to total count sum
    double delta = 0.1;   // 90% confidence
    CountMinSketch<int> sketch(epsilon, delta);

    int num_items = 100;
    unsigned int count_per_item = 10;
    unsigned int total_sum_of_counts = 0;

    for (int i = 0; i < num_items; ++i) {
        sketch.add(i, count_per_item);
        total_sum_of_counts += count_per_item;
    }

    // For each item, estimate should be >= count_per_item
    // And with probability 1-delta, estimate <= count_per_item + epsilon * total_sum_of_counts
    unsigned int error_margin = static_cast<unsigned int>(std::ceil(epsilon * total_sum_of_counts));

    int items_within_bounds = 0;
    int items_overestimated_correctly = 0; // estimate >= true_count

    for (int i = 0; i < num_items; ++i) {
        unsigned int estimate = sketch.estimate(i);
        EXPECT_GE(estimate, count_per_item);
        if (estimate >= count_per_item) {
            items_overestimated_correctly++;
        }
        // This is the probabilistic part
        if (estimate <= count_per_item + error_margin) {
            items_within_bounds++;
        }
        // For debugging:
        // if (estimate > count_per_item + error_margin) {
        //     std::cout << "Item " << i << ": estimate " << estimate
        //               << " > true_count " << count_per_item
        //               << " + error_margin " << error_margin << std::endl;
        // }
    }
    EXPECT_EQ(items_overestimated_correctly, num_items); // This must hold

    // We expect most items (around (1-delta)*num_items) to be within the error bound.
    // This is a probabilistic assertion, so it might flake in rare cases.
    // A common way is to check if it's a high percentage, e.g., > (1-delta-buffer)*num_items
    double expected_min_within_bounds = (1.0 - delta - 0.05) * num_items; // allow a small buffer for test flakiness
    EXPECT_GE(items_within_bounds, expected_min_within_bounds)
        << "Items within bounds: " << items_within_bounds << "/" << num_items
        << ", expected at least " << expected_min_within_bounds;

    // Check a non-existent item
    unsigned int estimate_non_existent = sketch.estimate(num_items + 100);
    EXPECT_GE(estimate_non_existent, 0);
    // With probability 1-delta, its estimate should be <= epsilon * total_sum_of_counts
    // This is harder to assert strictly for a single non-existent item.
    // Typically, estimates for non-existent items are low.
    EXPECT_LE(estimate_non_existent, error_margin)
        << "Estimate for non-existent item " << estimate_non_existent
        << " exceeded error margin " << error_margin;
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

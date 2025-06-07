#include "gtest/gtest.h"
#include "fenwick_tree.h" // Should be found via include_directories in CMake
#include <vector>
#include <numeric> // For std::accumulate in naive sum or other checks
#include <limits>  // For std::numeric_limits

// Note: Removed <iostream> as GTest handles output.
// Note: Removed forward declarations and old main().

// UT-1: Initialization Tests
TEST(FenwickTreeTest, Initialization) {
    // Default constructor
    FenwickTree ft_default(10);
    ASSERT_EQ(ft_default.size(), 10);
    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(ft_default.get(i), 0LL);
    }
    ASSERT_EQ(ft_default.prefixSum(9), 0LL);
    ASSERT_EQ(ft_default.prefixSum(0), 0LL); // Check specific index

    // Array constructor
    std::vector<long long> arr_ll = {1LL, 2LL, 3LL, 4LL, 5LL};
    FenwickTree ft_arr(arr_ll);
    ASSERT_EQ(ft_arr.size(), 5);
    for (int i = 0; i < 5; ++i) {
        ASSERT_EQ(ft_arr.get(i), arr_ll[i]);
    }
    ASSERT_EQ(ft_arr.prefixSum(4), (1LL + 2LL + 3LL + 4LL + 5LL));
    ASSERT_EQ(ft_arr.prefixSum(0), 1LL);

    // Size 0 tree (constructor for size)
    FenwickTree ft_zero_size(0);
    ASSERT_EQ(ft_zero_size.size(), 0);
    ASSERT_EQ(ft_zero_size.prefixSum(-1), 0LL); // Should be safe as per FenwickTree implementation

    // Size 0 tree (constructor for empty vector)
    std::vector<long long> empty_arr = {};
    FenwickTree ft_empty_vec(empty_arr);
    ASSERT_EQ(ft_empty_vec.size(), 0);
    ASSERT_EQ(ft_empty_vec.prefixSum(-1), 0LL); // Should be safe
}

// UT-2: Update and Set Tests
TEST(FenwickTreeTest, UpdateAndSet) {
    FenwickTree ft(5);
    ft.update(0, 10LL);
    ASSERT_EQ(ft.get(0), 10LL);

    ft.update(0, -5LL);
    ASSERT_EQ(ft.get(0), 5LL); // 10 - 5 = 5

    ft.update(2, 100LL);
    ASSERT_EQ(ft.get(2), 100LL);

    ft.update(4, 20LL);
    ASSERT_EQ(ft.get(4), 20LL);

    ASSERT_EQ(ft.prefixSum(4), (5LL + 0LL + 100LL + 0LL + 20LL)); // Elements at 1 and 3 are 0

    ft.set(2, 50LL);
    ASSERT_EQ(ft.get(2), 50LL); // Set value, replaces 100
    ASSERT_EQ(ft.prefixSum(4), (5LL + 0LL + 50LL + 0LL + 20LL));

    ft.set(2, -5LL);
    ASSERT_EQ(ft.get(2), -5LL); // Set negative value
    ASSERT_EQ(ft.prefixSum(4), (5LL + 0LL + (-5LL) + 0LL + 20LL));

    ft.set(0, 0LL);
    ASSERT_EQ(ft.get(0), 0LL);
    ASSERT_EQ(ft.prefixSum(0), 0LL);
    ASSERT_EQ(ft.prefixSum(4), (0LL + 0LL + (-5LL) + 0LL + 20LL));
}

// UT-3: Query and PrefixSum Tests
TEST(FenwickTreeTest, QueryAndPrefixSum) {
    std::vector<long long> arr_ll = {1LL, 2LL, 3LL, 4LL, 5LL};
    FenwickTree ft(arr_ll);

    ASSERT_EQ(ft.prefixSum(-1), 0LL); // Edge case for prefixSum
    ASSERT_EQ(ft.prefixSum(0), 1LL);
    ASSERT_EQ(ft.prefixSum(2), (1LL + 2LL + 3LL));
    ASSERT_EQ(ft.prefixSum(4), (1LL + 2LL + 3LL + 4LL + 5LL));

    ASSERT_EQ(ft.query(0, 4), (1LL + 2LL + 3LL + 4LL + 5LL));
    ASSERT_EQ(ft.query(1, 3), (2LL + 3LL + 4LL));
    ASSERT_EQ(ft.query(2, 2), 3LL); // Single element range
    ASSERT_EQ(ft.query(0, 0), 1LL); // Single element range at start
    ASSERT_EQ(ft.query(4, 4), 5LL); // Single element range at end
}

// UT-4: Edge Case Tests
TEST(FenwickTreeTest, EdgeCases) {
    // Size 1 tree
    FenwickTree ft1(1);
    ASSERT_EQ(ft1.size(), 1);
    ASSERT_EQ(ft1.get(0), 0LL);
    ft1.update(0, 100LL);
    ASSERT_EQ(ft1.get(0), 100LL);
    ASSERT_EQ(ft1.prefixSum(0), 100LL);
    ft1.set(0, -10LL);
    ASSERT_EQ(ft1.get(0), -10LL);
    ASSERT_EQ(ft1.query(0, 0), -10LL);

    // PrefixSum with -1 (already tested in testQueryAndPrefixSum, but good to reinforce)
    FenwickTree ft_any(5);
    ASSERT_EQ(ft_any.prefixSum(-1), 0LL);

    // Query on empty tree
    FenwickTree ft_empty(0);
    // Note: ft_empty.query(0,0) would assert fail due to bounds in FenwickTree. Correct.
    // No direct queries possible, but prefixSum(-1) is.
    ASSERT_EQ(ft_empty.prefixSum(-1), 0LL);
}

// IT-1: Naive Sum Comparison Test
TEST(FenwickTreeTest, NaiveSumComparison) {
    std::vector<long long> naive_arr_orig = {10LL, 20LL, 30LL, 40LL, 50LL, 5LL, -2LL, 100LL};
    std::vector<long long> naive_arr = naive_arr_orig; // mutable copy
    FenwickTree ft(naive_arr_orig);

    auto calculate_naive_range_sum = [&](int l, int r) {
        long long sum = 0LL;
        if (l < 0 || (size_t)r >= naive_arr.size() || l > r) return 0LL; // Basic bounds check for naive
        for (int i = l; i <= r; ++i) {
            sum += naive_arr[i];
        }
        return sum;
    };

    // Initial check
    long long current_naive_sum_init = 0LL;
    for (size_t i = 0; i < naive_arr.size(); ++i) {
        current_naive_sum_init += naive_arr[i];
        ASSERT_EQ(ft.prefixSum(i), current_naive_sum_init);
    }
    ASSERT_EQ(ft.query(0, naive_arr.size() -1), current_naive_sum_init);

    // Perform some updates
    ft.update(1, 5LL); naive_arr[1] += 5LL;
    ft.update(3, -10LL); naive_arr[3] -= 10LL;
    ft.set(0, 100LL); naive_arr[0] = 100LL;
    ft.update(naive_arr.size() - 1, 2LL); naive_arr[naive_arr.size()-1] += 2LL;

    // Check prefix sums after modifications
    long long current_naive_sum_mod = 0LL;
    for (size_t i = 0; i < naive_arr.size(); ++i) {
        current_naive_sum_mod += naive_arr[i];
        ASSERT_EQ(ft.prefixSum(i), current_naive_sum_mod);
    }

    // Check range sums after modifications
    ASSERT_EQ(ft.query(0, naive_arr.size() - 1), calculate_naive_range_sum(0, naive_arr.size() - 1));
    ASSERT_EQ(ft.query(1, 3), calculate_naive_range_sum(1, 3));
    ASSERT_EQ(ft.query(2, 2), calculate_naive_range_sum(2, 2));
    ASSERT_EQ(ft.query(0, 0), calculate_naive_range_sum(0, 0));
    ASSERT_EQ(ft.query(naive_arr.size() -1, naive_arr.size() -1), calculate_naive_range_sum(naive_arr.size()-1, naive_arr.size()-1));
    ASSERT_EQ(ft.query(1, naive_arr.size() - 2), calculate_naive_range_sum(1, naive_arr.size() - 2));
}

// Test with Large Values (NFR-6)
TEST(FenwickTreeTest, LargeValues) {
    FenwickTree ft(3);
    long long val1 = 2000000000LL; // 2 * 10^9
    long long val2 = 3000000000LL; // 3 * 10^9
    long long val3 = 4000000000LL; // 4 * 10^9
    long long sum_val123 = val1 + val2 + val3; // Approx 9 * 10^9, fits in long long

    ft.update(0, val1);
    ft.update(1, val2);
    ft.update(2, val3);
    ASSERT_EQ(ft.get(0), val1);
    ASSERT_EQ(ft.get(1), val2);
    ASSERT_EQ(ft.get(2), val3);
    ASSERT_EQ(ft.prefixSum(2), sum_val123);
    ASSERT_EQ(ft.query(0,2), sum_val123);
    ASSERT_EQ(ft.query(1,2), (val2 + val3));

    ft.set(1, -val2);
    ASSERT_EQ(ft.get(1), -val2);
    ASSERT_EQ(ft.prefixSum(2), (val1 - val2 + val3));
    ASSERT_EQ(ft.query(0,1), (val1 - val2));

    // Test with values around max/min long long
    FenwickTree ft_limit(2);
    long long max_ll = std::numeric_limits<long long>::max();
    long long min_ll = std::numeric_limits<long long>::min();

    ft_limit.set(0, max_ll / 2);
    ft_limit.set(1, max_ll / 2 - 100); // Ensure sum doesn't overflow
    ASSERT_EQ(ft_limit.prefixSum(1), (max_ll / 2 + max_ll / 2 - 100));

    ft_limit.set(0, min_ll / 2);
    ft_limit.set(1, min_ll / 2 + 100); // Ensure sum doesn't underflow
    ASSERT_EQ(ft_limit.prefixSum(1), (min_ll / 2 + min_ll / 2 + 100));
}

// Adapted from original performance test
TEST(FenwickTreeTest, Performance) {
    const int n_elements = 10000; // Using a fixed smaller N for automated tests
    FenwickTree ft(n_elements);

    // Add 1 to every position
    for (int i = 0; i < n_elements; i++) {
        ft.update(i, 1LL);
    }

    long long expected_sum_half = static_cast<long long>(n_elements) / 2;
    ASSERT_EQ(ft.prefixSum(n_elements/2 - 1), expected_sum_half);

    long long expected_sum_range = static_cast<long long>(n_elements) / 2;
    ASSERT_EQ(ft.query(n_elements/4, n_elements/4 + n_elements/2 -1), expected_sum_range);

    // Set some values
    for (int i = 0; i < n_elements; i+=100) {
        ft.set(i, static_cast<long long>(i)*2);
    }

    // Check a few values
    if (n_elements >= 100) {
      ASSERT_EQ(ft.get(0), 0LL); // Set to 0*2
      ASSERT_EQ(ft.get(100), 200LL); // Set to 100*2
    }
}

// Optional: Slower, more thorough performance test (from original)
// To run this, it might need to be enabled explicitly or run in a different test suite
// For now, it's commented out to keep standard test runs fast.
/*
TEST(FenwickTreeTest, PerformanceLarge) {
    const int n_elements = 1000000;
    FenwickTree ft(n_elements);

    for (int i = 0; i < n_elements; i++) {
        ft.update(i, 1LL);
    }
    ASSERT_EQ(ft.prefixSum(n_elements/2 - 1), static_cast<long long>(n_elements)/2);
    ASSERT_EQ(ft.query(n_elements/4, n_elements/4 + n_elements/2 -1), static_cast<long long>(n_elements)/2);

    for (int i = 0; i < n_elements; i+=100) {
        ft.set(i, static_cast<long long>(i)*2);
    }
    if (n_elements >= 100) {
      ASSERT_EQ(ft.get(0), 0LL);
      ASSERT_EQ(ft.get(100), 200LL);
    }
}
*/

// GTest main function is provided by GTest::gtest_main linked in CMakeLists.txt
// No need for a main() function here.

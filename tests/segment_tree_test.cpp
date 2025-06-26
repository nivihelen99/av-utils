#include "gtest/gtest.h"
#include "segment_tree.h"
#include <vector>
#include <numeric> // For std::iota
#include <limits>  // For std::numeric_limits

// Using the custom MinOp and MaxOp from segment_tree.h for convenience
using cpp_utils::MinOp;
using cpp_utils::MaxOp;

class SegmentTreeTest : public ::testing::Test {
protected:
    // Helper to create a vector for sum tests
    std::vector<int> sum_sample_data_ = {1, 2, 3, 4, 5, 6, 7, 8};
    // Identity for sum is 0
    const int sum_identity_ = 0;

    // Helper for min tests
    std::vector<int> min_max_sample_data_ = {5, 2, 8, 1, 9, 4, 6, 3};
    // Identity for min is positive infinity
    const int min_identity_ = std::numeric_limits<int>::max();
    // Identity for max is negative infinity
    const int max_identity_ = std::numeric_limits<int>::min();
};

// Test constructor with initial values (sum operation)
TEST_F(SegmentTreeTest, ConstructorAndBasicSum) {
    cpp_utils::SegmentTree<int> st(sum_sample_data_, std::plus<int>(), sum_identity_);
    EXPECT_EQ(st.size(), sum_sample_data_.size());
    EXPECT_FALSE(st.empty());
    // Query full range
    EXPECT_EQ(st.query(0, sum_sample_data_.size()), 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8);
}

// Test constructor with size and default value (sum operation)
TEST_F(SegmentTreeTest, ConstructorSizeAndDefaultSum) {
    size_t count = 10;
    int default_val = 5;
    cpp_utils::SegmentTree<int> st(count, default_val, std::plus<int>(), sum_identity_);
    EXPECT_EQ(st.size(), count);
    EXPECT_FALSE(st.empty());
    EXPECT_EQ(st.query(0, count), count * default_val);
    EXPECT_EQ(st.query(0, 1), default_val);
    EXPECT_EQ(st.query(count - 1, count), default_val);
}

// Test update operation (sum)
TEST_F(SegmentTreeTest, UpdateSum) {
    cpp_utils::SegmentTree<int> st(sum_sample_data_, std::plus<int>(), sum_identity_);
    // Initial sum of first 3 elements: 1+2+3 = 6
    EXPECT_EQ(st.query(0, 3), 6);

    // Update index 1 (value 2) to 10
    // New data: {1, 10, 3, 4, 5, 6, 7, 8}
    st.update(1, 10);
    EXPECT_EQ(st.query(0, 3), 1 + 10 + 3); // 14
    EXPECT_EQ(st.query(1, 2), 10);         // Query updated element

    // Full sum
    int current_sum = 0;
    for(int x : {1, 10, 3, 4, 5, 6, 7, 8}) current_sum +=x;
    EXPECT_EQ(st.query(0, sum_sample_data_.size()), current_sum);

    // Update first element
    st.update(0, 20); // {20, 10, 3, 4, 5, 6, 7, 8}
    EXPECT_EQ(st.query(0, 1), 20);
    current_sum = 0;
    for(int x : {20, 10, 3, 4, 5, 6, 7, 8}) current_sum +=x;
    EXPECT_EQ(st.query(0, sum_sample_data_.size()), current_sum);


    // Update last element
    st.update(sum_sample_data_.size() - 1, 100); // {20, 10, 3, 4, 5, 6, 7, 100}
    EXPECT_EQ(st.query(sum_sample_data_.size() - 1, sum_sample_data_.size()), 100);
    current_sum = 0;
    for(int x : {20, 10, 3, 4, 5, 6, 7, 100}) current_sum +=x;
    EXPECT_EQ(st.query(0, sum_sample_data_.size()), current_sum);
}

// Test query operation with various ranges (sum)
TEST_F(SegmentTreeTest, QueryRangesSum) {
    cpp_utils::SegmentTree<int> st(sum_sample_data_, std::plus<int>(), sum_identity_);
    EXPECT_EQ(st.query(0, 1), 1);       // Single element
    EXPECT_EQ(st.query(3, 4), 4);       // Single element
    EXPECT_EQ(st.query(0, sum_sample_data_.size()), 36); // Full range
    EXPECT_EQ(st.query(2, 5), 3 + 4 + 5); // Sub-range: 12
    EXPECT_EQ(st.query(5, 8), 6 + 7 + 8); // Sub-range: 21
    // Empty range
    EXPECT_EQ(st.query(0, 0), sum_identity_);
    EXPECT_EQ(st.query(5, 5), sum_identity_);
}

// Test with MinOp
TEST_F(SegmentTreeTest, MinOperation) {
    cpp_utils::SegmentTree<int, MinOp<int>> st(min_max_sample_data_, MinOp<int>(), min_identity_);
    EXPECT_EQ(st.size(), min_max_sample_data_.size());
    // Query full range min
    EXPECT_EQ(st.query(0, min_max_sample_data_.size()), 1); // Min in {5,2,8,1,9,4,6,3} is 1

    // Query sub-ranges
    EXPECT_EQ(st.query(0, 3), 2); // Min in {5,2,8} is 2
    EXPECT_EQ(st.query(2, 5), 1); // Min in {8,1,9} is 1
    EXPECT_EQ(st.query(4, 7), 4); // Min in {9,4,6} is 4
    EXPECT_EQ(st.query(7, 8), 3); // Min in {3} is 3

    // Update and query
    // {5,2,8,1,9,4,6,3} -> update index 3 (value 1) to 10 -> {5,2,8,10,9,4,6,3}
    st.update(3, 10);
    EXPECT_EQ(st.query(0, min_max_sample_data_.size()), 2); // New min is 2
    EXPECT_EQ(st.query(2, 5), 8); // Min in {8,10,9} is 8

    // Empty range
    EXPECT_EQ(st.query(0,0), min_identity_);
}

// Test with MaxOp
TEST_F(SegmentTreeTest, MaxOperation) {
    cpp_utils::SegmentTree<int, MaxOp<int>> st(min_max_sample_data_, MaxOp<int>(), max_identity_);
    EXPECT_EQ(st.size(), min_max_sample_data_.size());
    // Query full range max
    EXPECT_EQ(st.query(0, min_max_sample_data_.size()), 9); // Max in {5,2,8,1,9,4,6,3} is 9

    // Query sub-ranges
    EXPECT_EQ(st.query(0, 3), 8); // Max in {5,2,8} is 8
    EXPECT_EQ(st.query(2, 5), 9); // Max in {8,1,9} is 9
    EXPECT_EQ(st.query(4, 7), 9); // Max in {9,4,6} is 9
    EXPECT_EQ(st.query(7, 8), 3); // Max in {3} is 3

    // Update and query
    // {5,2,8,1,9,4,6,3} -> update index 4 (value 9) to 0 -> {5,2,8,1,0,4,6,3}
    st.update(4, 0);
    EXPECT_EQ(st.query(0, min_max_sample_data_.size()), 8); // New max is 8
    EXPECT_EQ(st.query(2, 5), 8); // Max in {8,1,0} is 8

    // Empty range
    EXPECT_EQ(st.query(0,0), max_identity_);
}

// Test with double data type
TEST_F(SegmentTreeTest, DoubleTypeSum) {
    std::vector<double> data = {1.5, 2.5, 3.5, 4.5};
    cpp_utils::SegmentTree<double> st(data, std::plus<double>(), 0.0);
    EXPECT_DOUBLE_EQ(st.query(0, data.size()), 1.5 + 2.5 + 3.5 + 4.5);
    EXPECT_DOUBLE_EQ(st.query(1, 3), 2.5 + 3.5);

    st.update(0, 10.0); // {10.0, 2.5, 3.5, 4.5}
    EXPECT_DOUBLE_EQ(st.query(0, data.size()), 10.0 + 2.5 + 3.5 + 4.5);
}

// Test edge cases: empty tree
TEST_F(SegmentTreeTest, EmptyTree) {
    std::vector<int> empty_data;
    cpp_utils::SegmentTree<int> st_vec(empty_data, std::plus<int>(), sum_identity_);
    EXPECT_EQ(st_vec.size(), 0);
    EXPECT_TRUE(st_vec.empty());
    EXPECT_EQ(st_vec.query(0, 0), sum_identity_);


    cpp_utils::SegmentTree<int> st_size(0, 0, std::plus<int>(), sum_identity_);
    EXPECT_EQ(st_size.size(), 0);
    EXPECT_TRUE(st_size.empty());
    EXPECT_EQ(st_size.query(0,0), sum_identity_);

    EXPECT_THROW(st_vec.update(0,10), std::out_of_range);
    EXPECT_THROW(st_size.update(0,10), std::out_of_range);
}

// Test query out of bounds
TEST_F(SegmentTreeTest, QueryOutOfBounds) {
    cpp_utils::SegmentTree<int> st(sum_sample_data_, std::plus<int>(), sum_identity_);
    EXPECT_THROW(st.query(0, sum_sample_data_.size() + 1), std::out_of_range);
    EXPECT_THROW(st.query(1, 0), std::out_of_range); // left > right
    EXPECT_THROW(st.query(sum_sample_data_.size() + 1, sum_sample_data_.size() + 1), std::out_of_range);
    EXPECT_THROW(st.query(sum_sample_data_.size(), sum_sample_data_.size() +1), std::out_of_range); // left is valid, right is out
}

// Test update out of bounds
TEST_F(SegmentTreeTest, UpdateOutOfBounds) {
    cpp_utils::SegmentTree<int> st(sum_sample_data_, std::plus<int>(), sum_identity_);
    EXPECT_THROW(st.update(sum_sample_data_.size(), 100), std::out_of_range);
    EXPECT_THROW(st.update(sum_sample_data_.size() + 10, 100), std::out_of_range);
}

// Test with a single element
TEST_F(SegmentTreeTest, SingleElementTree) {
    std::vector<int> data = {42};
    cpp_utils::SegmentTree<int> st(data, std::plus<int>(), sum_identity_);
    EXPECT_EQ(st.size(), 1);
    EXPECT_EQ(st.query(0, 1), 42);

    st.update(0, 100);
    EXPECT_EQ(st.query(0, 1), 100);

    // Query empty range on single element tree
    EXPECT_EQ(st.query(0,0), sum_identity_);
    EXPECT_EQ(st.query(1,1), sum_identity_);


    EXPECT_THROW(st.query(0,2), std::out_of_range);
    EXPECT_THROW(st.update(1,0), std::out_of_range);
}

// Test with custom struct and lambda for operation
struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
};

Point add_points(const Point& a, const Point& b) {
    return {a.x + b.x, a.y + b.y};
}

TEST_F(SegmentTreeTest, CustomStructAndLambda) {
    std::vector<Point> points = {{1,1}, {2,2}, {3,3}, {4,4}};
    Point identity_point = {0,0};

    auto point_adder_lambda = [](const Point& p1, const Point& p2) {
        return Point{p1.x + p2.x, p1.y + p2.y};
    };

    cpp_utils::SegmentTree<Point, decltype(point_adder_lambda)> st_lambda(points, point_adder_lambda, identity_point);
    EXPECT_EQ(st_lambda.query(0, points.size()), Point({1+2+3+4, 1+2+3+4}));
    EXPECT_EQ(st_lambda.query(1, 3), Point({2+3, 2+3}));

    st_lambda.update(0, {10,10});
    EXPECT_EQ(st_lambda.query(0,1), Point({10,10}));
    EXPECT_EQ(st_lambda.query(0, points.size()), Point({10+2+3+4, 10+2+3+4}));

    // Test with function pointer
    cpp_utils::SegmentTree<Point, Point(*)(const Point&, const Point&)> st_func_ptr(points, &add_points, identity_point);
    EXPECT_EQ(st_func_ptr.query(0, points.size()), Point({1+2+3+4, 1+2+3+4}));
}

// Test constructor with size and default value for custom struct
TEST_F(SegmentTreeTest, ConstructorSizeAndDefaultCustomStruct) {
    size_t count = 5;
    Point default_pt = {1,1};
    Point identity_point = {0,0};
     auto point_adder_lambda = [](const Point& p1, const Point& p2) {
        return Point{p1.x + p2.x, p1.y + p2.y};
    };
    cpp_utils::SegmentTree<Point, decltype(point_adder_lambda)> st(count, default_pt, point_adder_lambda, identity_point);
    EXPECT_EQ(st.size(), count);
    EXPECT_EQ(st.query(0, count), Point({(int)count * default_pt.x, (int)count * default_pt.y}));
}


// Test behavior of query(x,x) for various x
TEST_F(SegmentTreeTest, QueryEmptyRanges) {
    cpp_utils::SegmentTree<int> st(sum_sample_data_, std::plus<int>(), sum_identity_);
    for (size_t i = 0; i <= sum_sample_data_.size(); ++i) {
        EXPECT_EQ(st.query(i, i), sum_identity_) << "Query(" << i << "," << i << ") failed";
    }
}

TEST_F(SegmentTreeTest, ConstructorEmptyVector) {
    std::vector<int> data;
    cpp_utils::SegmentTree<int> st(data, std::plus<int>(), 0);
    EXPECT_EQ(st.size(), 0);
    EXPECT_TRUE(st.empty());
    EXPECT_EQ(st.query(0,0), 0);
}

TEST_F(SegmentTreeTest, ConstructorSizeZero) {
    cpp_utils::SegmentTree<int> st(0, 10, std::plus<int>(), 0);
    EXPECT_EQ(st.size(), 0);
    EXPECT_TRUE(st.empty());
    EXPECT_EQ(st.query(0,0), 0);
}

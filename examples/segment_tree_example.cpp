#include "segment_tree.h"
#include <iostream>
#include <vector>
#include <limits> // For std::numeric_limits

// Using custom operations from segment_tree.h for clarity if needed
using cpp_utils::MinOp;
using cpp_utils::MaxOp;

struct CustomData {
    int sum;
    int count;

    // Default constructor for identity element
    CustomData() : sum(0), count(0) {}
    CustomData(int s, int c) : sum(s), count(c) {}

    // For printing
    friend std::ostream& operator<<(std::ostream& os, const CustomData& cd) {
        if (cd.count == 0) {
            os << "{avg: N/A}";
        } else {
            os << "{sum: " << cd.sum << ", count: " << cd.count << ", avg: " << static_cast<double>(cd.sum) / cd.count << "}";
        }
        return os;
    }
};

CustomData combine_custom_data(const CustomData& a, const CustomData& b) {
    return CustomData(a.sum + b.sum, a.count + b.count);
}


int main() {
    std::cout << "--- Segment Tree Example: Summation ---" << std::endl;
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    cpp_utils::SegmentTree<int> sum_st(data, std::plus<int>(), 0 /* identity for sum */);

    std::cout << "Initial data: ";
    for (size_t i = 0; i < data.size(); ++i) {
        std::cout << sum_st.query(i, i + 1) << (i == data.size() - 1 ? "" : ", ");
    }
    std::cout << std::endl;

    std::cout << "Sum of all elements (0-10): " << sum_st.query(0, data.size()) << std::endl; // Expected: 55
    std::cout << "Sum of elements in range [2, 5) (i.e., index 2, 3, 4): "
              << sum_st.query(2, 5) << std::endl; // Expected: 3+4+5 = 12

    std::cout << "Updating element at index 3 (value 4) to 14..." << std::endl;
    sum_st.update(3, 14); // data becomes {1,2,3,14,5,6,7,8,9,10}

    std::cout << "New value at index 3: " << sum_st.query(3, 4) << std::endl; // Expected: 14
    std::cout << "New sum of all elements: " << sum_st.query(0, data.size()) << std::endl; // Expected: 55 - 4 + 14 = 65
    std::cout << "New sum of elements in range [2, 5): "
              << sum_st.query(2, 5) << std::endl; // Expected: 3+14+5 = 22

    std::cout << "\n--- Segment Tree Example: Range Minimum Query (RMQ) ---" << std::endl;
    std::vector<int> rmq_data = {50, 20, 80, 10, 90, 40, 60, 30};
    int min_identity = std::numeric_limits<int>::max();
    cpp_utils::SegmentTree<int, MinOp<int>> min_st(rmq_data, MinOp<int>(), min_identity);

    std::cout << "Initial data for RMQ: ";
    for (size_t i = 0; i < rmq_data.size(); ++i) {
        std::cout << min_st.query(i, i+1) << (i == rmq_data.size() - 1 ? "" : ", ");
    }
    std::cout << std::endl;

    std::cout << "Minimum in all elements (0-8): " << min_st.query(0, rmq_data.size()) << std::endl; // Expected: 10
    std::cout << "Minimum in range [1, 4) (i.e., index 1, 2, 3): "
              << min_st.query(1, 4) << std::endl; // Expected: min(20,80,10) = 10

    std::cout << "Updating element at index 3 (value 10) to 100..." << std::endl;
    min_st.update(3, 100); // data becomes {50,20,80,100,90,40,60,30}
    std::cout << "New minimum in all elements: " << min_st.query(0, rmq_data.size()) << std::endl; // Expected: 20
    std::cout << "New minimum in range [1, 4): "
              << min_st.query(1, 4) << std::endl; // Expected: min(20,80,100) = 20


    std::cout << "\n--- Segment Tree Example: Custom Data (Sum and Count for Average) ---" << std::endl;
    std::vector<CustomData> custom_vec;
    custom_vec.push_back(CustomData(10, 1)); // Value 10
    custom_vec.push_back(CustomData(20, 1)); // Value 20
    custom_vec.push_back(CustomData(5, 1));  // Value 5
    custom_vec.push_back(CustomData(15, 1)); // Value 15

    CustomData custom_identity = CustomData(0,0); // Identity for sum is 0, count is 0

    cpp_utils::SegmentTree<CustomData, decltype(&combine_custom_data)> custom_st(
        custom_vec,
        &combine_custom_data,
        custom_identity
    );

    std::cout << "Initial custom data elements (queried individually):" << std::endl;
    for(size_t i=0; i < custom_vec.size(); ++i) {
        std::cout << "  Index " << i << ": " << custom_st.query(i, i+1) << std::endl;
    }


    std::cout << "Combined data for range [0, " << custom_vec.size() << "): "
              << custom_st.query(0, custom_vec.size()) << std::endl;
    // Expected: sum = 10+20+5+15 = 50, count = 4

    std::cout << "Combined data for range [1, 3): "
              << custom_st.query(1, 3) << std::endl;
    // Expected: sum = 20+5 = 25, count = 2

    std::cout << "Updating element at index 0 from {10,1} to {30,1}" << std::endl;
    custom_st.update(0, CustomData(30,1));

    std::cout << "New combined data for range [0, " << custom_vec.size() << "): "
              << custom_st.query(0, custom_vec.size()) << std::endl;
    // Expected: sum = 30+20+5+15 = 70, count = 4

    // Example of querying an empty range
    std::cout << "Querying empty range [1,1): " << sum_st.query(1,1) << std::endl; // Expect 0 (identity for sum)
    std::cout << "Querying empty range [0,0) on min_st: " << min_st.query(0,0) << std::endl; // Expect int_max (identity for min)


    std::cout << "\n--- Segment Tree with default constructor (size and value) ---" << std::endl;
    size_t tree_size = 5;
    int default_val = 7;
    cpp_utils::SegmentTree<int> default_st(tree_size, default_val, std::plus<int>(), 0);
    std::cout << "Tree initialized with size " << tree_size << " and default value " << default_val << std::endl;
    std::cout << "Sum of all elements: " << default_st.query(0, tree_size) << std::endl; // Expected: 5 * 7 = 35
    default_st.update(2, 10); // {7,7,10,7,7}
    std::cout << "After updating index 2 to 10, sum: " << default_st.query(0, tree_size) << std::endl; // Expected: 7+7+10+7+7 = 38

    std::cout << "\n--- Segment Tree with empty initial data ---" << std::endl;
    std::vector<int> empty_initial_data;
    cpp_utils::SegmentTree<int> empty_st(empty_initial_data, std::plus<int>(), 0);
    std::cout << "Tree initialized with empty vector. Size: " << empty_st.size() << std::endl;
    std::cout << "Query on empty tree for range [0,0): " << empty_st.query(0,0) << std::endl; // Expected: 0 (identity)
    try {
        std::cout << "Query on empty tree for range [0,1) (should throw): ";
        empty_st.query(0,1);
    } catch (const std::out_of_range& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }


    return 0;
}

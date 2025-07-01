#include "../include/magnitude_map.h" // Adjust path as necessary
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm> // For std::sort, std::equal
#include <cmath>     // For std::fabs for double comparison robustness
#include <limits>    // For std::numeric_limits

// Helper to compare vectors of pairs, useful for results from find_within_magnitude
template <typename K, typename V>
bool compare_pair_vectors(const std::vector<std::pair<K, V>>& vec1, const std::vector<std::pair<K, V>>& vec2) {
    if (vec1.size() != vec2.size()) {
        return false;
    }
    // Assuming results from MagnitudeMap are sorted by key, so direct comparison is fine.
    // For floating point keys, direct comparison might be an issue if order is critical and values are extremely close.
    // However, std::map stores them sorted, so this should be okay.
    for (size_t i = 0; i < vec1.size(); ++i) {
        if (vec1[i].first != vec2[i].first) { // Exact comparison for keys
            return false;
        }
        if constexpr (std::is_floating_point_v<V>) {
            if (std::fabs(vec1[i].second - vec2[i].second) > std::numeric_limits<V>::epsilon()) {
                return false;
            }
        } else {
            if (vec1[i].second != vec2[i].second) {
                return false;
            }
        }
    }
    return true;
}


void test_constructor_and_basic_state() {
    std::cout << "--- Test: Constructor and Basic State ---" << std::endl;
    cpp_utils::MagnitudeMap<int, std::string> map_int_str;
    assert(map_int_str.empty());
    assert(map_int_str.size() == 0);

    cpp_utils::MagnitudeMap<double, int> map_double_int;
    assert(map_double_int.empty());
    assert(map_double_int.size() == 0);
    std::cout << "Constructor and Basic State Test Passed!" << std::endl;
}

void test_insert_and_get() {
    std::cout << "--- Test: Insert and Get ---" << std::endl;
    cpp_utils::MagnitudeMap<int, std::string> map;

    map.insert(10, "apple");
    assert(!map.empty());
    assert(map.size() == 1);
    assert(map.contains(10));
    assert(!map.contains(20));
    assert(map.get(10) != nullptr && *map.get(10) == "apple");
    assert(map.get(20) == nullptr);

    map.insert(20, "banana");
    assert(map.size() == 2);
    assert(map.contains(20));
    assert(map.get(20) != nullptr && *map.get(20) == "banana");

    // Test update
    map.insert(10, "apricot");
    assert(map.size() == 2); // Size should not change
    assert(map.get(10) != nullptr && *map.get(10) == "apricot");

    // Const get
    const auto& const_map = map;
    assert(const_map.get(10) != nullptr && *const_map.get(10) == "apricot");
    assert(const_map.get(30) == nullptr);
    assert(const_map.contains(20));

    std::cout << "Insert and Get Test Passed!" << std::endl;
}

void test_remove() {
    std::cout << "--- Test: Remove ---" << std::endl;
    cpp_utils::MagnitudeMap<int, std::string> map;
    map.insert(10, "one");
    map.insert(20, "two");
    map.insert(30, "three");

    assert(map.size() == 3);

    assert(map.remove(20)); // Remove existing
    assert(map.size() == 2);
    assert(!map.contains(20));
    assert(map.get(20) == nullptr);

    assert(!map.remove(50)); // Remove non-existing
    assert(map.size() == 2);

    assert(map.remove(10));
    assert(map.size() == 1);
    assert(!map.contains(10));

    assert(map.remove(30));
    assert(map.size() == 0);
    assert(map.empty());

    assert(!map.remove(10)); // Remove from empty map
    assert(map.empty());

    std::cout << "Remove Test Passed!" << std::endl;
}

void test_find_within_magnitude_int_keys() {
    std::cout << "--- Test: Find Within Magnitude (Int Keys) ---" << std::endl;
    cpp_utils::MagnitudeMap<int, std::string> map;
    map.insert(10, "A");
    map.insert(20, "B");
    map.insert(25, "C");
    map.insert(30, "D");
    map.insert(40, "E");
    map.insert(50, "F");

    std::vector<std::pair<int, std::string>> expected;
    std::vector<std::pair<int, std::string>> result;

    // Empty map
    cpp_utils::MagnitudeMap<int, std::string> empty_map;
    result = empty_map.find_within_magnitude(10, 5);
    assert(result.empty());

    // Magnitude zero (exact match)
    result = map.find_within_magnitude(25, 0);
    expected = {{25, "C"}};
    assert(compare_pair_vectors(result, expected));

    result = map.find_within_magnitude(22, 0); // No exact match
    expected = {};
    assert(compare_pair_vectors(result, expected));

    // Negative magnitude (treated as zero)
    result = map.find_within_magnitude(25, -5);
    expected = {{25, "C"}};
    assert(compare_pair_vectors(result, expected));

    // No elements within magnitude
    result = map.find_within_magnitude(100, 5);
    expected = {};
    assert(compare_pair_vectors(result, expected));

    result = map.find_within_magnitude(0, 5); // Query key far left
    expected = {};
    assert(compare_pair_vectors(result, expected));

    // All elements within magnitude
    result = map.find_within_magnitude(30, 100);
    expected = {{10, "A"}, {20, "B"}, {25, "C"}, {30, "D"}, {40, "E"}, {50, "F"}};
    assert(compare_pair_vectors(result, expected));

    // Some elements within magnitude
    result = map.find_within_magnitude(22, 3); // Range [19, 25] -> expecting {20, "B"}, {25, "C"}
    expected = {{20, "B"}, {25, "C"}};
    assert(compare_pair_vectors(result, expected));

    result = map.find_within_magnitude(45, 5); // Range [40, 50] -> expecting {40, "E"}, {50, "F"}
    expected = {{40, "E"}, {50, "F"}};
    assert(compare_pair_vectors(result, expected));

    result = map.find_within_magnitude(10, 2); // Range [8, 12] -> expecting {10, "A"}
    expected = {{10, "A"}};
    assert(compare_pair_vectors(result, expected));

    // Query key at the start
    result = map.find_within_magnitude(10, 10); // Range [0, 20] -> expecting {10, "A"}, {20, "B"}
    expected = {{10, "A"}, {20, "B"}};
    assert(compare_pair_vectors(result, expected));

    // Query key at the end
    result = map.find_within_magnitude(50, 10); // Range [40, 60] -> expecting {40, "E"}, {50, "F"}
    expected = {{40, "E"}, {50, "F"}};
    assert(compare_pair_vectors(result, expected));

    // Test with numeric limits for signed int
    cpp_utils::MagnitudeMap<int, int> map_limits;
    map_limits.insert(std::numeric_limits<int>::min(), 1);
    map_limits.insert(std::numeric_limits<int>::min() + 10, 2);
    map_limits.insert(0, 3);
    map_limits.insert(std::numeric_limits<int>::max() - 10, 4);
    map_limits.insert(std::numeric_limits<int>::max(), 5);

    result = map_limits.find_within_magnitude(std::numeric_limits<int>::min() + 5, 6);
    // Range: [min, min+11] -> expecting {min,1}, {min+10,2}
    expected = {{std::numeric_limits<int>::min(), 1}, {std::numeric_limits<int>::min() + 10, 2}};
    assert(compare_pair_vectors(result, expected));

    result = map_limits.find_within_magnitude(std::numeric_limits<int>::max() - 5, 6);
    // Range: [max-11, max] -> expecting {max-10,4}, {max,5}
    expected = {{std::numeric_limits<int>::max() - 10, 4}, {std::numeric_limits<int>::max(), 5}};
    assert(compare_pair_vectors(result, expected));

    result = map_limits.find_within_magnitude(0, std::numeric_limits<int>::max()); // Huge magnitude
    expected = {
        {std::numeric_limits<int>::min(), 1},
        {std::numeric_limits<int>::min() + 10, 2},
        {0, 3},
        {std::numeric_limits<int>::max() - 10, 4},
        {std::numeric_limits<int>::max(), 5}
    };
    assert(compare_pair_vectors(result, expected));


    std::cout << "Find Within Magnitude (Int Keys) Test Passed!" << std::endl;
}


void test_find_within_magnitude_double_keys() {
    std::cout << "--- Test: Find Within Magnitude (Double Keys) ---" << std::endl;
    cpp_utils::MagnitudeMap<double, int> map;
    map.insert(10.5, 100);
    map.insert(12.3, 200);
    map.insert(12.8, 300);
    map.insert(15.0, 400);
    map.insert(15.2, 500);

    std::vector<std::pair<double, int>> expected_double;
    std::vector<std::pair<double, int>> result_double;

    // Magnitude zero (exact match)
    result_double = map.find_within_magnitude(12.3, 0.0);
    expected_double = {{12.3, 200}};
    assert(compare_pair_vectors(result_double, expected_double));

    result_double = map.find_within_magnitude(12.3, -1.0); // Negative magnitude
    expected_double = {{12.3, 200}};
    assert(compare_pair_vectors(result_double, expected_double));

    // Some elements
    result_double = map.find_within_magnitude(12.5, 0.3); // Range [12.2, 12.8] -> expect {12.3,200}, {12.8,300}
    expected_double = {{12.3, 200}, {12.8, 300}};
    assert(compare_pair_vectors(result_double, expected_double));

    result_double = map.find_within_magnitude(15.1, 0.1); // Range [15.0, 15.2] -> expect {15.0,400}, {15.2,500}
    expected_double = {{15.0, 400}, {15.2, 500}};
    assert(compare_pair_vectors(result_double, expected_double));

    // No elements
    result_double = map.find_within_magnitude(5.0, 1.0);
    expected_double = {};
    assert(compare_pair_vectors(result_double, expected_double));

    // All elements
    result_double = map.find_within_magnitude(13.0, 10.0);
    expected_double = {{10.5, 100}, {12.3, 200}, {12.8, 300}, {15.0, 400}, {15.2, 500}};
    assert(compare_pair_vectors(result_double, expected_double));

    std::cout << "Find Within Magnitude (Double Keys) Test Passed!" << std::endl;
}

void test_find_within_magnitude_unsigned_keys() {
    std::cout << "--- Test: Find Within Magnitude (Unsigned Keys) ---" << std::endl;
    cpp_utils::MagnitudeMap<unsigned int, std::string> map;
    map.insert(10u, "TEN");
    map.insert(20u, "TWENTY");
    map.insert(0u, "ZERO"); // Test edge case with 0
    map.insert(std::numeric_limits<unsigned int>::max() - 5u, "MAX_MINUS_5");
    map.insert(std::numeric_limits<unsigned int>::max(), "MAX");


    std::vector<std::pair<unsigned int, std::string>> expected;
    std::vector<std::pair<unsigned int, std::string>> result;

    // Query near 0
    result = map.find_within_magnitude(2u, 3u); // Range [0, 5] -> expect {0, "ZERO"}
    expected = {{0u, "ZERO"}};
    assert(compare_pair_vectors(result, expected));

    // Query near max
    result = map.find_within_magnitude(std::numeric_limits<unsigned int>::max() - 2u, 3u);
    // Range [max-5, max+1 (effectively max)] -> expect {max-5, "MAX_MINUS_5"}, {max, "MAX"}
    expected = {{std::numeric_limits<unsigned int>::max() - 5u, "MAX_MINUS_5"}, {std::numeric_limits<unsigned int>::max(), "MAX"}};
    assert(compare_pair_vectors(result, expected));

    // Query with magnitude larger than key, for lower bound calculation
    result = map.find_within_magnitude(5u, 10u); // Range [0, 15] -> expect {0,"ZERO"}, {10,"TEN"}
    expected = {{0u, "ZERO"}, {10u, "TEN"}};
    assert(compare_pair_vectors(result, expected));

    std::cout << "Find Within Magnitude (Unsigned Keys) Test Passed!" << std::endl;
}


int main() {
    test_constructor_and_basic_state();
    std::cout << std::endl;
    test_insert_and_get();
    std::cout << std::endl;
    test_remove();
    std::cout << std::endl;
    test_find_within_magnitude_int_keys();
    std::cout << std::endl;
    test_find_within_magnitude_double_keys();
    std::cout << std::endl;
    test_find_within_magnitude_unsigned_keys();
    std::cout << std::endl;

    std::cout << "All MagnitudeMap tests completed." << std::endl;
    return 0;
}

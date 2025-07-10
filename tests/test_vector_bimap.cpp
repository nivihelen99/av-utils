#include "vector_bimap.h" // Adjust path as necessary
#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <algorithm> // For std::is_sorted
#include <stdexcept> // For std::out_of_range

// Simple test runner state
int tests_run = 0;
int tests_failed = 0;

#define TEST_ASSERT(condition) \
    do { \
        tests_run++; \
        if (!(condition)) { \
            std::cerr << "Assertion failed at " << __FILE__ << ":" << __LINE__ << ": " #condition << std::endl; \
            tests_failed++; \
        } \
    } while (0)

#define RUN_TEST(test_function) \
    do { \
        std::cout << "Running " #test_function "..." << std::endl; \
        test_function(); \
        std::cout << #test_function " completed." << std::endl; \
    } while (0)

// Helper to check if left view is sorted
template<typename L, typename R, typename CL, typename CR>
bool is_left_view_sorted(const cpp_collections::VectorBiMap<L, R, CL, CR>& vm) {
    if (vm.left_cbegin() == vm.left_cend()) return true;
    return std::is_sorted(vm.left_cbegin(), vm.left_cend(),
        [&](const typename cpp_collections::VectorBiMap<L, R, CL, CR>::left_value_type& a,
            const typename cpp_collections::VectorBiMap<L, R, CL, CR>::left_value_type& b) {
        return vm.left_key_comp()(a.first, b.first);
    });
}

// Helper to check if right view is sorted
template<typename L, typename R, typename CL, typename CR>
bool is_right_view_sorted(const cpp_collections::VectorBiMap<L, R, CL, CR>& vm) {
    if (vm.right_cbegin() == vm.right_cend()) return true;
    return std::is_sorted(vm.right_cbegin(), vm.right_cend(),
        [&](const typename cpp_collections::VectorBiMap<L, R, CL, CR>::right_value_type& a,
            const typename cpp_collections::VectorBiMap<L, R, CL, CR>::right_value_type& b) {
        return vm.right_key_comp()(a.first, b.first);
    });
}


void test_default_constructor_and_empty() {
    cpp_collections::VectorBiMap<int, std::string> vm;
    TEST_ASSERT(vm.empty());
    TEST_ASSERT(vm.size() == 0);
    TEST_ASSERT(is_left_view_sorted(vm));
    TEST_ASSERT(is_right_view_sorted(vm));
}

void test_initializer_list_constructor() {
    cpp_collections::VectorBiMap<int, std::string> vm = {
        {1, "one"}, {3, "three"}, {2, "two"}
    };
    TEST_ASSERT(!vm.empty());
    TEST_ASSERT(vm.size() == 3);
    TEST_ASSERT(vm.contains_left(1));
    TEST_ASSERT(vm.contains_right("two"));
    TEST_ASSERT(vm.at_left(3) == "three");
    TEST_ASSERT(is_left_view_sorted(vm));
    TEST_ASSERT(is_right_view_sorted(vm));

    // Test with empty list
    cpp_collections::VectorBiMap<int, std::string> vm_empty_init = {};
    TEST_ASSERT(vm_empty_init.empty());
    TEST_ASSERT(vm_empty_init.size() == 0);
}

void test_insert_basic() {
    cpp_collections::VectorBiMap<int, std::string> vm;
    TEST_ASSERT(vm.insert(1, "apple"));
    TEST_ASSERT(vm.size() == 1);
    TEST_ASSERT(vm.contains_left(1));
    TEST_ASSERT(vm.contains_right("apple"));
    TEST_ASSERT(vm.at_left(1) == "apple");
    TEST_ASSERT(vm.at_right("apple") == 1);

    TEST_ASSERT(vm.insert(2, "banana"));
    TEST_ASSERT(vm.size() == 2);
    TEST_ASSERT(vm.at_left(2) == "banana");

    // Insert duplicate left key
    TEST_ASSERT(!vm.insert(1, "apricot"));
    TEST_ASSERT(vm.size() == 2);
    TEST_ASSERT(vm.at_left(1) == "apple"); // Should not change

    // Insert duplicate right key
    TEST_ASSERT(!vm.insert(3, "apple"));
    TEST_ASSERT(vm.size() == 2);
    TEST_ASSERT(vm.at_right("apple") == 1); // Should not change

    TEST_ASSERT(is_left_view_sorted(vm));
    TEST_ASSERT(is_right_view_sorted(vm));
}

void test_insert_rvalues() {
    cpp_collections::VectorBiMap<std::string, int> vm;
    TEST_ASSERT(vm.insert(std::string("movable_key"), 20));
    TEST_ASSERT(vm.size() == 1);
    TEST_ASSERT(vm.contains_left("movable_key"));
    TEST_ASSERT(vm.at_left("movable_key") == 20);
    TEST_ASSERT(vm.contains_right(20));
    TEST_ASSERT(vm.at_right(20) == "movable_key");
    TEST_ASSERT(is_left_view_sorted(vm));
    TEST_ASSERT(is_right_view_sorted(vm));
}


void test_insert_or_assign() {
    cpp_collections::VectorBiMap<int, std::string> vm;
    vm.insert_or_assign(1, "one");
    TEST_ASSERT(vm.at_left(1) == "one");
    TEST_ASSERT(vm.at_right("one") == 1);
    TEST_ASSERT(vm.size() == 1);

    // Assign new right to existing left
    vm.insert_or_assign(1, "uno");
    TEST_ASSERT(vm.at_left(1) == "uno");
    TEST_ASSERT(vm.contains_right("uno"));
    TEST_ASSERT(!vm.contains_right("one")); // "one" should be gone
    TEST_ASSERT(vm.size() == 1);

    // Assign new left to existing right
    vm.insert(2, "two"); // 1=>uno, 2=>two
    vm.insert_or_assign(10, "uno"); // 10=>uno, 1 should be gone
    TEST_ASSERT(vm.at_right("uno") == 10);
    TEST_ASSERT(vm.contains_left(10));
    TEST_ASSERT(!vm.contains_left(1)); // 1 should be gone
    TEST_ASSERT(vm.size() == 2); // 10=>uno, 2=>two
    TEST_ASSERT(is_left_view_sorted(vm)); // Check sorted after complex insert_or_assign
    TEST_ASSERT(is_right_view_sorted(vm));


    // Conflict: assign (L_new, R_new) where L_new exists and R_new exists,
    // but L_new maps to R_old and R_new maps to L_old.
    // Current: (2, "two"), (10, "uno") -- assuming sorted this way after previous ops
    vm.insert_or_assign(10, "two"); // Should become (10, "two"). Old (2,"two") removed. Old (10,"uno") removed.
    TEST_ASSERT(vm.size() == 1);
    TEST_ASSERT(vm.at_left(10) == "two");
    TEST_ASSERT(vm.at_right("two") == 10);
    TEST_ASSERT(!vm.contains_left(2));
    TEST_ASSERT(!vm.contains_right("uno"));

    TEST_ASSERT(is_left_view_sorted(vm));
    TEST_ASSERT(is_right_view_sorted(vm));
}

void test_find_at_contains() {
    cpp_collections::VectorBiMap<int, std::string> vm = {{1, "a"}, {2, "b"}, {3, "c"}};

    // find_left
    TEST_ASSERT(vm.find_left(1) != nullptr && *vm.find_left(1) == "a");
    TEST_ASSERT(vm.find_left(4) == nullptr);
    const auto& cvm = vm;
    TEST_ASSERT(cvm.find_left(1) != nullptr && *cvm.find_left(1) == "a");

    // find_right
    TEST_ASSERT(vm.find_right("b") != nullptr && *vm.find_right("b") == 2);
    TEST_ASSERT(vm.find_right("d") == nullptr);
    TEST_ASSERT(cvm.find_right("b") != nullptr && *cvm.find_right("b") == 2);

    // at_left
    TEST_ASSERT(vm.at_left(1) == "a");
    bool caught_left = false;
    try { vm.at_left(4); } catch (const std::out_of_range&) { caught_left = true; }
    TEST_ASSERT(caught_left);

    // at_right
    TEST_ASSERT(vm.at_right("c") == 3);
    bool caught_right = false;
    try { vm.at_right("d"); } catch (const std::out_of_range&) { caught_right = true; }
    TEST_ASSERT(caught_right);

    // contains_left
    TEST_ASSERT(vm.contains_left(1));
    TEST_ASSERT(!vm.contains_left(4));

    // contains_right
    TEST_ASSERT(vm.contains_right("a"));
    TEST_ASSERT(!vm.contains_right("d"));
}

void test_erase() {
    cpp_collections::VectorBiMap<int, std::string> vm;
    vm.insert(1, "x");
    vm.insert(2, "y");
    vm.insert(3, "z");
    TEST_ASSERT(vm.size() == 3);

    // Erase_left
    TEST_ASSERT(vm.erase_left(2)); // Erase (2, "y")
    TEST_ASSERT(vm.size() == 2);
    TEST_ASSERT(!vm.contains_left(2));
    TEST_ASSERT(!vm.contains_right("y"));
    TEST_ASSERT(vm.contains_left(1));
    TEST_ASSERT(vm.contains_right("z"));
    TEST_ASSERT(!vm.erase_left(100)); // Key not found
    TEST_ASSERT(vm.size() == 2);
    TEST_ASSERT(is_left_view_sorted(vm));
    TEST_ASSERT(is_right_view_sorted(vm));


    // Erase_right
    TEST_ASSERT(vm.erase_right("x")); // Erase (1, "x")
    TEST_ASSERT(vm.size() == 1);
    TEST_ASSERT(!vm.contains_right("x"));
    TEST_ASSERT(!vm.contains_left(1));
    TEST_ASSERT(vm.at_left(3) == "z"); // (3, "z") should remain
    TEST_ASSERT(!vm.erase_right("nonexistent"));
    TEST_ASSERT(vm.size() == 1);

    TEST_ASSERT(is_left_view_sorted(vm));
    TEST_ASSERT(is_right_view_sorted(vm));
}

void test_clear_swap() {
    cpp_collections::VectorBiMap<int, std::string> vm1 = {{1, "a"}, {2, "b"}};
    cpp_collections::VectorBiMap<int, std::string> vm2 = {{3, "c"}, {4, "d"}, {5, "e"}};

    vm1.swap(vm2);
    TEST_ASSERT(vm1.size() == 3 && vm1.contains_left(3));
    TEST_ASSERT(vm2.size() == 2 && vm2.contains_left(1));
    TEST_ASSERT(is_left_view_sorted(vm1) && is_right_view_sorted(vm1));
    TEST_ASSERT(is_left_view_sorted(vm2) && is_right_view_sorted(vm2));


    cpp_collections::swap(vm1, vm2); // Non-member swap
    TEST_ASSERT(vm1.size() == 2 && vm1.contains_left(1));
    TEST_ASSERT(vm2.size() == 3 && vm2.contains_left(3));

    vm1.clear();
    TEST_ASSERT(vm1.empty());
    TEST_ASSERT(vm1.size() == 0);
    TEST_ASSERT(!vm1.contains_left(1));
    TEST_ASSERT(vm2.size() == 3); // vm2 should be unaffected
}

void test_iteration_and_sorted_order() {
    cpp_collections::VectorBiMap<int, std::string> vm;
    vm.insert(3, "zebra");
    vm.insert(1, "apple");
    vm.insert(2, "banana");
    vm.insert(0, "date");

    // Left view iteration (sorted by int key: 0, 1, 2, 3)
    std::vector<int> left_keys;
    for (const auto& pair : vm) { // Default iteration is left view
        left_keys.push_back(pair.first);
    }
    std::vector<int> expected_left_keys = {0, 1, 2, 3};
    TEST_ASSERT(left_keys == expected_left_keys);
    TEST_ASSERT(is_left_view_sorted(vm));

    // Right view iteration (sorted by string value: apple, banana, date, zebra)
    std::vector<std::string> right_keys_str_val;
    for (auto it = vm.right_cbegin(); it != vm.right_cend(); ++it) {
        right_keys_str_val.push_back(it->first);
    }
    std::vector<std::string> expected_right_keys_str_val = {"apple", "banana", "date", "zebra"};
    TEST_ASSERT(right_keys_str_val == expected_right_keys_str_val);
    TEST_ASSERT(is_right_view_sorted(vm));
}

void test_custom_comparators() {
    struct ReverseIntLess {
        bool operator()(int a, int b) const { return a > b; } // Sorts in descending order
    };
    struct StringLengthLess {
        bool operator()(const std::string& a, const std::string& b) const {
            if (a.length() != b.length()) {
                return a.length() < b.length();
            }
            return a < b; // Tie-break alphabetically
        }
    };

    cpp_collections::VectorBiMap<int, std::string, ReverseIntLess, StringLengthLess> vm;
    vm.insert(10, "short");
    vm.insert(30, "a");      // Shortest string
    vm.insert(20, "medium_len");

    // Check left view sorted by ReverseIntLess (30, 20, 10)
    std::vector<int> left_keys_actual;
    for (const auto& p : vm) { // Default iteration is left view
        left_keys_actual.push_back(p.first);
    }
    std::vector<int> expected_left = {30, 20, 10};
    TEST_ASSERT(left_keys_actual == expected_left);
    TEST_ASSERT(is_left_view_sorted(vm));


    // Check right view sorted by StringLengthLess ("a", "short", "medium_len")
    std::vector<std::string> right_keys_str_actual;
     for (auto it = vm.right_cbegin(); it != vm.right_cend(); ++it) {
        right_keys_str_actual.push_back(it->first);
    }
    std::vector<std::string> expected_right_str = {"a", "short", "medium_len"};
    TEST_ASSERT(right_keys_str_actual == expected_right_str);
    TEST_ASSERT(is_right_view_sorted(vm));

    TEST_ASSERT(vm.at_left(30) == "a");
    TEST_ASSERT(vm.at_right("short") == 10);
}


int main() {
    std::cout << "Starting VectorBiMap tests...\n";

    RUN_TEST(test_default_constructor_and_empty);
    RUN_TEST(test_initializer_list_constructor);
    RUN_TEST(test_insert_basic);
    RUN_TEST(test_insert_rvalues);
    RUN_TEST(test_insert_or_assign);
    RUN_TEST(test_find_at_contains);
    RUN_TEST(test_erase);
    RUN_TEST(test_clear_swap);
    RUN_TEST(test_iteration_and_sorted_order);
    RUN_TEST(test_custom_comparators);

    std::cout << "\nAll tests finished.\n";
    std::cout << "Tests run: " << tests_run << std::endl;
    std::cout << "Tests failed: " << tests_failed << std::endl;
    std::cout << "----------------------------------------\n";

    return (tests_failed == 0) ? 0 : 1;
}

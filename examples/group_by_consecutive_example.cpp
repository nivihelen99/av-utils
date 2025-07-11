#include "group_by_consecutive.h"
#include <iostream>
#include <vector>
#include <string>
#include <iomanip> // For std::quoted
#include <cmath>   // For std::abs
#include <type_traits> // For std::is_same_v

// Helper function to print groups
template <typename Key, typename Value>
void print_groups(const std::string& title,
                  const std::vector<cpp_collections::Group<Key, Value>>& groups) {
    std::cout << title << ":" << std::endl;
    if (groups.empty()) {
        std::cout << "  (empty)" << std::endl;
        return;
    }
    for (const auto& group : groups) {
        std::cout << "  Key: ";
        if constexpr (std::is_same_v<Key, std::string>) {
            std::cout << std::quoted(group.key);
        } else if constexpr (std::is_same_v<Key, char>) {
            std::cout << "'" << group.key << "'";
        } else {
            std::cout << group.key;
        }
        std::cout << ", Items: [";
        bool first_item = true;
        for (const auto& item : group.items) {
            if (!first_item) {
                std::cout << ", ";
            }
            if constexpr (std::is_same_v<Value, std::string>) {
                std::cout << std::quoted(item);
            } else {
                std::cout << item; // Assumes Value has an operator<<
            }
            first_item = false;
        }
        std::cout << "]" << std::endl;
    }
    std::cout << std::endl;
}

// Example struct for custom objects
struct MyObject {
    int id;
    std::string category;
    double value;

    friend std::ostream& operator<<(std::ostream& os, const MyObject& obj) {
        os << "{id:" << obj.id << ", cat:" << std::quoted(obj.category) << ", val:" << obj.value << "}";
        return os;
    }

    bool operator==(const MyObject& other) const {
        return id == other.id && category == other.category && value == other.value;
    }
};

// Key extractor for MyObject category
struct MyObjectCategoryExtractor {
    const std::string& operator()(const MyObject& obj) const {
        return obj.category;
    }
};

// Predicate to group MyObjects if their values are "close enough"
struct MyObjectValuePredicate {
    bool operator()(const MyObject& obj1, const MyObject& obj2) const {
        // Group if obj2's value is within 0.5 of obj1's value
        return std::abs(obj1.value - obj2.value) < 0.5;
    }
};

int main() {
    // --- Example 1: Simple integers, grouping by value itself (key function) ---
    std::vector<int> numbers1 = {1, 1, 1, 2, 2, 1, 1, 3, 3, 3, 3, 2};
    auto groups1 = cpp_collections::group_by_consecutive(
        numbers1.begin(), numbers1.end(),
        [](int x){ return x; }
    );
    print_groups("Integers grouped by value (key function)", groups1);

    // --- Example 2: Empty range (key function) ---
    std::vector<int> numbers_empty = {};
    auto groups_empty = cpp_collections::group_by_consecutive(
        numbers_empty.begin(), numbers_empty.end(),
        [](int x){ return x; }
    );
    print_groups("Empty range (key function)", groups_empty);

    // --- Example 3: All unique elements (key function) ---
    std::vector<int> numbers_unique = {1, 2, 3, 4, 5};
    auto groups_unique = cpp_collections::group_by_consecutive(
        numbers_unique.begin(), numbers_unique.end(),
        [](int x){ return x; }
    );
    print_groups("All unique integers (key function)", groups_unique);

    // --- Example 4: All same elements (key function) ---
    std::vector<int> numbers_same = {7, 7, 7, 7};
    auto groups_same = cpp_collections::group_by_consecutive(
        numbers_same.begin(), numbers_same.end(),
        [](int x){ return x; }
    );
    print_groups("All same integers (key function)", groups_same);

    // --- Example 5: Strings, grouping by first character (key function) ---
    std::vector<std::string> words = {"apple", "apricot", "banana", "blueberry", "cherry", "date", "dewberry"};
    auto groups_words = cpp_collections::group_by_consecutive(
        words.begin(), words.end(),
        [](const std::string& s){ return s.empty() ? ' ' : s[0]; }
    );
    print_groups("Strings grouped by first character (key function)", groups_words);

    // --- Example 6: Custom objects (MyObject), grouping by category (key function) ---
    std::vector<MyObject> objects = {
        {1, "A", 10.1}, {2, "A", 12.3}, {3, "B", 20.5},
        {4, "B", 22.1}, {5, "A", 30.0}, {6, "C", 40.7}
    };
    auto groups_objects_cat = cpp_collections::group_by_consecutive(
        objects.begin(), objects.end(),
        MyObjectCategoryExtractor()
    );
    print_groups("MyObjects grouped by category (key function)", groups_objects_cat);

    // --- Example 7: Using the predicate version with integers (group if difference is <= 1) ---
    std::vector<int> numbers2_pred = {1, 2, 3, 5, 6, 8, 9, 10, 12};
    auto groups2_pred = cpp_collections::group_by_consecutive_pred(
        numbers2_pred.begin(), numbers2_pred.end(),
        [](int prev, int curr){ return std::abs(curr - prev) <= 1; }
    );
    print_groups("Integers grouped by predicate (diff <= 1)", groups2_pred);

    // --- Example 8: Custom objects (MyObject), grouping by predicate (value proximity) ---
    // Note: For predicate, the key is the first element of the group.
    // The predicate (obj1, obj2) should check if obj2 belongs with obj1 (which started the current group or was the previous element).
    std::vector<MyObject> objects_val_pred = {
        {1, "V", 10.1}, {2, "V", 10.3}, // Group with {1, "V", 10.1} (key) because 10.3 is close to 10.1
        {3, "V", 10.8},                // New group with {3, "V", 10.8} (key) because 10.8 not close to 10.3
        {4, "W", 20.0}, {5, "W", 20.4},// Group with {4, "W", 20.0} (key)
        {6, "X", 20.7},                // New group {6, "X", 20.7} (key), 20.7 not close to 20.4
        {7, "Y", 30.0}                 // New group {7, "Y", 30.0} (key)
    };
    auto groups_objects_val_pred = cpp_collections::group_by_consecutive_pred(
        objects_val_pred.begin(), objects_val_pred.end(),
        MyObjectValuePredicate() // Are obj2 and obj1 in same group? (based on their values)
    );
    print_groups("MyObjects grouped by value proximity (predicate)", groups_objects_val_pred);

    // --- Example 9: Strings, predicate: group if same length ---
    std::vector<std::string> words_len_pred = {"a", "b", "cat", "dog", "Sun", "moon", "stars", "x", "y"};
    auto groups_words_len_pred = cpp_collections::group_by_consecutive_pred(
        words_len_pred.begin(), words_len_pred.end(),
        [](const std::string& s1, const std::string& s2){ return s1.length() == s2.length(); }
    );
    print_groups("Strings grouped by same length (predicate)", groups_words_len_pred);

    // --- Example 10: Predicate version with empty list ---
    std::vector<MyObject> objects_empty_pred = {};
    auto groups_empty_pred = cpp_collections::group_by_consecutive_pred(
        objects_empty_pred.begin(), objects_empty_pred.end(),
        MyObjectValuePredicate()
    );
    print_groups("Empty range (predicate)", groups_empty_pred);

    // --- Example 11: Predicate version, all unique (based on predicate) ---
    std::vector<int> numbers_unique_pred_data = {1, 3, 5, 7, 9}; // all differ by more than 1
    auto groups_unique_pred = cpp_collections::group_by_consecutive_pred(
        numbers_unique_pred_data.begin(), numbers_unique_pred_data.end(),
         [](int prev, int curr){ return std::abs(curr - prev) <= 1; }
    );
    print_groups("All unique (by predicate diff > 1)", groups_unique_pred);

    // --- Example 12: Predicate version, all same (based on predicate) ---
    std::vector<int> numbers_same_pred_data = {2, 2, 2, 2, 2}; // all differ by 0
     auto groups_same_pred = cpp_collections::group_by_consecutive_pred(
        numbers_same_pred_data.begin(), numbers_same_pred_data.end(),
         [](int prev, int curr){ return std::abs(curr - prev) <= 1; }
    );
    print_groups("All same (by predicate diff <=1)", groups_same_pred);

    return 0;
}

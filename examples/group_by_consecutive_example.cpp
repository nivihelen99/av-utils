#include "group_by_consecutive.hpp" // Assuming it's in the include path
#include <vector>
#include <string>
#include <iostream>
#include <iomanip> // For std::quoted

// Example 1: Basic usage with pairs, similar to the requirement spec
void example_basic() {
    std::cout << "--- Example 1: Basic Usage ---" << std::endl;
    std::vector<std::pair<char, int>> data = {
        {'a', 1}, {'a', 2}, {'b', 3}, {'b', 4}, {'a', 5}
    };

    auto groups = utils::group_by_consecutive(data, [](const auto& p) {
        return p.first;
    });

    for (const auto& group_pair : groups) {
        std::cout << "Key: " << group_pair.first << ", Values: [ ";
        for (const auto& item : group_pair.second) {
            std::cout << "{'" << item.first << "', " << item.second << "} ";
        }
        std::cout << "]" << std::endl;
    }
    std::cout << std::endl;
}

// Example 2: Grouping integers by their value
void example_integers() {
    std::cout << "--- Example 2: Grouping Integers ---" << std::endl;
    std::vector<int> numbers = {1, 1, 1, 2, 2, 3, 1, 1, 4, 4, 4, 4};

    auto groups = utils::group_by_consecutive(numbers.begin(), numbers.end(), [](int val) {
        return val; // Key is the number itself
    });

    for (const auto& group_pair : groups) {
        std::cout << "Key: " << group_pair.first << ", Values: [ ";
        for (int item : group_pair.second) {
            std::cout << item << " ";
        }
        std::cout << "]" << std::endl;
    }
    std::cout << std::endl;
}

// Example 3: Grouping strings by their first character
void example_strings_first_char() {
    std::cout << "--- Example 3: Grouping Strings by First Character ---" << std::endl;
    std::vector<std::string> words = {"apple", "apricot", "banana", "blueberry", "cherry", "fig", "grape"};

    auto groups = utils::group_by_consecutive(words, [](const std::string& s) {
        return s.empty() ? ' ' : s[0]; // Key is the first character
    });

    for (const auto& group_pair : groups) {
        std::cout << "Key: '" << group_pair.first << "', Values: [ ";
        for (const auto& item : group_pair.second) {
            std::cout << std::quoted(item) << " ";
        }
        std::cout << "]" << std::endl;
    }
    std::cout << std::endl;
}

// Example 4: Empty input
void example_empty() {
    std::cout << "--- Example 4: Empty Input ---" << std::endl;
    std::vector<std::pair<char, int>> data = {};

    auto groups = utils::group_by_consecutive(data, [](const auto& p) {
        return p.first;
    });

    if (groups.empty()) {
        std::cout << "Resulting groups vector is empty, as expected." << std::endl;
    } else {
        std::cout << "Error: Expected empty groups vector for empty input." << std::endl;
    }
    std::cout << std::endl;
}

// Example 5: All items with the same key
void example_same_key() {
    std::cout << "--- Example 5: All Items Same Key ---" << std::endl;
    std::vector<std::pair<char, int>> data = {
        {'x', 10}, {'x', 20}, {'x', 30}
    };

    auto groups = utils::group_by_consecutive(data, [](const auto& p) {
        return p.first;
    });

    for (const auto& group_pair : groups) {
        std::cout << "Key: " << group_pair.first << ", Values: [ ";
        for (const auto& item : group_pair.second) {
            std::cout << "{'" << item.first << "', " << item.second << "} ";
        }
        std::cout << "]" << std::endl;
    }
    std::cout << std::endl;
}

// Example 6: Custom struct and key function
struct MyStruct {
    int id;
    std::string category;
    double value;

    // For printing
    friend std::ostream& operator<<(std::ostream& os, const MyStruct& s) {
        os << "{id:" << s.id << ", cat:" << std::quoted(s.category) << ", val:" << s.value << "}";
        return os;
    }
};

auto get_category(const MyStruct& s) -> const std::string& {
    return s.category;
}

void example_custom_struct() {
    std::cout << "--- Example 6: Custom Struct and Key Function ---" << std::endl;
    std::vector<MyStruct> items = {
        {1, "A", 10.1}, {2, "A", 12.5}, {3, "B", 20.0},
        {4, "A", 15.3}, {5, "A", 18.7}, {6, "B", 22.1}
    };

    auto groups = utils::group_by_consecutive(items.begin(), items.end(), get_category);

    for (const auto& group_pair : groups) {
        std::cout << "Key: " << std::quoted(group_pair.first) << ", Values: [ ";
        for (const auto& item : group_pair.second) {
            std::cout << item << " ";
        }
        std::cout << "]" << std::endl;
    }
    std::cout << std::endl;
}


int main() {
    example_basic();
    example_integers();
    example_strings_first_char();
    example_empty();
    example_same_key();
    example_custom_struct();

    return 0;
}

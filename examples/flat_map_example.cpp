#include "flat_map.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    // Create a flat_map with string keys and int values
    cpp_collections::flat_map<std::string, int> word_counts;

    // Insert some elements using an initializer list
    word_counts.insert({
        {"apple", 10},
        {"banana", 5},
        {"cherry", 15}
    });

    // Use operator[] to insert or access elements
    word_counts["apple"] += 1;
    word_counts["date"] = 3;

    // Iterate over the elements (they will be in sorted order by key)
    std::cout << "Word counts:" << std::endl;
    for (const auto& pair : word_counts) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }

    // Find an element
    auto it = word_counts.find("banana");
    if (it != word_counts.end()) {
        std::cout << "\nFound 'banana' with count: " << it->second << std::endl;
    }

    // Erase an element
    word_counts.erase("banana");
    std::cout << "\nAfter erasing 'banana':" << std::endl;
    for (const auto& pair : word_counts) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }

    // Create a flat_map from a vector of pairs
    std::vector<std::pair<int, std::string>> data = {{3, "three"}, {1, "one"}, {2, "two"}};
    cpp_collections::flat_map<int, std::string> numbers(data.begin(), data.end());

    std::cout << "\nNumbers:" << std::endl;
    for (const auto& pair : numbers) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }

    return 0;
}

#include "pairwise.h" // Corrected include path
#include <iostream>
#include <vector>
#include <string>
#include <list>

int main() {
    // Example 1: Using pairwise with a std::vector of integers
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    std::cout << "Pairwise iteration over std::vector<int> {1, 2, 3, 4, 5}:" << std::endl;
    for (const auto& pair : std_ext::pairwise(numbers)) {
        std::cout << "(" << pair.first << ", " << pair.second << ")" << std::endl;
    }
    std::cout << std::endl;

    // Example 2: Using pairwise with a std::list of strings
    std::list<std::string> words = {"alpha", "beta", "gamma", "delta"};
    std::cout << "Pairwise iteration over std::list<std::string> {\"alpha\", \"beta\", \"gamma\", \"delta\"}:" << std::endl;
    for (const auto& pair : std_ext::pairwise(words)) {
        std::cout << "(\"" << pair.first << "\", \"" << pair.second << "\")" << std::endl;
    }
    std::cout << std::endl;

    // Example 3: Using pairwise with a C-style array
    const char* c_array[] = {"one", "two", "three"};
    std::cout << "Pairwise iteration over C-style array {\"one\", \"two\", \"three\"}:" << std::endl;
    for (const auto& pair : std_ext::pairwise(c_array)) {
        std::cout << "(\"" << pair.first << "\", \"" << pair.second << "\")" << std::endl;
    }
    std::cout << std::endl;

    // Example 4: Empty container
    std::vector<int> empty_vector;
    std::cout << "Pairwise iteration over an empty vector:" << std::endl;
    auto empty_view = std_ext::pairwise(empty_vector);
    if (empty_view.empty()) {
        std::cout << "View is empty, no pairs." << std::endl;
    }
    for (const auto& pair : empty_view) {
        // This loop will not execute
        std::cout << "(" << pair.first << ", " << pair.second << ")" << std::endl;
    }
    std::cout << std::endl;

    // Example 5: Single element container
    std::vector<int> single_element_vector = {42};
    std::cout << "Pairwise iteration over a single-element vector {42}:" << std::endl;
    auto single_view = std_ext::pairwise(single_element_vector);
    if (single_view.empty()) {
        std::cout << "View is empty, no pairs." << std::endl;
    }
    for (const auto& pair : single_view) {
        // This loop will not execute
        std::cout << "(" << pair.first << ", " << pair.second << ")" << std::endl;
    }
    std::cout << std::endl;

    const std::vector<int> const_numbers = {10, 20, 30};
    std::cout << "Pairwise iteration over a const std::vector<int> {10, 20, 30}:" << std::endl;
    for (const auto& pair : std_ext::pairwise(const_numbers)) {
        std::cout << "(" << pair.first << ", " << pair.second << ")" << std::endl;
    }
    std::cout << std::endl;

    return 0;
}

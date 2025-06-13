// This file demonstrates basic usage of the pydict::dict class.
// For comprehensive unit tests, please see tests/dict_test.cpp.

#include <iostream>
#include <string>
#include <cassert>

#include "dict.h"

int main() {
    using namespace pydict;
    
    // Test construction and basic operations
    dict<std::string, int> d = {{"apple", 5}, {"banana", 3}, {"cherry", 8}};
    
    // Test ordered iteration (maintains insertion order)
    std::cout << "Original dict: " << d << std::endl;
    
    // Test element access
    d["date"] = 12;  // Insert new element
    d["apple"] = 7;  // Update existing element
    
    std::cout << "After modifications: " << d << std::endl;
    
    // Test Python-like methods
    std::cout << "Keys: ";
    for (const auto& key : d.keys()) {
        std::cout << key << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Values: ";
    for (const auto& value : d.values()) {
        std::cout << value << " ";
    }
    std::cout << std::endl;
    
    // Test get method with default
    std::cout << "Get 'apple': " << d.get("apple") << std::endl;
    std::cout << "Get 'grape' (default 0): " << d.get("grape", 0) << std::endl;
    
    // Test pop method
    int popped = d.pop("banana");
    std::cout << "Popped 'banana': " << popped << std::endl;
    std::cout << "After pop: " << d << std::endl;
    
    // Test contains
    std::cout << "Contains 'apple': " << d.contains("apple") << std::endl;
    std::cout << "Contains 'banana': " << d.contains("banana") << std::endl;
    
    // Test STL algorithms
    auto it = std::find_if(d.begin(), d.end(), [](const auto& pair) {
        return pair.second() > 10;  // Use second() method instead of .second
    });
    
    if (it != d.end()) {
        std::cout << "Found element with value > 10: " << it->first() << std::endl;
    }
    
    // Test update
    dict<std::string, int> other = {{"elderberry", 15}, {"fig", 20}};
    d.update(other);
    std::cout << "After update: " << d << std::endl;
    
    return 0;
}

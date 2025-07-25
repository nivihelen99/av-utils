#include "sorted_vector_map.h"
#include <iostream>
#include <string>

int main() {
    // Create a sorted_vector_map with integer keys and string values
    sorted_vector_map<int, std::string> map;

    // Insert some elements
    map.insert({5, "apple"});
    map.insert({2, "banana"});
    map.insert({8, "cherry"});
    map.insert({1, "date"});

    // Print the elements in sorted order
    std::cout << "Elements in the map:" << std::endl;
    for (const auto& pair : map) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }

    // Find an element
    auto it = map.find(8);
    if (it != map.end()) {
        std::cout << "\nFound element with key 8: " << it->second << std::endl;
    } else {
        std::cout << "\nElement with key 8 not found" << std::endl;
    }

    // Try to find a non-existent element
    it = map.find(3);
    if (it != map.end()) {
        std::cout << "Found element with key 3: " << it->second << std::endl;
    } else {
        std::cout << "Element with key 3 not found" << std::endl;
    }

    // Use the subscript operator
    std::cout << "\nUsing the subscript operator:" << std::endl;
    std::cout << "map[2]: " << map[2] << std::endl;
    std::cout << "map[6]: " << map[6] << std::endl; // This will insert a new element

    // Print the elements again to see the new element
    std::cout << "\nElements in the map after using subscript operator:" << std::endl;
    for (const auto& pair : map) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }

    // Erase an element
    map.erase(5);
    std::cout << "\nElements in the map after erasing key 5:" << std::endl;
    for (const auto& pair : map) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }

    return 0;
}

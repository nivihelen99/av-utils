#include "../include/flat_map.h" // Adjust path as necessary
#include <iostream>
#include <string>
#include <stdexcept> // For std::out_of_range example

int main() {
    // Create a FlatMap with int keys and string values
    FlatMap<int, std::string> myMap;

    std::cout << "FlatMap Example" << std::endl;
    std::cout << "---------------" << std::endl;

    // Insert some elements
    std::cout << "\nInserting elements..." << std::endl;
    myMap.insert(3, "apple");
    myMap.insert(1, "banana");
    myMap.insert(4, "cherry");
    myMap.insert(2, "date");

    std::cout << "Map size: " << myMap.size() << std::endl;

    // Iterate and print elements (they will be sorted by key)
    std::cout << "\nIterating through map (sorted by key):" << std::endl;
    for (const auto& pair : myMap) {
        std::cout << pair.first << " => " << pair.second << std::endl;
    }

    // Find an element
    std::cout << "\nFinding element with key 2:" << std::endl;
    const std::string* valuePtr = myMap.find(2);
    if (valuePtr) {
        std::cout << "Found: key 2 => " << *valuePtr << std::endl;
    } else {
        std::cout << "Key 2 not found." << std::endl;
    }

    std::cout << "\nFinding element with key 5 (should not exist):" << std::endl;
    if (myMap.find(5)) {
        std::cout << "Found: key 5 (Error, should not be present)" << std::endl;
    } else {
        std::cout << "Key 5 not found, as expected." << std::endl;
    }

    // Using contains
    std::cout << "\nUsing contains():" << std::endl;
    std::cout << "Contains key 3? " << (myMap.contains(3) ? "Yes" : "No") << std::endl;
    std::cout << "Contains key 5? " << (myMap.contains(5) ? "Yes" : "No") << std::endl;


    // Using at()
    std::cout << "\nUsing at():" << std::endl;
    try {
        std::cout << "Value for key 1: " << myMap.at(1) << std::endl;
        std::cout << "Attempting to access key 5 using at()..." << std::endl;
        std::cout << "Value for key 5: " << myMap.at(5) << std::endl; // This will throw
    } catch (const std::out_of_range& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }

    // Using operator[]
    std::cout << "\nUsing operator[]:" << std::endl;
    std::cout << "Value for key 4 (existing): " << myMap[4] << std::endl;
    myMap[4] = "updated_cherry"; // Modify existing
    std::cout << "Value for key 4 (updated): " << myMap[4] << std::endl;

    std::cout << "Accessing key 5 (non-existing, will insert default std::string):" << std::endl;
    std::cout << "myMap[5] = \"" << myMap[5] << "\"" << std::endl; // Default std::string is empty
    std::cout << "Map size after operator[] default insert: " << myMap.size() << std::endl;
    myMap[5] = "elderberry";
    std::cout << "myMap[5] after assignment = \"" << myMap[5] << "\"" << std::endl;


    // Erase an element
    std::cout << "\nErasing element with key 1:" << std::endl;
    if (myMap.erase(1)) {
        std::cout << "Key 1 erased." << std::endl;
    } else {
        std::cout << "Key 1 not found for erasure." << std::endl;
    }
    std::cout << "Map size after erase: " << myMap.size() << std::endl;

    std::cout << "\nFinal map contents:" << std::endl;
    for (const auto& pair : myMap) {
        std::cout << pair.first << " => " << pair.second << std::endl;
    }

    // Example with custom comparator (std::greater)
    FlatMap<std::string, int, std::greater<std::string>> reverseSortedMap;
    std::cout << "\n\nExample with std::greater (keys sorted descending):" << std::endl;
    reverseSortedMap.insert("zebra", 10);
    reverseSortedMap.insert("apple", 20);
    reverseSortedMap.insert("monkey", 30);

    for(const auto& pair : reverseSortedMap) {
        std::cout << pair.first << " => " << pair.second << std::endl;
    }


    return 0;
}

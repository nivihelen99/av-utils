#include "flatMap.h" // Assuming flatMap.h is in a directory known to the include path
#include <iostream>
#include <string>
#include <vector> // For printing vector contents if needed

void print_line() {
    std::cout << "----------------------------------------" << std::endl;
}

int main() {
    // Create a FlatMap with int keys and string values
    FlatMap<int, std::string> myMap;
    std::cout << "Created an empty FlatMap<int, std::string>" << std::endl;
    std::cout << "Initial size: " << myMap.size() << std::endl;
    std::cout << "Is empty? " << (myMap.empty() ? "Yes" : "No") << std::endl;
    print_line();

    // Insert some elements
    std::cout << "Inserting elements..." << std::endl;
    myMap.insert(3, "Three");
    myMap.insert(1, "One");
    myMap.insert(4, "Four");
    myMap.insert(2, "Two"); // Insert out of order to test sorting

    std::cout << "Size after inserts: " << myMap.size() << std::endl;
    std::cout << "Is empty? " << (myMap.empty() ? "Yes" : "No") << std::endl;
    print_line();

    // Iterate and print elements (should be sorted by key)
    std::cout << "Contents of the map (sorted by key):" << std::endl;
    for (const auto& pair : myMap) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }
    print_line();

    // Find an element
    std::cout << "Finding element with key 3:" << std::endl;
    const std::string* valuePtr = myMap.find(3);
    if (valuePtr) {
        std::cout << "Found value: " << *valuePtr << std::endl;
    } else {
        std::cout << "Value not found." << std::endl;
    }

    std::cout << "Finding element with key 5 (should not exist):" << std::endl;
    valuePtr = myMap.find(5);
    if (valuePtr) {
        std::cout << "Found value: " << *valuePtr << std::endl;
    } else {
        std::cout << "Value not found (as expected)." << std::endl;
    }
    print_line();

    // Use contains
    std::cout << "Checking existence of keys:" << std::endl;
    std::cout << "Contains key 1? " << (myMap.contains(1) ? "Yes" : "No") << std::endl;
    std::cout << "Contains key 5? " << (myMap.contains(5) ? "Yes" : "No") << std::endl;
    print_line();

    // Use operator[] to access/insert
    std::cout << "Using operator[]:" << std::endl;
    std::cout << "Value for key 2 (existing): " << myMap[2] << std::endl;
    myMap[2] = "Two_updated"; // Update existing
    std::cout << "Updated value for key 2: " << myMap[2] << std::endl;

    std::cout << "Accessing key 5 with operator[] (should insert default):" << std::endl;
    std::cout << "myMap[5] = "" << myMap[5] << """ << std::endl; // Default for std::string is ""
    std::cout << "Size after operator[] insert: " << myMap.size() << std::endl;
    print_line();

    // Use at() to access (and handle potential exception)
    std::cout << "Using at():" << std::endl;
    try {
        std::cout << "Value for key 1 (existing): " << myMap.at(1) << std::endl;
        std::cout << "Attempting to access key 10 with at():" << std::endl;
        std::cout << myMap.at(10) << std::endl; // This should throw
    } catch (const std::out_of_range& e) {
        std::cout << "Caught exception: " << e.what() << " (as expected)" << std::endl;
    }
    print_line();

    // Erase an element
    std::cout << "Erasing element with key 3:" << std::endl;
    bool erased = myMap.erase(3);
    std::cout << "Was key 3 erased? " << (erased ? "Yes" : "No") << std::endl;
    std::cout << "Size after erase: " << myMap.size() << std::endl;
    std::cout << "Contents after erasing key 3:" << std::endl;
    for (const auto& pair : myMap) {
        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }
    print_line();

    std::cout << "Erasing element with key 100 (non-existent):" << std::endl;
    erased = myMap.erase(100);
    std::cout << "Was key 100 erased? " << (erased ? "Yes" : "No") << " (as expected)" << std::endl;
    std::cout << "Size after attempting to erase non-existent key: " << myMap.size() << std::endl;
    print_line();

    // Example with string keys
    FlatMap<std::string, int> cityPopulation;
    std::cout << "Creating FlatMap<std::string, int> for city populations." << std::endl;
    cityPopulation.insert("New York", 8399000);
    cityPopulation.insert("Los Angeles", 3972000);
    cityPopulation.insert("Chicago", 2705000);
    cityPopulation.insert("AlphaVille", 10000); // To check sorting with strings

    std::cout << "City populations (sorted by city name):" << std::endl;
    for (const auto& entry : cityPopulation) {
        std::cout << entry.first << ": " << entry.second << std::endl;
    }
    print_line();

    std::cout << "Example finished." << std::endl;

    return 0;
}

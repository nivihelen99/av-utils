#include "multikey_map.h"
#include <iostream>
#include <string>
#include <vector>

// A simple struct to demonstrate usage with custom types (must be hashable and equatable)
struct MyStruct {
    int id;
    std::string name;

    bool operator==(const MyStruct& other) const {
        return id == other.id && name == other.name;
    }
};

// Hash specialization for MyStruct
namespace std {
template <>
struct hash<MyStruct> {
    size_t operator()(const MyStruct& s) const {
        size_t h1 = hash<int>()(s.id);
        size_t h2 = hash<string>()(s.name);
        return h1 ^ (h2 << 1); // Simple combination
    }
};
} // namespace std

int main() {
    // Example 1: Basic usage with int and std::string keys, storing std::string values
    std::cout << "--- Example 1: Basic Usage (int, std::string) -> std::string ---" << std::endl;
    MultiKeyMap<std::string, int, std::string> map1;

    map1.insert(1, "apple", "Fruit: Red or Green");
    map1.emplace(2, "banana", "Fruit: Yellow");
    map1.insert(1, "apricot", "Fruit: Orange"); // Key (1, "apricot")

    // Using the call operator for insertion/assignment
    map1(3, "cherry") = "Fruit: Red";

    std::cout << "Map1 size: " << map1.size() << std::endl;

    // Find and print values
    if (auto it = map1.find(1, "apple"); it != map1.end()) {
        std::cout << "Found (1, \"apple\"): " << it->second << std::endl;
    }
    if (map1.contains(2, "banana")) {
        std::cout << "Value for (2, \"banana\"): " << map1.at(2, "banana") << std::endl;
    }

    // Try to find a non-existent key
    if (map1.find(10, "nonexistent") == map1.end()) {
        std::cout << "Key (10, \"nonexistent\") not found, as expected." << std::endl;
    }

    // Using operator() for access (like at, but creates if not exists with default value)
    std::cout << "Accessing (3, \"cherry\") via operator(): " << map1(3, "cherry") << std::endl;
    // Accessing a new key with operator() will default-construct the value (empty string here)
    // std::cout << "Accessing (4, \"date\") via operator() (new): " << map1(4, "date") << std::endl;
    // std::cout << "Map1 size after new access: " << map1.size() << std::endl;


    // Iteration
    std::cout << "Iterating map1:" << std::endl;
    for (const auto& pair : map1) {
        // pair.first is a std::tuple<int, std::string>
        // pair.second is the std::string value
        std::cout << "  Key: (" << std::get<0>(pair.first) << ", " << std::get<1>(pair.first)
                  << "), Value: " << pair.second << std::endl;
    }

    // Erase an element
    map1.erase(1, "apple");
    std::cout << "Map1 size after erasing (1, \"apple\"): " << map1.size() << std::endl;


    // Example 2: Using three keys (int, double, char) -> int
    std::cout << "\n--- Example 2: Three Keys (int, double, char) -> int ---" << std::endl;
    MultiKeyMap<int, int, double, char> map2;
    map2.insert(10, 3.14, 'a', 100);
    map2.insert(20, 2.71, 'b', 200);
    map2(10, 3.14, 'z') = 101; // Update if key parts are same, or new entry

    std::cout << "Value for (10, 3.14, 'a'): " << map2.at(10, 3.14, 'a') << std::endl;
    std::cout << "Value for (10, 3.14, 'z'): " << map2.at(10, 3.14, 'z') << std::endl;


    // Example 3: Using std::tuple directly for keys
    std::cout << "\n--- Example 3: Using std::tuple for keys ---" << std::endl;
    MultiKeyMap<std::string, int, std::string> map3;
    std::tuple<int, std::string> key1 = {100, "tuple_key_A"};
    std::tuple<int, std::string> key2 = {200, "tuple_key_B"};

    map3.insert(key1, "Value for key1"); // Corrected: use insert for tuples
    map3[key2] = "Value for key2 (via operator[])"; // operator[] takes a tuple

    std::cout << "Value for key1 (tuple): " << map3.at_tuple(key1) << std::endl;
    std::cout << "Value for key2 (tuple) from operator[]: " << map3[key2] << std::endl;

    // Using operator() with individual keys which is generally preferred for ergonomics
    std::cout << "Value for (100, \"tuple_key_A\") using operator(): " << map3(100, "tuple_key_A") << std::endl;


    // Example 4: Using custom struct as part of the key
    std::cout << "\n--- Example 4: Custom Struct in Key (MyStruct, int) -> std::string ---" << std::endl;
    MultiKeyMap<std::string, MyStruct, int> map4;

    MyStruct s1 = {1, "Obj1"};
    MyStruct s2 = {2, "Obj2"};

    map4.insert(s1, 10, "S1-10");
    map4.insert(s2, 20, "S2-20");
    map4.insert({1, "Obj1"}, 30, "S1-30"); // Using initializer list for MyStruct, then int

    std::cout << "Map4 size: " << map4.size() << std::endl;
    if(map4.contains(s1, 10)) {
        std::cout << "Found (s1, 10): " << map4.at(s1, 10) << std::endl;
    }
    if(map4.contains({1, "Obj1"}, 30)) {
        std::cout << "Found ({1, \"Obj1\"}, 30): " << map4.at({1, "Obj1"}, 30) << std::endl;
    }

    std::cout << "Iterating map4:" << std::endl;
    for (const auto& pair : map4) {
        const auto& key_tuple = pair.first; // std::tuple<MyStruct, int>
        const MyStruct& ms = std::get<0>(key_tuple);
        int ik = std::get<1>(key_tuple);
        std::cout << "  Key: (MyStruct{id=" << ms.id << ", name=" << ms.name << "}, " << ik
                  << "), Value: " << pair.second << std::endl;
    }


    // Example 5: Clear and empty
    std::cout << "\n--- Example 5: Clear and Empty ---" << std::endl;
    std::cout << "Map1 size before clear: " << map1.size() << std::endl;
    std::cout << "Map1 empty before clear? " << (map1.empty() ? "Yes" : "No") << std::endl;
    map1.clear();
    std::cout << "Map1 size after clear: " << map1.size() << std::endl;
    std::cout << "Map1 empty after clear? " << (map1.empty() ? "Yes" : "No") << std::endl;

    return 0;
}

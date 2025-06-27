#include "FrozenDict.h"
#include <iostream>
#include <string>
#include <unordered_map>

// Custom struct to demonstrate usage with custom types
struct MyKey {
    int id;
    std::string name;

    MyKey(int i, std::string n) : id(i), name(std::move(n)) {}

    // Equality for std::unordered_map keys
    bool operator==(const MyKey& other) const {
        return id == other.id && name == other.name;
    }
    // Less-than for sorting (e.g., if used in std::map or FrozenDict's internal sort)
    bool operator<(const MyKey& other) const {
        if (id != other.id) return id < other.id;
        return name < other.name;
    }
};

struct MyValue {
    double value;
    std::string description;

    MyValue(double v, std::string d) : value(v), description(std::move(d)) {}

    // Equality for std::unordered_map values (if FrozenDict is used as a value and needs comparison)
    bool operator==(const MyValue& other) const {
        return value == other.value && description == other.description;
    }
};

// Hash function for MyKey
namespace std {
template <>
struct hash<MyKey> {
    std::size_t operator()(const MyKey& k) const {
        std::size_t h1 = std::hash<int>{}(k.id);
        std::size_t h2 = std::hash<std::string>{}(k.name);
        return h1 ^ (h2 << 1); // Simple hash combining
    }
};

// Hash function for MyValue (needed if MyValue is part of a FrozenDict that's a key itself)
template <>
struct hash<MyValue> {
    std::size_t operator()(const MyValue& v) const {
        std::size_t h1 = std::hash<double>{}(v.value);
        std::size_t h2 = std::hash<std::string>{}(v.description);
        return h1 ^ (h2 << 1);
    }
};
} // namespace std


void basic_demonstration() {
    std::cout << "--- Basic Demonstration ---" << std::endl;

    cpp_collections::FrozenDict<std::string, int> fd1 = {
        {"apple", 1},
        {"banana", 2},
        {"cherry", 3}
    };

    std::cout << "fd1 created with " << fd1.size() << " elements." << std::endl;

    // Lookup
    std::cout << "Value of 'banana': " << fd1.at("banana") << std::endl;
    std::cout << "Value of 'apple' (op[]): " << fd1["apple"] << std::endl;

    if (fd1.contains("cherry")) {
        std::cout << "'cherry' is in fd1." << std::endl;
    }
    if (!fd1.contains("date")) {
        std::cout << "'date' is not in fd1." << std::endl;
    }

    auto it_banana = fd1.find("banana");
    if (it_banana != fd1.end()) {
        std::cout << "Found 'banana' via find: key=" << it_banana->first << ", value=" << it_banana->second << std::endl;
    }

    // Iteration (order is guaranteed by key sorting)
    std::cout << "Iterating fd1 (sorted by key):" << std::endl;
    for (const auto& pair : fd1) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }
    std::cout << std::endl;
}

void duplicate_key_construction() {
    std::cout << "--- Duplicate Key Construction (Last Wins) ---" << std::endl;
    cpp_collections::FrozenDict<std::string, int> fd_dup = {
        {"apple", 10},
        {"banana", 20},
        {"apple", 100} // Last "apple" should win
    };

    std::cout << "fd_dup created. Size: " << fd_dup.size() << std::endl;
    std::cout << "Value of 'apple': " << fd_dup.at("apple") << " (expected 100)" << std::endl;
    std::cout << "Iterating fd_dup:" << std::endl;
    for (const auto& pair : fd_dup) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }
    std::cout << std::endl;
}

void frozendict_as_map_key() {
    std::cout << "--- FrozenDict as std::unordered_map Key ---" << std::endl;

    // Note: For FrozenDict<K,V> to be a key, both K and V must be hashable.
    // std::hash<V> is used by std::hash<FrozenDict<...>>.
    // If V is a custom type, it needs a std::hash specialization.
    using FD_StringInt = cpp_collections::FrozenDict<std::string, int>;

    FD_StringInt fd_key1 = {{"a", 1}, {"b", 2}};
    FD_StringInt fd_key2 = {{"x", 10}, {"y", 20}};
    FD_StringInt fd_key1_again = {{"b", 2}, {"a", 1}}; // Same content, different order in initializer

    std::unordered_map<FD_StringInt, std::string> outer_map;

    outer_map[fd_key1] = "First FrozenDict";
    outer_map[fd_key2] = "Second FrozenDict";

    std::cout << "Value for fd_key1: " << outer_map.at(fd_key1) << std::endl;
    std::cout << "Value for fd_key1_again (should be same as fd_key1): " << outer_map.at(fd_key1_again) << std::endl;

    if (outer_map.count(fd_key2)) {
        std::cout << "fd_key2 is present in outer_map." << std::endl;
    }

    std::cout << "Size of outer_map: " << outer_map.size() << " (expected 2 if fd_key1 and fd_key1_again hash equally)" << std::endl;
    std::cout << std::endl;


    // Example with custom key and value types for FrozenDict
    // Explicitly provide std::less<MyKey> as the CompareFunc type
    // and std::hash<MyKey> as HashFunc, std::equal_to<MyKey> as KeyEqualFunc
    using FD_MyKeyMyValue = cpp_collections::FrozenDict<MyKey, MyValue, std::hash<MyKey>, std::equal_to<MyKey>, std::less<MyKey>>;

    FD_MyKeyMyValue fd_custom_key1(
        {
            {MyKey(1, "keyA"), MyValue(1.1, "valA")},
            {MyKey(2, "keyB"), MyValue(2.2, "valB")}
        },
        std::less<MyKey>(), // Pass comparator instance
        std::hash<MyKey>()  // Pass hasher instance
    );
    FD_MyKeyMyValue fd_custom_key2(
        {
            {MyKey(3, "keyC"), MyValue(3.3, "valC")}
        },
        std::less<MyKey>(),
        std::hash<MyKey>()
    );

    std::unordered_map<FD_MyKeyMyValue, std::string> custom_outer_map;
    custom_outer_map[fd_custom_key1] = "Custom FD Key 1";
    custom_outer_map[fd_custom_key2] = "Custom FD Key 2";

    std::cout << "Value for fd_custom_key1: " << custom_outer_map.at(fd_custom_key1) << std::endl;
    std::cout << "Size of custom_outer_map: " << custom_outer_map.size() << std::endl;
    std::cout << std::endl;
}


int main() {
    basic_demonstration();
    duplicate_key_construction();
    frozendict_as_map_key();

    // Test empty FrozenDict
    std::cout << "--- Empty FrozenDict ---" << std::endl;
    cpp_collections::FrozenDict<int, int> empty_fd;
    std::cout << "Empty fd size: " << empty_fd.size() << (empty_fd.empty() ? " (is empty)" : " (is not empty)") << std::endl;
    try {
        empty_fd.at(1);
    } catch (const std::out_of_range& e) {
        std::cout << "Accessing empty_fd with at(1) threw: " << e.what() << std::endl;
    }
    std::cout << std::endl;


    std::cout << "FrozenDict examples completed." << std::endl;
    return 0;
}

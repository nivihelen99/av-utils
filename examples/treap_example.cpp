#include "treap.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm> // For std::is_sorted

void print_treap_sorted(const Treap<int, std::string>& treap) {
    std::cout << "Treap contents (sorted by key):" << std::endl;
    for (const auto& pair : treap) {
        std::cout << "Key: " << pair.first << ", Value: \"" << pair.second << "\"" << std::endl;
    }
    if (treap.empty()) {
        std::cout << "(empty)" << std::endl;
    }
    std::cout << "Size: " << treap.size() << std::endl;
    std::cout << "--------------------------" << std::endl;
}

void print_treap_sorted_string_keys(const Treap<std::string, int>& treap) {
    std::cout << "Treap contents (sorted by key):" << std::endl;
    for (const auto& pair : treap) {
        std::cout << "Key: \"" << pair.first << "\", Value: " << pair.second << std::endl;
    }
    if (treap.empty()) {
        std::cout << "(empty)" << std::endl;
    }
    std::cout << "Size: " << treap.size() << std::endl;
    std::cout << "--------------------------" << std::endl;
}


int main() {
    // Treap with int keys and string values
    Treap<int, std::string> my_treap;

    std::cout << "Initial empty treap:" << std::endl;
    print_treap_sorted(my_treap);

    // Insertion
    std::cout << "Inserting elements..." << std::endl;
    my_treap.insert(10, "Apple");
    my_treap.insert(5, "Banana");
    auto insert_result = my_treap.insert(15, "Cherry");
    std::cout << "Inserted 15, Cherry. New element? " << (insert_result.second ? "Yes" : "No") << ". Iterator points to key: " << insert_result.first->first << std::endl;

    my_treap.insert(3, "Date");
    my_treap.insert(7, "Elderberry");
    print_treap_sorted(my_treap);

    // Inserting a duplicate key (should update value)
    std::cout << "Updating value for key 5..." << std::endl;
    insert_result = my_treap.insert(5, "Blueberry");
    std::cout << "Inserted 5, Blueberry. New element? " << (insert_result.second ? "Yes" : "No") << ". Iterator points to key: " << insert_result.first->first << std::endl;
    print_treap_sorted(my_treap);

    // Using operator[] for insertion and access
    std::cout << "Using operator[]..." << std::endl;
    my_treap[20] = "Fig"; // New element
    std::cout << "Value for key 20 (after insertion): " << my_treap[20] << std::endl;
    my_treap[10] = "Apricot"; // Update existing element
    std::cout << "Value for key 10 (after update): " << my_treap[10] << std::endl;
    print_treap_sorted(my_treap);

    std::cout << "Value for key 99 (will insert default string): " << my_treap[99] << std::endl;
    print_treap_sorted(my_treap);


    // Finding elements
    std::cout << "Finding elements..." << std::endl;
    int key_to_find = 15;
    if (my_treap.contains(key_to_find)) {
        std::cout << "Key " << key_to_find << " found. Value: \"" << *my_treap.find(key_to_find) << "\"" << std::endl;
    } else {
        std::cout << "Key " << key_to_find << " not found." << std::endl;
    }

    key_to_find = 9;
    if (my_treap.find(key_to_find)) { // find returns pointer, so check for nullptr
        std::cout << "Key " << key_to_find << " found. Value: \"" << *my_treap.find(key_to_find) << "\"" << std::endl;
    } else {
        std::cout << "Key " << key_to_find << " not found." << std::endl;
    }
    std::cout << "--------------------------" << std::endl;

    // Deletion
    std::cout << "Deleting elements..." << std::endl;
    int key_to_delete = 7;
    if (my_treap.erase(key_to_delete)) {
        std::cout << "Key " << key_to_delete << " deleted successfully." << std::endl;
    } else {
        std::cout << "Key " << key_to_delete << " not found for deletion." << std::endl;
    }
    print_treap_sorted(my_treap);

    key_to_delete = 999; // Non-existent key
    if (my_treap.erase(key_to_delete)) {
        std::cout << "Key " << key_to_delete << " deleted successfully." << std::endl;
    } else {
        std::cout << "Key " << key_to_delete << " not found for deletion." << std::endl;
    }
    print_treap_sorted(my_treap);

    // Delete root (potentially)
    std::cout << "Deleting key 10..." << std::endl;
    my_treap.erase(10);
    print_treap_sorted(my_treap);

    // Check iterator functionality (const iterator)
    const Treap<int, std::string>& const_treap = my_treap;
    std::cout << "Iterating using const_iterator (cbegin/cend):" << std::endl;
    std::vector<int> keys_from_const_iter;
    for (auto it = const_treap.cbegin(); it != const_treap.cend(); ++it) {
        std::cout << "Key: " << it->first << ", Value: \"" << it->second << "\"" << std::endl;
        keys_from_const_iter.push_back(it->first);
    }
    if (std::is_sorted(keys_from_const_iter.begin(), keys_from_const_iter.end())) {
        std::cout << "Const iteration order is sorted." << std::endl;
    } else {
        std::cout << "ERROR: Const iteration order is NOT sorted." << std::endl;
    }
    std::cout << "--------------------------" << std::endl;


    // Clear the treap
    std::cout << "Clearing the treap..." << std::endl;
    my_treap.clear();
    print_treap_sorted(my_treap);

    // Test with string keys
    std::cout << "\nTesting Treap with string keys and int values:" << std::endl;
    Treap<std::string, int> string_key_treap;
    string_key_treap.insert("David", 30);
    string_key_treap.insert("Alice", 25);
    string_key_treap.insert("Charlie", 35);
    string_key_treap.insert("Bob", 28);
    print_treap_sorted_string_keys(string_key_treap);

    std::cout << "Value for Bob: " << string_key_treap["Bob"] << std::endl;
    string_key_treap["Alice"] = 26; // Update Alice
    print_treap_sorted_string_keys(string_key_treap);

    // Move semantics test
    std::cout << "Testing move semantics..." << std::endl;
    Treap<std::string, int> moved_treap = std::move(string_key_treap);
    std::cout << "Moved treap contents:" << std::endl;
    print_treap_sorted_string_keys(moved_treap);
    std::cout << "Original string_key_treap after move:" << std::endl;
    print_treap_sorted_string_keys(string_key_treap); // Should be empty


    // Test insert rvalue keys and values
    std::cout << "Testing rvalue insert..." << std::endl;
    Treap<std::string, std::string> rvalue_treap;
    std::string rkey1 = "rvalue_key1";
    std::string rval1 = "rvalue_val1";
    rvalue_treap.insert(std::move(rkey1), std::move(rval1));
    // After move, rkey1 and rval1 are in a valid but unspecified state.
    // Do not use them directly expecting original values.

    rvalue_treap.insert("rvalue_key2", "rvalue_val2_literal"); // string literals are fine

    std::cout << "Rvalue treap contents:" << std::endl;
     for (const auto& pair : rvalue_treap) {
        std::cout << "Key: \"" << pair.first << "\", Value: \"" << pair.second << "\"" << std::endl;
    }
    std::cout << "Size: " << rvalue_treap.size() << std::endl;
    std::cout << "--------------------------" << std::endl;


    std::cout << "\nExample usage finished." << std::endl;

    return 0;
}

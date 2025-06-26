#include "RedBlackTree.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm> // For std::sort

// A more complex example demonstrating various features
int main() {
    // Use the collections namespace
    using collections::RedBlackTree;

    // Create a RedBlackTree with int keys and string values
    RedBlackTree<int, std::string> fruitTree;

    std::cout << "Is tree empty initially? " << (fruitTree.isEmpty() ? "Yes" : "No") << std::endl;

    // Insert some elements
    std::cout << "\n--- Inserting elements ---" << std::endl;
    fruitTree.insert(10, "Apple");
    fruitTree.insert(85, "Peach");
    fruitTree.insert(15, "Banana");
    fruitTree.insert(70, "Orange");
    fruitTree.insert(20, "Cherry");
    fruitTree.insert(60, "Plum");
    fruitTree.insert(30, "Date");
    fruitTree.insert(50, "Grape");
    fruitTree.insert(90, "Strawberry");
    fruitTree.insert(40, "Mango");
    fruitTree.insert(5, "Apricot");
    fruitTree.insert(25, "Blueberry");

    std::cout << "Tree structure after initial insertions:" << std::endl;
    fruitTree.printTree(); // Visual representation of the tree

    std::cout << "\nIs tree empty now? " << (fruitTree.isEmpty() ? "Yes" : "No") << std::endl;

    // Find elements
    std::cout << "\n--- Finding elements ---" << std::endl;
    std::vector<int> keysToFind = {15, 50, 99, 5};
    for (int key : keysToFind) {
        std::cout << "Finding key " << key << ": ";
        std::string* value = fruitTree.find(key);
        if (value) {
            std::cout << "Found value: " << *value << std::endl;
        } else {
            std::cout << "Key not found." << std::endl;
        }
    }

    // Check for existence using 'contains'
    std::cout << "\n--- Checking with 'contains' ---" << std::endl;
    std::cout << "Tree contains key 70 (Orange)? " << (fruitTree.contains(70) ? "Yes" : "No") << std::endl;
    std::cout << "Tree contains key 75 (Unknown)? " << (fruitTree.contains(75) ? "Yes" : "No") << std::endl;

    // Update an existing element
    std::cout << "\n--- Updating an element ---" << std::endl;
    std::cout << "Value for key 20 before update: " << *fruitTree.find(20) << std::endl;
    fruitTree.insert(20, "Sweet Cherry"); // Key 20 already exists, value will be updated
    std::cout << "Value for key 20 after update: " << *fruitTree.find(20) << std::endl;

    // Remove some elements
    std::cout << "\n--- Removing elements ---" << std::endl;
    std::vector<int> keysToRemove = {15, 70, 99, 5, 85}; // 99 is not in the tree
    for (int key : keysToRemove) {
        std::cout << "Attempting to remove key " << key << "..." << std::endl;
        fruitTree.remove(key);
        if (!fruitTree.contains(key)) {
            std::cout << "Key " << key << " successfully removed or was not present." << std::endl;
        } else {
            std::cout << "Error: Key " << key << " still present after removal attempt." << std::endl;
        }
    }

    std::cout << "\nTree structure after removals:" << std::endl;
    fruitTree.printTree();

    // Test finding removed elements
    std::cout << "\n--- Finding removed elements ---" << std::endl;
    std::cout << "Finding key 15 (Banana) after removal: ";
    if (fruitTree.find(15)) {
        std::cout << "Found (Error!)" << std::endl;
    } else {
        std::cout << "Not found (Correct)" << std::endl;
    }

    std::cout << "Finding key 70 (Orange) after removal: ";
    if (fruitTree.find(70)) {
        std::cout << "Found (Error!)" << std::endl;
    } else {
        std::cout << "Not found (Correct)" << std::endl;
    }

    // Insert more elements after deletions
    std::cout << "\n--- Inserting more elements ---" << std::endl;
    fruitTree.insert(100, "Watermelon");
    fruitTree.insert(1, "Fig");
    fruitTree.insert(55, "Kiwi");

    std::cout << "Tree structure after more insertions:" << std::endl;
    fruitTree.printTree();

    std::cout << "\n--- Final checks ---" << std::endl;
    std::cout << "Tree contains key 100 (Watermelon)? " << (fruitTree.contains(100) ? "Yes" : "No") << std::endl;
    std::cout << "Value for key 55 (Kiwi): " << *fruitTree.find(55) << std::endl;


    // Example with a custom comparator (descending order)
    std::cout << "\n\n--- Example with Custom Comparator (Descending Order) ---" << std::endl;
    RedBlackTree<int, std::string, std::greater<int>> descendingTree;
    descendingTree.insert(10, "Ten");
    descendingTree.insert(20, "Twenty");
    descendingTree.insert(5, "Five");

    std::cout << "Descending tree structure:" << std::endl;
    descendingTree.printTree(); // Root should be 10, 5 on right, 20 on left if printed with same logic

    std::cout << "Finding key 5 in descending tree: " << *descendingTree.find(5) << std::endl;


    std::cout << "\nExample complete." << std::endl;

    return 0;
}

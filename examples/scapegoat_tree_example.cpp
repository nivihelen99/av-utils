#include "include/ScapegoatTree.h" // Adjust path if necessary based on build setup
#include <iostream>
#include <string>

int main() {
    // Use the ScapegoatTree from the cpp_collections namespace
    cpp_collections::ScapegoatTree<int, std::string> s_tree(0.75); // Alpha = 0.75

    std::cout << "ScapegoatTree Example\n";
    std::cout << "---------------------\n";

    // Initial state
    std::cout << "Initial size: " << s_tree.size() << ", empty: " << (s_tree.empty() ? "yes" : "no") << std::endl;

    // Insertions
    std::cout << "\nInserting elements...\n";
    s_tree.insert(10, "Apple");
    s_tree.insert(5, "Banana");
    s_tree.insert(15, "Cherry");
    s_tree.insert(3, "Date");
    s_tree.insert(7, "Elderberry");
    s_tree.insert(12, "Fig");
    s_tree.insert(17, "Grape");

    std::cout << "Size after insertions: " << s_tree.size() << std::endl;

    // Finding elements
    std::cout << "\nFinding elements...\n";
    int key_to_find = 5;
    if (s_tree.contains(key_to_find)) {
        const std::string* value_ptr = s_tree.find(key_to_find);
        if (value_ptr) {
            std::cout << "Found key " << key_to_find << ": " << *value_ptr << std::endl;
        }
    } else {
        std::cout << "Key " << key_to_find << " not found." << std::endl;
    }

    key_to_find = 99; // Non-existent key
    if (s_tree.contains(key_to_find)) {
        std::cout << "Found key " << key_to_find << ": " << *s_tree.find(key_to_find) << std::endl;
    } else {
        std::cout << "Key " << key_to_find << " not found." << std::endl;
    }

    // Iterating over the tree
    std::cout << "\nTree elements (in-order traversal):" << std::endl;
    for (const auto& pair : s_tree) {
        std::cout << pair.first << " -> " << pair.second << std::endl;
    }

    // Erasing an element
    std::cout << "\nErasing element with key 10...\n";
    if (s_tree.erase(10)) {
        std::cout << "Element 10 erased successfully." << std::endl;
    } else {
        std::cout << "Element 10 not found for erasure." << std::endl;
    }
    std::cout << "Size after erasing 10: " << s_tree.size() << std::endl;
    std::cout << "Contains 10 after erase: " << (s_tree.contains(10) ? "yes" : "no") << std::endl;

    std::cout << "\nTree elements after erasing 10:" << std::endl;
    for (const auto& pair : s_tree) {
        std::cout << pair.first << " -> " << pair.second << std::endl;
    }

    // Re-inserting a previously erased element (tests reactivation)
    std::cout << "\nRe-inserting element with key 10 (value: Apricot)...\n";
    s_tree.insert(10, "Apricot");
    std::cout << "Size after re-inserting 10: " << s_tree.size() << std::endl;
    std::cout << "Contains 10 after re-insert: " << (s_tree.contains(10) ? "yes" : "no") << std::endl;
    if (s_tree.contains(10)) {
        std::cout << "Value for key 10: " << *s_tree.find(10) << std::endl;
    }

    std::cout << "\nTree elements after re-inserting 10:" << std::endl;
    for (const auto& pair : s_tree) {
        std::cout << pair.first << " -> " << pair.second << std::endl;
    }

    // Clear the tree
    std::cout << "\nClearing the tree...\n";
    s_tree.clear();
    std::cout << "Size after clear: " << s_tree.size() << ", empty: " << (s_tree.empty() ? "yes" : "no") << std::endl;

    std::cout << "\nExample finished.\n";

    return 0;
}

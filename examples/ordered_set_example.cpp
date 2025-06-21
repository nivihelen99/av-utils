#include <iostream>
#include "ordered_set.h" // Assuming ordered_set.h will be in the include path

int main() {
    OrderedSet<int> oset;

    // Add elements
    oset.insert(5);
    oset.insert(2);
    oset.insert(8);
    oset.insert(2); // Try inserting a duplicate

    std::cout << "OrderedSet after inserts:" << std::endl;
    for (int val : oset) {
        std::cout << val << " ";
    }
    std::cout << std::endl; // Expected: 2 5 8 (or 8 5 2 depending on internal order, but iteration should be sorted)

    // Check for existence
    std::cout << "Contains 5: " << (oset.contains(5) ? "Yes" : "No") << std::endl;
    std::cout << "Contains 3: " << (oset.contains(3) ? "Yes" : "No") << std::endl;

    // Remove an element
    oset.erase(5);
    std::cout << "OrderedSet after erasing 5:" << std::endl;
    for (int val : oset) {
        std::cout << val << " ";
    }
    std::cout << std::endl; // Expected: 2 8 (or 8 2)

    // Check size
    std::cout << "Size of set: " << oset.size() << std::endl;

    // Clear the set
    oset.clear();
    std::cout << "Size of set after clear: " << oset.size() << std::endl;
    std::cout << "Is set empty after clear: " << (oset.empty() ? "Yes" : "No") << std::endl;

    // Test with another type
    OrderedSet<std::string> strSet;
    strSet.insert("apple");
    strSet.insert("banana");
    strSet.insert("cherry");

    std::cout << "String OrderedSet:" << std::endl;
    for (const auto& s : strSet) {
        std::cout << s << " ";
    }
    std::cout << std::endl;


    return 0;
}

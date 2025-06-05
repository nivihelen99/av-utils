#include "skiplist.h"

// Example usage and test cases
int main() {
    SkipList<int> skipList;
    
    std::cout << "=== Inserting values ===" << std::endl;
    std::vector<int> values = {3, 6, 7, 9, 12, 19, 17, 26, 21, 25};
    
    for (int val : values) {
        skipList.insert(val);
    }
    
    skipList.display();
    
    std::cout << "=== Search operations ===" << std::endl;
    std::cout << "Search 19: " << (skipList.search(19) ? "Found" : "Not found") << std::endl;
    std::cout << "Search 15: " << (skipList.search(15) ? "Found" : "Not found") << std::endl;
    
    std::cout << "\n=== Skip list size ===" << std::endl;
    std::cout << "Size: " << skipList.size() << std::endl;
    
    std::cout << "\n=== All values in order ===" << std::endl;
    skipList.printValues();
    
    std::cout << "\n=== K-th element queries ===" << std::endl;
    try {
        std::cout << "3rd smallest (0-indexed): " << skipList.kthElement(3) << std::endl;
        std::cout << "5th smallest (0-indexed): " << skipList.kthElement(5) << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    
    std::cout << "\n=== Range query [10, 20] ===" << std::endl;
    auto rangeResult = skipList.rangeQuery(10, 20);
    std::cout << "Values in range [10, 20]: ";
    for (int val : rangeResult) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    std::cout << "\n=== Deletion operations ===" << std::endl;
    skipList.remove(19);
    skipList.remove(15); // Should fail
    
    skipList.display();
    skipList.printValues();

    std::cout << "\n\n=== Testing SkipList<std::string> ===" << std::endl;
    SkipList<std::string> stringSkipList;

    std::cout << "--- Inserting strings ---" << std::endl;
    stringSkipList.insert("apple");
    stringSkipList.insert("banana");
    stringSkipList.insert("cherry");
    stringSkipList.insert("date");
    stringSkipList.insert("fig");

    stringSkipList.display();
    stringSkipList.printValues();

    std::cout << "--- Search operations (string) ---" << std::endl;
    std::cout << "Search 'cherry': " << (stringSkipList.search("cherry") ? "Found" : "Not found") << std::endl;
    std::cout << "Search 'grape': " << (stringSkipList.search("grape") ? "Found" : "Not found") << std::endl;

    std::cout << "--- String skip list size ---" << std::endl;
    std::cout << "Size: " << stringSkipList.size() << std::endl;

    std::cout << "--- K-th element (string) ---" << std::endl;
    try {
        std::cout << "1st smallest (0-indexed): " << stringSkipList.kthElement(1) << std::endl; // Should be banana
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }

    std::cout << "--- Range query ['banana', 'fig'] (string) ---" << std::endl;
    auto stringRangeResult = stringSkipList.rangeQuery("banana", "fig");
    std::cout << "Values in range ['banana', 'fig']: ";
    for (const std::string& val : stringRangeResult) {
        std::cout << "\"" << val << "\" ";
    }
    std::cout << std::endl;


    std::cout << "--- Deletion operations (string) ---" << std::endl;
    stringSkipList.remove("banana");
    stringSkipList.remove("grape"); // Should fail

    stringSkipList.display();
    stringSkipList.printValues();
    
    return 0;
}

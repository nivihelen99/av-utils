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
    
    return 0;
}

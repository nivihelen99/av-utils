#include "persist_array.h"

int main() {
    try {
        demonstrate_persistent_array();
        
        // Basic correctness tests
        std::cout << "\n=== Running Basic Tests ===" << std::endl;
        
        PersistentArray<int> arr;
        assert(arr.empty());
        assert(arr.size() == 0);
        
        arr = arr.push_back(1).push_back(2).push_back(3);
        assert(arr.size() == 3);
        assert(arr[0] == 1);
        assert(arr[1] == 2);
        assert(arr[2] == 3);
        
        auto arr2 = arr.set(1, 100);
        assert(arr[1] == 2);    // Original unchanged
        assert(arr2[1] == 100); // New version changed
        
        std::cout << "All tests passed!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

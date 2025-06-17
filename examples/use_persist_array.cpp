#include "persist_array.h"

// Demonstration and test code
void demonstrate_persistent_array() {
    std::cout << "=== Persistent Array Demonstration ===" << '\n';
    
    // Create initial array
    PersistentArray<int> v1{1, 2, 3, 4, 5};
    std::cout << "v1 created with values: ";
    for (const auto& val : v1) {
        std::cout << val << " ";
    }
    std::cout << '\n';
    v1.print_debug_info();
    
    // Create version 2 by modifying v1
    auto v2 = v1.set(2, 100);
    std::cout << "\nv2 = v1.set(2, 100):" << '\n';
    std::cout << "v1: ";
    for (const auto& val : v1) {
        std::cout << val << " ";
    }
    std::cout << " (unchanged)" << '\n';
    std::cout << "v2: ";
    for (const auto& val : v2) {
        std::cout << val << " ";
    }
    std::cout << " (modified)" << '\n';
    
    v1.print_debug_info();
    v2.print_debug_info();
    
    // Create version 3 by adding to v2
    auto v3 = v2.push_back(200);
    std::cout << "\nv3 = v2.push_back(200):" << '\n';
    std::cout << "v2: ";
    for (const auto& val : v2) {
        std::cout << val << " ";
    }
    std::cout << '\n';
    std::cout << "v3: ";
    for (const auto& val : v3) {
        std::cout << val << " ";
    }
    std::cout << '\n';
    
    // Show that copying shares data initially
    auto v4 = v3;
    std::cout << "\nv4 = v3 (copy):" << '\n';
    v3.print_debug_info();
    v4.print_debug_info();
    
    // Modify v4 to trigger copy-on-write
    v4.set_inplace(0, 999);
    std::cout << "\nAfter v4.set_inplace(0, 999):" << '\n';
    std::cout << "v3: ";
    for (const auto& val : v3) {
        std::cout << val << " ";
    }
    std::cout << '\n';
    std::cout << "v4: ";
    for (const auto& val : v4) {
        std::cout << val << " ";
    }
    std::cout << '\n';
    v3.print_debug_info();
    v4.print_debug_info();
    
    // Demonstrate undo functionality
    std::cout << "\n=== Undo Functionality Demo ===" << '\n';
    std::vector<PersistentArray<int>> history;
    auto current = PersistentArray<int>{10, 20};
    history.push_back(current);
    
    std::cout << "Initial: ";
    for (const auto& val : current) {
        std::cout << val << " ";
    }
    std::cout << '\n';
    
    // Perform operations and save history
    current = current.push_back(30);
    history.push_back(current);
    std::cout << "After push_back(30): ";
    for (const auto& val : current) {
        std::cout << val << " ";
    }
    std::cout << '\n';
    
    current = current.set(1, 200);
    history.push_back(current);
    std::cout << "After set(1, 200): ";
    for (const auto& val : current) {
        std::cout << val << " ";
    }
    std::cout << '\n';
    
    // Undo operations
    std::cout << "\nUndo operations:" << '\n';
    for (int i = history.size() - 2; i >= 0; --i) {
        std::cout << "Undo to state " << i << ": ";
        for (const auto& val : history[i]) {
            std::cout << val << " ";
        }
        std::cout << '\n';
    }
}


int main() {
    try {
        demonstrate_persistent_array();
        
        // Basic correctness tests
        std::cout << "\n=== Running Basic Tests ===" << '\n';
        
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
        
        std::cout << "All tests passed!" << '\n';
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
}

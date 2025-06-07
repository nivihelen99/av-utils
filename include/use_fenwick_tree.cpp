#include "fenwick_tree.h"

// Example usage and testing
int main() {
    // Test 1: Basic operations
    std::cout << "=== Test 1: Basic Operations ===" << std::endl;
    FenwickTree ft(10);
    
    // Add some values
    ft.update(0, 5);   // arr[0] += 5
    ft.update(1, 3);   // arr[1] += 3  
    ft.update(2, 7);   // arr[2] += 7
    ft.update(3, 2);   // arr[3] += 2
    ft.update(4, 1);   // arr[4] += 1
    
    ft.printArray();
    
    std::cout << "Prefix sum [0,2]: " << ft.prefixSum(2) << std::endl;  // 5+3+7 = 15
    std::cout << "Range sum [1,3]: " << ft.query(1, 3) << std::endl;    // 3+7+2 = 12
    std::cout << "Single element [2]: " << ft.get(2) << std::endl;       // 7
    
    // Test 2: Initialize with array
    std::cout << "\n=== Test 2: Initialize with Array ===" << std::endl;
    std::vector<int> arr = {1, 3, 5, 7, 9, 11};
    FenwickTree ft2(arr);
    
    ft2.printArray();
    std::cout << "Sum of all elements: " << ft2.prefixSum(5) << std::endl;  // 36
    std::cout << "Sum [2,4]: " << ft2.query(2, 4) << std::endl;            // 5+7+9 = 21
    
    // Test 3: Set operation
    std::cout << "\n=== Test 3: Set Operation ===" << std::endl;
    ft2.set(2, 10);  // Change arr[2] from 5 to 10
    ft2.printArray();
    std::cout << "New sum [2,4]: " << ft2.query(2, 4) << std::endl;        // 10+7+9 = 26
    
    // Test 4: Large updates
    std::cout << "\n=== Test 4: Performance Test ===" << std::endl;
    FenwickTree ft3(1000000);
    
    // Add 1 to every position
    for (int i = 0; i < 1000000; i++) {
        ft3.update(i, 1);
    }
    
    std::cout << "Sum of first 500,000 elements: " << ft3.prefixSum(499999) << std::endl;
    std::cout << "Sum of elements [250000, 749999]: " << ft3.query(250000, 749999) << std::endl;
    
    return 0;
}

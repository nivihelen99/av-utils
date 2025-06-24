#include "slice_view.h"
#include <iostream>
#include <vector>
#include <string>
#include <array>

void print_slice(const auto& slice_view, const std::string& description) {
    std::cout << description << ": ";
    for (const auto& elem : slice_view) {
        std::cout << elem << " ";
    }
    std::cout << " (size: " << slice_view.size() << ")\n";
}

int main() {
    std::cout << "=== Unified Slicing and Negative Indexing Examples ===\n\n";
    
    // Example 1: Basic vector slicing
    std::cout << "1. Basic Vector Slicing:\n";
    std::vector<int> vec{10, 20, 30, 40, 50, 60, 70};
    
    std::cout << "Original vector: ";
    for (int x : vec) std::cout << x << " ";
    std::cout << "\n";
    
    auto last_two = slice(vec, -2);
    print_slice(last_two, "Last two elements slice(vec, -2)");
    
    auto first_three = slice(vec, 0, 3);
    print_slice(first_three, "First three slice(vec, 0, 3)");
    
    auto middle = slice(vec, 2, 5);
    print_slice(middle, "Middle slice(vec, 2, 5)");
    
    auto from_third = slice(vec, 2);
    print_slice(from_third, "From third to end slice(vec, 2)");
    
    std::cout << "\n";
    
    // Example 2: Step-based slicing
    std::cout << "2. Step-based Slicing:\n";
    
    auto every_second = slice(vec, 0, 7, 2);
    print_slice(every_second, "Every second slice(vec, 0, 7, 2)");
    
    auto every_third = slice(vec, 1, 7, 3);
    print_slice(every_third, "Every third from index 1 slice(vec, 1, 7, 3)");
    
    std::cout << "\n";
    
    // Example 3: Reverse slicing
    std::cout << "3. Reverse Slicing:\n";
    
    auto reversed_all = slice(vec, -1, -8, -1);
    print_slice(reversed_all, "Reversed all slice(vec, -1, -8, -1)");
    
    auto reversed_middle = slice(vec, 4, 1, -1);
    print_slice(reversed_middle, "Reversed middle slice(vec, 4, 1, -1)");
    
    std::cout << "\n";
    
    // Example 4: String slicing
    std::cout << "4. String Slicing:\n";
    std::string str = "Hello, World!";
    std::cout << "Original string: \"" << str << "\"\n";
    
    auto hello = slice(str, 0, 5);
    print_slice(hello, "Hello part slice(str, 0, 5)");
    
    auto world = slice(str, 7, 12);
    print_slice(world, "World part slice(str, 7, 12)");
    
    auto last_chars = slice(str, -6);
    print_slice(last_chars, "Last 6 chars slice(str, -6)");
    
    std::cout << "\n";
    
    // Example 5: Array slicing
    std::cout << "5. Array Slicing:\n";
    std::array<double, 6> arr{1.1, 2.2, 3.3, 4.4, 5.5, 6.6};
    
    std::cout << "Original array: ";
    for (double x : arr) std::cout << x << " ";
    std::cout << "\n";
    
    auto arr_slice = slice(arr, 2, -1);
    print_slice(arr_slice, "Middle elements slice(arr, 2, -1)");
    
    std::cout << "\n";
    
    // Example 6: Advanced usage with iteration
    std::cout << "6. Advanced Usage:\n";
    
    // Using range-based for loop
    std::cout << "Manual iteration over slice(vec, 1, 6, 2): ";
    for (const auto& elem : slice(vec, 1, 6, 2)) {
        std::cout << elem << " ";
    }
    std::cout << "\n";
    
    // Using iterators
    auto slice_view = slice(vec, 0, 4);
    std::cout << "Using iterators: ";
    for (auto it = slice_view.begin(); it != slice_view.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << "\n";
    
    // Using indexing
    std::cout << "Using indexing on slice: ";
    for (size_t i = 0; i < slice_view.size(); ++i) {
        std::cout << slice_view[i] << " ";
    }
    std::cout << "\n";
    
    // Front and back access
    std::cout << "Front: " << slice_view.front() << ", Back: " << slice_view.back() << "\n";
    
    std::cout << "\n";
    
    // Example 7: Mutable slicing
    std::cout << "7. Mutable Slicing:\n";
    std::vector<int> mutable_vec{1, 2, 3, 4, 5};
    std::cout << "Before modification: ";
    for (int x : mutable_vec) std::cout << x << " ";
    std::cout << "\n";
    
    // Modify through slice
    auto mut_slice = slice(mutable_vec, 1, 4);
    for (auto& elem : mut_slice) {
        elem *= 10;
    }
    
    std::cout << "After modifying slice(vec, 1, 4): ";
    for (int x : mutable_vec) std::cout << x << " ";
    std::cout << "\n";
    
    std::cout << "\n";
    
    // Example 8: Edge cases
    std::cout << "8. Edge Cases:\n";
    
    std::vector<int> small_vec{42};
    auto single_elem = slice(small_vec, -1);
    print_slice(single_elem, "Single element slice");
    
    std::vector<int> empty_vec;
    auto empty_slice = slice(empty_vec, 0, 0);
    print_slice(empty_slice, "Empty slice");
    
    auto out_of_bounds = slice(vec, 10, 20);  // Should be empty or clamped
    print_slice(out_of_bounds, "Out of bounds slice");
    
    auto full_slice = slice(vec);
    print_slice(full_slice, "Full slice slice(vec)");
    
    return 0;
}

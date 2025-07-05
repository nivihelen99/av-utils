#include "FrozenList.h"
#include <iostream>
#include <vector>
#include <string>
#include <numeric> // for std::iota

void print_line() {
    std::cout << "----------------------------------------" << std::endl;
}

template<typename T, typename Alloc>
void print_frozen_list_details(const cpp_collections::FrozenList<T, Alloc>& fl, const std::string& name) {
    std::cout << "Details for " << name << ":" << std::endl;
    std::cout << "  Size: " << fl.size() << std::endl;
    std::cout << "  Empty: " << (fl.empty() ? "yes" : "no") << std::endl;

    if (fl.empty()) {
        std::cout << "  Elements: []" << std::endl;
        return;
    }

    std::cout << "  Elements: [";
    for (size_t i = 0; i < fl.size(); ++i) {
        std::cout << fl[i] << (i == fl.size() - 1 ? "" : ", ");
    }
    std::cout << "]" << std::endl;

    std::cout << "  Front: " << fl.front() << std::endl;
    std::cout << "  Back: " << fl.back() << std::endl;

    std::cout << "  Iterating (forward): ";
    for (const auto& item : fl) {
        std::cout << item << " ";
    }
    std::cout << std::endl;

    std::cout << "  Iterating (backward using rbegin/rend): ";
    for (auto it = fl.rbegin(); it != fl.rend(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    
    try {
        std::cout << "  Element at(1) (if size > 1): " << fl.at(1) << std::endl;
    } catch (const std::out_of_range& oor) {
        std::cout << "  Element at(1): " << oor.what() << std::endl;
    }
    
    // Hash
    std::hash<cpp_collections::FrozenList<T, Alloc>> list_hasher;
    std::cout << "  Hash value: " << list_hasher(fl) << std::endl;
}

int main() {
    std::cout << "FrozenList Examples" << std::endl;
    print_line();

    // 1. Default construction
    cpp_collections::FrozenList<int> fl_empty;
    print_frozen_list_details(fl_empty, "fl_empty (default constructed)");
    print_line();

    // 2. Construction from initializer_list
    cpp_collections::FrozenList<std::string> fl_strings = {"alpha", "beta", "gamma"};
    print_frozen_list_details(fl_strings, "fl_strings (from initializer_list)");
    print_line();

    // 3. Construction from a std::vector (using iterators)
    std::vector<double> vec_doubles = {1.1, 2.2, 3.3, 4.4, 5.5};
    cpp_collections::FrozenList<double> fl_doubles(vec_doubles.begin(), vec_doubles.end());
    print_frozen_list_details(fl_doubles, "fl_doubles (from std::vector iterators)");
    print_line();

    // 4. Construction with count and value
    cpp_collections::FrozenList<char> fl_chars(5, 'x');
    print_frozen_list_details(fl_chars, "fl_chars (count and value)");
    print_line();

    // 5. Copy construction
    cpp_collections::FrozenList<std::string> fl_strings_copy(fl_strings);
    print_frozen_list_details(fl_strings_copy, "fl_strings_copy (copy of fl_strings)");
    print_line();

    // 6. Move construction
    std::vector<int> large_vec(10);
    std::iota(large_vec.begin(), large_vec.end(), 100);
    cpp_collections::FrozenList<int> fl_large_move_source(large_vec.begin(), large_vec.end());
    std::cout << "Original fl_large_move_source size: " << fl_large_move_source.size() << std::endl;
    cpp_collections::FrozenList<int> fl_moved = std::move(fl_large_move_source);
    // Note: After move, fl_large_move_source is in a valid but unspecified state.
    // Its size might be 0, or it might still hold elements if small buffer optimization applies to vector
    // or if the move constructor with custom allocator had to copy.
    // For FrozenList, the vector's move semantics apply.
    std::cout << "fl_large_move_source size after move: " << fl_large_move_source.size() << std::endl;
    print_frozen_list_details(fl_moved, "fl_moved (moved from fl_large_move_source)");
    print_line();

    // 7. Comparison
    cpp_collections::FrozenList<int> fl1 = {1, 2, 3};
    cpp_collections::FrozenList<int> fl2 = {1, 2, 3};
    cpp_collections::FrozenList<int> fl3 = {1, 2, 4};
    cpp_collections::FrozenList<int> fl4 = {1, 2};

    std::cout << "Comparisons:" << std::endl;
    std::cout << "  fl1 == fl2: " << (fl1 == fl2) << " (expected true)" << std::endl;
    std::cout << "  fl1 != fl3: " << (fl1 != fl3) << " (expected true)" << std::endl;
    std::cout << "  fl1 < fl3: " << (fl1 < fl3) << " (expected true)" << std::endl;
    std::cout << "  fl3 > fl1: " << (fl3 > fl1) << " (expected true)" << std::endl;
    std::cout << "  fl1 <= fl2: " << (fl1 <= fl2) << " (expected true)" << std::endl;
    std::cout << "  fl1 >= fl2: " << (fl1 >= fl2) << " (expected true)" << std::endl;
    std::cout << "  fl4 < fl1: " << (fl4 < fl1) << " (expected true)" << std::endl;
    print_line();
    
    // 8. Accessing data pointer
    if (!fl_doubles.empty()) {
        const double* data_ptr = fl_doubles.data();
        std::cout << "Raw data access for fl_doubles (first element): " << data_ptr[0] << std::endl;
    }
    print_line();

    // 9. Assignment
    std::cout << "Assignment examples:" << std::endl;
    cpp_collections::FrozenList<int> fl_assign1 = {10, 20};
    std::cout << "Before copy assignment (fl_assign1): ";
    for(int x : fl_assign1) std::cout << x << " "; std::cout << std::endl;
    
    cpp_collections::FrozenList<int> fl_assign2 = {30, 40, 50};
    fl_assign1 = fl_assign2; // Copy assignment
    std::cout << "After copy assignment (fl_assign1 from fl_assign2): ";
    for(int x : fl_assign1) std::cout << x << " "; std::cout << std::endl;

    cpp_collections::FrozenList<int> fl_assign3 = {60, 70, 80, 90};
    fl_assign1 = std::move(fl_assign3); // Move assignment
    std::cout << "After move assignment (fl_assign1 from fl_assign3): ";
    for(int x : fl_assign1) std::cout << x << " "; std::cout << std::endl;
    // std::cout << "fl_assign3 size after move: " << fl_assign3.size() << std::endl;

    fl_assign1 = {1, 2, 3, 4, 5}; // Initializer list assignment
    std::cout << "After initializer list assignment (fl_assign1): ";
    for(int x : fl_assign1) std::cout << x << " "; std::cout << std::endl;
    print_line();


    std::cout << "Example run complete." << std::endl;

    return 0;
}

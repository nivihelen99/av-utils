#include "small_vector.h"
#include <iostream>
#include <string>
#include <vector> // For comparison

// Helper to print vector contents and properties
template <typename T, size_t N>
void print_info(const small_vector<T, N>& vec, const std::string& name) {
    std::cout << "---- " << name << " ----\n";
    std::cout << "Size: " << vec.size() << ", Capacity: " << vec.capacity() << ", Empty: " << std::boolalpha << vec.empty() << "\n";
    std::cout << "Is inline: " << std::boolalpha << (vec.capacity() == N) << " (Note: this check is a simplification)\n"; // A more robust check would be internal
    std::cout << "Elements: ";
    if (vec.empty()) {
        std::cout << "<empty>";
    } else {
        for (size_t i = 0; i < vec.size(); ++i) {
            std::cout << vec[i] << (i == vec.size() - 1 ? "" : ", ");
        }
    }
    std::cout << "\nData pointer: " << static_cast<const void*>(vec.data()) << "\n";
    std::cout << "---------------------\n\n";
}

// A custom type to test non-trivial operations
struct MyStruct {
    int id;
    std::string name;

    MyStruct(int i, std::string s) : id(i), name(std::move(s)) {
        // std::cout << "MyStruct(" << id << ", " << name << ") constructed.\n";
    }
    MyStruct(const MyStruct& other) : id(other.id), name(other.name) {
        // std::cout << "MyStruct(" << id << ", " << name << ") copy constructed.\n";
    }
    MyStruct(MyStruct&& other) noexcept : id(other.id), name(std::move(other.name)) {
        other.id = -1; // Mark as moved
        // std::cout << "MyStruct(" << id << ", " << name << ") move constructed.\n";
    }
    MyStruct& operator=(const MyStruct& other) {
        id = other.id;
        name = other.name;
        // std::cout << "MyStruct(" << id << ", " << name << ") copy assigned.\n";
        return *this;
    }
    MyStruct& operator=(MyStruct&& other) noexcept {
        id = other.id;
        name = std::move(other.name);
        other.id = -1;
        // std::cout << "MyStruct(" << id << ", " << name << ") move assigned.\n";
        return *this;
    }
    ~MyStruct() {
        // std::cout << "MyStruct(" << id << ", " << name << ") destructed.\n";
    }

    // So it can be printed by cout
    friend std::ostream& operator<<(std::ostream& os, const MyStruct& s) {
        os << "MyStruct{id=" << s.id << ", name=\"" << s.name << "\"}";
        return os;
    }
};


int main() {
    std::cout << "===== Small Vector Examples =====\n\n";

    // 1. Basic usage with integers (inline capacity 5)
    std::cout << "1. Integers with inline capacity 5:\n";
    small_vector<int, 5> sv_int;
    print_info(sv_int, "sv_int (initial)");

    sv_int.push_back(10);
    sv_int.push_back(20);
    sv_int.push_back(30);
    print_info(sv_int, "sv_int (after 3 push_backs)");

    std::cout << "Adding two more to fill inline capacity...\n";
    sv_int.push_back(40);
    sv_int.push_back(50);
    print_info(sv_int, "sv_int (filled inline capacity)");

    std::cout << "Adding one more to trigger heap allocation...\n";
    sv_int.push_back(60);
    print_info(sv_int, "sv_int (after heap allocation)");

    sv_int.push_back(70);
    sv_int.push_back(80);
    print_info(sv_int, "sv_int (more heap elements)");

    sv_int.pop_back();
    print_info(sv_int, "sv_int (after pop_back)");

    sv_int.clear();
    print_info(sv_int, "sv_int (after clear)");
    // Note: after clear, capacity remains. If it was on heap, it stays on heap.

    // 2. Initializer list
    std::cout << "\n2. Initializer list construction:\n";
    small_vector<std::string, 3> sv_str_init = {"alpha", "beta"};
    print_info(sv_str_init, "sv_str_init (inline)");

    small_vector<std::string, 3> sv_str_init_heap = {"one", "two", "three", "four"};
    print_info(sv_str_init_heap, "sv_str_init_heap (heap)");

    // 3. Usage with non-trivial type (MyStruct)
    std::cout << "\n3. Non-trivial type (MyStruct) with inline capacity 2:\n";
    small_vector<MyStruct, 2> sv_mystruct;
    print_info(sv_mystruct, "sv_mystruct (initial)");

    sv_mystruct.emplace_back(1, "First");
    print_info(sv_mystruct, "sv_mystruct (after 1 emplace_back)");

    sv_mystruct.push_back(MyStruct(2, "Second"));
    print_info(sv_mystruct, "sv_mystruct (after 1 push_back, inline full)");

    std::cout << "Adding one more MyStruct to trigger heap allocation...\n";
    sv_mystruct.emplace_back(3, "Third");
    print_info(sv_mystruct, "sv_mystruct (after heap allocation)");

    // Test copy and move
    std::cout << "\n4. Copy and Move semantics:\n";
    small_vector<int, 4> sv_orig = {1, 2, 3};
    print_info(sv_orig, "sv_orig (inline)");

    small_vector<int, 4> sv_copy = sv_orig; // Copy constructor
    print_info(sv_copy, "sv_copy (copied from sv_orig)");
    sv_orig.push_back(4); // Modify original
    print_info(sv_orig, "sv_orig (modified after copy)");
    print_info(sv_copy, "sv_copy (should be unchanged)");

    small_vector<int, 4> sv_move_target;
    sv_move_target = std::move(sv_orig); // Move assignment
    print_info(sv_move_target, "sv_move_target (after move assign from sv_orig)");
    print_info(sv_orig, "sv_orig (after being moved from)"); // sv_orig should be empty or valid but unspecified

    small_vector<int, 2> sv_heap_source = {10, 20, 30, 40}; // Heap allocated
    print_info(sv_heap_source, "sv_heap_source (heap)");
    small_vector<int, 2> sv_heap_moved = std::move(sv_heap_source); // Move constructor
    print_info(sv_heap_moved, "sv_heap_moved (moved from sv_heap_source)");
    print_info(sv_heap_source, "sv_heap_source (after being moved from)");

    // 5. Reserve and resize
    std::cout << "\n5. Reserve and Resize:\n";
    small_vector<char, 10> sv_char;
    print_info(sv_char, "sv_char (initial)");
    sv_char.reserve(5); // Reserve within inline capacity
    print_info(sv_char, "sv_char (reserved 5 - still N=10 cap)");
    sv_char.reserve(15); // Reserve beyond inline capacity
    print_info(sv_char, "sv_char (reserved 15 - on heap)");

    sv_char.resize(7, 'a');
    print_info(sv_char, "sv_char (resized to 7 with 'a')");
    sv_char.resize(3);
    print_info(sv_char, "sv_char (resized to 3)");
    sv_char.resize(12, 'b');
    print_info(sv_char, "sv_char (resized to 12 with 'b')");


    // 6. Iteration and access
    std::cout << "\n6. Iteration and Access:\n";
    small_vector<double, 3> sv_double = {1.1, 2.2, 3.3, 4.4, 5.5};
    print_info(sv_double, "sv_double");

    std::cout << "Iterating using range-based for: ";
    for (const auto& val : sv_double) {
        std::cout << val << " ";
    }
    std::cout << "\n";

    std::cout << "Accessing elements: \n";
    if (sv_double.size() > 1) {
        std::cout << "sv_double[1] = " << sv_double[1] << "\n";
        sv_double[1] = 99.9;
        std::cout << "sv_double[1] (modified) = " << sv_double[1] << "\n";
    }
    std::cout << "Front: " << sv_double.front() << ", Back: " << sv_double.back() << "\n";
    print_info(sv_double, "sv_double (after modification)");

    // 7. Swap
    std::cout << "\n7. Swap operation:\n";
    small_vector<int, 3> sswap1 = {1,2}; // inline
    small_vector<int, 3> sswap2 = {3,4,5,6}; // heap
    print_info(sswap1, "sswap1 (before swap)");
    print_info(sswap2, "sswap2 (before swap)");
    sswap1.swap(sswap2);
    // std::swap(sswap1, sswap2); // Test std::swap specialization too
    print_info(sswap1, "sswap1 (after swap)");
    print_info(sswap2, "sswap2 (after swap)");

    small_vector<int, 3> sswap3 = {7}; // inline
    small_vector<int, 3> sswap4 = {8,9}; // inline
    print_info(sswap3, "sswap3 (before swap)");
    print_info(sswap4, "sswap4 (before swap)");
    sswap3.swap(sswap4);
    print_info(sswap3, "sswap3 (after swap)");
    print_info(sswap4, "sswap4 (after swap)");


    std::cout << "\n===== End of Small Vector Examples =====\n";
    return 0;
}

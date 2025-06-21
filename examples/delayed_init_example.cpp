#include "delayed_init.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm> // For std::sort

// A simple struct for demonstration
struct MyData {
    int id;
    std::string name;

    MyData(int i, std::string n) : id(i), name(std::move(n)) {
        std::cout << "MyData(" << id << ", " << name << ") constructed.\n";
    }
    ~MyData() {
        std::cout << "MyData(" << id << ", " << name << ") destructed.\n";
    }

    // Required for comparison examples
    bool operator==(const MyData& other) const {
        return id == other.id && name == other.name;
    }
    bool operator<(const MyData& other) const {
        if (id != other.id) {
            return id < other.id;
        }
        return name < other.name;
    }
};

std::ostream& operator<<(std::ostream& os, const MyData& data) {
    os << "MyData{id=" << data.id << ", name='" << data.name << "'}";
    return os;
}


void show_header(const std::string& title) {
    std::cout << "\n--- " << title << " ---\n";
}

template<typename T, DelayedInitPolicy P>
void print_di_state(const std::string& var_name, const DelayedInit<T,P>& di) {
    std::cout << var_name << ": is_initialized=" << di.is_initialized();
    if (di.is_initialized()) {
        std::cout << ", value=" << *di;
    }
    std::cout << std::endl;
}

int main() {
    show_header("Basic OnceOnly Policy");
    DelayedInit<int> num_once;
    print_di_state("num_once", num_once);
    num_once.init(42);
    print_di_state("num_once", num_once);
    std::cout << "Value: " << *num_once << std::endl;
    // num_once.init(100); // Would throw DelayedInitError

    DelayedInit<MyData> data_once;
    data_once.emplace(1, "TestOnce");
    print_di_state("data_once", data_once);
    std::cout << "Data name: " << data_once->name << std::endl;

    show_header("Mutable Policy");
    DelayedInitMutable<std::string> text_mutable;
    print_di_state("text_mutable", text_mutable);
    text_mutable.init("First value");
    print_di_state("text_mutable", text_mutable);
    text_mutable.init("Second value"); // OK for Mutable
    print_di_state("text_mutable", text_mutable);
    *text_mutable = "Third value"; // Using get() and assignment
    print_di_state("text_mutable", text_mutable);
    text_mutable.reset(); // Reset is allowed
    print_di_state("text_mutable", text_mutable);

    show_header("Nullable Policy");
    DelayedInitNullable<double> val_nullable;
    print_di_state("val_nullable", val_nullable);
    std::cout << "Value or default: " << val_nullable.value_or(3.14) << std::endl;
    val_nullable.init(1.23);
    print_di_state("val_nullable", val_nullable);
    std::cout << "Value or default: " << val_nullable.value_or(3.14) << std::endl;
    val_nullable.reset();
    print_di_state("val_nullable", val_nullable);

    show_header("Copy and Move Semantics");
    DelayedInit<MyData> original_data;
    original_data.emplace(10, "Original");
    print_di_state("original_data", original_data);

    DelayedInit<MyData> copied_data = original_data; // Copy constructor
    print_di_state("copied_data (from original_data)", copied_data);

    DelayedInit<MyData> moved_data = std::move(original_data); // Move constructor
    print_di_state("moved_data (from original_data)", moved_data);
    print_di_state("original_data (after move)", original_data); // original_data should be uninitialized or cleared for non-Nullable

    DelayedInit<MyData> assigned_data;
    assigned_data = moved_data; // Copy assignment
    print_di_state("assigned_data (from moved_data)", assigned_data);

    DelayedInit<MyData> move_assigned_data;
    move_assigned_data = std::move(assigned_data); // Move assignment
    print_di_state("move_assigned_data (from assigned_data)", move_assigned_data);
    print_di_state("assigned_data (after move)", assigned_data);


    show_header("Comparison Operators");
    DelayedInit<int> c1, c2, c3, c4;
    c1.init(10);
    c2.init(20);
    c3.init(10);
    // c4 remains uninitialized

    print_di_state("c1", c1);
    print_di_state("c2", c2);
    print_di_state("c3", c3);
    print_di_state("c4", c4);

    std::cout << "c1 == c1: " << (c1 == c1) << std::endl; // true
    std::cout << "c1 == c2: " << (c1 == c2) << std::endl; // false
    std::cout << "c1 == c3: " << (c1 == c3) << std::endl; // true
    std::cout << "c1 == c4: " << (c1 == c4) << std::endl; // false (initialized vs uninitialized)
    std::cout << "c4 == DelayedInit<int>(): " << (c4 == DelayedInit<int>()) << std::endl; // true (both uninitialized)

    std::cout << "c1 != c2: " << (c1 != c2) << std::endl; // true

    std::cout << "c1 < c2: " << (c1 < c2) << std::endl;   // true
    std::cout << "c2 < c1: " << (c2 < c1) << std::endl;   // false
    std::cout << "c1 < c3: " << (c1 < c3) << std::endl;   // false
    std::cout << "c1 <= c3: " << (c1 <= c3) << std::endl; // true

    std::cout << "c4 < c1: " << (c4 < c1) << std::endl;   // true (uninit < init)
    std::cout << "c1 < c4: " << (c1 < c4) << std::endl;   // false (init not < uninit)

    DelayedInit<MyData> md_comp1, md_comp2;
    md_comp1.emplace(1, "Apple");
    md_comp2.emplace(2, "Banana");
    print_di_state("md_comp1", md_comp1);
    print_di_state("md_comp2", md_comp2);
    std::cout << "md_comp1 < md_comp2: " << (md_comp1 < md_comp2) << std::endl;


    show_header("Swap Functionality");
    DelayedInit<std::string> s1, s2;
    s1.init("Hello");
    s2.init("World");
    print_di_state("s1 (before swap)", s1);
    print_di_state("s2 (before swap)", s2);
    swap(s1, s2); // Non-member swap
    print_di_state("s1 (after swap)", s1);
    print_di_state("s2 (after swap)", s2);

    DelayedInit<int> i1;
    i1.init(100);
    DelayedInit<int> i2; // Uninitialized
    print_di_state("i1 (before swap with uninit)", i1);
    print_di_state("i2 (before swap with init)", i2);
    i1.swap(i2); // Member swap
    print_di_state("i1 (after swap with uninit)", i1);
    print_di_state("i2 (after swap with init)", i2);

    DelayedInit<MyData> mds1, mds2;
    mds1.emplace(100, "SwapData1");
    // mds2 is uninitialized
    print_di_state("mds1 (before swap with uninit MyData)", mds1);
    print_di_state("mds2 (before swap with init MyData)", mds2);
    swap(mds1, mds2);
    print_di_state("mds1 (after swap with uninit MyData)", mds1);
    print_di_state("mds2 (after swap with init MyData)", mds2);


    show_header("Using in a vector and sorting");
    std::vector<DelayedInit<MyData>> vec_di;
    DelayedInit<MyData> v_d1, v_d2, v_d3, v_d4, v_d5;
    v_d1.emplace(3, "Charlie");
    v_d2.emplace(1, "Alice");
    // v_d3 is uninitialized
    v_d4.emplace(2, "Bob");
    v_d5.emplace(0, "UninitLater"); // Will be made uninit later for sorting demo

    vec_di.push_back(std::move(v_d1));
    vec_di.push_back(std::move(v_d2));
    vec_di.push_back(std::move(v_d3)); // Uninitialized
    vec_di.push_back(std::move(v_d4));
    vec_di.push_back(std::move(v_d5));

    // Forcing one to be uninitialized after adding to vector to test sorting:
    // Find "UninitLater" and reset it if it's Nullable, or re-assign an uninitialized one for OnceOnly/Mutable
    // For simplicity, let's assume we know its index or use a Nullable type for this specific demo.
    // Or, better, add an uninitialized one directly.
    // The example above for v_d3 already adds an uninitialized one.

    std::cout << "Vector before sort:\n";
    for (size_t i = 0; i < vec_di.size(); ++i) {
        std::cout << "vec_di[" << i << "]: ";
        print_di_state("", vec_di[i]);
    }

    std::sort(vec_di.begin(), vec_di.end());

    std::cout << "Vector after sort (uninitialized first, then by MyData rules):\n";
    for (size_t i = 0; i < vec_di.size(); ++i) {
        std::cout << "vec_di[" << i << "]: ";
        print_di_state("", vec_di[i]);
    }

    std::cout << "\n--- All examples finished ---\n";
    // Objects with MyData will go out of scope here, demonstrating destructors
    return 0;
}

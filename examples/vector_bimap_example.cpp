#include "vector_bimap.h" // Adjust path as necessary
#include <iostream>
#include <string>
#include <vector>

void print_line() {
    std::cout << "----------------------------------------\n";
}

int main() {
    // Create a VectorBiMap with integers to strings
    cpp_collections::VectorBiMap<int, std::string> vm;

    std::cout << "Initial state:\n";
    std::cout << "Size: " << vm.size() << (vm.empty() ? " (empty)" : " (not empty)") << std::endl;
    print_line();

    // Insert some elements
    std::cout << "Inserting elements...\n";
    vm.insert(1, "apple");
    vm.insert(2, "banana");
    vm.insert(3, "cherry");
    vm.insert(0, "date"); // Will be sorted

    std::cout << "Size after insertions: " << vm.size() << std::endl;
    print_line();

    // Iterate and print (left view - sorted by int key)
    std::cout << "Left view (sorted by integer key):\n";
    for (cpp_collections::VectorBiMap<int, std::string>::const_left_iterator it = vm.left_cbegin(); it != vm.left_cend(); ++it) {
        std::cout << it->first << " => " << it->second << std::endl;
    }
    // Or using range-based for (uses default begin/end, which is left view)
    // std::cout << "\nLeft view (using range-based for):\n";
    // for (const auto& pair : vm) {
    //     std::cout << pair.first << " => " << pair.second << std::endl;
    // }
    print_line();

    // Iterate and print (right view - sorted by string key)
    std::cout << "Right view (sorted by string key):\n";
    for (auto it = vm.right_cbegin(); it != vm.right_cend(); ++it) {
        std::cout << it->first << " => " << it->second << std::endl;
    }
    print_line();

    // Find elements
    std::cout << "Finding elements...\n";
    const std::string* val_at_1 = vm.find_left(1);
    if (val_at_1) {
        std::cout << "Value for key 1 (left): " << *val_at_1 << std::endl;
    } else {
        std::cout << "Key 1 not found (left).\n";
    }

    const int* key_for_banana = vm.find_right("banana");
    if (key_for_banana) {
        std::cout << "Key for value 'banana' (right): " << *key_for_banana << std::endl;
    } else {
        std::cout << "Value 'banana' not found (right).\n";
    }

    const std::string* val_at_5 = vm.find_left(5);
    if (val_at_5) {
        std::cout << "Value for key 5 (left): " << *val_at_5 << std::endl;
    } else {
        std::cout << "Key 5 not found (left).\n";
    }
    print_line();

    // Using at() (throws if not found)
    std::cout << "Using at()...\n";
    try {
        std::cout << "vm.at_left(3): " << vm.at_left(3) << std::endl;
        std::cout << "vm.at_right(\"apple\"): " << vm.at_right("apple") << std::endl;
        // std::cout << "vm.at_left(100): " << vm.at_left(100) << std::endl; // This would throw
    } catch (const std::out_of_range& e) {
        std::cout << "Exception caught: " << e.what() << std::endl;
    }
    print_line();

    // Contains
    std::cout << "Checking contains...\n";
    std::cout << "Contains left key 2: " << (vm.contains_left(2) ? "Yes" : "No") << std::endl;
    std::cout << "Contains right key \"date\": " << (vm.contains_right("date") ? "Yes" : "No") << std::endl;
    std::cout << "Contains left key 10: " << (vm.contains_left(10) ? "Yes" : "No") << std::endl;
    print_line();

    // Insert or assign
    std::cout << "Insert or assign demo...\n";
    std::cout << "Current value for key 1: " << vm.at_left(1) << std::endl;
    vm.insert_or_assign(1, "avocado"); // Update existing left key 1
    std::cout << "New value for key 1: " << vm.at_left(1) << std::endl;
    std::cout << "Key for 'apple' (old value for 1) should be gone: "
              << (vm.contains_right("apple") ? "Still exists!" : "Gone as expected.") << std::endl;

    vm.insert_or_assign(5, "elderberry"); // New pair
    std::cout << "Value for key 5: " << vm.at_left(5) << std::endl;

    // Assign a new left key to an existing right key
    // Current: 0 => "date"
    std::cout << "Current key for 'date': " << vm.at_right("date") << std::endl;
    vm.insert_or_assign(10, "date"); // 10 => "date", old 0 => "date" should be removed
    std::cout << "New key for 'date': " << vm.at_right("date") << std::endl;
    std::cout << "Left key 0 should be gone: "
              << (vm.contains_left(0) ? "Still exists!" : "Gone as expected.") << std::endl;

    std::cout << "Size after insert_or_assign: " << vm.size() << std::endl;
    std::cout << "Left view after insert_or_assign:\n";
    for (const auto& pair : vm) { // Uses default left begin/end
        std::cout << pair.first << " => " << pair.second << std::endl;
    }
    print_line();

    // Erase elements
    std::cout << "Erasing elements...\n";
    bool erased_2 = vm.erase_left(2); // Erase "banana"
    std::cout << "Erased key 2 (left): " << (erased_2 ? "Yes" : "No") << std::endl;

    bool erased_date = vm.erase_right("date"); // Erase 10
    std::cout << "Erased value 'date' (right): " << (erased_date ? "Yes" : "No") << std::endl;

    std::cout << "Size after erasures: " << vm.size() << std::endl;
    std::cout << "Left view after erasures:\n";
    for (const auto& pair : vm) {
        std::cout << pair.first << " => " << pair.second << std::endl;
    }
    print_line();

    // Clear
    std::cout << "Clearing the map...\n";
    vm.clear();
    std::cout << "Size after clear: " << vm.size() << (vm.empty() ? " (empty)" : " (not empty)") << std::endl;
    print_line();

    // Initializer list constructor
    cpp_collections::VectorBiMap<std::string, int> vm2 = {
        {"one", 1},
        {"two", 2},
        {"three", 3},
        {"alpha", 0}
    };
    std::cout << "VectorBiMap vm2 created with initializer list (size " << vm2.size() << "):\n";
    std::cout << "Left view (sorted by string key):\n";
    // Corrected to use default range-based for loop, which iterates the left view
    for(const auto& p : vm2) {
        std::cout << p.first << " => " << p.second << std::endl;
    }
     std::cout << "\nRight view (sorted by int key):\n";
    for(auto it = vm2.right_cbegin(); it != vm2.right_cend(); ++it) {
        std::cout << it->first << " => " << it->second << std::endl;
    }
    print_line();


    // Test with custom comparators (e.g., case-insensitive strings)
    struct CaseInsensitiveLess {
        bool operator()(const std::string& a, const std::string& b) const {
            return std::lexicographical_compare(
                a.begin(), a.end(),
                b.begin(), b.end(),
                [](char c1, char c2) {
                    return std::tolower(c1) < std::tolower(c2);
                }
            );
        }
    };

    cpp_collections::VectorBiMap<std::string, int, CaseInsensitiveLess> vm_custom_comp;
    vm_custom_comp.insert("Apple", 1);
    vm_custom_comp.insert("banana", 2); // different case from "Banana"
    vm_custom_comp.insert("Cherry", 3);
    vm_custom_comp.insert("apple", 10); // Should fail or update based on case-insensitive compare

    std::cout << "VectorBiMap with case-insensitive left keys (size " << vm_custom_comp.size() << "):\n";
    std::cout << "Value for 'apple': " << (vm_custom_comp.contains_left("apple") ? std::to_string(vm_custom_comp.at_left("apple")) : "not found") << std::endl;
    std::cout << "Value for 'APPLE': " << (vm_custom_comp.contains_left("APPLE") ? std::to_string(vm_custom_comp.at_left("APPLE")) : "not found") << std::endl;

    std::cout << "Left view (sorted case-insensitively by string key):\n";
    for(const auto& p : vm_custom_comp) {
        std::cout << p.first << " => " << p.second << std::endl;
    }
    print_line();


    return 0;
}

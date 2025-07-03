#include "deque_map.h" // Adjust path as necessary if include/ is not in default search path
#include <iostream>
#include <string>
#include <stdexcept> // For std::out_of_range

// Helper function to print the DequeMap
template<typename K, typename V, typename H, typename KE, typename A>
void print_deque_map(const std_ext::DequeMap<K, V, H, KE, A>& dm, const std::string& label) {
    std::cout << "--- " << label << " --- (Size: " << dm.size() << ")\n";
    if (dm.empty()) {
        std::cout << "(empty)\n";
    } else {
        for (const auto& pair : dm) {
            std::cout << "  {\"" << pair.first << "\": " << pair.second << "}\n";
        }
        // Show front and back if not empty
        std::cout << "  Front: {\"" << dm.front().first << "\": " << dm.front().second << "}\n";
        std::cout << "  Back:  {\"" << dm.back().first << "\": " << dm.back().second << "}\n";
    }
    std::cout << "---------------------------\n\n";
}

int main() {
    // Create a DequeMap with string keys and int values
    std_ext::DequeMap<std::string, int> my_dm;

    print_deque_map(my_dm, "Initial (Empty)");

    // 1. Add elements
    my_dm.push_back("apple", 10);
    my_dm.push_front("banana", 20);
    my_dm.push_back("cherry", 30);
    print_deque_map(my_dm, "After push_back/push_front");

    // Add using emplace_back and emplace_front (if key doesn't exist)
    auto emplace_res_back = my_dm.emplace_back("date", 40);
    if (emplace_res_back.second) {
        std::cout << "Emplaced \"date\" successfully.\n";
    }
    auto emplace_res_front = my_dm.emplace_front("elderberry", 5);
    if (emplace_res_front.second) {
        std::cout << "Emplaced \"elderberry\" successfully.\n";
    }
    print_deque_map(my_dm, "After emplace_back/emplace_front");

    // Try to emplace existing key (should fail to insert)
    auto emplace_fail = my_dm.emplace_back("apple", 100);
    if (!emplace_fail.second) {
        std::cout << "Failed to emplace \"apple\" again, as expected. Value remains: "
                  << emplace_fail.first->second << "\n";
    }
    print_deque_map(my_dm, "After trying to emplace existing key 'apple'");


    // 2. Access elements
    std::cout << "Accessing elements:\n";
    std::cout << "Value of \"apple\": " << my_dm["apple"] << std::endl;
    my_dm["apple"] = 15; // Modify using operator[]
    std::cout << "Modified value of \"apple\": " << my_dm.at("apple") << std::endl;

    // Access non-existent key with operator[] creates it
    std::cout << "Accessing \"fig\" with []: " << my_dm["fig"] << " (default initialized, then set to 0 for int)\n";
    my_dm["fig"] = 60; // Now set it properly
    print_deque_map(my_dm, "After operator[] access and modification");

    try {
        std::cout << "Value of \"grape\" (using at): " << my_dm.at("grape") << std::endl;
    } catch (const std::out_of_range& oor) {
        std::cerr << "Caught expected exception for at(\"grape\"): " << oor.what() << std::endl;
    }
    std::cout << "\n";

    // 3. Iterate (order should be: elderberry, banana, apple, cherry, date, fig)
    // Order depends on exact sequence of push_front/push_back and operator[] on new keys
    // Current expected order: elderberry, banana, apple, cherry, date, fig
    // elderberry (emplace_front)
    // banana (push_front)
    // apple (push_back)
    // cherry (push_back)
    // date (emplace_back)
    // fig (operator[] which appends)
    print_deque_map(my_dm, "Current state before removals");


    // 4. Remove elements
    if (!my_dm.empty()) {
        auto popped_front = my_dm.pop_front();
        std::cout << "Popped front: {\"" << popped_front.first << "\": " << popped_front.second << "}\n";
    }
    if (!my_dm.empty()) {
        auto popped_back = my_dm.pop_back();
        std::cout << "Popped back: {\"" << popped_back.first << "\": " << popped_back.second << "}\n";
    }
    print_deque_map(my_dm, "After pop_front and pop_back");

    // Erase by key
    size_t erased_count = my_dm.erase("apple");
    std::cout << (erased_count > 0 ? "Erased \"apple\"." : "Could not find \"apple\" to erase.") << std::endl;
    erased_count = my_dm.erase("non_existent_key");
    std::cout << (erased_count > 0 ? "Erased \"non_existent_key\"." : "Could not find \"non_existent_key\" to erase (as expected).") << std::endl;
    print_deque_map(my_dm, "After erasing 'apple'");

    // Erase by iterator
    if (!my_dm.empty()) {
        auto it_to_erase = my_dm.begin(); // Erase the first element (whatever it is now)
        if (it_to_erase != my_dm.end()) {
            std::cout << "Erasing element at begin(): {\"" << it_to_erase->first << "\": " << it_to_erase->second << "}\n";
            my_dm.erase(it_to_erase);
        }
    }
    print_deque_map(my_dm, "After erasing element at begin()");

    // 5. Check size and emptiness
    std::cout << "Final size: " << my_dm.size() << std::endl;
    std::cout << "Is empty? " << (my_dm.empty() ? "Yes" : "No") << std::endl;

    // Clear the map
    my_dm.clear();
    std::cout << "\nAfter clearing:\n";
    std::cout << "Size: " << my_dm.size() << std::endl;
    std::cout << "Is empty? " << (my_dm.empty() ? "Yes" : "No") << std::endl;
    print_deque_map(my_dm, "After clear()");

    // Test pop on empty
    try {
        my_dm.pop_front();
    } catch(const std::out_of_range& e) {
        std::cout << "Caught expected: " << e.what() << std::endl;
    }

    std_ext::DequeMap<int, std::string> dm2;
    dm2.push_back(1, "one");
    dm2.push_front(0, "zero");
    dm2[2] = "two";
    print_deque_map(dm2, "DequeMap with int keys");


    // Test initializer list constructor
    std_ext::DequeMap<std::string, int> dm_init = {
        {"first", 1},
        {"second", 2},
        {"third", 3}
    };
    print_deque_map(dm_init, "From initializer_list");
    dm_init.push_front("zeroth", 0);
    print_deque_map(dm_init, "After push_front to init list map");


    // Test range constructor
    std::vector<std::pair<const std::string, int>> vec_data = {{"vec_a", 100}, {"vec_b", 200}};
    std_ext::DequeMap<std::string, int> dm_range(vec_data.begin(), vec_data.end());
    print_deque_map(dm_range, "From range (vector)");


    std::cout << "\nExample finished.\n";
    return 0;
}

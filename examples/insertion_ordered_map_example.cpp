#include "InsertionOrderedMap.h" // Adjust path as needed
#include <iostream>
#include <string>
#include <vector>

// Helper function to print the map
template <typename K, typename V, typename H, typename KE, typename A>
void print_map(const cpp_collections::InsertionOrderedMap<K, V, H, KE, A>& map, const std::string& label) {
    std::cout << "--- " << label << " --- (Size: " << map.size() << ")\n";
    for (const auto& pair : map) {
        std::cout << "  {" << pair.first << ": " << pair.second << "}\n";
    }
    if (map.empty()) {
        std::cout << "  (empty)\n";
    }
    std::cout << "---------------------------\n\n";
}

int main() {
    std::cout << "=== InsertionOrderedMap Example ===\n\n";

    // 1. Default Construction and Basic Insertions
    cpp_collections::InsertionOrderedMap<std::string, int> map1;
    map1.insert({"apple", 10});
    map1.insert({"banana", 20});
    map1.insert({"cherry", 30});
    print_map(map1, "Map 1: After initial insertions");

    // 2. Iteration Order
    std::cout << "Iterating through Map 1 (should preserve insertion order):\n";
    for (const auto& [key, value] : map1) {
        std::cout << "Key: " << key << ", Value: " << value << std::endl;
    }
    std::cout << std::endl;

    // 3. Element Access and Update using operator[]
    std::cout << "Accessing and updating elements using []:\n";
    map1["banana"] = 25; // Update existing
    map1["date"] = 40;   // Insert new
    print_map(map1, "Map 1: After map1[\"banana\"] = 25 and map1[\"date\"] = 40");

    // 4. Element Access using at()
    std::cout << "Accessing elements using at():\n";
    try {
        std::cout << "Value of 'apple': " << map1.at("apple") << std::endl;
        // std::cout << "Value of 'fig': " << map1.at("fig") << std::endl; // This would throw
    } catch (const std::out_of_range& e) {
        std::cout << "Exception caught: " << e.what() << std::endl;
    }
    std::cout << std::endl;

    // 5. Erasure
    map1.erase("banana");
    print_map(map1, "Map 1: After erasing 'banana'");

    auto it_cherry = map1.find("cherry");
    if (it_cherry != map1.end()) {
        map1.erase(it_cherry);
        print_map(map1, "Map 1: After erasing 'cherry' by iterator");
    }

    // 6. find() and contains()
    std::cout << "Using find() and contains():\n";
    if (map1.contains("date")) {
        std::cout << "'date' is in the map. Value: " << map1.find("date")->second << std::endl;
    }
    if (!map1.contains("banana")) {
        std::cout << "'banana' is not in the map.\n";
    }
    std::cout << std::endl;

    // 7. Initializer List Constructor
    cpp_collections::InsertionOrderedMap<int, std::string> map2 = {
        {1, "one"},
        {2, "two"},
        {3, "three"}
    };
    print_map(map2, "Map 2: Constructed with initializer list");

    // 8. Range Constructor
    std::vector<std::pair<const char*, double>> vec_data = {{"pi", 3.14}, {"e", 2.71}, {"phi", 1.618}};
    cpp_collections::InsertionOrderedMap<std::string, double> map3(vec_data.begin(), vec_data.end());
    print_map(map3, "Map 3: Constructed from vector iterators");

    // 9. Copy Constructor and Assignment
    cpp_collections::InsertionOrderedMap<std::string, int> map1_copy = map1;
    print_map(map1_copy, "Map 1 Copy (from map1)");
    map1_copy.insert({"elderberry", 50});
    print_map(map1_copy, "Map 1 Copy: After adding 'elderberry'");
    print_map(map1, "Map 1: Original map1 (should be unchanged by copy's modification)");

    cpp_collections::InsertionOrderedMap<std::string, int> map1_assigned;
    map1_assigned = map1_copy;
    print_map(map1_assigned, "Map 1 Assigned (from map1_copy)");


    // 10. Special Operations: to_front, to_back, pop_front, pop_back
    cpp_collections::InsertionOrderedMap<char, int> map_special = {
        {'a', 1}, {'b', 2}, {'c', 3}, {'d', 4}, {'e', 5}
    };
    print_map(map_special, "Map Special: Initial");

    map_special.to_front('c');
    print_map(map_special, "Map Special: After to_front('c')");

    map_special.to_back('a');
    print_map(map_special, "Map Special: After to_back('a')");

    auto popped_front = map_special.pop_front();
    if (popped_front) {
        std::cout << "Popped front: {" << popped_front->first << ": " << popped_front->second << "}\n";
    }
    print_map(map_special, "Map Special: After pop_front()");

    auto popped_back = map_special.pop_back();
    if (popped_back) {
        std::cout << "Popped back: {" << popped_back->first << ": " << popped_back->second << "}\n";
    }
    print_map(map_special, "Map Special: After pop_back()");

    // 11. insert_or_assign
    print_map(map1, "Map 1: Before insert_or_assign");
    map1.insert_or_assign("apple", 111); // Assign to existing
    map1.insert_or_assign("fig", 60);   // Insert new
    print_map(map1, "Map 1: After insert_or_assign 'apple' and 'fig'");


    // 12. Clear and Empty
    std::cout << "Clearing Map 1 Copy...\n";
    map1_copy.clear();
    std::cout << "Is Map 1 Copy empty? " << std::boolalpha << map1_copy.empty() << std::endl;
    print_map(map1_copy, "Map 1 Copy: After clear()");

    std::cout << "=== Example Finished ===\n";
    return 0;
}

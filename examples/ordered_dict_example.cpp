#include "ordered_dict.h" // Adjust path as necessary
#include <iostream>
#include <string>
#include <vector>

void print_ordered_dict(const std_ext::OrderedDict<std::string, int>& od, const std::string& name) {
    std::cout << "---- " << name << " ----" << std::endl;
    if (od.empty()) {
        std::cout << "(empty)" << std::endl;
        return;
    }
    for (const auto& pair : od) {
        std::cout << "\"" << pair.first << "\": " << pair.second << std::endl;
    }
    std::cout << "Size: " << od.size() << std::endl;
    std::cout << "--------------------" << std::endl;
}

int main() {
    // 1. Default construction and insertion
    std_ext::OrderedDict<std::string, int> fruit_counts;
    std::cout << "1. Initial empty dictionary:" << std::endl;
    print_ordered_dict(fruit_counts, "fruit_counts");

    // 2. Inserting elements using operator[]
    std::cout << "\n2. Inserting with operator[]:" << std::endl;
    fruit_counts["apple"] = 5;
    fruit_counts["banana"] = 2;
    fruit_counts["orange"] = 8;
    print_ordered_dict(fruit_counts, "fruit_counts after operator[]");

    // 3. Order of iteration is preserved
    std::cout << "\n3. Iteration preserves insertion order:" << std::endl;
    // (Covered by print_ordered_dict)

    // 4. Updating an existing element (order is not changed)
    std::cout << "\n4. Updating 'apple':" << std::endl;
    fruit_counts["apple"] = 10;
    print_ordered_dict(fruit_counts, "fruit_counts after updating apple");

    // 5. Inserting a new element
    std::cout << "\n5. Inserting 'mango':" << std::endl;
    fruit_counts["mango"] = 3;
    print_ordered_dict(fruit_counts, "fruit_counts after inserting mango");

    // 6. Using insert method
    std::cout << "\n6. Using insert method for 'grape' (new) and 'banana' (existing):" << std::endl;
    auto result_grape = fruit_counts.insert({"grape", 4});
    if (result_grape.second) {
        std::cout << "'grape' inserted successfully." << std::endl;
    }
    auto result_banana = fruit_counts.insert({"banana", 100}); // Will not update
    if (!result_banana.second) {
        std::cout << "'banana' already exists, value: " << result_banana.first->second << " (not updated by insert)." << std::endl;
    }
    print_ordered_dict(fruit_counts, "fruit_counts after insert attempts");

    // 7. Using insert_or_assign
    std::cout << "\n7. Using insert_or_assign for 'pear' (new) and 'orange' (existing):" << std::endl;
    fruit_counts.insert_or_assign("pear", 6);
    fruit_counts.insert_or_assign("orange", 12); // Updates existing 'orange'
    print_ordered_dict(fruit_counts, "fruit_counts after insert_or_assign");

    // 8. Checking for existence and accessing with at()
    std::cout << "\n8. Checking existence and using at():" << std::endl;
    std::string key_to_check = "banana";
    if (fruit_counts.contains(key_to_check)) {
        std::cout << "'" << key_to_check << "' count: " << fruit_counts.at(key_to_check) << std::endl;
    }
    try {
        std::cout << "Trying to access 'coconut': " << fruit_counts.at("coconut") << std::endl;
    } catch (const std::out_of_range& e) {
        std::cout << "Caught exception for 'coconut': " << e.what() << std::endl;
    }

    // 9. Erasing elements
    std::cout << "\n9. Erasing 'apple' and 'non_existent_fruit':" << std::endl;
    fruit_counts.erase("apple");
    fruit_counts.erase("non_existent_fruit"); // Safe to call
    print_ordered_dict(fruit_counts, "fruit_counts after erasing 'apple'");

    // 10. Using popitem
    std::cout << "\n10. Using popitem():" << std::endl;
    if (!fruit_counts.empty()) {
        auto popped_last = fruit_counts.popitem(); // Pops last inserted
        std::cout << "Popped last: \"" << popped_last.first << "\": " << popped_last.second << std::endl;
        print_ordered_dict(fruit_counts, "After popitem (last)");
    }
    if (!fruit_counts.empty()) {
        auto popped_first = fruit_counts.popitem(false); // Pops first inserted
        std::cout << "Popped first: \"" << popped_first.first << "\": " << popped_first.second << std::endl;
        print_ordered_dict(fruit_counts, "After popitem (first)");
    }

    // 11. Initializer list constructor
    std::cout << "\n11. Construction from initializer list:" << std::endl;
    std_ext::OrderedDict<int, std::string> numbers = {
        {1, "one"},
        {2, "two"},
        {3, "three"},
        {2, "deux"} // Last one wins, order of '2' updated
    };
    std::cout << "---- numbers ----" << std::endl;
    for(const auto& p : numbers) {
        std::cout << p.first << ": " << p.second << std::endl;
    }
    std::cout << "--------------------" << std::endl;


    std::cout << "\nExample finished." << std::endl;
    return 0;
}

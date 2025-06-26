#include "ordered_multiset.h" // Adjust path if necessary, e.g., "../include/ordered_multiset.h"
#include <iostream>
#include <string>
#include <vector>

// Helper function to print the contents of the OrderedMultiset
template <typename T, typename Hash, typename KeyEqual>
void print_multiset(const cpp_utils::OrderedMultiset<T, Hash, KeyEqual>& oms, const std::string& label) {
    std::cout << label << " (size: " << oms.size() << "):" << std::endl;
    if (oms.empty()) {
        std::cout << "  <empty>" << std::endl;
        return;
    }
    std::cout << "  Forward: ";
    for (const auto& item : oms) {
        std::cout << item << " ";
    }
    std::cout << std::endl;

    std::cout << "  Reverse: ";
    for (auto it = oms.rbegin(); it != oms.rend(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "--- OrderedMultiset Example ---" << std::endl;

    // 1. Initialization
    cpp_utils::OrderedMultiset<std::string> shopping_list = {"milk", "bread", "apple", "milk", "orange"};
    print_multiset(shopping_list, "Initial shopping list");

    // 2. Insertion
    std::cout << "\n--- Insertion ---" << std::endl;
    shopping_list.insert("banana");
    std::cout << "Inserted 'banana'." << std::endl;
    shopping_list.insert("apple"); // Insert a duplicate apple
    std::cout << "Inserted another 'apple'." << std::endl;
    print_multiset(shopping_list, "Shopping list after insertions");

    // 3. Counting and Contains
    std::cout << "\n--- Counting and Contains ---" << std::endl;
    std::string item_to_check = "milk";
    std::cout << "Count of '" << item_to_check << "': " << shopping_list.count(item_to_check) << std::endl;
    if (shopping_list.contains(item_to_check)) {
        std::cout << "Shopping list contains '" << item_to_check << "'." << std::endl;
    }

    item_to_check = "butter";
    std::cout << "Count of '" << item_to_check << "': " << shopping_list.count(item_to_check) << std::endl;
    if (!shopping_list.contains(item_to_check)) {
        std::cout << "Shopping list does not contain '" << item_to_check << "'." << std::endl;
    }

    // 4. Erasing elements
    std::cout << "\n--- Erasing ---" << std::endl;
    // Erase one instance of "apple"
    // (The current implementation removes the one most recently added among those with the same value)
    size_t erased_count = shopping_list.erase("apple");
    std::cout << "Attempted to erase one 'apple'. Items erased: " << erased_count << std::endl;
    print_multiset(shopping_list, "After erasing one 'apple'");

    // Erase all instances of "milk"
    erased_count = shopping_list.erase_all("milk");
    std::cout << "Attempted to erase all 'milk'. Items erased: " << erased_count << std::endl;
    print_multiset(shopping_list, "After erasing all 'milk'");

    // Try to erase a non-existent item
    erased_count = shopping_list.erase("grape");
    std::cout << "Attempted to erase 'grape'. Items erased: " << erased_count << std::endl;
    print_multiset(shopping_list, "After trying to erase 'grape'");

    // 5. Clearing the multiset
    std::cout << "\n--- Clearing ---" << std::endl;
    shopping_list.clear();
    print_multiset(shopping_list, "After clearing the shopping list");
    std::cout << "Is list empty? " << (shopping_list.empty() ? "Yes" : "No") << std::endl;

    // 6. Example with integers
    std::cout << "\n--- Integer Example ---" << std::endl;
    cpp_utils::OrderedMultiset<int> event_ids;
    event_ids.insert(101);
    event_ids.insert(205);
    event_ids.insert(101);
    event_ids.insert(300);
    event_ids.insert(205);
    event_ids.insert(101);
    print_multiset(event_ids, "Event IDs");

    std::cout << "Count of event 101: " << event_ids.count(101) << std::endl;
    event_ids.erase(101);
    print_multiset(event_ids, "After erasing one 101");

    // 7. Copying and Swapping (demonstration)
    std::cout << "\n--- Copying and Swapping ---" << std::endl;
    cpp_utils::OrderedMultiset<int> event_ids_copy = event_ids;
    print_multiset(event_ids_copy, "Copied Event IDs");

    cpp_utils::OrderedMultiset<int> other_events = {99, 88};
    print_multiset(other_events, "Other Events (before swap)");
    event_ids_copy.swap(other_events);
    print_multiset(event_ids_copy, "Copied Event IDs (after swap with other_events)");
    print_multiset(other_events, "Other Events (after swap with copied_event_ids)");


    std::cout << "\n--- Example End ---" << std::endl;

    return 0;
}

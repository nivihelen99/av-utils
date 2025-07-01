#include "top_n_by_ratio_selector.h" // Adjust path if necessary
#include <iostream>
#include <string>
#include <vector>
#include <iomanip> // For std::fixed and std::setprecision

// Helper function to print selected items
template<typename ItemIDType, typename ValueType, typename CostType>
void print_selected_items(const std::string& title,
                          const std::vector<cpp_collections::ItemEntry<ItemIDType, ValueType, CostType>>& items) {
    std::cout << "--- " << title << " ---" << std::endl;
    if (items.empty()) {
        std::cout << "No items selected." << std::endl;
        return;
    }
    std::cout << std::fixed << std::setprecision(2);
    for (const auto& item : items) {
        std::cout << "ID: " << item.id
                  << ", Value: " << item.value
                  << ", Cost: " << item.cost
                  << ", Ratio: " << item.ratio
                  << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    // Instantiate the selector (string IDs, double for value and cost)
    cpp_collections::TopNByRatioSelector<std::string, double, double> selector;

    std::cout << "Initial selector size: " << selector.size() << std::endl;
    std::cout << "Is selector empty? " << (selector.empty() ? "Yes" : "No") << std::endl;
    std::cout << std::endl;

    // Add items
    std::cout << "Adding items..." << std::endl;
    selector.add_or_update_item("itemA", 10.0, 2.0); // ratio 5.0
    selector.add_or_update_item("itemB", 12.0, 3.0); // ratio 4.0
    selector.add_or_update_item("itemC", 8.0, 1.0);  // ratio 8.0
    selector.add_or_update_item("itemD", 15.0, 5.0); // ratio 3.0
    selector.add_or_update_item("itemE", 9.0, 1.5);  // ratio 6.0

    std::cout << "Selector size after adding: " << selector.size() << std::endl;
    if (selector.contains_item("itemA")) {
        auto details = selector.get_item_details("itemA");
        if (details) {
            std::cout << "Details for itemA: Value=" << details->value << ", Cost=" << details->cost << std::endl;
        }
    }
    std::cout << std::endl;

    // Test invalid cost
    try {
        std::cout << "Trying to add item with invalid cost (0)..." << std::endl;
        selector.add_or_update_item("itemF_invalid", 10.0, 0.0);
    } catch (const std::invalid_argument& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }
    try {
        std::cout << "Trying to add item with invalid cost (-1)..." << std::endl;
        selector.add_or_update_item("itemG_invalid", 10.0, -1.0);
    } catch (const std::invalid_argument& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }
    std::cout << std::endl;

    // Update an item
    std::cout << "Updating itemA (new value 12.0, new cost 4.0 -> ratio 3.0)..." << std::endl;
    selector.add_or_update_item("itemA", 12.0, 4.0); // Original ratio 5.0, new ratio 3.0

    if (selector.contains_item("itemA")) {
        auto details = selector.get_item_details("itemA");
         if (details) {
            std::cout << "Updated details for itemA: Value=" << details->value
                      << ", Cost=" << details->cost
                      << ", Ratio=" << details->ratio << std::endl;
        }
    }
    std::cout << std::endl;

    // --- Test Selector Methods ---

    // Select top N items
    print_selected_items("Top 3 items by ratio", selector.select_top_n(3));
    // Expected: itemC (8.0), itemE (6.0), itemB (4.0) - itemA now 3.0, itemD 3.0

    print_selected_items("Top 10 items by ratio (more than available)", selector.select_top_n(10));
    // Expected: All 5 items, ordered by ratio: C, E, B, A (or D), D (or A)

    print_selected_items("Top 0 items by ratio", selector.select_top_n(0));
    // Expected: No items selected.

    // Select by budget
    print_selected_items("Items selected with budget 5.0", selector.select_by_budget(5.0));
    // Expected: itemC (cost 1.0), itemE (cost 1.5). Total cost 2.5.
    // Next is itemB (cost 3.0), 2.5 + 3.0 = 5.5 > 5.0. So B is not included.
    // Or, if itemB is chosen before itemA/itemD due to tie-breaking on ID for same ratio:
    // itemC (cost 1.0, total 1.0)
    // itemE (cost 1.5, total 2.5)
    // itemB (cost 3.0, total 5.5 - too much!)
    // itemA (cost 4.0, total 6.5 - too much!)
    // itemD (cost 5.0, total 7.5 - too much!)
    // So, should be itemC, itemE.

    print_selected_items("Items selected with budget 0.0", selector.select_by_budget(0.0));
    // Expected: No items

    print_selected_items("Items selected with budget 100.0 (enough for all)", selector.select_by_budget(100.0));
    // Expected: All items, ordered by ratio.

    // Select top N by budget
    print_selected_items("Top 2 items by ratio with budget 4.0", selector.select_top_n_by_budget(2, 4.0));
    // Expected:
    // 1. itemC (value 8, cost 1, ratio 8.0). Budget left: 3.0. Count: 1.
    // 2. itemE (value 9, cost 1.5, ratio 6.0). Budget left: 1.5. Count: 2.
    // Result: itemC, itemE

    print_selected_items("Top 5 items by ratio with budget 3.0", selector.select_top_n_by_budget(5, 3.0));
    // Expected:
    // 1. itemC (cost 1.0). Budget left 2.0.
    // 2. itemE (cost 1.5). Budget left 0.5.
    // Next itemB (cost 3.0) - too expensive.
    // Result: itemC, itemE

    // Remove an item
    std::cout << "Removing itemC..." << std::endl;
    selector.remove_item("itemC");
    std::cout << "Selector size after removing itemC: " << selector.size() << std::endl;
    std::cout << "Does selector contain itemC? " << (selector.contains_item("itemC") ? "Yes" : "No") << std::endl;
    std::cout << std::endl;

    print_selected_items("Top 3 items after removing itemC", selector.select_top_n(3));
    // Expected: itemE (6.0), itemB (4.0), itemA (3.0) or itemD (3.0)

    // Clear selector
    std::cout << "Clearing selector..." << std::endl;
    selector.clear();
    std::cout << "Selector size after clearing: " << selector.size() << std::endl;
    std::cout << "Is selector empty? " << (selector.empty() ? "Yes" : "No") << std::endl;
    print_selected_items("Top 3 items after clearing", selector.select_top_n(3));


    // Example with integer IDs and values
    cpp_collections::TopNByRatioSelector<int, int, int> int_selector;
    std::cout << "\n--- Integer Selector Example ---" << std::endl;
    int_selector.add_or_update_item(1, 100, 10); // ratio 10
    int_selector.add_or_update_item(2, 150, 20); // ratio 7.5
    int_selector.add_or_update_item(3, 80, 5);   // ratio 16

    auto selected_ints = int_selector.select_top_n(2);
    print_selected_items("Top 2 ints by ratio", selected_ints);
    // Expected: ID 3 (ratio 16), ID 1 (ratio 10)

    return 0;
}

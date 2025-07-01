#include "top_n_by_ratio_selector.h" // Adjust path if necessary
#include <cassert>
#include <string>
#include <vector>
#include <iostream> // For debug prints, can be removed later
#include <algorithm> // For std::find_if
#include <iomanip>   // For std::fixed, std::setprecision in debug prints

// Helper to compare doubles with a tolerance
bool are_doubles_equal(double d1, double d2, double epsilon = 1e-9) {
    return std::abs(d1 - d2) < epsilon;
}

// Helper to print item entries for debugging
void debug_print_items(const std::string& title, const std::vector<cpp_collections::ItemEntry<std::string, double, double>>& items) {
    // std::cout << "DEBUG: --- " << title << " ---" << std::endl;
    // if (items.empty()) {
    //     std::cout << "DEBUG: No items." << std::endl;
    //     return;
    // }
    // std::cout << std::fixed << std::setprecision(2);
    // for (const auto& item : items) {
    //     std::cout << "DEBUG: ID: " << item.id
    //               << ", V: " << item.value
    //               << ", C: " << item.cost
    //               << ", R: " << item.ratio
    //               << std::endl;
    // }
    // std::cout << std::endl;
}


void test_add_update_remove() {
    std::cout << "Running test: test_add_update_remove" << std::endl;
    cpp_collections::TopNByRatioSelector<std::string, double, double> selector;

    // Add new item
    assert(selector.add_or_update_item("item1", 10.0, 2.0)); // ratio 5.0
    assert(selector.size() == 1);
    assert(selector.contains_item("item1"));
    auto details1 = selector.get_item_details("item1");
    assert(details1.has_value());
    assert(details1->id == "item1");
    assert(are_doubles_equal(details1->value, 10.0));
    assert(are_doubles_equal(details1->cost, 2.0));
    assert(are_doubles_equal(details1->ratio, 5.0));

    // Add another item
    assert(selector.add_or_update_item("item2", 12.0, 3.0)); // ratio 4.0
    assert(selector.size() == 2);

    // Update existing item (item1)
    assert(selector.add_or_update_item("item1", 20.0, 4.0)); // new ratio 5.0 (same as before, but value/cost changed)
    assert(selector.size() == 2); // Size should not change
    auto details1_updated = selector.get_item_details("item1");
    assert(details1_updated.has_value());
    assert(are_doubles_equal(details1_updated->value, 20.0));
    assert(are_doubles_equal(details1_updated->cost, 4.0));
    assert(are_doubles_equal(details1_updated->ratio, 5.0));

    // Update existing item (item1) to a new ratio
    assert(selector.add_or_update_item("item1", 15.0, 5.0)); // new ratio 3.0
    assert(selector.size() == 2);
    auto details1_reupdated = selector.get_item_details("item1");
    assert(details1_reupdated.has_value());
    assert(are_doubles_equal(details1_reupdated->ratio, 3.0));


    // Test adding item with invalid cost
    bool exception_thrown = false;
    try {
        selector.add_or_update_item("invalid_cost_item", 10.0, 0.0);
    } catch (const std::invalid_argument& e) {
        exception_thrown = true;
    }
    assert(exception_thrown);
    assert(selector.size() == 2); // Size should not change

    exception_thrown = false;
    try {
        selector.add_or_update_item("invalid_cost_item2", 10.0, -1.0);
    } catch (const std::invalid_argument& e) {
        exception_thrown = true;
    }
    assert(exception_thrown);
    assert(selector.size() == 2);


    // Remove item
    assert(selector.remove_item("item1"));
    assert(selector.size() == 1);
    assert(!selector.contains_item("item1"));
    assert(selector.get_item_details("item1") == std::nullopt);

    // Try to remove non-existent item
    assert(!selector.remove_item("non_existent_item"));
    assert(selector.size() == 1);

    // Clear
    selector.clear();
    assert(selector.size() == 0);
    assert(selector.empty());
    assert(!selector.contains_item("item2"));
    std::cout << "Passed test: test_add_update_remove" << std::endl;
}

void test_selectors() {
    std::cout << "Running test: test_selectors" << std::endl;
    cpp_collections::TopNByRatioSelector<std::string, double, double> selector;

    // Order by ratio: C (8.0), E (6.0), A (5.0), B (4.0), D (3.0)
    selector.add_or_update_item("itemA", 10.0, 2.0); // ratio 5.0
    selector.add_or_update_item("itemB", 12.0, 3.0); // ratio 4.0
    selector.add_or_update_item("itemC", 8.0, 1.0);  // ratio 8.0
    selector.add_or_update_item("itemD", 15.0, 5.0); // ratio 3.0
    selector.add_or_update_item("itemE", 9.0, 1.5);  // ratio 6.0
    assert(selector.size() == 5);

    // Test select_top_n
    auto top_n_result = selector.select_top_n(3);
    debug_print_items("Top 3", top_n_result);
    assert(top_n_result.size() == 3);
    assert(top_n_result[0].id == "itemC"); // ratio 8.0
    assert(top_n_result[1].id == "itemE"); // ratio 6.0
    assert(top_n_result[2].id == "itemA"); // ratio 5.0

    auto top_n_all = selector.select_top_n(10); // More than available
    debug_print_items("Top 10 (all)", top_n_all);
    assert(top_n_all.size() == 5);
    assert(top_n_all[0].id == "itemC");
    assert(top_n_all[1].id == "itemE");
    assert(top_n_all[2].id == "itemA");
    assert(top_n_all[3].id == "itemB");
    assert(top_n_all[4].id == "itemD");

    auto top_n_zero = selector.select_top_n(0);
    assert(top_n_zero.empty());

    // Test select_by_budget
    // Items by ratio: C(c1,r8), E(c1.5,r6), A(c2,r5), B(c3,r4), D(c5,r3)
    auto budget_result = selector.select_by_budget(5.0);
    // C (cost 1.0, total 1.0)
    // E (cost 1.5, total 2.5)
    // A (cost 2.0, total 4.5)
    // B (cost 3.0, total 7.5 - too much)
    debug_print_items("Budget 5.0", budget_result);
    assert(budget_result.size() == 3);
    assert(budget_result[0].id == "itemC");
    assert(budget_result[1].id == "itemE");
    assert(budget_result[2].id == "itemA");

    auto budget_result_small = selector.select_by_budget(2.0);
    // C (cost 1.0, total 1.0)
    // E (cost 1.5, total 2.5 - too much)
    debug_print_items("Budget 2.0", budget_result_small);
    assert(budget_result_small.size() == 1);
    assert(budget_result_small[0].id == "itemC");

    auto budget_result_zero = selector.select_by_budget(0.0);
    assert(budget_result_zero.empty());

    auto budget_result_negative = selector.select_by_budget(-1.0);
    assert(budget_result_negative.empty());

    auto budget_result_all = selector.select_by_budget(100.0); // Enough for all
    assert(budget_result_all.size() == 5); // All items should be selected


    // Test select_top_n_by_budget
    // Items by ratio: C(c1,r8), E(c1.5,r6), A(c2,r5), B(c3,r4), D(c5,r3)
    auto top_n_budget_result = selector.select_top_n_by_budget(2, 5.0);
    // C (cost 1.0, total 1.0) - count 1
    // E (cost 1.5, total 2.5) - count 2
    // Limit of 2 items reached.
    debug_print_items("Top 2 by Budget 5.0", top_n_budget_result);
    assert(top_n_budget_result.size() == 2);
    assert(top_n_budget_result[0].id == "itemC");
    assert(top_n_budget_result[1].id == "itemE");

    auto top_n_budget_result2 = selector.select_top_n_by_budget(3, 2.0);
    // C (cost 1.0, total 1.0) - count 1
    // E (cost 1.5, total 2.5 - too much for budget 2.0)
    debug_print_items("Top 3 by Budget 2.0", top_n_budget_result2);
    assert(top_n_budget_result2.size() == 1);
    assert(top_n_budget_result2[0].id == "itemC");

    auto top_n_budget_zero_n = selector.select_top_n_by_budget(0, 10.0);
    assert(top_n_budget_zero_n.empty());

    auto top_n_budget_zero_budget = selector.select_top_n_by_budget(3, 0.0);
    assert(top_n_budget_zero_budget.empty());

    // Test tie-breaking in ratio (IDs should make them unique in set)
    // Add itemF with same ratio as itemD (3.0) but different ID
    selector.add_or_update_item("itemF", 6.0, 2.0); // ratio 3.0
    assert(selector.size() == 6);

    // itemD: id "itemD", ratio 3.0
    // itemF: id "itemF", ratio 3.0
    // RatioCompare sorts by ID ascending for ties: itemD then itemF for same ratio.
    // But selection is by ratio descending.
    // Expected order for ratio 3.0 items: itemD, then itemF (if RatioCompare for set puts D before F for same ratio).
    // Let's check the full list from select_top_n
    auto top_n_with_tie = selector.select_top_n(6);
    debug_print_items("Top 6 with tie", top_n_with_tie);
    assert(top_n_with_tie.size() == 6);
    assert(top_n_with_tie[0].id == "itemC"); // 8.0
    assert(top_n_with_tie[1].id == "itemE"); // 6.0
    assert(top_n_with_tie[2].id == "itemA"); // 5.0
    assert(top_n_with_tie[3].id == "itemB"); // 4.0
    // Now for ratio 3.0 items: itemD and itemF.
    // RatioCompare sorts by ID ascending for ties. So in the std::set, for the same ratio,
    // itemD < itemF will be true.
    // Since std::set iterates in ascending order (as per its Compare function),
    // and RatioCompare sorts ratio descending (primary) then ID ascending (secondary),
    // items with higher ratio come first. For equal ratios, items with smaller ID come first.
    assert( (top_n_with_tie[4].id == "itemD" && top_n_with_tie[5].id == "itemF") ||
            (top_n_with_tie[4].id == "itemF" && top_n_with_tie[5].id == "itemD") );

    // To be more precise about tie-breaking:
    // RatioCompare { bool operator()(const Entry& a, const Entry& b) const { if (a.ratio != b.ratio) return a.ratio > b.ratio; return a.id < b.id; }}
    // This means for the set: (C,8), (E,6), (A,5), (B,4), (D,3), (F,3) -> assuming D < F
    // The iteration order of the set will be C, E, A, B, D, F. This is correct.
    assert(top_n_with_tie[4].id == "itemD");
    assert(top_n_with_tie[5].id == "itemF");


    std::cout << "Passed test: test_selectors" << std::endl;
}

void test_empty_and_clear() {
    std::cout << "Running test: test_empty_and_clear" << std::endl;
    cpp_collections::TopNByRatioSelector<int, int, int> selector;
    assert(selector.empty());
    assert(selector.size() == 0);

    auto top_n = selector.select_top_n(5);
    assert(top_n.empty());
    auto budget_sel = selector.select_by_budget(100);
    assert(budget_sel.empty());
    auto top_n_budget = selector.select_top_n_by_budget(5, 100);
    assert(top_n_budget.empty());

    selector.add_or_update_item(1, 10, 1);
    assert(!selector.empty());
    assert(selector.size() == 1);

    selector.clear();
    assert(selector.empty());
    assert(selector.size() == 0);
    std::cout << "Passed test: test_empty_and_clear" << std::endl;
}


void test_allocator_support() {
    std::cout << "Running test: test_allocator_support" << std::endl;
    // This is a basic check. A full allocator test would involve custom allocators
    // that track allocations, deallocations, etc.
    // For now, just ensure it compiles and runs with std::allocator.

    using StringItemIDSelector = cpp_collections::TopNByRatioSelector<std::string, double, double,
                                                                   std::hash<std::string>,
                                                                   std::equal_to<std::string>,
                                                                   std::allocator<char>>;
    StringItemIDSelector selector_with_alloc{std::allocator<char>()};
    selector_with_alloc.add_or_update_item("alloc_item", 100.0, 20.0);
    assert(selector_with_alloc.size() == 1);
    assert(selector_with_alloc.contains_item("alloc_item"));

    StringItemIDSelector selector_copy_constructed(selector_with_alloc);
    assert(selector_copy_constructed.size() == 1);
    assert(selector_copy_constructed.contains_item("alloc_item"));

    StringItemIDSelector selector_move_constructed(std::move(selector_with_alloc));
    assert(selector_move_constructed.size() == 1);
    assert(selector_move_constructed.contains_item("alloc_item"));
    // selector_with_alloc is now in a valid but unspecified state (likely empty)
     assert(selector_with_alloc.empty() || selector_with_alloc.size()==0);


    StringItemIDSelector selector_copy_assigned;
    selector_copy_assigned = selector_move_constructed; // Copy assign from the moved-to one
    assert(selector_copy_assigned.size() == 1);
    assert(selector_copy_assigned.contains_item("alloc_item"));

    StringItemIDSelector selector_move_assigned;
    // Re-populate selector_copy_assigned as it was the source of a copy
    selector_copy_assigned.clear();
    selector_copy_assigned.add_or_update_item("another_item", 50.0, 5.0);

    selector_move_assigned = std::move(selector_copy_assigned);
    assert(selector_move_assigned.size() == 1);
    assert(selector_move_assigned.contains_item("another_item"));
    assert(selector_copy_assigned.empty() || selector_copy_assigned.size() ==0);


    std::cout << "Passed test: test_allocator_support (basic compilation and run)" << std::endl;
}


int main() {
    test_add_update_remove();
    test_selectors();
    test_empty_and_clear();
    test_allocator_support();

    std::cout << "\nAll TopNByRatioSelector tests passed!" << std::endl;
    return 0;
}

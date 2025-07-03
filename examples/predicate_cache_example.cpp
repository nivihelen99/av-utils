#include "../include/predicate_cache.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm> // For std::for_each, std::all_of, etc.

// Example struct representing an item in a system, e.g., a document, a task, a UI element.
struct Item {
    int id;
    std::string category;
    int value; // e.g., priority, score, size
    bool is_active;

    // For std::unordered_map key requirements
    bool operator==(const Item& other) const {
        return id == other.id;
    }
};

// Hash function for Item
namespace std {
    template <>
    struct hash<Item> {
        size_t operator()(const Item& item) const {
            return std::hash<int>()(item.id);
        }
    };
}

// Helper to print item details
void print_item(const Item& item) {
    std::cout << "Item ID: " << item.id
              << ", Category: " << item.category
              << ", Value: " << item.value
              << ", Active: " << (item.is_active ? "Yes" : "No")
              << std::endl;
}

int main() {
    std::cout << "PredicateCache Example: Rule Engine / UI Filtering Simulation" << std::endl;
    std::cout << "-----------------------------------------------------------" << std::endl;

    PredicateCache<Item> item_cache;

    // --- Register Predicates (e.g., filter definitions or rules) ---
    int high_value_threshold = 500;
    PredicateId is_high_value_id = item_cache.register_predicate(
        [high_value_threshold](const Item& item) {
            std::cout << "  (Evaluating is_high_value for item " << item.id << ")" << std::endl;
            return item.value > high_value_threshold;
        }
    );

    PredicateId is_type_important_id = item_cache.register_predicate(
        [](const Item& item) {
            std::cout << "  (Evaluating is_type_important for item " << item.id << ")" << std::endl;
            return item.category == "Electronics" || item.category == "Medical";
        }
    );

    PredicateId is_currently_active_id = item_cache.register_predicate(
        [](const Item& item) {
            std::cout << "  (Evaluating is_currently_active for item " << item.id << ")" << std::endl;
            return item.is_active;
        }
    );

    // --- Sample Data ---
    std::vector<Item> all_items = {
        {1, "Books", 25, true},
        {2, "Electronics", 750, true}, // High value, Important
        {3, "Groceries", 50, false},
        {4, "Electronics", 300, true}, // Important
        {5, "Medical", 1200, true},    // High value, Important
        {6, "Books", 15, false},
        {7, "Medical", 200, false}     // Important (but inactive)
    };

    std::cout << "\n--- Initial Filtering: Find High Value & Active Electronics ---" << std::endl;
    std::vector<Item> filtered_items_pass1;
    for (const auto& item : all_items) {
        if (item_cache.evaluate(item, is_high_value_id) &&
            item_cache.evaluate(item, is_type_important_id) &&
            item.category == "Electronics" && // Additional direct check
            item_cache.evaluate(item, is_currently_active_id)) {
            filtered_items_pass1.push_back(item);
        }
    }

    std::cout << "\nResults (High Value & Active Electronics):" << std::endl;
    for (const auto& item : filtered_items_pass1) { print_item(item); }
    if (filtered_items_pass1.empty()) std::cout << "None." << std::endl;

    std::cout << "\n--- Second Filtering: Find All Important & Active Items (simulating different view/query) ---" << std::endl;
    // This time, many evaluations should be cached
    std::vector<Item> filtered_items_pass2;
    for (const auto& item : all_items) {
        if (item_cache.evaluate(item, is_type_important_id) &&
            item_cache.evaluate(item, is_currently_active_id)) {
            filtered_items_pass2.push_back(item);
        }
    }

    std::cout << "\nResults (All Important & Active Items):" << std::endl;
    for (const auto& item : filtered_items_pass2) { print_item(item); }
    if (filtered_items_pass2.empty()) std::cout << "None." << std::endl;


    // --- Simulate an update to an item and invalidation ---
    Item& item_to_update = all_items[1]; // Electronics, ID 2, Value 750
    std::cout << "\n--- Simulating update: Item ID " << item_to_update.id << " becomes inactive ---" << std::endl;
    item_to_update.is_active = false;
    item_cache.invalidate(item_to_update); // Invalidate its cached results

    std::cout << "\n--- Third Filtering: Repeat 'All Important & Active Items' after update ---" << std::endl;
    // is_currently_active_id will be re-evaluated for item 2.
    // is_type_important_id might still be cached if invalidate only clears specific predicates,
    // or re-evaluated if invalidate clears all for the object. Our current invalidate clears all.
    std::vector<Item> filtered_items_pass3;
    for (const auto& item : all_items) {
        if (item_cache.evaluate(item, is_type_important_id) &&
            item_cache.evaluate(item, is_currently_active_id)) {
            filtered_items_pass3.push_back(item);
        }
    }
    std::cout << "\nResults (All Important & Active Items after update):" << std::endl;
    for (const auto& item : filtered_items_pass3) { print_item(item); }
    if (filtered_items_pass3.empty()) std::cout << "None." << std::endl;

    std::cout << "\n--- Cache Stats ---" << std::endl;
    std::cout << "Number of items tracked in cache: " << item_cache.size() << std::endl;


    std::cout << "\n--- Prime Example: Manually setting a known state ---" << std::endl;
    Item newItem = {8, "Special", 100, true};
    all_items.push_back(newItem);
    // Let's say we know from an external source this item is 'high value' without running the predicate
    item_cache.prime(newItem, is_high_value_id, true);
    std::cout << "Primed item " << newItem.id << " for is_high_value_id as true." << std::endl;

    std::cout << "Evaluating is_high_value for item " << newItem.id << ": "
              << (item_cache.evaluate(newItem, is_high_value_id) ? "True" : "False")
              << " (Predicate function should NOT have run if prime worked)" << std::endl;


    std::cout << "\nExample finished." << std::endl;

    return 0;
}

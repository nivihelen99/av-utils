# TopNByRatioSelector

## Overview

`TopNByRatioSelector` is a C++ utility class designed to manage a collection of items, each associated with an ID, a value, and a cost. It provides efficient methods to select items based on their value/cost ratio, subject to constraints such as the number of items to select (top N) and a maximum total cost (budget).

This selector is useful in scenarios where you need to pick the most "efficient" items from a set, for example, in resource allocation problems, feature selection with cost/benefit analysis, or knapsack-like problem heuristics.

## Template Parameters

The class is templated to allow flexibility in the types used for item IDs, values, and costs:

```cpp
template<
    typename ItemIDType,
    typename ValueType = double,
    typename CostType = double,
    typename Hash = std::hash<ItemIDType>,
    typename KeyEqual = std::equal_to<ItemIDType>,
    typename Allocator = std::allocator<char> // Used to derive allocators for internal containers
>
class TopNByRatioSelector;
```

-   `ItemIDType`: The type for the unique identifier of each item (e.g., `std::string`, `int`). Must be hashable and comparable if `std::hash` and `std::equal_to` defaults are used.
-   `ValueType`: The type for the value of an item (defaults to `double`). Should be an arithmetic type.
-   `CostType`: The type for the cost of an item (defaults to `double`). Should be an arithmetic type.
    -   **Policy:** Costs must be strictly positive (`cost > 0`). Operations attempting to add/update items with non-positive costs will throw `std::invalid_argument`.
-   `Hash`: Hash function for `ItemIDType` for use in the internal `std::unordered_map` (defaults to `std::hash<ItemIDType>`).
-   `KeyEqual`: Equality comparison for `ItemIDType` for use in the internal `std::unordered_map` (defaults to `std::equal_to<ItemIDType>`).
-   `Allocator`: The allocator to be used for all internal memory allocations (defaults to `std::allocator<char>`). It will be rebound appropriately for internal containers.

## Internal Structure: `ItemEntry`

Selected items are returned as `ItemEntry` objects:

```cpp
template<typename ItemIDType, typename ValueType, typename CostType>
struct ItemEntry {
    ItemIDType id;
    ValueType value;
    CostType cost;
    double ratio; // Calculated as value / cost
};
```

## API

### Constructors

-   `explicit TopNByRatioSelector(const Hash& hash = Hash(), const KeyEqual& equal = KeyEqual(), const Allocator& alloc = Allocator())`
-   `explicit TopNByRatioSelector(const Allocator& alloc)`
    Constructs an empty selector, optionally with custom hash, key equality functions, and an allocator.

### Core Methods

-   `bool add_or_update_item(const ItemIDType& id, const ValueType& value, const CostType& cost)`
    Adds a new item or updates an existing one.
    -   `id`: Unique ID of the item.
    -   `value`: Value of the item.
    -   `cost`: Cost of the item. Must be `> 0`.
    -   Returns `true` on success.
    -   Throws `std::invalid_argument` if `cost <= 0`.

-   `bool remove_item(const ItemIDType& id)`
    Removes the item with the given `id`.
    -   Returns `true` if the item was found and removed, `false` otherwise.

-   `bool contains_item(const ItemIDType& id) const`
    Checks if an item with the given `id` exists.

-   `std::optional<item_entry_type> get_item_details(const ItemIDType& id) const`
    Retrieves the `ItemEntry` (ID, value, cost, ratio) for the given `id`.
    -   Returns `std::nullopt` if the item is not found.

-   `size_t size() const noexcept`
    Returns the number of items in the selector.

-   `bool empty() const noexcept`
    Checks if the selector is empty.

-   `void clear() noexcept`
    Removes all items from the selector.

-   `allocator_type get_allocator() const noexcept`
    Returns a copy of the allocator used by the container.

### Selector Methods

These methods return a `std::vector<item_entry_type>` containing the selected items, ordered by their value/cost ratio (highest ratio first). In case of tie in ratio, items are ordered by their `ItemIDType` (ascending) for deterministic behavior.

-   `std::vector<item_entry_type> select_top_n(size_t n) const`
    Selects the top `n` items with the highest ratios.

-   `std::vector<item_entry_type> select_by_budget(const CostType& max_total_cost) const`
    Selects items with the highest ratios whose cumulative cost does not exceed `max_total_cost`.

-   `std::vector<item_entry_type> select_top_n_by_budget(size_t n, const CostType& max_total_cost) const`
    Selects up to `n` items with the highest ratios whose cumulative cost does not exceed `max_total_cost`.

## Time Complexity

-   `add_or_update_item`: Average O(log N) due to operations on `std::set` (`sorted_items_by_ratio_`). `std::unordered_map` operations are O(1) on average.
-   `remove_item`: Average O(log N) for the same reasons.
-   `contains_item`, `get_item_details`: Average O(1) due to `std::unordered_map` lookup.
-   `size`, `empty`, `clear`: O(1) for map/set size/empty, O(N) for clear (for map/set clear).
-   Selector Methods (`select_top_n`, `select_by_budget`, `select_top_n_by_budget`):
    -   Generally O(M) or O(N) in the worst case, where M is the number of items to consider/iterate in `sorted_items_by_ratio_` up to the limit (either `n` or fitting the budget), and N is the total number of items. Copying results to the vector also takes time proportional to the number of selected items. More precisely, if K items are selected, complexity is roughly O(K + log N) if we consider initial search/iteration in the set, or O(min(N, limit)) for iteration.

## Basic Usage Example

```cpp
#include "top_n_by_ratio_selector.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

void print_items(const std::string& title, const std::vector<cpp_collections::ItemEntry<std::string, double, double>>& items) {
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
    cpp_collections::TopNByRatioSelector<std::string, double, double> selector;

    selector.add_or_update_item("itemA", 100.0, 20.0); // Ratio 5.0
    selector.add_or_update_item("itemB", 120.0, 30.0); // Ratio 4.0
    selector.add_or_update_item("itemC", 80.0, 10.0);  // Ratio 8.0
    selector.add_or_update_item("itemD", 75.0, 25.0);  // Ratio 3.0

    print_items("Top 2 items:", selector.select_top_n(2));
    // Expected: itemC (ratio 8.0), itemA (ratio 5.0)

    print_items("Items within budget 45.0:", selector.select_by_budget(45.0));
    // Expected: itemC (cost 10), itemA (cost 20). Total cost 30.
    // itemB (cost 30) would exceed budget (30+30=60).

    print_items("Top 1 item within budget 15.0:", selector.select_top_n_by_budget(1, 15.0));
    // Expected: itemC (cost 10).

    try {
        selector.add_or_update_item("invalidItem", 10.0, 0.0);
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}

```

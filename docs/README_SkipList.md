# SkipList Data Structure

## Overview

A `SkipList` is a probabilistic data structure that stores an ordered set of elements, allowing for efficient search, insertion, and deletion operations, typically with an average time complexity of O(log n), where n is the number of elements. It uses multiple levels of linked lists to achieve this performance, acting as an alternative to balanced trees like Red-Black Trees or AVL Trees.

The key idea is that each element is inserted into a base-level sorted linked list. Additionally, each element has a random chance of being included in higher-level linked lists. Searches start at the highest level, quickly skipping over large portions of the list, and then progressively drop down to lower, more detailed levels until the element is found or determined to be absent.

This implementation is a C++ template class `cpp_utils::SkipList`.

## Template Parameters

```cpp
template <typename T,
          typename Compare = std::less<T>,
          int MaxLevelParam = 16,
          double PParam = 0.5>
class SkipList { ... };
```

-   `T`: The type of elements to be stored in the SkipList. `T` must be comparable using the `Compare` functor and assignable/copyable (or movable if move semantics are fully utilized for insertion).
-   `Compare`: A functor type that provides a strict weak ordering for elements of type `T`. Defaults to `std::less<T>`, which means elements will be stored in ascending order. For descending order, `std::greater<T>` can be used.
-   `MaxLevelParam`: An integer specifying the maximum possible level for any node in the skip list (0-indexed). This effectively limits the maximum number of elements the skip list can efficiently store (roughly `(1/P)^MaxLevel`). The default is `16`. The actual number of levels in the head node will be `MaxLevelParam`.
-   `PParam`: A double representing the probability factor used to determine the level of a new node. A node at level `i` has a `PParam` chance of also being at level `i+1`. Common values are `0.5` or `0.25`. Defaults to `0.5`.

Inside the class, these are available as:
- `static constexpr int MaxLevel = MaxLevelParam;`
- `static constexpr double P = PParam;`

## Public Interface

### Constructors and Destructor
-   `SkipList()`: Constructs an empty skip list.
-   `~SkipList()`: Destroys the skip list, deallocating all stored elements.

### Basic Properties
-   `size_t size() const noexcept`: Returns the number of elements in the skip list.
-   `bool empty() const noexcept`: Returns `true` if the skip list is empty, `false` otherwise.
-   `int currentListLevel() const noexcept`: Returns the current highest level present in the skip list (0-indexed).

### Modifiers
-   `void clear() noexcept`: Removes all elements from the skip list, making it empty.
-   `bool insert(const T& value)`: Inserts `value` into the skip list. Returns `true` if insertion was successful, `false` if the value already existed (duplicates are not allowed).
-   `bool insert(T&& value)`: Inserts `value` (an r-value) into the skip list. Returns `true` if insertion was successful, `false` if the value already existed.
-   `bool erase(const T& value)`: Removes `value` from the skip list. Returns `true` if the value was found and removed, `false` otherwise.

### Lookup
-   `bool contains(const T& value) const`: Returns `true` if `value` is present in the skip list, `false` otherwise.

*(Note: Full iterator support and copy/move assignment/constructors are planned future enhancements and not detailed in this basic interface description.)*

## Usage Example

```cpp
#include <iostream>
#include "SkipList.h" // Ensure this path is correct for your setup

int main() {
    // Create a SkipList of integers
    cpp_utils::SkipList<int> sl;

    // Insert elements
    sl.insert(10);
    sl.insert(5);
    sl.insert(20);
    sl.insert(15); // Order: 5, 10, 15, 20

    std::cout << "SkipList size: " << sl.size() << std::endl; // Output: 4

    // Check for element presence
    std::cout << "Contains 10? " << (sl.contains(10) ? "Yes" : "No") << std::endl; // Output: Yes
    std::cout << "Contains 7? " << (sl.contains(7) ? "Yes" : "No") << std::endl;   // Output: No

    // Erase an element
    sl.erase(10);
    std::cout << "Contains 10 after erase? " << (sl.contains(10) ? "Yes" : "No") << std::endl; // Output: No
    std::cout << "SkipList size after erase: " << sl.size() << std::endl; // Output: 3

    // Clear the list
    sl.clear();
    std::cout << "SkipList empty after clear? " << (sl.empty() ? "Yes" : "No") << std::endl; // Output: Yes

    return 0;
}
```

### Using Custom Types and Comparators

```cpp
#include <iostream>
#include <string>
#include "SkipList.h"

struct MyData {
    int id;
    std::string name;

    // Required for default std::less or provide a custom comparator
    bool operator<(const MyData& other) const {
        return id < other.id;
    }
    // Required for contains/erase if using default equality check !(a<b || b<a)
    // Or if you want to construct MyData for lookup.
    bool operator==(const MyData& other) const { // For simple lookup in example
        return id == other.id;
    }
};

struct CompareMyDataByName {
    bool operator()(const MyData& a, const MyData& b) const {
        return a.name < b.name;
    }
};

int main() {
    cpp_utils::SkipList<MyData, CompareMyDataByName> sl_custom;

    sl_custom.insert({1, "Charlie"});
    sl_custom.insert({2, "Alice"}); // Will be ordered before Charlie by name
    sl_custom.insert({3, "Bob"});   // Will be ordered between Alice and Charlie

    std::cout << "Custom SkipList size: " << sl_custom.size() << std::endl;

    MyData search_alice = {2, "Alice"}; // ID doesn't matter for CompareMyDataByName if names differ
    std::cout << "Contains Alice? " << (sl_custom.contains(search_alice) ? "Yes" : "No") << std::endl;

    // To print elements, iterators would be needed.
    // For now, we rely on contains for verification.
    // Example: Check for Bob
    MyData search_bob = {3, "Bob"};
    if (sl_custom.contains(search_bob)) {
        std::cout << "Bob is in the list." << std::endl;
    }
}
```

## Performance

-   **Search**: Average O(log n), Worst Case O(n) (highly improbable with good randomness)
-   **Insertion**: Average O(log n), Worst Case O(n)
-   **Deletion**: Average O(log n), Worst Case O(n)
-   **Space Complexity**: Average O(n), Worst Case O(n log n) (if P is high and many nodes get promoted to many levels) or O(n * MaxLevel) more practically. With typical P=0.5, average pointers per node is 2.

The probabilistic nature means worst-case scenarios are possible but rare. `MaxLevel` should be chosen appropriately for the expected number of elements (e.g., `MaxLevel = log_1/P (N_max)`).

## Future Enhancements
-   Iterators (`begin`, `end`, `cbegin`, `cend`).
-   Copy constructor and copy assignment operator.
-   Move constructor and move assignment operator.
-   Emplace construction for elements.
-   Range insertion/construction.
-   `find()` method returning an iterator.
```

# Sliding Window Minimum/Maximum Utilities

## Overview

The `sliding_window_minmax.h` header provides C++ classes for efficiently tracking the minimum, maximum, or other "extreme" values within a sliding window of fixed capacity. As new elements are added to one end of the window and old elements are removed from the other (either explicitly via `pop()` or implicitly when `push()` is called on a full window), these utilities can report the current extreme value in O(1) time.

This is achieved by using a monotonic deque data structure internally, which ensures that push and pop operations on the window have an amortized O(1) time complexity.

The header provides:
-   `sliding_window::SlidingWindowMin<T>`: Specifically for tracking the minimum value.
-   `sliding_window::SlidingWindowMax<T>`: Specifically for tracking the maximum value.
-   `sliding_window::SlidingWindow<T, Compare>`: A generic version where the "extreme" value is determined by a custom `Compare` functor.

## Namespace
All utilities are within the `sliding_window` namespace.

## Core Technique
The classes maintain two deques:
1.  `data_`: Stores the actual elements currently in the window in FIFO order.
2.  `mono_`: A monotonic deque.
    -   For `SlidingWindowMin`, `mono_` stores elements (or indices to elements) in non-decreasing order. The minimum element of the current window is always at the front of `mono_`.
    -   For `SlidingWindowMax`, `mono_` stores elements in non-increasing order. The maximum element is at the front.
    -   For the generic `SlidingWindow`, the order in `mono_` is maintained based on the provided `Compare` functor to keep the "most extreme" element at the front.

When an element is `push`ed:
- It's added to `data_`.
- Elements are removed from the *back* of `mono_` that would violate its monotonic property with respect to the new element. Then, the new element is added to the back of `mono_`.
- If `data_` exceeds `capacity_`, `pop()` is called first.

When an element is `pop`ped from `data_` (the oldest element):
- If this oldest element was also the current extreme value (i.e., at the front of `mono_`), it's also removed from the front of `mono_`.

## Classes and Public Interface Highlights

### `sliding_window::SlidingWindowMin<T>`
Tracks the minimum element in the window.
-   **Constructor**: `explicit SlidingWindowMin(size_t capacity)`
-   **`void push(const T& value)` / `void push(T&& value)`**: Adds element; evicts oldest if full.
-   **`void pop()`**: Removes the oldest element. Throws `std::runtime_error` if empty.
-   **`const T& min() const`**: Returns current minimum. Throws `std::runtime_error` if empty. (O(1))
-   Common methods: `size()`, `empty()`, `capacity()`, `clear()`, `full()`.

### `sliding_window::SlidingWindowMax<T>`
Tracks the maximum element in the window.
-   **Constructor**: `explicit SlidingWindowMax(size_t capacity)`
-   **`void push(const T& value)` / `void push(T&& value)`**: Adds element; evicts oldest if full.
-   **`void pop()`**: Removes the oldest element. Throws `std::runtime_error` if empty.
-   **`const T& max() const`**: Returns current maximum. Throws `std::runtime_error` if empty. (O(1))
-   Common methods: `size()`, `empty()`, `capacity()`, `clear()`, `full()`.

### `sliding_window::SlidingWindow<T, Compare = std::less<T>>`
A generic version. `Compare` defines the ordering. If `Compare = std::less<T>` (default), it behaves like `SlidingWindowMin`. If `Compare = std::greater<T>`, it behaves like `SlidingWindowMax`.
-   **Constructor**: `explicit SlidingWindow(size_t capacity, Compare comp = Compare{})`
-   **`void push(const T& value)`**: Adds element.
-   **`void pop()`**: Removes oldest element.
-   **`const T& extreme() const`**: Returns the current "most extreme" element based on `Compare`.
-   Common methods: `size()`, `empty()`, `capacity()`, `clear()`, `full()`.

## Usage Examples

(Based on `examples/sliding_window_example.cpp`)

### Tracking Sliding Window Minimum

```cpp
#include "sliding_window_minmax.h"
#include <iostream>
#include <vector>

int main() {
    sliding_window::SlidingWindowMin<int> min_window(3); // Window capacity of 3

    std::vector<int> stream_data = {5, 2, 8, 1, 6, 3, 7};
    std::cout << "Input Stream: ";
    for(int x : stream_data) std::cout << x << " ";
    std::cout << std::endl;

    for (int val : stream_data) {
        min_window.push(val);
        std::cout << "Pushed " << val << " -> Window Min: " << min_window.min()
                  << " (Size: " << min_window.size() << ")" << std::endl;
    }
    // Example Output:
    // Pushed 5 -> Window Min: 5 (Size: 1)
    // Pushed 2 -> Window Min: 2 (Size: 2)
    // Pushed 8 -> Window Min: 2 (Size: 3)
    // Pushed 1 -> Window Min: 1 (Size: 3) (5 was evicted)
    // Pushed 6 -> Window Min: 1 (Size: 3) (2 was evicted)
    // Pushed 3 -> Window Min: 1 (Size: 3) (8 was evicted)
    // Pushed 7 -> Window Min: 3 (Size: 3) (1 was evicted)
}
```

### Tracking Sliding Window Maximum

```cpp
#include "sliding_window_minmax.h"
#include <iostream>
#include <vector>

int main() {
    sliding_window::SlidingWindowMax<int> max_window(4); // Window capacity of 4

    std::vector<int> stock_prices = {10, 12, 9, 15, 11, 18, 13};
    std::cout << "Stock Prices: ";
    for(int p : stock_prices) std::cout << p << " ";
    std::cout << std::endl;

    for (int price : stock_prices) {
        max_window.push(price);
        std::cout << "Pushed " << price << " -> Window Max: " << max_window.max()
                  << " (Size: " << max_window.size() << ")" << std::endl;
    }
}
```

### Generic Sliding Window with Custom Comparator

```cpp
#include "sliding_window_minmax.h"
#include <iostream>
#include <vector>
#include <string>
#include <cmath> // For std::sqrt

struct Point {
    double x, y;
    Point(double x_ = 0, double y_ = 0) : x(x_), y(y_) {}
    double distance_from_origin() const { return std::sqrt(x*x + y*y); }
    // Equality for pop comparison
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
};

// Comparator to find Point with max distance from origin
auto further_from_origin_cmp = [](const Point& a, const Point& b) {
    return a.distance_from_origin() > b.distance_from_origin();
};
// To make SlidingWindow find the max distance, the comparator for the *monotonic deque*
// should be such that if `comp(mono.back(), new_val)` is false, mono.back() is popped.
// For a max-heap like behavior (finding max distance):
// we want mono_ to be non-increasing by distance.
// `!comp_(mono_.back(), value)` -> `!(value < mono_.back())` -> `value >= mono_.back()`
// So, if `comp_` is `std::less` (on distance), then `value >= mono_.back()` removes `mono_.back()`. This is for min.
// For max distance, we need `comp_` to be `std::greater`.
// Then `!comp_(mono_.back(), value)` means `!(mono_.back() > value)` means `mono_.back() <= value`.
// This keeps `mono_` non-increasing. `extreme()` gives front, which is largest.

int main() {
    // Find point furthest from origin in a sliding window
    sliding_window::SlidingWindow<Point, decltype(further_from_origin_cmp)>
        furthest_point_window(3, further_from_origin_cmp);

    std::vector<Point> points = {{1,1}, {3,4}, {-1,0}, {2,-2}, {0,5}};

    for (const auto& p : points) {
        furthest_point_window.push(p);
        const auto& extreme_p = furthest_point_window.extreme();
        std::cout << "Pushed (" << p.x << "," << p.y << "); Furthest in window: ("
                  << extreme_p.x << "," << extreme_p.y << ") at dist "
                  << extreme_p.distance_from_origin() << std::endl;
    }
}
```

## Dependencies
- `<deque>`
- `<stdexcept>`
- `<type_traits>`
- `<chrono>` (though not directly used by the classes, often used with them)

These sliding window utilities are efficient for real-time tracking of extreme values in data streams or sequences.

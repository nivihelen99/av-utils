# SliceView Utility

The `SliceView` utility provides a flexible way to create views into existing contiguous containers like `std::vector`, `std::string`, and `std::array`. It allows for Python-like slicing, including negative indexing and step-based slicing, without copying the underlying data.

## Features

- **Non-owning views**: `SliceView` does not own the data it points to, making it lightweight.
- **Python-like slicing**: Supports `slice(container, start, stop, step)` syntax.
- **Negative indexing**: Access elements from the end of the container (e.g., -1 for the last element).
- **Step-based slicing**: Create slices with a specified step (e.g., every other element).
- **Reverse slicing**: Create slices that iterate in reverse order.
- **Mutable slices**: If the original container is mutable, the slice can be used to modify its elements.
- **Standard container interface**: Provides methods like `begin()`, `end()`, `size()`, `empty()`, `operator[]`, `front()`, `back()`.
- **Works with various containers**: Compatible with `std::vector`, `std::string`, `std::array`, and other containers that provide `.data()` and `.size()` methods and have contiguous storage.

## How to Use

Include the header file:
```cpp
#include "slice_view.h"
```

The main way to use `SliceView` is through the `slice()` helper function.

### Basic Slicing

```cpp
#include "slice_view.h"
#include <iostream>
#include <vector>
#include <string>

void print_slice(const auto& slice_view, const std::string& description) {
    std::cout << description << ": ";
    for (const auto& elem : slice_view) {
        std::cout << elem << " ";
    }
    std::cout << " (size: " << slice_view.size() << ")\n";
}

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50, 60, 70};
    std::cout << "Original vector: ";
    for (int x : vec) std::cout << x << " ";
    std::cout << "\n";

    // Get the last two elements
    auto last_two = slice(vec, -2);
    print_slice(last_two, "slice(vec, -2)"); // Output: 60 70 (size: 2)

    // Get the first three elements
    auto first_three = slice(vec, 0, 3);
    print_slice(first_three, "slice(vec, 0, 3)"); // Output: 10 20 30 (size: 3)

    // Get elements from index 2 up to (but not including) index 5
    auto middle = slice(vec, 2, 5);
    print_slice(middle, "slice(vec, 2, 5)"); // Output: 30 40 50 (size: 3)

    // Get elements from index 2 to the end
    auto from_third = slice(vec, 2);
    print_slice(from_third, "slice(vec, 2)"); // Output: 30 40 50 60 70 (size: 5)

    // Get a full slice (equivalent to the original container view)
    auto full = slice(vec);
    print_slice(full, "slice(vec)"); // Output: 10 20 30 40 50 60 70 (size: 7)
}
```

### Step-based Slicing

You can specify a step for the slice:

```cpp
    // ... (previous code) ...
    std::vector<int> vec = {10, 20, 30, 40, 50, 60, 70};

    // Get every second element starting from index 0 up to index 7
    auto every_second = slice(vec, 0, 7, 2);
    print_slice(every_second, "slice(vec, 0, 7, 2)"); // Output: 10 30 50 70 (size: 4)

    // Get every third element starting from index 1 up to index 7
    auto every_third = slice(vec, 1, 7, 3);
    print_slice(every_third, "slice(vec, 1, 7, 3)"); // Output: 20 50 (size: 2)
```

### Reverse Slicing

To create a reverse slice, provide a negative step. The `start` index should be greater than the `stop` index when using a negative step.

```cpp
    // ... (previous code) ...
    std::vector<int> vec = {10, 20, 30, 40, 50, 60, 70};

    // Reverse the entire vector
    // Starts at -1 (last element) and goes towards -8 (before the first element) with a step of -1
    auto reversed_all = slice(vec, -1, -8, -1); 
    print_slice(reversed_all, "slice(vec, -1, -8, -1)"); // Output: 70 60 50 40 30 20 10 (size: 7)

    // Reverse a sub-section: elements from index 4 down to (but not including) index 1
    auto reversed_middle = slice(vec, 4, 1, -1);
    print_slice(reversed_middle, "slice(vec, 4, 1, -1)"); // Output: 50 40 30 (size: 3)
```
**Note on reverse slicing bounds:** When `step` is negative, the slice includes elements from `data[start]`, `data[start + step]`, ..., down to the element just *before* reaching or passing `data[stop]`. `start` should be the index of the first element in the reversed sequence (higher index), and `stop` should be the index *one beyond* the last element in the reversed sequence (lower index). For `slice(vec, -1, -8, -1)`:
- `start = -1` (maps to index 6).
- `stop = -8` (maps to index -1, which is effectively one before index 0 in reverse).
- The elements are `vec[6]`, `vec[5]`, ..., `vec[0]`.

### Slicing Different Containers

`SliceView` works with `std::string` and `std::array` as well.

```cpp
    // String Slicing
    std::string str = "Hello, World!";
    auto world_part = slice(str, 7, 12);
    print_slice(world_part, "slice(str, 7, 12)"); // Output: W o r l d (size: 5)

    // Array Slicing
    std::array<double, 6> arr = {1.1, 2.2, 3.3, 4.4, 5.5, 6.6};
    auto arr_slice_middle = slice(arr, 2, -1); // Elements from index 2 to second to last
    print_slice(arr_slice_middle, "slice(arr, 2, -1)"); // Output: 3.3 4.4 5.5 (size: 3)
```

### Mutable Slices

If the original container is not const, the `SliceView` allows modification of its elements.

```cpp
    std::vector<int> mutable_vec = {1, 2, 3, 4, 5};
    std::cout << "Before modification: ";
    for (int x : mutable_vec) std::cout << x << " "; // Output: 1 2 3 4 5
    std::cout << "\n";

    auto mut_slice = slice(mutable_vec, 1, 4); // Elements at index 1, 2, 3
    for (auto& elem : mut_slice) {
        elem *= 10;
    }

    std::cout << "After modifying slice(mutable_vec, 1, 4): ";
    for (int x : mutable_vec) std::cout << x << " "; // Output: 1 20 30 40 5
    std::cout << "\n";
```

### Accessing Slice Properties and Elements

`SliceView` provides common container methods:

```cpp
    std::vector<int> vec = {10, 20, 30, 40, 50};
    auto sv = slice(vec, 1, 4); // Elements 20, 30, 40

    std::cout << "Size: " << sv.size() << std::endl;       // Output: Size: 3
    std::cout << "Is empty? " << sv.empty() << std::endl;  // Output: Is empty? 0 (false)
    std::cout << "Front: " << sv.front() << std::endl;     // Output: Front: 20
    std::cout << "Back: " << sv.back() << std::endl;       // Output: Back: 40
    std::cout << "Element at index 1: " << sv[1] << std::endl; // Output: Element at index 1: 30

    // Iteration
    std::cout << "Iterating: ";
    for (auto it = sv.begin(); it != sv.end(); ++it) {
        std::cout << *it << " "; // Output: Iterating: 20 30 40
    }
    std::cout << std::endl;

    // Raw data pointer (points to the first element of the slice in the original container)
    // int* p = sv.data(); // Note: This might not be what you expect if step != 1
                          // It gives the start of the underlying data, not a contiguous copy.
                          // The `SliceView` itself handles the step.
                          // The .data() method on SliceView returns the pointer to the first element of the slice.
    // std::cout << "Raw data pointer value (first element of slice): " << *sv.data() << std::endl; // Output: 20
```
The `data()` method of `SliceView` returns a pointer to the first element *this slice views*. The elements are not guaranteed to be contiguous in memory *if the step is not 1*.

### Edge Cases

- **Empty slice**: If `start >= stop` (for `step > 0`) or `start <= stop` (for `step < 0`), or if the original container is empty, an empty slice is created.
- **Out-of-bounds indices**: Indices are normalized. If `start` or `stop` are out of bounds, they are clamped to the valid range of the container. If this results in an invalid range (e.g., normalized `start` >= normalized `stop` for `step > 0`), an empty slice is produced.
- **Zero step**: A step of 0 is invalid and will result in an empty slice with a step of 1.

```cpp
    std::vector<int> v_edge = {1, 2, 3};
    auto empty_s = slice(v_edge, 5, 10); // start and stop out of bounds
    print_slice(empty_s, "slice(v_edge, 5, 10)"); // Output: (size: 0)

    auto also_empty = slice(v_edge, 2, 0); // start > stop with positive step
    print_slice(also_empty, "slice(v_edge, 2, 0)"); // Output: (size: 0)

    std::vector<int> empty_vec;
    auto from_empty = slice(empty_vec, 0, 0);
    print_slice(from_empty, "slice(empty_vec, 0, 0)"); // Output: (size: 0)
```

## `SliceView` Class Details

```cpp
template<typename T>
class SliceView {
public:
    // Types
    using value_type = std::remove_cv_t<T>;
    using reference = T&;
    using const_reference = const T&;
    // ... other typedefs ...

    class iterator {
        // Standard random access iterator implementation
    };

    // Constructors
    SliceView(); // Default empty slice
    SliceView(T* data, size_t size, int step = 1);

    // Iterators
    iterator begin() const;
    iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;

    // Capacity
    size_type size() const;
    bool empty() const;

    // Element access
    reference operator[](size_type idx) const;
    reference front() const;
    reference back() const;
    T* data() const; // Pointer to the first element in the slice
};
```

## Helper Functions

The `slice_util` namespace (aliased to global `slice` for convenience) provides overloaded helper functions to create `SliceView` objects:

```cpp
namespace slice_util {
    // Main slice function with start, stop, step
    template<typename Container>
    auto slice(Container& c, int start, int stop, int step = 1);

    template<typename Container>
    auto slice(const Container& c, int start, int stop, int step = 1); // Const version

    // Slice from start to end of the container
    template<typename Container>
    auto slice(Container& c, int start);

    template<typename Container>
    auto slice(const Container& c, int start); // Const version

    // Full slice (view of the entire container)
    template<typename Container>
    auto slice(Container& c);

    template<typename Container>
    auto slice(const Container& c); // Const version
}
```

These helpers handle normalization of negative indices and calculation of the correct size and data pointer for the `SliceView`.
The `slice_view.h` file also includes `using slice_util::slice;` to make the `slice` function available in the global namespace for easier use.

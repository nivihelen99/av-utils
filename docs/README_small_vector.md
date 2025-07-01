# `small_vector<T, N, Allocator>`

## Overview

`small_vector` is a sequence container similar to `std::vector` but with a significant optimization for scenarios involving many small vectors. It allocates a small buffer of `N` elements directly within the `small_vector` object itself. This is known as Small Buffer Optimization (SBO) or inline storage.

When the number of elements in the `small_vector` is less than or equal to `N`, no dynamic memory allocation (heap allocation) is performed. Elements are stored in the inline buffer. If the number of elements exceeds `N`, `small_vector` dynamically allocates memory on the heap to store its elements, just like a standard `std::vector`.

## Template Parameters

-   `T`: The type of elements stored in the vector.
-   `N`: The number of elements that can be stored inline without heap allocation. This is a `size_t` compile-time constant. `N` must be greater than 0.
-   `Allocator`: The allocator type to be used for memory management, defaulting to `std::allocator<T>`.

## Benefits

-   **Performance**: Avoids the overhead of heap allocation and deallocation for small numbers of elements. This can lead to significant performance improvements when creating and destroying many small vectors, or when locality of reference is important.
-   **Reduced Memory Fragmentation**: By avoiding frequent small heap allocations, it can help reduce memory fragmentation.
-   **Cache Locality**: Storing elements inline can improve cache performance as the elements are stored contiguously with the vector object itself (when inline).

## API Overview

`small_vector` aims to provide an API closely matching `std::vector`. Key members include:

**Constructors:**
- Default constructor
- Allocator-aware constructors
- Fill constructor (`small_vector(size_type count, const T& value, ...)` )
- Count constructor (`small_vector(size_type count, ...)` for default-insertable `T`)
- Iterator range constructor (`small_vector(InputIt first, InputIt last, ...)` )
- Copy constructor
- Move constructor
- Initializer list constructor (`small_vector(std::initializer_list<T> ilist, ...)` )

**Destructor:**
- Destroys all elements and deallocates heap memory if used.

**Assignment Operators:**
- Copy assignment (`operator=`)
- Move assignment (`operator=`)
- Initializer list assignment (`operator=`)

**Element Access:**
- `at(pos)`: Access element with bounds checking.
- `operator[](pos)`: Access element without bounds checking.
- `front()`: Access the first element.
- `back()`: Access the last element.
- `data()`: Get a direct pointer to the underlying element storage.

**Iterators:**
- `begin()`, `end()`: Get iterators to the beginning and end of the sequence.
- `cbegin()`, `cend()`: Get const iterators.
- (Reverse iterators like `rbegin`, `rend` are planned but might use `std::reverse_iterator` with basic iterators for now).

**Capacity:**
- `empty()`: Check if the vector is empty.
- `size()`: Get the number of elements.
- `max_size()`: Get the maximum possible number of elements.
- `reserve(new_cap)`: Increase capacity to at least `new_cap`.
- `capacity()`: Get the current total capacity (inline or heap).

**Modifiers:**
- `clear()`: Remove all elements.
- `push_back(value)`: Add an element to the end.
- `emplace_back(args...)`: Construct an element in-place at the end.
- `pop_back()`: Remove the last element.
- `resize(new_size)`: Change the number of elements (default-constructs or destroys).
- `resize(new_size, value)`: Change the number of elements (value-constructs or destroys).
- `swap(other)`: Exchange the contents with another `small_vector`.

**Non-Member Functions:**
- Comparison operators (`==`, `!=`, `<`, `<=`, `>`, `>=`)
- `std::swap` overload for `small_vector`.

## Usage Example

```cpp
#include "small_vector.hpp"
#include <iostream>
#include <string>

void print_status(const auto& vec, const char* name) {
    std::cout << name << ": Size=" << vec.size() << ", Capacity=" << vec.capacity();
    std::cout << ", Inline=" << std::boolalpha << (vec.capacity() == 3) << ". Elements: "; // Assuming N=3 for this example
    for(const auto& item : vec) std::cout << item << " ";
    std::cout << std::endl;
}

int main() {
    // Inline capacity of 3 for strings
    small_vector<std::string, 3> sv;
    print_status(sv, "Initial");

    sv.push_back("Hello");
    print_status(sv, "Pushed 'Hello'");

    sv.emplace_back("World");
    print_status(sv, "Emplaced 'World'");

    sv.push_back("Test");
    print_status(sv, "Pushed 'Test' (inline full)");

    // This push_back will trigger heap allocation
    sv.push_back("Overflow");
    print_status(sv, "Pushed 'Overflow' (heap allocated)");

    sv.pop_back();
    print_status(sv, "Popped 'Overflow'");

    sv.clear();
    print_status(sv, "Cleared");
    // Note: Capacity remains, and if it was on heap, it stays on heap until destruction or shrink_to_fit (if implemented).
}
```

Expected output of example:
```
Initial: Size=0, Capacity=3, Inline=true. Elements:
Pushed 'Hello': Size=1, Capacity=3, Inline=true. Elements: Hello
Emplaced 'World': Size=2, Capacity=3, Inline=true. Elements: Hello World
Pushed 'Test' (inline full): Size=3, Capacity=3, Inline=true. Elements: Hello World Test
Pushed 'Overflow' (heap allocated): Size=4, Capacity=X, Inline=false. Elements: Hello World Test Overflow  (X >= 4)
Popped 'Overflow': Size=3, Capacity=X, Inline=false. Elements: Hello World Test
Cleared: Size=0, Capacity=X, Inline=false. Elements:
```

## Performance Characteristics

-   **Best Case**: When `size() <= N`. Operations like `push_back` (if space available), `pop_back`, element access are very fast, typically O(1), without heap allocation overhead.
-   **Worst Case (Heap Allocation)**: When `size() > N`, or when an operation causes reallocation from inline to heap or heap to larger heap. Performance is similar to `std::vector`, with reallocations potentially being O(size).
-   **`N` (Inline Capacity)**: Choosing an appropriate `N` is crucial.
    -   Too small: Fewer vectors benefit from SBO.
    -   Too large: Increases `sizeof(small_vector<T, N>)`, potentially wasting stack space or increasing cache footprint for objects that don't use the full inline capacity or are empty. This could also make move operations more expensive if inline elements always need to be moved one by one.

Consider the typical number of elements your vectors will hold when choosing `N`.

## Notes

-   The implementation tries to be exception-safe, similar to `std::vector`.
-   Allocator support is included, respecting allocator propagation rules where appropriate.
-   Iterators are currently raw pointers but aim to be compatible with `std::vector` iterators. A dedicated iterator class might be added for enhanced features or safety.
-   A `shrink_to_fit()` method (like `std::vector`) is not explicitly part of the initial plan but could be added to allow users to request deallocation of unused heap capacity or to move elements back to inline storage if they fit. Currently, `pop_back` does not automatically shrink back to inline storage.
```

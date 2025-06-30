# InterningPool

## Overview

An `InterningPool<T>` is a data structure that stores and manages unique instances of objects of type `T`. When an object is "interned," the pool checks if an identical object already exists within its collection.
- If it exists, the pool returns a handle (a `const T*` pointer) to the pre-existing object.
- If it does not exist, the pool stores the new object and then returns a handle to it.

This mechanism ensures that for any given value, only one instance is stored by the pool, referenced by potentially many handles.

## Purpose and Benefits

The primary purposes and benefits of using an `InterningPool` are:

1.  **Memory Savings:** By storing only one copy of each unique object, especially for frequently repeated objects like strings, significant memory can be saved.
2.  **Faster Comparisons:** Handles (pointers) to interned objects can be compared directly for equality. If two handles are identical, the objects they point to are guaranteed to be the same. This pointer comparison is typically much faster than comparing the objects' actual values (e.g., string content comparison). This is particularly useful when objects are used as keys in maps or elements in sets, or when frequent equality checks are performed.
3.  **Centralized Management:** Provides a single point of management for the lifetime of these unique objects.

## Usage

### Include Header
```cpp
#include "interning_pool.hpp" // Adjust path as per your project structure
```

### Basic Example

```cpp
#include "interning_pool.hpp"
#include <iostream>
#include <string>

int main() {
    // Create an interning pool for strings
    cpp_collections::InterningPool<std::string> string_pool;

    // Intern some strings
    const std::string* handle1 = string_pool.intern("hello");
    const std::string* handle2 = string_pool.intern("world");
    const std::string* handle3 = string_pool.intern("hello"); // Duplicate

    // Check pool size (number of unique items)
    std::cout << "Pool size: " << string_pool.size() << std::endl; // Output: Pool size: 2

    // Compare handles
    if (handle1 == handle3) {
        std::cout << "Handles for \"hello\" (handle1 and handle3) are the same." << std::endl;
        // Output: Handles for "hello" (handle1 and handle3) are the same.
    }

    if (handle1 != handle2) {
        std::cout << "Handles for \"hello\" (handle1) and \"world\" (handle2) are different." << std::endl;
        // Output: Handles for "hello" (handle1) and "world" (handle2) are different.
    }

    // Access the interned value
    std::cout << "Value for handle1: " << *handle1 << std::endl; // Output: Value for handle1: hello

    // Intern using move semantics (for rvalues/temporaries)
    const std::string* handle4 = string_pool.intern(std::string("ephemeral"));
    const std::string* handle5 = string_pool.intern("ephemeral");
    std::cout << "Pool size after interning \"ephemeral\": " << string_pool.size() << std::endl; // Output: Pool size: 3
    if (handle4 == handle5) {
        std::cout << "Handles for \"ephemeral\" (handle4 and handle5) are the same." << std::endl;
    }
     // Output: Handles for "ephemeral" (handle4 and handle5) are the same.

    // Check if a value is contained
    if (string_pool.contains("world")) {
        std::cout << "\"world\" is in the pool." << std::endl; // Output: "world" is in the pool.
    }

    // Clear the pool (invalidates all existing handles)
    string_pool.clear();
    std::cout << "Pool size after clear: " << string_pool.size() << std::endl; // Output: Pool size after clear: 0
    // Note: handle1, handle2, handle3, handle4, handle5 are now dangling pointers.
}
```

## API

The `InterningPool<T, Hash, KeyEqual, Allocator>` class template provides the following key methods:

-   `InterningPool()`: Default constructor.
-   `InterningPool(const Allocator& alloc)`: Constructor with allocator.
-   `InterningPool(size_type bucket_count, const Hash& hash, const KeyEqual& equal, const Allocator& alloc)`: Constructor with hash parameters and allocator.
-   `InterningPool(InterningPool&& other) noexcept`: Move constructor.
-   `InterningPool& operator=(InterningPool&& other) noexcept`: Move assignment.
-   `handle_type intern(const T& value)`: Interns a copy of `value`.
-   `handle_type intern(T&& value)`: Interns `value` by moving it if it's new.
-   `bool contains(const T& value) const`: Checks if `value` is already interned.
-   `size_type size() const noexcept`: Returns the number of unique items.
-   `bool empty() const noexcept`: Checks if the pool is empty.
-   `void clear() noexcept`: Removes all items, invalidating all handles.
-   `allocator_type get_allocator() const`: Gets the allocator.
-   `hasher hash_function() const`: Gets the hash function.
-   `key_equal key_eq() const`: Gets the key equality predicate.

Copy constructor and copy assignment are deleted to prevent ambiguity with handle ownership and validity.

## Template Parameters

-   `T`: The type of objects to be interned.
-   `Hash`: A hash function for `T`. Defaults to `std::hash<T>`.
-   `KeyEqual`: An equality comparison function for `T`. Defaults to `std::equal_to<T>`.
-   `Allocator`: An allocator for `T`. Defaults to `std::allocator<T>`.

## Thread Safety

The current implementation of `InterningPool` is **not** thread-safe. If concurrent access is required, external synchronization (e.g., a mutex) must be used to protect all accesses to the pool.

## Design Choices and Limitations

-   **Handle Type:** Uses `const T*` as the handle. The lifetime of the pointed-to object is managed by the pool.
-   **Storage:** Internally uses `std::unordered_set<T, Hash, KeyEqual, Allocator>` to store the unique objects. Pointers to elements in `std::unordered_set` are stable as long as the element is not erased.
-   **No Erasure of Individual Items:** The pool does not support removing individual interned items. This is a common design choice for interning pools, as managing the validity of outstanding handles upon selective erasure is complex. The entire pool can be cleared using `clear()`, which invalidates all handles.
-   **Performance:** `intern()` operations have an average time complexity of O(1) due to the use of `std::unordered_set`, assuming a good hash function. In the worst case (many hash collisions), it can be O(N). Memory usage is O(M) where M is the sum of the sizes of the unique objects.

This `InterningPool` provides a fundamental building block for scenarios where managing unique instances of objects is beneficial.

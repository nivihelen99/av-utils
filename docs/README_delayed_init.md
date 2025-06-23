# `DelayedInit<T, Policy>`

## Overview

The `DelayedInit<T, Policy>` class (`delayed_init.h`) is a C++ utility designed for situations where an object's initialization needs to be deferred until a later point after its declaration. It provides a type-safe way to manage an object that might not have a valid value immediately upon construction, similar to having an uninitialized variable but with explicit state tracking and controlled access.

Accessing the value of a `DelayedInit` object before it has been initialized will result in a `DelayedInitError` exception. The behavior regarding re-initialization and making the object "uninitialized" again is controlled by the `DelayedInitPolicy`.

## Template Parameters

-   `T`: The type of the object whose initialization is delayed. `T` must be destructible, and either copy or move constructible.
-   `Policy`: An enum `DelayedInitPolicy` that dictates the initialization rules. Defaults to `DelayedInitPolicy::OnceOnly`.
    -   `DelayedInitPolicy::OnceOnly`: The object can be initialized only once. Subsequent calls to `init()` or `emplace()` will throw `DelayedInitError`.
    -   `DelayedInitPolicy::Mutable`: The object can be re-initialized. If `init()` or `emplace()` is called on an already initialized object, the old value is destroyed, and a new one is constructed. `reset()` is allowed.
    -   `DelayedInitPolicy::Nullable`: Similar to `Mutable`, allowing re-initialization and `reset()`. Additionally, it provides a `value_or()` method to safely get the value or a default if uninitialized.

## Features

-   **Type-Safe Deferred Initialization:** Prevents usage of uninitialized values by throwing exceptions.
-   **Policy-Based Behavior:** Customizable initialization rules (`OnceOnly`, `Mutable`, `Nullable`).
-   **RAII for Stored Object:** The destructor of `DelayedInit` ensures that the contained object `T` (if initialized) is properly destructed.
-   **In-Place Construction:** `emplace()` method allows constructing the `T` object directly within `DelayedInit`'s storage.
-   **Standard Access Patterns:** Provides `get()`, `operator*`, and `operator->` for accessing the contained value (throws if not initialized).
-   **State Checking:** `is_initialized()` and `explicit operator bool()` to check initialization status.
-   **Copy/Move Semantics:** Supports copying and moving `DelayedInit` objects. The behavior for the moved-from object depends on the policy.
-   **Comparison Operators:** `==`, `!=`, `<`, `<=`, `>`, `>=` are provided, considering both initialization state and value.
-   **Swap Functionality:** Member and non-member `swap` functions.
-   **Convenience Aliases:** `DelayedInitOnce<T>`, `DelayedInitMutable<T>`, `DelayedInitNullable<T>`.

## Exception
-   `DelayedInitError`: Derived from `std::logic_error`, thrown for invalid operations such as accessing an uninitialized value or re-initializing a `OnceOnly` instance.

## Public Interface Highlights

### Constructor & Destructor
-   **`DelayedInit()`**: Default constructor (creates an uninitialized object).
-   **`~DelayedInit()`**: Destroys the contained `T` object if it was initialized.
-   Copy/Move constructors and assignment operators.

### Initialization
-   **`void init(const T& value)`**: Initializes by copying `value`.
-   **`void init(T&& value)`**: Initializes by moving `value`.
-   **`template<typename... Args> void emplace(Args&&... args)`**: In-place constructs the value.
    *(Behavior of `init`/`emplace` on an already initialized object depends on the `Policy`)*

### Access
-   **`T& get()` / `const T& get() const`**: Get reference; throws `DelayedInitError` if not initialized.
-   **`T& operator*()` / `const T& operator*() const`**: Dereference; throws if not initialized.
-   **`T* operator->()` / `const T* operator->() const`**: Arrow operator; throws if not initialized.

### State & Policy-Specific
-   **`bool is_initialized() const noexcept`**: True if initialized.
-   **`explicit operator bool() const noexcept`**: Same as `is_initialized()`.
-   **`void reset() noexcept`**: Makes the object uninitialized (available for `Mutable` and `Nullable` policies only).
-   **`T value_or(U&& default_value) const`**: Get value or default (available for `Nullable` policy only).

### Swap & Comparison
-   **`void swap(DelayedInit& other) noexcept(...)`**: Member swap.
-   Standard comparison operators (`==`, `!=`, `<`, etc.).

## Usage Examples

(Based on `examples/delayed_init_example.cpp` and header examples)

### Basic `OnceOnly` Policy

```cpp
#include "delayed_init.h"
#include <iostream>
#include <string>

int main() {
    DelayedInit<std::string> config_value; // Default is OnceOnly

    if (!config_value.is_initialized()) {
        std::cout << "Config value is not set yet." << std::endl;
    }

    // Simulate loading configuration later
    config_value.init("Loaded Value");

    if (config_value) { // Uses operator bool()
        std::cout << "Config: " << *config_value << std::endl;
    }

    try {
        // This would throw DelayedInitError because it's OnceOnly
        // config_value.init("Attempt to overwrite");
    } catch (const DelayedInitError& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
```

### `Mutable` Policy

```cpp
#include "delayed_init.h" // Assuming DelayedInitMutable is defined via alias
#include <iostream>

int main() {
    DelayedInitMutable<int> mutable_counter;

    mutable_counter.init(10);
    std::cout << "Counter: " << *mutable_counter << std::endl; // 10

    mutable_counter.init(20); // Re-initialization is allowed
    std::cout << "Counter updated: " << *mutable_counter << std::endl; // 20

    *mutable_counter = 30; // Modify via dereference
    std::cout << "Counter modified: " << *mutable_counter << std::endl; // 30

    mutable_counter.reset(); // Allowed for Mutable
    std::cout << "Counter after reset, is_initialized: "
              << std::boolalpha << mutable_counter.is_initialized() << std::endl; // false
}
```

### `Nullable` Policy with `value_or`

```cpp
#include "delayed_init.h" // Assuming DelayedInitNullable is defined
#include <iostream>
#include <string>

int main() {
    DelayedInitNullable<std::string> user_preference;

    // Get value or a default if not set
    std::string theme = user_preference.value_or("default_theme");
    std::cout << "Initial theme: " << theme << std::endl; // "default_theme"

    user_preference.init("dark_theme");
    theme = user_preference.value_or("default_theme");
    std::cout << "Set theme: " << theme << std::endl; // "dark_theme"

    user_preference.reset();
    theme = user_preference.value_or("another_default");
    std::cout << "Theme after reset: " << theme << std::endl; // "another_default"
}
```

### Emplace Construction

```cpp
#include "delayed_init.h"
#include <iostream>
#include <string>

struct ComplexObject {
    int id;
    std::string data;
    ComplexObject(int i, std::string d) : id(i), data(std::move(d)) {
        std::cout << "ComplexObject(" << id << ", " << data << ") constructed." << std::endl;
    }
    ~ComplexObject() {
         std::cout << "ComplexObject(" << id << ", " << data << ") destructed." << std::endl;
    }
};

int main() {
    DelayedInit<ComplexObject> obj;
    // ... some logic ...
    obj.emplace(123, "Sample Data"); // Constructs ComplexObject in-place

    if (obj) {
        std::cout << "Object ID: " << obj->id << ", Data: " << obj->data << std::endl;
    }
} // obj destructor will call ~ComplexObject()
```

## Dependencies
- `<type_traits>`
- `<stdexcept>`
- `<utility>`
- `<cassert>` (used in header examples)
- `<iostream>` (used in header examples)

`DelayedInit` provides a robust and policy-driven approach to managing the lifetime and initialization of objects that cannot be initialized at their point of declaration.

# `with_resource` RAII Utility

The `with_resource` utility is a C++ template-based mechanism to manage resources using the RAII (Resource Acquisition Is Initialization) principle, inspired by Python's `with` statement. It ensures that resources are properly acquired and released, even in the presence of exceptions.

## Table of Contents
- [Overview](#overview)
- [Core Components](#core-components)
  - [`ScopedContext`](#scopedcontext)
  - [`ReturningContext`](#returningcontext)
  - [Cleanup Mechanisms](#cleanup-mechanisms)
- [Usage](#usage)
  - [`with_resource`](#with_resource)
  - [`with_resource_returning`](#with_resource_returning)
  - [Macros for Syntactic Sugar](#macros-for-syntactic-sugar)
- [Examples](#examples)
- [When to Use](#when-to-use)

## Overview

Managing resources like file handles, network connections, mutex locks, or memory allocations often involves a setup step and a cleanup step. Forgetting the cleanup or failing to perform it due to an exception can lead to resource leaks or deadlocks. `with_resource` automates this process.

You provide:
1.  A **resource** object (or a factory function that creates one).
2.  A **function** (lambda or callable) that operates on the resource.
3.  Optionally, a **custom cleanup function**.

The utility ensures that the main function is executed, and then the cleanup logic is performed when the context goes out of scope (either normally or due to an exception).

## Core Components

### `ScopedContext<Resource, Func, Cleanup>`

This class is the workhorse for `with_resource` calls that do not return a value from the main function (`Func`).
- It takes ownership of the `Resource` (usually via move semantics).
- It executes `Func` with a reference to the resource.
- In its destructor, it ensures that the `Cleanup` logic is called.
- If `Func` throws an exception, `ScopedContext` still ensures cleanup before propagating the exception.

### `ReturningContext<Resource, Func, Cleanup>`

This class is used for `with_resource_returning` calls, where the `Func` returns a value.
- Similar to `ScopedContext`, it manages the resource and cleanup.
- Additionally, it captures the return value of `Func`.
- The captured value can be retrieved using `get_result()` after the context has executed.
- If `Func` returns `void`, it behaves like `ScopedContext`.

### Cleanup Mechanisms

1.  **RAII Destructor (Default)**:
    If the `Resource` type has a destructor that performs cleanup (e.g., `std::ofstream` closes the file, `std::lock_guard` releases the mutex), you don't need to provide a custom cleanup function. `with_resource` will use `detail::NoOpCleanup`, and the resource's own destructor will be invoked when the context object (`ScopedContext` or `ReturningContext`) is destroyed.

2.  **Custom Cleanup Function**:
    You can provide a specific lambda or callable as the `Cleanup` argument. This function will be called with a reference to the resource before the context is destroyed. This is useful for resources that don't have RAII semantics or require specific cleanup steps.
    *Important*: The custom cleanup function is called *before* the `Resource` object itself is destroyed. If the `Resource` also has a destructor with side effects, both will run.

## Usage

The primary interface consists of two function templates: `with_resource` and `with_resource_returning`.

### `with_resource`

Used when the operation on the resource does not produce a return value that needs to be captured (or returns `void`).

**Signatures:**

```cpp
// Uses RAII destructor of Resource for cleanup
template<typename Resource, typename Func>
void with_resource(Resource&& resource, Func&& func);

// Uses a custom cleanup function
template<typename Resource, typename Func, typename Cleanup>
void with_resource(Resource&& resource, Func&& func, Cleanup&& cleanup);
```

**Parameters:**
- `resource`: An rvalue reference to the resource to be managed. It will be moved into the context. This can be an object or the result of a function call that produces a resource.
- `func`: A callable (e.g., lambda) that takes a reference to the `Resource` (e.g., `auto& res`). This is where you perform operations with the resource.
- `cleanup`: (Optional) A callable that takes a reference to the `Resource`. This is executed after `func` completes or if an exception occurs in `func`.

### `with_resource_returning`

Used when the operation on the resource produces a value that you want to use afterwards.

**Signatures:**

```cpp
// Uses RAII destructor of Resource for cleanup
template<typename Resource, typename Func>
auto with_resource_returning(Resource&& resource, Func&& func);

// Uses a custom cleanup function
template<typename Resource, typename Func, typename Cleanup>
auto with_resource_returning(Resource&& resource, Func&& func, Cleanup&& cleanup);
```

**Parameters:** Same as `with_resource`.
**Return Value:** The value returned by `func`. If `func` returns `void`, `with_resource_returning` also effectively returns `void` (though its internal structure handles this to avoid errors).

### Macros for Syntactic Sugar

To make the syntax more concise, two helper macros are provided:

1.  `WITH_RESOURCE(resource_expression, variable_name)`
    - `resource_expression`: The expression that yields the resource (e.g., `std::ofstream("file.txt")`).
    - `variable_name`: The name you want to use for the resource within the subsequent lambda block.
    - This macro sets up a `with_resource` call using the resource's RAII destructor for cleanup.

    Example:
    ```cpp
    WITH_RESOURCE(std::ofstream("output.txt"), my_file) {
        // my_file is a reference to the std::ofstream object
        my_file << "Hello via macro!" << std::endl;
    }); // File is closed here
    ```

2.  `WITH_RESOURCE_CLEANUP(resource_expression, variable_name, cleanup_lambda)`
    - `resource_expression`: Same as above.
    - `variable_name`: Same as above.
    - `cleanup_lambda`: The custom cleanup lambda.

    Example:
    ```cpp
    struct MyResource { int data; };
    MyResource res {123};
    bool cleaned = false;

    WITH_RESOURCE_CLEANUP(std::move(res), r_alias, [&](MyResource& r_clean) {
        std::cout << "Cleaning resource with data: " << r_clean.data << std::endl;
        cleaned = true;
    }) {
        std::cout << "Using resource with data: " << r_alias.data << std::endl;
    });
    // cleaned is true here
    ```

## Examples

For detailed examples, please refer to `examples/with_resource_example.cpp`. Here are a few snippets:

**File Handling (RAII):**
```cpp
raii::with_resource(std::ofstream("test.txt"), [](auto& file) {
    file << "Hello, RAII!" << std::endl;
}); // std::ofstream destructor closes the file
```

**Mutex Locking (RAII with std::lock_guard):**
```cpp
std::mutex mtx;
raii::with_resource(std::lock_guard<std::mutex>(mtx), [](auto& lock) {
    // Critical section: mtx is locked
    std::cout << "Mutex is locked." << std::endl;
}); // lock_guard destructor unlocks mtx
```
Alternatively, for more control (e.g. try_lock or explicit unlock before scope end), use `std::unique_lock` and a custom cleanup if needed, or rely on `unique_lock`'s RAII behavior.

**Custom Cleanup:**
```cpp
struct NonRaiiResource {
    FILE* fp = nullptr;
    NonRaiiResource(const char* name, const char* mode) : fp(fopen(name, mode)) {}
    // No destructor to close fp
};

raii::with_resource(
    NonRaiiResource("data.txt", "w"),
    [](NonRaiiResource& res) {
        if (res.fp) {
            fputs("Writing with custom cleanup.\n", res.fp);
        }
    },
    [](NonRaiiResource& res) {
        if (res.fp) {
            fclose(res.fp);
            std::cout << "data.txt closed by custom cleanup." << std::endl;
        }
    }
);
```

**Returning a Value:**
```cpp
int sum = raii::with_resource_returning(std::vector<int>{1, 2, 3}, [](auto& vec) {
    return std::accumulate(vec.begin(), vec.end(), 0);
});
// sum is 6
```

## When to Use

- When you need to ensure a resource is cleaned up regardless of how a scope is exited (normal completion or exception).
- To simplify resource management code and make it more readable.
- For resources that don't naturally follow RAII or require complex cleanup logic that doesn't fit well into a destructor.
- To emulate Python's `with` statement for consistent resource handling patterns.
```

# Context Management Utilities (`context_mgr.h`)

## Overview

The `context_mgr.h` header provides a collection of C++ utilities for Resource Acquisition Is Initialization (RAII). These tools help manage resources and execute cleanup code automatically when a scope is exited, regardless of whether the exit is normal or due to an exception. This promotes safer, cleaner, and more robust code, similar to Python's `with` statement or Go's `defer` keyword.

The primary components are:
- `ContextManager`: For paired enter/exit actions.
- `ScopeExit`: For deferred cleanup actions (scope guard).
- `NamedScope`: For logging scope entry/exit.
- `ThreadLocalOverride`: For temporarily overriding variable values within a scope.
- Helper factory functions and a macro for ease of use.

All context managers provided are move-only to ensure clear ownership and prevent accidental copying that might lead to double execution of cleanup logic.

## Core Utilities

### `context::ContextManager<FEnter, FExit>`
-   A generic context manager that executes an `enter` function upon construction and an `exit` function upon destruction.
-   **Constructor**: `ContextManager(FEnter enter, FExit exit)`
-   **`cancel()`**: Prevents the `exit` function from being called during destruction.
-   **`is_active()`**: Checks if the `exit` function is still scheduled.
-   **Factory**: `context::make_context(FEnter&& enter, FExit&& exit)` for type deduction.

### `context::ScopeExit<F>`
-   A scope guard that executes a given function `F` upon destruction.
-   **Constructor**: `explicit ScopeExit(F exit_func)`
-   **`dismiss()`**: Prevents the `exit_func` from being called during destruction.
-   **`is_active()`**: Checks if the `exit_func` is still scheduled.
-   **Factory**: `context::make_scope_exit(F&& exit_func)` for type deduction.
-   **Macro**: `SCOPE_EXIT({ /* lambda body for cleanup */ });` for concise usage.

### `context::NamedScope`
-   A simple RAII class that logs entry and exit from a named scope to `std::cout`.
-   **Constructor**: `explicit NamedScope(const std::string& name)`

### `context::ThreadLocalOverride<T>`
-   Temporarily overrides the value of a variable `T&` within a scope. The original value is restored upon destruction.
-   **Constructor**: `ThreadLocalOverride(T& variable, T new_value)`
-   **Factory**: `context::make_override(T& variable, T new_value)` for type deduction.

## Features
- **RAII Principle**: Automates resource cleanup and paired actions.
- **Exception Safety**: Cleanup functions are executed even if exceptions are thrown within the scope.
- **Move-Only Semantics**: Prevents accidental copying and ensures clear ownership.
- **Type Deduction**: Factory functions (`make_context`, `make_scope_exit`, `make_override`) simplify instantiation.
- **Convenience Macro**: `SCOPE_EXIT` provides a very concise way to define deferred cleanup.
- **Customizable Actions**: Accepts any callable (lambdas, function pointers, functors) for enter/exit logic.

## Usage Examples

(Based on `examples/context_mgr_example.cpp`)

### `ScopeExit` and `SCOPE_EXIT` Macro (Deferred Cleanup)

```cpp
#include "context_mgr.h"
#include <iostream>
#include <fstream> // For FILE* example

void file_example() {
    const char* filename = "temp.txt";
    FILE* f = fopen(filename, "w");
    if (!f) {
        std::cerr << "Failed to open file!" << std::endl;
        return;
    }

    // Ensure file is closed using make_scope_exit
    auto close_file_guard = context::make_scope_exit([f, filename]() {
        fclose(f);
        std::cout << "File '" << filename << "' closed by make_scope_exit." << std::endl;
    });

    fprintf(f, "Hello from ScopeExit!\n");
    // File will be closed automatically here.
}

void macro_example() {
    std::cout << "Entering macro_example function." << std::endl;

    SCOPE_EXIT({
        std::cout << "Cleanup action from SCOPE_EXIT macro executed." << std::endl;
    });

    std::cout << "Performing operations within macro_example..." << std::endl;
    // SCOPE_EXIT lambda will execute when this scope ends.
}
```

### `ContextManager` (Paired Enter/Exit Actions)

```cpp
#include "context_mgr.h"
#include <iostream>

int g_indent = 0; // Global for demo

void indent_example() {
    auto indent_manager = context::make_context(
        [] {
            ++g_indent;
            std::cout << std::string(g_indent * 2, ' ') << "Enter scope, indent = " << g_indent << std::endl;
        },
        [] {
            std::cout << std::string(g_indent * 2, ' ') << "Exit scope, indent was " << g_indent << std::endl;
            --g_indent;
        }
    );

    std::cout << std::string(g_indent * 2, ' ') << "Inside managed scope." << std::endl;
    // Indent level restored automatically.
}
```

### `NamedScope` (Logging Scope Entry/Exit)

```cpp
#include "context_mgr.h"
#include <iostream>

void named_scope_example() {
    context::NamedScope outer("OuterTask");
    std::cout << "  Doing work in OuterTask...\n";
    {
        context::NamedScope inner("InnerTask");
        std::cout << "    Doing work in InnerTask...\n";
    } // InnerTask exits here
    std::cout << "  Resuming work in OuterTask...\n";
} // OuterTask exits here
```

### `ThreadLocalOverride` (Temporary Variable Change)

```cpp
#include "context_mgr.h"
#include <iostream>

bool g_is_debug_mode = false;

void override_example() {
    std::cout << "Debug mode before: " << std::boolalpha << g_is_debug_mode << std::endl;
    {
        auto debug_override = context::make_override(g_is_debug_mode, true);
        std::cout << "Debug mode during override: " << g_is_debug_mode << std::endl;
        // Perform operations that depend on g_is_debug_mode being true
    } // g_is_debug_mode is restored here
    std::cout << "Debug mode after: " << g_is_debug_mode << std::endl;
}
```

### Cancellation / Dismissal

```cpp
#include "context_mgr.h"
#include <iostream>

void cancellation_example() {
    auto guard = context::make_scope_exit([]{
        std::cout << "This cleanup should NOT run." << std::endl;
    });

    std::cout << "Guard is active: " << std::boolalpha << guard.is_active() << std::endl;
    guard.dismiss(); // Cancel the cleanup
    std::cout << "Guard is active after dismiss: " << guard.is_active() << std::endl;
    // When 'guard' goes out of scope, its lambda won't execute.
}
```

## Dependencies
- `<functional>` (for `std::function` if used, though lambdas are common)
- `<optional>` (for storing the exit function)
- `<type_traits>` (for `std::decay_t`, `std::is_nothrow_destructible_v`)
- `<utility>` (for `std::move`, `std::forward`)
- `<string>`, `<iostream>`, `<ostream>` (used by `NamedScope` and examples)

These context management utilities provide powerful and easy-to-use mechanisms for writing safer and more maintainable C++ code by leveraging RAII for resource and state management.

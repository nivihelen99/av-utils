# `inline_function`

`inline_function` is a header-only C++ component that provides a lightweight, type-erased, and small-buffer-optimized callable wrapper. It is similar in concept to `std::function` but is designed to avoid heap allocations for small callable objects (like lambdas with small captures or simple functors).

## Features

- **Small Buffer Optimization (SBO):** Small callable objects are stored directly within the `inline_function` object itself, avoiding the overhead of dynamic memory allocation.
- **Type Erasure:** It can hold any callable object (function pointer, lambda, functor) that matches the specified function signature.
- **Move-Only Semantics:** To keep the implementation simple and efficient, `inline_function` is move-only. Copying is disabled.
- **`std::function`-like API:** Provides a familiar interface, including `operator()`, an explicit boolean conversion, and a `reset()` method.

## Usage

To use `inline_function`, include the `inline_function.h` header.

```cpp
#include "inline_function.h"
```

### Basic Example

Here's how to create and use an `inline_function`:

```cpp
#include "inline_function.h"
#include <iostream>

// A simple function
int add(int a, int b) {
    return a + b;
}

int main() {
    // Wrap a free function
    inline_function<int(int, int)> func1 = &add;
    std::cout << "2 + 3 = " << func1(2, 3) << std::endl;

    // Wrap a lambda
    inline_function<int(int)> func2 = [](int x) { return x * x; };
    std::cout << "Square of 5 = " << func2(5) << std::endl;

    // Check if the function is valid (not empty)
    if (func2) {
        std::cout << "func2 is not empty." << std::endl;
    }

    // Reset the function
    func2.reset();
    if (!func2) {
        std::cout << "func2 is now empty." << std::endl;
    }

    // Attempting to call an empty function throws an exception
    try {
        func2(10);
    } catch (const std::bad_function_call& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }
}
```

## Template Parameters

```cpp
template <
    typename Signature,
    size_t InlineSize = 3 * sizeof(void*)
>
class inline_function;
```

- `Signature`: The function signature of the callable, e.g., `int(std::string, double)`.
- `InlineSize`: The size of the internal buffer in bytes. Callables smaller than or equal to this size will be stored inline. If a callable is larger than `InlineSize`, compilation will fail with a `static_assert`.

## API Reference

- `inline_function()`: Constructs an empty function.
- `inline_function(nullptr_t)`: Constructs an empty function.
- `inline_function(F&& f)`: Constructs from a callable object `f`.
- `inline_function(inline_function&& other)`: Move constructor.
- `operator=(inline_function&& other)`: Move assignment operator.
- `~inline_function()`: Destructor.
- `reset()`: Destroys the contained callable and makes the `inline_function` empty.
- `operator bool()`: Returns `true` if the `inline_function` holds a callable, `false` otherwise.
- `operator()(Args... args)`: Invokes the contained callable. Throws `std::bad_function_call` if empty.

# `functools::partial` - Partial Function Application

## Overview

The `partial.h` header provides a C++ utility, `functools::partial`, that enables partial function application. This is similar in concept to Python's `functools.partial` or C++'s `std::bind` (from `<functional>`), but often with a more focused goal on pre-filling arguments from left-to-right and allowing easy composition of partial calls.

Partial application allows you to create a new callable object (a "partial object") by fixing some of the arguments of an existing function or callable. When the partial object is called, it invokes the original function with the pre-filled (bound) arguments followed by any new arguments provided in the call.

## Namespace
All utilities are within the `functools` namespace.

## Core Components

-   **`functools::partial_impl<F, ...BoundArgs>`**:
    -   The class template that actually implements the partial object.
    -   It stores the original callable `F` and a `std::tuple` of the initially bound arguments (`BoundArgs...`).
    -   Its `operator()` takes any remaining arguments, combines them with the bound arguments, and invokes the original callable using `std::invoke`.
    -   It handles different types of callables, including free functions, lambdas, and member function pointers (when the object instance is one of the bound arguments).
    -   Supports conversion to `std::function` if the signature is compatible.

-   **`functools::partial(F&& func, Args&&... args)`**:
    -   This is the primary free function used to create partial objects.
    -   **Composition/Nesting:** If `func` is already a `partial` object, this function "unwraps" it. It takes the underlying callable from the existing partial object, concatenates its already bound arguments with the new `args`, and creates a new, single `partial` object. This means `partial(partial(f, a), b, c)` is effectively `partial(f, a, b, c)`.
    -   It uses perfect forwarding for both the callable and the arguments.

## Features

-   **Argument Binding:** Pre-fill one or more leading arguments of a function.
-   **Generic:** Works with various callable types: free functions, lambdas, member function pointers (when object is bound), and other function objects.
-   **Composition of Partials:** Applying `partial` to an existing partial object correctly flattens the arguments.
-   **Perfect Forwarding:** Preserves value categories (lvalue/rvalue) of bound arguments and arguments passed during the call.
-   **`std::function` Compatibility:** Partial objects can often be converted to `std::function` if their call signature matches.
-   **Type Safety:** Uses C++ templates and type deduction.

## Public Interface

### Creating Partial Objects
The primary way to create a partial object is via the `functools::partial` free function:
```cpp
auto p = functools::partial(my_function, arg1, arg2 /*, ... */);
// p can now be called with the remaining arguments for my_function.
```

### Calling Partial Objects
Once created, a partial object `p` can be called like a regular function:
```cpp
p(remaining_arg1, remaining_arg2 /*, ... */);
```

## Usage Examples

(Based on `examples/partial_example.cpp`)

### Basic Function and Lambda Binding

```cpp
#include "partial.h" // Adjust path as needed
#include <iostream>
#include <string>
#include <functional>

void greet(const std::string& greeting, const std::string& name, char punctuation) {
    std::cout << greeting << ", " << name << punctuation << std::endl;
}

int add(int a, int b, int c) {
    return a + b + c;
}

int main() {
    // Bind the first argument of greet
    auto greet_hello = functools::partial(greet, "Hello");
    greet_hello("World", '!'); // Output: Hello, World!
    greet_hello("Alice", '.'); // Output: Hello, Alice.

    // Bind the first two arguments of greet
    auto greet_hi_bob = functools::partial(greet, "Hi", "Bob");
    greet_hi_bob('?'); // Output: Hi, Bob?

    // Using a lambda
    auto multiply = [](int x, int y) { return x * y; };
    auto multiply_by_5 = functools::partial(multiply, 5);
    std::cout << "5 * 6 = " << multiply_by_5(6) << std::endl; // Output: 5 * 6 = 30

    // Nested partials / Chaining partial application
    auto add_partial_1 = functools::partial(add, 10);          // add_partial_1(b, c) -> 10 + b + c
    auto add_partial_2 = functools::partial(add_partial_1, 20); // add_partial_2(c)    -> 10 + 20 + c
    int sum = add_partial_2(30);                                // sum = 10 + 20 + 30 = 60
    std::cout << "10 + 20 + 30 = " << sum << std::endl;         // Output: 60
}
```

### Member Function Binding

```cpp
#include "partial.h"
#include <iostream>
#include <string>

class Greeter {
public:
    std::string prefix;
    Greeter(std::string p) : prefix(std::move(p)) {}

    void say(const std::string& message) const {
        std::cout << prefix << ": " << message << std::endl;
    }

    int calculate(int a, int b) {
        return (a + b) * std::stoi(prefix); // Example using prefix
    }
};

int main() {
    Greeter my_greeter("LOG");
    Greeter another_greeter("ALERT");

    // Bind member function 'say' to my_greeter instance
    auto log_message = functools::partial(&Greeter::say, &my_greeter);
    log_message("System initialized."); // Output: LOG: System initialized.

    // Bind member function 'say' to another_greeter, and also bind the first argument of 'say'
    auto alert_critical = functools::partial(&Greeter::say, &another_greeter, "CRITICAL ERROR");
    alert_critical(); // Output: ALERT: CRITICAL ERROR (no further args needed for 'say')

    // Bind member function 'calculate'
    auto calc_with_my_greeter = functools::partial(&Greeter::calculate, &my_greeter, 5);
    // Assuming my_greeter.prefix was "LOG", and stoi("LOG") is not valid.
    // Let's use a Greeter with a numeric prefix for this example.
    Greeter calc_greeter("2"); // prefix "2"
    auto calc_with_numeric_prefix = functools::partial(&Greeter::calculate, &calc_greeter, 5);
    // calc_with_numeric_prefix(b) will call calc_greeter.calculate(5, b)
    // which is (5 + b) * 2
    std::cout << "Calculation (5+3)*2 = " << calc_with_numeric_prefix(3) << std::endl; // (5+3)*2 = 16
}
```

### Using with STL Algorithms

```cpp
#include "partial.h"
#include <iostream>
#include <vector>
#include <algorithm> // For std::transform

int power(int base, int exp) {
    int res = 1;
    for (int i = 0; i < exp; ++i) res *= base;
    return res;
}

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    std::vector<int> squares;

    // Create a function that squares a number (power(num, 2))
    auto square_it = functools::partial(power, std::placeholders::_1, 2); // Using std::bind style placeholder for clarity
                                                                      // Though direct partial(power, ?, 2) is not how functools.partial works.
                                                                      // functools.partial binds from left.
    // Correct way with functools::partial to make a "square" function if 'power' was (exp, base):
    // auto square_it = functools::partial(power, 2); // if power was power(exp, base)
    // For power(base, exp), we need to bind 'exp' which is the second argument.
    // This requires a lambda if we only want to use functools::partial:
    auto square_it_lambda = functools::partial( [](int b){ return power(b, 2); } );


    // More directly, if we want to use std::transform which provides one argument:
    // We need a unary function. `partial` is good for reducing arity.
    // If `power` was `power(base_val, exponent_val)`
    // `partial(power, some_base)` would give `func(exponent_val)`
    // `partial(power, some_base, some_exponent)` would give `func()`
    // For std::transform, we need a UnaryOperation.
    // If we want to square each number, `power(number, 2)`.
    // `partial` binds from left. So, `partial(power, base)` creates `fn(exp)`.
    // We need `fn(base)` where `exp` is fixed.
    // Solution: Use a lambda to reorder or use std::bind.
    // Or, if `power` was `power_of_two(base)`, then `partial(power_of_two)` works.

    // Let's use the lambda approach for clarity with current `power(base, exp)`:
    std::transform(numbers.begin(), numbers.end(),
                   std::back_inserter(squares),
                   [](int n){ return power(n, 2); });
    // Or using partial if we had a function `raise_to_power(exponent, base)`
    // auto raise_to_2 = functools::partial(raise_to_power, 2);
    // std::transform(numbers.begin(), numbers.end(), std::back_inserter(squares), raise_to_2);


    std::cout << "Original numbers: ";
    for(int n : numbers) std::cout << n << " ";
    std::cout << "\nSquares: ";
    for(int n : squares) std::cout << n << " ";
    std::cout << std::endl;
}
```

## Dependencies
- `<tuple>`, `<utility>`, `<functional>`, `<type_traits>`

`functools::partial` is a powerful utility for creating more specialized functions from general ones by fixing certain arguments, leading to more concise and reusable code in various scenarios.

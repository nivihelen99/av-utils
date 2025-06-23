# `pipeline::FunctionPipeline`

## Overview

The `function_pipeline.h` header provides a C++ utility for creating and executing function pipelines. This allows for a sequence of functions to be chained together, where the output of one function becomes the input to the next. It offers a fluent and readable way to express complex data transformations or processing workflows.

The primary components are:
-   `pipeline::FunctionPipeline<F>`: A class that wraps a callable `F` and allows chaining more functions using the `.then()` method.
-   `pipeline::pipe(...)`: Factory functions to start a new pipeline (left-to-right execution).
-   `pipeline::compose(...)`: Factory functions for traditional right-to-left function composition.

## Features

-   **Fluent Chaining:** Create pipelines using an intuitive `pipe(f1).then(f2).then(f3)` syntax.
-   **Type Transformation:** Functions in the pipeline can transform data from one type to another.
-   **Variadic Factories:** `pipe()` and `compose()` can take multiple functions to initialize a longer chain directly.
-   **Generic:** Works with any callable objects (lambdas, free functions, `std::function`, functors).
-   **Perfect Forwarding:** Arguments are perfectly forwarded through the pipeline.
-   **Left-to-Right (`pipe`) and Right-to-Left (`compose`) Semantics:** Supports both common styles of function composition.

## Core Components

### `pipeline::FunctionPipeline<F>`
-   A class template that encapsulates a callable `F`.
-   **`then(G g)`**: Returns a new `FunctionPipeline` where `g` is chained after the current pipeline's function.
-   **`operator()(Args... args)`**: Executes the entire pipeline with the given arguments.

### `pipeline::pipe(...)`
-   **`pipe(F f)`**: Starts a pipeline with function `f`.
-   **`pipe(F1 f1, F2 f2, Fs... fs)`**: Creates a pipeline `fs(...(f2(f1(x)))...)`. Output of `f1` goes to `f2`, output of `f2` goes to the next function, and so on.

### `pipeline::compose(...)`
-   **`compose(F f)`**: Starts a composable unit with function `f`.
-   **`compose(F f, G g)`**: Creates a composition `f(g(x))`.
-   **`compose(F f, G g, Fs... fs)`**: Creates a composition `f(g(fs(...)))`. This is the standard mathematical order of function composition (rightmost function applied first).

## Usage Examples

(Based on `examples/function_pipeline_example.cpp` and examples embedded in `function_pipeline.h`)

### Basic Left-to-Right Pipeline with `pipe`

```cpp
#include "function_pipeline.h" // Assuming header is in include path
#include <iostream>
#include <string>
#include <vector>
#include <algorithm> // For std::transform
#include <cctype>    // For ::tolower

// Example helper functions
std::string trim_whitespace(std::string s) {
    // Basic trim for example purposes
    s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));
    s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);
    return s;
}

std::string string_to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::vector<std::string> split_string(const std::string& s, char delimiter = ' ') {
    std::vector<std::string> tokens;
    std::string token;
    for (char ch : s) {
        if (ch == delimiter) {
            if (!token.empty()) tokens.push_back(token);
            token.clear();
        } else {
            token += ch;
        }
    }
    if (!token.empty()) tokens.push_back(token);
    return tokens;
}


int main() {
    using namespace pipeline;

    // Example 1: Arithmetic pipeline
    auto arithmetic_pipeline = pipe([](int x) { return x * 2; })  // 5*2 = 10
                               .then([](int x) { return x + 3; })  // 10+3 = 13
                               .then([](int x) { return x * x; }); // 13*13 = 169

    int result1 = arithmetic_pipeline(5);
    std::cout << "Arithmetic pipeline(5) = " << result1 << std::endl; // Output: 169

    // Example 2: String processing pipeline
    auto string_pipeline = pipe(trim_whitespace)
                           .then(string_to_lower)
                           .then([](const std::string& s) { return split_string(s, ' '); });

    std::string test_str = "  This IS a TeSt  STRING  ";
    std::vector<std::string> words = string_pipeline(test_str);

    std::cout << "Processed words from \"" << test_str << "\":" << std::endl;
    for (const auto& word : words) {
        std::cout << "  \"" << word << "\"" << std::endl;
    }
    // Expected output: "this", "is", "a", "test", "string"

    // Example 3: Variadic pipe for direct chaining
    auto variadic_pipeline = pipe(
        [](int x) { return x + 1; },      // 10+1 = 11
        [](int x) { return x * 2; },      // 11*2 = 22
        [](int x) { return x - 5; }       // 22-5 = 17
    );
    int result_variadic = variadic_pipeline(10);
    std::cout << "Variadic pipe(10) = " << result_variadic << std::endl; // Output: 17
}
```

### Right-to-Left Composition with `compose`

```cpp
#include "function_pipeline.h"
#include <iostream>

int main() {
    using namespace pipeline;

    // compose(f, g, h) executes as f(g(h(x)))
    auto composed_functions = compose(
        [](int x) { return x * x; },      // f: applied last (result of g + 1)
        [](int x) { return x + 1; },      // g: applied second (result of h * 2)
        [](int x) { return x * 2; }       // h: applied first
    );

    int input = 3;
    // h(3) = 3 * 2 = 6
    // g(6) = 6 + 1 = 7
    // f(7) = 7 * 7 = 49
    int result = composed_functions(input);
    std::cout << "compose(f,g,h)(" << input << ") = " << result << std::endl; // Output: 49
}
```

### Type Transformations in a Pipeline

```cpp
#include "function_pipeline.h"
#include <iostream>
#include <string>
#include <vector> // Only for example data type

int main() {
    using namespace pipeline;

    auto type_transform_pipeline =
        pipe([](int i) { return std::to_string(i); })                 // int -> std::string
       .then([](const std::string& s) { return "Number: " + s; })     // std::string -> std::string
       .then([](const std::string& s) { return s.length(); });        // std::string -> size_t (int-like)

    auto final_length = type_transform_pipeline(12345);
    // Step 1: 12345 -> "12345"
    // Step 2: "12345" -> "Number: 12345"
    // Step 3: "Number: 12345" -> length (e.g., 14)
    std::cout << "Result of type_transform_pipeline(12345): " << final_length << std::endl;
}
```

## Dependencies
- `<utility>` (for `std::move`, `std::forward`)
- `<type_traits>` (for `std::invoke_result_t`, `std::decay_t` etc. if used more extensively, though current snippet is light on these)

The `FunctionPipeline` provides a clean, functional-style approach to chaining operations in C++.

# CachedProperty

## Overview

`CachedProperty<Owner, T>` is a C++ utility class template that provides functionality similar to Python's `functools.cached_property`. It allows a class member (the "property") to be computed lazily upon its first access and then have its result cached for subsequent accesses. This is particularly useful for properties that are computationally expensive and whose values do not change once computed, given the state of the owning object at the time of computation.

The computation is performed by a user-provided function (lambda, bound member function, etc.) that takes a pointer to the owning object.

## Features

-   **Lazy Computation**: The property's value is computed only when it's accessed for the first time.
-   **Caching**: After the first computation, the value is stored, and subsequent accesses return the cached value directly without re-computation.
-   **Invalidation**: The cache can be explicitly invalidated, forcing re-computation on the next access.
-   **Flexible Initialization**: Can be initialized with lambdas, free functions (via `std::function`), or member functions of the owner class (facilitated by `make_cached_property` helpers).
-   **Const-Correctness**: Designed to be usable from `const` contexts of the owning class, provided the computation logic itself is `const`-correct. The `get()` method and conversion operator are `const`.

## Basic Usage

To use `CachedProperty`, you declare it as a member in your class. It requires two template arguments: the type of the owning class and the type of the property value.

The constructor takes:
1.  A pointer to the instance of the owning class (`Owner*`).
2.  A callable (e.g., `std::function<T(Owner*)>`) that will compute the value.

### Example

```cpp
#include "cached_property.h" // Or <cached_property.h>
#include <iostream>
#include <vector>
#include <string>
#include <numeric> // For std::accumulate

class DataAnalytics {
public:
    std::string dataset_name;
    std::vector<double> data_points;
    mutable int sum_computation_count = 0;
    mutable int average_computation_count = 0;

    DataAnalytics(std::string name, std::vector<double> points)
        : dataset_name(std::move(name)), data_points(std::move(points)) {}

    // Property: Sum of data_points (computed by a lambda)
    CachedProperty<DataAnalytics, double> total_sum{this, [](DataAnalytics* self) {
        std::cout << "Computing total_sum for " << self->dataset_name << "..." << std::endl;
        self->sum_computation_count++;
        return std::accumulate(self->data_points.begin(), self->data_points.end(), 0.0);
    }};

    // Property: Average of data_points (computed by a member function)
    // We use make_cached_property for cleaner syntax with member functions.
    CachedProperty<DataAnalytics, double> average_value =
        make_cached_property(this, &DataAnalytics::calculate_average);

private:
    double calculate_average() { // Can be non-const if it modifies internal counters, etc.
        std::cout << "Computing average_value for " << dataset_name << "..." << std::endl;
        average_computation_count++;
        if (data_points.empty()) {
            return 0.0;
        }
        // total_sum is another CachedProperty; accessing it here will use its cached value
        // or compute it if not already done.
        return total_sum / data_points.size();
    }

public:
    void print_report() const {
        std::cout << "\n--- Report for " << dataset_name << " ---" << std::endl;
        std::cout << "Total Sum: " << total_sum << std::endl;       // Access via conversion operator
        std::cout << "Average: " << average_value.get() << std::endl; // Access via .get()
        std::cout << "Sum computations: " << sum_computation_count << std::endl;
        std::cout << "Average computations: " << average_computation_count << std::endl;
    }
};

int main() {
    DataAnalytics stats("Sample A", {1.0, 2.5, 3.0, 4.5, 5.0});

    stats.print_report(); // First access, computes sum and average

    // Accessing again doesn't recompute
    std::cout << "\nAccessing sum again: " << stats.total_sum << std::endl;
    stats.print_report();

    // Invalidate one property
    std::cout << "\nInvalidating average_value..." << std::endl;
    stats.average_value.invalidate();

    // Average will be recomputed, sum will not (unless average's computation also invalidates sum)
    // In this example, calculate_average uses total_sum, which will use its cached value.
    stats.print_report();

    return 0;
}
```

## API Reference

### Template Parameters

-   `Owner`: The type of the class that owns the `CachedProperty`.
-   `T`: The type of the cached value.

### Constructor

-   `CachedProperty(Owner* owner, std::function<T(Owner*)> compute_func)`:
    -   `owner`: Pointer to the owning object. Must not be null and must outlive the `CachedProperty`.
    -   `compute_func`: A callable that takes `Owner*` and returns `T`. Throws `std::invalid_argument` if `owner` or `compute_func` is null.

### Member Functions

-   `const T& get() const`:
    Returns a const reference to the cached value. If the value is not yet cached, it calls the `compute_func`, caches the result, and then returns it.

-   `operator const T&() const`:
    Conversion operator that calls `get()`. Allows accessing the value more naturally (e.g., `double val = my_prop;`).

-   `bool is_cached() const noexcept`:
    Returns `true` if the value has been computed and is currently cached, `false` otherwise.

-   `void invalidate()`:
    Clears the cached value. The next call to `get()` or the conversion operator will trigger re-computation.

### Helper Functions

-   `make_cached_property(Owner* owner, Callable&& callable)`:
    A factory function to simplify creating `CachedProperty` instances, especially with lambdas where `T` can be deduced.

-   `make_cached_property(Owner* owner, Ret(ClassType::*mem_fn)())`:
    Overload for non-const member functions of `Owner` (or its base `ClassType`).

-   `make_cached_property(Owner* owner, Ret(ClassType::*mem_fn)() const)`:
    Overload for `const` member functions of `Owner` (or its base `ClassType`).

## Thread Safety

The current implementation of `CachedProperty` is **not thread-safe**.
If multiple threads attempt to access `get()` concurrently on an uncomputed property, the computation function might be called multiple times, and data races can occur during caching. Similarly, concurrent calls to `invalidate()` and `get()` can lead to undefined behavior.

For scenarios requiring thread safety:
-   External synchronization (e.g., `std::mutex`) must be used around accesses to the `CachedProperty` instance.
-   Alternatively, consider implementing a custom thread-safe version using `std::call_once` or other synchronization primitives if this utility is frequently used in multi-threaded contexts.

## Design Considerations

-   **Ownership**: The `CachedProperty` stores a raw pointer (`Owner*`) to the owning object. It is crucial that the `Owner` object outlives the `CachedProperty` instance. Typically, `CachedProperty` is a member of `Owner`, so this is naturally handled.
-   **Copying/Moving**: `CachedProperty` is non-copyable and non-movable to prevent issues with cache state and owner pointer validity.
-   **Const-Correctness**: The `get()` method and conversion operator are `const`, allowing cached properties to be accessed from `const` methods of the `Owner` class. The internal cache (`std::optional<T>`) is marked `mutable` to allow its update during a `const` call to `get()`. This assumes that the computation itself is logically const (i.e., doesn't change the observable state of `Owner` in a way that violates const-correctness, though it can modify mutable members like counters).

## When to Use

-   When a class has properties that are expensive to compute.
-   When the value of such a property, once computed, does not change for a given state of the object.
-   To simplify the pattern of lazy initialization and caching for individual members.
-   To improve performance by avoiding repeated computations.
```

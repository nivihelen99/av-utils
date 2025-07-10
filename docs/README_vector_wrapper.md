# VectorWrapper

## Table of Contents
1. [Introduction](#introduction)
2. [Purpose](#purpose)
3. [Template Parameters](#template-parameters)
4. [Basic Usage](#basic-usage)
5. [Extending VectorWrapper](#extending-vectorwrapper)
    - [Example: ObservableVector](#example-observablevector)
6. [Key Methods](#key-methods)
7. [Thread Safety](#thread-safety)
8. [Notes on InnerContainer Compatibility](#notes-on-innercontainer-compatibility)


## Introduction

`VectorWrapper<T, InnerContainer>` is a C++ class template that acts as a wrapper around a standard vector-like sequential container (by default, `std::vector<T>`). It is designed to make it easy to create custom list-like types by inheriting from `VectorWrapper` and overriding its virtual methods. This concept is similar to Python's `collections.UserList` or `collections.UserDict` (for which `DictWrapper` is an analog in this library).

## Purpose

The primary purpose of `VectorWrapper` is to provide a convenient base class for users who want to create specialized sequential container types with custom behaviors without having to re-implement the entire container interface. By inheriting from `VectorWrapper`, users can:

- Modify the behavior of specific operations (e.g., logging actions, validating inputs before `push_back`, custom logic on insertion/deletion/access).
- Add new methods to their custom container.
- Use their custom container where a standard-like vector interface is expected (if the interface matches).

## Template Parameters

`VectorWrapper<T, InnerContainer = std::vector<T>>`

-   `T`: The type of the elements stored in the container.
-   `InnerContainer`: The underlying sequential container type to be used. Defaults to `std::vector<T>`. This allows users to wrap other vector-like types (e.g., `std::deque<T>`) or custom sequential container implementations if needed, provided they offer a compatible interface (see [Notes on InnerContainer Compatibility](#notes-on-innercontainer-compatibility)).

## Basic Usage

You can use `VectorWrapper` directly as a wrapper around `std::vector<T>` if you don't need to customize its behavior, although its main strength lies in extension.

```cpp
#include "vector_wrapper.h" // Adjust path as necessary
#include <iostream>
#include <string>
#include <vector> // Only needed if you want to explicitly use std::vector, VectorWrapper includes it

int main() {
    // Basic usage with default std::vector<int>
    VectorWrapper<int> myVec;

    myVec.push_back(10);
    myVec.push_back(20);
    myVec.emplace_back(30); // Using emplace_back

    std::cout << "Contents of myVec: ";
    for (size_t i = 0; i < myVec.size(); ++i) {
        std::cout << myVec[i] << " "; // Using operator[]
    }
    std::cout << std::endl;

    if (!myVec.empty()) {
        std::cout << "Front: " << myVec.front() << ", Back: " << myVec.back() << std::endl;
    }

    myVec.insert(myVec.begin() + 1, 15); // Insert 15 at index 1

    std::cout << "After insert: ";
    for (const auto& val : myVec) { // Using range-based for loop
        std::cout << val << " ";
    }
    std::cout << std::endl;

    myVec.pop_back(); // Removes 30
    myVec.erase(myVec.begin()); // Removes 10

    std::cout << "After pop_back and erase: ";
    for (const auto& val : myVec) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    std::cout << "Size: " << myVec.size() << std::endl;
    myVec.clear();
    std::cout << "Is empty after clear? " << (myVec.empty() ? "Yes" : "No") << std::endl;

    return 0;
}
```

## Extending VectorWrapper

The main power of `VectorWrapper` comes from inheritance. You can create a new class that derives from `VectorWrapper` and override its `virtual` methods to customize behavior.

### Example: ObservableVector

Here's an example of an `ObservableVector` that logs messages to the console whenever elements are added or removed.

```cpp
#include "vector_wrapper.h" // Adjust path as necessary
#include <iostream>
#include <string>
#include <vector> // For std::vector as the default InnerContainer

// Custom vector that logs certain operations
template <typename T, typename InnerContainer = std::vector<T>>
class ObservableVector : public VectorWrapper<T, InnerContainer> {
public:
    // Inherit constructors from the base class
    using Base = VectorWrapper<T, InnerContainer>;
    using Base::Base; // C++11 constructor inheritance

    // Override push_back to log
    void push_back(const T& value) override {
        std::cout << "LOG: ObservableVector: push_back(" << value << ")" << std::endl;
        Base::push_back(value); // Call base class method
    }

    void push_back(T&& value) override {
        // Note: For logging moved values, direct output might not be simple if T is not copyable.
        std::cout << "LOG: ObservableVector: push_back(moved value)" << std::endl;
        Base::push_back(std::move(value));
    }

    // Override pop_back to log
    void pop_back() override {
        if (!this->empty()) {
            std::cout << "LOG: ObservableVector: pop_back() on value " << this->back() << std::endl;
        } else {
            std::cout << "LOG: ObservableVector: pop_back() on empty vector" << std::endl;
        }
        Base::pop_back();
    }

    // Override clear to log
    void clear() noexcept override {
        std::cout << "LOG: ObservableVector: clear()" << std::endl;
        Base::clear();
    }

    // Override insert to log
    typename Base::iterator insert(typename Base::const_iterator pos, const T& value) override {
        // For simplicity, just log the value. Could convert pos to index for more detail.
        std::cout << "LOG: ObservableVector: insert(" << value << ")" << std::endl;
        return Base::insert(pos, value);
    }

    // Override erase to log
    typename Base::iterator erase(typename Base::const_iterator pos) override {
        if (pos != this->cend()) { // Check if iterator is valid before dereferencing
             std::cout << "LOG: ObservableVector: erase() on value " << *pos << std::endl;
        } else {
             std::cout << "LOG: ObservableVector: erase() on end iterator" << std::endl;
        }
        return Base::erase(pos);
    }
};

int main() {
    ObservableVector<std::string> myObsVec;

    myObsVec.push_back("event_A");
    myObsVec.push_back("event_B");

    std::cout << "ObservableVector contents: ";
    for (const auto& val : myObsVec) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    myObsVec.insert(myObsVec.begin() + 1, "event_C"); // Insert at index 1

    myObsVec.pop_back(); // Removes "event_B"
    myObsVec.erase(myObsVec.begin()); // Removes "event_A"

    std::cout << "ObservableVector final contents: ";
    for (const auto& val : myObsVec) {
        std::cout << val << " ";
    }
    std::cout << std::endl; // Should print "event_C"

    myObsVec.clear();
    return 0;
}

```

## Key Methods

`VectorWrapper` forwards most of the `std::vector` interface. Most of these methods are `virtual` and can be overridden:

**Element Access:**
-   `operator[]`: Access element (no bounds check in release builds for `std::vector`).
-   `at()`: Access element with bounds checking (throws `std::out_of_range` if out of bounds).
-   `front()`: Access the first element.
-   `back()`: Access the last element.
-   `data_ptr()`: Get a direct pointer to the underlying array (if `InnerContainer` supports it, like `std::vector`).

**Iterators:**
-   `begin()`, `end()`: For forward iteration.
-   `cbegin()`, `cend()`: For const forward iteration.
-   `rbegin()`, `rend()`: For reverse iteration.
-   `crbegin()`, `crend()`: For const reverse iteration.

**Capacity:**
-   `empty()`: Check if the container is empty.
-   `size()`: Get the number of elements.
-   `max_size()`: Get the maximum possible number of elements.
-   `reserve()`: Request a change in capacity.
-   `capacity()`: Get the current capacity.
-   `shrink_to_fit()`: Request to reduce capacity to fit the size.

**Modifiers:**
-   `assign()`: Assign new contents, replacing current ones.
-   `clear()`: Remove all elements.
-   `insert()`: Insert elements at a specific position. Various overloads.
-   `emplace()`: Construct element in-place at a specific position.
-   `erase()`: Erase elements.
-   `push_back()`: Add element to the end.
-   `emplace_back()`: Construct element in-place at the end.
-   `pop_back()`: Remove the last element.
-   `resize()`: Change the number of elements stored.
-   `swap()`: Swap contents with another `VectorWrapper`.

**Non-Virtual Template Methods:**
Methods like `emplace()`, `emplace_back()`, and some overloads of `insert()` and `assign()` are template methods (e.g., taking iterator pairs `template <typename InputIt> insert(...)`). These template methods cannot be `virtual` themselves.
If customization for these specific template methods is needed, you can:
1.  Hide the base class template method by providing your own non-virtual template method with the same signature in the derived class.
2.  Often, these template methods internally call other virtual methods (e.g., a template `insert(InputIt, InputIt)` might call `push_back` or a single-element `insert` repeatedly). Overriding those underlying virtual methods might be sufficient.

## Thread Safety

`VectorWrapper` itself is not inherently thread-safe, similar to standard library containers like `std::vector`. If you need to use a `VectorWrapper` or its derived classes in a multi-threaded environment, you must implement external synchronization (e.g., using `std::mutex`) to protect concurrent accesses, especially for write operations or any operation that modifies the container's state. Overridden methods in derived classes must also be mindful of thread safety if they introduce shared state or side effects.

## Notes on InnerContainer Compatibility

While `VectorWrapper` defaults to `std::vector<T>`, it can be templated on other `InnerContainer` types. For full compatibility, the `InnerContainer` should provide an interface that matches the methods `VectorWrapper` attempts to call.

For example, `std::list<T>` could be used as an `InnerContainer`. However, `std::list` does not provide `operator[]`, `at()`, `capacity()`, `reserve()`, or `data_ptr()`. If a `VectorWrapper<T, std::list<T>>` instance has these methods called on it, it will result in a compile-time error because `VectorWrapper`'s implementation of these methods directly calls them on the `InnerContainer`.

To create a truly generic wrapper that adapts to different underlying container capabilities (e.g., using SFINAE or C++20 concepts to enable/disable methods), `VectorWrapper` would need to be more complex. The current implementation assumes a `std::vector`-like interface from the `InnerContainer` for all its exposed methods. If you use an `InnerContainer` with a reduced interface, ensure you only call/override methods supported by that `InnerContainer`.

The non-member comparison operators (`operator==`, `operator!=`) rely on the `InnerContainer` itself being comparable and on the element type `T` being comparable.

---

This documentation should provide a good overview for users of the `VectorWrapper` class.Tool output for `create_file_with_block`:

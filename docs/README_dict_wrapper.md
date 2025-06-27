# DictWrapper

## Table of Contents
1. [Introduction](#introduction)
2. [Purpose](#purpose)
3. [Template Parameters](#template-parameters)
4. [Basic Usage](#basic-usage)
5. [Extending DictWrapper](#extending-dictwrapper)
    - [Example: LoggingDict](#example-loggingdict)
6. [Key Methods](#key-methods)
7. [Thread Safety](#thread-safety)

## Introduction

`DictWrapper` is a C++ class template that acts as a wrapper around a standard dictionary-like data structure (by default, `std::unordered_map`). It is designed to make it easy to create custom dictionary types by inheriting from `DictWrapper` and overriding its virtual methods. This is similar in concept to Python's `collections.UserDict`.

## Purpose

The primary purpose of `DictWrapper` is to provide a convenient base class for users who want to create specialized dictionary types with custom behaviors without having to re-implement the entire dictionary interface. By inheriting from `DictWrapper`, users can:

- Modify the behavior of specific dictionary operations (e.g., logging, validation, custom logic on insertion/deletion/access).
- Add new methods to their custom dictionary.
- Use their custom dictionary wherever a standard-like dictionary interface is expected (if the interface matches).

## Template Parameters

`DictWrapper<K, V, InnerMap = std::unordered_map<K, V>>`

-   `K`: The type of the keys in the dictionary.
-   `V`: The type of the values in the dictionary.
-   `InnerMap`: The underlying map type to be used. Defaults to `std::unordered_map<K, V>`. This allows users to wrap other map types like `std::map` or custom map implementations if needed, provided they offer a compatible interface.

## Basic Usage

You can use `DictWrapper` directly as a wrapper around `std::unordered_map` if you don't need to customize its behavior, although its main strength lies in extension.

```cpp
#include "dict_wrapper.h" // Assuming it's in the include path
#include <iostream>
#include <string>

int main() {
    // Basic usage with default std::unordered_map
    DictWrapper<std::string, int> myDict;

    // Insert elements
    myDict.insert({"apple", 1});
    myDict["banana"] = 2; // Using operator[]

    // Access elements
    std::cout << "Value of apple: " << myDict.at("apple") << std::endl;
    if (myDict.contains("banana")) {
        std::cout << "Value of banana: " << myDict["banana"] << std::endl;
    }

    // Iterate
    std::cout << "Contents of myDict:" << std::endl;
    for (const auto& pair : myDict) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }

    // Size and empty check
    std::cout << "Size: " << myDict.size() << std::endl;
    std::cout << "Is empty? " << (myDict.empty() ? "Yes" : "No") << std::endl;

    myDict.erase("apple");
    std::cout << "Size after erasing apple: " << myDict.size() << std::endl;

    myDict.clear();
    std::cout << "Size after clear: " << myDict.size() << std::endl;

    return 0;
}
```

## Extending DictWrapper

The main power of `DictWrapper` comes from inheritance. You can create a new class that derives from `DictWrapper` and override its `virtual` methods to customize behavior.

### Example: LoggingDict

Here's an example of a `LoggingDict` that logs messages to the console whenever elements are inserted, accessed, or erased.

```cpp
#include "dict_wrapper.h"
#include <iostream>
#include <string>
#include <stdexcept> // For std::out_of_range

// Custom dictionary that logs insertions, accesses, and deletions
class LoggingDict : public DictWrapper<std::string, int> {
public:
    // Inherit constructors from the base class
    using Base = DictWrapper<std::string, int>;
    using Base::Base;

    // Override insert to log
    std::pair<iterator, bool> insert(const value_type& value) override {
        std::cout << "LOG: Attempting to insert key='" << value.first << "', value=" << value.second << std::endl;
        auto result = Base::insert(value); // Call base class method
        if (result.second) {
            std::cout << "LOG: Successfully inserted key='" << value.first << "'" << std::endl;
        } else {
            std::cout << "LOG: Key='" << value.first << "' already exists. Insertion failed." << std::endl;
        }
        return result;
    }

    // Override erase (by key) to log
    size_type erase(const key_type& key) override {
        std::cout << "LOG: Attempting to erase key='" << key << "'" << std::endl;
        size_type result = Base::erase(key); // Call base class method
        if (result > 0) {
            std::cout << "LOG: Successfully erased key='" << key << "'" << std::endl;
        } else {
            std::cout << "LOG: Key='" << key << "' not found for erasure." << std::endl;
        }
        return result;
    }

    // Override operator[] to log access/modification
    mapped_type& operator[](const key_type& key) override {
        std::cout << "LOG: Accessing/modifying key='" << key << "' via operator[]" << std::endl;
        if (!this->contains(key)) { // `this->` is optional for calling base/own methods
            std::cout << "LOG: Key='" << key << "' not found by operator[], will be default-inserted if not assigned." << std::endl;
        }
        return Base::operator[](key); // Call base class method
    }

    // Override at (const version) to log access
    const mapped_type& at(const key_type& key) const override {
        std::cout << "LOG: Accessing key='" << key << "' via at() const" << std::endl;
        try {
            return Base::at(key); // Call base class method
        } catch (const std::out_of_range& e) {
            std::cout << "LOG: Key='" << key << "' not found in at() const. Exception: " << e.what() << std::endl;
            throw; // Re-throw the exception
        }
    }
     // Override at (non-const version) to log access
    mapped_type& at(const key_type& key) override {
        std::cout << "LOG: Accessing key='" << key << "' via at() non-const" << std::endl;
         try {
            return Base::at(key); // Call base class method
        } catch (const std::out_of_range& e) {
            std::cout << "LOG: Key='" << key << "' not found in at() non-const. Exception: " << e.what() << std::endl;
            throw; // Re-throw the exception
        }
    }
};

int main() {
    LoggingDict myLogDict;

    myLogDict.insert({"event", 2024});
    myLogDict["user_id"] = 12345;

    try {
        std::cout << "User ID: " << myLogDict.at("user_id") << std::endl;
        std::cout << "Event Year: " << static_cast<const LoggingDict&>(myLogDict).at("event") << std::endl;
        // Accessing non-existent key
        // std::cout << "Status: " << myLogDict.at("status") << std::endl;
    } catch (const std::out_of_range& e) {
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }

    myLogDict.erase("event");
    return 0;
}
```

## Key Methods

`DictWrapper` forwards most of the `std::unordered_map` interface. Most of these methods are `virtual` and can be overridden:

-   `operator[]`: Access or insert element.
-   `at()`: Access element, throws `std::out_of_range` if key not found.
-   `insert()`: Insert elements. Various overloads.
-   `erase()`: Erase elements. Various overloads.
-   `clear()`: Remove all elements.
-   `empty()`: Check if the dictionary is empty.
-   `size()`: Get the number of elements.
-   `find()`: Find element by key.
-   `count()`: Count elements with a specific key (typically 0 or 1).
-   `contains()`: (C++20 style) Check if key exists.
-   `begin()`, `end()`: Iterators for range-based for loops and algorithms.
-   `cbegin()`, `cend()`: Const iterators.

**Non-Virtual Template Methods:**
Methods like `emplace()` and `emplace_hint()` are template methods and thus cannot be `virtual`. If customization for emplacement is needed, you can:
1. Hide the base class `emplace` by providing your own non-virtual `emplace` in the derived class.
2. Override `insert` methods, as `emplace` might internally use them (though this is implementation-dependent for the underlying map).

## Thread Safety

`DictWrapper` itself is not inherently thread-safe, similar to standard library containers like `std::unordered_map`. If you need to use a `DictWrapper` or its derived classes in a multi-threaded environment, you must implement external synchronization (e.g., using mutexes) to protect concurrent accesses, especially for write operations or any operation that modifies the container's state. Overridden methods in derived classes must also be mindful of thread safety if they introduce shared state or side effects.

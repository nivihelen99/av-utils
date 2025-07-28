# History

The `History` class is a container adapter that provides a way to record and manage the history of an object. It allows you to commit new states of the object, access previous states, and revert to any point in the object's history.

## Usage

To use the `History` class, you need to include the `history.h` header file:

```cpp
#include "history.h"
```

The `History` class is a template class that takes the type of the object to be versioned as a template parameter. For example, to create a history of a `std::vector<int>`, you would do the following:

```cpp
cpp_collections::History<std::vector<int>> history({1, 2, 3});
```

### Committing Changes

To save the current state of the object as a new version, you can use the `commit()` method:

```cpp
history.latest().push_back(4);
history.commit();
```

### Accessing Previous Versions

You can access previous versions of the object using the `get()` method, which takes a version number as a parameter:

```cpp
const auto& version0 = history.get(0);
const auto& version1 = history.get(1);
```

### Reverting to a Previous Version

You can revert the object to a previous version using the `revert()` method. This will create a new version with the state of the reverted version:

```cpp
history.revert(0);
```

### Other Operations

- `versions()`: Returns the total number of versions.
- `current_version()`: Returns the current version number.
- `latest()`: Returns a reference to the latest version of the object, allowing modification.
- `clear()`: Clears the history, keeping only the latest version as the initial state.
- `reset()`: Clears the history and resets to a default-constructed object.

## Example

```cpp
#include "history.h"
#include <iostream>
#include <vector>

void print_vector(const std::vector<int>& vec) {
    std::cout << "{ ";
    for (const auto& i : vec) {
        std::cout << i << " ";
    }
    std::cout << "}" << std::endl;
}

int main() {
    cpp_collections::History<std::vector<int>> history({1, 2, 3});

    std::cout << "Initial state (v0): ";
    print_vector(history.latest());

    history.latest().push_back(4);
    history.commit();
    std::cout << "After adding 4 (v1): ";
    print_vector(history.latest());

    history.revert(0);
    std::cout << "After reverting to v0 (new v2): ";
    print_vector(history.latest());

    return 0;
}

```

This will output:
```
Initial state (v0): { 1 2 3 }
After adding 4 (v1): { 1 2 3 4 }
After reverting to v0 (new v2): { 1 2 3 }
```

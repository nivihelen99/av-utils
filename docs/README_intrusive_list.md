# Intrusive List

An intrusive list is a doubly linked list where the nodes are not allocated by the list itself. Instead, the user embeds a `intrusive_list_hook` within their own data structures. This approach avoids memory allocation overhead when adding or removing elements from the list, which can lead to significant performance improvements in scenarios with frequent modifications.

## Usage

To use the intrusive list, you need to embed an `intrusive_list_hook` in your struct or class:

```cpp
#include "intrusive_list.h"

struct MyObject {
    int data;
    cpp_utils::intrusive_list_hook hook;

    MyObject(int d) : data(d) {}
};
```

Then, you can create an `intrusive_list` and specify the type of the object and the member hook:

```cpp
cpp_utils::intrusive_list<MyObject, &MyObject::hook> list;
```

Now you can use the list like a standard container:

```cpp
MyObject obj1(1);
MyObject obj2(2);

list.push_back(obj1);
list.push_front(obj2);

for (const auto& obj : list) {
    // ...
}
```

## API

The `intrusive_list` class provides the following methods:

- `push_back(T& value)`: Adds an element to the end of the list.
- `push_front(T& value)`: Adds an element to the beginning of the list.
- `pop_back()`: Removes the last element of the list.
- `pop_front()`: Removes the first element of the list.
- `empty() const`: Checks if the list is empty.
- `size() const`: Returns the number of elements in the list.
- `begin()`, `end()`, `cbegin()`, `cend()`: Provide iterators to the list.
- `front()`, `back()`: Access the first and last elements.
- `insert(iterator pos, T& value)`: Inserts an element at a specific position.
- `erase(iterator pos)`: Removes an element at a specific position.
- `clear()`: Removes all elements from the list.

The `intrusive_list_hook` class provides the following method:

- `is_linked() const`: Checks if the hook is part of a list.

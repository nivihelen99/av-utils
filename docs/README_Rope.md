# Rope

A Rope is a data structure for efficiently storing and manipulating strings. It is a binary tree where each leaf (or "rope") holds a string fragment. This structure allows for fast concatenation and splitting of strings, as these operations can be performed by simply creating new tree nodes, without copying the string data itself.

## Advantages

- **Fast Concatenation:** Concatenating two ropes is much faster than concatenating two standard strings, as it only involves creating a new root node.
- **Efficient Substrings:** Creating a substring is also efficient and does not involve copying data.
- **Less Memory Overhead for Large Strings:** For very large strings, ropes can be more memory-efficient than contiguous arrays, especially when many modifications are made.

## API

- `Rope()`: Constructs an empty rope.
- `Rope(const std::string& str)`: Constructs a rope from a string.
- `void append(const std::string& str)`: Appends a string to the end of the rope.
- `char at(size_t index)`: Returns the character at the specified index.
- `std::string to_string() const`: Converts the rope to a standard string.
- `size_t size() const`: Returns the total length of the string stored in the rope.

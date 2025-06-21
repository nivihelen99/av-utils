#include "zip_view.h"
#include <iostream>
#include <vector>
#include <list>
#include <deque>
#include <string>

int main() {
    using namespace zip_utils;

    // Example 1: Basic zip with vectors
    std::vector<int> ids = {1, 2, 3, 4};
    std::vector<std::string> names = {"one", "two", "three"};

    std::cout << "Example 1: Basic zip\n";
    for (auto&& [id, name] : zip(ids, names)) {
        std::cout << id << " = " << name << "\n";
    }

    // Example 2: Three containers with different types
    std::vector<int> a = {1, 2, 3};
    std::list<std::string> b = {"a", "b", "c"};
    std::deque<char> c = {'x', 'y', 'z'};

    std::cout << "\nExample 2: Three containers\n";
    for (auto&& [i, s, ch] : zip(a, b, c)) {
        std::cout << i << " " << s << " " << ch << "\n";
    }

    // Example 3: Mutable references
    std::vector<int> vec1 = {1, 2, 3};
    std::vector<int> vec2 = {10, 20, 30};

    std::cout << "\nExample 3: Before modification\n";
    for (auto&& [x, y] : zip(vec1, vec2)) {
        std::cout << x << " " << y << "\n";
    }

    // Modify through zip
    for (auto&& [x, y] : zip(vec1, vec2)) {
        x += y;
    }

    std::cout << "After modification (vec1 += vec2):\n";
    for (int x : vec1) {
        std::cout << x << " ";
    }
    std::cout << "\n";

    // Example 4: Enumerate
    std::vector<std::string> words = {"hello", "world", "cpp"};

    std::cout << "\nExample 4: Enumerate\n";
    for (auto&& [index, word] : enumerate(words)) {
        std::cout << index << ": " << word << "\n";
    }

    // Example 5: Const containers
    const std::vector<int> const_vec = {5, 6, 7};
    const std::vector<char> const_chars = {'a', 'b', 'c'};

    std::cout << "\nExample 5: Const containers\n";
    for (auto&& [num, ch] : zip(const_vec, const_chars)) {
        std::cout << num << " -> " << ch << "\n";
    }

    return 0;
}

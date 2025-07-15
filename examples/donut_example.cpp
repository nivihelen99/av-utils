#include "donut.h"
#include <iostream>

int main() {
    // Create a Donut with a capacity of 5
    cpp_utils::Donut<int> donut(5);

    // Add some elements
    donut.push(1);
    donut.push(2);
    donut.push(3);
    donut.push(4);
    donut.push(5);

    std::cout << "Donut elements: ";
    for (int i : donut) {
        std::cout << i << " ";
    }
    std::cout << std::endl;

    // Add more elements to overflow the capacity
    donut.push(6);
    donut.push(7);

    std::cout << "Donut elements after overflow: ";
    for (int i : donut) {
        std::cout << i << " ";
    }
    std::cout << std::endl;

    // Access elements by index
    std::cout << "Element at index 0: " << donut[0] << std::endl;
    std::cout << "Element at index 2: " << donut[2] << std::endl;

    return 0;
}

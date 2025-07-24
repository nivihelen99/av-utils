#include "Rope.h"
#include <iostream>

int main() {
    Rope r1("Hello, ");
    r1.append("World!");
    r1.append(" This is a test.");

    std::cout << "Rope: " << r1.to_string() << std::endl;
    std::cout << "Size: " << r1.size() << std::endl;
    std::cout << "Char at 10: " << r1.at(10) << std::endl;

    return 0;
}

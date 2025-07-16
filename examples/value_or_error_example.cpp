#include "value_or_error.h"
#include <iostream>
#include <string>

cxxds::value_or_error<int, std::string> parse_int(const std::string& s) {
    try {
        return std::stoi(s);
    } catch (const std::invalid_argument& e) {
        return std::string("Invalid argument");
    } catch (const std::out_of_range& e) {
        return std::string("Out of range");
    }
}

int main() {
    auto result1 = parse_int("123");
    if (result1.has_value()) {
        std::cout << "Parsed integer: " << result1.value() << std::endl;
    } else {
        std::cout << "Error: " << result1.error() << std::endl;
    }

    auto result2 = parse_int("abc");
    if (result2.has_value()) {
        std::cout << "Parsed integer: " << result2.value() << std::endl;
    } else {
        std::cout << "Error: " << result2.error() << std::endl;
    }

    return 0;
}

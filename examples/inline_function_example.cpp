#include "inline_function.h"
#include <iostream>
#include <string>

// 1. A free function
int free_function(int x, int y) {
    return x + y;
}

// 2. A functor (struct with operator())
struct Functor {
    int operator()(int x) const {
        return x * x;
    }
};

int main() {
    // Using a free function
    inline_function<int(int, int)> func1 = &free_function;
    std::cout << "Free function: 5 + 3 = " << func1(5, 3) << std::endl;

    // Using a lambda
    inline_function<int(int)> func2 = [](int x) { return x * 2; };
    std::cout << "Lambda: 10 * 2 = " << func2(10) << std::endl;

    // Using a stateful lambda
    int factor = 10;
    inline_function<int(int)> func3 = [factor](int x) { return x * factor; };
    std::cout << "Stateful lambda: 7 * 10 = " << func3(7) << std::endl;

    // Using a functor
    Functor functor_instance;
    inline_function<int(int)> func4 = functor_instance;
    std::cout << "Functor: 8 * 8 = " << func4(8) << std::endl;

    // Move construction
    inline_function<int(int)> func5 = std::move(func2);
    std::cout << "Moved function: 10 * 2 = " << func5(10) << std::endl;
    if (!func2) {
        std::cout << "Original function (func2) is empty after move." << std::endl;
    }

    // Move assignment
    inline_function<int(int)> func_for_assignment;
    func_for_assignment = std::move(func3);
    std::cout << "Move-assigned function: 7 * 10 = " << func_for_assignment(7) << std::endl;
    if (!func3) {
        std::cout << "Original function (func3) is empty after move assignment." << std::endl;
    }

    // Using with void return type
    inline_function<void(std::string)> func6 = [](const std::string& msg) {
        std::cout << "Void return lambda: " << msg << std::endl;
    };
    func6("Hello, World!");

    // Check empty function call
    inline_function<void()> empty_func;
    try {
        empty_func();
    } catch (const std::bad_function_call& e) {
        std::cout << "Successfully caught exception for calling an empty function: " << e.what() << std::endl;
    }

    return 0;
}

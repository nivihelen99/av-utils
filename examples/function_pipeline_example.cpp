
#define PIPELINE_EXAMPLES
#include "function_pipeline.h"

#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <functional>

void test_basic_pipeline() {
    using namespace pipeline;
    
    std::cout << "Testing basic pipeline...\n";
    
    // Simple arithmetic pipeline
    auto p1 = pipe([](int x) { return x + 1; })
                 .then([](int x) { return x * 2; });
    
    assert(p1(5) == 12);  // (5 + 1) * 2 = 12
    std::cout << "✓ Basic arithmetic pipeline works\n";
    
    // Three-stage pipeline
    auto p2 = pipe([](int x) { return x * 2; })
                 .then([](int x) { return x + 3; })
                 .then([](int x) { return x * x; });
    
    assert(p2(5) == 169);  // ((5 * 2) + 3)^2 = 13^2 = 169
    std::cout << "✓ Three-stage pipeline works\n";
}

void test_type_transformations() {
    using namespace pipeline;
    
    std::cout << "\nTesting type transformations...\n";
    
    // int -> string -> string
    auto p1 = pipe([](int x) { return std::to_string(x); })
                 .then([](const std::string& s) { return "Value: " + s; });
    
    assert(p1(42) == "Value: 42");
    std::cout << "✓ int -> string transformation works\n";
    
    // string -> int -> double
    auto p2 = pipe([](const std::string& s) { return static_cast<int>(s.length()); })
                 .then([](int len) { return static_cast<double>(len) / 2.0; });
    
    assert(p2("hello") == 2.5);  // "hello".length() = 5, 5/2.0 = 2.5
    std::cout << "✓ string -> int -> double transformation works\n";
}

void test_variadic_pipe() {
    using namespace pipeline;
    
    std::cout << "\nTesting variadic pipe...\n";
    
    // Test with 3 functions
    auto p1 = pipe(
        [](int x) { return x + 1; },
        [](int x) { return x * 2; },
        [](int x) { return x - 5; }
    );
    
    assert(p1(10) == 17);  // ((10 + 1) * 2) - 5 = 22 - 5 = 17
    std::cout << "✓ Three-function variadic pipe works\n";
    
    // Test with 4 functions
    auto p2 = pipe(
        [](int x) { return x * 2; },
        [](int x) { return x + 1; },
        [](int x) { return x * x; },
        [](int x) { return x - 10; }
    );
    
    assert(p2(3) == 39);  // (((3 * 2) + 1)^2) - 10 = (7^2) - 10 = 49 - 10 = 39
    std::cout << "✓ Four-function variadic pipe works\n";
}

void test_composition() {
    using namespace pipeline;
    
    std::cout << "\nTesting right-to-left composition...\n";
    
    // compose(f, g, h) should create f(g(h(x)))
    auto c1 = compose(
        [](int x) { return x * x; },      // f - applied last
        [](int x) { return x + 1; },      // g - applied second  
        [](int x) { return x * 2; }       // h - applied first
    );
    
    // For input 3: h(3) = 6, g(6) = 7, f(7) = 49
    assert(c1(3) == 49);
    std::cout << "✓ Three-function composition works\n";
    
    // Compare with left-to-right pipe
    auto p1 = pipe([](int x) { return x * 2; })
                 .then([](int x) { return x + 1; })
                 .then([](int x) { return x * x; });
    
    assert(p1(3) == 49);  // Same result since it's the same order
    std::cout << "✓ Pipe and compose give same result for same function order\n";
}

void test_std_function_compatibility() {
    using namespace pipeline;
    
    std::cout << "\nTesting std::function compatibility...\n";
    
    std::function<int(int)> f1 = [](int x) { return x + 10; };
    std::function<int(int)> f2 = [](int x) { return x * 3; };
    
    auto p1 = pipe(f1).then(f2);
    assert(p1(5) == 45);  // (5 + 10) * 3 = 45
    std::cout << "✓ std::function compatibility works\n";
}

void test_perfect_forwarding() {
    using namespace pipeline;
    
    std::cout << "\nTesting perfect forwarding...\n";
    
    // Test with move-only type simulation
    struct MoveOnly {
        int value;
        MoveOnly(int v) : value(v) {}
        MoveOnly(const MoveOnly&) = delete;
        MoveOnly& operator=(const MoveOnly&) = delete;
        MoveOnly(MoveOnly&&) = default;
        MoveOnly& operator=(MoveOnly&&) = default;
    };
    
    auto p1 = pipe([](MoveOnly&& m) { return m.value * 2; })
                 .then([](int x) { return x + 1; });
    
    assert(p1(MoveOnly{5}) == 11);  // 5 * 2 + 1 = 11
    std::cout << "✓ Perfect forwarding works with move-only types\n";
}

void test_complex_scenarios() {
    using namespace pipeline;
    
    std::cout << "\nTesting complex scenarios...\n";
    
    // Mixed types and operations
    auto complex = pipe([](const std::vector<int>& v) { 
                           int sum = 0;
                           for (int x : v) sum += x;
                           return sum;
                       })
                      .then([](int sum) { return static_cast<double>(sum) / 2.0; })
                      .then([](double avg) { return std::to_string(avg); })
                      .then([](const std::string& s) { return "Average: " + s; });
    
    std::vector<int> data = {1, 2, 3, 4, 5};
    std::string result = complex(data);
    assert(result == "Average: 7.500000");
    std::cout << "✓ Complex multi-type pipeline works\n";
    
    // Nested pipelines
    auto inner = pipe([](int x) { return x * 2; })
                    .then([](int x) { return x + 1; });
    
    auto outer = pipe([inner](int x) { return inner(x); })  // Use inner pipeline as function
                    .then([](int x) { return x * x; });
    
    assert(outer(3) == 49);  // inner(3) = 7, 7^2 = 49
    std::cout << "✓ Nested pipelines work\n";
}

int main() {
    std::cout << "=== FunctionPipeline Test Suite ===\n\n";
    
    try {
        test_basic_pipeline();
        test_type_transformations();
        test_variadic_pipe();
        test_composition();
        test_std_function_compatibility();
        test_perfect_forwarding();
        test_complex_scenarios();
        
        std::cout << "\n=== All Tests Passed! ===\n\n";
        
        // Run examples
        examples::run_examples();
        
    } catch (const std::exception& e) {
        std::cout << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

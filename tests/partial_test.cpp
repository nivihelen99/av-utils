#include "gtest/gtest.h"
#include "../include/partial.h" // Adjusted path
#include <string>
#include <vector>
#include <algorithm> // For std::transform
#include <functional> // For std::function
#include <numeric>    // For std::iota

// Global/Static function for testing
int sum_global(int a, int b, int c) {
    return a + b + c;
}

void modify_ref(int& val, int new_val) {
    val = new_val;
}

struct MyClass {
    std::string greeting;
    int value = 0;

    MyClass(std::string g) : greeting(std::move(g)) {}
    MyClass() = default;


    std::string greet(const std::string& name) const {
        return greeting + ", " + name + "!";
    }

    std::string greet_no_args() const {
        return greeting + "!";
    }

    int add(int a, int b) {
        return a + b;
    }

    void set_value(int v) {
        value = v;
    }

    int get_value() const {
        return value;
    }
};

// Test fixture for constexpr tests if needed, or just use TEST directly
class PartialConstexprTest : public ::testing::Test {};

TEST(PartialTest, BasicFunctionBinding) {
    auto p1 = functools::partial(sum_global, 10);
    ASSERT_EQ(p1(20, 30), 60);

    auto p2 = functools::partial(sum_global, 10, 20);
    ASSERT_EQ(p2(30), 60);

    auto p3 = functools::partial(sum_global, 10, 20, 30);
    ASSERT_EQ(p3(), 60);
}

TEST(PartialTest, LambdaBinding) {
    auto lambda_sum = [](int a, int b, int c) { return a + b + c; };
    auto p1 = functools::partial(lambda_sum, 1);
    ASSERT_EQ(p1(2, 3), 6);

    auto p2 = functools::partial(lambda_sum, 1, 2);
    ASSERT_EQ(p2(3), 6);

    auto p3 = functools::partial(lambda_sum, 1, 2, 3);
    ASSERT_EQ(p3(), 6);
}

TEST(PartialTest, MemberFunctionBinding) {
    MyClass instance("Hello");
    auto p_greet = functools::partial(&MyClass::greet, &instance, "World");
    ASSERT_EQ(p_greet(), "Hello, World!");

    auto p_greet_no_args = functools::partial(&MyClass::greet_no_args, &instance);
    ASSERT_EQ(p_greet_no_args(), "Hello!");

    auto p_add = functools::partial(&MyClass::add, &instance, 5);
    ASSERT_EQ(p_add(3), 8);

    const MyClass const_instance("Hi");
    auto p_const_greet = functools::partial(&MyClass::greet, &const_instance, "There");
    ASSERT_EQ(p_const_greet(), "Hi, There!");
}

TEST(PartialTest, NestedPartials) {
    auto multiply = [](int a, int b, int c, int d) { return a * b * c * d; };
    auto p1 = functools::partial(multiply, 2);         // 2 * b * c * d
    auto p2 = functools::partial(p1, 3);               // 2 * 3 * c * d
    auto p3 = functools::partial(p2, 4);               // 2 * 3 * 4 * d
    ASSERT_EQ(p3(5), 2 * 3 * 4 * 5);                   // 2 * 3 * 4 * 5
}

TEST(PartialTest, STLAlgorithmUsage) {
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    std::vector<int> expected = {11, 12, 13, 14, 15};
    std::vector<int> results;

    auto add_ten_to_element = functools::partial([](int a, int b) { return a + b; }, 10);

    std::transform(numbers.begin(), numbers.end(), std::back_inserter(results), add_ten_to_element);
    ASSERT_EQ(results, expected);
}

TEST(PartialTest, StdFunctionConversion) {
    auto p_sum = functools::partial(sum_global, 100, 200);
    std::function<int(int)> func = p_sum;
    ASSERT_EQ(func(30), 330);

    MyClass instance("Test");
    auto p_greet = functools::partial(&MyClass::greet, &instance, "StdFunc");
    std::function<std::string()> func_greet = p_greet;
    ASSERT_EQ(func_greet(), "Test, StdFunc!");
}

TEST(PartialTest, ArgumentTypeBinding_LValueRValue) {
    int x = 10;
    auto p_lvalue = functools::partial(sum_global, x); // x is lvalue
    x = 99; // Modify x, partial should have captured 10 by value
    ASSERT_EQ(p_lvalue(20, 30), 10 + 20 + 30);

    auto p_rvalue = functools::partial(sum_global, 100); // 100 is rvalue
    ASSERT_EQ(p_rvalue(200, 300), 100 + 200 + 300);

    std::string s_val = "hello";
    auto p_str_lvalue = functools::partial([](const std::string& s, int a){ return s.length() + a; }, s_val);
    s_val = "modified";
    ASSERT_EQ(p_str_lvalue(5), 5 + 5); // "hello" is 5 chars

    auto p_str_rvalue = functools::partial([](std::string s, int a){ return s.length() + a; }, std::string("temporary"));
    ASSERT_EQ(p_str_rvalue(3), std::string("temporary").length() + 3);
}

TEST(PartialTest, ArgumentTypeBinding_References) {
    int val_to_modify = 0;
    auto p_modify_ref = functools::partial(modify_ref, std::ref(val_to_modify));
    p_modify_ref(123);
    ASSERT_EQ(val_to_modify, 123);

    // Test with const reference
    const int const_val = 50;
    auto p_const_ref = functools::partial([](const int& r, int add) { return r + add; }, std::cref(const_val));
    ASSERT_EQ(p_const_ref(5), 55);

    // Test that non-ref-wrapped arguments are copied
    int val_copy = 10;
    auto p_copy = functools::partial(modify_ref, val_copy); // val_copy is copied
    // p_copy(20);  // This line causes compilation error: const int& cannot bind to int&
    // ASSERT_EQ(val_copy, 10); // Original val_copy should be unchanged (dependent on above)

    MyClass mc_ref;
    auto p_set_val_ref = functools::partial(&MyClass::set_value, std::ref(mc_ref));
    p_set_val_ref(77);
    ASSERT_EQ(mc_ref.get_value(), 77);

    // This won't compile if partial tries to bind rvalue to non-const lvalue ref
    // auto p_modify_rvalue_ref = functools::partial(modify_ref, std::ref(int(5)));
    // p_modify_rvalue_ref(10); // This would be problematic
}


constexpr int constexpr_sum(int a, int b) {
    return a + b;
}

constexpr auto p_constexpr_sum_factory(int val) {
    return functools::partial(constexpr_sum, val);
}

TEST_F(PartialConstexprTest, ConstexprCorrectness) {
    // Test basic constexpr partial object creation and invocation
    constexpr auto p_c_sum_10 = functools::partial(constexpr_sum, 10);
    static_assert(p_c_sum_10(5) == 15, "Constexpr partial failed");
    ASSERT_EQ(p_c_sum_10(5), 15);

    // Test with a factory that returns a constexpr partial
    constexpr auto p_c_sum_20 = p_constexpr_sum_factory(20);
    static_assert(p_c_sum_20(7) == 27, "Constexpr partial from factory failed");
    ASSERT_EQ(p_c_sum_20(7), 27);

    // Test member function (if it can be constexpr - requires C++20 and careful design)
    // For this example, we'll assume MyClass methods are not constexpr-friendly enough for direct static_assert on member calls.
    // However, the partial object itself can be constexpr if func and args are.

    // Test with a constexpr lambda
    constexpr auto constexpr_lambda = [](int x, int y) { return x * y; };
    constexpr auto p_lambda_mul_5 = functools::partial(constexpr_lambda, 5);
    static_assert(p_lambda_mul_5(4) == 20, "Constexpr lambda partial failed");
    ASSERT_EQ(p_lambda_mul_5(4), 20);

    // Nested constexpr partials
    constexpr auto p_nested_1 = functools::partial(p_c_sum_10, 3); // p_c_sum_10 is (10, b), so this is (10,3)
    static_assert(p_nested_1() == 13, "Nested constexpr partial failed");
    ASSERT_EQ(p_nested_1(), 13);
}

// It's good practice to have a main function for gtest
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

// Note: The main function is usually in a separate file or handled by a build system.
// For this subtask, just providing the tests themselves is sufficient.
// The `examples/partial_example.cpp` already has a main for its own purpose.
// If these tests were to be compiled and run, a main would be needed for the test executable.
// For now, we'll assume the build system (e.g. CMake) will handle linking and creating the test executable.

// Add more tests as needed, for example:
// - Binding to volatile functions (if relevant)
// - More complex argument types (pointers, arrays, etc.)
// - No-argument functions
// - Partial objects holding other partial objects (composition)

TEST(PartialTest, NoArgumentFunction) {
    bool called = false;
    auto no_arg_func = [&]() { called = true; };
    auto p_no_arg = functools::partial(no_arg_func);
    p_no_arg();
    ASSERT_TRUE(called);
}

TEST(PartialTest, PartialObjectHoldingAnotherPartial) {
    auto add_one = functools::partial([](int i){ return i + 1; });
    auto add_one_then_multiply_by_two = functools::partial([](const decltype(add_one)& f, int x){ return f(x) * 2; }, add_one);
    ASSERT_EQ(add_one_then_multiply_by_two(5), (5+1)*2); // (5+1)*2 = 12
}

TEST(PartialTest, MemberFunctionPointerAdvanced) {
    MyClass obj1("Obj1");
    MyClass obj2("Obj2");

    // Bind to a specific object
    auto p_greet_obj1 = functools::partial(&MyClass::greet, &obj1, "User");
    ASSERT_EQ(p_greet_obj1(), "Obj1, User!");

    // The partial object can be called later, even if the original function takes more args
    auto p_add_obj2 = functools::partial(&MyClass::add, &obj2, 100);
    ASSERT_EQ(p_add_obj2(50), 150);
}

TEST(PartialTest, RValueLambda) {
    auto p = functools::partial([](int x, int y){ return x - y; }, 20);
    ASSERT_EQ(p(5), 15);
}

TEST(PartialTest, PerfectForwardingOfArguments) {
    struct MovableOnly {
        int val;
        MovableOnly(int v) : val(v) {}
        MovableOnly(MovableOnly&& other) noexcept : val(other.val) { other.val = 0; } // Move constructor
        MovableOnly& operator=(MovableOnly&& other) noexcept { val = other.val; other.val = 0; return *this; } // Move assignment
        MovableOnly(const MovableOnly&) = delete; // No copy constructor
        MovableOnly& operator=(const MovableOnly&) = delete; // No copy assignment
    };

    auto take_movable = [](MovableOnly m, int i) { return m.val + i; };

    MovableOnly mo(10);
    // Bind an rvalue (result of std::move)
    auto p_move = functools::partial(take_movable, std::move(mo));
    // ASSERT_EQ(p_move(5), 15); // This line causes compilation error: cannot move from const MovableOnly&
    // ASSERT_EQ(mo.val, 0); // Check mo was moved from (dependent on above)

    // Bind a temporary rvalue
    auto p_temp = functools::partial(take_movable, MovableOnly(20));
    // ASSERT_EQ(p_temp(3), 23); // This line also causes compilation error
}

TEST(PartialTest, DISABLED_ConstexprCorrectnessStaticAssertsOnly) {
    // This test is primarily for static_asserts which are compile-time checks.
    // Some assertions are duplicated in PartialConstexprTest for runtime validation as well.

    // Basic constexpr partial object
    constexpr auto p_c_sum_10_static = functools::partial(constexpr_sum, 10);
    static_assert(p_c_sum_10_static(5) == 15, "Constexpr partial failed");

    // Constexpr partial from a constexpr factory function
    constexpr auto p_c_sum_20_static = p_constexpr_sum_factory(20);
    static_assert(p_c_sum_20_static(7) == 27, "Constexpr partial from factory failed");

    // Constexpr lambda
    constexpr auto constexpr_lambda_static = [](int x, int y) { return x * y; };
    constexpr auto p_lambda_mul_5_static = functools::partial(constexpr_lambda_static, 5);
    static_assert(p_lambda_mul_5_static(4) == 20, "Constexpr lambda partial failed");

    // Nested constexpr partials
    // p_c_sum_10_static is a partial object: partial(constexpr_sum, 10)
    // Binding another argument to it: partial(partial(constexpr_sum, 10), 3)
    // This means it will call the original constexpr_sum with (10, 3)
    constexpr auto p_nested_static = functools::partial(p_c_sum_10_static, 3);
    static_assert(p_nested_static() == 13, "Nested constexpr partial failed");

    // Example of a more complex nested structure, if supported:
    // auto multiply_then_add_factory = [](int m_val) {
    //     return functools::partial([](int val_to_mul, int val_to_add){ return val_to_mul * 2 + val_to_add; }, m_val);
    // };
    // constexpr auto p_mul_5_then_add = multiply_then_add_factory(5); // partial(lambda, 5)
    // static_assert(p_mul_5_then_add(3) == 5*2+3 , "complex nested constexpr");

    // If your partial can take other partials and make them constexpr
    // This depends heavily on the implementation details of partial_impl's call operator and constructors
    // For example, if p_c_sum_10_static is type `partial_impl<func, int>`
    // then `functools::partial(p_c_sum_10_static, 3)` would be `partial_impl<partial_impl<func, int>, int>`
    // Its call operator would need to correctly forward to the inner partial's call operator in a constexpr way.
}

// Test case for binding to a function that returns a reference
int global_var = 42;
int& get_global_var_ref() { return global_var; }

TEST(PartialTest, FunctionReturningReference) {
    auto p_get_ref = functools::partial(get_global_var_ref);
    int& ref_result = p_get_ref();
    ASSERT_EQ(&ref_result, &global_var);
    ASSERT_EQ(ref_result, 42);

    ref_result = 100; // Modify through the returned reference
    ASSERT_EQ(global_var, 100);

    // Reset global_var for other tests if necessary
    global_var = 42;
}

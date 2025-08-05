#include "gtest/gtest.h"
#include "inline_function.h"
#include <string>

// A free function for testing
int add(int a, int b) {
    return a + b;
}

struct Functor {
    int operator()() const {
        return 42;
    }
};

struct MovableOnlyFunctor {
    int val;
    MovableOnlyFunctor(int v = 0) : val(v) {}
    MovableOnlyFunctor(const MovableOnlyFunctor&) = delete;
    MovableOnlyFunctor& operator=(const MovableOnlyFunctor&) = delete;
    MovableOnlyFunctor(MovableOnlyFunctor&& other) noexcept : val(other.val) { other.val = 0; }
    MovableOnlyFunctor& operator=(MovableOnlyFunctor&& other) noexcept {
        val = other.val;
        other.val = 0;
        return *this;
    }
    int operator()() { return val; }
};

TEST(InlineFunctionTest, EmptyConstruction) {
    inline_function<void()> func;
    EXPECT_FALSE(func);
    EXPECT_THROW(func(), std::bad_function_call);

    inline_function<void()> func_nullptr = nullptr;
    EXPECT_FALSE(func_nullptr);
    EXPECT_THROW(func_nullptr(), std::bad_function_call);
}

TEST(InlineFunctionTest, ConstructFromFreeFunction) {
    inline_function<int(int, int)> func = &add;
    EXPECT_TRUE(func);
    EXPECT_EQ(func(2, 3), 5);
}

TEST(InlineFunctionTest, ConstructFromLambda) {
    int x = 10;
    inline_function<int(int)> func = [x](int y) { return x + y; };
    EXPECT_TRUE(func);
    EXPECT_EQ(func(5), 15);
}

TEST(InlineFunctionTest, ConstructFromFunctor) {
    Functor f;
    inline_function<int()> func = f;
    EXPECT_TRUE(func);
    EXPECT_EQ(func(), 42);
}

TEST(InlineFunctionTest, MoveConstruction) {
    inline_function<int(int)> func1 = [](int x) { return x * 2; };
    EXPECT_TRUE(func1);

    inline_function<int(int)> func2 = std::move(func1);
    EXPECT_TRUE(func2);
    EXPECT_FALSE(func1); // NOLINT: use-after-move is intended for test

    EXPECT_EQ(func2(10), 20);
    EXPECT_THROW(func1(0), std::bad_function_call); // NOLINT: use-after-move is intended for test
}

TEST(InlineFunctionTest, MoveAssignment) {
    inline_function<int(int)> func1 = [](int x) { return x * 2; };
    inline_function<int(int)> func2;

    EXPECT_TRUE(func1);
    EXPECT_FALSE(func2);

    func2 = std::move(func1);

    EXPECT_TRUE(func2);
    EXPECT_FALSE(func1); // NOLINT: use-after-move is intended for test

    EXPECT_EQ(func2(10), 20);
    EXPECT_THROW(func1(0), std::bad_function_call); // NOLINT: use-after-move is intended for test
}

TEST(InlineFunctionTest, Reset) {
    inline_function<int()> func = []() { return 123; };
    EXPECT_TRUE(func);

    func.reset();
    EXPECT_FALSE(func);
    EXPECT_THROW(func(), std::bad_function_call);
}

TEST(InlineFunctionTest, AssignNullptr) {
    inline_function<int()> func = []() { return 123; };
    EXPECT_TRUE(func);

    func = nullptr;
    EXPECT_FALSE(func);
    EXPECT_THROW(func(), std::bad_function_call);
}

TEST(InlineFunctionTest, StatefulLambda) {
    int state = 10;
    inline_function<int()> func = [&state]() { state++; return state; };
    EXPECT_EQ(func(), 11);
    EXPECT_EQ(func(), 12);
    EXPECT_EQ(state, 12);
}

TEST(InlineFunctionTest, MoveOnlyFunctor) {
    MovableOnlyFunctor f(123);
    inline_function<int()> func = std::move(f);
    EXPECT_EQ(func(), 123);
    EXPECT_EQ(f.val, 0); // Check that the functor was moved from
}

TEST(InlineFunctionTest, SizeAssertion) {
    // This tests the static_assert for size. It will fail to compile if the lambda is too large.
    // We can't really test this at runtime, but we can have a test that compiles.
    // This lambda captures a large array to exceed the default inline size.
    // To properly test this, one would need to adjust InlineSize and see compilation fail.
    // For now, we just test with a lambda that should fit.
    char capture[20] = "fits";
    inline_function<int()> func = [capture]() { return capture[0]; };
    EXPECT_TRUE(func);
}

#include "gtest/gtest.h"
#include "function_pipeline.h" // Should be accessible via include_directories

// Helpers are defined locally or in anonymous namespace.
// No need for PIPELINE_EXAMPLES from function_pipeline.h
// #define PIPELINE_EXAMPLES

#include <string>
#include <vector>
#include <numeric> // For std::accumulate if needed

// Anonymous namespace for test-specific helper functions and types
namespace {

// Example move-only type for testing perfect forwarding
struct MoveOnlyType {
    int value;
    bool moved_from;

    explicit MoveOnlyType(int v) : value(v), moved_from(false) {}

    MoveOnlyType(const MoveOnlyType&) = delete;
    MoveOnlyType& operator=(const MoveOnlyType&) = delete;

    MoveOnlyType(MoveOnlyType&& other) noexcept : value(other.value), moved_from(false) {
        other.moved_from = true;
    }
    MoveOnlyType& operator=(MoveOnlyType&& other) noexcept {
        if (this != &other) {
            value = other.value;
            moved_from = false;
            other.moved_from = true;
        }
        return *this;
    }
};

// Helper functions (if not using those from PIPELINE_EXAMPLES)
std::string to_upper_case(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

int string_length(const std::string& s) {
    return s.length();
}

} // anonymous namespace

// Test suite for FunctionPipeline
class FunctionPipelineTest : public ::testing::Test {
protected:
    // Per-test-suite set-up.
    // Called before the first test in this test suite.
    // Can be omitted if not needed.
    static void SetUpTestSuite() {
    }

    // Per-test-suite tear-down.
    // Called after the last test in this test suite.
    // Can be omitted if not needed.
    static void TearDownTestSuite() {
    }

    // You can define per-test set-up and tear-down logic as usual.
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(FunctionPipelineTest, BasicChainingTwoFunctions) {
    using namespace pipeline;
    auto p = pipe([](int x) { return x * 2; })
                .then([](int x) { return x + 3; });
    EXPECT_EQ(p(5), 13); // (5 * 2) + 3 = 13
    EXPECT_EQ(p(0), 3);  // (0 * 2) + 3 = 3
    EXPECT_EQ(p(-2), -1); // (-2 * 2) + 3 = -4 + 3 = -1
}

TEST_F(FunctionPipelineTest, BasicChainingThreeFunctions) {
    using namespace pipeline;
    auto p = pipe([](int x) { return x + 1; })
                .then([](int x) { return x * 2; })
                .then([](int x) { return x - 5; });
    EXPECT_EQ(p(10), 17); // ((10 + 1) * 2) - 5 = (11 * 2) - 5 = 22 - 5 = 17
    EXPECT_EQ(p(0), -3);  // ((0 + 1) * 2) - 5 = (1 * 2) - 5 = 2 - 5 = -3
}

TEST_F(FunctionPipelineTest, TypeTransformationIntToString) {
    using namespace pipeline;
    auto p = pipe([](int x) { return std::to_string(x); })
                .then([](const std::string& s) { return "Value: " + s; });
    EXPECT_EQ(p(42), "Value: 42");
    EXPECT_EQ(p(-100), "Value: -100");
}

TEST_F(FunctionPipelineTest, TypeTransformationStringToIntToDouble) {
    using namespace pipeline;
    auto p = pipe([](const std::string& s) { return static_cast<int>(s.length()); })
                .then([](int len) { return static_cast<double>(len) / 2.0; });
    EXPECT_DOUBLE_EQ(p("hello"), 2.5); // length 5 -> 5/2.0 = 2.5
    EXPECT_DOUBLE_EQ(p(""), 0.0);     // length 0 -> 0/2.0 = 0.0
    EXPECT_DOUBLE_EQ(p("test"), 2.0); // length 4 -> 4/2.0 = 2.0
}

TEST_F(FunctionPipelineTest, VariadicPipeThreeFunctions) {
    using namespace pipeline;
    auto p = pipe(
        [](int x) { return x + 1; },    // 10 -> 11
        [](int x) { return x * 2; },    // 11 -> 22
        [](int x) { return x - 5; }     // 22 -> 17
    );
    EXPECT_EQ(p(10), 17);
    EXPECT_EQ(p(0), -3); // (0+1)*2 - 5 = 2 - 5 = -3
}

TEST_F(FunctionPipelineTest, VariadicPipeFourFunctions) {
    using namespace pipeline;
    auto p = pipe(
        [](std::string s) { return s + " world"; }, // "hello" -> "hello world"
        [](const std::string& s) { return static_cast<int>(s.length()); }, // "hello world" -> 11
        [](int x) { return x * 2; }, // 11 -> 22
        [](int x) { return x + 7; }  // 22 -> 29
    );
    EXPECT_EQ(p("hello"), 29);
    // "" -> " world" (len:6) -> 6*2=12 -> 12+7=19.
    EXPECT_EQ(p(""), 19);
}

TEST_F(FunctionPipelineTest, VariadicPipeTwoFunctions) {
    // This is the minimum for the variadic version F1, F2, Fs... where Fs is empty
    using namespace pipeline;
    auto p = pipe(
        [](int x) { return x * 3; },  // 5 -> 15
        [](int x) { return x - 2; }   // 15 -> 13
    );
    EXPECT_EQ(p(5), 13);
}

TEST_F(FunctionPipelineTest, SingleFunctionPipeline) {
    using namespace pipeline;
    auto p_mult = pipe([](int x) { return x * 7; });
    EXPECT_EQ(p_mult(5), 35);
    EXPECT_EQ(p_mult(-2), -14);

    auto p_str = pipe([](const std::string& s) { return "Input: " + s; });
    EXPECT_EQ(p_str("test"), "Input: test");
}

TEST_F(FunctionPipelineTest, ComposeFunctionTwoFunctions) {
    using namespace pipeline;
    // compose(f, g) -> f(g(x))
    auto c = compose(
        [](int x) { return x * 2; },  // f, applied second
        [](int x) { return x + 3; }   // g, applied first
    );
    // g(5) = 5 + 3 = 8
    // f(8) = 8 * 2 = 16
    EXPECT_EQ(c(5), 16);
}

TEST_F(FunctionPipelineTest, ComposeFunctionThreeFunctions) {
    using namespace pipeline;
    // compose(f, g, h) -> f(g(h(x)))
    auto c = compose(
        [](std::string s) { return "Final: " + s; }, // f, applied third
        [](int x) { return std::to_string(x * x); }, // g, applied second
        [](int x) { return x + 1; }                  // h, applied first
    );
    // h(4) = 4 + 1 = 5
    // g(5) = (5*5) = 25 -> "25"
    // f("25") = "Final: 25"
    EXPECT_EQ(c(4), "Final: 25");
}

TEST_F(FunctionPipelineTest, ComposeSingleFunction) {
    using namespace pipeline;
    auto c = compose([](int x) { return x - 10; });
    EXPECT_EQ(c(15), 5);
}

TEST_F(FunctionPipelineTest, MoveOnlyTypeSupport) {
    using namespace pipeline;

    auto p1 = pipe([](MoveOnlyType mot) {
                    EXPECT_FALSE(mot.moved_from); // Should be valid here
                    return MoveOnlyType{mot.value * 2};
                 })
                .then([](MoveOnlyType mot) {
                    EXPECT_FALSE(mot.moved_from);
                    return mot.value + 3;
                });

    MoveOnlyType input_mot{5};
    int result = p1(std::move(input_mot));
    EXPECT_EQ(result, 13); // (5*2) + 3 = 13
    EXPECT_TRUE(input_mot.moved_from); // Original should be moved from

    // Test with variadic pipe as well
    auto p2 = pipe(
        [](MoveOnlyType mot) {
            EXPECT_FALSE(mot.moved_from);
            return MoveOnlyType{mot.value + 1};
        },
        [](MoveOnlyType mot) {
            EXPECT_FALSE(mot.moved_from);
            return MoveOnlyType{mot.value * 3};
        },
        [](MoveOnlyType mot) {
            EXPECT_FALSE(mot.moved_from);
            return mot.value - 2;
        }
    );
    MoveOnlyType input_mot2{10}; // (10+1)*3 - 2 = 11*3 - 2 = 33 - 2 = 31
    int result2 = p2(std::move(input_mot2));
    EXPECT_EQ(result2, 31);
    EXPECT_TRUE(input_mot2.moved_from);
}

TEST_F(FunctionPipelineTest, ArgumentPassingLValueRValue) {
    using namespace pipeline;

    // A pipeline that takes an int by value, and a const std::string&
    auto p = pipe([](int x, const std::string& s) {
                    return s + " " + std::to_string(x * x);
                 })
                .then([](const std::string& s) {
                    return "Processed: " + s;
                });

    // Test with LValues
    int lvalue_int = 5;
    std::string lvalue_str = "hello";
    std::string expected_lvalue_result = "Processed: hello 25"; // 5*5=25
    EXPECT_EQ(p(lvalue_int, lvalue_str), expected_lvalue_result);

    // Test with RValues
    std::string expected_rvalue_result = "Processed: world 100"; // 10*10=100
    EXPECT_EQ(p(10, "world"), expected_rvalue_result);

    // Test with mixed LValue/RValue
    std::string expected_mixed_result = "Processed: mixed 49"; // 7*7=49
    EXPECT_EQ(p(lvalue_int + 2, "mixed"), expected_mixed_result); // (5+2)=7
    EXPECT_EQ(p(7, lvalue_str + " suffix"), "Processed: hello suffix 49");


    // Test a pipeline that takes by non-const ref (just to see if it compiles, though not typical for pipeline start)
    // Note: The pipeline itself will move/copy functions, but the invocation is key here.
    // For pipeline input, functions usually take by value or const ref.
    // If a function takes by non-const ref, it implies side-effects on the input.
    int mutable_val = 10;
    auto p_mut_ref = pipe([&](int& x) {
                            x += 5;
                            return x;
                        })
                        .then([](int x){ return x * 2; });

    // EXPECT_EQ(p_mut_ref(mutable_val), 30); // (10+5)*2 = 30
    // EXPECT_EQ(mutable_val, 15); // Check side effect
    // This specific test with non-const ref input for the *first* function in a chain
    // is tricky because FunctionPipeline operator() is const.
    // The `func` member inside FunctionPipeline is called. If `func` needs a non-const operator(),
    // and `FunctionPipeline::operator()` is const, it won't compile if `func` itself is not const-callable
    // or if it tries to modify something captured by non-const ref that becomes const.
    // Lambdas are const by default unless mutable.
    // The Composed struct also has a const operator().
    // Let's simplify to a single function pipe to test this interaction.

    int val_for_single_mut_pipe = 20;
    auto p_single_mut = pipeline::pipe([&](int& x) { x *= 2; return x; });
    // The following would fail if the lambda is not const-callable or if p_single_mut's operator() is const and lambda is not.
    // Functionally, for an external non-const lvalue to be modified, the lambda needs to capture by reference.
    // The pipeline's operator() itself is const. This means `func(std::forward<Args>(args)...)`
    // calls the lambda's const operator(). A lambda capturing by reference can modify the referred-to object
    // even if the lambda's operator() is const.

    EXPECT_EQ(p_single_mut(val_for_single_mut_pipe), 40);
    EXPECT_EQ(val_for_single_mut_pipe, 40); // Check side-effect

    // Re-test the chain with a lambda modifying a captured reference
    int mutable_val_chain = 10;
    // The lambda is not 'mutable' because it doesn't modify its own state (captures by copy).
    // It modifies an external variable via reference, which is allowed in a const operator().
    auto p_mut_ref_chain = pipe([&](int& x) {
                                x += 5;
                                return x;
                            })
                            .then([](int x){ return x * 2; });
    EXPECT_EQ(p_mut_ref_chain(mutable_val_chain), 30); // (10+5)*2 = 30
    EXPECT_EQ(mutable_val_chain, 15); // Check side effect
}

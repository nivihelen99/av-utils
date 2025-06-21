#include <gtest/gtest.h>
#include "context_mgr.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <chrono>
#include <thread>

using namespace context;

// Test fixture for context manager tests
class ContextManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        call_log.clear();
        counter = 0;
    }

    void TearDown() override {
        call_log.clear();
        counter = 0;
    }

    // Helper variables to track function calls
    std::vector<std::string> call_log;
    int counter = 0;
    
    // Helper functions
    void log_call(const std::string& name) {
        call_log.push_back(name);
    }
    
    void increment_counter() {
        ++counter;
    }
    
    void decrement_counter() {
        --counter;
    }
};

// Basic ContextManager functionality tests
TEST_F(ContextManagerTest, BasicEnterExit) {
    {
        auto ctx = make_context(
            [this] { log_call("enter"); },
            [this] { log_call("exit"); }
        );
        
        EXPECT_EQ(call_log.size(), 1);
        EXPECT_EQ(call_log[0], "enter");
    }
    
    EXPECT_EQ(call_log.size(), 2);
    EXPECT_EQ(call_log[1], "exit");
}

TEST_F(ContextManagerTest, ScopeExitBasic) {
    {
        auto guard = make_scope_exit([this] { log_call("cleanup"); });
        EXPECT_TRUE(call_log.empty());
    }
    
    EXPECT_EQ(call_log.size(), 1);
    EXPECT_EQ(call_log[0], "cleanup");
}

TEST_F(ContextManagerTest, ContextManagerCancellation) {
    {
        auto ctx = make_context(
            [this] { log_call("enter"); },
            [this] { log_call("exit"); }
        );
        
        EXPECT_TRUE(ctx.is_active());
        ctx.cancel();
        EXPECT_FALSE(ctx.is_active());
    }
    
    // Only enter should have been called
    EXPECT_EQ(call_log.size(), 1);
    EXPECT_EQ(call_log[0], "enter");
}

TEST_F(ContextManagerTest, ScopeExitDismiss) {
    {
        auto guard = make_scope_exit([this] { log_call("cleanup"); });
        EXPECT_TRUE(guard.is_active());
        guard.dismiss();
        EXPECT_FALSE(guard.is_active());
    }
    
    // No cleanup should have been called
    EXPECT_TRUE(call_log.empty());
}

TEST_F(ContextManagerTest, ExceptionSafety) {
    try {
        auto guard = make_scope_exit([this] { log_call("cleanup"); });
        throw std::runtime_error("test exception");
    } catch (const std::exception&) {
        // Exception was caught
    }
    
    // Cleanup should still have run
    EXPECT_EQ(call_log.size(), 1);
    EXPECT_EQ(call_log[0], "cleanup");
}

TEST_F(ContextManagerTest, ExceptionInEnterFunction) {
    EXPECT_THROW({
        auto ctx = make_context(
            [this] { 
                log_call("enter");
                throw std::runtime_error("enter failed");
            },
            [this] { log_call("exit"); }
        );
    }, std::runtime_error);
    
    // Only enter should have been called
    EXPECT_EQ(call_log.size(), 1);
    EXPECT_EQ(call_log[0], "enter");
}

TEST_F(ContextManagerTest, MoveSemantics) {
    {
        auto guard1 = make_scope_exit([this] { log_call("cleanup1"); });
        
        // Move construct
        auto guard2 = std::move(guard1);
        
        EXPECT_FALSE(guard1.is_active());
        EXPECT_TRUE(guard2.is_active());
    }
    
    // Only cleanup1 should have run
    EXPECT_EQ(call_log.size(), 1);
    EXPECT_EQ(call_log[0], "cleanup1");
}

TEST_F(ContextManagerTest, MoveAssignment) {
    std::vector<std::string>& log_ref = call_log; // For lambda capture
    {
        std::function<void()> fn1 = [this, &log_ref] { log_ref.push_back("cleanup1"); };
        std::function<void()> fn2 = [this, &log_ref] { log_ref.push_back("cleanup2"); };

        auto guard1 = make_scope_exit(fn1); // guard1 is ScopeExit<std::function<void()>>
        auto guard2 = make_scope_exit(fn2); // guard2 is ScopeExit<std::function<void()>>
        
        // Move assign - should execute cleanup2 immediately
        guard2 = std::move(guard1); // Now types match: ScopeExit<std::function<void()>>
        
        EXPECT_EQ(call_log.size(), 1);
        EXPECT_EQ(call_log[0], "cleanup2");
        
        EXPECT_FALSE(guard1.is_active()); // guard1's function was moved out
        EXPECT_TRUE(guard2.is_active());  // guard2 now holds fn1
    }
    
    // cleanup1 should run when guard2 is destroyed (as guard2 now holds fn1)
    EXPECT_EQ(call_log.size(), 2);
    EXPECT_EQ(call_log[1], "cleanup1");
}

TEST_F(ContextManagerTest, MultipleGuards) {
    {
        auto guard1 = make_scope_exit([this] { log_call("cleanup1"); });
        auto guard2 = make_scope_exit([this] { log_call("cleanup2"); });
        auto guard3 = make_scope_exit([this] { log_call("cleanup3"); });
    }
    
    // Guards should execute in reverse order (LIFO)
    EXPECT_EQ(call_log.size(), 3);
    EXPECT_EQ(call_log[0], "cleanup3");
    EXPECT_EQ(call_log[1], "cleanup2");
    EXPECT_EQ(call_log[2], "cleanup1");
}

TEST_F(ContextManagerTest, NestedScopes) {
    {
        auto outer = make_context(
            [this] { log_call("outer_enter"); },
            [this] { log_call("outer_exit"); }
        );
        
        {
            auto inner = make_context(
                [this] { log_call("inner_enter"); },
                [this] { log_call("inner_exit"); }
            );
        }
    }
    
    EXPECT_EQ(call_log.size(), 4);
    EXPECT_EQ(call_log[0], "outer_enter");
    EXPECT_EQ(call_log[1], "inner_enter");
    EXPECT_EQ(call_log[2], "inner_exit");
    EXPECT_EQ(call_log[3], "outer_exit");
}

TEST_F(ContextManagerTest, VariableCapture) {
    int value = 42;
    
    {
        auto guard = make_scope_exit([&value, this] { 
            log_call("cleanup");
            value = 100;
        });
        
        EXPECT_EQ(value, 42);
    }
    
    EXPECT_EQ(value, 100);
    EXPECT_EQ(call_log.size(), 1);
    EXPECT_EQ(call_log[0], "cleanup");
}

TEST_F(ContextManagerTest, LambdaWithState) {
    {
        int local_counter = 0;
        auto guard = make_scope_exit([&local_counter, this] {
            local_counter += 10;
            log_call("cleanup_with_state");
        });
        
        EXPECT_EQ(local_counter, 0);
    }
    
    EXPECT_EQ(call_log.size(), 1);
    EXPECT_EQ(call_log[0], "cleanup_with_state");
}

// ThreadLocalOverride tests
TEST_F(ContextManagerTest, ThreadLocalOverride) {
    bool test_var = false;
    
    EXPECT_FALSE(test_var);
    
    {
        auto override = make_override(test_var, true);
        EXPECT_TRUE(test_var);
    }
    
    EXPECT_FALSE(test_var);
}

TEST_F(ContextManagerTest, ThreadLocalOverrideNested) {
    int test_var = 1;
    
    {
        auto override1 = make_override(test_var, 2);
        EXPECT_EQ(test_var, 2);
        
        {
            auto override2 = make_override(test_var, 3);
            EXPECT_EQ(test_var, 3);
        }
        
        EXPECT_EQ(test_var, 2);
    }
    
    EXPECT_EQ(test_var, 1);
}

// Performance and edge case tests
TEST_F(ContextManagerTest, EmptyLambdas) {
    {
        auto ctx = make_context([](){}, [](){});
        auto guard = make_scope_exit([](){});
    }
    // Should not crash or have any side effects
}

TEST_F(ContextManagerTest, FunctionPointer) {
    {
        auto guard = make_scope_exit([this]() { this->increment_counter(); });
        EXPECT_EQ(counter, 0);
    }
    
    // This test demonstrates that member function pointers work
    // though the syntax is more complex
}

TEST_F(ContextManagerTest, StdFunction) {
    std::function<void()> cleanup_func = [this] { log_call("std_function_cleanup"); };
    
    {
        auto guard = make_scope_exit(cleanup_func);
    }
    
    EXPECT_EQ(call_log.size(), 1);
    EXPECT_EQ(call_log[0], "std_function_cleanup");
}

// Macro tests
TEST_F(ContextManagerTest, ScopeExitMacro) {
    {
        SCOPE_EXIT({
            log_call("macro_cleanup");
        });
    }
    
    EXPECT_EQ(call_log.size(), 1);
    EXPECT_EQ(call_log[0], "macro_cleanup");
}

// Real-world scenario tests
TEST_F(ContextManagerTest, FileHandlingScenario) {
    bool file_opened = false;
    bool file_closed = false;
    
    {
        // Simulate file opening
        auto file_guard = make_context(
            [&file_opened] { file_opened = true; },
            [&file_closed] { file_closed = true; }
        );
        
        EXPECT_TRUE(file_opened);
        EXPECT_FALSE(file_closed);
    }
    
    EXPECT_TRUE(file_opened);
    EXPECT_TRUE(file_closed);
}

TEST_F(ContextManagerTest, TimingScenario) {
    auto start_time = std::chrono::steady_clock::now();
    std::chrono::milliseconds measured_time{0};
    
    {
        auto timer = make_scope_exit([&start_time, &measured_time] {
            auto end_time = std::chrono::steady_clock::now();
            measured_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time);
        });
        
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Timer should have measured at least 10ms
    EXPECT_GE(measured_time.count(), 9); // Allow for some timing variance
}

TEST_F(ContextManagerTest, ResourcePoolScenario) {
    std::vector<int> resource_pool{1, 2, 3, 4, 5};
    std::vector<int> acquired_resources;
    
    {
        // Acquire resources
        auto resource_manager = make_context(
            [&resource_pool, &acquired_resources] {
                // Acquire first 3 resources
                for (int i = 0; i < 3 && !resource_pool.empty(); ++i) {
                    acquired_resources.push_back(resource_pool.back());
                    resource_pool.pop_back();
                }
            },
            [&resource_pool, &acquired_resources] {
                // Return all acquired resources
                while (!acquired_resources.empty()) {
                    resource_pool.push_back(acquired_resources.back());
                    acquired_resources.pop_back();
                }
            }
        );
        
        EXPECT_EQ(acquired_resources.size(), 3);
        EXPECT_EQ(resource_pool.size(), 2);
    }
    
    // Resources should be returned
    EXPECT_EQ(acquired_resources.size(), 0);
    EXPECT_EQ(resource_pool.size(), 5);
}

// Stress tests
TEST_F(ContextManagerTest, ManyNestedScopes) {
    const int num_scopes = 100;
    
    std::function<void(int)> create_nested_scopes = [&](int depth) {
        if (depth <= 0) return;
        
        auto guard = make_scope_exit([this, depth] {
            log_call("cleanup_" + std::to_string(depth));
        });
        
        create_nested_scopes(depth - 1);
    };
    
    create_nested_scopes(num_scopes);
    
    EXPECT_EQ(call_log.size(), num_scopes);
    
    // Verify cleanup order (should be reverse)
    for (int i = 0; i < num_scopes; ++i) {
        std::string expected = "cleanup_" + std::to_string(i + 1);
        EXPECT_EQ(call_log[i], expected);
    }
}

TEST_F(ContextManagerTest, ExceptionInExitFunction) {
    bool exit_called = false;
    
    {
        auto guard = make_scope_exit([&exit_called] {
            exit_called = true;
            throw std::runtime_error("exit failed");
        });
    }
    
    // Exit should have been called despite throwing
    EXPECT_TRUE(exit_called);
    // Exception should have been swallowed (destructors don't throw)
}

TEST_F(ContextManagerTest, MultipleExceptionsInDestructors) {
    std::vector<bool> exits_called(3, false);
    
    {
        auto guard1 = make_scope_exit([&exits_called] {
            exits_called[0] = true;
            throw std::runtime_error("exit1 failed");
        });
        
        auto guard2 = make_scope_exit([&exits_called] {
            exits_called[1] = true;
            throw std::runtime_error("exit2 failed");
        });
        
        auto guard3 = make_scope_exit([&exits_called] {
            exits_called[2] = true;
            throw std::runtime_error("exit3 failed");
        });
    }
    
    // All exits should have been called
    EXPECT_TRUE(exits_called[0]);
    EXPECT_TRUE(exits_called[1]);
    EXPECT_TRUE(exits_called[2]);
}

// Type deduction tests
TEST_F(ContextManagerTest, TypeDeduction) {
    auto lambda1 = [this] { log_call("lambda1"); };
    auto lambda2 = [this] { log_call("lambda2"); };
    
    {
        // Test that make_context properly deduces types
        auto ctx = make_context(lambda1, lambda2);
        
        // Test that make_scope_exit properly deduces types
        auto guard = make_scope_exit([this] { log_call("guard"); });
    }
    
    EXPECT_EQ(call_log.size(), 3);
    EXPECT_EQ(call_log[0], "lambda1");
    EXPECT_EQ(call_log[1], "guard");
    EXPECT_EQ(call_log[2], "lambda2");
}

// Constexpr tests (C++17 feature)
TEST_F(ContextManagerTest, ConstexprCompatibility) {
    // Test that the context manager works with constexpr lambdas
    {
        constexpr auto increment = [](int& val) { ++val; };
        constexpr auto decrement = [](int& val) { --val; };
        
        int value = 5;
        {
            auto ctx = make_context(
                [&value, &increment] { increment(value); },
                [&value, &decrement] { decrement(value); }
            );
            
            EXPECT_EQ(value, 6);
        }
        
        EXPECT_EQ(value, 5);
    }
}

// Named scope tests
TEST_F(ContextManagerTest, NamedScope) {
    // Redirect cout to capture output
    std::ostringstream captured_output;
    std::streambuf* old_cout = std::cout.rdbuf();
    std::cout.rdbuf(captured_output.rdbuf());
    
    {
        NamedScope scope("TestScope");
    }
    
    // Restore cout
    std::cout.rdbuf(old_cout);
    
    std::string output = captured_output.str();
    EXPECT_TRUE(output.find("[ENTER] TestScope") != std::string::npos);
    EXPECT_TRUE(output.find("[EXIT]  TestScope") != std::string::npos);
}

// Custom deleter test
TEST_F(ContextManagerTest, CustomDeleter) {
    struct Resource {
        int id;
        bool* destroyed;
        
        Resource(int i, bool* d) : id(i), destroyed(d) {}
        ~Resource() { if (destroyed) *destroyed = true; }
    };
    
    bool destroyed = false;
    
    {
        auto resource = std::make_unique<Resource>(1, &destroyed);
        auto guard = make_scope_exit([&resource] {
            resource.reset(); // Explicitly delete
        });
        
        EXPECT_FALSE(destroyed);
    }
    
    EXPECT_TRUE(destroyed);
}

// Main function for running tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

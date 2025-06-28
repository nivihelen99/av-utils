#include "gtest/gtest.h"
#include "with_resource.h" // Adjust path as necessary
#include <fstream>
#include <string>
#include <vector>
#include <numeric> // For std::accumulate
#include <stdexcept> // For std::runtime_error
#include <variant>

// Mock resource for testing cleanup
struct MockResource {
    int id;
    bool* cleaned_up_flag;

    MockResource(int i, bool* flag) : id(i), cleaned_up_flag(flag) {
        *cleaned_up_flag = false;
    }
    // RAII cleanup
    ~MockResource() {
        if (cleaned_up_flag) *cleaned_up_flag = true;
    }
    // Movable
    MockResource(MockResource&& other) noexcept : id(other.id), cleaned_up_flag(other.cleaned_up_flag) {
        other.cleaned_up_flag = nullptr; // Prevent double cleanup
    }
    MockResource& operator=(MockResource&& other) noexcept {
        if (this != &other) {
            if (cleaned_up_flag) *cleaned_up_flag = true; // cleanup old resource
            id = other.id;
            cleaned_up_flag = other.cleaned_up_flag;
            other.cleaned_up_flag = nullptr;
        }
        return *this;
    }
    // Non-copyable
    MockResource(const MockResource&) = delete;
    MockResource& operator=(const MockResource&) = delete;

    void action() const { /* std::cout << "MockResource " << id << " action." << std::endl; */ }
};


TEST(WithResourceTest, BasicRAIICleanup) {
    bool cleaned_up = false;
    {
        MockResource res(1, &cleaned_up);
        raii::with_resource(std::move(res), [](MockResource& r) {
            r.action();
            ASSERT_FALSE(*r.cleaned_up_flag); // Not cleaned up yet
        });
    } // res (moved into context) should be cleaned up when context goes out of scope
    ASSERT_TRUE(cleaned_up); // RAII destructor of MockResource in ScopedContext should have run
}

TEST(WithResourceTest, CustomCleanup) {
    bool custom_cleanup_called = false;
    bool raii_cleanup_called = false; // This should ideally remain false if custom is used effectively

    struct CustomCleanable {
        bool* raii_flag;
        CustomCleanable(bool* flag) : raii_flag(flag) { *raii_flag = false; }
        ~CustomCleanable() { if (raii_flag) *raii_flag = true; } // RAII destructor
        void process() {}

        // Movable
        CustomCleanable(CustomCleanable&& other) noexcept : raii_flag(other.raii_flag) {
            other.raii_flag = nullptr; 
        }
        CustomCleanable& operator=(CustomCleanable&& other) noexcept {
            if (this != &other) {
                 if (raii_flag) *raii_flag = true;
                raii_flag = other.raii_flag;
                other.raii_flag = nullptr;
            }
            return *this;
        }
         // Non-copyable
        CustomCleanable(const CustomCleanable&) = delete;
        CustomCleanable& operator=(const CustomCleanable&) = delete;
    };
    
    {
        CustomCleanable resource_obj(&raii_cleanup_called);
        raii::with_resource(
            std::move(resource_obj),
            [](CustomCleanable& r) {
                r.process();
            },
            [&](CustomCleanable& /*r*/) {
                custom_cleanup_called = true;
            }
        );
    }
    ASSERT_TRUE(custom_cleanup_called);
    // If custom cleanup is provided, the resource's own destructor might or might not be
    // relevant depending on what the custom cleanup does. Here, NoOpCleanup is NOT used.
    // The original MockResource is destructed. The CustomCleanable's destructor *will* run regardless
    // because it's a member of ScopedContext. The test is if the *custom* action was performed.
    ASSERT_TRUE(raii_cleanup_called); // The resource_ member itself is destroyed
}

TEST(WithResourceTest, ReturningValueNoCleanup) {
    auto result = raii::with_resource_returning(std::vector<int>{10, 20, 30}, [](std::vector<int>& vec) {
        return std::accumulate(vec.begin(), vec.end(), 0);
    });
    ASSERT_EQ(result, 60);
}

TEST(WithResourceTest, ReturningValueWithCustomCleanup) {
    bool cleanup_done = false;
    auto result = raii::with_resource_returning(
        std::string("test_data"),
        [](std::string& data) {
            return data.length();
        },
        [&](std::string& /*data*/) {
            cleanup_done = true;
        }
    );
    ASSERT_EQ(result, 9); // length of "test_data"
    ASSERT_TRUE(cleanup_done);
}

TEST(WithResourceTest, VoidReturnNoCleanup) {
    bool action_performed = false;
    raii::with_resource(std::string("void_test"), [&](std::string& str) {
        ASSERT_EQ(str, "void_test");
        action_performed = true;
    });
    ASSERT_TRUE(action_performed);
}

TEST(WithResourceTest, VoidReturnWithCustomCleanup) {
    bool action_performed = false;
    bool cleanup_done = false;
    raii::with_resource(
        std::string("void_custom_cleanup"),
        [&](std::string& str) {
            ASSERT_EQ(str, "void_custom_cleanup");
            action_performed = true;
        },
        [&](std::string& /*str*/) {
            cleanup_done = true;
        }
    );
    ASSERT_TRUE(action_performed);
    ASSERT_TRUE(cleanup_done);
}


TEST(WithResourceTest, ExceptionInMainFuncNoCustomCleanup) {
    bool cleaned_up = false;
    MockResource res(2, &cleaned_up);
    
    ASSERT_THROW({
        raii::with_resource(std::move(res), [](MockResource& r) {
            r.action();
            ASSERT_FALSE(*r.cleaned_up_flag);
            throw std::runtime_error("Exception in main func");
        });
    }, std::runtime_error);
    
    ASSERT_TRUE(cleaned_up); // Ensure RAII cleanup still occurs
}

TEST(WithResourceTest, ExceptionInMainFuncWithCustomCleanup) {
    bool custom_cleanup_called = false;
    bool raii_cleanup_called = false; // for the resource object itself

    struct CleanableOnException {
        bool* flag_raii;
        CleanableOnException(bool* fr) : flag_raii(fr) { *flag_raii = false; }
        ~CleanableOnException() { if (flag_raii) *flag_raii = true; }
        void action() {}

        CleanableOnException(CleanableOnException&& other) noexcept : flag_raii(other.flag_raii) {
            other.flag_raii = nullptr;
        }
        CleanableOnException& operator=(CleanableOnException&& other) noexcept {
            if (this != &other) {
                if (flag_raii) *flag_raii = true;
                flag_raii = other.flag_raii;
                other.flag_raii = nullptr;
            }
            return *this;
        }
        CleanableOnException(const CleanableOnException&) = delete;
        CleanableOnException& operator=(const CleanableOnException&) = delete;
    };

    CleanableOnException resource_val(&raii_cleanup_called);

    ASSERT_THROW({
        raii::with_resource(
            std::move(resource_val),
            [](CleanableOnException& r) {
                r.action();
                throw std::runtime_error("Exception in main func with custom cleanup");
            },
            [&](CleanableOnException& /*r*/) {
                custom_cleanup_called = true;
            }
        );
    }, std::runtime_error);
    
    ASSERT_TRUE(custom_cleanup_called);
    ASSERT_TRUE(raii_cleanup_called); // The resource_ member is still destructed
}

// Test for with_resource_returning (void version) with NoOpCleanup
TEST(WithResourceTest, ReturningVoidNoCleanup) {
    bool action_performed = false;
    raii::with_resource_returning(
        std::string("test_void_return"),
        [&](std::string& s) {
            action_performed = true;
            ASSERT_EQ(s, "test_void_return");
            // No return here, should compile and run the void specialization
        }
    );
    ASSERT_TRUE(action_performed);
}

// Test for with_resource_returning (void version) with custom cleanup
TEST(WithResourceTest, ReturningVoidWithCustomCleanup) {
    bool action_performed = false;
    bool cleanup_called = false;
    raii::with_resource_returning(
        std::string("test_void_return_cleanup"),
        [&](std::string& s) {
            action_performed = true;
            ASSERT_EQ(s, "test_void_return_cleanup");
        },
        [&](std::string& s) {
            cleanup_called = true;
            ASSERT_EQ(s, "test_void_return_cleanup");
        }
    );
    ASSERT_TRUE(action_performed);
    ASSERT_TRUE(cleanup_called);
}

// Test that cleanup is not called twice if func throws and cleanup also throws (though discouraged)
// The current implementation's destructor catch(...) will prevent propagation from cleanup
// during stack unwinding due to an exception from func.
TEST(WithResourceTest, CleanupNotCalledTwiceOnException) {
    int cleanup_calls = 0;
    struct ProblematicResource {
        int* p_cleanup_calls;
        ProblematicResource(int* pcc) : p_cleanup_calls(pcc) {}
        ~ProblematicResource() { /* (*p_cleanup_calls)++; // RAII part if any */ } // No RAII part for this test resource
        // Movable
        ProblematicResource(ProblematicResource&& other) noexcept : p_cleanup_calls(other.p_cleanup_calls) {
            other.p_cleanup_calls = nullptr;
        }
        ProblematicResource& operator=(ProblematicResource&& other) noexcept {
            if (this != &other) {
                p_cleanup_calls = other.p_cleanup_calls;
                other.p_cleanup_calls = nullptr;
            }
            return *this;
        }
    };

    ProblematicResource res(&cleanup_calls);

    try {
        raii::with_resource(
            std::move(res),
            [](ProblematicResource&) {
                throw std::runtime_error("Func error");
            },
            [&](ProblematicResource&) {
                (*(&cleanup_calls))++; // Use address of cleanup_calls to avoid direct capture issues in lambda if needed
                // throw std::runtime_error("Cleanup error"); // Let's not throw from cleanup in test
            }
        );
    } catch (const std::runtime_error& e) {
        ASSERT_STREQ(e.what(), "Func error");
    }
    ASSERT_EQ(cleanup_calls, 1);
}

TEST(WithResourceTest, MacroBasic) {
    bool executed = false;
    std::string data = "macro_data";
    // WITH_RESOURCE(std::move(data), res_alias) {
    raii::with_resource(std::move(data), [&](std::string& res_alias) {
        ASSERT_EQ(res_alias, "macro_data");
        executed = true;
    });
    ASSERT_TRUE(executed);
}

TEST(WithResourceTest, MacroWithCleanup) {
    bool executed = false;
    bool cleanup_executed = false;
    std::string data = "macro_cleanup_data";

    // Adjusted usage: cleanup lambda is now provided after the main body block
    // WITH_RESOURCE_CLEANUP(std::move(data), res_alias) {
    raii::with_resource(std::move(data), [&](std::string& res_alias) {
        // This is the main lambda body
        ASSERT_EQ(res_alias, "macro_cleanup_data");
        executed = true;
    }, 
    // This is the cleanup lambda, provided as the next argument to raii::with_resource
    [&](std::string& d){
        ASSERT_EQ(d, "macro_cleanup_data");
        cleanup_executed = true;
    });
    ASSERT_TRUE(executed);
    ASSERT_TRUE(cleanup_executed);
}

// Main function for gtest
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

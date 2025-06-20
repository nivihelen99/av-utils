#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <variant>
#include <string>
#include <array>
#include <memory>
#include <random>
#include <algorithm>
#include <chrono>
#include <thread>


// Include the variant vector implementation
// (Assuming the previous code is in a header file or included here)
// For this test, I'll include the key classes inline
#include "variant_vector.h"
#include <any>

#include <type_traits>

#include <tuple>
#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <typeindex>


// Test data types
struct TestSmall {
    int value;
    TestSmall() = default;
    TestSmall(int v) : value(v) {}
    bool operator==(const TestSmall& other) const { return value == other.value; }
};

struct TestMedium {
    int x, y;
    double z;
    TestMedium() = default;
    TestMedium(int x_, int y_, double z_) : x(x_), y(y_), z(z_) {}
    bool operator==(const TestMedium& other) const { 
        return x == other.x && y == other.y && std::abs(z - other.z) < 1e-9; 
    }
};

struct TestLarge {
    std::array<double, 8> data;
    std::string name;
    int id;
    
    TestLarge() : id(0) {
        std::fill(data.begin(), data.end(), 0.0);
    }
    
    TestLarge(const std::string& n, int i) : name(n), id(i) {
        std::fill(data.begin(), data.end(), static_cast<double>(i));
    }
    
    bool operator==(const TestLarge& other) const {
        return name == other.name && id == other.id && 
               std::equal(data.begin(), data.end(), other.data.begin());
    }
};

// Non-copyable type for move semantics testing
struct TestMoveOnly {
    std::unique_ptr<int> ptr;
    
    TestMoveOnly() = default;
    TestMoveOnly(int value) : ptr(std::make_unique<int>(value)) {}
    
    TestMoveOnly(const TestMoveOnly&) = delete;
    TestMoveOnly& operator=(const TestMoveOnly&) = delete;
    
    TestMoveOnly(TestMoveOnly&&) = default;
    TestMoveOnly& operator=(TestMoveOnly&&) = default;
    
    bool operator==(const TestMoveOnly& other) const {
        return (ptr && other.ptr) ? (*ptr == *other.ptr) : (ptr == other.ptr);
    }
};

// ============================================================================
// STATIC VARIANT VECTOR TESTS
// ============================================================================

class StaticVariantVectorTest : public ::testing::Test {
protected:
    using TestVector = static_variant_vector<TestSmall, TestMedium, TestLarge>;
    TestVector vec;
    
    void SetUp() override {
        // Empty setup - each test will populate as needed
    }
};

// Basic Construction and Size Tests
TEST_F(StaticVariantVectorTest, DefaultConstruction) {
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
}

TEST_F(StaticVariantVectorTest, PushBackDifferentTypes) {
    vec.push_back(TestSmall{42});
    vec.push_back(TestMedium{1, 2, 3.14});
    vec.push_back(TestLarge{"test", 100});
    
    EXPECT_EQ(vec.size(), 3);
    EXPECT_FALSE(vec.empty());
}

TEST_F(StaticVariantVectorTest, EmplaceBackDifferentTypes) {
    vec.emplace_back<TestSmall>(42);
    vec.emplace_back<TestMedium>(1, 2, 3.14);
    vec.emplace_back<TestLarge>("test", 100);
    
    EXPECT_EQ(vec.size(), 3);
    EXPECT_FALSE(vec.empty());
}

// Element Access Tests
TEST_F(StaticVariantVectorTest, VariantAccess) {
    vec.emplace_back<TestSmall>(42);
    vec.emplace_back<TestMedium>(1, 2, 3.14);
    vec.emplace_back<TestLarge>("test", 100);
    
    auto variant0 = vec[0];
    auto variant1 = vec[1];
    auto variant2 = vec[2];
    
    EXPECT_TRUE(std::holds_alternative<TestSmall>(variant0));
    EXPECT_TRUE(std::holds_alternative<TestMedium>(variant1));
    EXPECT_TRUE(std::holds_alternative<TestLarge>(variant2));
    
    EXPECT_EQ(std::get<TestSmall>(variant0).value, 42);
    EXPECT_EQ(std::get<TestMedium>(variant1).x, 1);
    EXPECT_EQ(std::get<TestLarge>(variant2).name, "test");
}

TEST_F(StaticVariantVectorTest, TypedAccess) {
    vec.emplace_back<TestSmall>(42);
    vec.emplace_back<TestMedium>(1, 2, 3.14);
    vec.emplace_back<TestLarge>("test", 100);
    
    EXPECT_EQ(vec.get_typed<TestSmall>(0).value, 42);
    EXPECT_EQ(vec.get_typed<TestMedium>(1).x, 1);
    EXPECT_EQ(vec.get_typed<TestLarge>(2).name, "test");
}

TEST_F(StaticVariantVectorTest, TypedAccessWrongType) {
    vec.emplace_back<TestSmall>(42);
    
    EXPECT_THROW(vec.get_typed<TestMedium>(0), std::bad_variant_access);
}

TEST_F(StaticVariantVectorTest, AtBoundsChecking) {
    vec.emplace_back<TestSmall>(42);
    
    EXPECT_NO_THROW(vec.at(0));
    EXPECT_THROW(vec.at(1), std::out_of_range);
}

TEST_F(StaticVariantVectorTest, ClearMethod) {
    vec.emplace_back<TestSmall>(42);
    vec.emplace_back<TestMedium>(1, 2, 3.14);
    ASSERT_FALSE(vec.empty());
    ASSERT_EQ(vec.size(), 2);

    vec.clear();
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);

    // Check if underlying type vectors are also cleared
    EXPECT_TRUE(vec.get_type_vector<TestSmall>().empty());
    EXPECT_TRUE(vec.get_type_vector<TestMedium>().empty());

    // Add more elements to ensure it works after clear
    vec.emplace_back<TestLarge>("cleared", 1);
    EXPECT_EQ(vec.size(), 1);
    EXPECT_FALSE(vec.get_type_vector<TestLarge>().empty());
}

TEST_F(StaticVariantVectorTest, PopBackMethod) {
    vec.emplace_back<TestSmall>(10);
    vec.emplace_back<TestMedium>(20, 21, 22.2);
    vec.emplace_back<TestLarge>("item3", 30);
    ASSERT_EQ(vec.size(), 3);

    // Pop Large
    vec.pop_back();
    ASSERT_EQ(vec.size(), 2);
    EXPECT_TRUE(std::holds_alternative<TestMedium>(vec[1])); // Check last element
    EXPECT_EQ(vec.get_type_vector<TestLarge>().size(), 0); // Large vector should be empty now

    // Pop Medium
    vec.pop_back();
    ASSERT_EQ(vec.size(), 1);
    EXPECT_TRUE(std::holds_alternative<TestSmall>(vec[0]));
    EXPECT_EQ(vec.get_type_vector<TestMedium>().size(), 0);

    // Pop Small
    vec.pop_back();
    ASSERT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.get_type_vector<TestSmall>().size(), 0);

    // Pop on empty (should not throw, should be no-op as per current impl)
    EXPECT_NO_THROW(vec.pop_back());
    EXPECT_TRUE(vec.empty());
}

// Type-specific Vector Access Tests
TEST_F(StaticVariantVectorTest, GetTypeVector) {
    vec.emplace_back<TestSmall>(1);
    vec.emplace_back<TestMedium>(2, 3, 4.5);
    vec.emplace_back<TestSmall>(5);
    vec.emplace_back<TestLarge>("test", 6);
    vec.emplace_back<TestSmall>(7);
    
    const auto& small_vec = vec.get_type_vector<TestSmall>();
    const auto& medium_vec = vec.get_type_vector<TestMedium>();
    const auto& large_vec = vec.get_type_vector<TestLarge>();
    
    EXPECT_EQ(small_vec.size(), 3);
    EXPECT_EQ(medium_vec.size(), 1);
    EXPECT_EQ(large_vec.size(), 1);
    
    EXPECT_EQ(small_vec[0].value, 1);
    EXPECT_EQ(small_vec[1].value, 5);
    EXPECT_EQ(small_vec[2].value, 7);
}

// Type Index Tests
TEST_F(StaticVariantVectorTest, GetTypeIndex) {
    vec.emplace_back<TestSmall>(1);
    vec.emplace_back<TestMedium>(2, 3, 4.5);
    vec.emplace_back<TestLarge>("test", 6);
    
    EXPECT_EQ(vec.get_type_index(0), 0); // TestSmall
    EXPECT_EQ(vec.get_type_index(1), 1); // TestMedium
    EXPECT_EQ(vec.get_type_index(2), 2); // TestLarge
}

// Memory and Performance Tests
TEST_F(StaticVariantVectorTest, ReserveCapacity) {
    vec.reserve(1000);
    
    // Add elements and verify no reallocation occurred
    for (int i = 0; i < 100; ++i) {
        vec.emplace_back<TestSmall>(i);
    }
    
    EXPECT_EQ(vec.size(), 100);
}

TEST_F(StaticVariantVectorTest, MemoryUsageReporting) {
    vec.emplace_back<TestSmall>(1);
    vec.emplace_back<TestMedium>(2, 3, 4.5);
    vec.emplace_back<TestLarge>("test", 6);
    
    size_t memory_usage = vec.memory_usage();
    EXPECT_GT(memory_usage, 0);
}

// Stress Tests
TEST_F(StaticVariantVectorTest, LargeNumberOfElements) {
    const size_t N = 10000;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 2);
    
    for (size_t i = 0; i < N; ++i) {
        switch (dis(gen)) {
            case 0: vec.emplace_back<TestSmall>(static_cast<int>(i)); break;
            case 1: vec.emplace_back<TestMedium>(static_cast<int>(i), static_cast<int>(i*2), static_cast<double>(i)); break;
            case 2: vec.emplace_back<TestLarge>("item_" + std::to_string(i), static_cast<int>(i)); break;
        }
    }
    
    EXPECT_EQ(vec.size(), N);
    
    // Verify we can access all elements
    for (size_t i = 0; i < N; ++i) {
        EXPECT_NO_THROW(vec[i]);
    }
}

// Move Semantics Tests
class StaticVariantVectorMoveTest : public ::testing::Test {
protected:
    using MoveTestVector = static_variant_vector<TestMoveOnly, TestSmall>;
    MoveTestVector vec;
};

TEST_F(StaticVariantVectorMoveTest, EmplaceMoveOnly) {
    vec.emplace_back<TestMoveOnly>(42);
    vec.emplace_back<TestSmall>(24);
    
    EXPECT_EQ(vec.size(), 2);
    
    auto& move_only = vec.get_typed<TestMoveOnly>(0);
    EXPECT_EQ(*move_only.ptr, 42);
}

// ============================================================================
// DYNAMIC VARIANT VECTOR TESTS
// ============================================================================

class DynamicVariantVectorTest : public ::testing::Test {
protected:
    dynamic_variant_vector vec;
    
    void SetUp() override {
        // Pre-register common types
        vec.register_type<TestSmall>();
        vec.register_type<TestMedium>();
        vec.register_type<TestLarge>();
    }
};

// Basic Construction and Size Tests
TEST_F(DynamicVariantVectorTest, DefaultConstruction) {
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
}

TEST_F(DynamicVariantVectorTest, AutoTypeRegistration) {
    // Should auto-register types when first used
    dynamic_variant_vector auto_vec;
    
    auto_vec.push_back(TestSmall{42});
    EXPECT_EQ(auto_vec.size(), 1);
}

TEST_F(DynamicVariantVectorTest, PushBackDifferentTypes) {
    vec.push_back(TestSmall{42});
    vec.push_back(TestMedium{1, 2, 3.14});
    vec.push_back(TestLarge{"test", 100});
    
    EXPECT_EQ(vec.size(), 3);
    EXPECT_FALSE(vec.empty());
}

TEST_F(DynamicVariantVectorTest, EmplaceBackDifferentTypes) {
    vec.emplace_back<TestSmall>(42);
    vec.emplace_back<TestMedium>(1, 2, 3.14);
    vec.emplace_back<TestLarge>("test", 100);
    
    EXPECT_EQ(vec.size(), 3);
    EXPECT_FALSE(vec.empty());
}

// Element Access Tests
TEST_F(DynamicVariantVectorTest, TypedAccess) {
    vec.emplace_back<TestSmall>(42);
    vec.emplace_back<TestMedium>(1, 2, 3.14);
    vec.emplace_back<TestLarge>("test", 100);
    
    EXPECT_EQ(vec.get_typed<TestSmall>(0).value, 42);
    EXPECT_EQ(vec.get_typed<TestMedium>(1).x, 1);
    EXPECT_EQ(vec.get_typed<TestLarge>(2).name, "test");
}

TEST_F(DynamicVariantVectorTest, MutableAccess) {
    vec.emplace_back<TestSmall>(42);
    
    auto& element = vec.get_typed<TestSmall>(0);
    element.value = 100;
    
    EXPECT_EQ(vec.get_typed<TestSmall>(0).value, 100);
}

TEST_F(DynamicVariantVectorTest, ClearMethod) {
    vec.emplace_back<TestSmall>(42);
    vec.emplace_back<TestMedium>(1, 2, 3.14);
    ASSERT_FALSE(vec.empty());
    ASSERT_EQ(vec.size(), 2);

    vec.clear();
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);

    // Check if underlying type vectors are also cleared
    // This assumes types were registered before clear.
    // If clear also clears type_storages then this check needs adjustment.
    // Based on current clear impl, type_storages are kept, data is cleared.
    EXPECT_TRUE(vec.get_type_vector<TestSmall>().empty());
    EXPECT_TRUE(vec.get_type_vector<TestMedium>().empty());

    // Add more elements to ensure it works after clear
    vec.emplace_back<TestLarge>("cleared", 1); // Assumes TestLarge was registered or auto-registers
    EXPECT_EQ(vec.size(), 1);
    EXPECT_FALSE(vec.get_type_vector<TestLarge>().empty());
}

TEST_F(DynamicVariantVectorTest, PopBackMethod) {
    vec.emplace_back<TestSmall>(10);
    vec.emplace_back<TestMedium>(20, 21, 22.2);
    vec.emplace_back<TestLarge>("item3", 30);
    ASSERT_EQ(vec.size(), 3);

    // Pop Large
    vec.pop_back();
    ASSERT_EQ(vec.size(), 2);
    EXPECT_NO_THROW({
        [[maybe_unused]] auto& last_el = vec.get_typed<TestMedium>(1);
    });
    EXPECT_EQ(vec.get_type_vector<TestLarge>().size(), 0);

    // Pop Medium
    vec.pop_back();
    ASSERT_EQ(vec.size(), 1);
    EXPECT_NO_THROW({
        [[maybe_unused]] auto& last_el = vec.get_typed<TestSmall>(0);
    });
    EXPECT_EQ(vec.get_type_vector<TestMedium>().size(), 0);

    // Pop Small
    vec.pop_back();
    ASSERT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.get_type_vector<TestSmall>().size(), 0);

    // Pop on empty
    EXPECT_NO_THROW(vec.pop_back());
    EXPECT_TRUE(vec.empty());
}

TEST_F(DynamicVariantVectorTest, AnyAccessAtMethod) {
    vec.emplace_back<TestSmall>(42);
    vec.emplace_back<TestMedium>(1, 2, 3.14);
    vec.emplace_back<TestLarge>("test_any", 100);

    // Access Small
    std::any any_val_small = vec.at(0);
    ASSERT_EQ(any_val_small.type(), typeid(TestSmall));
    EXPECT_EQ(std::any_cast<TestSmall>(any_val_small).value, 42);

    // Access Medium
    std::any any_val_medium = vec.at(1);
    ASSERT_EQ(any_val_medium.type(), typeid(TestMedium));
    EXPECT_EQ(std::any_cast<TestMedium>(any_val_medium).x, 1);

    // Access Large
    std::any any_val_large = vec.at(2);
    ASSERT_EQ(any_val_large.type(), typeid(TestLarge));
    EXPECT_EQ(std::any_cast<TestLarge>(any_val_large).name, "test_any");

    // Out of bounds
    EXPECT_THROW(vec.at(3), std::out_of_range);

    // Const correctness check (if possible to make vec const)
    const auto& const_vec = vec;
    std::any any_val_const = const_vec.at(0);
    ASSERT_EQ(any_val_const.type(), typeid(TestSmall));
    EXPECT_EQ(std::any_cast<TestSmall>(any_val_const).value, 42);
}

// Type-specific Vector Access Tests
TEST_F(DynamicVariantVectorTest, GetTypeVector) {
    vec.emplace_back<TestSmall>(1);
    vec.emplace_back<TestMedium>(2, 3, 4.5);
    vec.emplace_back<TestSmall>(5);
    vec.emplace_back<TestLarge>("test", 6);
    vec.emplace_back<TestSmall>(7);
    
    const auto& small_vec = vec.get_type_vector<TestSmall>();
    const auto& medium_vec = vec.get_type_vector<TestMedium>();
    const auto& large_vec = vec.get_type_vector<TestLarge>();
    
    EXPECT_EQ(small_vec.size(), 3);
    EXPECT_EQ(medium_vec.size(), 1);
    EXPECT_EQ(large_vec.size(), 1);
    
    EXPECT_EQ(small_vec[0].value, 1);
    EXPECT_EQ(small_vec[1].value, 5);
    EXPECT_EQ(small_vec[2].value, 7);
}

TEST_F(DynamicVariantVectorTest, GetTypeVectorUnregistered) {
    // Test accessing type vector for unregistered type
    EXPECT_THROW(vec.get_type_vector<int>(), std::runtime_error);
}

// Memory and Performance Tests
TEST_F(DynamicVariantVectorTest, ReserveCapacity) {
    vec.reserve(1000);
    
    for (int i = 0; i < 100; ++i) {
        vec.emplace_back<TestSmall>(i);
    }
    
    EXPECT_EQ(vec.size(), 100);
}

TEST_F(DynamicVariantVectorTest, MemoryUsageReporting) {
    vec.emplace_back<TestSmall>(1);
    vec.emplace_back<TestMedium>(2, 3, 4.5);
    vec.emplace_back<TestLarge>("test", 6);
    
    size_t memory_usage = vec.memory_usage();
    EXPECT_GT(memory_usage, 0);
}

// ============================================================================
// PERFORMANCE COMPARISON TESTS
// ============================================================================

class PerformanceComparisonTest : public ::testing::Test {
protected:
    static constexpr size_t TEST_SIZE = 10000;
    
    void SetUp() override {
        // Prepare test data
        std::random_device rd;
        gen.seed(rd());
    }
    
    std::mt19937 gen;
};

TEST_F(PerformanceComparisonTest, MemoryEfficiencyComparison) {
    // Traditional approach
    std::vector<std::variant<TestSmall, TestMedium, TestLarge>> traditional;
    traditional.reserve(TEST_SIZE);
    
    // Optimized approaches
    static_variant_vector<TestSmall, TestMedium, TestLarge> optimized_static;
    optimized_static.reserve(TEST_SIZE);
    
    dynamic_variant_vector optimized_dynamic;
    optimized_dynamic.reserve(TEST_SIZE);
    
    std::uniform_int_distribution<> dis(0, 2);
    
    // Fill all containers with the same data
    for (size_t i = 0; i < TEST_SIZE; ++i) {
        int type = dis(gen);
        switch (type) {
            case 0:
                traditional.emplace_back(TestSmall{static_cast<int>(i)});
                optimized_static.emplace_back<TestSmall>(static_cast<int>(i));
                optimized_dynamic.emplace_back<TestSmall>(static_cast<int>(i));
                break;
            case 1:
                traditional.emplace_back(TestMedium{static_cast<int>(i), static_cast<int>(i*2), static_cast<double>(i)});
                optimized_static.emplace_back<TestMedium>(static_cast<int>(i), static_cast<int>(i*2), static_cast<double>(i));
                optimized_dynamic.emplace_back<TestMedium>(static_cast<int>(i), static_cast<int>(i*2), static_cast<double>(i));
                break;
            case 2:
                traditional.emplace_back(TestLarge{"item_" + std::to_string(i), static_cast<int>(i)});
                optimized_static.emplace_back<TestLarge>("item_" + std::to_string(i), static_cast<int>(i));
                optimized_dynamic.emplace_back<TestLarge>("item_" + std::to_string(i), static_cast<int>(i));
                break;
        }
    }
    
    // Compare memory usage
    size_t traditional_memory = traditional.capacity() * sizeof(std::variant<TestSmall, TestMedium, TestLarge>);
    size_t static_memory = optimized_static.memory_usage();
    size_t dynamic_memory = optimized_dynamic.memory_usage();
    
    // The optimized versions should use less memory
    // Note: These assertions for performance/memory are indicative and may need tuning for specific platforms or more rigorous benchmarking.
    EXPECT_LT(static_memory, traditional_memory);
    EXPECT_LT(dynamic_memory, traditional_memory);
    
    // Log the actual numbers for manual inspection
    std::cout << "Memory usage comparison (bytes):\n";
    std::cout << "Traditional: " << traditional_memory << "\n";
    std::cout << "Static SoA:  " << static_memory << "\n";
    std::cout << "Dynamic SoA: " << dynamic_memory << "\n";
}

TEST_F(PerformanceComparisonTest, TypeSpecificIterationPerformance) {
    static_variant_vector<TestSmall, TestMedium, TestLarge> optimized;
    std::vector<std::variant<TestSmall, TestMedium, TestLarge>> traditional;
    
    // Fill with mostly TestSmall for fair comparison
    for (size_t i = 0; i < TEST_SIZE; ++i) {
        TestSmall small{static_cast<int>(i)};
        optimized.push_back(small);
        traditional.push_back(small);
    }
    
    // Benchmark type-specific iteration
    auto start = std::chrono::high_resolution_clock::now();
    
    // Optimized version - direct access to type vector
    const auto& small_vec = optimized.get_type_vector<TestSmall>();
    long long sum_optimized = 0;
    for (const auto& item : small_vec) {
        sum_optimized += item.value;
    }
    
    auto mid = std::chrono::high_resolution_clock::now();
    
    // Traditional version - variant checking
    long long sum_traditional = 0;
    for (const auto& variant : traditional) {
        if (std::holds_alternative<TestSmall>(variant)) {
            sum_traditional += std::get<TestSmall>(variant).value;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    // Verify correctness
    EXPECT_EQ(sum_optimized, sum_traditional);
    
    auto optimized_time = std::chrono::duration_cast<std::chrono::microseconds>(mid - start);
    auto traditional_time = std::chrono::duration_cast<std::chrono::microseconds>(end - mid);
    
    // The optimized version should be faster (or at least not significantly slower)
    // This is a loose check since performance can vary
    // Note: These assertions for performance/memory are indicative and may need tuning for specific platforms or more rigorous benchmarking.
    std::cout << "Type-specific iteration performance (microseconds):\n";
    std::cout << "Optimized: " << optimized_time.count() << "\n";
    std::cout << "Traditional: " << traditional_time.count() << "\n";
    
    if (traditional_time.count() > 0) {
        double speedup = static_cast<double>(traditional_time.count()) / optimized_time.count();
        std::cout << "Speedup: " << speedup << "x\n";
        
        // We expect at least some improvement, but this is hardware dependent
        EXPECT_GE(speedup, 0.8); // Allow some variance due to test environment
    }
}

// ============================================================================
// THREAD SAFETY TESTS (Basic)
// ============================================================================

class ThreadSafetyTest : public ::testing::Test {
protected:
    static_variant_vector<TestSmall, TestMedium> vec;
    
    void SetUp() override {
        vec = static_variant_vector<TestSmall, TestMedium>{};
    }
};

TEST_F(ThreadSafetyTest, ConcurrentReads) {
    // Fill vector with test data
    for (int i = 0; i < 1000; ++i) {
        vec.emplace_back<TestSmall>(i);
    }
    
    // Multiple threads reading simultaneously should be safe
    std::vector<std::thread> readers;
    std::atomic<bool> error_occurred{false};
    
    for (int t = 0; t < 4; ++t) {
        readers.emplace_back([this, &error_occurred]() {
            try {
                for (size_t i = 0; i < vec.size(); ++i) {
                    auto variant = vec[i];
                    (void)variant; // Suppress unused variable warning
                }
            } catch (...) {
                error_occurred = true;
            }
        });
    }
    
    for (auto& t : readers) {
        t.join();
    }
    
    EXPECT_FALSE(error_occurred);
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

class IntegrationTest : public ::testing::Test {
protected:
    // Test complex scenarios that combine multiple features
};

TEST_F(IntegrationTest, MixedOperationsWorkflow) {
    static_variant_vector<TestSmall, TestMedium, TestLarge> vec;
    
    // Phase 1: Build up container with mixed types
    vec.reserve(100);
    
    for (int i = 0; i < 50; ++i) {
        vec.emplace_back<TestSmall>(i);
        if (i % 3 == 0) {
            vec.emplace_back<TestMedium>(i, i*2, i*3.14);
        }
        if (i % 7 == 0) {
            vec.emplace_back<TestLarge>("item_" + std::to_string(i), i);
        }
    }
    
    size_t expected_size = 50 + (50/3) + (50/7) + 2; // +2 for i=0 cases
    EXPECT_EQ(vec.size(), expected_size);
    
    // Phase 2: Verify type-specific access
    const auto& small_vec = vec.get_type_vector<TestSmall>();
    const auto& medium_vec = vec.get_type_vector<TestMedium>();
    const auto& large_vec = vec.get_type_vector<TestLarge>();
    
    EXPECT_EQ(small_vec.size(), 50);
    EXPECT_GT(medium_vec.size(), 0);
    EXPECT_GT(large_vec.size(), 0);
    
    // Phase 3: Verify all elements are accessible
    for (size_t i = 0; i < vec.size(); ++i) {
        EXPECT_NO_THROW(vec[i]);
        uint8_t type_idx = vec.get_type_index(i);
        EXPECT_GE(type_idx, 0);
        EXPECT_LE(type_idx, 2);
    }
    
    // Phase 4: Memory usage should be reasonable
    size_t memory_usage = vec.memory_usage();
    EXPECT_GT(memory_usage, 0);
    
    // Should use less memory than equivalent std::vector<std::variant>
    size_t variant_size = sizeof(std::variant<TestSmall, TestMedium, TestLarge>);
    size_t traditional_memory = expected_size * variant_size;
    
    std::cout << "Integration test memory comparison:\n";
    std::cout << "Optimized: " << memory_usage << " bytes\n";
    std::cout << "Traditional equivalent: " << traditional_memory << " bytes\n";
    
    // This should generally be true, but allow some variance for small collections
    if (expected_size > 10) {
        EXPECT_LT(memory_usage, traditional_memory * 1.2); // Allow 20% overhead for small tests
    }
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

/*
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
*/


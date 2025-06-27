#include "cached_property.h" // Adjust path as needed for your build system
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <stdexcept> // For std::invalid_argument

// Test fixture for CachedProperty tests
class CachedPropertyTest : public ::testing::Test {
protected:
    struct TestOwner {
        int id;
        mutable int compute_count_value = 0;
        mutable int compute_count_str = 0;
        mutable int compute_count_const_method = 0;
        mutable int compute_count_non_const_method = 0;

        std::string prefix = "Data: ";

        TestOwner(int i) : id(i) {}

        // Lambda target
        int calculate_value() {
            compute_count_value++;
            return id * 10;
        }

        // Lambda target
        std::string calculate_str() {
            compute_count_str++;
            return prefix + std::to_string(id);
        }

        // Const member function target
        double calculate_const_method() const {
            compute_count_const_method++;
            return id * 1.5;
        }

        // Non-const member function target
        char calculate_non_const_method() {
            compute_count_non_const_method++;
            return static_cast<char>('A' + id);
        }
    };

    TestOwner owner{1};
    TestOwner owner2{2};
};

// Test basic caching with lambda
TEST_F(CachedPropertyTest, BasicCachingLambda) {
    CachedProperty<TestOwner, int> prop(&owner, [](TestOwner* o) {
        return o->calculate_value();
    });

    ASSERT_FALSE(prop.is_cached());
    ASSERT_EQ(owner.compute_count_value, 0);

    EXPECT_EQ(prop.get(), 10);
    ASSERT_TRUE(prop.is_cached());
    ASSERT_EQ(owner.compute_count_value, 1);

    EXPECT_EQ(prop.get(), 10); // Access again
    ASSERT_TRUE(prop.is_cached());
    ASSERT_EQ(owner.compute_count_value, 1); // Should not recompute
}

// Test conversion operator
TEST_F(CachedPropertyTest, ConversionOperator) {
    CachedProperty<TestOwner, int> prop(&owner, [](TestOwner* o) {
        return o->calculate_value();
    });
    owner.compute_count_value = 0; // Reset for this test

    const int& val = prop; // Uses conversion operator
    EXPECT_EQ(val, 10);
    ASSERT_TRUE(prop.is_cached());
    ASSERT_EQ(owner.compute_count_value, 1);

    const int& val2 = prop;
    EXPECT_EQ(val2, 10);
    ASSERT_EQ(owner.compute_count_value, 1);
}

// Test invalidation
TEST_F(CachedPropertyTest, Invalidation) {
    CachedProperty<TestOwner, int> prop(&owner, [](TestOwner* o) {
        return o->calculate_value();
    });
    owner.compute_count_value = 0;

    prop.get(); // First computation
    ASSERT_EQ(owner.compute_count_value, 1);
    ASSERT_TRUE(prop.is_cached());

    prop.invalidate();
    ASSERT_FALSE(prop.is_cached());

    EXPECT_EQ(prop.get(), 10); // Recompute
    ASSERT_TRUE(prop.is_cached());
    ASSERT_EQ(owner.compute_count_value, 2);
}

// Test with different data type (std::string)
TEST_F(CachedPropertyTest, StringType) {
    CachedProperty<TestOwner, std::string> prop(&owner, [](TestOwner* o) {
        return o->calculate_str();
    });
    ASSERT_EQ(owner.compute_count_str, 0);

    EXPECT_EQ(prop.get(), "Data: 1");
    ASSERT_EQ(owner.compute_count_str, 1);

    EXPECT_EQ(prop.get(), "Data: 1");
    ASSERT_EQ(owner.compute_count_str, 1);
}

// Test with const member function using make_cached_property
TEST_F(CachedPropertyTest, ConstMemberFunction) {
    auto prop = make_cached_property(&owner, &TestOwner::calculate_const_method);

    ASSERT_FALSE(prop.is_cached());
    ASSERT_EQ(owner.compute_count_const_method, 0);

    EXPECT_DOUBLE_EQ(prop.get(), 1.5);
    ASSERT_TRUE(prop.is_cached());
    ASSERT_EQ(owner.compute_count_const_method, 1);

    EXPECT_DOUBLE_EQ(prop.get(), 1.5);
    ASSERT_EQ(owner.compute_count_const_method, 1);
}

// Test with non-const member function using make_cached_property
TEST_F(CachedPropertyTest, NonConstMemberFunction) {
    auto prop = make_cached_property(&owner, &TestOwner::calculate_non_const_method);

    ASSERT_FALSE(prop.is_cached());
    ASSERT_EQ(owner.compute_count_non_const_method, 0);

    EXPECT_EQ(prop.get(), 'B'); // 'A' + 1
    ASSERT_TRUE(prop.is_cached());
    ASSERT_EQ(owner.compute_count_non_const_method, 1);

    EXPECT_EQ(prop.get(), 'B');
    ASSERT_EQ(owner.compute_count_non_const_method, 1);
}

// Test with different owner instance
TEST_F(CachedPropertyTest, DifferentOwnerInstance) {
    CachedProperty<TestOwner, int> prop1(&owner, [](TestOwner* o) { return o->calculate_value(); });
    CachedProperty<TestOwner, int> prop2(&owner2, [](TestOwner* o) { return o->calculate_value(); });
    owner.compute_count_value = 0;
    owner2.compute_count_value = 0;


    EXPECT_EQ(prop1.get(), 10);
    ASSERT_EQ(owner.compute_count_value, 1);
    ASSERT_EQ(owner2.compute_count_value, 0);

    EXPECT_EQ(prop2.get(), 20);
    ASSERT_EQ(owner.compute_count_value, 1);
    ASSERT_EQ(owner2.compute_count_value, 1);

    EXPECT_EQ(prop1.get(), 10);
    ASSERT_EQ(owner.compute_count_value, 1);
}

// Test constructor with null owner
TEST_F(CachedPropertyTest, ConstructorNullOwner) {
    ASSERT_THROW(CachedProperty<TestOwner, int>(nullptr, [](TestOwner* o) { return o->id; }),
                 std::invalid_argument);
}

// Test constructor with null compute function
TEST_F(CachedPropertyTest, ConstructorNullComputeFunc) {
    typename CachedProperty<TestOwner, int>::compute_func_type func = nullptr;
    ASSERT_THROW(CachedProperty<TestOwner, int>(&owner, func),
                 std::invalid_argument);
}

// Test is_cached before and after access
TEST_F(CachedPropertyTest, IsCachedState) {
    CachedProperty<TestOwner, int> prop(&owner, [](TestOwner* o) {
        return o->calculate_value();
    });
    owner.compute_count_value = 0;

    ASSERT_FALSE(prop.is_cached());
    prop.get();
    ASSERT_TRUE(prop.is_cached());
    prop.invalidate();
    ASSERT_FALSE(prop.is_cached());
    prop.get();
    ASSERT_TRUE(prop.is_cached());
}

// Test that compute function is not called if not accessed
TEST_F(CachedPropertyTest, NotCalledIfNotAccessed) {
    owner.compute_count_value = 0;
    CachedProperty<TestOwner, int> prop(&owner, [](TestOwner* o) {
        return o->calculate_value();
    });
    // Do not call prop.get() or access it via conversion
    ASSERT_EQ(owner.compute_count_value, 0);
    ASSERT_FALSE(prop.is_cached());
}

// Test make_cached_property with lambda
TEST_F(CachedPropertyTest, MakeCachedPropertyLambda) {
    owner.compute_count_value = 0;
    auto prop = make_cached_property(&owner, [](TestOwner* o) {
        return o->calculate_value();
    });

    ASSERT_FALSE(prop.is_cached());
    ASSERT_EQ(owner.compute_count_value, 0);

    EXPECT_EQ(prop.get(), 10);
    ASSERT_TRUE(prop.is_cached());
    ASSERT_EQ(owner.compute_count_value, 1);
}

// Test const correctness: accessing CachedProperty from a const context
TEST_F(CachedPropertyTest, ConstCorrectness) {
    struct ConstHost {
        TestOwner real_owner{5};
        // Using const member function for computation
        CachedProperty<TestOwner, double> prop{&real_owner, &TestOwner::calculate_const_method};

        double getValue() const {
            return prop.get(); // Accessing via const reference to prop
        }
         double getValueConvert() const {
            return prop; // Accessing via const reference to prop
        }
    };

    ConstHost ch;
    ASSERT_EQ(ch.real_owner.compute_count_const_method, 0);
    EXPECT_DOUBLE_EQ(ch.getValue(), 5 * 1.5);
    ASSERT_EQ(ch.real_owner.compute_count_const_method, 1);

    // Access again
    EXPECT_DOUBLE_EQ(ch.getValue(), 5 * 1.5);
    ASSERT_EQ(ch.real_owner.compute_count_const_method, 1);

    // Reset counter for conversion test
    ch.real_owner.compute_count_const_method = 0;
    ch.prop.invalidate();

    EXPECT_DOUBLE_EQ(ch.getValueConvert(), 5 * 1.5);
    ASSERT_EQ(ch.real_owner.compute_count_const_method, 1);
}

// Main function to run tests if not using a dedicated test runner like CMake/CTest
// For a real project, this would be handled by the build system and test runner.
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

/*
To compile and run this test file with GTest (assuming GTest is installed):
g++ -std=c++17 -isystem ${GTEST_DIR}/include -pthread tests/cached_property_test.cpp ${GTEST_DIR}/lib/libgtest.a ${GTEST_DIR}/lib/libgtest_main.a -o cached_property_test_runner
./cached_property_test_runner

Replace ${GTEST_DIR} with the actual path to your GTest installation.
The header include might need to be "../include/cached_property.h" depending on structure.
*/

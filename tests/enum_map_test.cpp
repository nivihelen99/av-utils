#include "gtest/gtest.h"
#include "enum_map.h" // Assuming it's accessible via include paths
#include <string>
#include <vector>
#include <algorithm> // For std::equal

// Define a sample enum for testing
enum class TestEnum {
    A,
    B,
    C,
    COUNT
};

// Another enum for different size/types
enum class Color {
    RED,
    GREEN,
    BLUE,
    ALPHA,
    COUNT
};

// A simple struct for testing with non-primitive types
struct MyStruct {
    int id;
    std::string name;

    MyStruct(int i = 0, std::string s = "") : id(i), name(std::move(s)) {}

    // Needed for some tests (e.g., default construction by EnumMap, erase, clear)
    bool operator==(const MyStruct& other) const {
        return id == other.id && name == other.name;
    }
    // For printing with GTest
    friend std::ostream& operator<<(std::ostream& os, const MyStruct& s) {
        return os << "MyStruct{id=" << s.id << ", name=\"" << s.name << "\"}";
    }
};


// Test fixture for EnumMap tests
class EnumMapTest : public ::testing::Test {
protected:
    // Per-test set-up logic, if needed
    void SetUp() override {
        // Code here will be called immediately after the constructor (right
        // before each test).
    }

    // Per-test tear-down logic, if needed
    void TearDown() override {
        // Code here will be called immediately after each test (right
        // before the destructor).
    }
};

TEST_F(EnumMapTest, DefaultConstructor) {
    EnumMap<TestEnum, int> map_int;
    ASSERT_EQ(map_int.size(), static_cast<size_t>(TestEnum::COUNT));
    ASSERT_FALSE(map_int.empty());
    // Default constructor should value-initialize/default-construct elements
    for (size_t i = 0; i < map_int.size(); ++i) {
        EXPECT_EQ(map_int[static_cast<TestEnum>(i)], 0) << "Element at index " << i << " not default initialized to 0.";
    }

    EnumMap<TestEnum, std::string> map_str;
    ASSERT_EQ(map_str.size(), static_cast<size_t>(TestEnum::COUNT));
    for (size_t i = 0; i < map_str.size(); ++i) {
        EXPECT_EQ(map_str[static_cast<TestEnum>(i)], "") << "Element at index " << i << " not default initialized to empty string.";
    }

    EnumMap<TestEnum, MyStruct> map_struct;
    ASSERT_EQ(map_struct.size(), static_cast<size_t>(TestEnum::COUNT));
    for (size_t i = 0; i < map_struct.size(); ++i) {
        EXPECT_EQ(map_struct[static_cast<TestEnum>(i)], MyStruct(0, "")) << "Element at index " << i << " not default initialized.";
    }
}

TEST_F(EnumMapTest, InitializerListConstructor) {
    EnumMap<TestEnum, int> map = {
        {TestEnum::A, 10},
        {TestEnum::C, 30}
    };
    EXPECT_EQ(map.size(), static_cast<size_t>(TestEnum::COUNT));
    EXPECT_EQ(map[TestEnum::A], 10);
    EXPECT_EQ(map[TestEnum::B], 0); // Should be default-initialized
    EXPECT_EQ(map[TestEnum::C], 30);

    EnumMap<Color, std::string> color_map = {
        {Color::RED, "Red"},
        {Color::BLUE, "Blue"}
    };
    EXPECT_EQ(color_map.size(), static_cast<size_t>(Color::COUNT));
    EXPECT_EQ(color_map[Color::RED], "Red");
    EXPECT_EQ(color_map[Color::GREEN], ""); // Default
    EXPECT_EQ(color_map[Color::BLUE], "Blue");
    EXPECT_EQ(color_map[Color::ALPHA], ""); // Default
}

TEST_F(EnumMapTest, CtadConstructor) {
    EnumMap map = { // CTAD
        std::pair{TestEnum::A, 100},
        std::pair{TestEnum::B, 200}
    };
    EXPECT_EQ(map[TestEnum::A], 100);
    EXPECT_EQ(map[TestEnum::B], 200);
    EXPECT_EQ(map[TestEnum::C], 0); // Default
    ASSERT_EQ(map.size(), static_cast<size_t>(TestEnum::COUNT));
    // Check type deduction
    static_assert(std::is_same_v<decltype(map)::key_type, TestEnum>);
    static_assert(std::is_same_v<decltype(map)::mapped_type, int>);
}


TEST_F(EnumMapTest, OperatorSquareBrackets) {
    EnumMap<TestEnum, int> map;
    map[TestEnum::A] = 1;
    map[TestEnum::B] = 2;
    EXPECT_EQ(map[TestEnum::A], 1);
    EXPECT_EQ(map[TestEnum::B], 2);
    EXPECT_EQ(map[TestEnum::C], 0); // Default

    map[TestEnum::A] = 11; // Modify
    EXPECT_EQ(map[TestEnum::A], 11);

    const EnumMap<TestEnum, int> const_map = {{TestEnum::A, 101}};
    EXPECT_EQ(const_map[TestEnum::A], 101);
    EXPECT_EQ(const_map[TestEnum::B], 0);
}

TEST_F(EnumMapTest, AtMethod) {
    EnumMap<TestEnum, std::string> map;
    map.at(TestEnum::A) = "Apple";
    map.at(TestEnum::B) = "Banana";
    EXPECT_EQ(map.at(TestEnum::A), "Apple");
    EXPECT_EQ(map.at(TestEnum::B), "Banana");
    EXPECT_EQ(map.at(TestEnum::C), ""); // Default

    map.at(TestEnum::A) = "Apricot"; // Modify
    EXPECT_EQ(map.at(TestEnum::A), "Apricot");

    const EnumMap<TestEnum, std::string> const_map = {{TestEnum::B, "Blueberry"}};
    EXPECT_EQ(const_map.at(TestEnum::B), "Blueberry");
    EXPECT_EQ(const_map.at(TestEnum::A), "");

    // Bounds checking
    // TestEnum only goes up to C (index 2). COUNT is 3.
    // Accessing an enum value equal to or greater than COUNT is out of bounds.
    // The current implementation of to_index and is_valid_enum checks < N.
    // So, static_cast<TestEnum>(TestEnum::COUNT) would be an invalid key for at().
    // Note: This depends on how the enum is defined and used. If an enum value
    // `TestEnum::INVALID = 100` existed, at(TestEnum::INVALID) should throw.
    // For now, we test with an enum value that is definitely out of range if N is small.
    EnumMap<TestEnum, int, static_cast<size_t>(TestEnum::C) + 1> small_map; // Size is 3 (A, B, C)
    EXPECT_THROW(small_map.at(static_cast<TestEnum>(10)), std::out_of_range);
    EXPECT_THROW(map.at(static_cast<TestEnum>(static_cast<int>(TestEnum::COUNT))), std::out_of_range); // Accessing COUNT itself
    EXPECT_THROW(map.at(static_cast<TestEnum>(99)), std::out_of_range); // Arbitrary large value

    // Valid access should not throw
    EXPECT_NO_THROW(map.at(TestEnum::A));
    EXPECT_NO_THROW(map.at(TestEnum::C));
}

TEST_F(EnumMapTest, Iterators) {
    EnumMap<TestEnum, int> map = {
        {TestEnum::A, 1}, {TestEnum::B, 2}, {TestEnum::C, 3}
    };

    // Non-const iterator
    int expected_val = 1;
    TestEnum expected_key_arr[] = {TestEnum::A, TestEnum::B, TestEnum::C};
    size_t idx = 0;
    for (auto it = map.begin(); it != map.end(); ++it, ++idx) {
        EXPECT_EQ((*it).first, expected_key_arr[idx]); // Use (*it).first
        EXPECT_EQ((*it).second, expected_val + idx);   // Use (*it).second
        (*it).second += 10; // Modify through iterator's proxy
    }
    EXPECT_EQ(map[TestEnum::A], 11);
    EXPECT_EQ(map[TestEnum::B], 12);
    EXPECT_EQ(map[TestEnum::C], 13);

    // Range-based for loop (non-const) using auto&&
    idx = 0;
    for (auto&& pair : map) { // Use auto&& for forwarding reference
        EXPECT_EQ(pair.first, expected_key_arr[idx]);
        EXPECT_EQ(pair.second, 11 + idx);
        pair.second -= 5; // Modification should work via proxy's reference member
        idx++;
    }
    EXPECT_EQ(map[TestEnum::A], 6);
    EXPECT_EQ(map[TestEnum::B], 7);
    EXPECT_EQ(map[TestEnum::C], 8);

    // Const iterator
    const EnumMap<TestEnum, int>& const_map = map;
    expected_val = 6;
    idx = 0;
    for (auto it = const_map.cbegin(); it != const_map.cend(); ++it, ++idx) {
        EXPECT_EQ((*it).first, expected_key_arr[idx]);  // Use (*it).first
        EXPECT_EQ((*it).second, expected_val + idx); // Use (*it).second
        // (*it).second += 10; // Should fail to compile: error: assignment of read-only member
    }

    // Range-based for loop (const)
    idx = 0;
    for (const auto& pair : const_map) { // const auto& is fine for const iteration
        EXPECT_EQ(pair.first, expected_key_arr[idx]);
        EXPECT_EQ(pair.second, expected_val + idx);
        idx++;
    }

    // Iterator random access
    auto it_begin = map.begin();
    auto it_c = it_begin + 2; // Points to C
    EXPECT_EQ((*it_c).first, TestEnum::C);    // Use (*it_c).first
    EXPECT_EQ((*it_c).second, 8);   // Use (*it_c).second
    it_c--; // Points to B
    EXPECT_EQ((*it_c).first, TestEnum::B);    // Use (*it_c).first
    EXPECT_EQ((*it_c).second, 7);   // Use (*it_c).second

    EXPECT_EQ((map.end() - map.begin()), static_cast<ptrdiff_t>(TestEnum::COUNT));
}

TEST_F(EnumMapTest, ValueIterators) {
    EnumMap<TestEnum, int> map = {
        {TestEnum::A, 10}, {TestEnum::B, 20}, {TestEnum::C, 30}
    };

    // Non-const value iterator
    std::vector<int> values;
    for (auto it = map.value_begin(); it != map.value_end(); ++it) {
        values.push_back(*it);
        (*it) *= 2; // Modify
    }
    ASSERT_EQ(values.size(), 3);
    EXPECT_EQ(values[0], 10); EXPECT_EQ(map[TestEnum::A], 20);
    EXPECT_EQ(values[1], 20); EXPECT_EQ(map[TestEnum::B], 40);
    EXPECT_EQ(values[2], 30); EXPECT_EQ(map[TestEnum::C], 60);

    // Const value iterator (using const_value_cbegin for explicitness)
    const EnumMap<TestEnum, int>& const_map = map;
    values.clear();
    for (auto it = const_map.const_value_cbegin(); it != const_map.const_value_cend(); ++it) {
        values.push_back(*it);
        // (*it) *= 2; // Should not compile
    }
    ASSERT_EQ(values.size(), 3);
    EXPECT_EQ(values[0], 20);
    EXPECT_EQ(values[1], 40);
    EXPECT_EQ(values[2], 60);
}


TEST_F(EnumMapTest, SizeEmptyMaxSize) {
    EnumMap<TestEnum, int> map;
    EXPECT_EQ(map.size(), static_cast<size_t>(TestEnum::COUNT));
    EXPECT_EQ(map.max_size(), static_cast<size_t>(TestEnum::COUNT));
    EXPECT_FALSE(map.empty());

    EnumMap<Color, bool> map_color;
    EXPECT_EQ(map_color.size(), static_cast<size_t>(Color::COUNT));
    EXPECT_EQ(map_color.max_size(), static_cast<size_t>(Color::COUNT));
    EXPECT_FALSE(map_color.empty());

    // Test with explicit N=0
    EnumMap<TestEnum, int, 0> zero_map;
    EXPECT_EQ(zero_map.size(), 0);
    EXPECT_EQ(zero_map.max_size(), 0);
    EXPECT_TRUE(zero_map.empty());
}

TEST_F(EnumMapTest, Contains) {
    EnumMap<TestEnum, int> map;
    EXPECT_TRUE(map.contains(TestEnum::A));
    EXPECT_TRUE(map.contains(TestEnum::B));
    EXPECT_TRUE(map.contains(TestEnum::C));

    // Values outside the valid enum range [0, COUNT-1)
    EXPECT_FALSE(map.contains(static_cast<TestEnum>(static_cast<int>(TestEnum::COUNT))));
    EXPECT_FALSE(map.contains(static_cast<TestEnum>(-1)));
    EXPECT_FALSE(map.contains(static_cast<TestEnum>(100)));
}

TEST_F(EnumMapTest, Fill) {
    EnumMap<TestEnum, int> map;
    map.fill(42);
    EXPECT_EQ(map[TestEnum::A], 42);
    EXPECT_EQ(map[TestEnum::B], 42);
    EXPECT_EQ(map[TestEnum::C], 42);

    EnumMap<TestEnum, std::string> map_str;
    map_str.fill("filled");
    EXPECT_EQ(map_str[TestEnum::A], "filled");
    EXPECT_EQ(map_str[TestEnum::B], "filled");
    EXPECT_EQ(map_str[TestEnum::C], "filled");
}

TEST_F(EnumMapTest, Clear) {
    EnumMap<TestEnum, int> map = {{TestEnum::A, 1}, {TestEnum::B, 2}};
    map.clear(); // Resets to default (0 for int)
    EXPECT_EQ(map[TestEnum::A], 0);
    EXPECT_EQ(map[TestEnum::B], 0);
    EXPECT_EQ(map[TestEnum::C], 0);

    EnumMap<TestEnum, std::string> map_str = {{TestEnum::A, "Hi"}, {TestEnum::B, "There"}};
    map_str.clear(); // Resets to default ("" for string)
    EXPECT_EQ(map_str[TestEnum::A], "");
    EXPECT_EQ(map_str[TestEnum::B], "");
    EXPECT_EQ(map_str[TestEnum::C], "");

    EnumMap<TestEnum, MyStruct> map_struct = {{TestEnum::A, MyStruct{1, "ObjA"}}};
    map_struct.clear(); // Resets to default (MyStruct{0, ""})
    EXPECT_EQ(map_struct[TestEnum::A], MyStruct{});
    EXPECT_EQ(map_struct[TestEnum::B], MyStruct{});
}

TEST_F(EnumMapTest, Erase) {
    EnumMap<TestEnum, int> map = {{TestEnum::A, 10}, {TestEnum::B, 20}, {TestEnum::C, 30}};
    bool erased_b = map.erase(TestEnum::B);
    EXPECT_TRUE(erased_b);
    EXPECT_EQ(map[TestEnum::A], 10);
    EXPECT_EQ(map[TestEnum::B], 0); // Defaulted
    EXPECT_EQ(map[TestEnum::C], 30);

    bool erased_a = map.erase(TestEnum::A);
    EXPECT_TRUE(erased_a);
    EXPECT_EQ(map[TestEnum::A], 0); // Defaulted

    // Erase already default
    bool erased_c_default = map.erase(TestEnum::C); // C is 30, will become 0
    EXPECT_TRUE(erased_c_default);
    EXPECT_EQ(map[TestEnum::C], 0); // Defaulted
    bool erased_c_again = map.erase(TestEnum::C); // C is already 0, erase again
    EXPECT_TRUE(erased_c_again);
    EXPECT_EQ(map[TestEnum::C], 0); // Still default

    // Erase invalid key
    bool erased_invalid = map.erase(static_cast<TestEnum>(99));
    EXPECT_FALSE(erased_invalid);
    EXPECT_EQ(map[TestEnum::A], 0); // Unchanged
    EXPECT_EQ(map[TestEnum::B], 0); // Unchanged
    EXPECT_EQ(map[TestEnum::C], 0); // Unchanged

    EnumMap<TestEnum, MyStruct> map_struct = {
        {TestEnum::A, MyStruct{1, "A"}},
        {TestEnum::B, MyStruct{2, "B"}}
    };
    map_struct.erase(TestEnum::A);
    EXPECT_EQ(map_struct[TestEnum::A], MyStruct{});
    EXPECT_EQ(map_struct[TestEnum::B], (MyStruct{2, "B"})); // Parentheses for EXPECT_EQ
}

TEST_F(EnumMapTest, Swap) {
    EnumMap<TestEnum, int> map1 = {{TestEnum::A, 1}, {TestEnum::B, 2}};
    EnumMap<TestEnum, int> map2 = {{TestEnum::A, 10}, {TestEnum::C, 30}};

    map1.swap(map2);

    EXPECT_EQ(map1[TestEnum::A], 10);
    EXPECT_EQ(map1[TestEnum::B], 0); // Default from map2
    EXPECT_EQ(map1[TestEnum::C], 30);

    EXPECT_EQ(map2[TestEnum::A], 1);
    EXPECT_EQ(map2[TestEnum::B], 2);
    EXPECT_EQ(map2[TestEnum::C], 0); // Default from map1

    // Using std::swap
    std::swap(map1, map2);

    EXPECT_EQ(map1[TestEnum::A], 1);
    EXPECT_EQ(map1[TestEnum::B], 2);
    EXPECT_EQ(map1[TestEnum::C], 0);

    EXPECT_EQ(map2[TestEnum::A], 10);
    EXPECT_EQ(map2[TestEnum::B], 0);
    EXPECT_EQ(map2[TestEnum::C], 30);
}

TEST_F(EnumMapTest, EqualityOperators) {
    EnumMap<TestEnum, int> map1 = {{TestEnum::A, 1}, {TestEnum::B, 2}};
    EnumMap<TestEnum, int> map2 = {{TestEnum::A, 1}, {TestEnum::B, 2}};
    EnumMap<TestEnum, int> map3 = {{TestEnum::A, 1}, {TestEnum::B, 3}}; // B is different
    EnumMap<TestEnum, int> map4 = {{TestEnum::A, 1}}; // C is different (0 vs map1's 0)

    EXPECT_TRUE(map1 == map2);
    EXPECT_FALSE(map1 != map2);

    EXPECT_FALSE(map1 == map3);
    EXPECT_TRUE(map1 != map3);

    // map4 will have C=0 by default, map1 also has C=0 by default.
    // So map1 and map4 should be equal if TestEnum::C is the only other element.
    // If TestEnum has A, B, C, then map1 is {A:1, B:2, C:0} and map4 is {A:1, B:0, C:0}
    // So they are different.
    EXPECT_FALSE(map1 == map4);
    EXPECT_TRUE(map1 != map4);

    map4[TestEnum::B] = 2; // Now map4 is {A:1, B:2, C:0}, same as map1
    EXPECT_TRUE(map1 == map4);
}

TEST_F(EnumMapTest, ConstCorrectness) {
    const EnumMap<TestEnum, int> const_map = {
        {TestEnum::A, 10}, {TestEnum::B, 20}
    };

    EXPECT_EQ(const_map[TestEnum::A], 10);
    EXPECT_EQ(const_map.at(TestEnum::B), 20);
    EXPECT_TRUE(const_map.contains(TestEnum::A));
    EXPECT_FALSE(const_map.empty());
    EXPECT_EQ(const_map.size(), static_cast<size_t>(TestEnum::COUNT));

    // Iterate const map
    int sum = 0;
    for(const auto& pair : const_map) {
        sum += pair.second;
    }
    EXPECT_EQ(sum, 30); // 10 (A) + 20 (B) + 0 (C, default)

    // const_map[TestEnum::A] = 100; // Compile error: assignment of read-only location
    // const_map.at(TestEnum::A) = 100; // Compile error: passing 'const EnumMap<...>' as 'this' argument discards qualifiers
    // const_map.fill(5); // Compile error
    // const_map.clear(); // Compile error
    // const_map.erase(TestEnum::A); // Compile error

    // Test data() const version
    const std::array<int, static_cast<size_t>(TestEnum::COUNT)>& internal_data = const_map.data();
    EXPECT_EQ(internal_data[static_cast<size_t>(TestEnum::A)], 10);
}

TEST_F(EnumMapTest, ZeroSizedEnumMap) {
    enum class EmptyEnum {
        COUNT // Size is 0
    };
    EnumMap<EmptyEnum, int> map; // Uses default N = 0

    EXPECT_EQ(map.size(), 0);
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.max_size(), 0);

    EXPECT_FALSE(map.contains(static_cast<EmptyEnum>(0))); // No valid keys
    EXPECT_THROW(map.at(static_cast<EmptyEnum>(0)), std::out_of_range);
    // operator[] on a zero-sized map is problematic, depends on underlying std::array behavior.
    // std::array<T,0> is valid. data_[to_index(e)] where to_index(e) could be 0.
    // Accessing data_[0] on a 0-sized array is UB.
    // The static_assert for to_index(e) < N should prevent this if N=0.
    // Let's check `is_valid_enum` for N=0.
    // `to_index(e) < 0` is always false for unsigned `to_index(e)`.
    // So `contains` should be false. `at` should throw.
    // `operator[]` behavior is thus UB if not guarded.
    // Our operator[] has no guard, relies on underlying array.
    // Let's assume for N=0, operator[] isn't typically used or its use is conditional.

    auto it_begin = map.begin();
    auto it_end = map.end();
    EXPECT_EQ(it_begin, it_end); // Empty range

    map.fill(100); // Should do nothing gracefully
    map.clear();   // Should do nothing gracefully
    EXPECT_FALSE(map.erase(static_cast<EmptyEnum>(0))); // No valid key to erase

    EnumMap<EmptyEnum, int> map2;
    EXPECT_TRUE(map == map2); // Two empty maps are equal
    map.swap(map2); // Swapping empty maps
    EXPECT_TRUE(map == map2);
}

// Test for iterator proxy's operator->
struct Point { int x, y; };
TEST_F(EnumMapTest, IteratorProxyOperatorArrow) {
    EnumMap<TestEnum, Point> map_of_points;
    map_of_points[TestEnum::A] = {1, 2};
    map_of_points[TestEnum::B] = {3, 4};

    auto it = map_of_points.begin();
    // Access PairProxy members using (*it)
    ASSERT_EQ((*it).first, TestEnum::A);
    ASSERT_EQ((*it).second.x, 1);
    ASSERT_EQ((*it).second.y, 2);

    // Accessing members of Point (TValue) via iterator's special operator->
    // it.operator->() returns PairProxy.
    // PairProxy.operator->() returns Point*.
    // So it->x should work and be equivalent to (*it).second.x
    EXPECT_EQ(it->x, 1);
    EXPECT_EQ(it->y, 2);

    it->x = 10; // Modify TValue's member through proxy's operator->
    EXPECT_EQ(map_of_points[TestEnum::A].x, 10);
    EXPECT_EQ((*it).second.x, 10); // Verify change via (*it).second

    // Const iterator version
    const EnumMap<TestEnum, Point>& const_map = map_of_points;
    auto cit = const_map.cbegin();
    ASSERT_EQ((*cit).first, TestEnum::A);
    ASSERT_EQ((*cit).second.x, 10);
    ASSERT_EQ(cit->x, 10); // Access TValue's member via const proxy's operator->
    // cit->x = 100; // Should not compile (const TValue*)
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// Notes for CMake linkage:
// Link against GTest::gtest and GTest::gtest_main
// Ensure include directories are set up for enum_map.h
// Add this test file to the CMake build process.
// Example:
// add_executable(enum_map_test enum_map_test.cpp)
// target_link_libraries(enum_map_test PRIVATE GTest::gtest GTest::gtest_main YourLibContainingEnumMap)
// target_include_directories(enum_map_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
// include(GoogleTest)
// gtest_discover_tests(enum_map_test)

// Ensure MyStruct is default constructible for some tests
static_assert(std::is_default_constructible_v<MyStruct>, "MyStruct must be default constructible for these tests.");
static_assert(std::is_default_constructible_v<Point>, "Point must be default constructible for these tests.");

// Test that TValue does not need to be default constructible for basic construction if all values are provided
// This is more complex to test as `clear` and `erase` and default EnumMap constructor now require it.
// If an EnumMap is fully initialized via initializer list, TValue wouldn't strictly need default constructibility
// for *that instance* if it weren't for other methods.
// The current design with default construction in EnumMap() and clear/erase makes this less relevant.

// TODO: Consider testing with non-default-constructible types if EnumMap is adapted,
// or test that it correctly fails to compile if methods requiring default construction are used.
// For now, the static_asserts in EnumMap cover this.

// TODO: Test with enums that don't start at 0 (though C++ standard enums typically do if not specified).
// The `to_index` function `static_cast<std::size_t>(e)` assumes 0-based contiguous enums for array indexing.
// This is a fundamental design assumption of EnumMap.

// TODO: Test iterators with empty map (N=0). (Covered by ZeroSizedEnumMap test)

// TODO: Test iterator comparison operators thoroughly (<, <=, >, >=). (Partially covered)
// TODO: Test iterator arithmetic (it + n, it - n, it += n, it -= n, it[n]). (Partially covered)

// Test for potential issues with `operator EnumMapIterator<true>() const` (const conversion)
TEST_F(EnumMapTest, IteratorConstConversion) {
    EnumMap<TestEnum, int> map;
    map[TestEnum::A] = 1;

    EnumMap<TestEnum, int>::iterator it = map.begin();
    EnumMap<TestEnum, int>::const_iterator cit;

    cit = it; // Implicit conversion

    EXPECT_EQ((*cit).first, TestEnum::A);  // Use (*cit).first
    EXPECT_EQ((*cit).second, 1); // Use (*cit).second
    // (*cit).second = 2; // Should not compile

    // Ensure it still points to the same element
    EXPECT_EQ((*it).first, TestEnum::A); // Use (*it).first
    EXPECT_EQ((*it).second, 1); // Use (*it).second
}

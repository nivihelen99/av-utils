#include <gtest/gtest.h>
#include "FrozenList.h" // The header for the class we are testing
#include <vector>
#include <string>
#include <list>         // For testing construction from different iterator types
#include <numeric>      // For std::iota
#include <map>          // For testing with more complex types

// Helper to check contents of a FrozenList against a vector
template<typename T>
void require_list_equals_vector(const cpp_collections::FrozenList<T>& fl, const std::vector<T>& vec) {
    EXPECT_EQ(fl.size(), vec.size());
    EXPECT_EQ(fl.empty(), vec.empty());
    for (size_t i = 0; i < fl.size(); ++i) {
        EXPECT_EQ(fl[i], vec[i]);
        EXPECT_EQ(fl.at(i), vec.at(i));
    }
    EXPECT_TRUE(std::equal(fl.begin(), fl.end(), vec.begin(), vec.end()));
    if (!fl.empty()) {
        EXPECT_EQ(fl.front(), vec.front());
        EXPECT_EQ(fl.back(), vec.back());
    }
}

class FrozenListTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }
    
    void TearDown() override {
        // Cleanup code if needed
    }
};

// Construction Tests
TEST_F(FrozenListTest, DefaultConstructor) {
    cpp_collections::FrozenList<int> fl;
    EXPECT_TRUE(fl.empty());
    EXPECT_EQ(fl.size(), 0);
    EXPECT_THROW(fl.at(0), std::out_of_range);
}

TEST_F(FrozenListTest, ConstructorWithAllocator) {
    std::allocator<int> alloc;
    cpp_collections::FrozenList<int> fl(alloc);
    EXPECT_TRUE(fl.empty());
    EXPECT_EQ(fl.size(), 0);
    EXPECT_EQ(fl.get_allocator(), alloc);
}

TEST_F(FrozenListTest, ConstructorWithCountAndValue) {
    cpp_collections::FrozenList<int> fl(5, 10);
    std::vector<int> expected_vec(5, 10);
    require_list_equals_vector(fl, expected_vec);

    cpp_collections::FrozenList<std::string> fl_str(3, "test");
    std::vector<std::string> expected_vec_str(3, "test");
    require_list_equals_vector(fl_str, expected_vec_str);
}

TEST_F(FrozenListTest, ConstructorWithCountValueAndAllocator) {
    std::allocator<int> alloc;
    cpp_collections::FrozenList<int> fl(5, 10, alloc);
    std::vector<int> expected_vec(5, 10);
    require_list_equals_vector(fl, expected_vec);
    EXPECT_EQ(fl.get_allocator(), alloc);
}

TEST_F(FrozenListTest, ConstructorFromIteratorsVector) {
    std::vector<int> source_vec = {1, 2, 3, 4, 5};
    cpp_collections::FrozenList<int> fl(source_vec.begin(), source_vec.end());
    require_list_equals_vector(fl, source_vec);
}

TEST_F(FrozenListTest, ConstructorFromIteratorsList) {
    std::list<std::string> source_list = {"a", "b", "c"};
    cpp_collections::FrozenList<std::string> fl(source_list.begin(), source_list.end());
    std::vector<std::string> expected_vec = {"a", "b", "c"};
    require_list_equals_vector(fl, expected_vec);
}

TEST_F(FrozenListTest, ConstructorFromIteratorsEmptyRange) {
    std::vector<int> empty_vec;
    cpp_collections::FrozenList<int> fl(empty_vec.begin(), empty_vec.end());
    EXPECT_TRUE(fl.empty());
    EXPECT_EQ(fl.size(), 0);
}

TEST_F(FrozenListTest, ConstructorFromIteratorsWithAllocator) {
    std::vector<int> source_vec = {1, 2, 3};
    std::allocator<int> alloc;
    cpp_collections::FrozenList<int> fl(source_vec.begin(), source_vec.end(), alloc);
    require_list_equals_vector(fl, source_vec);
    EXPECT_EQ(fl.get_allocator(), alloc);
}

TEST_F(FrozenListTest, ConstructorFromInitializerList) {
    cpp_collections::FrozenList<int> fl = {10, 20, 30};
    std::vector<int> expected_vec = {10, 20, 30};
    require_list_equals_vector(fl, expected_vec);

    cpp_collections::FrozenList<int> fl_empty_init = {};
    EXPECT_TRUE(fl_empty_init.empty());
}

TEST_F(FrozenListTest, ConstructorFromInitializerListWithAllocator) {
    std::allocator<int> alloc;
    cpp_collections::FrozenList<int> fl({10, 20, 30}, alloc);
    std::vector<int> expected_vec = {10, 20, 30};
    require_list_equals_vector(fl, expected_vec);
    EXPECT_EQ(fl.get_allocator(), alloc);
}

TEST_F(FrozenListTest, CopyConstructor) {
    cpp_collections::FrozenList<int> original = {1, 2, 3};
    cpp_collections::FrozenList<int> copy(original);
    require_list_equals_vector(copy, {1, 2, 3});
    EXPECT_NE(original.data(), copy.data()); // Should be a deep copy
}

TEST_F(FrozenListTest, CopyConstructorWithAllocator) {
    cpp_collections::FrozenList<int> original = {1, 2, 3};
    std::allocator<int> alloc;
    cpp_collections::FrozenList<int> copy(original, alloc);
    require_list_equals_vector(copy, {1, 2, 3});
    EXPECT_EQ(copy.get_allocator(), alloc);
    EXPECT_NE(original.data(), copy.data());
}

TEST_F(FrozenListTest, MoveConstructor) {
    cpp_collections::FrozenList<int> original = {1, 2, 3, 4, 5};
    size_t original_size = original.size();

    cpp_collections::FrozenList<int> moved_to(std::move(original));
    
    require_list_equals_vector(moved_to, {1, 2, 3, 4, 5});
    EXPECT_EQ(moved_to.size(), original_size);

    // `original` is in a valid but unspecified state.
    // Common behavior for std::vector move is original becomes empty.
    EXPECT_TRUE(original.empty()); // Check common behavior
    EXPECT_EQ(original.size(), 0);
}

TEST_F(FrozenListTest, MoveConstructorWithAllocator) {
    std::allocator<int> alloc;
    cpp_collections::FrozenList<int> original = {1, 2, 3};
    size_t original_size = original.size();

    cpp_collections::FrozenList<int> moved_to(std::move(original), alloc);
    
    require_list_equals_vector(moved_to, {1, 2, 3});
    EXPECT_EQ(moved_to.size(), original_size);
    EXPECT_EQ(moved_to.get_allocator(), alloc);
    
    EXPECT_TRUE(original.empty()); 
}

// Element Access Tests
class FrozenListElementAccessTest : public ::testing::Test {
protected:
    void SetUp() override {
        fl = {10, 20, 30, 40};
    }
    
    cpp_collections::FrozenList<int> fl;
};

TEST_F(FrozenListElementAccessTest, OperatorBrackets) {
    const cpp_collections::FrozenList<int>& cfl = fl; // const reference
    
    EXPECT_EQ(fl[0], 10);
    EXPECT_EQ(fl[2], 30);
    EXPECT_EQ(cfl[0], 10); // const version
}

TEST_F(FrozenListElementAccessTest, AtMethod) {
    const cpp_collections::FrozenList<int>& cfl = fl; // const reference
    
    EXPECT_EQ(fl.at(0), 10);
    EXPECT_EQ(fl.at(3), 40);
    EXPECT_EQ(cfl.at(1), 20); // const version
    EXPECT_THROW(fl.at(4), std::out_of_range);
    EXPECT_THROW(cfl.at(10), std::out_of_range);
}

TEST_F(FrozenListElementAccessTest, FrontMethod) {
    const cpp_collections::FrozenList<int>& cfl = fl;
    
    EXPECT_EQ(fl.front(), 10);
    EXPECT_EQ(cfl.front(), 10);
}

TEST_F(FrozenListElementAccessTest, BackMethod) {
    const cpp_collections::FrozenList<int>& cfl = fl;
    
    EXPECT_EQ(fl.back(), 40);
    EXPECT_EQ(cfl.back(), 40);
}

TEST_F(FrozenListElementAccessTest, DataMethod) {
    const cpp_collections::FrozenList<int>& cfl = fl;
    
    const int* ptr = fl.data();
    EXPECT_EQ(ptr[0], 10);
    EXPECT_EQ(ptr[1], 20);

    const int* cptr = cfl.data();
    EXPECT_EQ(cptr[0], 10);
}

TEST_F(FrozenListElementAccessTest, AccessOnEmptyList) {
    cpp_collections::FrozenList<int> empty_fl;
    const cpp_collections::FrozenList<int>& c_empty_fl = empty_fl;
    
    EXPECT_THROW(empty_fl.at(0), std::out_of_range);
    EXPECT_EQ(empty_fl.data(), nullptr); // Or some valid ptr if vector impl does that
    EXPECT_EQ(c_empty_fl.data(), nullptr);
}

// Iterator Tests
class FrozenListIteratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        fl = {1, 2, 3};
    }
    
    cpp_collections::FrozenList<int> fl;
};

TEST_F(FrozenListIteratorTest, BeginAndEnd) {
    const cpp_collections::FrozenList<int>& cfl = fl;
    
    std::vector<int> collected;
    for (auto it = fl.begin(); it != fl.end(); ++it) {
        collected.push_back(*it);
    }
    require_list_equals_vector(fl, collected);
}

TEST_F(FrozenListIteratorTest, CBeginAndCEnd) {
    const cpp_collections::FrozenList<int>& cfl = fl;
    
    std::vector<int> collected;
    for (auto it = fl.cbegin(); it != fl.cend(); ++it) {
        collected.push_back(*it);
    }
    require_list_equals_vector(fl, collected);
    
    std::vector<int> c_collected;
    for (auto it = cfl.begin(); it != cfl.end(); ++it) { // cfl.begin() is const_iterator
        c_collected.push_back(*it);
    }
    require_list_equals_vector(cfl, c_collected);
}

TEST_F(FrozenListIteratorTest, RangeBasedForLoop) {
    const cpp_collections::FrozenList<int>& cfl = fl;
    
    std::vector<int> collected;
    for(const auto& item : fl) {
        collected.push_back(item);
    }
    require_list_equals_vector(fl, collected);

    std::vector<int> c_collected;
    for(const auto& item : cfl) {
        c_collected.push_back(item);
    }
    require_list_equals_vector(cfl, c_collected);
}

TEST_F(FrozenListIteratorTest, RBeginAndREnd) {
    std::vector<int> collected_rev;
    for (auto it = fl.rbegin(); it != fl.rend(); ++it) {
        collected_rev.push_back(*it);
    }
    std::vector<int> expected_rev = {3, 2, 1};
    EXPECT_EQ(collected_rev, expected_rev);
}

TEST_F(FrozenListIteratorTest, CRBeginAndCREnd) {
    const cpp_collections::FrozenList<int>& cfl = fl;
    
    std::vector<int> collected_rev;
    for (auto it = fl.crbegin(); it != fl.crend(); ++it) {
        collected_rev.push_back(*it);
    }
    std::vector<int> expected_rev = {3, 2, 1};
    EXPECT_EQ(collected_rev, expected_rev);

    std::vector<int> c_collected_rev;
    for (auto it = cfl.rbegin(); it != cfl.rend(); ++it) { // cfl.rbegin() is const_reverse_iterator
        c_collected_rev.push_back(*it);
    }
    EXPECT_EQ(c_collected_rev, expected_rev);
}

TEST_F(FrozenListIteratorTest, IteratorsOnEmptyList) {
    cpp_collections::FrozenList<int> empty_fl;
    EXPECT_EQ(empty_fl.begin(), empty_fl.end());
    EXPECT_EQ(empty_fl.cbegin(), empty_fl.cend());
    EXPECT_EQ(empty_fl.rbegin(), empty_fl.rend());
    EXPECT_EQ(empty_fl.crbegin(), empty_fl.crend());
}

// Capacity Tests
TEST_F(FrozenListTest, EmptyAndSize) {
    cpp_collections::FrozenList<int> fl_empty;
    EXPECT_TRUE(fl_empty.empty());
    EXPECT_EQ(fl_empty.size(), 0);

    cpp_collections::FrozenList<int> fl_non_empty = {1, 2, 3};
    EXPECT_FALSE(fl_non_empty.empty());
    EXPECT_EQ(fl_non_empty.size(), 3);
}

TEST_F(FrozenListTest, MaxSize) {
    cpp_collections::FrozenList<int> fl;
    std::vector<int> vec;
    EXPECT_EQ(fl.max_size(), vec.max_size()); // Should be same as underlying vector
}

// Comparison Operators Tests
class FrozenListComparisonTest : public ::testing::Test {
protected:
    void SetUp() override {
        fl1 = {1, 2, 3};
        fl2 = {1, 2, 3};
        fl3 = {1, 2, 4};
        fl4 = {1, 2};
        // fl_empty is default constructed (empty)
    }
    
    cpp_collections::FrozenList<int> fl1;
    cpp_collections::FrozenList<int> fl2;
    cpp_collections::FrozenList<int> fl3;
    cpp_collections::FrozenList<int> fl4;
    cpp_collections::FrozenList<int> fl_empty;
};

TEST_F(FrozenListComparisonTest, EqualityOperator) {
    EXPECT_EQ(fl1, fl2);
    EXPECT_FALSE(fl1 == fl3);
    EXPECT_FALSE(fl1 == fl4);
    EXPECT_FALSE(fl1 == fl_empty);
    EXPECT_EQ(cpp_collections::FrozenList<int>(), cpp_collections::FrozenList<int>());
}

TEST_F(FrozenListComparisonTest, InequalityOperator) {
    EXPECT_FALSE(fl1 != fl2);
    EXPECT_NE(fl1, fl3);
    EXPECT_NE(fl1, fl4);
    EXPECT_NE(fl1, fl_empty);
}

TEST_F(FrozenListComparisonTest, LessThanOperator) {
    EXPECT_FALSE(fl1 < fl2); // equal
    EXPECT_LT(fl1, fl3);     // {1,2,3} < {1,2,4}
    EXPECT_LT(fl4, fl1);     // {1,2} < {1,2,3} (shorter)
    EXPECT_FALSE(fl3 < fl1);
    EXPECT_LT(fl_empty, fl1);
    EXPECT_FALSE(fl1 < fl_empty);
}

TEST_F(FrozenListComparisonTest, LessThanOrEqualOperator) {
    EXPECT_LE(fl1, fl2); // equal
    EXPECT_LE(fl1, fl3);
    EXPECT_LE(fl4, fl1);
    EXPECT_FALSE(fl3 <= fl1);
    EXPECT_LE(fl_empty, fl1);
    EXPECT_LE(fl_empty, cpp_collections::FrozenList<int>());
}

TEST_F(FrozenListComparisonTest, GreaterThanOperator) {
    EXPECT_FALSE(fl1 > fl2); // equal
    EXPECT_FALSE(fl1 > fl3);
    EXPECT_FALSE(fl4 > fl1);
    EXPECT_GT(fl3, fl1);
    EXPECT_GT(fl1, fl4);
    EXPECT_GT(fl1, fl_empty);
    EXPECT_FALSE(fl_empty > fl1);
}

TEST_F(FrozenListComparisonTest, GreaterThanOrEqualOperator) {
    EXPECT_GE(fl1, fl2); // equal
    EXPECT_FALSE(fl1 >= fl3);
    EXPECT_FALSE(fl4 >= fl1);
    EXPECT_GE(fl3, fl1);
    EXPECT_GE(fl1, fl4);
    EXPECT_GE(fl1, fl_empty);
    EXPECT_GE(cpp_collections::FrozenList<int>(), cpp_collections::FrozenList<int>());
}

// Swap Tests
TEST_F(FrozenListTest, MemberSwap) {
    cpp_collections::FrozenList<int> fl1 = {1, 2, 3};
    cpp_collections::FrozenList<int> fl2 = {10, 20};
    
    std::vector<int> v1 = {1,2,3};
    std::vector<int> v2 = {10,20};

    fl1.swap(fl2);
    require_list_equals_vector(fl1, v2);
    require_list_equals_vector(fl2, v1);
}

TEST_F(FrozenListTest, NonMemberSwap) {
    cpp_collections::FrozenList<int> fl1 = {1, 2, 3};
    cpp_collections::FrozenList<int> fl2 = {10, 20};
    
    std::vector<int> v1 = {1,2,3};
    std::vector<int> v2 = {10,20};

    using std::swap; // Enable ADL
    swap(fl1, fl2); // Should call our non-member swap
    require_list_equals_vector(fl1, v2);
    require_list_equals_vector(fl2, v1);
}

// Hash Tests
TEST_F(FrozenListTest, HashSpecialization) {
    std::hash<cpp_collections::FrozenList<int>> hasher;
    
    cpp_collections::FrozenList<int> fl1 = {1, 2, 3};
    cpp_collections::FrozenList<int> fl2 = {1, 2, 3};
    cpp_collections::FrozenList<int> fl3 = {3, 2, 1};
    cpp_collections::FrozenList<int> fl_empty;

    EXPECT_EQ(hasher(fl1), hasher(fl2)); // Equal lists, equal hashes
    
    // Hash of empty list should be consistent
    EXPECT_EQ(hasher(fl_empty), hasher(cpp_collections::FrozenList<int>()));

    // Hash involving different types
    std::hash<cpp_collections::FrozenList<std::string>> str_hasher;
    cpp_collections::FrozenList<std::string> fl_str1 = {"a", "b"};
    cpp_collections::FrozenList<std::string> fl_str2 = {"a", "b"};
    cpp_collections::FrozenList<std::string> fl_str3 = {"b", "a"};
    EXPECT_EQ(str_hasher(fl_str1), str_hasher(fl_str2));
}

// Assignment Operators Tests
class FrozenListAssignmentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup if needed
    }
};

TEST_F(FrozenListAssignmentTest, CopyAssignment) {
    cpp_collections::FrozenList<int> fl1 = {1, 2, 3};
    cpp_collections::FrozenList<int> fl2 = {10, 20};
    
    fl1 = fl2; // fl1 now {10, 20}
    require_list_equals_vector(fl1, {10, 20});
    EXPECT_EQ(fl1.size(), 2);
    EXPECT_EQ(fl1[0], 10);
    
    // Original fl2 should be unchanged
    require_list_equals_vector(fl2, {10, 20});

    // Self-assignment
    fl1 = fl1;
    require_list_equals_vector(fl1, {10, 20});
}

TEST_F(FrozenListAssignmentTest, MoveAssignment) {
    cpp_collections::FrozenList<int> fl1 = {1, 2, 3};
    cpp_collections::FrozenList<int> fl2 = {10, 20, 30, 40};

    fl1 = std::move(fl2);
    require_list_equals_vector(fl1, {10, 20, 30, 40});

    // fl2 is in a valid but unspecified state (likely empty for std::vector)
    EXPECT_TRUE(fl2.empty());

    // Self-move assignment (should not crash, well-defined)
    cpp_collections::FrozenList<int> fl_self = {5,6,7};
    fl_self = std::move(fl_self);
    require_list_equals_vector(fl_self, {5,6,7}); // Should be unchanged
}

TEST_F(FrozenListAssignmentTest, InitializerListAssignment) {
    cpp_collections::FrozenList<int> fl1 = {1, 2, 3};
    fl1 = {100, 200, 300, 400, 500};
    require_list_equals_vector(fl1, {100, 200, 300, 400, 500});
    EXPECT_EQ(fl1.size(), 5);

    fl1 = {}; // Assign empty list
    EXPECT_TRUE(fl1.empty());
}

// Complex Types Tests
TEST_F(FrozenListTest, ListOfStrings) {
    cpp_collections::FrozenList<std::string> fl_str = {"hello", "world", "frozen", "list"};
    std::vector<std::string> vec_str = {"hello", "world", "frozen", "list"};
    require_list_equals_vector(fl_str, vec_str);
}

TEST_F(FrozenListTest, ListOfVectors) {
    using VecInt = std::vector<int>;
    cpp_collections::FrozenList<VecInt> fl_vec = {{1,2}, {3,4,5}, {}};
    std::vector<VecInt> vec_vec = {{1,2}, {3,4,5}, {}};
    require_list_equals_vector(fl_vec, vec_vec);
}

// Deduction Guides Tests (C++17)
#if __cplusplus >= 201703L
TEST_F(FrozenListTest, DeductionGuides) {
    // These tests will only compile if deduction guides are working correctly in C++17 or later
    std::vector<int> v_int = {1, 2, 3};
    cpp_collections::FrozenList fl_from_iter(v_int.begin(), v_int.end()); // Deduce FrozenList<int>
    EXPECT_TRUE((std::is_same_v<decltype(fl_from_iter), cpp_collections::FrozenList<int, std::allocator<int>>>));
    require_list_equals_vector(fl_from_iter, v_int);

    cpp_collections::FrozenList fl_from_init = {10.0, 20.0, 30.0}; // Deduce FrozenList<double>
    EXPECT_TRUE((std::is_same_v<decltype(fl_from_init), cpp_collections::FrozenList<double, std::allocator<double>>>));
    std::vector<double> v_double = {10.0, 20.0, 30.0};
    require_list_equals_vector(fl_from_init, v_double);
    
    cpp_collections::FrozenList fl_from_fill(5u, std::string("fill")); // Deduce FrozenList<std::string>
    EXPECT_TRUE((std::is_same_v<decltype(fl_from_fill), cpp_collections::FrozenList<std::string, std::allocator<std::string>>>));
    std::vector<std::string> v_string(5, "fill");
    require_list_equals_vector(fl_from_fill, v_string);

    // With explicit allocator
    std::allocator<long> myAlloc;
    cpp_collections::FrozenList fl_from_iter_alloc(v_int.begin(), v_int.end(), myAlloc);
    EXPECT_TRUE((std::is_same_v<decltype(fl_from_iter_alloc), cpp_collections::FrozenList<int, std::allocator<long>>>));
    require_list_equals_vector(fl_from_iter_alloc, v_int);
    EXPECT_EQ(fl_from_iter_alloc.get_allocator(), myAlloc);

    cpp_collections::FrozenList fl_from_init_alloc({1L, 2L, 3L}, myAlloc); // Deduce FrozenList<long, std::allocator<long>>
    EXPECT_TRUE((std::is_same_v<decltype(fl_from_init_alloc), cpp_collections::FrozenList<long, std::allocator<long>>>));
    std::vector<long> v_long = {1L, 2L, 3L};
    require_list_equals_vector(fl_from_init_alloc, v_long);
    EXPECT_EQ(fl_from_init_alloc.get_allocator(), myAlloc);
}
#endif

// Main function for running tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

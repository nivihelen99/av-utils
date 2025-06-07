#include <gtest/gtest.h>
#include "persist_array.h" // Adjust path as necessary if headers are not directly in include/
#include <string>
#include <vector>

// Test fixture for PersistentArray tests
class PersistentArrayTest : public ::testing::Test {
protected:
    PersistentArray<int> arr_int;
    PersistentArray<std::string> arr_str;

    // Helper to get underlying data pointer address for checking CoW
    // This relies on the internal structure (root->data.data())
    // May need to add a debug method to PersistentArray if direct access is not possible/desirable
    // For now, we can use the public use_count() as an indicator.
};

// REQ-9: Construction and Initialization
TEST_F(PersistentArrayTest, DefaultConstructor) { // REQ-9.1
    PersistentArray<int> pa;
    EXPECT_EQ(pa.size(), 0);
    EXPECT_TRUE(pa.empty());
    EXPECT_EQ(pa.use_count(), 1);
}

TEST_F(PersistentArrayTest, SizeConstructor) { // REQ-9.2
    PersistentArray<int> pa(5);
    EXPECT_EQ(pa.size(), 5);
    EXPECT_FALSE(pa.empty());
    EXPECT_EQ(pa.use_count(), 1);
    // Values are default-initialized (0 for int)
    for (size_t i = 0; i < pa.size(); ++i) {
        EXPECT_EQ(pa[i], 0);
    }
}

TEST_F(PersistentArrayTest, SizeAndValueConstructor) { // REQ-9.3
    PersistentArray<std::string> pa(3, "hello");
    EXPECT_EQ(pa.size(), 3);
    EXPECT_FALSE(pa.empty());
    EXPECT_EQ(pa.use_count(), 1);
    for (size_t i = 0; i < pa.size(); ++i) {
        EXPECT_EQ(pa[i], "hello");
    }
}

TEST_F(PersistentArrayTest, InitializerListConstructor) { // REQ-9.4
    PersistentArray<int> pa = {10, 20, 30};
    EXPECT_EQ(pa.size(), 3);
    EXPECT_EQ(pa[0], 10);
    EXPECT_EQ(pa[1], 20);
    EXPECT_EQ(pa[2], 30);
    EXPECT_EQ(pa.use_count(), 1);
}

// REQ-1: Copy-on-Write Semantics & REQ-9.5: Copy Constructor
TEST_F(PersistentArrayTest, CopyConstructorSharesData) { // REQ-1.1, REQ-9.5
    PersistentArray<int> pa1 = {1, 2, 3};
    EXPECT_EQ(pa1.use_count(), 1);

    PersistentArray<int> pa2 = pa1; // Copy constructor
    EXPECT_EQ(pa1.use_count(), 2); // Data should be shared
    EXPECT_EQ(pa2.use_count(), 2);
    EXPECT_EQ(pa1.size(), 3);
    EXPECT_EQ(pa2.size(), 3);
    EXPECT_EQ(pa1[1], 2);
    EXPECT_EQ(pa2[1], 2);
    EXPECT_TRUE(pa1 == pa2); // Content comparison
}

TEST_F(PersistentArrayTest, ModificationTriggersCopyOnWrite) { // REQ-1.2, REQ-1.3
    PersistentArray<int> pa1 = {10, 20, 30};
    PersistentArray<int> pa2 = pa1; // pa1 and pa2 share data

    EXPECT_EQ(pa1.use_count(), 2);
    EXPECT_EQ(pa2.use_count(), 2);

    PersistentArray<int> pa3 = pa2.set(1, 200); // Modify pa2, should create pa3

    // pa1 should be unchanged and share data with the original pa2's data node
    EXPECT_EQ(pa1[0], 10);
    EXPECT_EQ(pa1[1], 20);
    EXPECT_EQ(pa1[2], 30);
    EXPECT_EQ(pa1.use_count(), 2); // pa1 still shares with original state of pa2

    // pa2 itself (the object instance) is const, set returns a new PersistentArray.
    // The original pa2 object still points to the shared data.
    EXPECT_EQ(pa2[0], 10);
    EXPECT_EQ(pa2[1], 20);
    EXPECT_EQ(pa2[2], 30);
    EXPECT_EQ(pa2.use_count(), 2);


    // pa3 should have the modification and its own data
    EXPECT_EQ(pa3[0], 10);
    EXPECT_EQ(pa3[1], 200);
    EXPECT_EQ(pa3[2], 30);
    EXPECT_EQ(pa3.use_count(), 1); // pa3 has its own unique data

    // Check that pa1 and pa2 are still equal and different from pa3
    EXPECT_TRUE(pa1 == pa2);
    EXPECT_FALSE(pa1 == pa3);
    EXPECT_FALSE(pa2 == pa3);
}

TEST_F(PersistentArrayTest, AssignmentOperatorSharesData) { // REQ-1.1
    PersistentArray<int> pa1 = {1, 2, 3};
    PersistentArray<int> pa2 = {4, 5, 6};

    EXPECT_EQ(pa1.use_count(), 1);
    EXPECT_EQ(pa2.use_count(), 1);

    pa2 = pa1; // Assignment operator

    EXPECT_EQ(pa1.use_count(), 2); // Data should be shared
    EXPECT_EQ(pa2.use_count(), 2);
    EXPECT_EQ(pa2.size(), 3);
    EXPECT_EQ(pa2[1], 2);
    EXPECT_TRUE(pa1 == pa2);
}


TEST_F(PersistentArrayTest, ModificationAfterAssignmentTriggersCoW) { // REQ-1.2, REQ-1.3
    PersistentArray<int> pa1 = {10, 20, 30};
    PersistentArray<int> pa2;

    pa2 = pa1; // pa1 and pa2 share data
    EXPECT_EQ(pa1.use_count(), 2);
    EXPECT_EQ(pa2.use_count(), 2);

    PersistentArray<int> pa3 = pa2.set(1, 200);

    EXPECT_EQ(pa1[1], 20); // pa1 unchanged
    EXPECT_EQ(pa2[1], 20); // pa2 unchanged (set is const)
    EXPECT_EQ(pa3[1], 200); // pa3 has modification

    EXPECT_EQ(pa1.use_count(), 2); // pa1 and pa2 still share
    EXPECT_EQ(pa2.use_count(), 2);
    EXPECT_EQ(pa3.use_count(), 1); // pa3 is unique

    EXPECT_TRUE(pa1 == pa2);
    EXPECT_FALSE(pa1 == pa3);
}

// REQ-9.6: Move Constructor and Assignment
TEST_F(PersistentArrayTest, MoveConstructor) {
    PersistentArray<int> pa1 = {1, 2, 3};
    // Ensure pa1's root is unique before move, or use_count might be misleading after if it wasn't.
    // Default construction already gives unique root.

    PersistentArray<int> pa2 = std::move(pa1);

    EXPECT_EQ(pa2.size(), 3);
    EXPECT_EQ(pa2[0], 1);
    EXPECT_EQ(pa2.use_count(), 1); // pa2 should now own the data exclusively.

    // According to standard library practices for moved-from objects,
    // pa1 should be in a valid but unspecified state.
    // Checking size/empty is reasonable. Accessing elements might be risky.
    // Our PersistentArray's move constructor explicitly sets other.root to nullptr.
    EXPECT_EQ(pa1.size(), 0);
    EXPECT_TRUE(pa1.empty());
    // Accessing pa1.use_count() might be problematic if root is nullptr.
    // The use_count() method in PersistentArray returns root.use_count(),
    // which would dereference a nullptr if root is null.
    // Let's assume pa1.root is nullptr, so pa1.use_count() would crash.
    // A safer check for a moved-from object is its state (empty, size 0).
}

TEST_F(PersistentArrayTest, MoveAssignment) {
    PersistentArray<int> pa1 = {1, 2, 3};
    PersistentArray<int> pa2 = {4, 5, 6, 7};

    // Ensure pa1's data is uniquely held if we want to check its use_count after move.
    // And pa2's data will be released.
    long pa2_initial_use_count = pa2.use_count(); // Should be 1 if constructed as {4,5,6,7}
    ASSERT_EQ(pa2_initial_use_count, 1); // Ensure it's not shared before assignment.

    pa2 = std::move(pa1);

    EXPECT_EQ(pa2.size(), 3);
    EXPECT_EQ(pa2[0], 1);
    EXPECT_EQ(pa2.use_count(), 1); // pa2 owns the data from pa1.

    // pa1 should be in a valid but unspecified state (empty for our impl).
    EXPECT_EQ(pa1.size(), 0);
    EXPECT_TRUE(pa1.empty());
}


// REQ-2.1: Creating new version O(1)
// REQ-2.2: Independently accessible/modifiable
// REQ-2.3: Lifetime management (automatic cleanup)
// REQ-2.4: Arbitrary number of concurrent versions
TEST_F(PersistentArrayTest, VersionManagement) { // REQ-2.1, REQ-2.2, REQ-2.3, REQ-2.4
    PersistentArray<int> v1 = {10, 20, 30};
    EXPECT_EQ(v1.use_count(), 1);

    PersistentArray<int> v2 = v1; // New "version" via copy (shares data) - O(1) for shared_ptr copy
    EXPECT_EQ(v1.use_count(), 2);
    EXPECT_EQ(v2.use_count(), 2);

    PersistentArray<int> v3 = v1.set(0, 100); // New version via modification
    EXPECT_EQ(v1.use_count(), 2); // v1, v2 share original data
    EXPECT_EQ(v2.use_count(), 2);
    EXPECT_EQ(v3.use_count(), 1); // v3 has new data
    EXPECT_EQ(v1[0], 10);
    EXPECT_EQ(v2[0], 10);
    EXPECT_EQ(v3[0], 100);

    PersistentArray<int> v4 = v2.set(1, 200);
    EXPECT_EQ(v1.use_count(), 2); // v1, v2 share
    EXPECT_EQ(v2.use_count(), 2);
    EXPECT_EQ(v3.use_count(), 1); // v3 unique
    EXPECT_EQ(v4.use_count(), 1); // v4 unique
    EXPECT_EQ(v1[1], 20);
    EXPECT_EQ(v2[1], 20);
    EXPECT_EQ(v3[1], 20); // v3 was based on v1
    EXPECT_EQ(v4[1], 200);

    // Test lifetime management (versions go out of scope)
    {
        PersistentArray<int> v_temp = v1;
        EXPECT_EQ(v1.use_count(), 3); // v1, v2, v_temp share
    } // v_temp goes out of scope
    EXPECT_EQ(v1.use_count(), 2); // v1, v2 share
}


// REQ-3: Array Operations
TEST_F(PersistentArrayTest, AccessOperatorAndAt) { // REQ-3.1
    PersistentArray<int> pa = {0, 10, 20, 30, 40};

    // Test const operator[]
    const PersistentArray<int>& cpa = pa;
    EXPECT_EQ(cpa[0], 0);
    EXPECT_EQ(cpa[2], 20);
    EXPECT_EQ(cpa[4], 40);

    // Test at()
    EXPECT_EQ(pa.at(1), 10);
    EXPECT_EQ(cpa.at(1), 10);

    // Bounds checking for operator[] (as per Step 1 refinement)
    EXPECT_THROW(pa[5], std::out_of_range);
    EXPECT_THROW(cpa[5], std::out_of_range);
    EXPECT_THROW(pa[100], std::out_of_range);
    EXPECT_THROW(cpa[100], std::out_of_range);
    PersistentArray<int> empty_pa;
    EXPECT_THROW(empty_pa[0], std::out_of_range);


    // Bounds checking for at()
    EXPECT_THROW(pa.at(5), std::out_of_range);
    EXPECT_THROW(cpa.at(5), std::out_of_range);
    EXPECT_THROW(pa.at(100), std::out_of_range);
    EXPECT_THROW(cpa.at(100), std::out_of_range);
    EXPECT_THROW(empty_pa.at(0), std::out_of_range);
}

TEST_F(PersistentArrayTest, SetOperation) { // REQ-3.2
    PersistentArray<int> pa1 = {1, 2, 3};
    PersistentArray<int> pa2 = pa1.set(1, 200);

    EXPECT_EQ(pa1[1], 2); // Original unchanged
    EXPECT_EQ(pa2[1], 200); // New version changed
    EXPECT_EQ(pa1.use_count(), 1); // pa1 is now unique if no other copies exist
    EXPECT_EQ(pa2.use_count(), 1);
    EXPECT_NE(pa1, pa2);

    EXPECT_THROW(pa1.set(3, 99), std::out_of_range); // Bounds check
    EXPECT_THROW(pa1.set(100, 99), std::out_of_range);
}

TEST_F(PersistentArrayTest, PushBackOperation) { // REQ-3.5
    PersistentArray<int> pa1;
    EXPECT_TRUE(pa1.empty());

    PersistentArray<int> pa2 = pa1.push_back(10);
    EXPECT_EQ(pa1.size(), 0); // Original unchanged
    EXPECT_EQ(pa2.size(), 1);
    EXPECT_EQ(pa2[0], 10);
    EXPECT_EQ(pa1.use_count(), 1);
    EXPECT_EQ(pa2.use_count(), 1);

    PersistentArray<int> pa3 = pa2.push_back(20);
    EXPECT_EQ(pa2.size(), 1); // pa2 unchanged
    EXPECT_EQ(pa2[0], 10);
    EXPECT_EQ(pa3.size(), 2);
    EXPECT_EQ(pa3[0], 10);
    EXPECT_EQ(pa3[1], 20);
    EXPECT_EQ(pa2.use_count(), 1);
    EXPECT_EQ(pa3.use_count(), 1);
}

TEST_F(PersistentArrayTest, PopBackOperation) { // REQ-3.6
    PersistentArray<int> pa1 = {1, 2, 3};
    PersistentArray<int> pa2 = pa1.pop_back();

    EXPECT_EQ(pa1.size(), 3); // Original unchanged
    EXPECT_EQ(pa2.size(), 2);
    EXPECT_EQ(pa2[0], 1);
    EXPECT_EQ(pa2[1], 2);
    EXPECT_EQ(pa1.use_count(), 1);
    EXPECT_EQ(pa2.use_count(), 1);

    PersistentArray<int> pa3 = pa2.pop_back().pop_back(); // Pop twice
    EXPECT_EQ(pa2.size(), 2); // pa2 unchanged
    EXPECT_TRUE(pa3.empty());
    EXPECT_EQ(pa3.use_count(), 1);

    PersistentArray<int> empty_pa;
    EXPECT_THROW(empty_pa.pop_back(), std::runtime_error); // Pop from empty
}

TEST_F(PersistentArrayTest, InsertOperation) { // REQ-3.3
    PersistentArray<int> pa1 = {10, 20, 30};

    // Insert at beginning
    PersistentArray<int> pa2 = pa1.insert(0, 5);
    EXPECT_EQ(pa1.size(), 3); // Original
    ASSERT_EQ(pa2.size(), 4);
    EXPECT_EQ(pa2[0], 5); EXPECT_EQ(pa2[1], 10); EXPECT_EQ(pa2[2], 20); EXPECT_EQ(pa2[3], 30);
    EXPECT_EQ(pa1.use_count(), 1); EXPECT_EQ(pa2.use_count(), 1);

    // Insert in middle
    PersistentArray<int> pa3 = pa1.insert(1, 15);
    ASSERT_EQ(pa3.size(), 4);
    EXPECT_EQ(pa3[0], 10); EXPECT_EQ(pa3[1], 15); EXPECT_EQ(pa3[2], 20); EXPECT_EQ(pa3[3], 30);
    EXPECT_EQ(pa3.use_count(), 1);

    // Insert at end (equivalent to push_back)
    PersistentArray<int> pa4 = pa1.insert(3, 35);
    ASSERT_EQ(pa4.size(), 4);
    EXPECT_EQ(pa4[0], 10); EXPECT_EQ(pa4[1], 20); EXPECT_EQ(pa4[2], 30); EXPECT_EQ(pa4[3], 35);
    EXPECT_EQ(pa4.use_count(), 1);

    // Insert into empty
    PersistentArray<int> empty_pa;
    PersistentArray<int> pa5 = empty_pa.insert(0, 100);
    ASSERT_EQ(pa5.size(), 1);
    EXPECT_EQ(pa5[0], 100);

    // Bounds checking
    EXPECT_THROW(pa1.insert(4, 99), std::out_of_range); // index > size
}

TEST_F(PersistentArrayTest, EraseOperation) { // REQ-3.4
    PersistentArray<int> pa1 = {10, 20, 30, 40, 50};

    // Erase from beginning
    PersistentArray<int> pa2 = pa1.erase(0);
    EXPECT_EQ(pa1.size(), 5); // Original
    ASSERT_EQ(pa2.size(), 4);
    EXPECT_EQ(pa2[0], 20); EXPECT_EQ(pa2[1], 30); EXPECT_EQ(pa2[2], 40); EXPECT_EQ(pa2[3], 50);
    EXPECT_EQ(pa1.use_count(), 1); EXPECT_EQ(pa2.use_count(), 1);

    // Erase from middle
    PersistentArray<int> pa3 = pa1.erase(2);
    ASSERT_EQ(pa3.size(), 4);
    EXPECT_EQ(pa3[0], 10); EXPECT_EQ(pa3[1], 20); EXPECT_EQ(pa3[2], 40); EXPECT_EQ(pa3[3], 50);
    EXPECT_EQ(pa3.use_count(), 1);

    // Erase from end
    PersistentArray<int> pa4 = pa1.erase(4);
    ASSERT_EQ(pa4.size(), 4);
    EXPECT_EQ(pa4[0], 10); EXPECT_EQ(pa4[1], 20); EXPECT_EQ(pa4[2], 30); EXPECT_EQ(pa4[3], 40);
    EXPECT_EQ(pa4.use_count(), 1);

    // Erase to empty
    PersistentArray<int> pa_single = {100};
    PersistentArray<int> pa5 = pa_single.erase(0);
    EXPECT_TRUE(pa5.empty());
    EXPECT_EQ(pa5.use_count(), 1);

    // Bounds checking
    EXPECT_THROW(pa1.erase(5), std::out_of_range); // index >= size
    PersistentArray<int> empty_pa;
    EXPECT_THROW(empty_pa.erase(0), std::out_of_range);
}


// REQ-3.7: Size query, REQ-3.8: Empty check
TEST_F(PersistentArrayTest, SizeAndEmpty) {
    PersistentArray<int> pa;
    EXPECT_EQ(pa.size(), 0);
    EXPECT_TRUE(pa.empty());

    PersistentArray<int> pa2 = {1, 2, 3};
    EXPECT_EQ(pa2.size(), 3);
    EXPECT_FALSE(pa2.empty());

    PersistentArray<int> pa3 = pa2.set(0, 10); // New version
    EXPECT_EQ(pa3.size(), 3);
    EXPECT_FALSE(pa3.empty());

    PersistentArray<int> pa4 = pa2.pop_back();
    EXPECT_EQ(pa4.size(), 2);
    EXPECT_FALSE(pa4.empty());

    PersistentArray<int> pa5 = pa4.pop_back().pop_back();
    EXPECT_EQ(pa5.size(), 0);
    EXPECT_TRUE(pa5.empty());
}


// REQ-10: Comparison Operations
TEST_F(PersistentArrayTest, ComparisonOperators) { // REQ-10.1, REQ-10.2, REQ-10.3
    PersistentArray<int> pa1 = {1, 2, 3};
    PersistentArray<int> pa2 = {1, 2, 3};
    PersistentArray<int> pa3 = {1, 2, 4};
    PersistentArray<int> pa4 = {1, 2};
    PersistentArray<int> empty1;
    PersistentArray<int> empty2;

    // Equality
    EXPECT_TRUE(pa1 == pa2); // Same content, different objects initially
    EXPECT_FALSE(pa1 == pa3); // Different content
    EXPECT_FALSE(pa1 == pa4); // Different size
    EXPECT_TRUE(empty1 == empty2); // Empty arrays
    EXPECT_FALSE(pa1 == empty1);

    // Inequality
    EXPECT_FALSE(pa1 != pa2);
    EXPECT_TRUE(pa1 != pa3);
    EXPECT_TRUE(pa1 != pa4);
    EXPECT_FALSE(empty1 != empty2);
    EXPECT_TRUE(pa1 != empty1);

    // Test after CoW
    PersistentArray<int> pa1_copy = pa1;
    EXPECT_TRUE(pa1 == pa1_copy); // Share data
    PersistentArray<int> pa1_modified = pa1.set(0, 100);
    EXPECT_FALSE(pa1 == pa1_modified);
    EXPECT_TRUE(pa1 != pa1_modified);
}


// REQ-6.1: Template-based implementation supporting any copyable type T
// REQ-6.2: Strong type safety with compile-time type checking
// REQ-6.3: Proper const-correctness for read operations
TEST_F(PersistentArrayTest, TypeSafetyAndGenerics) {
    // Test with std::string
    PersistentArray<std::string> str_pa1 = {"hello", "world"};
    EXPECT_EQ(str_pa1.size(), 2);
    EXPECT_EQ(str_pa1[0], "hello");

    PersistentArray<std::string> str_pa2 = str_pa1.set(1, "gtest");
    EXPECT_EQ(str_pa1[1], "world");
    EXPECT_EQ(str_pa2[1], "gtest");
    EXPECT_EQ(str_pa1.use_count(), 1); // Assuming no other copies of str_pa1
    EXPECT_EQ(str_pa2.use_count(), 1);

    // Test with a custom struct (must be copyable)
    struct MyStruct {
        int id;
        std::string name;
        bool operator==(const MyStruct& other) const {
            return id == other.id && name == other.name;
        }
        // Default constructor for vector initialization in PersistentArray(size_t)
        MyStruct(int i = 0, std::string n = "") : id(i), name(n) {}
    };

    PersistentArray<MyStruct> struct_pa1(2); // Default constructed MyStruct
    EXPECT_EQ(struct_pa1[0].id, 0);
    EXPECT_EQ(struct_pa1[0].name, "");

    struct_pa1 = struct_pa1.set(0, MyStruct{1, "one"});
    struct_pa1 = struct_pa1.set(1, MyStruct{2, "two"});

    EXPECT_EQ(struct_pa1[0].id, 1);
    EXPECT_EQ(struct_pa1[1].name, "two");

    PersistentArray<MyStruct> struct_pa2 = struct_pa1; // Share
    EXPECT_EQ(struct_pa1.use_count(), 2);
    EXPECT_EQ(struct_pa2.use_count(), 2);

    struct_pa2 = struct_pa2.set(0, MyStruct{3, "three"}); // CoW
    EXPECT_EQ(struct_pa1[0].id, 1);
    EXPECT_EQ(struct_pa2[0].id, 3);
    EXPECT_EQ(struct_pa1.use_count(), 1);
    EXPECT_EQ(struct_pa2.use_count(), 1);

    // Const-correctness (compile-time check mostly, but test const access)
    const PersistentArray<int> const_pa = {10, 20};
    EXPECT_EQ(const_pa[0], 10);
    EXPECT_EQ(const_pa.at(1), 20);
    EXPECT_TRUE(const_pa.size() == 2);
    // const_pa.set(0,5); // This should not compile if uncommented
}


// REQ-7: Iterator Support
TEST_F(PersistentArrayTest, IteratorSupport) { // REQ-7.1, REQ-7.2
    PersistentArray<int> pa = {1, 2, 3, 4, 5};
    const PersistentArray<int>& cpa = pa;

    // Range-based for loop (const iterator)
    std::vector<int> collected_values;
    for (const auto& val : pa) {
        collected_values.push_back(val);
    }
    ASSERT_EQ(collected_values.size(), 5);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(collected_values[i], pa[i]);
    }

    collected_values.clear();
    for (const auto& val : cpa) { // On const object
        collected_values.push_back(val);
    }
    ASSERT_EQ(collected_values.size(), 5);

    // begin() and end()
    collected_values.clear();
    for (auto it = pa.begin(); it != pa.end(); ++it) {
        collected_values.push_back(*it);
    }
    ASSERT_EQ(collected_values.size(), 5);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(collected_values[i], pa[i]);
    }

    // Iterators on empty array
    PersistentArray<int> empty_pa;
    EXPECT_EQ(empty_pa.begin(), empty_pa.end());
    int count = 0;
    for (const auto& val : empty_pa) { (void)val; count++; }
    EXPECT_EQ(count, 0);

    // REQ-7.3: Iterator invalidation (conceptual for now, testing behavior)
    PersistentArray<int> pa_v1 = {10, 20, 30};
    auto it_v1_begin = pa_v1.begin();
    auto it_v1_val20 = pa_v1.begin() + 1;
    EXPECT_EQ(*it_v1_begin, 10);
    EXPECT_EQ(*it_v1_val20, 20);

    PersistentArray<int> pa_v2 = pa_v1.set(1, 200); // CoW

    // Iterators for v1 should still be valid and point to original data
    EXPECT_EQ(*it_v1_begin, 10);
    EXPECT_EQ(*it_v1_val20, 20); // Unchanged for v1's perspective

    // Iterators for v2
    auto it_v2_begin = pa_v2.begin();
    auto it_v2_val200 = pa_v2.begin() + 1;
    EXPECT_EQ(*it_v2_begin, 10);
    EXPECT_EQ(*it_v2_val200, 200);
}


// REQ-4: In-Place Operations (Optional)
TEST_F(PersistentArrayTest, InPlaceModificationUniqueOwner) { // REQ-4.1
    PersistentArray<int> pa = {1, 2, 3};
    EXPECT_EQ(pa.use_count(), 1);

    pa.set_inplace(1, 200);
    EXPECT_EQ(pa.use_count(), 1); // Still unique
    ASSERT_EQ(pa.size(), 3);
    EXPECT_EQ(pa[0], 1);
    EXPECT_EQ(pa[1], 200);
    EXPECT_EQ(pa[2], 3);

    pa.push_back_inplace(400);
    EXPECT_EQ(pa.use_count(), 1);
    ASSERT_EQ(pa.size(), 4);
    EXPECT_EQ(pa[3], 400);

    pa.pop_back_inplace();
    EXPECT_EQ(pa.use_count(), 1);
    ASSERT_EQ(pa.size(), 3);
    EXPECT_EQ(pa[2], 3);

    // Bounds checking for set_inplace
    EXPECT_THROW(pa.set_inplace(3, 99), std::out_of_range);
    PersistentArray<int> empty_pa_inplace;
    EXPECT_THROW(empty_pa_inplace.pop_back_inplace(), std::runtime_error);
}

TEST_F(PersistentArrayTest, InPlaceModificationTriggersCoW) { // REQ-4.2
    PersistentArray<int> pa1 = {10, 20, 30};
    PersistentArray<int> pa2 = pa1; // pa1 and pa2 share data

    EXPECT_EQ(pa1.use_count(), 2);
    EXPECT_EQ(pa2.use_count(), 2);

    // Modify pa2 in-place. Should trigger CoW.
    // pa2 will get its own unique copy and modify it. pa1 remains untouched.
    pa2.set_inplace(1, 200);

    EXPECT_EQ(pa1.use_count(), 1); // pa1 is now unique
    EXPECT_EQ(pa2.use_count(), 1); // pa2 is now unique

    EXPECT_EQ(pa1[0], 10); EXPECT_EQ(pa1[1], 20); EXPECT_EQ(pa1[2], 30); // pa1 unchanged
    EXPECT_EQ(pa2[0], 10); EXPECT_EQ(pa2[1], 200); EXPECT_EQ(pa2[2], 30); // pa2 changed

    // Further test with push_back_inplace
    PersistentArray<int> pa3 = pa1; // pa1 and pa3 share again
    EXPECT_EQ(pa1.use_count(), 2);
    EXPECT_EQ(pa3.use_count(), 2);

    pa3.push_back_inplace(40);
    EXPECT_EQ(pa1.use_count(), 1); // pa1 unique
    EXPECT_EQ(pa3.use_count(), 1); // pa3 unique
    EXPECT_EQ(pa1.size(), 3);
    EXPECT_EQ(pa3.size(), 4);
    EXPECT_EQ(pa3[3], 40);

    // Further test with pop_back_inplace
    PersistentArray<int> pa4 = pa1; // pa1 and pa4 share
    EXPECT_EQ(pa1.use_count(), 2);
    EXPECT_EQ(pa4.use_count(), 2);

    pa4.pop_back_inplace();
    EXPECT_EQ(pa1.use_count(), 1);
    EXPECT_EQ(pa4.use_count(), 1);
    EXPECT_EQ(pa1.size(), 3);
    EXPECT_EQ(pa4.size(), 2);
    EXPECT_EQ(pa4[1], pa1[1]); // Content check
}

TEST_F(PersistentArrayTest, ClearOperation) { // REQ-4.3
    PersistentArray<int> pa1 = {1, 2, 3};
    PersistentArray<int> pa2 = pa1; // Share data
    EXPECT_EQ(pa1.use_count(), 2);

    pa1.clear(); // pa1 should become empty and unique
    EXPECT_TRUE(pa1.empty());
    EXPECT_EQ(pa1.size(), 0);
    EXPECT_EQ(pa1.use_count(), 1);

    // pa2 should be unaffected
    EXPECT_FALSE(pa2.empty());
    EXPECT_EQ(pa2.size(), 3);
    EXPECT_EQ(pa2[0], 1);
    EXPECT_EQ(pa2.use_count(), 1); // pa2 is now unique

    PersistentArray<int> pa3 = {5,6,7};
    pa3.clear();
    EXPECT_TRUE(pa3.empty());
    EXPECT_EQ(pa3.use_count(),1);
}

// REQ-8: Exception Safety (mainly bounds checking for now)
TEST_F(PersistentArrayTest, ExceptionSafetyBounds) { // REQ-8.2
    PersistentArray<int> pa = {10, 20};
    const PersistentArray<int>& cpa = pa;

    // operator[]
    EXPECT_THROW(pa[2], std::out_of_range);
    EXPECT_THROW(cpa[2], std::out_of_range);
    EXPECT_THROW(pa[-1], std::out_of_range); // Assuming size_t wraps or implementation handles

    // at()
    EXPECT_THROW(pa.at(2), std::out_of_range);
    EXPECT_THROW(cpa.at(2), std::out_of_range);

    // set()
    EXPECT_THROW(pa.set(2, 30), std::out_of_range);

    // insert() - index > size
    EXPECT_THROW(pa.insert(3, 30), std::out_of_range);

    // erase() - index >= size
    EXPECT_THROW(pa.erase(2), std::out_of_range);
    PersistentArray<int> empty_pa;
    EXPECT_THROW(empty_pa.erase(0), std::out_of_range);


    // pop_back() from empty
    EXPECT_THROW(empty_pa.pop_back(), std::runtime_error);

    // set_inplace()
    EXPECT_THROW(pa.set_inplace(2, 30), std::out_of_range);

    // pop_back_inplace() from empty
    EXPECT_THROW(empty_pa.pop_back_inplace(), std::runtime_error);
}

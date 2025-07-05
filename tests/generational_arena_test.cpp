#include "gtest/gtest.h"
#include "GenerationalArena.h" // Adjust path as necessary
#include <string>
#include <vector>
#include <algorithm> // For std::sort, std::find

// Simple struct for testing
struct TestObject {
    int id;
    std::string data;
    static int constructor_calls;
    static int destructor_calls;

    TestObject(int i = 0, std::string d = "") : id(i), data(std::move(d)) {
        constructor_calls++;
    }

    TestObject(const TestObject& other) : id(other.id), data(other.data) {
        constructor_calls++;
        // Note: This is a simplified copy constructor for tracking.
        // Real-world scenarios might need more complex logic.
    }
    TestObject& operator=(const TestObject& other) {
        id = other.id;
        data = other.data;
        // No change to constructor_calls here, this is assignment
        return *this;
    }


    ~TestObject() {
        destructor_calls++;
    }

    bool operator==(const TestObject& other) const {
        return id == other.id && data == other.data;
    }
     // Required for sorting in some tests
    bool operator<(const TestObject& other) const {
        if (id != other.id) {
            return id < other.id;
        }
        return data < other.data;
    }
};

int TestObject::constructor_calls = 0;
int TestObject::destructor_calls = 0;

// Helper to reset counters for each test
void ResetTestObjectCounters() {
    TestObject::constructor_calls = 0;
    TestObject::destructor_calls = 0;
}

// Test fixture for GenerationalArena tests
class GenerationalArenaTest : public ::testing::Test {
protected:
    cpp_collections::GenerationalArena<TestObject> arena;
    // cpp_collections::GenerationalArena<int> int_arena; // Example for int if needed

    void SetUp() override {
        ResetTestObjectCounters();
    }

    void TearDown() override {
        // Arena destructor will call clear, which should call destructors
        // Verify all constructed objects were destructed if arena goes out of scope
    }
};

TEST_F(GenerationalArenaTest, InitialState) {
    EXPECT_EQ(arena.size(), 0);
    EXPECT_TRUE(arena.empty());
    EXPECT_GE(arena.capacity(), 0); // Capacity can be 0 or more initially
    ResetTestObjectCounters(); // Reset after arena construction if it allocates
}

TEST_F(GenerationalArenaTest, AllocateSingleObject) {
    cpp_collections::ArenaHandle handle = arena.allocate(1, "obj1");
    EXPECT_EQ(arena.size(), 1);
    EXPECT_FALSE(arena.empty());
    EXPECT_TRUE(arena.is_valid(handle));
    ASSERT_NE(arena.get(handle), nullptr);
    EXPECT_EQ(arena.get(handle)->id, 1);
    EXPECT_EQ(arena.get(handle)->data, "obj1");
    EXPECT_EQ(TestObject::constructor_calls, 1);
}

TEST_F(GenerationalArenaTest, AllocateMultipleObjects) {
    cpp_collections::ArenaHandle h1 = arena.allocate(1, "obj1");
    cpp_collections::ArenaHandle h2 = arena.allocate(2, "obj2");
    cpp_collections::ArenaHandle h3 = arena.allocate(3, "obj3");

    EXPECT_EQ(arena.size(), 3);
    EXPECT_EQ(TestObject::constructor_calls, 3);

    ASSERT_NE(arena.get(h1), nullptr);
    EXPECT_EQ(arena.get(h1)->id, 1);
    ASSERT_NE(arena.get(h2), nullptr);
    EXPECT_EQ(arena.get(h2)->id, 2);
    ASSERT_NE(arena.get(h3), nullptr);
    EXPECT_EQ(arena.get(h3)->id, 3);
}

TEST_F(GenerationalArenaTest, DeallocateObject) {
    cpp_collections::ArenaHandle h1 = arena.allocate(1, "obj1");
    int initial_constructors = TestObject::constructor_calls;
    int initial_destructors = TestObject::destructor_calls;

    arena.deallocate(h1);
    EXPECT_EQ(arena.size(), 0);
    EXPECT_FALSE(arena.is_valid(h1));
    EXPECT_EQ(arena.get(h1), nullptr);
    EXPECT_EQ(TestObject::constructor_calls, initial_constructors); // No new constructions
    EXPECT_EQ(TestObject::destructor_calls, initial_destructors + 1); // One destruction
}

TEST_F(GenerationalArenaTest, DeallocateInvalidHandle) {
    cpp_collections::ArenaHandle h_invalid = cpp_collections::ArenaHandle::null(); // Default invalid
    cpp_collections::ArenaHandle h_valid = arena.allocate(1, "test");
    arena.deallocate(h_valid); // h_valid is now invalid (generation changed)

    size_t current_size = arena.size();
    int current_destructors = TestObject::destructor_calls;

    arena.deallocate(h_invalid); // Try deallocating null handle
    arena.deallocate(h_valid);   // Try deallocating already deallocated handle
    cpp_collections::ArenaHandle h_out_of_bounds(1000, 0); // Index likely out of bounds
    arena.deallocate(h_out_of_bounds);

    EXPECT_EQ(arena.size(), current_size); // Size should not change
    EXPECT_EQ(TestObject::destructor_calls, current_destructors); // No additional destructions
}


TEST_F(GenerationalArenaTest, ReuseSlotAndGeneration) {
    cpp_collections::ArenaHandle h1 = arena.allocate(1, "obj1");
    uint32_t h1_index = h1.index;
    uint32_t h1_gen = h1.generation;

    arena.deallocate(h1);
    EXPECT_FALSE(arena.is_valid(h1)); // h1 is invalid

    // Allocate another object, likely reusing the slot
    cpp_collections::ArenaHandle h2 = arena.allocate(2, "obj2");
    EXPECT_EQ(arena.size(), 1);
    EXPECT_TRUE(arena.is_valid(h2));
    ASSERT_NE(arena.get(h2), nullptr);
    EXPECT_EQ(arena.get(h2)->id, 2);

    // Check if slot was reused and generation incremented
    if (h2.index == h1_index) {
        EXPECT_NE(h2.generation, h1_gen); // Generation must be different
        EXPECT_GT(h2.generation, h1_gen); // Or wrapped around logic
    }
    EXPECT_FALSE(arena.is_valid(h1)); // Original handle h1 must remain invalid
}

TEST_F(GenerationalArenaTest, GetObject) {
    cpp_collections::ArenaHandle h1 = arena.allocate(1, "obj1");
    TestObject* obj_ptr = arena.get(h1);
    ASSERT_NE(obj_ptr, nullptr);
    EXPECT_EQ(obj_ptr->id, 1);
    EXPECT_EQ(obj_ptr->data, "obj1");

    const TestObject* const_obj_ptr = static_cast<const decltype(arena)*>(&arena)->get(h1);
    ASSERT_NE(const_obj_ptr, nullptr);
    EXPECT_EQ(const_obj_ptr->id, 1);

    // Modify through non-const pointer
    obj_ptr->data = "modified";
    EXPECT_EQ(arena.get(h1)->data, "modified");
}

TEST_F(GenerationalArenaTest, GetInvalidObject) {
    cpp_collections::ArenaHandle h_null = cpp_collections::ArenaHandle::null();
    EXPECT_EQ(arena.get(h_null), nullptr);

    cpp_collections::ArenaHandle h1 = arena.allocate(1, "obj1");
    arena.deallocate(h1);
    EXPECT_EQ(arena.get(h1), nullptr); // Access after deallocate

    cpp_collections::ArenaHandle h_wrong_gen(h1.index, h1.generation + 5); // Valid index, wrong gen
    EXPECT_EQ(arena.get(h_wrong_gen), nullptr);

    cpp_collections::ArenaHandle h_wrong_idx(999, 0); // Invalid index
    EXPECT_EQ(arena.get(h_wrong_idx), nullptr);
}

TEST_F(GenerationalArenaTest, ClearArena) {
    arena.allocate(1, "obj1");
    arena.allocate(2, "obj2");
    EXPECT_EQ(arena.size(), 2);
    EXPECT_EQ(TestObject::constructor_calls, 2);

    arena.clear();
    EXPECT_EQ(arena.size(), 0);
    EXPECT_TRUE(arena.empty());
    // Capacity might or might not be reset, depends on implementation (std::vector::clear doesn't change capacity)
    // EXPECT_EQ(arena.capacity(), 0); // This might be too strict
    EXPECT_EQ(TestObject::destructor_calls, 2); // Both objects should be destructed
}

TEST_F(GenerationalArenaTest, ReserveCapacity) {
    EXPECT_EQ(arena.capacity(), 0); // Assuming default starts at 0 or small
    arena.reserve(100);
    EXPECT_GE(arena.capacity(), 100);

    cpp_collections::ArenaHandle h1 = arena.allocate(1, "obj1"); // Should not reallocate if capacity is enough
    EXPECT_EQ(arena.size(), 1);
    EXPECT_GE(arena.capacity(), 100);
}


TEST_F(GenerationalArenaTest, IterationEmpty) {
    int count = 0;
    for (const auto& obj : arena) {
        count++;
        (void)obj; // Use obj to prevent unused variable warning
    }
    EXPECT_EQ(count, 0);

    for (auto& obj : arena) { // Non-const
        count++;
        (void)obj;
    }
    EXPECT_EQ(count, 0);
}

TEST_F(GenerationalArenaTest, IterationSingle) {
    arena.allocate(10, "single");
    int count = 0;
    for (const auto& obj : arena) {
        EXPECT_EQ(obj.id, 10);
        EXPECT_EQ(obj.data, "single");
        count++;
    }
    EXPECT_EQ(count, 1);
}

TEST_F(GenerationalArenaTest, IterationMultiple) {
    std::vector<TestObject> expected_objects;
    expected_objects.push_back({1, "one"});
    expected_objects.push_back({2, "two"});
    expected_objects.push_back({3, "three"});

    arena.allocate(1, "one");
    arena.allocate(2, "two");
    arena.allocate(3, "three");

    std::vector<TestObject> iterated_objects;
    for (const auto& obj : arena) {
        iterated_objects.push_back(obj);
    }

    // Iteration order is not guaranteed to be insertion order by GenerationalArena
    // So, sort both before comparing if order is not important for the test.
    // However, current implementation likely iterates by slot index.
    // For this test, let's assume it's slot order.
    // If it's dense (no deallocations), it should be insertion order.
    std::sort(iterated_objects.begin(), iterated_objects.end());
    std::sort(expected_objects.begin(), expected_objects.end());

    EXPECT_EQ(iterated_objects.size(), 3);
    EXPECT_EQ(iterated_objects, expected_objects);
}

TEST_F(GenerationalArenaTest, IterationWithDeallocations) {
    cpp_collections::ArenaHandle h1 = arena.allocate(1, "one");
    cpp_collections::ArenaHandle h2 = arena.allocate(2, "two"); // Will be deallocated
    cpp_collections::ArenaHandle h3 = arena.allocate(3, "three");

    arena.deallocate(h2);
    arena.allocate(4, "four"); // Might reuse h2's slot

    std::vector<TestObject> expected_objects;
    expected_objects.push_back({1, "one"});
    expected_objects.push_back({3, "three"});
    expected_objects.push_back({4, "four"});
    std::sort(expected_objects.begin(), expected_objects.end());


    std::vector<TestObject> iterated_objects;
    for (const auto& obj : arena) {
        iterated_objects.push_back(obj);
    }
    std::sort(iterated_objects.begin(), iterated_objects.end());

    EXPECT_EQ(iterated_objects.size(), 3);
    EXPECT_EQ(iterated_objects, expected_objects);
}

TEST_F(GenerationalArenaTest, NonConstIterationAndModification) {
    cpp_collections::ArenaHandle h1 = arena.allocate(1, "one");
    cpp_collections::ArenaHandle h2 = arena.allocate(2, "two");

    for (auto& obj : arena) {
        obj.id += 100;
        obj.data += "_mod";
    }

    ASSERT_TRUE(arena.is_valid(h1));
    EXPECT_EQ(arena.get(h1)->id, 101);
    EXPECT_EQ(arena.get(h1)->data, "one_mod");

    ASSERT_TRUE(arena.is_valid(h2));
    EXPECT_EQ(arena.get(h2)->id, 102);
    EXPECT_EQ(arena.get(h2)->data, "two_mod");
}


TEST_F(GenerationalArenaTest, DestructorCallsOnArenaDestruction) {
    {
        cpp_collections::GenerationalArena<TestObject> local_arena;
        local_arena.allocate(1, "obj1_local");
        local_arena.allocate(2, "obj2_local");
        EXPECT_EQ(TestObject::constructor_calls, 2);
        EXPECT_EQ(TestObject::destructor_calls, 0);
    } // local_arena goes out of scope here
    EXPECT_EQ(TestObject::destructor_calls, 2); // Destructors should have been called
}

TEST_F(GenerationalArenaTest, MoveConstruction) {
    arena.allocate(1, "one");
    arena.allocate(2, "two");
    EXPECT_EQ(TestObject::constructor_calls, 2);
    int destructors_before_move = TestObject::destructor_calls;

    cpp_collections::GenerationalArena<TestObject> new_arena(std::move(arena));

    EXPECT_EQ(arena.size(), 0); // Original arena should be empty
    EXPECT_TRUE(arena.empty());

    EXPECT_EQ(new_arena.size(), 2);
    EXPECT_FALSE(new_arena.empty());
    EXPECT_EQ(TestObject::constructor_calls, 2); // No new constructions
    EXPECT_EQ(TestObject::destructor_calls, destructors_before_move); // No destructions during move itself

    // Verify content in new_arena
    int count = 0;
    std::vector<int> ids;
    for(const auto& item : new_arena) {
        ids.push_back(item.id);
        count++;
    }
    std::sort(ids.begin(), ids.end());
    EXPECT_EQ(count, 2);
    EXPECT_EQ(ids[0], 1);
    EXPECT_EQ(ids[1], 2);

    // Let new_arena go out of scope, its destructor should clean up
    ResetTestObjectCounters(); // Reset for clarity
    {
         cpp_collections::GenerationalArena<TestObject> temp_arena = std::move(new_arena);
         EXPECT_EQ(temp_arena.size(), 2);
    } // temp_arena destructs
    EXPECT_EQ(TestObject::destructor_calls, 2);
}

TEST_F(GenerationalArenaTest, MoveAssignment) {
    arena.allocate(1, "one");
    cpp_collections::ArenaHandle h_arena_1 = arena.allocate(10, "ten");

    cpp_collections::GenerationalArena<TestObject> new_arena;
    new_arena.allocate(2, "two");
    new_arena.allocate(3, "three");
    cpp_collections::ArenaHandle h_new_arena_1 = new_arena.allocate(20, "twenty");

    ResetTestObjectCounters(); // Reset before the operation we're testing

    // new_arena's objects (2,3,20) should be destructed
    // arena's objects (1,10) will be moved
    new_arena = std::move(arena);

    EXPECT_EQ(TestObject::destructor_calls, 3); // Objects from old new_arena (2,3,20)
    EXPECT_EQ(TestObject::constructor_calls, 0); // No new constructions

    EXPECT_EQ(arena.size(), 0);
    EXPECT_TRUE(arena.empty());

    EXPECT_EQ(new_arena.size(), 2);
    EXPECT_FALSE(new_arena.empty());

    // Check content of new_arena (should be from original arena)
    // Note: handles from original arena are not valid for new_arena.
    // We need to iterate or get new handles if we had stored them.
    bool found1 = false, found10 = false;
    for(const auto& item : new_arena) {
        if(item.id == 1 && item.data == "one") found1 = true;
        if(item.id == 10 && item.data == "ten") found10 = true;
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found10);
}


TEST_F(GenerationalArenaTest, HandleNullStatic) {
    cpp_collections::ArenaHandle h_null = cpp_collections::ArenaHandle::null();
    EXPECT_TRUE(h_null.is_null());
    EXPECT_FALSE(arena.is_valid(h_null));
}

TEST_F(GenerationalArenaTest, GenerationOverflow) {
    // This test is tricky to make deterministic and fast without exposing internals
    // or making generation a smaller type. The idea is to deallocate/allocate
    // the same slot many times. uint32_t is large.
    // For now, we'll assume the wrap-around logic (if any) is correct
    // and primarily test that generations do change.
    cpp_collections::ArenaHandle h = arena.allocate(1, "gen_test");
    uint32_t initial_gen = h.generation;
    uint32_t slot_idx = h.index;

    arena.deallocate(h);
    cpp_collections::ArenaHandle h_next = arena.allocate(2, "gen_test2");

    EXPECT_EQ(h_next.index, slot_idx); // Assuming reuse
    if (initial_gen == std::numeric_limits<uint32_t>::max()) {
         EXPECT_EQ(h_next.generation, 0); // Wrapped
    } else {
         EXPECT_EQ(h_next.generation, initial_gen + 1);
    }

    // Test the specific wrap-around case if we can force a generation to max
    // This would require a specialized test or a way to set generation.
    // For now, this basic check is sufficient.
}

// Test with a type that is not trivially destructible (uses TestObject)
TEST_F(GenerationalArenaTest, NonTriviallyDestructible) {
    ResetTestObjectCounters();
    {
        cpp_collections::GenerationalArena<TestObject> local_arena;
        cpp_collections::ArenaHandle h1 = local_arena.allocate(1, "obj1");
        cpp_collections::ArenaHandle h2 = local_arena.allocate(2, "obj2");
        EXPECT_EQ(TestObject::constructor_calls, 2);
        local_arena.deallocate(h1);
        EXPECT_EQ(TestObject::destructor_calls, 1);
    } // local_arena goes out of scope, h2 should be destructed
    EXPECT_EQ(TestObject::destructor_calls, 2);
}

TEST_F(GenerationalArenaTest, IntArena) {
    cpp_collections::GenerationalArena<int> int_arena;
    cpp_collections::ArenaHandle h1 = int_arena.allocate(100);
    ASSERT_NE(int_arena.get(h1), nullptr);
    EXPECT_EQ(*int_arena.get(h1), 100);

    *int_arena.get(h1) = 200;
    EXPECT_EQ(*int_arena.get(h1), 200);

    int_arena.deallocate(h1);
    EXPECT_EQ(int_arena.get(h1), nullptr);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

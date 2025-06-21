#include "gtest/gtest.h"
#include "delayed_init.h"
#include <string>
#include <vector> // For testing move semantics with vector
#include <stdexcept> // For DelayedInitError

// Helper struct for testing construction/destruction/copy/move
struct TestResource {
    int id;
    std::string data;
    static int construction_count;
    static int destruction_count;
    static int copy_construction_count;
    static int move_construction_count;
    static int copy_assignment_count;
    static int move_assignment_count;

    TestResource(int i = 0, std::string d = "default") : id(i), data(std::move(d)) {
        construction_count++;
    }

    TestResource(const TestResource& other) : id(other.id), data(other.data) {
        copy_construction_count++;
        construction_count++; // Also a construction
    }

    TestResource(TestResource&& other) noexcept : id(other.id), data(std::move(other.data)) {
        move_construction_count++;
        construction_count++; // Also a construction
        other.id = -1; // Mark as moved from
        other.data = "moved_from";
    }

    TestResource& operator=(const TestResource& other) {
        if (this != &other) {
            id = other.id;
            data = other.data;
            copy_assignment_count++;
        }
        return *this;
    }

    TestResource& operator=(TestResource&& other) noexcept {
        if (this != &other) {
            id = other.id;
            data = std::move(other.data);
            move_assignment_count++;
            other.id = -1;
            other.data = "moved_from_assign";
        }
        return *this;
    }

    ~TestResource() {
        destruction_count++;
    }

    bool operator==(const TestResource& other) const {
        return id == other.id && data == other.data;
    }
    bool operator<(const TestResource& other) const {
        if (id != other.id) return id < other.id;
        return data < other.data;
    }

    static void reset_counts() {
        construction_count = 0;
        destruction_count = 0;
        copy_construction_count = 0;
        move_construction_count = 0;
        copy_assignment_count = 0;
        move_assignment_count = 0;
    }
};

int TestResource::construction_count = 0;
int TestResource::destruction_count = 0;
int TestResource::copy_construction_count = 0;
int TestResource::move_construction_count = 0;
int TestResource::copy_assignment_count = 0;
int TestResource::move_assignment_count = 0;


// Test fixture for DelayedInit tests
class DelayedInitTest : public ::testing::Test {
protected:
    void SetUp() override {
        TestResource::reset_counts();
    }
};

TEST_F(DelayedInitTest, DefaultConstruction) {
    DelayedInit<int> di_int;
    ASSERT_FALSE(di_int.is_initialized());
    ASSERT_FALSE(static_cast<bool>(di_int));

    DelayedInit<TestResource> di_tr;
    ASSERT_FALSE(di_tr.is_initialized());
    ASSERT_EQ(TestResource::construction_count, 0);
    ASSERT_EQ(TestResource::destruction_count, 0);
}

TEST_F(DelayedInitTest, InitOnceOnlyPolicy) {
    DelayedInitOnce<int> di;
    di.init(10);
    ASSERT_TRUE(di.is_initialized());
    ASSERT_EQ(*di, 10);
    ASSERT_THROW(di.init(20), DelayedInitError); // Cannot re-init

    DelayedInitOnce<TestResource> di_tr;
    di_tr.init(TestResource(1, "once"));
    ASSERT_TRUE(di_tr.is_initialized());
    ASSERT_EQ(di_tr->id, 1);
    ASSERT_EQ(di_tr->data, "once");
    // Exact counts depend on RVO/NRVO, focus on behavior
    ASSERT_GE(TestResource::construction_count, 1);
    ASSERT_THROW(di_tr.init(TestResource(2, "again")), DelayedInitError);
}

TEST_F(DelayedInitTest, EmplaceOnceOnlyPolicy) {
    DelayedInitOnce<TestResource> di_tr;
    di_tr.emplace(2, "emplace_once");
    ASSERT_TRUE(di_tr.is_initialized());
    ASSERT_EQ(di_tr->id, 2);
    ASSERT_EQ(di_tr->data, "emplace_once");
    ASSERT_EQ(TestResource::construction_count, 1); // Direct emplacement
    ASSERT_THROW(di_tr.emplace(3, "emplace_again"), DelayedInitError);
}

TEST_F(DelayedInitTest, InitMutablePolicy) {
    DelayedInitMutable<int> di;
    di.init(10);
    ASSERT_TRUE(di.is_initialized());
    ASSERT_EQ(*di, 10);

    di.init(20); // Can re-init
    ASSERT_TRUE(di.is_initialized());
    ASSERT_EQ(*di, 20);

    DelayedInitMutable<TestResource> di_tr;
    TestResource::reset_counts();
    di_tr.init(TestResource(1, "mutable1"));
    ASSERT_EQ(TestResource::construction_count, 2); // 1 for temp, 1 for internal storage (copy/move)
    ASSERT_EQ(TestResource::destruction_count, 1); // 1 for temp
    ASSERT_TRUE(di_tr.is_initialized());
    ASSERT_EQ(di_tr->id, 1);

    TestResource::reset_counts();
    di_tr.init(TestResource(2, "mutable2")); // Re-initializes
    ASSERT_TRUE(di_tr.is_initialized());
    ASSERT_EQ(di_tr->id, 2);
    ASSERT_EQ(di_tr->data, "mutable2");
    ASSERT_EQ(TestResource::construction_count, 2); // Temp + internal
    ASSERT_EQ(TestResource::destruction_count, 2); // Old internal + temp
}

TEST_F(DelayedInitTest, EmplaceMutablePolicy) {
    DelayedInitMutable<TestResource> di_tr;
    TestResource::reset_counts();
    di_tr.emplace(1, "emplace_mutable1");
    ASSERT_TRUE(di_tr.is_initialized());
    ASSERT_EQ(di_tr->id, 1);
    ASSERT_EQ(TestResource::construction_count, 1); // Direct emplacement
    ASSERT_EQ(TestResource::destruction_count, 0);

    TestResource::reset_counts();
    di_tr.emplace(2, "emplace_mutable2"); // Re-emplaces
    ASSERT_TRUE(di_tr.is_initialized());
    ASSERT_EQ(di_tr->id, 2);
    ASSERT_EQ(TestResource::construction_count, 1); // New direct emplacement
    ASSERT_EQ(TestResource::destruction_count, 1); // Old internal
}


TEST_F(DelayedInitTest, InitNullablePolicy) {
    DelayedInitNullable<int> di;
    di.init(10);
    ASSERT_TRUE(di.is_initialized());
    ASSERT_EQ(*di, 10);

    di.init(20); // Can re-init
    ASSERT_TRUE(di.is_initialized());
    ASSERT_EQ(*di, 20);
}

TEST_F(DelayedInitTest, AccessUninitialized) {
    DelayedInit<int> di_int;
    ASSERT_THROW(di_int.get(), DelayedInitError);
    ASSERT_THROW(*di_int, DelayedInitError);
    // ASSERT_THROW(di_int->operator int(), DelayedInitError); // This was incorrect for int*, ->id below tests operator->

    DelayedInit<TestResource> di_tr;
    ASSERT_THROW(di_tr.get(), DelayedInitError);
    ASSERT_THROW(*di_tr, DelayedInitError);
    ASSERT_THROW(di_tr->id, DelayedInitError);
}

TEST_F(DelayedInitTest, GetAndOperators) {
    DelayedInit<std::string> di;
    di.init("hello");
    ASSERT_EQ(di.get(), "hello");
    ASSERT_EQ(*di, "hello");
    *di = "world";
    ASSERT_EQ(*di, "world");
    ASSERT_EQ(di->length(), 5);
}

TEST_F(DelayedInitTest, CopyConstruction) {
    DelayedInitOnce<TestResource> original;
    original.emplace(1, "original");
    ASSERT_EQ(TestResource::construction_count, 1);

    TestResource::reset_counts();
    DelayedInitOnce<TestResource> copy = original; // Copy constructor
    ASSERT_TRUE(original.is_initialized());
    ASSERT_TRUE(copy.is_initialized());
    ASSERT_EQ(copy->id, 1);
    ASSERT_EQ(copy->data, "original");
    ASSERT_EQ(TestResource::copy_construction_count, 1);
    ASSERT_EQ(TestResource::construction_count, 1);
    ASSERT_NE(&original.get(), &copy.get());

    DelayedInitOnce<TestResource> uninit_original;
    TestResource::reset_counts();
    DelayedInitOnce<TestResource> copy_uninit = uninit_original;
    ASSERT_FALSE(uninit_original.is_initialized());
    ASSERT_FALSE(copy_uninit.is_initialized());
    ASSERT_EQ(TestResource::construction_count, 0);
}

TEST_F(DelayedInitTest, MoveConstruction) {
    DelayedInitOnce<TestResource> original;
    original.emplace(1, "original_move");
    ASSERT_EQ(TestResource::construction_count, 1);

    TestResource::reset_counts();
    DelayedInitOnce<TestResource> moved_to = std::move(original); // Move constructor

    ASSERT_TRUE(moved_to.is_initialized());
    ASSERT_EQ(moved_to->id, 1);
    ASSERT_EQ(moved_to->data, "original_move");

    // Original state after move for OnceOnly/Mutable:
    ASSERT_FALSE(original.is_initialized()); // Should be destroyed and uninitialized

    ASSERT_EQ(TestResource::move_construction_count, 1); // Internal object in moved_to
    ASSERT_EQ(TestResource::construction_count, 1);
    ASSERT_EQ(TestResource::destruction_count, 1); // Internal object in original

    // Test move of uninitialized
    DelayedInitOnce<TestResource> uninit_original;
    TestResource::reset_counts();
    DelayedInitOnce<TestResource> moved_uninit = std::move(uninit_original);
    ASSERT_FALSE(uninit_original.is_initialized());
    ASSERT_FALSE(moved_uninit.is_initialized());
    ASSERT_EQ(TestResource::construction_count, 0);
}

TEST_F(DelayedInitTest, MoveConstructionNullable) {
    DelayedInitNullable<TestResource> original;
    original.emplace(1, "original_move_nullable");
    TestResource::reset_counts();

    DelayedInitNullable<TestResource> moved_to = std::move(original);
    ASSERT_TRUE(moved_to.is_initialized());
    ASSERT_EQ(moved_to->id, 1);
    ASSERT_EQ(TestResource::move_construction_count, 1);

    // For Nullable, the source of a move might or might not be cleared.
    // The current implementation clears it for consistency with other policies if not Nullable,
    // but for Nullable, it could also leave the source intact if T is only copyable.
    // Let's check current behavior: original is initialized but its state is moved-from.
    ASSERT_TRUE(original.is_initialized());
    ASSERT_EQ(original->id, -1); // Moved-from state
}


TEST_F(DelayedInitTest, CopyAssignment) {
    DelayedInitOnce<TestResource> original_once;
    original_once.emplace(1, "assign_original_once");
    DelayedInitOnce<TestResource> target_once; // Uninitialized target

    TestResource::reset_counts();
    target_once = original_once;
    ASSERT_TRUE(original_once.is_initialized());
    ASSERT_TRUE(target_once.is_initialized());
    ASSERT_EQ(target_once->id, 1);
    ASSERT_EQ(TestResource::copy_construction_count, 1); // Copy into target's storage

    // Test with Mutable policy
    DelayedInitMutable<TestResource> original_mut;
    original_mut.emplace(1, "assign_original_mut");
    DelayedInitMutable<TestResource> target_mut_init;
    target_mut_init.emplace(2, "assign_target_mut_init");

    TestResource::reset_counts();
    target_mut_init = original_mut; // Assign to initialized mutable
    ASSERT_TRUE(target_mut_init.is_initialized());
    ASSERT_EQ(target_mut_init->id, 1);
    ASSERT_EQ(target_mut_init->data, "assign_original_mut");
    ASSERT_EQ(TestResource::destruction_count, 1); // Destruction of old value in target_mut_init
    ASSERT_EQ(TestResource::copy_construction_count, 1); // Copy into target's storage

    // Test assigning uninitialized to Nullable
    DelayedInitNullable<TestResource> uninit_source_nullable; // Correct type
    DelayedInitNullable<TestResource> target_nullable_init;
    target_nullable_init.emplace(3, "nullable_target");

    TestResource::reset_counts();
    target_nullable_init = uninit_source_nullable; // Assign uninitialized Nullable to Nullable
    ASSERT_FALSE(target_nullable_init.is_initialized());
    ASSERT_EQ(TestResource::destruction_count, 1); // Destruction of old value in target_nullable_init
}

TEST_F(DelayedInitTest, MoveAssignment) {
    DelayedInitOnce<TestResource> source;
    source.emplace(1, "move_assign_source");
    DelayedInitOnce<TestResource> target; // Uninitialized target

    TestResource::reset_counts();
    target = std::move(source);
    ASSERT_TRUE(target.is_initialized());
    ASSERT_EQ(target->id, 1);
    ASSERT_FALSE(source.is_initialized()); // Source is cleared (for OnceOnly/Mutable)
    ASSERT_EQ(TestResource::move_construction_count, 1); // Move into target's storage
    ASSERT_EQ(TestResource::destruction_count, 1); // Destruction of source's moved-from object

    DelayedInitMutable<TestResource> source_mut;
    source_mut.emplace(2, "move_assign_source_mut");
    DelayedInitMutable<TestResource> target_mut_init;
    target_mut_init.emplace(3, "move_assign_target_mut_init");

    TestResource::reset_counts();
    target_mut_init = std::move(source_mut);
    ASSERT_TRUE(target_mut_init.is_initialized());
    ASSERT_EQ(target_mut_init->id, 2);
    ASSERT_FALSE(source_mut.is_initialized());
    ASSERT_EQ(TestResource::destruction_count, 2); // Old in target + source's moved-from object
    ASSERT_EQ(TestResource::move_construction_count, 1); // Move into target

    DelayedInitNullable<TestResource> uninit_source;
    DelayedInitNullable<TestResource> target_nullable_init;
    target_nullable_init.emplace(4, "move_assign_target_nullable");
    TestResource::reset_counts();
    target_nullable_init = std::move(uninit_source); // Assign uninitialized
    ASSERT_FALSE(target_nullable_init.is_initialized());
    ASSERT_EQ(TestResource::destruction_count, 1); // Destruction of old value
}


TEST_F(DelayedInitTest, Reset) {
    DelayedInitMutable<TestResource> di_mut;
    di_mut.emplace(1, "mutable_reset");
    ASSERT_TRUE(di_mut.is_initialized());
    ASSERT_EQ(TestResource::construction_count, 1);
    ASSERT_EQ(TestResource::destruction_count, 0);

    TestResource::reset_counts();
    di_mut.reset();
    ASSERT_FALSE(di_mut.is_initialized());
    ASSERT_EQ(TestResource::destruction_count, 1);

    DelayedInitNullable<TestResource> di_null;
    di_null.emplace(2, "nullable_reset");
    ASSERT_TRUE(di_null.is_initialized());
    ASSERT_EQ(TestResource::construction_count, 1); // From the emplace above

    TestResource::reset_counts(); // Reset for this specific reset operation
    di_null.reset();
    ASSERT_FALSE(di_null.is_initialized());
    ASSERT_EQ(TestResource::destruction_count, 1);

    // Reset on uninitialized should be fine
    TestResource::reset_counts();
    DelayedInitMutable<TestResource> di_mut_uninit;
    di_mut_uninit.reset();
    ASSERT_FALSE(di_mut_uninit.is_initialized());
    ASSERT_EQ(TestResource::destruction_count, 0);
}

TEST_F(DelayedInitTest, ValueOr) {
    DelayedInitNullable<int> di;
    ASSERT_EQ(di.value_or(100), 100);
    di.init(50);
    ASSERT_EQ(di.value_or(100), 50);
    di.reset();
    ASSERT_EQ(di.value_or(200), 200);

    DelayedInitNullable<std::string> di_str;
    ASSERT_EQ(di_str.value_or("default"), "default");
    di_str.init("custom");
    ASSERT_EQ(di_str.value_or("default"), "custom");

    // Test with rvalue default
    ASSERT_EQ(di_str.value_or(std::string("rval_default")), "custom");
    di_str.reset();
    ASSERT_EQ(di_str.value_or(std::string("rval_default_after_reset")), "rval_default_after_reset");
}

TEST_F(DelayedInitTest, ComparisonOperators) {
    DelayedInit<int> i1, i2, i3, i_uninit1, i_uninit2;
    i1.init(10);
    i2.init(20);
    i3.init(10);

    // Equality
    ASSERT_TRUE(i1 == i3);
    ASSERT_FALSE(i1 == i2);
    ASSERT_TRUE(i_uninit1 == i_uninit2);
    ASSERT_FALSE(i1 == i_uninit1);
    ASSERT_FALSE(i_uninit1 == i1);

    // Inequality
    ASSERT_FALSE(i1 != i3);
    ASSERT_TRUE(i1 != i2);
    ASSERT_FALSE(i_uninit1 != i_uninit2);
    ASSERT_TRUE(i1 != i_uninit1);

    // Less than
    ASSERT_TRUE(i1 < i2);
    ASSERT_FALSE(i2 < i1);
    ASSERT_FALSE(i1 < i3); // Equal, not less
    ASSERT_TRUE(i_uninit1 < i1); // Uninit < Init
    ASSERT_FALSE(i1 < i_uninit1); // Init not < Uninit
    ASSERT_FALSE(i_uninit1 < i_uninit2); // Uninit not < Uninit

    // Less than or equal
    ASSERT_TRUE(i1 <= i2);
    ASSERT_TRUE(i1 <= i3);
    ASSERT_FALSE(i2 <= i1);
    ASSERT_TRUE(i_uninit1 <= i1);
    ASSERT_TRUE(i_uninit1 <= i_uninit2);

    // Greater than
    ASSERT_TRUE(i2 > i1);
    ASSERT_FALSE(i1 > i2);
    ASSERT_FALSE(i1 > i3);
    ASSERT_TRUE(i1 > i_uninit1);
    ASSERT_FALSE(i_uninit1 > i1);
    ASSERT_FALSE(i_uninit1 > i_uninit2);

    // Greater than or equal
    ASSERT_TRUE(i2 >= i1);
    ASSERT_TRUE(i1 >= i3);
    ASSERT_FALSE(i1 >= i2);
    ASSERT_TRUE(i1 >= i_uninit1);
    ASSERT_TRUE(i_uninit1 >= i_uninit2);

    // Test with custom type
    DelayedInit<TestResource> tr1, tr2, tr3, tr_uninit;
    tr1.emplace(1, "apple");
    tr2.emplace(2, "banana");
    tr3.emplace(1, "apple");

    ASSERT_TRUE(tr1 == tr3);
    ASSERT_TRUE(tr1 < tr2);
    ASSERT_TRUE(tr_uninit < tr1);
}

TEST_F(DelayedInitTest, SwapFunctionality) {
    // Case 1: Both initialized
    DelayedInit<TestResource> s1, s2;
    s1.emplace(1, "alpha");
    s2.emplace(2, "beta");
    TestResource::reset_counts();

    swap(s1, s2); // Non-member swap

    ASSERT_TRUE(s1.is_initialized());
    ASSERT_EQ(s1->id, 2);
    ASSERT_EQ(s1->data, "beta");
    ASSERT_TRUE(s2.is_initialized());
    ASSERT_EQ(s2->id, 1);
    ASSERT_EQ(s2->data, "alpha");
    // std::swap on TestResource involves moves if available, or copies.
    // Assuming TestResource has efficient swap or move ops.
    // If TestResource only has copy, this would be 2 copy_ctor + 1 copy_assign (or 3 moves)
    // For our TestResource with move ops:
    // 1 move construction (for temp), 2 move assignments (for swapping back)
    // The exact counts depend on std::swap impl for T.
    // Let's just check values.

    // Case 2: One initialized, one not
    DelayedInit<TestResource> s3, s4_uninit;
    s3.emplace(3, "gamma");
    TestResource::reset_counts();

    s3.swap(s4_uninit); // Member swap

    ASSERT_FALSE(s3.is_initialized());
    ASSERT_TRUE(s4_uninit.is_initialized());
    ASSERT_EQ(s4_uninit->id, 3);
    ASSERT_EQ(s4_uninit->data, "gamma");
    ASSERT_EQ(TestResource::move_construction_count, 1); // s3's content moved to s4_uninit
    ASSERT_EQ(TestResource::destruction_count, 1); // s3's original (moved-from) object destroyed

    // Swap it back
    TestResource::reset_counts();
    s3.swap(s4_uninit);
    ASSERT_TRUE(s3.is_initialized());
    ASSERT_EQ(s3->id, 3);
    ASSERT_EQ(s3->data, "gamma");
    ASSERT_FALSE(s4_uninit.is_initialized());
    ASSERT_EQ(TestResource::move_construction_count, 1);
    ASSERT_EQ(TestResource::destruction_count, 1);

    // Case 3: Both uninitialized
    DelayedInit<TestResource> s5_uninit, s6_uninit;
    TestResource::reset_counts();
    swap(s5_uninit, s6_uninit);
    ASSERT_FALSE(s5_uninit.is_initialized());
    ASSERT_FALSE(s6_uninit.is_initialized());
    ASSERT_EQ(TestResource::construction_count, 0);
    ASSERT_EQ(TestResource::destruction_count, 0);
    ASSERT_EQ(TestResource::move_construction_count, 0);
}

TEST_F(DelayedInitTest, DestructorCalled) {
    TestResource::reset_counts();
    {
        DelayedInit<TestResource> di_tr;
        di_tr.emplace(1, "scope_test");
        ASSERT_EQ(TestResource::construction_count, 1);
        ASSERT_EQ(TestResource::destruction_count, 0);
    } // di_tr goes out of scope
    ASSERT_EQ(TestResource::destruction_count, 1);

    TestResource::reset_counts();
    {
        DelayedInit<TestResource> di_tr_uninit;
        // Never initialized
    }
    ASSERT_EQ(TestResource::destruction_count, 0); // No object, no destruction
}

TEST_F(DelayedInitTest, TypeAliases) {
    DelayedInitOnce<int> once;
    once.init(1);
    ASSERT_THROW(once.init(2), DelayedInitError);

    DelayedInitMutable<int> mut;
    mut.init(1);
    mut.init(2); // OK
    ASSERT_EQ(*mut, 2);
    mut.reset();
    ASSERT_FALSE(mut.is_initialized());

    DelayedInitNullable<int> null_val;
    null_val.init(1);
    null_val.init(2); // OK
    ASSERT_EQ(*null_val, 2);
    ASSERT_EQ(null_val.value_or(0), 2);
    null_val.reset();
    ASSERT_FALSE(null_val.is_initialized());
    ASSERT_EQ(null_val.value_or(0), 0);
}

// Main function for running tests (needed if not linking with a separate gtest_main)
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

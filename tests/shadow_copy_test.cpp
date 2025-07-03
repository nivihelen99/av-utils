#include "gtest/gtest.h"
#include <string>
#include <vector>
#include <memory>   // For std::unique_ptr
#include <stdexcept> // For std::logic_error

// Adjust path as necessary if your build system places headers elsewhere
#include "shadow_copy.h" // Assuming shadow_copy.h is in include directory, and tests are in tests/

// --- Helper Structs for Testing ---

struct SimpleData {
    int id;
    std::string name;

    bool operator==(const SimpleData& other) const {
        return id == other.id && name == other.name;
    }
    bool operator!=(const SimpleData& other) const {
        return !(*this == other);
    }
};

struct MoveOnlyData {
    std::unique_ptr<int> ptr;
    std::string id;

    MoveOnlyData(int val, std::string s_id) : ptr(std::make_unique<int>(val)), id(std::move(s_id)) {}
    MoveOnlyData(MoveOnlyData&& other) noexcept = default;
    MoveOnlyData& operator=(MoveOnlyData&& other) noexcept = default;

    // No copy constructor/assignment
    MoveOnlyData(const MoveOnlyData&) = delete;
    MoveOnlyData& operator=(const MoveOnlyData&) = delete;

    bool operator==(const MoveOnlyData& other) const {
        if (!ptr || !other.ptr) return !ptr && !other.ptr && id == other.id; // both null or one null
        return *ptr == *other.ptr && id == other.id;
    }
    bool operator!=(const MoveOnlyData& other) const {
        return !(*this == other);
    }
};

// To track copy/move operations for ShadowCopy itself
struct LifecycleTracker {
    static int copy_ctor_count;
    static int move_ctor_count;
    static int copy_assign_count;
    static int move_assign_count;
    int id;

    LifecycleTracker(int i = 0) : id(i) {}

    LifecycleTracker(const LifecycleTracker& other) : id(other.id) {
        copy_ctor_count++;
    }
    LifecycleTracker(LifecycleTracker&& other) noexcept : id(other.id) {
        move_ctor_count++;
        other.id = -1; // Mark as moved from
    }
    LifecycleTracker& operator=(const LifecycleTracker& other) {
        if (this != &other) {
            id = other.id;
            copy_assign_count++;
        }
        return *this;
    }
    LifecycleTracker& operator=(LifecycleTracker&& other) noexcept {
        if (this != &other) {
            id = other.id;
            move_assign_count++;
            other.id = -1; // Mark as moved from
        }
        return *this;
    }
    bool operator==(const LifecycleTracker& other) const { return id == other.id; }
    bool operator!=(const LifecycleTracker& other) const { return !(*this == other); }

    static void reset_counts() {
        copy_ctor_count = 0;
        move_ctor_count = 0;
        copy_assign_count = 0;
        move_assign_count = 0;
    }
};

int LifecycleTracker::copy_ctor_count = 0;
int LifecycleTracker::move_ctor_count = 0;
int LifecycleTracker::copy_assign_count = 0;
int LifecycleTracker::move_assign_count = 0;


// --- Test Fixture for LifecycleTracker ---
class LifecycleTrackerTest : public ::testing::Test {
protected:
    void SetUp() override {
        LifecycleTracker::reset_counts();
    }
};


// --- Test Cases ---

TEST(ShadowCopyTest, ConstructionAndInitialState) {
    SimpleData data = {1, "original"};
    ShadowCopy<SimpleData> sc_const_ref(data);

    EXPECT_EQ(sc_const_ref.original(), data);
    EXPECT_EQ(sc_const_ref.current(), data);
    EXPECT_FALSE(sc_const_ref.has_shadow());
    EXPECT_FALSE(sc_const_ref.modified()); // Not modified yet

    ShadowCopy<SimpleData> sc_rvalue(SimpleData{2, "rvalue_original"});
    EXPECT_EQ(sc_rvalue.original().id, 2);
    EXPECT_EQ(sc_rvalue.original().name, "rvalue_original");
    EXPECT_EQ(sc_rvalue.current().id, 2);
    EXPECT_FALSE(sc_rvalue.has_shadow());
    EXPECT_FALSE(sc_rvalue.modified());
}

TEST(ShadowCopyTest, GetAndModification) {
    SimpleData data = {10, "base"};
    ShadowCopy<SimpleData> sc(data);

    // First call to get()
    SimpleData& shadow1 = sc.get();
    EXPECT_TRUE(sc.has_shadow());
    EXPECT_TRUE(sc.modified()); // Modified because get() was called
    EXPECT_EQ(sc.current(), data); // Value initially same as original
    EXPECT_EQ(&shadow1, &sc.current()); // current() should return shadow
    EXPECT_EQ(sc.original(), data); // Original unchanged

    // Modify through shadow1
    shadow1.name = "modified_name";
    shadow1.id = 11;

    EXPECT_EQ(sc.current().name, "modified_name");
    EXPECT_EQ(sc.current().id, 11);
    EXPECT_EQ(sc.original().name, "base"); // Original still unchanged
    EXPECT_EQ(sc.original().id, 10);
    EXPECT_TRUE(sc.modified()); // Still modified (and now value differs too)

    // Second call to get()
    SimpleData& shadow2 = sc.get();
    EXPECT_EQ(&shadow1, &shadow2); // Should return same shadow object
    EXPECT_EQ(shadow2.name, "modified_name");

    // Test modified() when value is same as original but get() was called
    SimpleData data_s = {1, "same"};
    ShadowCopy<SimpleData> sc_same(data_s);
    sc_same.get(); // call get
    EXPECT_TRUE(sc_same.modified()); // true because get() was called
    EXPECT_EQ(sc_same.current(), sc_same.original()); // values are same
    
    sc_same.get().id = 2; // now change value
    EXPECT_TRUE(sc_same.modified()); // true because value differs (and get was called)
    EXPECT_NE(sc_same.current(), sc_same.original());
}

TEST(ShadowCopyTest, Commit) {
    SimpleData data = {20, "committable"};
    ShadowCopy<SimpleData> sc(data);

    sc.get().name = "new_name_to_commit";
    sc.get().id = 21;
    EXPECT_TRUE(sc.modified());
    EXPECT_TRUE(sc.has_shadow());

    SimpleData modified_val = sc.current(); // value before commit

    sc.commit();

    EXPECT_FALSE(sc.has_shadow());
    EXPECT_FALSE(sc.modified()); // Not modified after commit
    EXPECT_EQ(sc.original(), modified_val);
    EXPECT_EQ(sc.current(), modified_val); // Current is now the new original

    // Commit when no shadow exists (should be a no-op, state remains clean)
    sc.commit();
    EXPECT_FALSE(sc.has_shadow());
    EXPECT_FALSE(sc.modified());
    EXPECT_EQ(sc.original(), modified_val); // Original remains the same
}

TEST(ShadowCopyTest, Reset) {
    SimpleData data = {30, "resettable"};
    ShadowCopy<SimpleData> sc(data);

    sc.get().name = "temporary_name";
    sc.get().id = 31;
    EXPECT_TRUE(sc.modified());
    EXPECT_TRUE(sc.has_shadow());
    EXPECT_EQ(sc.current().name, "temporary_name");

    sc.reset();

    EXPECT_FALSE(sc.has_shadow());
    EXPECT_FALSE(sc.modified()); // Not modified after reset
    EXPECT_EQ(sc.original(), data); // Original is unchanged
    EXPECT_EQ(sc.current(), data);  // Current is now original

    // Reset when no shadow exists (should be a no-op)
    sc.reset();
    EXPECT_FALSE(sc.has_shadow());
    EXPECT_FALSE(sc.modified());
    EXPECT_EQ(sc.current(), data);
}

TEST(ShadowCopyTest, Take) {
    SimpleData data = {40, "takable"};
    ShadowCopy<SimpleData> sc(data);

    sc.get().name = "name_to_take";
    sc.get().id = 41;
    SimpleData shadow_val_before_take = sc.current();

    EXPECT_TRUE(sc.has_shadow());
    EXPECT_TRUE(sc.modified());

    SimpleData taken_val = sc.take();

    EXPECT_EQ(taken_val, shadow_val_before_take);
    EXPECT_FALSE(sc.has_shadow());
    EXPECT_FALSE(sc.modified());
    EXPECT_EQ(sc.original(), data); // Original unchanged
    EXPECT_EQ(sc.current(), data);  // Current is original

    // Test take when no shadow (should throw)
    EXPECT_THROW(sc.take(), std::logic_error);
}

TEST(ShadowCopyTest, MoveOnlyTypeConstructionAndMove) {
    // Test construction with a move-only type
    ShadowCopy<MoveOnlyData> sc(MoveOnlyData(100, "move_orig"));
    ASSERT_TRUE(sc.original().ptr);
    EXPECT_EQ(*sc.original().ptr, 100);
    EXPECT_EQ(sc.original().id, "move_orig");
    EXPECT_FALSE(sc.has_shadow());
    EXPECT_FALSE(sc.modified());

    // Test move construction of ShadowCopy<MoveOnlyData>
    ShadowCopy<MoveOnlyData> sc_moved_to(std::move(sc));
    ASSERT_TRUE(sc_moved_to.original().ptr); // original data should be moved
    EXPECT_EQ(*sc_moved_to.original().ptr, 100);
    EXPECT_EQ(sc_moved_to.original().id, "move_orig");
    EXPECT_FALSE(sc_moved_to.has_shadow()); // no shadow was created or moved
    EXPECT_FALSE(sc_moved_to.modified());

    // After move, 'sc' is in a valid but unspecified state.
    // For MoveOnlyData, its internal ptr is likely null after being moved from.
    // We check that original data is no longer in 'sc' or sc.original().ptr is null.
    EXPECT_FALSE(sc.original().ptr); // Original ptr should be null after move
                                     // or its value should not be the original if it wasn't nulled out by move.
                                     // For unique_ptr, it becomes null.
}

// Note: Testing commit/take for MoveOnlyData where a shadow is created via get() is problematic
// because get() requires T to be copy-constructible to create the shadow.
// The static_assert in shadow_copy.h correctly prevents this for non-copyable T.
// The "movable types T" support primarily applies to:
// 1. Construction: ShadowCopy(T&& value) can move T into original_.
// 2. Commit: original_ = std::move(*shadow_) can move T from shadow to original (if shadow exists and contains T).
// 3. Take: return std::move(*shadow_) can move T from shadow (if shadow exists and contains T).
// 4. ShadowCopy<T> itself being movable (as tested in MoveOnlyTypeConstructionAndMove and LifecycleTrackerTest).
//
// For (2) and (3) to be tested with a MoveOnlyData, the shadow must have been populated
// by means other than copying original_ (e.g., by moving a ShadowCopy that already had a shadow).


TEST_F(LifecycleTrackerTest, ShadowCopyObjectSemantics) {
    // Initial object
    ShadowCopy<LifecycleTracker> sc1(LifecycleTracker(1));
    sc1.get().id = 2; // Create shadow, modify it

    EXPECT_EQ(sc1.original().id, 1);
    EXPECT_EQ(sc1.current().id, 2);
    EXPECT_TRUE(sc1.has_shadow());
    EXPECT_TRUE(sc1.modified());

    // Copy constructor
    LifecycleTracker::reset_counts();
    ShadowCopy<LifecycleTracker> sc2 = sc1; // Copy ctor for ShadowCopy, copy for T, copy for optional<T>
    EXPECT_GE(LifecycleTracker::copy_ctor_count, 2); // At least one for original_, one for shadow_
    EXPECT_EQ(sc2.original().id, 1);
    EXPECT_EQ(sc2.current().id, 2);
    EXPECT_TRUE(sc2.has_shadow());
    EXPECT_EQ(sc2.modified(), sc1.modified());
    EXPECT_EQ(sc2.has_shadow(), sc1.has_shadow());
    if (sc1.has_shadow()) {
        EXPECT_EQ(sc2.current(), sc1.current());
    }
    EXPECT_EQ(sc2.original(), sc1.original());

    // Modify sc2, sc1 should be independent
    sc2.get().id = 3;
    EXPECT_EQ(sc1.current().id, 2);
    EXPECT_EQ(sc2.current().id, 3);

    // Copy assignment
    LifecycleTracker::reset_counts();
    ShadowCopy<LifecycleTracker> sc3(LifecycleTracker(10));
    sc3 = sc1; // Copy assign
    EXPECT_GE(LifecycleTracker::copy_ctor_count + LifecycleTracker::copy_assign_count, 2); // For T and optional<T>
    EXPECT_EQ(sc3.original().id, 1);
    EXPECT_EQ(sc3.current().id, 2);
    EXPECT_TRUE(sc3.has_shadow());
    EXPECT_EQ(sc3.modified(), sc1.modified());
    EXPECT_EQ(sc3.has_shadow(), sc1.has_shadow());
     if (sc1.has_shadow()) {
        EXPECT_EQ(sc3.current(), sc1.current());
    }
    EXPECT_EQ(sc3.original(), sc1.original());

    // Move constructor
    // Re-initialize sc1 to ensure it's in a known state with a shadow for moving
    sc1 = ShadowCopy<LifecycleTracker>(LifecycleTracker(1));
    sc1.get().id = 2;

    LifecycleTracker::reset_counts();
    ShadowCopy<LifecycleTracker> sc4 = std::move(sc1); // Move ctor
    EXPECT_GE(LifecycleTracker::move_ctor_count, 2); // original_ and shadow_ content
    EXPECT_EQ(sc4.original().id, 1);
    EXPECT_EQ(sc4.current().id, 2); // State moved
    EXPECT_TRUE(sc4.has_shadow());
    EXPECT_TRUE(sc4.modified());
    // sc1 is now in a valid but unspecified state. LifecycleTracker sets id to -1 on move.
    EXPECT_EQ(sc1.original().id, -1); // Check if original_ was moved from
    // Check if shadow was moved from (optional might be disengaged or its content moved)
    // If sc1 had a shadow, after move its shadow_ should be empty or contain a moved-from T.
    // If LifecycleTracker::move_ctor_count is 0, it means T was not moved, which is not expected for LifecycleTracker.

    // Move assignment
    ShadowCopy<LifecycleTracker> sc5(LifecycleTracker(20));
    sc5.get().id = 21; // Has its own shadow

    // Re-initialize sc1 for a fresh move source (since it was moved from)
    sc1 = ShadowCopy<LifecycleTracker>(LifecycleTracker(30));
    sc1.get().id = 31;

    LifecycleTracker::reset_counts();
    sc5 = std::move(sc1); // Move assign
    EXPECT_GE(LifecycleTracker::move_ctor_count + LifecycleTracker::move_assign_count, 2);
    EXPECT_EQ(sc5.original().id, 30);
    EXPECT_EQ(sc5.current().id, 31);
    EXPECT_TRUE(sc5.has_shadow());
    EXPECT_TRUE(sc5.modified());
    EXPECT_EQ(sc1.original().id, -1); // Check if original_ was moved from
}

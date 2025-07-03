#include <iostream> // For printing test names
#include <cassert>
#include <string>
#include <vector>
#include <memory>   // For std::unique_ptr
#include <stdexcept> // For std::logic_error

// Adjust path as necessary if your build system places headers elsewhere
#include "../include/shadow_copy.h"

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


// --- Test Functions ---

void test_construction_and_initial_state() {
    std::cout << "Running test_construction_and_initial_state..." << std::endl;
    SimpleData data = {1, "original"};
    ShadowCopy<SimpleData> sc_const_ref(data);

    assert(sc_const_ref.original() == data);
    assert(sc_const_ref.current() == data);
    assert(!sc_const_ref.has_shadow());
    assert(!sc_const_ref.modified()); // Not modified yet

    ShadowCopy<SimpleData> sc_rvalue(SimpleData{2, "rvalue_original"});
    assert(sc_rvalue.original().id == 2);
    assert(sc_rvalue.original().name == "rvalue_original");
    assert(sc_rvalue.current().id == 2);
    assert(!sc_rvalue.has_shadow());
    assert(!sc_rvalue.modified());
    std::cout << "PASSED test_construction_and_initial_state\n";
}

void test_get_and_modification() {
    std::cout << "Running test_get_and_modification..." << std::endl;
    SimpleData data = {10, "base"};
    ShadowCopy<SimpleData> sc(data);

    // First call to get()
    SimpleData& shadow1 = sc.get();
    assert(sc.has_shadow());
    assert(sc.modified()); // Modified because get() was called
    assert(sc.current() == data); // Value initially same as original
    assert(&shadow1 == &sc.current()); // current() should return shadow
    assert(sc.original() == data); // Original unchanged

    // Modify through shadow1
    shadow1.name = "modified_name";
    shadow1.id = 11;

    assert(sc.current().name == "modified_name");
    assert(sc.current().id == 11);
    assert(sc.original().name == "base"); // Original still unchanged
    assert(sc.original().id == 10);
    assert(sc.modified()); // Still modified (and now value differs too)

    // Second call to get()
    SimpleData& shadow2 = sc.get();
    assert(&shadow1 == &shadow2); // Should return same shadow object
    assert(shadow2.name == "modified_name");

    // Test modified() when value is same as original but get() was called
    SimpleData data_s = {1, "same"};
    ShadowCopy<SimpleData> sc_same(data_s);
    sc_same.get(); // call get
    assert(sc_same.modified()); // true because get() was called
    assert(sc_same.current() == sc_same.original()); // values are same
    
    sc_same.get().id = 2; // now change value
    assert(sc_same.modified()); // true because value differs (and get was called)
    assert(sc_same.current() != sc_same.original());


    std::cout << "PASSED test_get_and_modification\n";
}

void test_commit() {
    std::cout << "Running test_commit..." << std::endl;
    SimpleData data = {20, "committable"};
    ShadowCopy<SimpleData> sc(data);

    sc.get().name = "new_name_to_commit";
    sc.get().id = 21;
    assert(sc.modified());
    assert(sc.has_shadow());

    SimpleData modified_val = sc.current(); // value before commit

    sc.commit();

    assert(!sc.has_shadow());
    assert(!sc.modified()); // Not modified after commit
    assert(sc.original() == modified_val);
    assert(sc.current() == modified_val); // Current is now the new original

    // Commit when no shadow exists (should be a no-op, state remains clean)
    sc.commit();
    assert(!sc.has_shadow());
    assert(!sc.modified());
    assert(sc.original() == modified_val); // Original remains the same
    std::cout << "PASSED test_commit\n";
}

void test_reset() {
    std::cout << "Running test_reset..." << std::endl;
    SimpleData data = {30, "resettable"};
    ShadowCopy<SimpleData> sc(data);

    sc.get().name = "temporary_name";
    sc.get().id = 31;
    assert(sc.modified());
    assert(sc.has_shadow());
    assert(sc.current().name == "temporary_name");

    sc.reset();

    assert(!sc.has_shadow());
    assert(!sc.modified()); // Not modified after reset
    assert(sc.original() == data); // Original is unchanged
    assert(sc.current() == data);  // Current is now original

    // Reset when no shadow exists (should be a no-op)
    sc.reset();
    assert(!sc.has_shadow());
    assert(!sc.modified());
    assert(sc.current() == data);
    std::cout << "PASSED test_reset\n";
}

void test_take() {
    std::cout << "Running test_take..." << std::endl;
    SimpleData data = {40, "takable"};
    ShadowCopy<SimpleData> sc(data);

    sc.get().name = "name_to_take";
    sc.get().id = 41;
    SimpleData shadow_val_before_take = sc.current();

    assert(sc.has_shadow());
    assert(sc.modified());

    SimpleData taken_val = sc.take();

    assert(taken_val == shadow_val_before_take);
    assert(!sc.has_shadow());
    assert(!sc.modified());
    assert(sc.original() == data); // Original unchanged
    assert(sc.current() == data);  // Current is original

    // Test take when no shadow (should throw)
    bool thrown = false;
    try {
        sc.take();
    } catch (const std::logic_error&) {
        thrown = true;
    }
    assert(thrown);
    std::cout << "PASSED test_take\n";
}

void test_move_only_type() {
    std::cout << "Running test_move_only_type..." << std::endl;
    
    // Test construction with a move-only type
    ShadowCopy<MoveOnlyData> sc(MoveOnlyData(100, "move_orig"));
    assert(sc.original().ptr && *sc.original().ptr == 100);
    assert(sc.original().id == "move_orig");
    assert(!sc.has_shadow());
    assert(!sc.modified());

    // Calling ShadowCopy<MoveOnlyData>::get() to *create* a shadow from original_
    // would fail a static_assert(std::is_copy_constructible_v<MoveOnlyData>, ...),
    // because original_ (MoveOnlyData) cannot be copied to initialize shadow_.
    // The unique_ptr within MoveOnlyData makes it non-copyable.
    //
    // Therefore, direct testing of get() creating a shadow, and subsequent commit/reset/take
    // that rely on this mode of shadow creation, is not applicable for strictly move-only types
    // with the current ShadowCopy design.
    //
    // The "movable types T" support primarily applies to:
    // 1. Construction: ShadowCopy(T&& value) can move into original_.
    // 2. Commit: original_ = std::move(*shadow_) can move from shadow to original.
    // 3. Take: return std::move(*shadow_) can move from shadow.
    // 4. ShadowCopy<T> itself being movable.
    //
    // To test items 2 and 3, a shadow containing MoveOnlyData would need to exist.
    // This could happen if a ShadowCopy<MoveOnlyData> object that *has* a shadow is moved.

    std::cout << "  NOTE: test_move_only_type: ShadowCopy<T>::get() for shadow creation requires T to be copy-constructible." << std::endl;
    std::cout << "  This test primarily verifies construction with MoveOnlyData and its behavior during ShadowCopy moves." << std::endl;

    // Test move construction of ShadowCopy<MoveOnlyData>
    ShadowCopy<MoveOnlyData> sc_moved_to(std::move(sc));
    assert(sc_moved_to.original().ptr && *sc_moved_to.original().ptr == 100); // original data should be moved
    assert(sc_moved_to.original().id == "move_orig");
    assert(!sc_moved_to.has_shadow()); // no shadow was created or moved
    assert(!sc_moved_to.modified());

    // After move, 'sc' is in a valid but unspecified state.
    // For MoveOnlyData, its internal ptr is likely null after being moved from.
    // We check that original data is no longer in 'sc' or sc.original().ptr is null.
    assert(!sc.original().ptr || (sc.original().ptr && *sc.original().ptr != 100));


    // To further test commit/take with MoveOnlyData, one would need a ShadowCopy<MoveOnlyData>
    // that already has a shadow. This typically involves moving a ShadowCopy object.
    // Example sketch (not fully testable without a more complex setup or a T that can be put into shadow):
    // ShadowCopy<MoveOnlyData> source_with_shadow(MoveOnlyData(300, "src_orig"));
    // // IF we could somehow set a shadow in source_with_shadow without copy, e.g.
    // // source_with_shadow.force_set_shadow(MoveOnlyData(301, "src_shadow")); // (Not an existing API)
    // // THEN:
    // // ShadowCopy<MoveOnlyData> sc_dest_for_commit_take = std::move(source_with_shadow);
    // // assert(sc_dest_for_commit_take.has_shadow());
    // // MoveOnlyData taken = sc_dest_for_commit_take.take();
    // // assert(*taken.ptr == 301);
    // // Similar for commit.

    std::cout << "PASSED test_move_only_type (construction and move of ShadowCopy<MoveOnlyData> tested; get() for shadow creation is correctly disallowed by static_assert for non-copyable T)\n";
}

void test_shadow_copy_object_semantics() {
    std::cout << "Running test_shadow_copy_object_semantics..." << std::endl;
    LifecycleTracker::reset_counts();

    // Initial object
    ShadowCopy<LifecycleTracker> sc1(LifecycleTracker(1));
    sc1.get().id = 2; // Create shadow, modify it

    assert(sc1.original().id == 1);
    assert(sc1.current().id == 2);
    assert(sc1.has_shadow());
    assert(sc1.modified());

    // Copy constructor
    LifecycleTracker::reset_counts();
    ShadowCopy<LifecycleTracker> sc2 = sc1; // Copy ctor for ShadowCopy, copy for T, copy for optional<T>
    assert(LifecycleTracker::copy_ctor_count >= 2); // At least one for original_, one for shadow_
    assert(sc2.original().id == 1);
    assert(sc2.current().id == 2);
    assert(sc2.has_shadow());
    assert(sc2.modified()); // State should be copied
    // Verify state consistency which implies get_called_ was handled correctly
    assert(sc2.modified() == sc1.modified());
    assert(sc2.has_shadow() == sc1.has_shadow());
    if (sc1.has_shadow()) { // If there's a shadow, current values should match
        assert(sc2.current() == sc1.current());
    }
    assert(sc2.original() == sc1.original());


    // Modify sc2, sc1 should be independent
    sc2.get().id = 3;
    assert(sc1.current().id == 2);
    assert(sc2.current().id == 3);

    // Copy assignment
    LifecycleTracker::reset_counts();
    ShadowCopy<LifecycleTracker> sc3(LifecycleTracker(10));
    sc3 = sc1; // Copy assign
    assert(LifecycleTracker::copy_ctor_count + LifecycleTracker::copy_assign_count >= 2); // For T and optional<T>
    assert(sc3.original().id == 1);
    assert(sc3.current().id == 2);
    assert(sc3.has_shadow());
    assert(sc3.modified());
    assert(sc3.modified() == sc1.modified());
    assert(sc3.has_shadow() == sc1.has_shadow());
    if (sc1.has_shadow()) {
        assert(sc3.current() == sc1.current());
    }
    assert(sc3.original() == sc1.original());


    // Move constructor
    LifecycleTracker::reset_counts();
    ShadowCopy<LifecycleTracker> sc4 = std::move(sc1); // Move ctor
    // For T (original_) and std::optional<T> (shadow_), their move ctors should be called.
    // LifecycleTracker's move ctor increments move_ctor_count.
    assert(LifecycleTracker::move_ctor_count >= 2); // original_ and shadow_ content
    assert(sc4.original().id == 1);
    assert(sc4.current().id == 2); // State moved
    assert(sc4.has_shadow());
    assert(sc4.modified());
    // sc1 is now in a valid but unspecified state, check if its members were moved from
    // This depends on T's move op. Our LifecycleTracker sets id to -1.
    assert(sc1.original().id == -1 || LifecycleTracker::move_ctor_count == 0); // if T is not moved, original id remains
                                                                            // but shadow should be empty or moved
    // A well-behaved optional<T>::optional(optional&&) will move T if T is movable.
    // And T original_ = std::move(other.original_) will move T.


    // Move assignment
    ShadowCopy<LifecycleTracker> sc5(LifecycleTracker(20));
    sc5.get().id = 21; // Has its own shadow

    // Re-initialize sc1 for a fresh move source (since it was moved from)
    sc1 = ShadowCopy<LifecycleTracker>(LifecycleTracker(30));
    sc1.get().id = 31;

    LifecycleTracker::reset_counts();
    sc5 = std::move(sc1); // Move assign
    assert(LifecycleTracker::move_ctor_count + LifecycleTracker::move_assign_count >= 2);
    assert(sc5.original().id == 30);
    assert(sc5.current().id == 31);
    assert(sc5.has_shadow());
    assert(sc5.modified());

    std::cout << "PASSED test_shadow_copy_object_semantics\n";
}


int main() {
    std::cout << "--- Starting ShadowCopy Tests ---" << std::endl;

    test_construction_and_initial_state();
    test_get_and_modification();
    test_commit();
    test_reset();
    test_take();
    test_move_only_type();
    test_shadow_copy_object_semantics();

    std::cout << "\n--- All ShadowCopy Tests Completed Successfully ---" << std::endl;
    return 0;
}

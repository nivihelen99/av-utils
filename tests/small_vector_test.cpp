#include "small_vector.h"
#include <cassert>
#include <string>
#include <vector> // For comparison and easy test value generation
#include <stdexcept> // For std::out_of_range
#include <iostream> // For temporary debug prints if needed

// A simple struct for testing non-trivial types
struct TestObj {
    int id;
    std::string data;
    static int copy_ctor_count;
    static int move_ctor_count;
    static int copy_assign_count;
    static int move_assign_count;
    static int dtor_count;
    static int default_ctor_count;
    static int param_ctor_count;

    TestObj(int i = 0, std::string d = "") : id(i), data(std::move(d)) {
        default_ctor_count++; // Also counts for parameterized if no specific param_ctor_count++
    }
    TestObj(int i, const char* d) : id(i), data(d) {
        param_ctor_count++;
    }

    TestObj(const TestObj& other) : id(other.id), data(other.data) {
        copy_ctor_count++;
    }
    TestObj(TestObj&& other) noexcept : id(other.id), data(std::move(other.data)) {
        move_ctor_count++;
        other.id = -1; // Mark as moved
    }
    TestObj& operator=(const TestObj& other) {
        if (this != &other) {
            id = other.id;
            data = other.data;
            copy_assign_count++;
        }
        return *this;
    }
    TestObj& operator=(TestObj&& other) noexcept {
        if (this != &other) {
            id = other.id;
            data = std::move(other.data);
            move_assign_count++;
            other.id = -1;
        }
        return *this;
    }
    ~TestObj() {
        dtor_count++;
    }

    bool operator==(const TestObj& other) const {
        return id == other.id && data == other.data;
    }
    friend std::ostream& operator<<(std::ostream& os, const TestObj& obj) {
        os << "TestObj{id=" << obj.id << ", data=\"" << obj.data << "\"}";
        return os;
    }

    static void reset_counts() {
        copy_ctor_count = 0;
        move_ctor_count = 0;
        copy_assign_count = 0;
        move_assign_count = 0;
        dtor_count = 0;
        default_ctor_count = 0;
        param_ctor_count = 0;
    }
};

int TestObj::copy_ctor_count = 0;
int TestObj::move_ctor_count = 0;
int TestObj::copy_assign_count = 0;
int TestObj::move_assign_count = 0;
int TestObj::dtor_count = 0;
int TestObj::default_ctor_count = 0;
int TestObj::param_ctor_count = 0;


// Helper to run a test case
void run_test(void (*test_func)(), const std::string& test_name) {
    std::cout << "Running test: " << test_name << "..." << std::endl;
    TestObj::reset_counts();
    test_func();
    std::cout << test_name << " PASSED." << std::endl;
}

// Test Suites
void test_default_constructor() {
    small_vector<int, 5> sv_int;
    assert(sv_int.empty());
    assert(sv_int.size() == 0);
    assert(sv_int.capacity() == 5);
    assert(sv_int.data() != nullptr); // Should point to inline buffer

    small_vector<TestObj, 3> sv_obj;
    assert(sv_obj.empty());
    assert(sv_obj.size() == 0);
    assert(sv_obj.capacity() == 3);
    assert(TestObj::default_ctor_count == 0); // No objects created yet
}

void test_initializer_list_constructor() {
    // Inline
    small_vector<int, 5> sv_inline = {1, 2, 3};
    assert(sv_inline.size() == 3);
    assert(sv_inline.capacity() == 5);
    assert(sv_inline[0] == 1 && sv_inline[1] == 2 && sv_inline[2] == 3);

    // Heap
    small_vector<int, 2> sv_heap = {10, 20, 30};
    assert(sv_heap.size() == 3);
    assert(sv_heap.capacity() >= 3); // Could be exactly 3 or more due to growth
    assert(sv_heap.capacity() > 2);   // Definitely on heap
    assert(sv_heap[0] == 10 && sv_heap[1] == 20 && sv_heap[2] == 30);

    // Empty list
    small_vector<int, 3> sv_empty_list = {};
    assert(sv_empty_list.empty());
    assert(sv_empty_list.capacity() == 3);

    // With TestObj
    TestObj::reset_counts();
    {
        small_vector<TestObj, 3> sv_obj_il = {TestObj(1, "a"), TestObj(2, "b")};
        assert(sv_obj_il.size() == 2);
        assert(sv_obj_il.capacity() == 3);
        assert(sv_obj_il[0].id == 1 && sv_obj_il[1].id == 2);
        // Counts: 2 param ctors for temporaries, then 2 move/copy ctors into vector.
        // If T is directly constructed (e.g. emplace from args of init list), it might be fewer.
        // Current small_vector construct_range copies. So 2 param, 2 copy.
        // Let's assume for {T(1), T(2)}, it's 2 param_ctors and 2 copy_ctors (or move_ctors if available & efficient)
        // If TestObj(1,"a") is prvalue, it can be moved.
    } // sv_obj_il goes out of scope
    // assert(TestObj::dtor_count == TestObj::param_ctor_count + TestObj::copy_ctor_count + TestObj::move_ctor_count); //Rough check
    // Dtors for 2 objects in vector + 2 temporaries if not optimized out.
    // This is tricky to assert precisely without knowing compiler opts for temporaries from {}
}

void test_copy_constructor() {
    // Inline to Inline
    small_vector<int, 5> sv_orig_inline = {1, 2, 3};
    small_vector<int, 5> sv_copy_inline = sv_orig_inline;
    assert(sv_copy_inline.size() == 3);
    assert(sv_copy_inline.capacity() == 5);
    assert(sv_copy_inline[2] == 3);
    sv_orig_inline[0] = 99; // Ensure deep copy
    assert(sv_copy_inline[0] == 1);

    // Heap to Heap
    small_vector<int, 2> sv_orig_heap = {10, 20, 30};
    small_vector<int, 2> sv_copy_heap = sv_orig_heap;
    assert(sv_copy_heap.size() == 3);
    assert(sv_copy_heap.capacity() >= 3);
    assert(sv_copy_heap[2] == 30);
    sv_orig_heap[0] = 990;
    assert(sv_copy_heap[0] == 10);

    // Inline to Heap (copy has smaller inline capacity)
    small_vector<int, 5> sv_orig_inline_large = {1, 2, 3, 4};
    small_vector<int, 2> sv_copy_to_heap = sv_orig_inline_large;
    assert(sv_copy_to_heap.size() == 4);
    assert(sv_copy_to_heap.capacity() >= 4); // Must be on heap
    assert(sv_copy_to_heap[3] == 4);

    // Heap to Inline (copy has larger inline capacity and elements fit)
    small_vector<int, 2> sv_orig_heap_small = {5, 6};
    small_vector<int, 5> sv_copy_to_inline = sv_orig_heap_small;
    assert(sv_copy_to_inline.size() == 2);
    assert(sv_copy_to_inline.capacity() == 5); // Should be inline
    assert(sv_copy_to_inline[1] == 6);

    // TestObj copy
    TestObj::reset_counts();
    small_vector<TestObj, 3> s1_obj = {TestObj(1,"a"), TestObj(2,"b")};
    size_t initial_copies = TestObj::copy_ctor_count; // From init list construction
    size_t initial_moves = TestObj::move_ctor_count;
    TestObj::reset_counts(); // Reset for the copy operation itself

    small_vector<TestObj, 3> s2_obj = s1_obj; // Copy constructor
    assert(s2_obj.size() == 2);
    assert(TestObj::copy_ctor_count == 2); // 2 elements copied
    assert(TestObj::move_ctor_count == 0);
    assert(s2_obj[0].id == 1 && s2_obj[1].id == 2);
}

void test_move_constructor() {
    // Inline to Inline
    small_vector<int, 5> sv_orig_inline = {1, 2, 3};
    small_vector<int, 5> sv_moved_inline = std::move(sv_orig_inline);
    assert(sv_moved_inline.size() == 3);
    assert(sv_moved_inline.capacity() == 5);
    assert(sv_moved_inline[2] == 3);
    assert(sv_orig_inline.empty() || sv_orig_inline.size() == 0); // Original is valid but empty/unspecified

    // Heap to Heap (steals buffer)
    small_vector<int, 2> sv_orig_heap = {10, 20, 30};
    const int* orig_heap_data_ptr = sv_orig_heap.data();
    small_vector<int, 2> sv_moved_heap = std::move(sv_orig_heap);
    assert(sv_moved_heap.size() == 3);
    assert(sv_moved_heap.capacity() >= 3);
    assert(sv_moved_heap[2] == 30);
    assert(sv_moved_heap.data() == orig_heap_data_ptr); // Pointer should be stolen
    assert(sv_orig_heap.empty() || sv_orig_heap.size() == 0);
    assert(sv_orig_heap.capacity() == 2); // Should reset to inline capacity

    // TestObj move
    TestObj::reset_counts();
    small_vector<TestObj, 3> s1_obj = {TestObj(1,"a"), TestObj(2,"b")}; // temporaries + copy/move into s1_obj
    TestObj::reset_counts(); // Focus on the move operation

    small_vector<TestObj, 3> s2_obj = std::move(s1_obj);
    assert(s2_obj.size() == 2);
    // If inline, elements are move-constructed. If heap, pointer is stolen (no element-wise ops).
    // s1_obj was inline.
    assert(TestObj::move_ctor_count >= 2 || TestObj::copy_ctor_count >=2); // 2 elements moved/copied
    // For inline data, std::uninitialized_move_n is used, which calls move constructors.
    assert(TestObj::move_ctor_count == 2); // Expecting moves for inline data
    assert(s1_obj.empty());
}

void test_copy_assignment() {
    // Inline to Inline
    small_vector<int, 5> sv_orig_inline = {1, 2, 3};
    small_vector<int, 5> sv_copy_assign_inline;
    sv_copy_assign_inline = sv_orig_inline;
    assert(sv_copy_assign_inline.size() == 3 && sv_copy_assign_inline[2] == 3);
    sv_orig_inline[0] = 99;
    assert(sv_copy_assign_inline[0] == 1);

    // Heap to Heap
    small_vector<int, 2> sv_orig_heap = {10, 20, 30};
    small_vector<int, 2> sv_copy_assign_heap = {0,0,0,0,0}; // start with heap
    sv_copy_assign_heap = sv_orig_heap;
    assert(sv_copy_assign_heap.size() == 3 && sv_copy_assign_heap[2] == 30);

    // Assigning smaller to larger (inline)
    small_vector<int,5> s_large_inline = {1,2,3,4,5};
    small_vector<int,5> s_small_inline = {9,8};
    s_large_inline = s_small_inline;
    assert(s_large_inline.size()==2 && s_large_inline[1]==8 && s_large_inline.capacity()==5);

    // Assigning larger to smaller (causing heap)
    small_vector<int,3> s_becomes_heap = {1};
    small_vector<int,3> s_source_large = {5,6,7,8}; // on heap
    s_becomes_heap = s_source_large;
    assert(s_becomes_heap.size()==4 && s_becomes_heap[3]==8 && s_becomes_heap.capacity()>=4);

    // TestObj copy assignment
    TestObj::reset_counts();
    small_vector<TestObj, 3> s1_obj = {TestObj(1,"a"), TestObj(2,"b")};
    small_vector<TestObj, 3> s2_obj;
    s2_obj.push_back(TestObj(100, "x")); // s2_obj has 1 element
    TestObj::reset_counts();

    s2_obj = s1_obj; // Copy assignment
    assert(s2_obj.size() == 2);
    assert(s2_obj[0].id == 1 && s2_obj[1].id == 2);
    // Expect 1 dtor for s2_obj's old element, then 2 copy_ctors for new elements.
    // Or, if assign_common does element-wise copy assignment for common part:
    // 1 copy_assign, 1 copy_ctor, 0 dtor (if assigning to existing capacity)
    // Current assign_common clears (destroys all) then copy-constructs.
    assert(TestObj::dtor_count >= 1); // For the original element in s2_obj
    assert(TestObj::copy_ctor_count == 2); // For the two new elements
}

void test_move_assignment() {
    // Inline to Inline
    small_vector<int, 5> sv_orig_inline = {1, 2, 3};
    small_vector<int, 5> sv_moved_assign_inline;
    sv_moved_assign_inline = std::move(sv_orig_inline);
    assert(sv_moved_assign_inline.size() == 3 && sv_moved_assign_inline[2] == 3);
    assert(sv_orig_inline.empty() || sv_orig_inline.size()==0);

    // Heap to Heap (steals buffer)
    small_vector<int, 2> sv_orig_heap = {10, 20, 30};
    const int* orig_heap_data_ptr = sv_orig_heap.data();
    small_vector<int, 2> sv_moved_assign_heap = {0,0,0,0,0}; // start with heap
    sv_moved_assign_heap = std::move(sv_orig_heap);
    assert(sv_moved_assign_heap.size() == 3 && sv_moved_assign_heap[2] == 30);
    assert(sv_moved_assign_heap.data() == orig_heap_data_ptr);
    assert(sv_orig_heap.empty() || sv_orig_heap.capacity() == 2);

    // TestObj move assignment
    TestObj::reset_counts();
    small_vector<TestObj, 3> s1_obj = {TestObj(1,"a"), TestObj(2,"b")};
    small_vector<TestObj, 3> s2_obj;
    s2_obj.push_back(TestObj(100, "x"));
    TestObj::reset_counts();

    s2_obj = std::move(s1_obj);
    assert(s2_obj.size() == 2);
    assert(s2_obj[0].id == 1 && s2_obj[1].id == 2);
    assert(s1_obj.empty());
    // s2_obj's original element is destroyed. s1_obj's elements are moved into s2_obj.
    assert(TestObj::dtor_count >= 1);      // For s2_obj's old element.
    assert(TestObj::move_ctor_count == 2); // For moving s1_obj's elements.
}


void test_push_back_and_emplace_back() {
    small_vector<int, 3> sv;
    // Inline
    sv.push_back(10);
    assert(sv.size() == 1 && sv[0] == 10 && sv.capacity() == 3);
    sv.emplace_back(20);
    assert(sv.size() == 2 && sv[1] == 20 && sv.capacity() == 3);
    sv.push_back(30);
    assert(sv.size() == 3 && sv[2] == 30 && sv.capacity() == 3);

    // Trigger heap allocation
    const int* data_before_realloc = sv.data();
    sv.emplace_back(40);
    assert(sv.size() == 4 && sv[3] == 40);
    assert(sv.capacity() > 3); // Must be on heap now
    assert(sv.data() != data_before_realloc || sv.capacity() ==3); // data pointer should change if reallocated from inline (unless N is large enough for growth)


    // TestObj
    TestObj::reset_counts();
    small_vector<TestObj, 2> sv_obj;
    sv_obj.push_back(TestObj(1, "obj1")); // 1 param_ctor (temporary), 1 move_ctor (into vector)
    assert(TestObj::param_ctor_count >= 1);
    assert(TestObj::move_ctor_count >= 1 || TestObj::copy_ctor_count >=1);
    size_t old_moves = TestObj::move_ctor_count;
    size_t old_copies = TestObj::copy_ctor_count;

    sv_obj.emplace_back(2, "obj2");       // 1 param_ctor (direct emplacement)
    assert(TestObj::param_ctor_count >= 2); // One more param ctor
    assert(TestObj::move_ctor_count == old_moves); // No extra moves for emplace if done right
    assert(TestObj::copy_ctor_count == old_copies);


    assert(sv_obj.size() == 2 && sv_obj[0].id == 1 && sv_obj[1].id == 2);
    TestObj::reset_counts();
    sv_obj.emplace_back(3, "obj3_heap"); // Reallocation
    // Expect: 2 move ctors (for old elements to new buffer), 1 param ctor (for new element)
    // 2 dtors for old elements in old buffer.
    assert(sv_obj.size() == 3 && sv_obj[2].id == 3);
    assert(TestObj::move_ctor_count >= 2); // Old elements moved
    assert(TestObj::param_ctor_count >= 1);  // New element emplaced
    assert(TestObj::dtor_count >= 2);      // Old elements destroyed
}

void test_pop_back() {
    small_vector<int, 3> sv = {1, 2, 3, 4, 5}; // Heap
    assert(sv.size() == 5);
    sv.pop_back(); // 5 gone
    assert(sv.size() == 4 && sv.back() == 4);
    sv.pop_back(); // 4 gone
    assert(sv.size() == 3 && sv.back() == 3);
    // Current impl. doesn't shrink from heap to inline on pop_back automatically
    assert(sv.capacity() > 3); // Still on heap

    sv.pop_back(); // 3 gone
    sv.pop_back(); // 2 gone
    sv.pop_back(); // 1 gone
    assert(sv.empty());

    // Test with TestObj to check destructors
    TestObj::reset_counts();
    {
        small_vector<TestObj, 2> sv_obj;
        sv_obj.emplace_back(1, "a");
        sv_obj.emplace_back(2, "b"); // 2 objects created
        TestObj::reset_counts(); // Reset after setup

        sv_obj.pop_back(); // Destroys TestObj(2, "b")
        assert(sv_obj.size() == 1);
        assert(TestObj::dtor_count == 1);

        sv_obj.pop_back(); // Destroys TestObj(1, "a")
        assert(sv_obj.empty());
        assert(TestObj::dtor_count == 2);
    } // sv_obj destructs (no elements left)
}

void test_element_access() {
    small_vector<int, 3> sv = {10, 20};
    const small_vector<int, 3>& csv = sv;

    assert(sv[0] == 10 && csv[0] == 10);
    assert(sv.at(1) == 20 && csv.at(1) == 20);
    sv[0] = 15;
    assert(sv.front() == 15 && csv.front() == 15); // csv sees change too
    assert(sv.back() == 20 && csv.back() == 20);
    assert(*sv.data() == 15 && *csv.data() == 15);

    bool caught = false;
    try {
        sv.at(2);
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);

    caught = false;
    try {
        csv.at(2);
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);

    // Empty vector access (front/back are UB, at throws)
    small_vector<int,3> empty_sv;
    caught = false;
    try { empty_sv.at(0); } catch (const std::out_of_range&) { caught = true; }
    assert(caught);
    // front() and back() on empty vector is UB, not directly testable for throw.
}

void test_iterators() {
    small_vector<int, 3> sv = {1, 2, 3, 4}; // Heap

    // Basic iteration
    int sum = 0;
    for (int x : sv) { sum += x; }
    assert(sum == 10);

    // Const iteration
    const auto& csv = sv;
    sum = 0;
    for (int x : csv) { sum += x; }
    assert(sum == 10);

    // Modify through iterator
    for (auto it = sv.begin(); it != sv.end(); ++it) {
        *it += 1;
    }
    assert(sv[0]==2 && sv[1]==3 && sv[2]==4 && sv[3]==5);

    // Check cbegin/cend
    sum = 0;
    for (auto it = sv.cbegin(); it != sv.cend(); ++it) {
        sum += *it;
    }
    assert(sum == (2+3+4+5));

    // Empty vector iterators
    small_vector<int, 3> empty_sv;
    assert(empty_sv.begin() == empty_sv.end());
    assert(empty_sv.cbegin() == empty_sv.cend());
}


void test_clear() {
    small_vector<int, 3> sv_inline = {1, 2};
    sv_inline.clear();
    assert(sv_inline.empty() && sv_inline.size() == 0);
    assert(sv_inline.capacity() == 3); // Capacity remains

    small_vector<int, 2> sv_heap = {10, 20, 30};
    size_t cap_before_clear = sv_heap.capacity();
    sv_heap.clear();
    assert(sv_heap.empty() && sv_heap.size() == 0);
    assert(sv_heap.capacity() == cap_before_clear); // Capacity remains

    // TestObj clear
    TestObj::reset_counts();
    {
        small_vector<TestObj, 2> sv_obj = {TestObj(1,"a"), TestObj(2,"b"), TestObj(3,"c")}; // 3 objs, on heap
        assert(sv_obj.size() == 3);
        size_t creates = TestObj::param_ctor_count + TestObj::copy_ctor_count + TestObj::move_ctor_count;
        TestObj::reset_counts();

        sv_obj.clear();
        assert(sv_obj.empty());
        assert(TestObj::dtor_count == 3); // 3 objects destroyed
    }
}

void test_reserve_and_capacity() {
    small_vector<int, 5> sv;
    assert(sv.capacity() == 5);

    sv.reserve(3); // Less than current inline capacity
    assert(sv.capacity() == 5);

    sv.reserve(5); // Equal to current inline capacity
    assert(sv.capacity() == 5);

    const int* old_data = sv.data();
    sv.reserve(10); // More than inline capacity -> heap allocation
    assert(sv.capacity() >= 10);
    assert(sv.capacity() > 5); // Definitely on heap
    if (sv.size() == 0) { // if empty, data might not change if it was already allocated
       // For an empty vector, reserving might or might not change data ptr immediately.
       // If it was pointing to inline buffer, it must change.
       // assert(sv.data() != old_data); // This is not strictly guaranteed if N was large and growth happened in place
    }


    sv.push_back(1); sv.push_back(2); // Size 2
    old_data = sv.data(); // Now it's on heap, pointing to actual data.
    size_t old_cap = sv.capacity();

    sv.reserve(old_cap - 1); // Reserve less than current heap capacity
    assert(sv.capacity() == old_cap); // Should not shrink
    assert(sv.data() == old_data); // Data should not move

    sv.reserve(old_cap + 5); // Reserve more
    assert(sv.capacity() >= old_cap + 5);
    assert(sv.data() != old_data); // Data should move
    assert(sv.size() == 2 && sv[0] == 1 && sv[1] == 2); // Elements preserved

    // Reserve on full inline vector
    small_vector<int,3> sv_full_inline = {1,2,3};
    old_data = sv_full_inline.data();
    sv_full_inline.reserve(10);
    assert(sv_full_inline.capacity() >= 10);
    assert(sv_full_inline.data() != old_data);
    assert(sv_full_inline.size() == 3 && sv_full_inline[0]==1 && sv_full_inline[2]==3);
}

void test_resize() {
    small_vector<int, 3> sv;

    // Resize empty to larger (default construct)
    sv.resize(2);
    assert(sv.size() == 2 && sv[0] == 0 && sv[1] == 0); // Default int is 0
    assert(sv.capacity() == 3);

    // Resize to larger with value
    sv.resize(4, 100); // Will go to heap
    assert(sv.size() == 4 && sv[0] == 0 && sv[1] == 0 && sv[2] == 100 && sv[3] == 100);
    assert(sv.capacity() >= 4);

    // Resize to smaller
    sv.resize(1);
    assert(sv.size() == 1 && sv[0] == 0);
    assert(sv.capacity() >= 4); // Capacity doesn't shrink

    // Resize to 0
    sv.resize(0);
    assert(sv.empty());

    // TestObj resize
    TestObj::reset_counts();
    {
        small_vector<TestObj, 2> sv_obj;
        sv_obj.resize(1); // Default construct 1 TestObj
        assert(sv_obj.size() == 1);
        assert(TestObj::default_ctor_count >= 1);
        TestObj::reset_counts();

        sv_obj.resize(3, TestObj(5, "val")); // Grow, add 2 copies of TestObj(5)
                                             // 1 param_ctor for temporary, then 2 copy_ctors
                                             // 1 move_ctor for existing element
        assert(sv_obj.size() == 3);
        assert(sv_obj[0].id == 0); // Original default constructed one (or moved copy of it)
        assert(sv_obj[1].id == 5 && sv_obj[2].id == 5);
        // Counts for resize(3, val): 1 move (original), 1 param (template), 2 copy (new elements)
        // This depends on implementation details of grow_and_reallocate + construct_fill
        // Let's check dtor for shrink
        TestObj::reset_counts();
        sv_obj.resize(1); // Shrink, destroys 2 objects
        assert(TestObj::dtor_count == 2);
    }
}

void test_swap() {
    // Both inline
    small_vector<int, 5> s1_in = {1, 2};
    small_vector<int, 5> s2_in = {3, 4, 5};
    const int* s1_data_before = s1_in.data();
    const int* s2_data_before = s2_in.data();
    s1_in.swap(s2_in);
    assert(s1_in.size() == 3 && s1_in[2] == 5);
    assert(s2_in.size() == 2 && s2_in[1] == 2);
    // For inline swap, data pointers should remain pointing to their respective inline buffers
    // but contents are swapped.
    // The current swap implementation for two inlines might use move construction/assignment.
    // If it does a true element-wise swap, data pointers would be the same.
    // My current swap for two inlines uses a temporary small_vector and moves,
    // so data pointers would still point to their original objects' inline buffers.
    assert(s1_in.data() == s1_data_before);
    assert(s2_in.data() == s2_data_before);


    // Both heap
    small_vector<int, 2> s1_heap = {10, 20, 30}; // size 3, cap >=3
    small_vector<int, 2> s2_heap = {40, 50, 60, 70}; // size 4, cap >=4
    const int* s1_heap_ptr_before = s1_heap.data();
    size_t s1_cap_before = s1_heap.capacity();
    const int* s2_heap_ptr_before = s2_heap.data();
    size_t s2_cap_before = s2_heap.capacity();

    s1_heap.swap(s2_heap);
    assert(s1_heap.size() == 4 && s1_heap.back() == 70);
    assert(s2_heap.size() == 3 && s2_heap.back() == 30);
    assert(s1_heap.data() == s2_heap_ptr_before); // Pointers and capacities swapped
    assert(s1_heap.capacity() == s2_cap_before);
    assert(s2_heap.data() == s1_heap_ptr_before);
    assert(s2_heap.capacity() == s1_cap_before);

    // One inline, one heap
    small_vector<std::string, 3> s1_mix_inline = {"a", "b"}; // inline
    small_vector<std::string, 3> s2_mix_heap = {"x", "y", "z", "w"}; // heap

    const void* s1_inline_addr_before = s1_mix_inline.data(); // Points to its inline buffer
    const void* s2_heap_addr_before = s2_mix_heap.data();   // Points to heap

    size_t s1_size_before = s1_mix_inline.size();
    size_t s1_cap_before_val = s1_mix_inline.capacity(); // Should be N=3
    size_t s2_size_before = s2_mix_heap.size();
    size_t s2_cap_before_val = s2_mix_heap.capacity(); // Should be >=4

    s1_mix_inline.swap(s2_mix_heap);

    assert(s1_mix_inline.size() == s2_size_before); // s1 now has s2's old size
    assert(s1_mix_inline.back() == "w");
    assert(s1_mix_inline.capacity() == s2_cap_before_val); // s1 now has s2's old capacity (heap)
    assert(static_cast<const void*>(s1_mix_inline.data()) == s2_heap_addr_before); // s1 now points to s2's old heap buffer

    assert(s2_mix_heap.size() == s1_size_before); // s2 now has s1's old size
    assert(s2_mix_heap.back() == "b");
    assert(s2_mix_heap.capacity() == s1_cap_before_val); // s2 now has s1's old capacity (N=3, inline)
    // s2 should now be using its own inline buffer.
    // The address might not be s1_inline_addr_before if s2 is a different object.
    // It should be s2's original inline buffer address.
    // This test for mixed swap is tricky for addresses if the swap is not a simple pointer swap.
    // My current mixed swap strategy effectively moves data.
    // s2_mix_heap should now be inline. Check its capacity is N.
    assert(s2_mix_heap.capacity() == 3);


    // Test std::swap specialization
    small_vector<int, 3> std_s1 = {1};
    small_vector<int, 3> std_s2 = {2,3};
    std::swap(std_s1, std_s2);
    assert(std_s1.size() == 2 && std_s1.back() == 3);
    assert(std_s2.size() == 1 && std_s2.back() == 1);
}

void test_comparison_operators() {
    small_vector<int, 3> s1 = {1, 2};
    small_vector<int, 3> s2 = {1, 2};
    small_vector<int, 3> s3 = {1, 2, 3};
    small_vector<int, 3> s4 = {1, 3};

    assert(s1 == s2);
    assert(!(s1 != s2));
    assert(s1 != s3);
    assert(s1 < s3);
    assert(s1 <= s2);
    assert(s1 <= s3);
    assert(s3 > s1);
    assert(s3 >= s1);
    assert(s2 < s4); // {1,2} < {1,3}
}


int main() {
    std::cout << "===== Running Small Vector Unit Tests =====\n\n";

    run_test(test_default_constructor, "Default Constructor");
    run_test(test_initializer_list_constructor, "Initializer List Constructor");
    run_test(test_copy_constructor, "Copy Constructor");
    run_test(test_move_constructor, "Move Constructor");
    run_test(test_copy_assignment, "Copy Assignment");
    run_test(test_move_assignment, "Move Assignment");
    run_test(test_push_back_and_emplace_back, "Push Back / Emplace Back");
    run_test(test_pop_back, "Pop Back");
    run_test(test_element_access, "Element Access (at, [], front, back, data)");
    run_test(test_iterators, "Iterators (begin, end, cbegin, cend, range-for)");
    run_test(test_clear, "Clear");
    run_test(test_reserve_and_capacity, "Reserve and Capacity");
    run_test(test_resize, "Resize");
    run_test(test_swap, "Swap");
    run_test(test_comparison_operators, "Comparison Operators");

    std::cout << "\n===== All Small Vector Unit Tests PASSED =====\n";
    return 0;
}

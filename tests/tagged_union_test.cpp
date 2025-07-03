#include "../include/tagged_union.h" // Adjust path as necessary
#include <cassert>
#include <string>
#include <iostream>    // For printing test names
#include <memory>      // For std::unique_ptr
#include <utility>     // For std::move

// A simple struct for testing
struct MyStruct {
    int id;
    std::string name;

    MyStruct(int i = 0, std::string n = "") : id(i), name(std::move(n)) {}

    bool operator==(const MyStruct& other) const {
        return id == other.id && name == other.name;
    }
    // Make it non-copyable to test move-only struct storage if needed,
    // but TaggedUnion itself stores copies unless T is move-only.
    // For now, keep it copyable for simplicity in other tests.
};

// Specialization of type_name_trait for MyStruct for better tag name
template<>
struct type_name_trait<MyStruct> {
    static constexpr std::string_view tag() { return "MyStruct"; }
};

// Specialization for unique_ptr<int> for testing
template<>
struct type_name_trait<std::unique_ptr<int>> {
    static constexpr std::string_view tag() { return "std::unique_ptr<int>"; }
};

// Helper to print test names
#define RUN_TEST(test_function) \
    std::cout << "Running " << #test_function << "..." << std::endl; \
    test_function(); \
    std::cout << #test_function << " PASSED" << std::endl;

void test_default_construction() {
    TaggedUnion tu;
    assert(!tu.has_value());
    assert(tu.type_tag() == "empty"); // Assuming "empty" is the tag for valueless_t or similar
    assert(tu.get_if<int>() == nullptr);
}

void test_set_and_get_primitive() {
    TaggedUnion tu;

    tu.set(10);
    assert(tu.has_value());
    assert(tu.type_tag() == type_name_trait<int>::tag());
    assert(tu.get_if<int>() != nullptr);
    assert(*tu.get_if<int>() == 10);
    assert(tu.get_if<double>() == nullptr);

    tu.set(20.5);
    assert(tu.has_value());
    assert(tu.type_tag() == type_name_trait<double>::tag());
    assert(tu.get_if<double>() != nullptr);
    assert(*tu.get_if<double>() == 20.5);
    assert(tu.get_if<int>() == nullptr);
}

void test_set_and_get_string() {
    TaggedUnion tu;
    std::string s = "hello";
    tu.set(s); // lvalue
    assert(tu.has_value());
    assert(tu.type_tag() == type_name_trait<std::string>::tag());
    assert(tu.get_if<std::string>() != nullptr);
    assert(*tu.get_if<std::string>() == "hello");

    tu.set(std::string("world")); // rvalue
    assert(*tu.get_if<std::string>() == "world");

    const char* literal = "literal";
    tu.set(literal);
    assert(tu.has_value());
    // The tag will be for 'const char*' not 'std::string'
    // Add specialization for const char* if a specific name is desired.
    // For now, it will use typeid name.
    assert(tu.type_tag() == type_name_trait<const char*>::tag());
    assert(tu.get_if<const char*>() != nullptr);
    assert(std::string(*tu.get_if<const char*>()) == "literal");
    assert(tu.get_if<std::string>() == nullptr);
}
// Specialization for const char* is now in tagged_union.h


void test_set_and_get_custom_struct() {
    TaggedUnion tu;
    MyStruct s = {1, "test_struct"};
    tu.set(s); // lvalue copy

    assert(tu.has_value());
    assert(tu.type_tag() == type_name_trait<MyStruct>::tag());
    const MyStruct* retrieved_s = tu.get_if<MyStruct>();
    assert(retrieved_s != nullptr);
    assert(retrieved_s->id == 1);
    assert(retrieved_s->name == "test_struct");
    assert(*retrieved_s == s);
    assert(tu.get_if<int>() == nullptr);

    // Test setting with rvalue custom struct
    tu.set(MyStruct{2, "rvalue_struct"});
    assert(tu.has_value());
    assert(tu.type_tag() == type_name_trait<MyStruct>::tag());
    const MyStruct* retrieved_s_rval = tu.get_if<MyStruct>();
    assert(retrieved_s_rval != nullptr);
    assert(retrieved_s_rval->id == 2);
    assert(retrieved_s_rval->name == "rvalue_struct");
}


void test_reset() {
    TaggedUnion tu;
    tu.set(100);
    assert(tu.has_value());

    tu.reset();
    assert(!tu.has_value());
    assert(tu.type_tag() == "empty");
    assert(tu.get_if<int>() == nullptr);
}

void test_replacement() {
    TaggedUnion tu;
    tu.set(50);
    assert(*tu.get_if<int>() == 50);
    assert(tu.type_tag() == type_name_trait<int>::tag());

    tu.set(std::string("replaced"));
    assert(tu.get_if<int>() == nullptr);
    assert(tu.get_if<std::string>() != nullptr);
    assert(*tu.get_if<std::string>() == "replaced");
    assert(tu.type_tag() == type_name_trait<std::string>::tag());
}

void test_get_if_const() {
    TaggedUnion tu;
    tu.set(123);

    const TaggedUnion& ctu = tu;
    assert(ctu.has_value());
    assert(ctu.type_tag() == type_name_trait<int>::tag());
    const int* val = ctu.get_if<int>();
    assert(val != nullptr);
    assert(*val == 123);
    assert(ctu.get_if<double>() == nullptr);
}

void test_move_constructor() {
    TaggedUnion tu1;
    tu1.set(MyStruct{10, "move_test"});

    // Ensure data is there
    assert(tu1.has_value());
    assert(tu1.get_if<MyStruct>() != nullptr);
    assert(tu1.get_if<MyStruct>()->id == 10);

    TaggedUnion tu2 = std::move(tu1);

    // tu1 should be empty (or in a valid but unspecified state, typically empty for unique_ptr move)
    assert(!tu1.has_value());
    assert(tu1.get_if<MyStruct>() == nullptr);
    assert(tu1.type_tag() == "empty");


    // tu2 should have the data
    assert(tu2.has_value());
    assert(tu2.type_tag() == type_name_trait<MyStruct>::tag());
    assert(tu2.get_if<MyStruct>() != nullptr);
    assert(tu2.get_if<MyStruct>()->id == 10);
    assert(tu2.get_if<MyStruct>()->name == "move_test");
}

void test_move_assignment() {
    TaggedUnion tu1;
    tu1.set(MyStruct{20, "move_assign_test"});

    TaggedUnion tu2;
    tu2.set(12345); // Give tu2 some initial content

    assert(tu1.has_value());
    assert(tu2.has_value());
    assert(tu2.get_if<int>() != nullptr && *tu2.get_if<int>() == 12345);


    tu2 = std::move(tu1);

    // tu1 should be empty
    assert(!tu1.has_value());
    assert(tu1.get_if<MyStruct>() == nullptr);
    assert(tu1.type_tag() == "empty");

    // tu2 should have tu1's original data
    assert(tu2.has_value());
    assert(tu2.type_tag() == type_name_trait<MyStruct>::tag());
    assert(tu2.get_if<MyStruct>() != nullptr);
    assert(tu2.get_if<MyStruct>()->id == 20);
    assert(tu2.get_if<MyStruct>()->name == "move_assign_test");
    assert(tu2.get_if<int>() == nullptr); // Original tu2 data should be gone
}

void test_set_move_only_type() {
    TaggedUnion tu;
    auto u_ptr = std::make_unique<int>(42);

    // std::unique_ptr is move-only. set needs to accept T&&.
    tu.set(std::move(u_ptr));

    assert(u_ptr == nullptr); // Original unique_ptr should be nulled out after move
    assert(tu.has_value());
    assert(tu.type_tag() == type_name_trait<std::unique_ptr<int>>::tag);

    std::unique_ptr<int>* retrieved_ptr_ptr = tu.get_if<std::unique_ptr<int>>();
    assert(retrieved_ptr_ptr != nullptr);
    assert(*retrieved_ptr_ptr != nullptr); // Check the unique_ptr itself is not null
    assert(**retrieved_ptr_ptr == 42);

    // Replace with another move-only type
    auto u_ptr2 = std::make_unique<int>(100);
    tu.set(std::move(u_ptr2));
    assert(u_ptr2 == nullptr);
    assert(tu.has_value());
    retrieved_ptr_ptr = tu.get_if<std::unique_ptr<int>>();
    assert(retrieved_ptr_ptr != nullptr);
    assert(*retrieved_ptr_ptr != nullptr);
    assert(**retrieved_ptr_ptr == 100);


    // Resetting should correctly destroy the unique_ptr and the int it manages
    tu.reset();
    assert(!tu.has_value());
}

void test_self_move_assignment() {
    TaggedUnion tu;
    tu.set(MyStruct{30, "self_move"});
    MyStruct* original_ptr = tu.get_if<MyStruct>(); // Get pointer before self-assignment

    // Suppress warning about self-move, as this is exactly what we are testing
    #if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wself-move"
    #endif
    tu = std::move(tu);
    #if defined(__clang__)
    #pragma clang diagnostic pop
    #endif

    assert(tu.has_value()); // Should still have value
    assert(tu.type_tag() == type_name_trait<MyStruct>::tag());
    assert(tu.get_if<MyStruct>() != nullptr);
    // The address of the contained object might change or might not,
    // but its content should be the same.
    // The important part is that it's in a valid state and contains the original value.
    assert(tu.get_if<MyStruct>()->id == 30);
    assert(tu.get_if<MyStruct>()->name == "self_move");

    // If the internal unique_ptr was reallocated, original_ptr might be dangling.
    // However, a correct self-move (with if (this != &other)) should prevent issues.
    // Let's check if the data is still accessible and correct.
    assert(original_ptr != nullptr); // This might be risky if object was reallocated
                                    // but if (this != &other) check in operator= should make it a no-op.
    if (tu.get_if<MyStruct>() == original_ptr) { // If it's a no-op, pointer is same
        assert(original_ptr->id == 30);
    }
}


int main() {
    RUN_TEST(test_default_construction);
    RUN_TEST(test_set_and_get_primitive);
    RUN_TEST(test_set_and_get_string);
    RUN_TEST(test_set_and_get_custom_struct);
    RUN_TEST(test_reset);
    RUN_TEST(test_replacement);
    RUN_TEST(test_get_if_const);
    RUN_TEST(test_move_constructor);
    RUN_TEST(test_move_assignment);
    RUN_TEST(test_set_move_only_type);
    RUN_TEST(test_self_move_assignment);

    std::cout << "\nAll expanded tests PASSED (basic assertions)" << std::endl;
    return 0;
}

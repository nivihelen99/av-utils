#include "TypeMap.h" // Adjust path as necessary for your build system
#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <stdexcept> // For std::out_of_range
#include <memory>    // For std::shared_ptr

// A simple struct for testing
struct TestStruct {
    int id;
    std::string value;

    TestStruct(int i = 0, std::string v = "") : id(i), value(std::move(v)) {}

    bool operator==(const TestStruct& other) const {
        return id == other.id && value == other.value;
    }
     bool operator!=(const TestStruct& other) const {
        return !(*this == other);
    }
};

// Another distinct struct for testing
struct AnotherStruct {
    double data;
    AnotherStruct(double d = 0.0) : data(d) {}

    bool operator==(const AnotherStruct& other) const {
        return data == other.data;
    }
};

// Custom type for testing non-default constructible / non-copyable types (if TypeMap supports them)
class NonCopyable {
public:
    int val;
    explicit NonCopyable(int v) : val(v) {}
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&& other) noexcept : val(other.val) { other.val = 0; }
    NonCopyable& operator=(NonCopyable&& other) noexcept {
        if (this != &other) {
            val = other.val;
            other.val = 0;
        }
        return *this;
    }
    bool operator==(const NonCopyable& other) const {
        return val == other.val;
    }
};


void test_initial_state() {
    std::cout << "Running test: test_initial_state" << std::endl;
    cpp_utils::TypeMap tm;
    assert(tm.empty());
    assert(tm.size() == 0);
    std::cout << "PASS: test_initial_state" << std::endl;
}

void test_put_and_get() {
    std::cout << "Running test: test_put_and_get" << std::endl;
    cpp_utils::TypeMap tm;

    // Put and get int
    int& r1 = tm.put<int>(42);
    assert(r1 == 42);
    assert(*tm.get<int>() == 42);
    assert(tm.get_ref<int>() == 42);
    r1 = 43; // Modify through reference from put
    assert(*tm.get<int>() == 43);


    // Put and get string
    std::string& rs1 = tm.put<std::string>("hello");
    assert(rs1 == "hello");
    assert(*tm.get<std::string>() == "hello");
    assert(tm.get_ref<std::string>() == "hello");
    rs1 = "world"; // Modify through reference from put
    assert(tm.get_ref<std::string>() == "world");


    // Put and get custom struct
    TestStruct ts1{1, "one"};
    TestStruct& rts1 = tm.put(ts1); // Allow deduction
    assert(rts1 == ts1);
    assert(*tm.get<TestStruct>() == ts1);
    assert(tm.get_ref<TestStruct>() == ts1);
    rts1.value = "modified";
    assert(tm.get_ref<TestStruct>().value == "modified");


    // Put and get another custom struct
    AnotherStruct as1{3.14};
    tm.put(as1); // Allow deduction
    assert(*tm.get<AnotherStruct>() == as1);
    assert(tm.get_ref<AnotherStruct>() == as1);

    assert(tm.size() == 4);
    assert(!tm.empty());

    std::cout << "PASS: test_put_and_get" << std::endl;
}

void test_put_overwrite() {
    std::cout << "Running test: test_put_overwrite" << std::endl;
    cpp_utils::TypeMap tm;
    tm.put<int>(10);
    assert(*tm.get<int>() == 10);
    assert(tm.size() == 1);

    tm.put<int>(20); // Overwrite
    assert(*tm.get<int>() == 20);
    assert(tm.size() == 1); // Size should not change

    tm.put<std::string>("first");
    assert(tm.get_ref<std::string>() == "first");
    assert(tm.size() == 2);
    tm.put<std::string>("second");
    assert(tm.get_ref<std::string>() == "second");
    assert(tm.size() == 2);

    std::cout << "PASS: test_put_overwrite" << std::endl;
}

void test_get_non_existent() {
    std::cout << "Running test: test_get_non_existent" << std::endl;
    cpp_utils::TypeMap tm;
    assert(tm.get<int>() == nullptr);
    assert(tm.get<std::string>() == nullptr);

    bool caught = false;
    try {
        tm.get_ref<double>();
    } catch (const std::out_of_range& e) {
        caught = true;
        std::cout << "  Caught expected out_of_range: " << e.what() << std::endl;
    }
    assert(caught);

    // Check const versions
    const cpp_utils::TypeMap& ctm = tm;
    assert(ctm.get<int>() == nullptr);
    caught = false;
    try {
        ctm.get_ref<double>();
    } catch (const std::out_of_range& e) {
        caught = true;
         std::cout << "  Caught expected out_of_range (const): " << e.what() << std::endl;
    }
    assert(caught);

    std::cout << "PASS: test_get_non_existent" << std::endl;
}

void test_contains() {
    std::cout << "Running test: test_contains" << std::endl;
    cpp_utils::TypeMap tm;
    assert(!tm.contains<int>());
    assert(!tm.contains<std::string>());

    tm.put<int>(5);
    assert(tm.contains<int>());
    assert(!tm.contains<std::string>());

    tm.put<std::string>("test");
    assert(tm.contains<int>());
    assert(tm.contains<std::string>());
    assert(!tm.contains<double>());

    // Const version
    const cpp_utils::TypeMap& ctm = tm;
    assert(ctm.contains<int>());
    assert(ctm.contains<std::string>());
    assert(!ctm.contains<double>());


    std::cout << "PASS: test_contains" << std::endl;
}

void test_remove() {
    std::cout << "Running test: test_remove" << std::endl;
    cpp_utils::TypeMap tm;
    tm.put<int>(1);
    tm.put<std::string>("str");
    tm.put<TestStruct>({2, "ts"});

    assert(tm.size() == 3);
    assert(tm.contains<int>());
    assert(tm.contains<std::string>());
    assert(tm.contains<TestStruct>());

    assert(tm.remove<int>());
    assert(tm.size() == 2);
    assert(!tm.contains<int>());
    assert(tm.get<int>() == nullptr);
    assert(tm.contains<std::string>());
    assert(tm.contains<TestStruct>());

    assert(!tm.remove<double>()); // Remove non-existent
    assert(tm.size() == 2);

    assert(tm.remove<TestStruct>());
    assert(tm.size() == 1);
    assert(!tm.contains<TestStruct>());
    assert(tm.get<TestStruct>() == nullptr);
    assert(tm.contains<std::string>());

    assert(tm.remove<std::string>());
    assert(tm.size() == 0);
    assert(tm.empty());
    assert(!tm.contains<std::string>());
    assert(tm.get<std::string>() == nullptr);

    assert(!tm.remove<int>()); // Remove already removed

    std::cout << "PASS: test_remove" << std::endl;
}

void test_clear_and_empty() {
    std::cout << "Running test: test_clear_and_empty" << std::endl;
    cpp_utils::TypeMap tm;
    assert(tm.empty());
    assert(tm.size() == 0);

    tm.put<int>(10);
    tm.put<char>('a');
    assert(!tm.empty());
    assert(tm.size() == 2);

    tm.clear();
    assert(tm.empty());
    assert(tm.size() == 0);
    assert(!tm.contains<int>());
    assert(!tm.contains<char>());
    assert(tm.get<int>() == nullptr);

    // Clear an already empty map
    tm.clear();
    assert(tm.empty());
    assert(tm.size() == 0);

    std::cout << "PASS: test_clear_and_empty" << std::endl;
}

void test_const_correctness() {
    std::cout << "Running test: test_const_correctness" << std::endl;
    cpp_utils::TypeMap tm;
    tm.put<int>(123);
    tm.put<std::string>("const_test");
    tm.put<TestStruct>({7, "seven"});

    const cpp_utils::TypeMap& ctm = tm;

    assert(ctm.size() == 3);
    assert(!ctm.empty());

    // Test const get()
    assert(*ctm.get<int>() == 123);
    assert(ctm.get_ref<std::string>() == "const_test");
    assert(ctm.get<double>() == nullptr);

    bool caught = false;
    try {
        ctm.get_ref<float>();
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);

    // Test const contains()
    assert(ctm.contains<int>());
    assert(ctm.contains<TestStruct>());
    assert(!ctm.contains<char>());

    // Test const get_ref()
    const TestStruct& ts_ref = ctm.get_ref<TestStruct>();
    assert(ts_ref.id == 7 && ts_ref.value == "seven");

    // The following would/should fail to compile if uncommented:
    // ctm.put<float>(3.14f);
    // ctm.remove<int>();
    // ctm.clear();
    // int* val = ctm.get<int>(); *val = 10; // get returns const T*
    // ctm.get_ref<int>() = 10; // get_ref returns const T&

    std::cout << "PASS: test_const_correctness" << std::endl;
}

void test_move_only_types() {
    std::cout << "Running test: test_move_only_types" << std::endl;
    cpp_utils::TypeMap tm;

    // Test with std::unique_ptr
    tm.put(std::make_unique<int>(100)); // Allow deduction, pass rvalue
    assert(tm.contains<std::unique_ptr<int>>());
    std::unique_ptr<int>* ptr_to_uptr = tm.get<std::unique_ptr<int>>();
    assert(ptr_to_uptr != nullptr);
    assert(**ptr_to_uptr == 100);

    // Overwrite
    tm.put(std::make_unique<int>(200)); // Allow deduction, pass rvalue
    ptr_to_uptr = tm.get<std::unique_ptr<int>>();
    assert(**ptr_to_uptr == 200);

    // Test with custom NonCopyable type
    tm.put(NonCopyable(50)); // Allow deduction, pass rvalue
    assert(tm.contains<NonCopyable>());
    NonCopyable* nc_ptr = tm.get<NonCopyable>();
    assert(nc_ptr != nullptr);
    assert(nc_ptr->val == 50);

    // Retrieve and move out (if TypeMap stored T and not T*, this would be more complex)
    // Current TypeMap stores T directly in std::any.
    // If we want to move out, we'd need a special method.
    // Standard 'get' gives pointer/ref to internal object.
    // Let's test replacing it.
    tm.put(NonCopyable(60)); // Allow deduction, pass rvalue
    nc_ptr = tm.get<NonCopyable>();
    assert(nc_ptr->val == 60);

    assert(tm.remove<std::unique_ptr<int>>());
    assert(!tm.contains<std::unique_ptr<int>>());

    assert(tm.remove<NonCopyable>());
    assert(!tm.contains<NonCopyable>());

    std::cout << "PASS: test_move_only_types" << std::endl;
}


void test_shared_ptr_storage() {
    std::cout << "Running test: test_shared_ptr_storage" << std::endl;
    cpp_utils::TypeMap tm;

    auto sptr1 = std::make_shared<TestStruct>(10, "sptr_test");
    tm.put(sptr1); // Allow deduction

    assert(tm.contains<std::shared_ptr<TestStruct>>());

    // Retrieve the shared_ptr
    std::shared_ptr<TestStruct>* retrieved_sptr_ptr = tm.get<std::shared_ptr<TestStruct>>();
    assert(retrieved_sptr_ptr != nullptr);
    assert(retrieved_sptr_ptr->get() == sptr1.get()); // Ensure it's the same managed object
    assert((*retrieved_sptr_ptr)->id == 10);
    assert(sptr1.use_count() == 2); // One in main, one in TypeMap

    // Modify through original, check in map
    sptr1->value = "modified_sptr";
    assert((*tm.get<std::shared_ptr<TestStruct>>())->value == "modified_sptr");

    // Replace with a new shared_ptr
    auto sptr2 = std::make_shared<TestStruct>(20, "sptr_test2");
    tm.put(sptr2); // Allow deduction
    assert(sptr1.use_count() == 1); // sptr1 is no longer in map
    assert(sptr2.use_count() == 2); // sptr2 is now in map

    retrieved_sptr_ptr = tm.get<std::shared_ptr<TestStruct>>();
    assert(retrieved_sptr_ptr->get() == sptr2.get());
    assert((*retrieved_sptr_ptr)->id == 20);

    tm.remove<std::shared_ptr<TestStruct>>();
    assert(sptr2.use_count() == 1);
    assert(!tm.contains<std::shared_ptr<TestStruct>>());

    std::cout << "PASS: test_shared_ptr_storage" << std::endl;
}


int main() {
    std::cout << "--- Starting TypeMap Tests ---" << std::endl;

    test_initial_state();
    test_put_and_get();
    test_put_overwrite();
    test_get_non_existent();
    test_contains();
    test_remove();
    test_clear_and_empty();
    test_const_correctness();
    test_move_only_types();
    test_shared_ptr_storage();

    std::cout << "\n--- All TypeMap Tests Passed ---" << std::endl;
    return 0;
}

#include "gtest/gtest.h"
#include "expected.h" // Assuming expected.h is accessible via include paths
#include <string>
#include <utility> // For std::move
#include <vector>  // For potential complex types for T or E

// --- Unexpected Tests ---
TEST(UnexpectedTest, ConstructionAndAccess) {
    aos_utils::Unexpected<std::string> unex_str("error message");
    EXPECT_EQ(unex_str.error(), "error message");

    const auto& unex_str_const = unex_str;
    EXPECT_EQ(unex_str_const.error(), "error message");

    aos_utils::Unexpected<int> unex_int(123);
    EXPECT_EQ(unex_int.error(), 123);

    // Test rvalue access for error()
    EXPECT_EQ(aos_utils::Unexpected<std::string>("rvalue_error").error(), "rvalue_error");
    const aos_utils::Unexpected<std::string> const_unex_rval("const_rvalue_error");
    EXPECT_EQ(std::move(const_unex_rval).error(), "const_rvalue_error"); // const E&& error() const&&
}

TEST(UnexpectedTest, MoveConstruction) {
    aos_utils::Unexpected<std::string> unex_str_move_src("move me");
    aos_utils::Unexpected<std::string> unex_str_move_dest = std::move(unex_str_move_src);
    EXPECT_EQ(unex_str_move_dest.error(), "move me");
    // Note: The state of unex_str_move_src.error() is valid but unspecified for std::string.
    // For fundamental types, it would be a copy.
}

TEST(UnexpectedTest, Comparison) {
    aos_utils::Unexpected<std::string> unex1("error");
    aos_utils::Unexpected<std::string> unex2("error");
    aos_utils::Unexpected<std::string> unex3("different");

    EXPECT_EQ(unex1, unex2);
    EXPECT_NE(unex1, unex3);
    EXPECT_TRUE(unex1 != unex3);
    EXPECT_FALSE(unex1 == unex3);
}

// --- Expected Tests: Constructors and Basic Observers ---
namespace { // Anonymous namespace for helper types/structs

struct DefaultConstructible {
    int x;
    DefaultConstructible() : x(10) {} // Default value
    bool operator==(const DefaultConstructible& other) const { return x == other.x; }
};

struct NonDefaultConstructible {
    int x;
    NonDefaultConstructible(int val) : x(val) {}
    bool operator==(const NonDefaultConstructible& other) const { return x == other.x; }
};

// Custom error type for testing
struct CustomError {
    int code;
    std::string msg;
    CustomError(int c, std::string m) : code(c), msg(std::move(m)) {}
    // Constructor for make_expected test case
    explicit CustomError(const char* m) : code(500), msg(m) {}
    bool operator==(const CustomError& other) const { return code == other.code && msg == other.msg; }
};

} // anonymous namespace

TEST(ExpectedTest, DefaultConstruction) {
    aos_utils::Expected<int, std::string> e_int;
    EXPECT_TRUE(e_int.has_value());
    EXPECT_TRUE(static_cast<bool>(e_int));
    EXPECT_EQ(*e_int, 0);

    aos_utils::Expected<DefaultConstructible, std::string> e_custom;
    EXPECT_TRUE(e_custom.has_value());
    EXPECT_EQ(e_custom.value().x, 10);
}

TEST(ExpectedTest, ValueConstruction) {
    aos_utils::Expected<int, std::string> e_val(42);
    EXPECT_TRUE(e_val.has_value());
    EXPECT_EQ(e_val.value(), 42);
    EXPECT_EQ(*e_val, 42);

    std::string s = "hello";
    aos_utils::Expected<std::string, int> e_str(s);
    EXPECT_TRUE(e_str.has_value());
    EXPECT_EQ(*e_str, "hello");

    aos_utils::Expected<std::string, int> e_str_rvalue(std::string("rvalue_hello"));
    EXPECT_TRUE(e_str_rvalue.has_value());
    EXPECT_EQ(*e_str_rvalue, "rvalue_hello");

    NonDefaultConstructible ndc(100);
    aos_utils::Expected<NonDefaultConstructible, std::string> e_ndc(ndc);
    EXPECT_TRUE(e_ndc.has_value());
    EXPECT_EQ(e_ndc.value().x, 100);
}

TEST(ExpectedTest, ErrorConstructionFromUnexpected) {
    aos_utils::Unexpected<std::string> unex_str("network error");
    aos_utils::Expected<int, std::string> e_err_lval(unex_str);
    EXPECT_FALSE(e_err_lval.has_value());
    EXPECT_FALSE(static_cast<bool>(e_err_lval));
    EXPECT_EQ(e_err_lval.error(), "network error");

    aos_utils::Expected<int, std::string> e_err_rval(aos_utils::make_unexpected("rvalue network error"));
    EXPECT_FALSE(e_err_rval.has_value());
    EXPECT_EQ(e_err_rval.error(), "rvalue network error");

    aos_utils::Unexpected<CustomError> unex_custom(CustomError(1, "custom"));
    aos_utils::Expected<int, CustomError> e_custom_err(unex_custom);
    EXPECT_FALSE(e_custom_err.has_value());
    EXPECT_EQ(e_custom_err.error().code, 1);
    EXPECT_EQ(e_custom_err.error().msg, "custom");
}

TEST(ExpectedTest, ErrorConstructionWithConvertibleErrorType) {
    // Test Expected(const Unexpected<G>&) where G is convertible to E
    aos_utils::Unexpected<int> unex_int(404); // G is int
    aos_utils::Expected<double, long> e_long_err(unex_int); // E is long. int is convertible to long.
    EXPECT_FALSE(e_long_err.has_value());
    ASSERT_FALSE(e_long_err.has_value()); // Use ASSERT if further checks depend on this
    EXPECT_EQ(e_long_err.error(), 404L);
}


TEST(ExpectedTest, InPlaceValueConstruction) {
    aos_utils::Expected<std::string, int> e_str(std::in_place, "in-place value");
    EXPECT_TRUE(e_str.has_value());
    EXPECT_EQ(*e_str, "in-place value");

    aos_utils::Expected<std::pair<int, double>, std::string> e_pair(std::in_place, 10, 3.14);
    EXPECT_TRUE(e_pair.has_value());
    ASSERT_TRUE(e_pair.has_value());
    EXPECT_EQ(e_pair->first, 10);
    EXPECT_DOUBLE_EQ(e_pair->second, 3.14);

    aos_utils::Expected<NonDefaultConstructible, std::string> e_ndc(std::in_place, 200);
    EXPECT_TRUE(e_ndc.has_value());
    EXPECT_EQ(e_ndc->x, 200);
}

TEST(ExpectedTest, InPlaceErrorConstruction) {
    aos_utils::Expected<int, std::string> e_str_err(std::in_place_type_t<aos_utils::Unexpected<std::string>>{}, "in-place error");
    EXPECT_FALSE(e_str_err.has_value());
    ASSERT_FALSE(e_str_err.has_value());
    EXPECT_EQ(e_str_err.error(), "in-place error");

    aos_utils::Expected<int, CustomError> e_custom_err(std::in_place_type_t<aos_utils::Unexpected<CustomError>>{}, 2, "custom in-place");
    EXPECT_FALSE(e_custom_err.has_value());
    ASSERT_FALSE(e_custom_err.has_value());
    EXPECT_EQ(e_custom_err.error().code, 2);
    EXPECT_EQ(e_custom_err.error().msg, "custom in-place");
}

TEST(ExpectedTest, CopyConstruction) {
    aos_utils::Expected<std::string, CustomError> val_orig("value_for_copy");
    aos_utils::Expected<std::string, CustomError> val_copy = val_orig;
    EXPECT_TRUE(val_copy.has_value());
    ASSERT_TRUE(val_copy.has_value());
    EXPECT_EQ(*val_copy, "value_for_copy");
    EXPECT_TRUE(val_orig.has_value()); // Original unchanged
    ASSERT_TRUE(val_orig.has_value());
    EXPECT_EQ(*val_orig, "value_for_copy");

    aos_utils::Expected<std::string, CustomError> err_orig(aos_utils::make_unexpected(CustomError(3, "error_for_copy")));
    aos_utils::Expected<std::string, CustomError> err_copy = err_orig;
    EXPECT_FALSE(err_copy.has_value());
    ASSERT_FALSE(err_copy.has_value());
    EXPECT_EQ(err_copy.error().code, 3);
    EXPECT_EQ(err_copy.error().msg, "error_for_copy");
    EXPECT_FALSE(err_orig.has_value()); // Original unchanged
    ASSERT_FALSE(err_orig.has_value());
    EXPECT_EQ(err_orig.error().msg, "error_for_copy");
}

namespace { // For move-only tests
struct MoveOnlyType {
    int id;
    std::string data;
    explicit MoveOnlyType(int i, std::string d = "default") : id(i), data(std::move(d)) {}
    MoveOnlyType(const MoveOnlyType&) = delete;
    MoveOnlyType& operator=(const MoveOnlyType&) = delete;
    MoveOnlyType(MoveOnlyType&& other) noexcept : id(other.id), data(std::move(other.data)) { other.id = 0; }
    MoveOnlyType& operator=(MoveOnlyType&& other) noexcept {
        if (this != &other) {
            id = other.id;
            data = std::move(other.data);
            other.id = 0;
        }
        return *this;
    }
    bool operator==(const MoveOnlyType& other) const { return id == other.id && data == other.data; }
};
} // anonymous namespace

TEST(ExpectedTest, MoveConstructionWithValue) {
    aos_utils::Expected<MoveOnlyType, std::string> val_orig(std::in_place, 123, "move_me_value");
    aos_utils::Expected<MoveOnlyType, std::string> val_moved = std::move(val_orig);

    EXPECT_TRUE(val_moved.has_value());
    ASSERT_TRUE(val_moved.has_value());
    EXPECT_EQ(val_moved->id, 123);
    EXPECT_EQ(val_moved->data, "move_me_value");

    // Check original after move: T is in a valid but unspecified state.
    // For this MoveOnlyType, we defined it to set id to 0.
    EXPECT_TRUE(val_orig.has_value()); // std::variant holds the moved-from object
    ASSERT_TRUE(val_orig.has_value());
    EXPECT_EQ(val_orig->id, 0);
    EXPECT_TRUE(val_orig->data.empty()); // Assuming std::string move ctor leaves source empty
}

TEST(ExpectedTest, MoveConstructionWithError) {
    aos_utils::Expected<int, MoveOnlyType> err_orig(std::in_place_type_t<aos_utils::Unexpected<MoveOnlyType>>{}, 456, "move_me_error");
    aos_utils::Expected<int, MoveOnlyType> err_moved = std::move(err_orig);

    EXPECT_FALSE(err_moved.has_value());
    ASSERT_FALSE(err_moved.has_value());
    EXPECT_EQ(err_moved.error().id, 456);
    EXPECT_EQ(err_moved.error().data, "move_me_error");

    // Check original after move: E is in a valid but unspecified state.
    EXPECT_FALSE(err_orig.has_value());
    ASSERT_FALSE(err_orig.has_value());
    EXPECT_EQ(err_orig.error().id, 0);
    EXPECT_TRUE(err_orig.error().data.empty());
}

// --- Expected Tests: Assignment Operators ---

TEST(ExpectedTest, CopyAssignment) {
    aos_utils::Expected<std::string, int> val_orig("original_value");
    aos_utils::Expected<std::string, int> err_orig(aos_utils::make_unexpected(100));

    aos_utils::Expected<std::string, int> target_val("target_initial_value");
    aos_utils::Expected<std::string, int> target_err(aos_utils::make_unexpected(200));

    // Value to Value
    target_val = val_orig;
    EXPECT_TRUE(target_val.has_value());
    EXPECT_EQ(*target_val, "original_value");
    EXPECT_TRUE(val_orig.has_value()); // Source unchanged
    EXPECT_EQ(*val_orig, "original_value");

    // Error to Error
    target_err = err_orig;
    EXPECT_FALSE(target_err.has_value());
    EXPECT_EQ(target_err.error(), 100);
    EXPECT_FALSE(err_orig.has_value()); // Source unchanged
    EXPECT_EQ(err_orig.error(), 100);

    // Value to Error
    target_err = val_orig;
    EXPECT_TRUE(target_err.has_value());
    EXPECT_EQ(*target_err, "original_value");

    // Error to Value
    target_val = err_orig;
    EXPECT_FALSE(target_val.has_value());
    EXPECT_EQ(target_val.error(), 100);
}

TEST(ExpectedTest, MoveAssignment) {
    // Value to Value
    aos_utils::Expected<MoveOnlyType, std::string> val_source(std::in_place, 1, "src_val");
    aos_utils::Expected<MoveOnlyType, std::string> val_target(std::in_place, 2, "tgt_val");
    val_target = std::move(val_source);
    EXPECT_TRUE(val_target.has_value());
    EXPECT_EQ(val_target->id, 1);
    EXPECT_EQ(val_target->data, "src_val");
    EXPECT_TRUE(val_source.has_value()); // Source is valid but moved-from
    EXPECT_EQ(val_source->id, 0);        // As per our MoveOnlyType

    // Error to Error
    aos_utils::Expected<int, MoveOnlyType> err_source(std::in_place_type_t<aos_utils::Unexpected<MoveOnlyType>>{}, 3, "src_err");
    aos_utils::Expected<int, MoveOnlyType> err_target(std::in_place_type_t<aos_utils::Unexpected<MoveOnlyType>>{}, 4, "tgt_err");
    err_target = std::move(err_source);
    EXPECT_FALSE(err_target.has_value());
    EXPECT_EQ(err_target.error().id, 3);
    EXPECT_EQ(err_target.error().data, "src_err");
    EXPECT_FALSE(err_source.has_value()); // Source is valid but moved-from
    EXPECT_EQ(err_source.error().id, 0);   // As per our MoveOnlyType

    // Value to Error
    aos_utils::Expected<MoveOnlyType, std::string> val_source2(std::in_place, 5, "src_val2");
    aos_utils::Expected<MoveOnlyType, std::string> err_target2(aos_utils::make_unexpected<std::string>("initial_error"));
    err_target2 = std::move(val_source2);
    EXPECT_TRUE(err_target2.has_value());
    EXPECT_EQ(err_target2->id, 5);
    EXPECT_TRUE(val_source2.has_value());
    EXPECT_EQ(val_source2->id, 0);

    // Error to Value
    aos_utils::Expected<int, MoveOnlyType> err_source2(std::in_place_type_t<aos_utils::Unexpected<MoveOnlyType>>{}, 6, "src_err2");
    aos_utils::Expected<int, MoveOnlyType> val_target2(std::in_place, 7);
    val_target2 = std::move(err_source2);
    EXPECT_FALSE(val_target2.has_value());
    EXPECT_EQ(val_target2.error().id, 6);
    EXPECT_FALSE(err_source2.has_value());
    EXPECT_EQ(err_source2.error().id, 0);
}

TEST(ExpectedTest, ValueAssignmentOperator) {
    aos_utils::Expected<int, std::string> e(10);
    EXPECT_TRUE(e.has_value());
    EXPECT_EQ(*e, 10);

    e = 20;
    EXPECT_TRUE(e.has_value());
    EXPECT_EQ(*e, 20);

    // Assign value to an error state
    aos_utils::Expected<int, std::string> e_err(aos_utils::make_unexpected("error"));
    EXPECT_FALSE(e_err.has_value());
    e_err = 30;
    EXPECT_TRUE(e_err.has_value());
    EXPECT_EQ(*e_err, 30);
}

TEST(ExpectedTest, UnexpectedAssignmentOperator) {
    aos_utils::Expected<int, std::string> e(10);
    EXPECT_TRUE(e.has_value());

    e = aos_utils::make_unexpected("new error");
    EXPECT_FALSE(e.has_value());
    EXPECT_EQ(e.error(), "new error");

    // Assign error to an error state
    aos_utils::Expected<int, std::string> e_err(aos_utils::make_unexpected("original error"));
    EXPECT_FALSE(e_err.has_value());
    e_err = aos_utils::make_unexpected("another error");
    EXPECT_FALSE(e_err.has_value());
    EXPECT_EQ(e_err.error(), "another error");
}

// --- Expected Tests: Accessors ---

TEST(ExpectedTest, ValueAccessor) {
    aos_utils::Expected<std::string, int> e_val("hello");
    EXPECT_EQ(e_val.value(), "hello");
    *e_val = "world"; // Test non-const operator*
    EXPECT_EQ(e_val.value(), "world");

    const aos_utils::Expected<std::string, int> ce_val("const_hello");
    EXPECT_EQ(ce_val.value(), "const_hello");
    EXPECT_EQ(*ce_val, "const_hello"); // Test const operator*

    aos_utils::Expected<std::string, int> e_val_rval("rval_hello");
    EXPECT_EQ(std::move(e_val_rval).value(), "rval_hello");
    // After move, e_val_rval is in a valid but unspecified state. Reinitialize for next test.
    e_val_rval = "reinit";
    EXPECT_EQ(std::move(static_cast<const aos_utils::Expected<std::string, int>&>(e_val_rval)).value(), "reinit");


    aos_utils::Expected<int, std::string> e_err(aos_utils::make_unexpected("access error"));
    EXPECT_THROW(e_err.value(), std::bad_variant_access);

    const aos_utils::Expected<int, std::string> ce_err(aos_utils::make_unexpected("const access error"));
    EXPECT_THROW(ce_err.value(), std::bad_variant_access);
}

TEST(ExpectedTest, ErrorAccessor) {
    aos_utils::Expected<int, std::string> e_err(aos_utils::make_unexpected("error msg"));
    EXPECT_EQ(e_err.error(), "error msg");
    e_err.error() = "new error msg"; // Test non-const error()
    EXPECT_EQ(e_err.error(), "new error msg");

    const aos_utils::Expected<int, std::string> ce_err(aos_utils::make_unexpected("const error msg"));
    EXPECT_EQ(ce_err.error(), "const error msg");

    aos_utils::Expected<int, std::string> e_err_rval(aos_utils::make_unexpected("rvalue error"));
    EXPECT_EQ(std::move(e_err_rval).error(), "rvalue error");
    // After move, e_err_rval is in a valid but unspecified state. Reinitialize for next test.
    e_err_rval = aos_utils::make_unexpected("reinit_error");
    EXPECT_EQ(std::move(static_cast<const aos_utils::Expected<int, std::string>&>(e_err_rval)).error(), "reinit_error");

    aos_utils::Expected<std::string, int> e_val("value");
    EXPECT_THROW(e_val.error(), std::bad_variant_access);

    const aos_utils::Expected<std::string, int> ce_val("const_value");
    EXPECT_THROW(ce_val.error(), std::bad_variant_access);
}

TEST(ExpectedTest, DereferenceOperator) {
    aos_utils::Expected<std::string, int> e_val("data");
    EXPECT_EQ(*e_val, "data");
    (*e_val).append("_appended");
    EXPECT_EQ(*e_val, "data_appended");

    const aos_utils::Expected<std::string, int> ce_val("const_data");
    EXPECT_EQ(*ce_val, "const_data");
    // *ce_val = "new_const_data"; // Should fail compilation if uncommented
}

TEST(ExpectedTest, ArrowOperator) {
    struct MyStruct { std::string s; int i; };
    aos_utils::Expected<MyStruct, std::string> e_val(std::in_place, "struct_data", 42);
    EXPECT_EQ(e_val->s, "struct_data");
    EXPECT_EQ(e_val->i, 42);
    e_val->s = "new_struct_data";
    EXPECT_EQ(e_val->s, "new_struct_data");

    const aos_utils::Expected<MyStruct, std::string> ce_val(std::in_place, "const_struct_data", 100);
    EXPECT_EQ(ce_val->s, "const_struct_data");
    EXPECT_EQ(ce_val->i, 100);
    // ce_val->s = "new_data"; // Should fail compilation
}

TEST(ExpectedTest, ValueOr) {
    aos_utils::Expected<int, std::string> e_val(123);
    EXPECT_EQ(e_val.value_or(456), 123);
    EXPECT_EQ(std::move(e_val).value_or(456), 123); // Original e_val is now moved from.

    aos_utils::Expected<int, std::string> e_err(aos_utils::make_unexpected("error"));
    EXPECT_EQ(e_err.value_or(789), 789);
    EXPECT_EQ(std::move(e_err).value_or(789), 789); // Original e_err is now moved from.

    aos_utils::Expected<std::string, int> e_str_val("hello");
    EXPECT_EQ(e_str_val.value_or("world"), "hello");

    // Test rvalue overload for value_or
    aos_utils::Expected<MoveOnlyType, std::string> mot_val(std::in_place, 1, "mot_val");
    MoveOnlyType mot_default(0, "default");
    MoveOnlyType result_mot = std::move(mot_val).value_or(std::move(mot_default));
    EXPECT_EQ(result_mot.id, 1);
    EXPECT_EQ(result_mot.data, "mot_val");
    // mot_val is moved from, mot_default is potentially moved from.

    aos_utils::Expected<MoveOnlyType, std::string> mot_err(aos_utils::make_unexpected<std::string>("err"));
    MoveOnlyType mot_default2(2, "default2");
    MoveOnlyType result_mot_err = std::move(mot_err).value_or(std::move(mot_default2));
    EXPECT_EQ(result_mot_err.id, 2);
    EXPECT_EQ(result_mot_err.data, "default2");
}

// --- Expected Tests: Monadic Operations ---

// Helper functions for monadic tests
struct MonadicOpsHelpers {
    static int times_two(int x) { return x * 2; }
    static std::string to_str(int x) { return std::to_string(x); }
    static aos_utils::Expected<int, std::string> times_three_expected(int x) { return x * 3; }
    static aos_utils::Expected<int, std::string> always_error_expected(int) { return aos_utils::make_unexpected<std::string>("always_error"); }

    static aos_utils::Expected<std::string, std::string> recover_with_value(const std::string& err_msg) {
        return "recovered_from_" + err_msg;
    }
    static aos_utils::Expected<std::string, std::string> recover_with_new_error(const std::string& /*err_msg*/) {
        return aos_utils::make_unexpected<std::string>("new_recovery_error");
    }
     static aos_utils::Expected<int, std::string> recover_int_with_value(const std::string& /*err_msg*/) {
        return 0; // Default recovery value
    }

    static std::string transform_error(const std::string& err_msg) { return "transformed_" + err_msg; }
    static CustomError transform_to_custom_error(const std::string& err_msg) { return CustomError(99, "custom_" + err_msg); }

    static void void_func(int) { /* do nothing */ }
};

TEST(ExpectedMonadicTest, Map) {
    aos_utils::Expected<int, std::string> e_val(10);
    aos_utils::Expected<int, std::string> e_err(aos_utils::make_unexpected<std::string>("map_error"));

    // Map on value (const&)
    auto mapped_val = e_val.map(MonadicOpsHelpers::times_two);
    EXPECT_TRUE(mapped_val.has_value());
    EXPECT_EQ(*mapped_val, 20);
    EXPECT_TRUE(e_val.has_value()); // Original unchanged
    EXPECT_EQ(*e_val, 10);

    // Map on error (const&)
    auto mapped_err = e_err.map(MonadicOpsHelpers::times_two);
    EXPECT_FALSE(mapped_err.has_value());
    EXPECT_EQ(mapped_err.error(), "map_error");

    // Map on value (&&)
    auto mapped_val_rval = std::move(e_val).map(MonadicOpsHelpers::to_str);
    EXPECT_TRUE(mapped_val_rval.has_value());
    EXPECT_EQ(*mapped_val_rval, "10");
    // e_val is now moved from.

    // Map on error (&&)
    aos_utils::Expected<int, std::string> e_err_rval_src(aos_utils::make_unexpected<std::string>("map_error_rval"));
    auto mapped_err_rval = std::move(e_err_rval_src).map(MonadicOpsHelpers::times_two);
    EXPECT_FALSE(mapped_err_rval.has_value());
    EXPECT_EQ(mapped_err_rval.error(), "map_error_rval");

    // Map to void
    aos_utils::Expected<int, std::string> e_val_for_void(5);
    auto map_void_res = e_val_for_void.map(MonadicOpsHelpers::void_func);
    EXPECT_TRUE(map_void_res.has_value()); // Expected<void, E> still "has a value" if no error
    // Cannot dereference Expected<void,E>::value()

    aos_utils::Expected<int, std::string> e_err_for_void(aos_utils::make_unexpected<std::string>("void_map_err"));
    auto map_void_err_res = e_err_for_void.map(MonadicOpsHelpers::void_func);
    EXPECT_FALSE(map_void_err_res.has_value());
    EXPECT_EQ(map_void_err_res.error(), "void_map_err");
}

TEST(ExpectedMonadicTest, AndThen) {
    aos_utils::Expected<int, std::string> e_val(5);
    aos_utils::Expected<int, std::string> e_err(aos_utils::make_unexpected<std::string>("and_then_error"));

    // AndThen on value, function returns value (const&)
    auto then_val_val = e_val.and_then(MonadicOpsHelpers::times_three_expected);
    EXPECT_TRUE(then_val_val.has_value());
    EXPECT_EQ(*then_val_val, 15);
    EXPECT_TRUE(e_val.has_value()); // Original unchanged
    EXPECT_EQ(*e_val, 5);

    // AndThen on value, function returns error (const&)
    auto then_val_err = e_val.and_then(MonadicOpsHelpers::always_error_expected);
    EXPECT_FALSE(then_val_err.has_value());
    EXPECT_EQ(then_val_err.error(), "always_error");

    // AndThen on error (const&)
    auto then_err = e_err.and_then(MonadicOpsHelpers::times_three_expected);
    EXPECT_FALSE(then_err.has_value());
    EXPECT_EQ(then_err.error(), "and_then_error");

    // AndThen on value, function returns value (&&)
    auto then_val_val_rval = std::move(e_val).and_then(MonadicOpsHelpers::times_three_expected);
    EXPECT_TRUE(then_val_val_rval.has_value());
    EXPECT_EQ(*then_val_val_rval, 15);
    // e_val is moved from

    // AndThen on error (&&)
    aos_utils::Expected<int, std::string> e_err_rval_src(aos_utils::make_unexpected<std::string>("and_then_error_rval"));
    auto then_err_rval = std::move(e_err_rval_src).and_then(MonadicOpsHelpers::times_three_expected);
    EXPECT_FALSE(then_err_rval.has_value());
    EXPECT_EQ(then_err_rval.error(), "and_then_error_rval");
}

TEST(ExpectedMonadicTest, OrElse) {
    aos_utils::Expected<std::string, std::string> e_val("original_value"); // Type T must be constructible for NextExpected(T)
    aos_utils::Expected<std::string, std::string> e_err(aos_utils::make_unexpected<std::string>("or_else_error"));

    // OrElse on value (const&) - recovery function not called
    // fallback_handler returns Expected<std::string, std::string>
    // static_assert checks if Expected<std::string, std::string> can be constructed from const std::string&
    auto else_val = e_val.or_else(MonadicOpsHelpers::recover_with_value);
    EXPECT_TRUE(else_val.has_value());
    EXPECT_EQ(*else_val, "original_value");
    EXPECT_TRUE(e_val.has_value()); // Original unchanged
    EXPECT_EQ(*e_val, "original_value");

    // OrElse on error, recovery returns value (const&)
    auto else_err_val = e_err.or_else(MonadicOpsHelpers::recover_with_value);
    EXPECT_TRUE(else_err_val.has_value());
    EXPECT_EQ(*else_err_val, "recovered_from_or_else_error");

    // OrElse on error, recovery returns new error (const&)
    auto else_err_err = e_err.or_else(MonadicOpsHelpers::recover_with_new_error);
    EXPECT_FALSE(else_err_err.has_value());
    EXPECT_EQ(else_err_err.error(), "new_recovery_error");

    // OrElse on value (&&)
    aos_utils::Expected<std::string, std::string> e_val_rval_src("rval_original_value");
    auto else_val_rval = std::move(e_val_rval_src).or_else(MonadicOpsHelpers::recover_with_value);
    EXPECT_TRUE(else_val_rval.has_value());
    EXPECT_EQ(*else_val_rval, "rval_original_value");
    // e_val_rval_src is moved from

    // OrElse on error (&&), recovery returns value
    aos_utils::Expected<std::string, std::string> e_err_rval_src(aos_utils::make_unexpected<std::string>("or_else_error_rval"));
    auto else_err_rval_val = std::move(e_err_rval_src).or_else(MonadicOpsHelpers::recover_with_value);
    EXPECT_TRUE(else_err_rval_val.has_value());
    EXPECT_EQ(*else_err_rval_val, "recovered_from_or_else_error_rval");

    // Test case from original issue (adapted)
    // parse_int("invalid") -> Expected<int, std::string>(unexpected("Invalid integer: invalid"))
    // .or_else(new_fallback_handler) -> new_fallback_handler returns Expected<int, std::string>(0)
    // .map([](int x) { return x + 100; }) -> Expected<int, std::string>(100)
    aos_utils::Expected<int, std::string> parse_err_res(aos_utils::make_unexpected<std::string>("Invalid integer: invalid"));
    auto recovered_chain = parse_err_res.or_else(MonadicOpsHelpers::recover_int_with_value)
                                       .map([](int x){ return x + 100; });
    EXPECT_TRUE(recovered_chain.has_value());
    EXPECT_EQ(recovered_chain.value(), 100);
}

TEST(ExpectedMonadicTest, MapError) {
    aos_utils::Expected<int, std::string> e_val(123);
    aos_utils::Expected<int, std::string> e_err(aos_utils::make_unexpected<std::string>("map_this_error"));

    // MapError on value (const&) - error function not called
    auto map_err_on_val = e_val.map_error(MonadicOpsHelpers::transform_error);
    EXPECT_TRUE(map_err_on_val.has_value());
    EXPECT_EQ(*map_err_on_val, 123);
    EXPECT_EQ(e_val.error_type{}, std::string{}); // Check original error type remains (if needed for type checks)
    EXPECT_EQ(map_err_on_val.error_type{}, MonadicOpsHelpers::transform_error(std::string{})); // Check new error type

    // MapError on error (const&)
    auto map_err_on_err = e_err.map_error(MonadicOpsHelpers::transform_error);
    EXPECT_FALSE(map_err_on_err.has_value());
    EXPECT_EQ(map_err_on_err.error(), "transformed_map_this_error");

    // MapError on value (&&)
    auto map_err_on_val_rval = std::move(e_val).map_error(MonadicOpsHelpers::transform_error);
    EXPECT_TRUE(map_err_on_val_rval.has_value());
    EXPECT_EQ(*map_err_on_val_rval, 123);
    // e_val is moved from

    // MapError on error (&&)
    aos_utils::Expected<int, std::string> e_err_rval_src(aos_utils::make_unexpected<std::string>("map_this_error_rval"));
    auto map_err_on_err_rval = std::move(e_err_rval_src).map_error(MonadicOpsHelpers::transform_error);
    EXPECT_FALSE(map_err_on_err_rval.has_value());
    EXPECT_EQ(map_err_on_err_rval.error(), "transformed_map_this_error_rval");

    // MapError to a different error type
    aos_utils::Expected<int, std::string> e_err_custom(aos_utils::make_unexpected<std::string>("error_for_custom"));
    auto map_err_custom_type = e_err_custom.map_error(MonadicOpsHelpers::transform_to_custom_error);
    EXPECT_FALSE(map_err_custom_type.has_value());
    ASSERT_FALSE(map_err_custom_type.has_value()); // Ensure not proceeding if false
    EXPECT_EQ(map_err_custom_type.error().code, 99);
    EXPECT_EQ(map_err_custom_type.error().msg, "custom_error_for_custom");
    // Verify original error type and new error type using ::error_type
    // This is more of a static check, but can be reflected in template instantiation
    using OriginalErrorType = decltype(e_err_custom)::error_type;
    using NewErrorType = decltype(map_err_custom_type)::error_type;
    EXPECT_TRUE((std::is_same_v<OriginalErrorType, std::string>));
    EXPECT_TRUE((std::is_same_v<NewErrorType, CustomError>));
}

// --- Expected Tests: Comparison Operators ---

TEST(ExpectedComparisonTest, ExpectedToExpected) {
    aos_utils::Expected<int, std::string> val1(10);
    aos_utils::Expected<int, std::string> val1_again(10);
    aos_utils::Expected<int, std::string> val2(20);
    aos_utils::Expected<int, std::string> err1(aos_utils::make_unexpected<std::string>("error1"));
    aos_utils::Expected<int, std::string> err1_again(aos_utils::make_unexpected<std::string>("error1"));
    aos_utils::Expected<int, std::string> err2(aos_utils::make_unexpected<std::string>("error2"));

    // Value comparisons
    EXPECT_TRUE(val1 == val1_again);
    EXPECT_FALSE(val1 == val2);
    EXPECT_FALSE(val1 != val1_again);
    EXPECT_TRUE(val1 != val2);

    // Error comparisons
    EXPECT_TRUE(err1 == err1_again);
    EXPECT_FALSE(err1 == err2);
    EXPECT_FALSE(err1 != err1_again);
    EXPECT_TRUE(err1 != err2);

    // Value vs Error comparisons
    EXPECT_FALSE(val1 == err1);
    EXPECT_TRUE(val1 != err1);

    // Different error types (if comparable underlying E and E2)
    aos_utils::Expected<int, long> err_long(aos_utils::make_unexpected(100L));
    aos_utils::Expected<int, int> err_int(aos_utils::make_unexpected(100));
    // This relies on E == E2. If E and E2 are different types but comparable, it should work.
    // The current Expected::operator== requires has_value() to be same, then compares errors.
    // Error comparison is error() == other.error() which works if E and E2 are comparable.
    EXPECT_TRUE(err_long == err_int); // Assuming long(100) == int(100)
}

TEST(ExpectedComparisonTest, ExpectedToValue) {
    aos_utils::Expected<int, std::string> val1(10);
    aos_utils::Expected<int, std::string> err1(aos_utils::make_unexpected<std::string>("error"));

    EXPECT_TRUE(val1 == 10);
    EXPECT_FALSE(val1 == 20);
    EXPECT_TRUE(val1 != 20);
    EXPECT_FALSE(val1 != 10);

    EXPECT_FALSE(err1 == 10); // Comparing error state to value
    EXPECT_TRUE(err1 != 10);

    // Comparison with value of different type (if T is comparable with T2)
    aos_utils::Expected<long, std::string> val_long(100L);
    EXPECT_TRUE(val_long == 100); // long == int
    EXPECT_FALSE(val_long == 200);
}

TEST(ExpectedComparisonTest, ExpectedToUnexpected) {
    aos_utils::Expected<int, std::string> val1(10);
    aos_utils::Expected<int, std::string> err1(aos_utils::make_unexpected<std::string>("error1"));

    aos_utils::Unexpected<std::string> unex1("error1");
    aos_utils::Unexpected<std::string> unex2("error2");

    EXPECT_TRUE(err1 == unex1);
    EXPECT_FALSE(err1 == unex2);
    EXPECT_TRUE(err1 != unex2);
    EXPECT_FALSE(err1 != unex1);

    EXPECT_FALSE(val1 == unex1); // Comparing value state to Unexpected
    EXPECT_TRUE(val1 != unex1);
}


// --- Expected Tests: Helper Free Functions ---

TEST(ExpectedHelperTest, MakeUnexpected) {
    auto unex_str = aos_utils::make_unexpected("test_error");
    EXPECT_EQ(unex_str.error(), "test_error");
    static_assert(std::is_same_v<decltype(unex_str), aos_utils::Unexpected<std::string>>, "make_unexpected type deduction failed for string literal");

    auto unex_int = aos_utils::make_unexpected(123);
    EXPECT_EQ(unex_int.error(), 123);
    static_assert(std::is_same_v<decltype(unex_int), aos_utils::Unexpected<int>>, "make_unexpected type deduction failed for int");

    std::string s = "move_this";
    auto unex_moved_str = aos_utils::make_unexpected(std::move(s));
    EXPECT_EQ(unex_moved_str.error(), "move_this");
    EXPECT_TRUE(s.empty()); // Check if string was moved from
}

namespace {
    int func_val() { return 42; }
    void func_void() { /* do nothing */ }
    int func_throws() { throw std::runtime_error("func_throws_error"); }
    struct UnknownError {};
    int func_throws_unknown() { throw UnknownError{}; }
}

TEST(ExpectedHelperTest, MakeExpectedWithValue) {
    auto ex_val = aos_utils::make_expected(func_val);
    EXPECT_TRUE(ex_val.has_value());
    EXPECT_EQ(*ex_val, 42);
    static_assert(std::is_same_v<decltype(ex_val)::value_type, int>, "make_expected value type deduction failed");
    static_assert(std::is_same_v<decltype(ex_val)::error_type, std::string>, "make_expected default error type failed");
}

TEST(ExpectedHelperTest, MakeExpectedWithVoid) {
    auto ex_void = aos_utils::make_expected(func_void);
    EXPECT_TRUE(ex_void.has_value()); // Expected<void, E> has a value if no error
    static_assert(std::is_same_v<decltype(ex_void)::value_type, void>, "make_expected void value type deduction failed");
}

TEST(ExpectedHelperTest, MakeExpectedWithThrowStdException) {
    auto ex_throws = aos_utils::make_expected(func_throws);
    EXPECT_FALSE(ex_throws.has_value());
    EXPECT_EQ(ex_throws.error(), "func_throws_error");
}

TEST(ExpectedHelperTest, MakeExpectedWithThrowUnknownException) {
    auto ex_throws_unknown = aos_utils::make_expected(func_throws_unknown);
    EXPECT_FALSE(ex_throws_unknown.has_value());
    EXPECT_EQ(ex_throws_unknown.error(), "Unknown exception");

    // Test with a custom error type for make_expected
    auto ex_throws_custom_err = aos_utils::make_expected<decltype(func_throws_unknown), CustomError>(func_throws_unknown);
    EXPECT_FALSE(ex_throws_custom_err.has_value());
    // This relies on CustomError being constructible from "Unknown exception" string.
    // The current make_expected converts e.what() or "Unknown exception" to E.
    // If E is CustomError, it needs a constructor from const char* or std::string.
    // We added `explicit CustomError(const char* m)` to CustomError struct for this.
    EXPECT_EQ(ex_throws_custom_err.error().msg, "Unknown exception");
    EXPECT_EQ(ex_throws_custom_err.error().code, 500); // Default code from new ctor
}


// --- Expected Tests: Swap Functionality ---

TEST(ExpectedSwapTest, MemberSwap) {
    aos_utils::Expected<int, std::string> val1(10);
    aos_utils::Expected<int, std::string> val2(20);
    aos_utils::Expected<int, std::string> err1(aos_utils::make_unexpected<std::string>("error1"));
    aos_utils::Expected<int, std::string> err2(aos_utils::make_unexpected<std::string>("error2"));

    // Value-Value
    val1.swap(val2);
    EXPECT_EQ(*val1, 20);
    EXPECT_EQ(*val2, 10);

    // Error-Error
    err1.swap(err2);
    EXPECT_EQ(err1.error(), "error2");
    EXPECT_EQ(err2.error(), "error1");

    // Value-Error
    val1.swap(err1); // val1=20 (val), err1="error2" (err)
    EXPECT_FALSE(val1.has_value());
    EXPECT_EQ(val1.error(), "error2");
    EXPECT_TRUE(err1.has_value());
    EXPECT_EQ(*err1, 20);
}

TEST(ExpectedSwapTest, NonMemberSwap) {
    aos_utils::Expected<std::string, int> val1("alpha");
    aos_utils::Expected<std::string, int> err1(aos_utils::make_unexpected(100));

    aos_utils::Expected<std::string, int> val2("beta");
    swap(val1, val2); // val1="beta", val2="alpha"
    EXPECT_EQ(*val1, "beta");
    EXPECT_EQ(*val2, "alpha");

    aos_utils::Expected<std::string, int> err2(aos_utils::make_unexpected(200));
    swap(err1, err2); // err1=unexpected(200), err2=unexpected(100)
    EXPECT_EQ(err1.error(), 200);
    EXPECT_EQ(err2.error(), 100);

    swap(val1, err1); // val1=unexpected(200), err1="beta"
    EXPECT_FALSE(val1.has_value());
    EXPECT_EQ(val1.error(), 200);
    EXPECT_TRUE(err1.has_value());
    EXPECT_EQ(*err1, "beta");
}

// --- Expected<void, E> Tests ---
// Note: Expected<void, E> uses a placeholder type internally for the value.

TEST(ExpectedVoidTest, ConstructionAndState) {
    aos_utils::Expected<void, std::string> void_val; // Default constructed, should have value
    EXPECT_TRUE(void_val.has_value());
    EXPECT_TRUE(static_cast<bool>(void_val));

    aos_utils::Expected<void, std::string> void_val_inplace(std::in_place);
    EXPECT_TRUE(void_val_inplace.has_value());

    aos_utils::Expected<void, std::string> void_err(aos_utils::make_unexpected<std::string>("void_error"));
    EXPECT_FALSE(void_err.has_value());
    EXPECT_FALSE(static_cast<bool>(void_err));
    EXPECT_EQ(void_err.error(), "void_error");

    // value() accessor is typically not available or returns void.
    // operator* and operator-> are not available for Expected<void, E>.
    // Let's check for compilation failure if we tried to use them (conceptual test).
}

TEST(ExpectedVoidTest, Map) {
    aos_utils::Expected<void, std::string> void_val;
    aos_utils::Expected<void, std::string> void_err(aos_utils::make_unexpected<std::string>("err"));
    int side_effect = 0;

    // Test map with a function that takes no arguments and returns a value
    auto map_val_ret_int = void_val.map([&]() { side_effect = 1; return 42; });
    EXPECT_TRUE(map_val_ret_int.has_value());
    ASSERT_TRUE(map_val_ret_int.has_value());
    EXPECT_EQ(*map_val_ret_int, 42);
    EXPECT_EQ(side_effect, 1);

    side_effect = 0; // Reset
    // Test map with a function that takes no arguments and returns void
    auto map_val_ret_void = void_val.map([&]() { side_effect = 2; });
    EXPECT_TRUE(map_val_ret_void.has_value());
    EXPECT_EQ(side_effect, 2);

    side_effect = 0; // Reset
    // Test map on an error Expected<void, E>
    auto map_err_ret_int = void_err.map([&]() { side_effect = 3; return 100; });
    EXPECT_FALSE(map_err_ret_int.has_value());
    EXPECT_EQ(map_err_ret_int.error(), "err");
    EXPECT_EQ(side_effect, 0); // Function should not be called
}

TEST(ExpectedVoidTest, AndThen) {
    aos_utils::Expected<void, std::string> void_val;
    aos_utils::Expected<void, std::string> void_err(aos_utils::make_unexpected<std::string>("err"));
    int side_effect = 0;

    auto then_func_val = [&](/* implicitly void */) -> aos_utils::Expected<int, std::string> { side_effect = 1; return 42; };
    auto then_func_err = [&](/* implicitly void */) -> aos_utils::Expected<int, std::string> { side_effect = 2; return aos_utils::make_unexpected<std::string>("then_err"); };
    auto then_func_void_val = [&](/* implicitly void */) -> aos_utils::Expected<void, std::string> { side_effect = 3; return {}; };


    // AndThen on value, function returns Expected<int, E> with value
    auto res_val_val = void_val.and_then(then_func_val);
    EXPECT_TRUE(res_val_val.has_value());
    ASSERT_TRUE(res_val_val.has_value());
    EXPECT_EQ(*res_val_val, 42);
    EXPECT_EQ(side_effect, 1);

    side_effect = 0; // Reset
    // AndThen on value, function returns Expected<int, E> with error
    auto res_val_err = void_val.and_then(then_func_err);
    EXPECT_FALSE(res_val_err.has_value());
    ASSERT_FALSE(res_val_err.has_value());
    EXPECT_EQ(res_val_err.error(), "then_err");
    EXPECT_EQ(side_effect, 2);

    side_effect = 0; // Reset
    // AndThen on value, function returns Expected<void, E> with value
    auto res_val_void = void_val.and_then(then_func_void_val);
    EXPECT_TRUE(res_val_void.has_value());
    EXPECT_EQ(side_effect, 3);

    side_effect = 0; // Reset
    // AndThen on error, function not called
    auto res_err_val = void_err.and_then(then_func_val);
    EXPECT_FALSE(res_err_val.has_value());
    ASSERT_FALSE(res_err_val.has_value());
    EXPECT_EQ(res_err_val.error(), "err");
    EXPECT_EQ(side_effect, 0);
}

TEST(ExpectedVoidTest, OrElse) {
    aos_utils::Expected<void, std::string> void_val;
    aos_utils::Expected<void, std::string> void_err(aos_utils::make_unexpected<std::string>("original_err"));

    auto recovery_func_val = [](const std::string&) -> aos_utils::Expected<void, std::string> { return {}; };
    auto recovery_func_err = [](const std::string& s) -> aos_utils::Expected<void, std::string> { return aos_utils::make_unexpected("new_err_from_" + s); };

    auto res_val = void_val.or_else(recovery_func_val);
    EXPECT_TRUE(res_val.has_value());

    auto res_err_to_val = void_err.or_else(recovery_func_val);
    EXPECT_TRUE(res_err_to_val.has_value());

    auto res_err_to_err = void_err.or_else(recovery_func_err);
    EXPECT_FALSE(res_err_to_err.has_value());
    ASSERT_FALSE(res_err_to_err.has_value());
    EXPECT_EQ(res_err_to_err.error(), "new_err_from_original_err");
}

TEST(ExpectedVoidTest, MapError) {
    aos_utils::Expected<void, std::string> void_val;
    aos_utils::Expected<void, std::string> void_err(aos_utils::make_unexpected<std::string>("map_this_void_err"));

    auto mapped_val = void_val.map_error(MonadicOpsHelpers::transform_error);
    EXPECT_TRUE(mapped_val.has_value());

    auto mapped_err = void_err.map_error(MonadicOpsHelpers::transform_to_custom_error);
    EXPECT_FALSE(mapped_err.has_value());
    ASSERT_FALSE(mapped_err.has_value());
    EXPECT_EQ(mapped_err.error().code, 99);
    EXPECT_EQ(mapped_err.error().msg, "custom_map_this_void_err");
}

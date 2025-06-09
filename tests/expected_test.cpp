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
    EXPECT_EQ(aos_utils::Unexpected<std::string>("rvalue_error").error(), "rvalue_error");
    const aos_utils::Unexpected<std::string> const_unex_rval("const_rvalue_error");
    EXPECT_EQ(std::move(const_unex_rval).error(), "const_rvalue_error");
}
TEST(UnexpectedTest, MoveConstruction) {
    aos_utils::Unexpected<std::string> unex_str_move_src("move me");
    aos_utils::Unexpected<std::string> unex_str_move_dest = std::move(unex_str_move_src);
    EXPECT_EQ(unex_str_move_dest.error(), "move me");
}
TEST(UnexpectedTest, Comparison) {
    aos_utils::Unexpected<std::string> unex1("error");
    aos_utils::Unexpected<std::string> unex2("error");
    aos_utils::Unexpected<std::string> unex3("different");
    EXPECT_EQ(unex1, unex2); EXPECT_NE(unex1, unex3);
}

namespace { // Anonymous namespace for helper types/structs
struct DefaultConstructible {
    int x; DefaultConstructible() : x(10) {}
    bool operator==(const DefaultConstructible& other) const { return x == other.x; }
};
struct NonDefaultConstructible {
    int x; NonDefaultConstructible(int val) : x(val) {}
    bool operator==(const NonDefaultConstructible& other) const { return x == other.x; }
};
struct CustomError {
    int code; std::string msg;
    CustomError(int c, std::string m) : code(c), msg(std::move(m)) {}
    explicit CustomError(const char* m) : code(500), msg(m) {}
    bool operator==(const CustomError& other) const { return code == other.code && msg == other.msg; }
};
struct MyStruct {
    std::string s; int i;
    MyStruct(std::string val_s, int val_i) : s(std::move(val_s)), i(val_i) {}
    bool operator==(const MyStruct& other) const { return s == other.s && i == other.i; }
};
} // anonymous namespace

// --- Expected Tests: Constructors and Basic Observers ---
TEST(ExpectedTest, DefaultConstruction) {
    aos_utils::Expected<int, std::string> e_int;
    EXPECT_TRUE(e_int.has_value()); EXPECT_EQ(*e_int, 0);
    aos_utils::Expected<DefaultConstructible, std::string> e_custom;
    EXPECT_TRUE(e_custom.has_value()); EXPECT_EQ(e_custom.value().x, 10);
}
TEST(ExpectedTest, ValueConstruction) {
    aos_utils::Expected<int, std::string> e_val(42);
    EXPECT_TRUE(e_val.has_value()); EXPECT_EQ(e_val.value(), 42);
    aos_utils::Expected<std::string, int> e_str_rvalue(std::string("rvalue_hello"));
    EXPECT_TRUE(e_str_rvalue.has_value()); EXPECT_EQ(*e_str_rvalue, "rvalue_hello");
}
TEST(ExpectedTest, ErrorConstructionFromUnexpected) {
    aos_utils::Unexpected<std::string> unex_str("network error");
    aos_utils::Expected<int, std::string> e_err_lval(unex_str);
    EXPECT_FALSE(e_err_lval.has_value()); EXPECT_EQ(e_err_lval.error(), "network error");
    aos_utils::Expected<int, CustomError> e_custom_err(aos_utils::make_unexpected(CustomError(1, "custom")));
    EXPECT_FALSE(e_custom_err.has_value()); EXPECT_EQ(e_custom_err.error().code, 1);
}
TEST(ExpectedTest, ErrorConstructionWithConvertibleErrorType) {
    aos_utils::Unexpected<int> unex_int(404);
    aos_utils::Expected<double, long> e_long_err(unex_int);
    EXPECT_FALSE(e_long_err.has_value()); EXPECT_EQ(e_long_err.error(), 404L);
}
TEST(ExpectedTest, InPlaceValueConstruction) {
    aos_utils::Expected<MyStruct, std::string> e_struct(std::in_place, std::string("my_data"), 123);
    EXPECT_TRUE(e_struct.has_value());
    EXPECT_EQ(e_struct->s, "my_data");
    EXPECT_EQ(e_struct->i, 123);
}
TEST(ExpectedTest, InPlaceErrorConstruction) {
    aos_utils::Expected<int, CustomError> e_custom_err(std::in_place_type_t<aos_utils::Unexpected<CustomError>>{}, 2, "custom in-place");
    EXPECT_FALSE(e_custom_err.has_value()); EXPECT_EQ(e_custom_err.error().code, 2);
}
TEST(ExpectedTest, CopyConstruction) {
    aos_utils::Expected<std::string, CustomError> val_orig("value_for_copy");
    aos_utils::Expected<std::string, CustomError> val_copy = val_orig;
    EXPECT_TRUE(val_copy.has_value()); EXPECT_EQ(*val_copy, "value_for_copy");
    aos_utils::Expected<std::string, CustomError> err_orig(aos_utils::make_unexpected(CustomError(3, "error_for_copy")));
    aos_utils::Expected<std::string, CustomError> err_copy = err_orig;
    EXPECT_FALSE(err_copy.has_value()); EXPECT_EQ(err_copy.error().code, 3);
}
namespace {
struct MoveOnlyType {
    int id; std::string data;
    explicit MoveOnlyType(int i, std::string d = "default") : id(i), data(std::move(d)) {}
    MoveOnlyType(const MoveOnlyType&) = delete; MoveOnlyType& operator=(const MoveOnlyType&) = delete;
    MoveOnlyType(MoveOnlyType&& other) noexcept : id(other.id), data(std::move(other.data)) { other.id = 0; }
    MoveOnlyType& operator=(MoveOnlyType&& other) noexcept {
        if (this != &other) { id = other.id; data = std::move(other.data); other.id = 0; }
        return *this;
    }
};
}
TEST(ExpectedTest, MoveConstructionWithValue) {
    aos_utils::Expected<MoveOnlyType, std::string> val_orig(std::in_place, 123, "move_me_value");
    aos_utils::Expected<MoveOnlyType, std::string> val_moved = std::move(val_orig);
    EXPECT_TRUE(val_moved.has_value()); EXPECT_EQ(val_moved->id, 123);
    EXPECT_TRUE(val_orig.has_value()); EXPECT_EQ(val_orig->id, 0);
}
TEST(ExpectedTest, MoveConstructionWithError) {
    aos_utils::Expected<int, MoveOnlyType> err_orig(std::in_place_type_t<aos_utils::Unexpected<MoveOnlyType>>{}, 456, "move_me_error");
    aos_utils::Expected<int, MoveOnlyType> err_moved = std::move(err_orig);
    EXPECT_FALSE(err_moved.has_value()); EXPECT_EQ(err_moved.error().id, 456);
    EXPECT_FALSE(err_orig.has_value()); EXPECT_EQ(err_orig.error().id, 0);
}

// --- Expected Tests: Assignment Operators ---
TEST(ExpectedTest, CopyAssignment) {
    aos_utils::Expected<std::string, int> val_orig("original_value");
    aos_utils::Expected<std::string, int> err_orig(aos_utils::make_unexpected(100));
    aos_utils::Expected<std::string, int> target_val("target_initial_value");
    target_val = val_orig; EXPECT_TRUE(target_val.has_value()); EXPECT_EQ(*target_val, "original_value");
    aos_utils::Expected<std::string, int> target_err(aos_utils::make_unexpected(200));
    target_err = err_orig; EXPECT_FALSE(target_err.has_value()); EXPECT_EQ(target_err.error(), 100);
    target_err = val_orig; EXPECT_TRUE(target_err.has_value()); EXPECT_EQ(*target_err, "original_value");
    target_val = err_orig; EXPECT_FALSE(target_val.has_value()); EXPECT_EQ(target_val.error(), 100);
}
TEST(ExpectedTest, MoveAssignment) {
    aos_utils::Expected<MoveOnlyType, std::string> val_source(std::in_place, 1, "src_val");
    aos_utils::Expected<MoveOnlyType, std::string> val_target(std::in_place, 2, "tgt_val");
    val_target = std::move(val_source); EXPECT_TRUE(val_target.has_value()); EXPECT_EQ(val_target->id, 1);
    aos_utils::Expected<int, MoveOnlyType> err_source(std::in_place_type_t<aos_utils::Unexpected<MoveOnlyType>>{}, 3, "src_err");
    aos_utils::Expected<int, MoveOnlyType> err_target(std::in_place_type_t<aos_utils::Unexpected<MoveOnlyType>>{}, 4, "tgt_err");
    err_target = std::move(err_source); EXPECT_FALSE(err_target.has_value()); EXPECT_EQ(err_target.error().id, 3);
}
TEST(ExpectedTest, ValueAssignmentOperator) {
    aos_utils::Expected<int, std::string> e(10); e = 20; EXPECT_TRUE(e.has_value()); EXPECT_EQ(*e, 20);
    aos_utils::Expected<int, std::string> e_err(aos_utils::make_unexpected("error"));
    e_err = 30; EXPECT_TRUE(e_err.has_value()); EXPECT_EQ(*e_err, 30);
}
TEST(ExpectedTest, UnexpectedAssignmentOperator) {
    aos_utils::Expected<int, std::string> e(10); e = aos_utils::make_unexpected("new error");
    EXPECT_FALSE(e.has_value()); EXPECT_EQ(e.error(), "new error");
}

// --- Expected Tests: Accessors ---
TEST(ExpectedTest, ValueAccessor) {
    aos_utils::Expected<std::string, int> e_val("hello"); EXPECT_EQ(e_val.value(), "hello");
    *e_val = "world"; EXPECT_EQ(e_val.value(), "world");
    const aos_utils::Expected<std::string, int> ce_val("const_hello"); EXPECT_EQ(ce_val.value(), "const_hello");
    aos_utils::Expected<int, std::string> e_err(aos_utils::make_unexpected("access error"));
    EXPECT_THROW(e_err.value(), std::bad_variant_access);
}
TEST(ExpectedTest, ErrorAccessor) {
    aos_utils::Expected<int, std::string> e_err(aos_utils::make_unexpected("error msg")); EXPECT_EQ(e_err.error(), "error msg");
    const aos_utils::Expected<int, std::string> ce_err(aos_utils::make_unexpected("const error msg")); EXPECT_EQ(ce_err.error(), "const error msg");
    aos_utils::Expected<std::string, int> e_val("value"); EXPECT_THROW(e_val.error(), std::bad_variant_access);
}
TEST(ExpectedTest, DereferenceOperator) {
    aos_utils::Expected<std::string, int> e_val("data"); (*e_val).append("_appended"); EXPECT_EQ(*e_val, "data_appended");
}
TEST(ExpectedTest, ArrowOperator) {
    aos_utils::Expected<MyStruct, std::string> e_val(std::in_place, std::string("struct_data"), 42);
    EXPECT_EQ(e_val->s, "struct_data"); EXPECT_EQ(e_val->i, 42);
    const aos_utils::Expected<MyStruct, std::string> ce_val(std::in_place, std::string("const_struct_data"), 100);
    EXPECT_EQ(ce_val->s, "const_struct_data"); EXPECT_EQ(ce_val->i, 100);
}
TEST(ExpectedTest, ValueOr) {
    aos_utils::Expected<int, std::string> e_val(123); EXPECT_EQ(e_val.value_or(456), 123);
    aos_utils::Expected<int, std::string> e_err(aos_utils::make_unexpected("error")); EXPECT_EQ(e_err.value_or(789), 789);
}

// --- Expected Tests: Monadic Operations ---
struct MonadicOpsHelpers {
    static int times_two(int x) { return x * 2; }
    static std::string to_str(int x) { return std::to_string(x); }
    static aos_utils::Expected<int, std::string> times_three_expected(int x) { return x * 3; }
    static aos_utils::Expected<int, std::string> always_error_expected(int) { return aos_utils::make_unexpected<std::string>("always_error"); }
    static aos_utils::Expected<std::string, std::string> recover_with_value(const std::string& err_msg) { return "recovered_from_" + err_msg; }
    static aos_utils::Expected<std::string, std::string> recover_with_new_error(const std::string&) { return aos_utils::make_unexpected<std::string>("new_recovery_error"); }
    static aos_utils::Expected<int, std::string> recover_int_with_value(const std::string&) { return 0; }
    static std::string transform_error(const std::string& err_msg) { return "transformed_" + err_msg; }
    static CustomError transform_to_custom_error(const std::string& err_msg) { return CustomError(99, "custom_" + err_msg); }
    static void void_func_for_int(int) { /* do nothing */ } // Renamed for clarity
    static void void_func_no_args() {}
    static int int_func_no_args() {return 42;}
    static aos_utils::Expected<int,std::string> expected_int_func_no_args() {return 42;}
    static aos_utils::Expected<void,std::string> expected_void_func_no_args() {return aos_utils::Expected<void,std::string>(std::in_place);}
};

TEST(ExpectedMonadicTest, Map) {
    aos_utils::Expected<int, std::string> e_val(10);
    auto mapped_val = e_val.map(MonadicOpsHelpers::times_two);
    EXPECT_TRUE(mapped_val.has_value()); EXPECT_EQ(*mapped_val, 20);
    aos_utils::Expected<int, std::string> e_err(aos_utils::make_unexpected<std::string>("map_error"));
    auto mapped_err = e_err.map(MonadicOpsHelpers::times_two);
    EXPECT_FALSE(mapped_err.has_value()); EXPECT_EQ(mapped_err.error(), "map_error");
    aos_utils::Expected<int, std::string> e_val_for_void(5);
    auto map_void_res = e_val_for_void.map(MonadicOpsHelpers::void_func_for_int); // Use correct void func
    EXPECT_TRUE(map_void_res.has_value());
}
TEST(ExpectedMonadicTest, AndThen) {
    aos_utils::Expected<int, std::string> e_val(5);
    auto then_val_val = e_val.and_then(MonadicOpsHelpers::times_three_expected);
    EXPECT_TRUE(then_val_val.has_value()); EXPECT_EQ(*then_val_val, 15);
}
TEST(ExpectedMonadicTest, OrElse) { // Adjusted to use matching types for T for simpler test
    aos_utils::Expected<int, std::string> e_int_val(10);
    auto else_val = e_int_val.or_else(MonadicOpsHelpers::recover_int_with_value);
    EXPECT_TRUE(else_val.has_value()); EXPECT_EQ(*else_val, 10);
    aos_utils::Expected<int, std::string> e_int_err(aos_utils::make_unexpected<std::string>("or_else_error_int"));
    auto else_err_val = e_int_err.or_else(MonadicOpsHelpers::recover_int_with_value);
    EXPECT_TRUE(else_err_val.has_value()); EXPECT_EQ(*else_err_val, 0);
}
TEST(ExpectedMonadicTest, MapError) {
    aos_utils::Expected<int, std::string> e_val(123);
    auto map_err_on_val = e_val.map_error(MonadicOpsHelpers::transform_error); // G is std::string
    EXPECT_TRUE(map_err_on_val.has_value());
    static_assert(std::is_same_v<typename decltype(e_val)::error_type, std::string>);
    static_assert(std::is_same_v<typename decltype(map_err_on_val)::error_type, std::string>);
    aos_utils::Expected<int, std::string> e_err_custom(aos_utils::make_unexpected<std::string>("error_for_custom"));
    auto map_err_custom_type = e_err_custom.map_error(MonadicOpsHelpers::transform_to_custom_error); // G is CustomError
    EXPECT_FALSE(map_err_custom_type.has_value()); EXPECT_EQ(map_err_custom_type.error().code, 99);
}

// --- Expected Tests: Comparison Operators ---
TEST(ExpectedComparisonTest, ExpectedToExpected) {
    aos_utils::Expected<int, std::string> val1(10); aos_utils::Expected<int, std::string> val1_again(10); EXPECT_TRUE(val1 == val1_again);
    aos_utils::Expected<int, std::string> err1(aos_utils::make_unexpected<std::string>("error1")); aos_utils::Expected<int, std::string> err1_again(aos_utils::make_unexpected<std::string>("error1")); EXPECT_TRUE(err1 == err1_again);
}
TEST(ExpectedComparisonTest, ExpectedToValue) {
    aos_utils::Expected<int, std::string> val1(10); EXPECT_TRUE(val1 == 10);
    aos_utils::Expected<int, std::string> err1(aos_utils::make_unexpected<std::string>("error")); EXPECT_FALSE(err1 == 10);
}
TEST(ExpectedComparisonTest, ExpectedToUnexpected) {
    aos_utils::Expected<int, std::string> err1(aos_utils::make_unexpected<std::string>("error1")); aos_utils::Unexpected<std::string> unex1("error1"); EXPECT_TRUE(err1 == unex1);
}

// --- Expected Tests: Helper Free Functions ---
TEST(ExpectedHelperTest, MakeUnexpected) {
    auto unex_str = aos_utils::make_unexpected("test_error");
    EXPECT_EQ(unex_str.error(), "test_error");
    static_assert(std::is_same_v<std::decay_t<decltype(unex_str.error())>, const char*>);
}
namespace {
    int func_val_for_make() { return 42; } void func_void_for_make() { } // Renamed to avoid conflict
    int func_throws_for_make() { throw std::runtime_error("func_throws_error"); }
    struct UnknownError_for_make {}; int func_throws_unknown_for_make() { throw UnknownError_for_make{}; }
}
TEST(ExpectedHelperTest, MakeExpectedWithValue) {
    auto ex_val = aos_utils::make_expected(func_val_for_make); EXPECT_TRUE(ex_val.has_value()); EXPECT_EQ(*ex_val, 42);
}
TEST(ExpectedHelperTest, MakeExpectedWithVoid) {
    auto ex_void = aos_utils::make_expected(func_void_for_make); EXPECT_TRUE(ex_void.has_value());
}
TEST(ExpectedHelperTest, MakeExpectedWithThrowStdException) {
    auto ex_throws = aos_utils::make_expected(func_throws_for_make); EXPECT_FALSE(ex_throws.has_value()); EXPECT_EQ(ex_throws.error(), "func_throws_error");
}
TEST(ExpectedHelperTest, MakeExpectedWithThrowUnknownException) {
    auto ex_throws_unknown = aos_utils::make_expected(func_throws_unknown_for_make); EXPECT_FALSE(ex_throws_unknown.has_value()); EXPECT_EQ(ex_throws_unknown.error(), "Unknown exception");
    auto ex_throws_custom_err = aos_utils::make_expected<decltype(func_throws_unknown_for_make), CustomError>(func_throws_unknown_for_make); EXPECT_FALSE(ex_throws_custom_err.has_value()); EXPECT_EQ(ex_throws_custom_err.error().msg, "Unknown exception");
}

// --- Expected Tests: Swap Functionality ---
TEST(ExpectedSwapTest, MemberSwap) {
    aos_utils::Expected<int, std::string> val1(10), val2(20); val1.swap(val2); EXPECT_EQ(*val1, 20); EXPECT_EQ(*val2, 10);
}
TEST(ExpectedSwapTest, NonMemberSwap) {
    aos_utils::Expected<std::string, int> val1("alpha"), val2("beta"); swap(val1, val2); EXPECT_EQ(*val1, "beta"); EXPECT_EQ(*val2, "alpha");
}

// --- Expected<void, E> Tests ---
TEST(ExpectedVoidTest, ConstructionAndState) {
    aos_utils::Expected<void, std::string> void_val; EXPECT_TRUE(void_val.has_value());
    aos_utils::Expected<void, std::string> void_val_inplace(std::in_place); EXPECT_TRUE(void_val_inplace.has_value());
    aos_utils::Expected<void, std::string> void_err(aos_utils::make_unexpected<std::string>("void_error")); EXPECT_FALSE(void_err.has_value());
}
TEST(ExpectedVoidTest, Map) {
    aos_utils::Expected<void, std::string> void_val; int side_effect = 0;
    auto map_val_ret_int = void_val.map([&]() { side_effect = 1; return MonadicOpsHelpers::int_func_no_args(); });
    EXPECT_TRUE(map_val_ret_int.has_value()); EXPECT_EQ(*map_val_ret_int, 42); EXPECT_EQ(side_effect, 1);
    side_effect = 0;
    auto map_val_ret_void = void_val.map([&]() { side_effect = 2; MonadicOpsHelpers::void_func_no_args();});
    EXPECT_TRUE(map_val_ret_void.has_value()); EXPECT_EQ(side_effect, 2);
}
TEST(ExpectedVoidTest, AndThen) {
    aos_utils::Expected<void, std::string> void_val; int side_effect = 0;
    auto then_func_void_val = [&](void) -> aos_utils::Expected<void, std::string> { side_effect = 3; return aos_utils::Expected<void, std::string>(std::in_place); };
    auto res_val_void = void_val.and_then(then_func_void_val); EXPECT_TRUE(res_val_void.has_value()); EXPECT_EQ(side_effect, 3);
}
TEST(ExpectedVoidTest, OrElse) {
    aos_utils::Expected<void, std::string> void_val(std::in_place);
    auto recovery_func_val = [](const std::string&) -> aos_utils::Expected<void, std::string> { return aos_utils::Expected<void, std::string>(std::in_place); };
    auto res_val = void_val.or_else(recovery_func_val); EXPECT_TRUE(res_val.has_value());
}
TEST(ExpectedVoidTest, MapError) {
    aos_utils::Expected<void, std::string> void_val(std::in_place); auto mapped_val = void_val.map_error(MonadicOpsHelpers::transform_error); EXPECT_TRUE(mapped_val.has_value());
    aos_utils::Expected<void, std::string> void_err(aos_utils::make_unexpected<std::string>("map_this_void_err"));
    auto mapped_err = void_err.map_error(MonadicOpsHelpers::transform_to_custom_error); EXPECT_FALSE(mapped_err.has_value()); EXPECT_EQ(mapped_err.error().code, 99);
}

// InPlaceValueConstructionMyStruct was moved into InPlaceValueConstruction
// OrElseString was to test or_else with T=string, E=string. The main OrElse test has been simplified.
// Adding it back for specific coverage if needed, or ensuring the generic or_else test covers it.
TEST(ExpectedMonadicTest, OrElseString) {
    aos_utils::Expected<std::string, std::string> e_s_val("original_value");
    auto else_s_val = e_s_val.or_else(MonadicOpsHelpers::recover_with_value);
    EXPECT_TRUE(else_s_val.has_value()); EXPECT_EQ(*else_s_val, "original_value");

    aos_utils::Expected<std::string, std::string> e_s_err(aos_utils::make_unexpected<std::string>("or_else_s_error"));
    auto else_s_err_val = e_s_err.or_else(MonadicOpsHelpers::recover_with_value);
    EXPECT_TRUE(else_s_err_val.has_value()); EXPECT_EQ(*else_s_err_val, "recovered_from_or_else_s_error");
}

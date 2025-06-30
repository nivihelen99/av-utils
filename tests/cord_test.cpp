#include "gtest/gtest.h"
#include "cord.h"
#include <string>
#include <stdexcept>

// Test fixture for Cord tests
class CordTest : public ::testing::Test {
protected:
    Cord c_empty;
    Cord c_hello;
    Cord c_world;
    std::string s_hello = "Hello";
    std::string s_world = ", World";
    std::string s_hw = "Hello, World";

    CordTest() : c_hello(s_hello.c_str()), c_world(s_world.c_str()) {}
};

// Test Default Constructor and Empty State
TEST_F(CordTest, DefaultConstructor) {
    Cord c;
    EXPECT_EQ(c.length(), 0);
    EXPECT_TRUE(c.empty());
    EXPECT_EQ(c.ToString(), "");
}

// Test Constructor from const char*
TEST_F(CordTest, ConstructorFromCString) {
    const char* test_str = "Test C-String";
    Cord c(test_str);
    EXPECT_EQ(c.length(), strlen(test_str));
    EXPECT_FALSE(c.empty());
    EXPECT_EQ(c.ToString(), test_str);

    Cord c_null(nullptr);
    EXPECT_EQ(c_null.length(), 0);
    EXPECT_TRUE(c_null.empty());
    EXPECT_EQ(c_null.ToString(), "");

    Cord c_empty_cstr("");
    EXPECT_EQ(c_empty_cstr.length(), 0);
    EXPECT_TRUE(c_empty_cstr.empty());
    EXPECT_EQ(c_empty_cstr.ToString(), "");
}

// Test Constructor from std::string (lvalue and rvalue)
TEST_F(CordTest, ConstructorFromString) {
    std::string str_lval = "LValue String";
    Cord c_lval(str_lval);
    EXPECT_EQ(c_lval.length(), str_lval.length());
    EXPECT_EQ(c_lval.ToString(), str_lval);

    Cord c_rval(std::string("RValue String"));
    EXPECT_EQ(c_rval.length(), std::string("RValue String").length());
    EXPECT_EQ(c_rval.ToString(), "RValue String");

    std::string empty_s = "";
    Cord c_empty_s(empty_s);
    EXPECT_EQ(c_empty_s.length(), 0);
    EXPECT_TRUE(c_empty_s.empty());
    EXPECT_EQ(c_empty_s.ToString(), "");
}

// Test Copy Constructor
TEST_F(CordTest, CopyConstructor) {
    Cord original("Copy Me");
    Cord copy = original;
    EXPECT_EQ(copy.length(), original.length());
    EXPECT_EQ(copy.ToString(), original.ToString());
    // Ensure original is not affected
    EXPECT_EQ(original.ToString(), "Copy Me");
}

// Test Move Constructor
TEST_F(CordTest, MoveConstructor) {
    Cord original("Move Me");
    std::string original_str = original.ToString();
    size_t original_len = original.length();

    Cord moved_to = std::move(original);
    EXPECT_EQ(moved_to.length(), original_len);
    EXPECT_EQ(moved_to.ToString(), original_str);

    // Original Cord should be in a valid but unspecified (likely empty) state
    // Depending on implementation, it might be empty or hold some other valid state.
    // For this implementation, it becomes empty.
    EXPECT_EQ(original.length(), 0);
    EXPECT_TRUE(original.empty());
    EXPECT_EQ(original.ToString(), "");
}

// Test Assignment Operators
TEST_F(CordTest, AssignmentOperators) {
    Cord c;
    c = "Assigned C-String";
    EXPECT_EQ(c.ToString(), "Assigned C-String");

    std::string s_assign = "Assigned std::string";
    c = s_assign;
    EXPECT_EQ(c.ToString(), s_assign);

    c = std::string("Assigned moved std::string");
    EXPECT_EQ(c.ToString(), "Assigned moved std::string");

    Cord c_other("Another Cord");
    c = c_other; // Copy assignment
    EXPECT_EQ(c.ToString(), c_other.ToString());
    EXPECT_EQ(c_other.ToString(), "Another Cord"); // Ensure other is not affected

    Cord c_to_move("To Be Moved");
    std::string moved_str_content = c_to_move.ToString();
    c = std::move(c_to_move); // Move assignment
    EXPECT_EQ(c.ToString(), moved_str_content);
    EXPECT_EQ(c_to_move.length(), 0); // Moved-from state
    EXPECT_TRUE(c_to_move.empty());
}


// Test Length and Empty
TEST_F(CordTest, LengthAndEmpty) {
    EXPECT_EQ(c_empty.length(), 0);
    EXPECT_TRUE(c_empty.empty());

    EXPECT_EQ(c_hello.length(), s_hello.length());
    EXPECT_FALSE(c_hello.empty());
}

// Test Clear
TEST_F(CordTest, Clear) {
    Cord c("Clearable");
    EXPECT_FALSE(c.empty());
    c.clear();
    EXPECT_EQ(c.length(), 0);
    EXPECT_TRUE(c.empty());
    EXPECT_EQ(c.ToString(), "");
}

// Test Concatenation
TEST_F(CordTest, Concatenation) {
    Cord c_hw_concat = c_hello + c_world;
    EXPECT_EQ(c_hw_concat.length(), s_hw.length());
    EXPECT_EQ(c_hw_concat.ToString(), s_hw);

    Cord c_hw_cstr = c_hello + ", World";
    EXPECT_EQ(c_hw_cstr.length(), s_hw.length());
    EXPECT_EQ(c_hw_cstr.ToString(), s_hw);

    std::string s_suffix = "!";
    Cord c_hw_str = c_hw_concat + s_suffix;
    EXPECT_EQ(c_hw_str.length(), s_hw.length() + s_suffix.length());
    EXPECT_EQ(c_hw_str.ToString(), s_hw + s_suffix);

    Cord c_prefix_cstr = "Prefix: " + c_hello;
    EXPECT_EQ(c_prefix_cstr.ToString(), "Prefix: Hello");

    Cord c_prefix_str = std::string("PrefixStr: ") + c_hello;
    EXPECT_EQ(c_prefix_str.ToString(), "PrefixStr: Hello");

    // Concatenating with empty
    Cord c_empty_plus_hello = c_empty + c_hello;
    EXPECT_EQ(c_empty_plus_hello.ToString(), s_hello);

    Cord c_hello_plus_empty = c_hello + c_empty;
    EXPECT_EQ(c_hello_plus_empty.ToString(), s_hello);
}

// Test Character Access (at and operator[])
TEST_F(CordTest, AtOperator) {
    Cord c(s_hw);
    ASSERT_EQ(c.length(), s_hw.length());

    for (size_t i = 0; i < s_hw.length(); ++i) {
        EXPECT_EQ(c[i], s_hw[i]) << "Mismatch at index " << i << " using operator[]";
        EXPECT_EQ(c.at(i), s_hw[i]) << "Mismatch at index " << i << " using at()";
    }

    EXPECT_THROW(c.at(c.length()), std::out_of_range);
    EXPECT_THROW(c.at(c.length() + 10), std::out_of_range);

    // Test on more complex cord (concatenated)
    Cord part1("Part1-");
    Cord part2("Part2-");
    Cord part3("Part3");
    Cord complex_cord = part1 + part2 + part3;
    std::string complex_str = "Part1-Part2-Part3";
    ASSERT_EQ(complex_cord.length(), complex_str.length());

    for (size_t i = 0; i < complex_str.length(); ++i) {
        EXPECT_EQ(complex_cord[i], complex_str[i]) << "Complex cord mismatch at index " << i;
    }
    EXPECT_THROW(complex_cord.at(complex_cord.length()), std::out_of_range);
}

// Test Substring
TEST_F(CordTest, SubstrBasic) {
    Cord c(s_hw); // "Hello, World"

    Cord sub1 = c.substr(0, 5); // "Hello"
    EXPECT_EQ(sub1.length(), 5);
    EXPECT_EQ(sub1.ToString(), "Hello");

    Cord sub2 = c.substr(7, 5); // "World"
    EXPECT_EQ(sub2.length(), 5);
    EXPECT_EQ(sub2.ToString(), "World");

    Cord sub3 = c.substr(c.length() - 1, 1); // "d"
    EXPECT_EQ(sub3.length(), 1);
    EXPECT_EQ(sub3.ToString(), "d");

    Cord sub_full = c.substr(); // Full string
    EXPECT_EQ(sub_full.length(), c.length());
    EXPECT_EQ(sub_full.ToString(), c.ToString());
}

TEST_F(CordTest, SubstrEdgeCases) {
    Cord c(s_hw); // "Hello, World" (length 12)

    // Substring of length 0
    Cord sub_len0 = c.substr(3, 0);
    EXPECT_EQ(sub_len0.length(), 0);
    EXPECT_TRUE(sub_len0.empty());
    EXPECT_EQ(sub_len0.ToString(), "");

    // Substring from end (should be empty)
    Cord sub_from_end = c.substr(c.length());
    EXPECT_EQ(sub_from_end.length(), 0);
    EXPECT_TRUE(sub_from_end.empty());

    // Substring with count exceeding length
    Cord sub_count_overflow = c.substr(7); // "World"
    EXPECT_EQ(sub_count_overflow.length(), 5);
    EXPECT_EQ(sub_count_overflow.ToString(), "World");

    Cord sub_count_overflow2 = c.substr(7, 100); // "World"
    EXPECT_EQ(sub_count_overflow2.length(), 5);
    EXPECT_EQ(sub_count_overflow2.ToString(), "World");


    // Substring from pos > length (should throw)
    EXPECT_THROW(c.substr(c.length() + 1), std::out_of_range);
    EXPECT_THROW(c.substr(c.length() + 1, 5), std::out_of_range);

    // Substring from empty cord
    Cord empty_c;
    Cord sub_from_empty_c = empty_c.substr(0,0);
    EXPECT_TRUE(sub_from_empty_c.empty());
    Cord sub_from_empty_c2 = empty_c.substr();
    EXPECT_TRUE(sub_from_empty_c2.empty());
    EXPECT_THROW(empty_c.substr(1), std::out_of_range);

    // Substring on a more complex (concatenated) cord
    Cord c_complex = Cord("One") + Cord("-Two-") + Cord("Three"); // "One-Two-Three" (length 13)
    std::string s_complex = "One-Two-Three";

    Cord sub_c1 = c_complex.substr(0, 3); // "One"
    EXPECT_EQ(sub_c1.ToString(), "One");

    Cord sub_c2 = c_complex.substr(4, 3); // "Two"
    EXPECT_EQ(sub_c2.ToString(), "Two");

    Cord sub_c3 = c_complex.substr(8, 5); // "Three"
    EXPECT_EQ(sub_c3.ToString(), "Three");

    Cord sub_c_span = c_complex.substr(2, 7); // "e-Two-T"
    EXPECT_EQ(sub_c_span.ToString(), "e-Two-T");
}


// Test ToString
TEST_F(CordTest, ToString) {
    EXPECT_EQ(c_empty.ToString(), "");
    EXPECT_EQ(c_hello.ToString(), s_hello);

    Cord c_hw_concat = c_hello + c_world;
    EXPECT_EQ(c_hw_concat.ToString(), s_hw);

    Cord complex_cord = Cord("Alpha") + (Cord("Beta") + Cord("Gamma")) + Cord("Delta");
    EXPECT_EQ(complex_cord.ToString(), "AlphaBetaGammaDelta");
}

// Test with Cords containing empty strings
TEST_F(CordTest, EmptyStringParts) {
    Cord c1("");
    Cord c2("Data");
    Cord c3 = c1 + c2;
    EXPECT_EQ(c3.ToString(), "Data");
    EXPECT_EQ(c3.length(), 4);

    Cord c4 = c2 + c1;
    EXPECT_EQ(c4.ToString(), "Data");
    EXPECT_EQ(c4.length(), 4);

    Cord c5 = c1 + c1;
    EXPECT_EQ(c5.ToString(), "");
    EXPECT_EQ(c5.length(), 0);
    EXPECT_TRUE(c5.empty());

    Cord c6 = Cord("") + "NonEmpty" + Cord("");
    EXPECT_EQ(c6.ToString(), "NonEmpty");
    EXPECT_EQ(c6.length(), 8);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

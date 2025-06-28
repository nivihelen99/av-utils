#include "gtest/gtest.h"
#include "split_view.h" // Assuming split_view.h is in the include path
#include <vector>
#include <string> // For std::string in tests if needed for constructing string_view easily

// Helper function to collect tokens into a vector for easy comparison
std::vector<std::string_view> collect_tokens(SplitView& view) {
    std::vector<std::string_view> tokens;
    for (auto token : view) {
        tokens.push_back(token);
    }
    return tokens;
}

// Helper function to compare vector of string_views with vector of const char*
void compare_tokens(const std::vector<std::string_view>& actual, const std::vector<const char*>& expected) {
    ASSERT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < actual.size(); ++i) {
        EXPECT_EQ(actual[i], expected[i]);
    }
}

TEST(SplitViewTest, EmptyInput) {
    std::string_view sv = "";
    SplitView view(sv, ',');
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {""});
}

TEST(SplitViewTest, EmptyInputStringDelimiter) {
    std::string_view sv = "";
    SplitView view(sv, ",,");
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {""});
}

TEST(SplitViewTest, NoDelimiterFound) {
    std::string_view sv = "abc";
    SplitView view(sv, ',');
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"abc"});
}

TEST(SplitViewTest, NoDelimiterFoundStringDelimiter) {
    std::string_view sv = "abc";
    SplitView view(sv, "::");
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"abc"});
}

TEST(SplitViewTest, BasicSplitCharDelimiter) {
    std::string_view sv = "one,two,three";
    SplitView view(sv, ',');
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"one", "two", "three"});
}

TEST(SplitViewTest, BasicSplitStringDelimiter) {
    std::string_view sv = "one::two::three";
    SplitView view(sv, "::");
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"one", "two", "three"});
}

TEST(SplitViewTest, LeadingDelimiterChar) {
    std::string_view sv = ",one,two";
    SplitView view(sv, ',');
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"", "one", "two"});
}

TEST(SplitViewTest, LeadingDelimiterString) {
    std::string_view sv = "::one::two";
    SplitView view(sv, "::");
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"", "one", "two"});
}

TEST(SplitViewTest, TrailingDelimiterChar) {
    std::string_view sv = "one,two,";
    SplitView view(sv, ',');
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"one", "two", ""});
}

TEST(SplitViewTest, TrailingDelimiterString) {
    std::string_view sv = "one::two::";
    SplitView view(sv, "::");
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"one", "two", ""});
}

TEST(SplitViewTest, ConsecutiveDelimitersChar) {
    std::string_view sv = "one,,two";
    SplitView view(sv, ',');
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"one", "", "two"});
}

TEST(SplitViewTest, ConsecutiveDelimitersString) {
    std::string_view sv = "one::::two"; // Delimiter "::"
    SplitView view(sv, "::");
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"one", "", "two"});
}

TEST(SplitViewTest, ExampleFromRequirements) {
    std::string_view sv = "one,two,,three";
    SplitView view(sv, ',');
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"one", "two", "", "three"});
}

TEST(SplitViewTest, OnlyDelimitersChar) {
    std::string_view sv = ",,,"; // 3 delimiters, 4 tokens
    SplitView view(sv, ',');
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"", "", "", ""});
}

TEST(SplitViewTest, OnlyDelimitersString) {
    std::string_view sv = "::::"; // Delimiter "::", 2 delimiters, 3 tokens
    SplitView view(sv, "::");
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"", "", ""});
}

TEST(SplitViewTest, SingleTokenNoDelimiterChar) {
    std::string_view sv = "token";
    SplitView view(sv, ',');
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"token"});
}

TEST(SplitViewTest, SingleTokenNoDelimiterString) {
    std::string_view sv = "token";
    SplitView view(sv, "::");
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"token"});
}

TEST(SplitViewTest, StringDelimiterLongerThanInput) {
    std::string_view sv = "hi";
    SplitView view(sv, "hello");
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"hi"});
}

TEST(SplitViewTest, StringDelimiterSameAsInput) {
    std::string_view sv = "delim";
    SplitView view(sv, "delim");
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"", ""});
}


TEST(SplitViewTest, SplitByWhitespaceString) {
    std::string_view sv = "hello world  test";
    SplitView view(sv, " "); // Single space delimiter
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"hello", "world", "", "test"});
}

TEST(SplitViewTest, IteratorPostIncrement) {
    std::string_view sv = "a,b";
    SplitView view(sv, ',');
    auto it = view.begin();
    ASSERT_NE(it, view.end());
    EXPECT_EQ(*it++, "a"); // Check value before increment, then increment
    ASSERT_NE(it, view.end());
    EXPECT_EQ(*it, "b");
    EXPECT_EQ(*it++, "b"); // Check value before increment, then increment
    ASSERT_EQ(it, view.end());      // After fetching "b" and incrementing, iterator should be at end.
}

TEST(SplitViewTest, IteratorComparison) {
    std::string_view sv = "x,y";
    SplitView view(sv, ',');
    auto it1 = view.begin();
    auto it2 = view.begin();
    auto end_it = view.end();

    ASSERT_EQ(it1, it2);
    ASSERT_NE(it1, end_it);

    ++it1; // it1 points to "y"
    ASSERT_NE(it1, it2); // it2 still points to "x"
    EXPECT_EQ(*it2, "x");
    EXPECT_EQ(*it1, "y");

    ++it2; // it2 points to "y"
    ASSERT_EQ(it1, it2);

    ++it1; // it1 points to end
    ASSERT_EQ(it1, end_it);
    ASSERT_NE(it2, end_it); //it2 still points to "y"

    ++it2; // it2 points to end
    ASSERT_EQ(it1, it2);
    ASSERT_EQ(it2, end_it);
}

TEST(SplitViewTest, EmptyDelimiterStringView) {
    // Python's split('') raises ValueError. C#'s string.Split(new string[] { "" }, ...) has specific behavior.
    // The requirements state "Single char, string literal, or std::string_view delimiter".
    // An empty std::string_view is possible.
    // Current implementation detail: "If delimiter_ is empty, we'll treat it as 'no delimiter found'."
    // This means it yields the whole string as one token.
    std::string_view sv = "abc";
    SplitView view(sv, std::string_view("")); // Empty string_view delimiter
    std::vector<std::string_view> tokens = collect_tokens(view);
    // Based on current logic in find_next_token for empty delimiter_:
    // current_token_ = input_.substr(current_pos_); current_pos_ = input_.length(); is_end_ = true;
    compare_tokens(tokens, {"abc"});
}

TEST(SplitViewTest, EmptyDelimiterStringViewWithEmptyInput) {
    std::string_view sv = "";
    SplitView view(sv, std::string_view(""));
    std::vector<std::string_view> tokens = collect_tokens(view);
    // find_next_token for empty input: current_token_="", is_end_=true.
    compare_tokens(tokens, {""});
}

// What if the delimiter itself contains characters that could be part of another delimiter match?
// e.g., input = "ababab", delimiter = "aba"
// Expected: "", "b", ""  (aba bab ab -> first "aba" at 0, token "", next_pos=3. input.find("aba",3) -> "aba" at 3. token "b". next_pos=3+3=6. input.find("aba",6) -> npos. last token should be ""? )
// Let's trace:
// input="ababab", delim="aba"
// begin(): current_pos=0. find("aba",0) -> 0. token=input.substr(0,0-0)="". current_pos_ = 0 + 3 = 3.
// ++: current_pos=3. find("aba",3) -> 3. token=input.substr(3,3-3)="". current_pos_ = 3 + 3 = 6.
// ++: current_pos=6. find("aba",6) -> npos. token=input.substr(6) = "". current_pos_ = 6+1=7. is_end_=true.
// Tokens: {"", "", ""} -> This matches "delim delim delim" ":::" with "::" -> "", "", ""
// Let's try input="ababa", delim="aba"
// begin(): current_pos=0. find("aba",0) -> 0. token="". current_pos_=3.
// ++: current_pos=3. find("aba",3) -> npos. token=input.substr(3)="ba". current_pos_=5+1=6. is_end_=true.
// Tokens: {"", "ba"}

TEST(SplitViewTest, StringDelimiterOverlappingPotential) {
    std::string_view sv = "ababa"; // "aba" then "ba" remaining
    SplitView view(sv, "aba");
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"", "ba"});
}



TEST(SplitViewTest, DelimiterAtVeryBeginningAndEndString) {
    std::string_view sv = "::abc::";
    SplitView view(sv, "::");
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"", "abc", ""});
}

TEST(SplitViewTest, DelimiterIsWholeStringChar) {
    std::string_view sv = ",";
    SplitView view(sv, ',');
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"", ""});
}

TEST(SplitViewTest, DelimiterLongerThanStringButStartsWithIt) {
    std::string_view sv = "ab";
    SplitView view(sv, "abc");
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"ab"});
}

// Test from example output in requirements
TEST(SplitViewTest, RequirementExampleOutput) {
    std::string_view sv = "one,two,,three";
    SplitView sv_view(sv, ','); // Renamed to avoid conflict with the variable name
    std::vector<std::string_view> result_tokens;
    for (auto t : sv_view) {
        result_tokens.push_back(t);
    }
    compare_tokens(result_tokens, {"one", "two", "", "three"});
}

// Test for an input that is just the delimiter (string version)
TEST(SplitViewTest, InputIsJustStringDelimiter) {
    std::string_view sv = "DELIM";
    SplitView view(sv, "DELIM");
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"", ""});
}

// Test for an input that is just the delimiter (char version)
TEST(SplitViewTest, InputIsJustCharDelimiter) {
    std::string_view sv = ",";
    SplitView view(sv, ',');
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"", ""});
}


// Test for input like "a:b:c" from requirements example for CLI
TEST(SplitViewTest, CLIExampleSimple) {
    std::string_view sv = "a:b:c:d";
    SplitView args(sv, ':');
    std::vector<std::string_view> fields = collect_tokens(args);
    compare_tokens(fields, {"a", "b", "c", "d"});
}

// Test for input like "key=value"
TEST(SplitViewTest, KeyValueExample) {
    std::string_view line = "key=value";
    SplitView parts(line, '=');
    auto it = parts.begin();
    std::string_view key, val;

    ASSERT_NE(it, parts.end());
    key = *it;
    ++it;
    ASSERT_NE(it, parts.end());
    val = *it;
    ++it;
    ASSERT_EQ(it, parts.end());

    EXPECT_EQ(key, "key");
    EXPECT_EQ(val, "value");
}

TEST(SplitViewTest, KeyValueExampleOnlyKey) {
    std::string_view line = "key=";
    SplitView parts(line, '=');
    auto it = parts.begin();
    std::string_view key, val;

    ASSERT_NE(it, parts.end());
    key = *it; // "key"
    ++it;
    ASSERT_NE(it, parts.end());
    val = *it; // ""
    ++it;
    ASSERT_EQ(it, parts.end());

    EXPECT_EQ(key, "key");
    EXPECT_EQ(val, "");
}

TEST(SplitViewTest, KeyValueExampleOnlyValue) { // actually "=value"
    std::string_view line = "=value";
    SplitView parts(line, '=');
    auto it = parts.begin();
    std::string_view key, val;

    ASSERT_NE(it, parts.end());
    key = *it; // ""
    ++it;
    ASSERT_NE(it, parts.end());
    val = *it; // "value"
    ++it;
    ASSERT_EQ(it, parts.end());

    EXPECT_EQ(key, "");
    EXPECT_EQ(val, "value");
}

TEST(SplitViewTest, KeyValueExampleNoDelimiter) {
    std::string_view line = "keyvalue";
    SplitView parts(line, '=');
    auto it = parts.begin();
    std::string_view key, val;

    ASSERT_NE(it, parts.end());
    key = *it; // "keyvalue"
    ++it;
    ASSERT_EQ(it, parts.end()); // No second token

    EXPECT_EQ(key, "keyvalue");
    // val remains unassigned or default
}

TEST(SplitViewTest, MultipleIteratorsIndependent) {
    std::string_view sv = "1,2,3";
    SplitView view(sv, ',');

    auto it1 = view.begin();
    auto it2 = view.begin();

    EXPECT_EQ(*it1, "1");
    EXPECT_EQ(*it2, "1");

    ++it1;
    EXPECT_EQ(*it1, "2");
    EXPECT_EQ(*it2, "1"); // it2 should be unaffected

    ++it2;
    EXPECT_EQ(*it1, "2");
    EXPECT_EQ(*it2, "2"); // it2 caught up

    ++it1;
    EXPECT_EQ(*it1, "3");
    EXPECT_EQ(*it2, "2");

    auto it3 = view.begin();
    std::vector<std::string_view> path1, path2, path3;
    for(auto it = view.begin(); it != view.end(); ++it) path1.push_back(*it);
    for(auto val : view) path2.push_back(val); // Range-for uses begin()/end() each time

    it3 = view.begin(); // Reset it3
    path3.push_back(*it3++);
    path3.push_back(*it3++);
    path3.push_back(*it3++);

    compare_tokens(path1, {"1", "2", "3"});
    compare_tokens(path2, {"1", "2", "3"});
    compare_tokens(path3, {"1", "2", "3"});
}


// CORRECTED TEST CASES
// The original test cases had errors. Here are the corrected versions:

// Original failing test - CORRECTED:
TEST(SplitViewTest, StringDelimiterComplexCase) {
    // FIXED: Changed "axbya" to "axybya" to actually contain the "xy" delimiter
    std::string_view sv = "axybya";  // Was "axbya" - this actually contains "xy"
    SplitView view(sv, "xy");
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"a", "bya"});
}

// Alternative fix using single character delimiter:
TEST(SplitViewTest, StringDelimiterComplexCaseAlt) {
    // Or use single character delimiter which would work with original string
    std::string_view sv = "axbya";
    SplitView view(sv, 'x');  // Single char delimiter
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"a", "bya"});
}

// Original failing test - CORRECTED:
TEST(SplitViewTest, StringDelimiterExactMatchSeries) {
    // FIXED: Changed to "abaaba" to get the expected result {"", "", ""}
    std::string_view sv = "abaaba";  // Was "ababab" 
    SplitView view(sv, "aba");
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"", "", ""});
}

// If you want to keep the original "ababab" string, the expected result should be:
TEST(SplitViewTest, StringDelimiterExactMatchSeriesOriginal) {
    std::string_view sv = "ababab";
    SplitView view(sv, "aba");
    std::vector<std::string_view> tokens = collect_tokens(view);
    compare_tokens(tokens, {"", "bab"});  // Correct expectation for "ababab" split by "aba"
}

// Additional test cases to verify the implementation:
TEST(SplitViewTest, StringDelimiterVerification) {
    // Verify the "axbya" case works with correct delimiter
    std::string_view sv1 = "axbya";
    SplitView view1(sv1, "x");
    std::vector<std::string_view> tokens1 = collect_tokens(view1);
    compare_tokens(tokens1, {"a", "bya"});
    
    // Verify the "axybya" case works with "xy" delimiter  
    std::string_view sv2 = "axybya";
    SplitView view2(sv2, "xy");
    std::vector<std::string_view> tokens2 = collect_tokens(view2);
    compare_tokens(tokens2, {"a", "bya"});
    
    // Verify standard splitting behavior
    std::string_view sv3 = "a,b,c";
    SplitView view3(sv3, ",");
    std::vector<std::string_view> tokens3 = collect_tokens(view3);
    compare_tokens(tokens3, {"a", "b", "c"});
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#include "gtest/gtest.h"
#include "SuffixArray.h" // Assuming it's in the include path relative to tests or via CMake include_directories

#include <string>
#include <vector>
#include <algorithm> // For std::sort
#include <iostream>  // For debugging SA failures

// Helper function to print suffix for debugging
std::string get_suffix_for_print(const std::string& text, int index, int len = 30) {
    if (index < 0 || static_cast<size_t>(index) >= text.length()) {
        return "[invalid index]";
    }
    return text.substr(index, len) + (text.length() - index > static_cast<size_t>(len) ? "..." : "");
}

// Helper function to compare suffix array results
void ExpectSAEq(const SuffixArray& sa_obj, const std::string& text, const std::vector<int>& expected_sa_indices) {
    const auto& actual_sa_ref = sa_obj.get_array();
    ASSERT_EQ(expected_sa_indices.size(), actual_sa_ref.size()) << "SA size mismatch for text: \"" << text << "\"";
    for (size_t i = 0; i < expected_sa_indices.size(); ++i) {
        EXPECT_EQ(expected_sa_indices[i], actual_sa_ref[i])
            << "SA content mismatch at index " << i << " for text: \"" << text << "\". "
            << "Expected index: " << expected_sa_indices[i] << " ('" << get_suffix_for_print(text, expected_sa_indices[i]) << "'), "
            << "Got index: " << actual_sa_ref[i] << " ('" << get_suffix_for_print(text, actual_sa_ref[i]) << "').";
    }
    // Optionally, verify overall sort order based on actual_sa_ref
    for (size_t i = 0; i + 1 < actual_sa_ref.size(); ++i) {
        ASSERT_LE(text.substr(actual_sa_ref[i]), text.substr(actual_sa_ref[i+1]))
            << "Actual SA is not sorted correctly at SA index " << i << " vs " << i+1 <<". Suffixes: '"
            << get_suffix_for_print(text, actual_sa_ref[i]) << "' vs '"
            << get_suffix_for_print(text, actual_sa_ref[i+1]) << "'";
    }
}

TEST(SuffixArrayTest, EmptyString) {
    std::string text = "";
    SuffixArray sa(text);
    EXPECT_TRUE(sa.empty());
    EXPECT_EQ(0, sa.size());
    EXPECT_TRUE(sa.get_array().empty());
    EXPECT_EQ(0, sa.count_occurrences("a"));
    EXPECT_TRUE(sa.find_occurrences("a").empty());
    EXPECT_EQ(0, sa.count_occurrences("")); // Per current SuffixArray impl, empty pattern finds nothing
    EXPECT_TRUE(sa.find_occurrences("").empty());// Per current SuffixArray impl
}

TEST(SuffixArrayTest, SingleCharacter) {
    std::string text = "a";
    SuffixArray sa(text);
    EXPECT_FALSE(sa.empty());
    EXPECT_EQ(1, sa.size());
    ExpectSAEq(sa, text, {0});
    EXPECT_EQ(1, sa.count_occurrences("a"));
    EXPECT_EQ(std::vector<int>{0}, sa.find_occurrences("a"));
    EXPECT_EQ(0, sa.count_occurrences("b"));
    EXPECT_TRUE(sa.find_occurrences("b").empty());
}

TEST(SuffixArrayTest, RepeatedCharacters) {
    std::string text = "aaaaa";
    SuffixArray sa(text);
    EXPECT_EQ(5, sa.size());
    // Expected SA: "a", "aa", "aaa", "aaaa", "aaaaa"
    // Indices: 4, 3, 2, 1, 0
    ExpectSAEq(sa, text, {4, 3, 2, 1, 0});
    EXPECT_EQ(5, sa.count_occurrences("a"));
    EXPECT_EQ(std::vector<int>({0, 1, 2, 3, 4}), sa.find_occurrences("a")); // Sorted by index
    EXPECT_EQ(2, sa.count_occurrences("aaaa"));
    EXPECT_EQ(std::vector<int>({0, 1}), sa.find_occurrences("aaaa"));
    EXPECT_EQ(1, sa.count_occurrences("aaaaa"));
    EXPECT_EQ(std::vector<int>({0}), sa.find_occurrences("aaaaa"));
    EXPECT_EQ(0, sa.count_occurrences("b"));
    EXPECT_EQ(0, sa.count_occurrences("aaab"));
}

TEST(SuffixArrayTest, SimpleBanana) {
    std::string text = "banana";
    SuffixArray sa(text);
    // Expected suffixes sorted:
    // a        (5)
    // ana      (3)
    // anana    (1)
    // banana   (0)
    // na       (4)
    // nana     (2)
    std::vector<int> expected_sa_indices = {5, 3, 1, 0, 4, 2};
    ExpectSAEq(sa, text, expected_sa_indices);

    EXPECT_EQ(6, sa.size());
    EXPECT_FALSE(sa.empty());

    EXPECT_EQ(3, sa.count_occurrences("a"));
    std::vector<int> expected_a = {1, 3, 5};
    EXPECT_EQ(expected_a, sa.find_occurrences("a"));


    EXPECT_EQ(2, sa.count_occurrences("na"));
    std::vector<int> expected_na = {2, 4};
    EXPECT_EQ(expected_na, sa.find_occurrences("na"));

    EXPECT_EQ(1, sa.count_occurrences("banana"));
    EXPECT_EQ(std::vector<int>{0}, sa.find_occurrences("banana"));

    EXPECT_EQ(1, sa.count_occurrences("nana"));
    EXPECT_EQ(std::vector<int>{2}, sa.find_occurrences("nana"));

    EXPECT_EQ(0, sa.count_occurrences("bna"));
    EXPECT_TRUE(sa.find_occurrences("bna").empty());
    EXPECT_EQ(0, sa.count_occurrences("apple"));
    EXPECT_TRUE(sa.find_occurrences("apple").empty());
    EXPECT_EQ(0, sa.count_occurrences("bananarama"));
    EXPECT_TRUE(sa.find_occurrences("bananarama").empty());
}

TEST(SuffixArrayTest, Mississippi) {
    std::string text = "mississippi";
    SuffixArray sa(text);
    // Expected SA: 10, 7, 4, 1, 0, 9, 8, 6, 3, 5, 2
    // i (10) $
    // ippi (7) $
    // issippi (4) $
    // ississippi (1) $
    // mississippi (0) $
    // pi (9) $
    // ppi (8) $
    // sippi (6) $
    // sissippi (3) $
    // ssippi (5) $
    // ssissippi (2) $
    std::vector<int> expected_sa_indices = {10, 7, 4, 1, 0, 9, 8, 6, 3, 5, 2};
    ExpectSAEq(sa, text, expected_sa_indices);

    EXPECT_EQ(4, sa.count_occurrences("i"));
    std::vector<int> expected_i = {1, 4, 7, 10};
    EXPECT_EQ(expected_i, sa.find_occurrences("i"));

    EXPECT_EQ(2, sa.count_occurrences("issi"));
    std::vector<int> expected_issi = {1, 4};
    EXPECT_EQ(expected_issi, sa.find_occurrences("issi"));

    EXPECT_EQ(1, sa.count_occurrences("mississippi"));
    EXPECT_EQ(std::vector<int>{0}, sa.find_occurrences("mississippi"));

    EXPECT_EQ(0, sa.count_occurrences("apple"));
}


TEST(SuffixArrayTest, SearchNonExistent) {
    std::string text = "abcdef";
    SuffixArray sa(text);
    EXPECT_EQ(0, sa.count_occurrences("x"));
    EXPECT_TRUE(sa.find_occurrences("x").empty());
    EXPECT_EQ(0, sa.count_occurrences("acy"));
    EXPECT_TRUE(sa.find_occurrences("acy").empty());
    EXPECT_EQ(0, sa.count_occurrences("efg"));
    EXPECT_TRUE(sa.find_occurrences("efg").empty());
}

TEST(SuffixArrayTest, SearchPrefixAndSuffix) {
    std::string text = "abracadabra"; // Ends with 'a', 'ra', 'bra', 'abra', ...
    SuffixArray sa(text);
    // SA: 10, 7, 0, 3, 5, 8, 1, 4, 6, 9, 2
    // a (10) $
    // abra (7) $
    // abracadabra (0) $
    // aca (3) $
    // adabra (5) $
    // bra (8) $
    // bracadabra (1) $
    // cada (4) $
    // dabra (6) $
    // ra (9) $
    // racadabra (2) $
    std::vector<int> expected_sa_val = {10, 7, 0, 3, 5, 8, 1, 4, 6, 9, 2};
    ExpectSAEq(sa, text, expected_sa_val);


    EXPECT_EQ(2, sa.count_occurrences("abr")); // abr at 0, abracadabra; abr at 7, abra
    std::vector<int> expected_abr = {0, 7};
    EXPECT_EQ(expected_abr, sa.find_occurrences("abr"));

    EXPECT_EQ(2, sa.count_occurrences("bra"));
    std::vector<int> expected_bra = {1, 8};
    EXPECT_EQ(expected_bra, sa.find_occurrences("bra"));

    EXPECT_EQ(1, sa.count_occurrences("cadabra"));
    EXPECT_EQ(std::vector<int>{4}, sa.find_occurrences("cadabra"));
}

TEST(SuffixArrayTest, TextWithDollarSign) {
    std::string text = "banana$";
    SuffixArray sa(text);
    // Expected Suffixes sorted:
    // $        (6)
    // a$       (5)
    // ana$     (3)
    // anana$   (1)
    // banana$  (0)
    // na$      (4)
    // nana$    (2)
    std::vector<int> expected_sa_indices = {6, 5, 3, 1, 0, 4, 2};
    ExpectSAEq(sa, text, expected_sa_indices);

    EXPECT_EQ(1, sa.count_occurrences("$"));
    EXPECT_EQ(std::vector<int>{6}, sa.find_occurrences("$"));

    EXPECT_EQ(1, sa.count_occurrences("na$"));
    EXPECT_EQ(std::vector<int>{4}, sa.find_occurrences("na$"));
}

TEST(SuffixArrayTest, CaseSensitivity) {
    std::string text = "Apple";
    SuffixArray sa(text);
    // Sorted suffixes of "Apple":
    // "A"pple (0)
    // "e"     (4)
    // "le"    (3)
    // "ple"   (2)
    // "pple"  (1)
    std::vector<int> expected_sa_indices = {0, 4, 3, 2, 1};
    ExpectSAEq(sa, text, expected_sa_indices);

    EXPECT_EQ(1, sa.count_occurrences("A"));
    EXPECT_EQ(std::vector<int>{0}, sa.find_occurrences("A"));
    EXPECT_EQ(0, sa.count_occurrences("a"));
    EXPECT_TRUE(sa.find_occurrences("a").empty());
    EXPECT_EQ(1, sa.count_occurrences("Apple"));
    EXPECT_EQ(std::vector<int>{0}, sa.find_occurrences("Apple"));
    EXPECT_EQ(0, sa.count_occurrences("apple"));
}

TEST(SuffixArrayTest, FindOccurrencesReturnsSorted) {
    std::string text = "ababab";
    SuffixArray sa(text);
    // Suffixes of "ababab"
    // ababab (0)
    // babab  (1)
    // abab   (2)
    // bab    (3)
    // ab     (4)
    // b      (5)
    // Sorted (lexicographically):
    // ab     (4)
    // abab   (2)
    // ababab (0)
    // b      (5)
    // bab    (3)
    // babab  (1)
    std::vector<int> expected_sa_val = {4, 2, 0, 5, 3, 1};
    ExpectSAEq(sa, text, expected_sa_val);

    std::vector<int> actual_occurrences_ab = sa.find_occurrences("ab");
    std::vector<int> expected_occurrences_ab = {0, 2, 4}; // Must be sorted by index
    EXPECT_EQ(expected_occurrences_ab, actual_occurrences_ab);

    std::vector<int> actual_occurrences_b = sa.find_occurrences("b");
    std::vector<int> expected_occurrences_b = {1, 3, 5}; // Must be sorted by index
    EXPECT_EQ(expected_occurrences_b, actual_occurrences_b);
}


TEST(SuffixArrayTest, PatternLongerThanText) {
    std::string text = "short";
    SuffixArray sa(text);
    std::string pattern = "shorttext";
    EXPECT_EQ(0, sa.count_occurrences(pattern));
    EXPECT_TRUE(sa.find_occurrences(pattern).empty());
}

TEST(SuffixArrayTest, SpecialCharsInText) {
    std::string text = "a!b@c#a"; // ASCII: ! (33), # (35), @ (64), a (97), b (98), c (99)
    SuffixArray sa(text);
    // Suffixes:
    // a!b@c#a (0)
    // !b@c#a  (1)
    // b@c#a   (2)
    // @c#a    (3)
    // c#a     (4)
    // #a      (5)
    // a       (6)
    // Sorted:
    // !b@c#a  (1)
    // #a      (5)
    // @c#a    (3)
    // a       (6)
    // a!b@c#a (0)
    // b@c#a   (2)
    // c#a     (4)
    std::vector<int> expected_sa_indices = {1, 5, 3, 6, 0, 2, 4};
    ExpectSAEq(sa, text, expected_sa_indices);

    EXPECT_EQ(2, sa.count_occurrences("a"));
    std::vector<int> expected_a = {0,6};
    EXPECT_EQ(expected_a, sa.find_occurrences("a"));

    EXPECT_EQ(1, sa.count_occurrences("!b@"));
    EXPECT_EQ(std::vector<int>{1}, sa.find_occurrences("!b@"));
}

TEST(SuffixArrayTest, SubstringAtEndOfText) {
    std::string text = "testing"; // g, ing, sting, esting, ...
    SuffixArray sa(text);
    EXPECT_EQ(1, sa.count_occurrences("ing"));
    EXPECT_EQ(std::vector<int>{4}, sa.find_occurrences("ing"));
    EXPECT_EQ(1, sa.count_occurrences("g"));
    EXPECT_EQ(std::vector<int>{6}, sa.find_occurrences("g"));
}

TEST(SuffixArrayTest, OverlappingOccurrences) {
    std::string text = "aaaa";
    SuffixArray sa(text); // SA: 3,2,1,0
    EXPECT_EQ(3, sa.count_occurrences("aa")); // at 0, 1, 2
    std::vector<int> expected_aa = {0,1,2};
    EXPECT_EQ(expected_aa, sa.find_occurrences("aa"));

    std::string text2 = "ababa";
    SuffixArray sa2(text2); // SA: 4,2,0,3,1 (a, aba, ababa, b, ba)
    EXPECT_EQ(2, sa2.count_occurrences("aba")); // at 0, 2
    std::vector<int> expected_aba = {0,2};
    EXPECT_EQ(expected_aba, sa2.find_occurrences("aba"));
}

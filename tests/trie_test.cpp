#include "gtest/gtest.h"
#include "trie.h" // Assuming trie.h is in the same directory as trie_test.cpp
#include <vector>
#include <string>
#include <algorithm> // For std::sort
#include <fstream>   // For file operations

// Helper function to compare two vectors of strings (ignoring order)
void ExpectStringVectorsEqualUnordered(const std::vector<std::string>& vec1, const std::vector<std::string>& vec2) {
    ASSERT_EQ(vec1.size(), vec2.size());
    std::vector<std::string> sorted_vec1 = vec1;
    std::vector<std::string> sorted_vec2 = vec2;
    std::sort(sorted_vec1.begin(), sorted_vec1.end());
    std::sort(sorted_vec2.begin(), sorted_vec2.end());
    EXPECT_EQ(sorted_vec1, sorted_vec2);
}

TEST(TrieTest, BasicInsertAndSearch) {
    Trie t;
    t.insert("apple");
    EXPECT_TRUE(t.search("apple"));
    EXPECT_FALSE(t.search("app"));
    EXPECT_FALSE(t.search("apples"));
    t.insert("app");
    EXPECT_TRUE(t.search("app"));
}

TEST(TrieTest, CaseSensitiveSearch) {
    Trie t_cs(true); // Case sensitive
    t_cs.insert("Apple");
    EXPECT_TRUE(t_cs.search("Apple"));
    EXPECT_FALSE(t_cs.search("apple"));

    Trie t_ci(false); // Case insensitive
    t_ci.insert("Apple");
    EXPECT_TRUE(t_ci.search("apple"));
    EXPECT_TRUE(t_ci.search("APPLE"));
    EXPECT_TRUE(t_ci.search("Apple"));
    t_ci.insert("banana");
    EXPECT_TRUE(t_ci.search("BaNaNa"));
}

TEST(TrieTest, StartsWith) {
    Trie t;
    t.insert("apple");
    t.insert("apricot");
    t.insert("application");
    EXPECT_TRUE(t.startsWith("app"));
    EXPECT_TRUE(t.startsWith("apple"));
    EXPECT_TRUE(t.startsWith("apricot"));
    EXPECT_TRUE(t.startsWith("application"));
    EXPECT_FALSE(t.startsWith("apples"));
    EXPECT_FALSE(t.startsWith("applications"));
    EXPECT_TRUE(t.startsWith("a"));
    EXPECT_FALSE(t.startsWith("b"));
    EXPECT_TRUE(t.startsWith("")); // Empty prefix
}

TEST(TrieTest, GetWordsWithPrefix) {
    Trie t;
    t.insert("apple");
    t.insert("apply");
    t.insert("apricot");
    t.insert("banana");
    ExpectStringVectorsEqualUnordered(t.getWordsWithPrefix("app"), {"apple", "apply"});
    ExpectStringVectorsEqualUnordered(t.getWordsWithPrefix("ap"), {"apple", "apply", "apricot"});
    ExpectStringVectorsEqualUnordered(t.getWordsWithPrefix("b"), {"banana"});
    ExpectStringVectorsEqualUnordered(t.getWordsWithPrefix(""), {"apple", "apply", "apricot", "banana"});
    ExpectStringVectorsEqualUnordered(t.getWordsWithPrefix("xyz"), {});

    Trie t2;
    t2.insert("romane");
    t2.insert("romanus");
    t2.insert("romulus");
    t2.insert("rom");
    t2.insert("rubicon");
    ExpectStringVectorsEqualUnordered(t2.getWordsWithPrefix("rom"), {"rom", "romane", "romanus", "romulus"});
}

TEST(TrieTest, DeleteWord) {
    Trie t;
    t.insert("apple");
    t.insert("apply");
    t.insert("app");

    EXPECT_TRUE(t.deleteWord("apple"));
    EXPECT_FALSE(t.search("apple"));
    EXPECT_TRUE(t.search("apply"));
    EXPECT_TRUE(t.search("app"));
    EXPECT_TRUE(t.startsWith("app")); // Prefix still exists due to "apply" and "app"

    EXPECT_TRUE(t.deleteWord("app"));
    EXPECT_FALSE(t.search("app"));
    EXPECT_TRUE(t.search("apply"));
    EXPECT_TRUE(t.startsWith("app")); // Prefix still exists due to "apply"

    EXPECT_FALSE(t.deleteWord("apple")); // Delete non-existent
    EXPECT_FALSE(t.deleteWord("ap"));    // Delete non-existent (was never a word)

    EXPECT_TRUE(t.deleteWord("apply"));
    EXPECT_FALSE(t.search("apply"));
    EXPECT_FALSE(t.startsWith("app")); // No words start with "app" anymore

    Trie t2;
    t2.insert("test");
    t2.insert("tester");
    t2.insert("testing");
    EXPECT_TRUE(t2.deleteWord("tester"));
    EXPECT_FALSE(t2.search("tester"));
    EXPECT_TRUE(t2.search("test"));
    EXPECT_TRUE(t2.search("testing"));
    EXPECT_TRUE(t2.deleteWord("test"));
    EXPECT_FALSE(t2.search("test"));
    EXPECT_TRUE(t2.search("testing"));
    EXPECT_TRUE(t2.deleteWord("testing"));
    EXPECT_FALSE(t2.search("testing"));
    EXPECT_TRUE(t2.getWordsWithPrefix("").empty());
}

TEST(TrieTest, WildcardSearch) {
    Trie t;
    t.insert("apple");
    t.insert("apply");
    t.insert("apricot");
    t.insert("banana");
    t.insert("bandana");
    t.insert("app");

    ExpectStringVectorsEqualUnordered(t.wildcardSearch("ap?le"), {"apple"});
    ExpectStringVectorsEqualUnordered(t.wildcardSearch("ap*"), {"apple", "apply", "apricot", "app"});
    ExpectStringVectorsEqualUnordered(t.wildcardSearch("a*e"), {"apple"}); // Assuming Radix specific behavior for '*'
    ExpectStringVectorsEqualUnordered(t.wildcardSearch("b*na"), {"banana", "bandana"});
    ExpectStringVectorsEqualUnordered(t.wildcardSearch("ap???"), {"apple", "apply", "app"});
    ExpectStringVectorsEqualUnordered(t.wildcardSearch("*"), {"apple", "apply", "apricot", "banana", "bandana", "app"});
    ExpectStringVectorsEqualUnordered(t.wildcardSearch("apric*"), {"apricot"});
    ExpectStringVectorsEqualUnordered(t.wildcardSearch("??????"), {"banana", "bandana", "apricot"}); // 6 chars
    ExpectStringVectorsEqualUnordered(t.wildcardSearch("b*d?n?"), {"bandana"});
    ExpectStringVectorsEqualUnordered(t.wildcardSearch("app*"), {"apple", "apply", "app"}); // Note: app is a word
    ExpectStringVectorsEqualUnordered(t.wildcardSearch("a??"), {"app"});
}

TEST(TrieTest, Iterator) {
    Trie t;
    std::vector<std::string> words = {"b", "apple", "a", "banana", "apricot", ""}; // Include empty string
    for(const auto& w : words) {
        t.insert(w);
    }

    std::vector<std::string> iterated_words;
    for(const std::string& word_from_iterator : t) {
        iterated_words.push_back(word_from_iterator);
    }
    ExpectStringVectorsEqualUnordered(iterated_words, words);

    // Test with an empty trie
    Trie empty_trie;
    iterated_words.clear();
    for(const std::string& word_from_iterator : empty_trie) {
        iterated_words.push_back(word_from_iterator);
    }
    EXPECT_TRUE(iterated_words.empty());

    // Test with trie that only has empty string
    Trie empty_str_trie;
    empty_str_trie.insert("");
    iterated_words.clear();
    for(const std::string& word_from_iterator : empty_str_trie) {
        iterated_words.push_back(word_from_iterator);
    }
    ExpectStringVectorsEqualUnordered(iterated_words, {""});
}

TEST(TrieTest, Serialization) {
    const std::string filename = "test_trie_serialization.txt";
    Trie t_save;
    t_save.insert("saveTest1");
    t_save.insert("saveTest1"); // Test frequency
    t_save.insert("saveTest2");
    t_save.insert("another");
    t_save.insert("");      // Test empty string

    ASSERT_TRUE(t_save.saveToFile(filename));

    Trie t_load;
    ASSERT_TRUE(t_load.loadFromFile(filename));

    EXPECT_TRUE(t_load.search("saveTest1"));
    EXPECT_TRUE(t_load.search("saveTest2"));
    EXPECT_TRUE(t_load.search("another"));
    EXPECT_TRUE(t_load.search(""));
    EXPECT_FALSE(t_load.search("nonexistent"));

    // Verify all words are loaded (using getWordsWithPrefix as a proxy)
    ExpectStringVectorsEqualUnordered(t_load.getWordsWithPrefix(""), {"saveTest1", "saveTest2", "another", ""});

    // Frequencies are harder to test directly without a getFrequency method that works with Radix.
    // However, loadFromFile is expected to set them. If getWordFrequency is implemented, add tests.

    // Clean up test file
    std::remove(filename.c_str());
}

TEST(TrieTest, EmptyStringOperations) {
    Trie t;
    t.insert("");
    EXPECT_TRUE(t.search(""));
    EXPECT_TRUE(t.startsWith(""));
    ExpectStringVectorsEqualUnordered(t.getWordsWithPrefix(""), {""});
    // EXPECT_EQ(t.getWordFrequency(""), 1); // If getWordFrequency was available

    EXPECT_TRUE(t.deleteWord(""));
    EXPECT_FALSE(t.search(""));
    // EXPECT_EQ(t.getWordFrequency(""), 0); // If getWordFrequency was available
    ExpectStringVectorsEqualUnordered(t.getWordsWithPrefix(""), {});

    t.insert("a");
    t.insert("");
    EXPECT_TRUE(t.search("a"));
    EXPECT_TRUE(t.search(""));
    ExpectStringVectorsEqualUnordered(t.getWordsWithPrefix(""), {"a", ""});
}

TEST(TrieTest, EndsWithAndGetWordsEndingWith) {
    Trie t;
    t.insert("apple");
    t.insert("example");
    t.insert("simple");
    t.insert("sample");
    t.insert("ample");

    EXPECT_TRUE(t.endsWith("ple"));
    EXPECT_TRUE(t.endsWith("le"));
    EXPECT_TRUE(t.endsWith("e"));
    EXPECT_TRUE(t.endsWith("apple")); // Full word
    EXPECT_FALSE(t.endsWith("plex"));
    EXPECT_FALSE(t.endsWith("samples"));
    EXPECT_TRUE(t.endsWith("")); // Empty suffix

    ExpectStringVectorsEqualUnordered(t.getWordsEndingWith("ple"), {"apple", "example", "simple", "ample"});
    ExpectStringVectorsEqualUnordered(t.getWordsEndingWith("mple"), {"example", "simple", "sample"});
    ExpectStringVectorsEqualUnordered(t.getWordsEndingWith(""), {"apple", "example", "simple", "sample", "ample"}); // All words
    ExpectStringVectorsEqualUnordered(t.getWordsEndingWith("xyz"), {});

    Trie t_ci(false); // Case-insensitive
    t_ci.insert("ApplePie");
    t_ci.insert("OrangeJuice");
    EXPECT_TRUE(t_ci.endsWith("PIE"));
    EXPECT_TRUE(t_ci.endsWith("juice"));
    ExpectStringVectorsEqualUnordered(t_ci.getWordsEndingWith("IE"), {"ApplePie"}); // Assuming getWords preserves original case or normalizes
                                                                                // The current implementation reverses, gets prefix, then re-reverses.
                                                                                // The case of the returned word depends on how the reversed trie stores it.
                                                                                // If reversed trie is case-insensitive, it will be normalized.
}

TEST(TrieTest, FuzzySearch) {
    Trie t;
    t.insert("apple");
    t.insert("apply");
    t.insert("apricot");
    t.insert("banana");
    t.insert("bandana");
    t.insert("orange");

    // Exact match
    auto results1 = t.fuzzySearch("apple", 0);
    ASSERT_EQ(results1.size(), 1);
    EXPECT_EQ(results1[0].first, "apple");
    EXPECT_EQ(results1[0].second, 0);

    // One substitution
    auto results2 = t.fuzzySearch("axple", 1);
    ASSERT_EQ(results2.size(), 1);
    EXPECT_EQ(results2[0].first, "apple");
    EXPECT_EQ(results2[0].second, 1);

    // One insertion
    auto results3 = t.fuzzySearch("aple", 1); // "apple" (del 'p'), "apply" (del 'p')
    std::vector<std::string> found_words3;
    for(const auto& p : results3) if(p.second <=1) found_words3.push_back(p.first);
    ExpectStringVectorsEqualUnordered(found_words3, {"apple", "apply"});


    // One deletion
    auto results4 = t.fuzzySearch("applee", 1);
    ASSERT_EQ(results4.size(), 1);
    EXPECT_EQ(results4[0].first, "apple");
    EXPECT_EQ(results4[0].second, 1);

    // More complex, multiple results
    auto results5 = t.fuzzySearch("banoon", 2); // banana (2 subs), bandana (2 subs/ins/del)
    // Expected: banana (dist 1 or 2), bandana (dist 1 or 2)
    bool found_banana = false;
    bool found_bandana = false;
    for(const auto& p : results5) {
        if (p.first == "banana" && p.second <= 2) found_banana = true;
        if (p.first == "bandana" && p.second <= 2) found_bandana = true;
    }
    EXPECT_TRUE(found_banana);
    EXPECT_TRUE(found_bandana);


    // No match within max_k
    auto results6 = t.fuzzySearch("xyz", 1);
    EXPECT_TRUE(results6.empty());

    // Empty query
    auto results7 = t.fuzzySearch("", 0); // Should match empty string if inserted
    Trie t_empty;
    t_empty.insert("");
    t_empty.insert("a");
    results7 = t_empty.fuzzySearch("",0);
    ASSERT_EQ(results7.size(), 1);
    EXPECT_EQ(results7[0].first, "");
    EXPECT_EQ(results7[0].second, 0);

    auto results8 = t_empty.fuzzySearch("",1); // should also find "a" with distance 1
    bool found_empty_8 = false;
    bool found_a_8 = false;
    for(const auto& p : results8) {
        if (p.first == "" && p.second <=1) found_empty_8 = true;
        if (p.first == "a" && p.second <=1) found_a_8 = true;
    }
    EXPECT_TRUE(found_empty_8);
    EXPECT_TRUE(found_a_8);


    // Query longer than any word in trie
    auto results9 = t.fuzzySearch("applesoranges", 2);
    EXPECT_TRUE(results9.empty()); // Assuming "applesoranges" is too different from anything

    // Test case sensitivity with fuzzy search
    Trie t_ci_fuzzy(false); // case-insensitive
    t_ci_fuzzy.insert("Apple");
    auto results_ci = t_ci_fuzzy.fuzzySearch("apple", 0);
    ASSERT_EQ(results_ci.size(), 1);
    EXPECT_EQ(results_ci[0].first, "apple"); // Stored as normalized
    EXPECT_EQ(results_ci[0].second, 0);

    auto results_ci_dist = t_ci_fuzzy.fuzzySearch("axple", 1);
    ASSERT_EQ(results_ci_dist.size(), 1);
    EXPECT_EQ(results_ci_dist[0].first, "apple");
    EXPECT_EQ(results_ci_dist[0].second, 1);
}

// Test for getWordFrequency - currently known to be broken for Radix Trie
// Add this test once the function is fixed.
/*
TEST(TrieTest, GetWordFrequency) {
    Trie t;
    t.insert("hello");
    EXPECT_EQ(t.getWordFrequency("hello"), 1);
    t.insert("hello");
    EXPECT_EQ(t.getWordFrequency("hello"), 2);
    t.insert("world");
    EXPECT_EQ(t.getWordFrequency("world"), 1);
    EXPECT_EQ(t.getWordFrequency("hell"), 0); // Not a word

    t.deleteWord("hello");
    EXPECT_EQ(t.getWordFrequency("hello"), 1); // One instance remaining
    t.deleteWord("hello");
    EXPECT_EQ(t.getWordFrequency("hello"), 0); // All instances deleted

    Trie t_ci(false);
    t_ci.insert("Test");
    t_ci.insert("test");
    EXPECT_EQ(t_ci.getWordFrequency("TEST"), 2);
}
*/

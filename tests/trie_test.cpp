#include "trie.h"
#include <gtest/gtest.h>

// Test fixture for Trie tests
class TrieTest : public ::testing::Test {
protected:
    Trie trie;
};

// Test case for insertion and search of a single word
TEST_F(TrieTest, InsertAndSearchSingleWord) {
    trie.insert("hello");
    ASSERT_TRUE(trie.search("hello"));
    ASSERT_FALSE(trie.search("he"));
    ASSERT_FALSE(trie.search("helloworld"));
    ASSERT_FALSE(trie.search(""));
}

// Test case for inserting multiple words
TEST_F(TrieTest, InsertMultipleWords) {
    trie.insert("apple");
    trie.insert("app");
    trie.insert("apricot");
    ASSERT_TRUE(trie.search("apple"));
    ASSERT_TRUE(trie.search("app"));
    ASSERT_TRUE(trie.search("apricot"));
    ASSERT_FALSE(trie.search("apples"));
}

// Test case for startsWith functionality
TEST_F(TrieTest, StartsWithPrefix) {
    trie.insert("apple");
    trie.insert("app");
    trie.insert("apricot");
    trie.insert("banana");

    ASSERT_TRUE(trie.startsWith("a"));
    ASSERT_TRUE(trie.startsWith("ap"));
    ASSERT_TRUE(trie.startsWith("app"));
    ASSERT_TRUE(trie.startsWith("appl"));
    ASSERT_TRUE(trie.startsWith("apple"));
    ASSERT_TRUE(trie.startsWith("b"));
    ASSERT_TRUE(trie.startsWith("ban"));
    ASSERT_TRUE(trie.startsWith("banana"));

    ASSERT_FALSE(trie.startsWith("c"));
    ASSERT_FALSE(trie.startsWith("appo"));
    ASSERT_FALSE(trie.startsWith("bananas"));

    // Behavior of startsWith("")
    // If trie is not empty, startsWith("") should be true.
    ASSERT_TRUE(trie.startsWith(""));
}

// Test case for searching non-existent words
TEST_F(TrieTest, SearchNonExistentWords) {
    trie.insert("apple");
    ASSERT_FALSE(trie.search("orange"));
    ASSERT_FALSE(trie.search("applet"));
}

// Test case for inserting the same word multiple times
TEST_F(TrieTest, InsertSameWordMultipleTimes) {
    trie.insert("hello");
    trie.insert("hello");
    ASSERT_TRUE(trie.search("hello"));
}

// Test case for empty string operations
TEST_F(TrieTest, EmptyStringOperations) {
    trie.insert("");
    ASSERT_TRUE(trie.search(""));
    ASSERT_TRUE(trie.startsWith("")); // Root is marked as end of word, and it's a valid prefix start

    Trie trie2; // New empty trie
    ASSERT_FALSE(trie2.search("")); // "" not inserted
    ASSERT_TRUE(trie2.startsWith("")); // Empty prefix from root is always true by current design
}

// Test for startsWith on an empty trie
TEST_F(TrieTest, StartsWithEmptyTrie) {
    // trie is empty here
    ASSERT_FALSE(trie.startsWith("a"));
    ASSERT_TRUE(trie.startsWith("")); // Empty prefix is true by current design for any trie
}


// Test for inserting words with common prefixes
TEST_F(TrieTest, WordsWithCommonPrefixes) {
    trie.insert("team");
    trie.insert("tea");
    trie.insert("te");

    ASSERT_TRUE(trie.search("team"));
    ASSERT_TRUE(trie.search("tea"));
    ASSERT_TRUE(trie.search("te"));

    ASSERT_TRUE(trie.startsWith("t"));
    ASSERT_TRUE(trie.startsWith("te"));
    ASSERT_TRUE(trie.startsWith("tea"));
    ASSERT_TRUE(trie.startsWith("team"));

    ASSERT_FALSE(trie.search("teams"));
    ASSERT_FALSE(trie.startsWith("teams"));
    ASSERT_FALSE(trie.startsWith("tex"));
}

// Test with longer words and more complex scenarios
TEST_F(TrieTest, ComplexScenario) {
    trie.insert("testing");
    trie.insert("test");
    trie.insert("tester");
    trie.insert("temporary");

    ASSERT_TRUE(trie.search("test"));
    ASSERT_TRUE(trie.search("testing"));
    ASSERT_TRUE(trie.search("tester"));
    ASSERT_TRUE(trie.search("temporary"));

    ASSERT_FALSE(trie.search("temp")); // "temp" is a prefix, not a full word
    ASSERT_TRUE(trie.startsWith("temp"));

    trie.insert("temp");
    ASSERT_TRUE(trie.search("temp"));

    ASSERT_FALSE(trie.search("tes")); // "tes" is a prefix
    ASSERT_TRUE(trie.startsWith("tes"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

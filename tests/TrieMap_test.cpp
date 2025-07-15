#include "gtest/gtest.h"
#include "TrieMap.h"

TEST(TrieMapTest, InsertAndFind) {
    TrieMap<int> trie;
    trie.insert("apple", 1);
    trie.insert("apply", 2);

    EXPECT_EQ(trie.find("apple").value(), 1);
    EXPECT_EQ(trie.find("apply").value(), 2);
    EXPECT_FALSE(trie.find("app").has_value());
    EXPECT_FALSE(trie.find("apples").has_value());
}

TEST(TrieMapTest, Contains) {
    TrieMap<int> trie;
    trie.insert("apple", 1);

    EXPECT_TRUE(trie.contains("apple"));
    EXPECT_FALSE(trie.contains("app"));
}

TEST(TrieMapTest, StartsWith) {
    TrieMap<int> trie;
    trie.insert("apple", 1);
    trie.insert("apply", 2);

    EXPECT_TRUE(trie.starts_with("app"));
    EXPECT_TRUE(trie.starts_with("appl"));
    EXPECT_TRUE(trie.starts_with("apple"));
    EXPECT_TRUE(trie.starts_with("apply"));
    EXPECT_FALSE(trie.starts_with("apples"));
}

TEST(TrieMapTest, GetKeysWithPrefix) {
    TrieMap<int> trie;
    trie.insert("apple", 1);
    trie.insert("apply", 2);
    trie.insert("apples", 3);
    trie.insert("orange", 4);

    std::vector<std::string> keys = trie.get_keys_with_prefix("app");
    std::sort(keys.begin(), keys.end());

    std::vector<std::string> expected_keys = {"apple", "apples", "apply"};
    EXPECT_EQ(keys, expected_keys);
}

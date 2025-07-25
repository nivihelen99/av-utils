#include "flat_map.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

TEST(FlatMapTest, Constructor) {
    cpp_collections::flat_map<int, std::string> map;
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);
}

TEST(FlatMapTest, Insert) {
    cpp_collections::flat_map<int, std::string> map;
    auto result = map.insert({1, "one"});
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->first, 1);
    EXPECT_EQ(result.first->second, "one");
    EXPECT_EQ(map.size(), 1);

    result = map.insert({1, "another one"});
    EXPECT_FALSE(result.second);
    EXPECT_EQ(result.first->first, 1);
    EXPECT_EQ(result.first->second, "one");
    EXPECT_EQ(map.size(), 1);
}

TEST(FlatMapTest, OperatorBracket) {
    cpp_collections::flat_map<int, std::string> map;
    map[1] = "one";
    EXPECT_EQ(map[1], "one");
    EXPECT_EQ(map.size(), 1);

    map[1] = "another one";
    EXPECT_EQ(map[1], "another one");
    EXPECT_EQ(map.size(), 1);
}

TEST(FlatMapTest, At) {
    cpp_collections::flat_map<int, std::string> map;
    map[1] = "one";
    EXPECT_EQ(map.at(1), "one");
    EXPECT_THROW(map.at(2), std::out_of_range);

    const auto& cmap = map;
    EXPECT_EQ(cmap.at(1), "one");
    EXPECT_THROW(cmap.at(2), std::out_of_range);
}

TEST(FlatMapTest, Find) {
    cpp_collections::flat_map<int, std::string> map;
    map[1] = "one";
    auto it = map.find(1);
    EXPECT_NE(it, map.end());
    EXPECT_EQ(it->second, "one");

    it = map.find(2);
    EXPECT_EQ(it, map.end());
}

TEST(FlatMapTest, Erase) {
    cpp_collections::flat_map<int, std::string> map;
    map[1] = "one";
    map[2] = "two";
    EXPECT_EQ(map.erase(1), 1);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map.find(1), map.end());
    EXPECT_EQ(map.erase(3), 0);
}

TEST(FlatMapTest, Clear) {
    cpp_collections::flat_map<int, std::string> map;
    map[1] = "one";
    map[2] = "two";
    map.clear();
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);
}

TEST(FlatMapTest, RangeConstructor) {
    std::vector<std::pair<int, std::string>> data = {{3, "three"}, {1, "one"}, {2, "two"}};
    cpp_collections::flat_map<int, std::string> map(data.begin(), data.end());
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map.at(1), "one");
    EXPECT_EQ(map.at(2), "two");
    EXPECT_EQ(map.at(3), "three");
}

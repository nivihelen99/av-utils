#include "gtest/gtest.h"
#include "GroupedSet.h" // Adjust path as necessary
#include <string>
#include <vector>
#include <set>
#include <algorithm> // For std::sort for vector comparison

// Define types for convenience in tests
using Item = std::string;
using Group = std::string;
using TestGroupedSet = cpp_collections::GroupedSet<Item, Group>;
using ItemSet = std::set<Item>;
using GroupSet = std::set<Group>;

// Helper to compare vectors by content regardless of order (after sorting)
template<typename T>
void EXPECT_VECTORS_EQ_UNORDERED(std::vector<T> vec1, std::vector<T> vec2) {
    std::sort(vec1.begin(), vec1.end());
    std::sort(vec2.begin(), vec2.end());
    EXPECT_EQ(vec1, vec2);
}

TEST(GroupedSetTest, InitialState) {
    TestGroupedSet gs;
    EXPECT_TRUE(gs.empty());
    EXPECT_EQ(gs.size(), 0);
    EXPECT_EQ(gs.group_count(), 0);
    EXPECT_TRUE(gs.get_all_items().empty());
    EXPECT_TRUE(gs.get_all_groups().empty());
    EXPECT_TRUE(gs.get_ungrouped_items().empty());
}

TEST(GroupedSetTest, AddItem) {
    TestGroupedSet gs;
    EXPECT_TRUE(gs.add_item("item1"));
    EXPECT_FALSE(gs.empty());
    EXPECT_EQ(gs.size(), 1);
    EXPECT_TRUE(gs.item_exists("item1"));
    EXPECT_FALSE(gs.item_exists("item2"));
    EXPECT_FALSE(gs.add_item("item1")); // Already exists
    EXPECT_EQ(gs.size(), 1);

    ItemSet expected_all = {"item1"};
    EXPECT_EQ(gs.get_all_items(), expected_all);
    ItemSet expected_ungrouped = {"item1"};
    EXPECT_EQ(gs.get_ungrouped_items(), expected_ungrouped);
}

TEST(GroupedSetTest, AddItemToGroup) {
    TestGroupedSet gs;
    EXPECT_TRUE(gs.add_item_to_group("item1", "groupA"));

    EXPECT_TRUE(gs.item_exists("item1"));
    EXPECT_TRUE(gs.group_exists("groupA"));
    EXPECT_TRUE(gs.is_item_in_group("item1", "groupA"));
    EXPECT_EQ(gs.size(), 1);
    EXPECT_EQ(gs.group_count(), 1);
    EXPECT_EQ(gs.items_in_group_count("groupA"), 1);
    EXPECT_EQ(gs.groups_for_item_count("item1"), 1);

    ItemSet expected_group_a = {"item1"};
    EXPECT_EQ(gs.get_items_in_group("groupA"), expected_group_a);
    GroupSet expected_item1_groups = {"groupA"};
    EXPECT_EQ(gs.get_groups_for_item("item1"), expected_item1_groups);
    EXPECT_TRUE(gs.get_ungrouped_items().empty()); // item1 is now grouped

    // Add same item to another group
    EXPECT_TRUE(gs.add_item_to_group("item1", "groupB"));
    EXPECT_EQ(gs.groups_for_item_count("item1"), 2);
    GroupSet expected_item1_groups_updated = {"groupA", "groupB"};
    EXPECT_EQ(gs.get_groups_for_item("item1"), expected_item1_groups_updated);

    // Add new item to existing group
    EXPECT_TRUE(gs.add_item_to_group("item2", "groupA"));
    EXPECT_EQ(gs.items_in_group_count("groupA"), 2);
    ItemSet expected_group_a_updated = {"item1", "item2"};
    EXPECT_EQ(gs.get_items_in_group("groupA"), expected_group_a_updated);

    // Add item that was previously only in all_items_
    gs.add_item("item3");
    EXPECT_TRUE(gs.add_item_to_group("item3", "groupC"));
    EXPECT_TRUE(gs.is_item_in_group("item3", "groupC"));
    EXPECT_EQ(gs.groups_for_item_count("item3"), 1);

    // Try adding item already in group
    EXPECT_FALSE(gs.add_item_to_group("item1", "groupA"));
}

TEST(GroupedSetTest, RemoveItemFromGroup) {
    TestGroupedSet gs;
    gs.add_item_to_group("item1", "groupA");
    gs.add_item_to_group("item1", "groupB");
    gs.add_item_to_group("item2", "groupA");

    EXPECT_TRUE(gs.remove_item_from_group("item1", "groupA"));
    EXPECT_FALSE(gs.is_item_in_group("item1", "groupA"));
    EXPECT_TRUE(gs.is_item_in_group("item1", "groupB")); // Still in groupB
    EXPECT_EQ(gs.items_in_group_count("groupA"), 1);     // item2 still in groupA
    EXPECT_EQ(gs.groups_for_item_count("item1"), 1);

    // Item becomes ungrouped if removed from its only group
    gs.add_item_to_group("item3", "groupC");
    EXPECT_TRUE(gs.remove_item_from_group("item3", "groupC"));
    EXPECT_EQ(gs.groups_for_item_count("item3"), 0);
    ItemSet expected_ungrouped = {"item3"};
    EXPECT_EQ(gs.get_ungrouped_items(), expected_ungrouped);


    EXPECT_FALSE(gs.remove_item_from_group("item1", "groupNonExistent"));
    EXPECT_FALSE(gs.remove_item_from_group("itemNonExistent", "groupA"));
    EXPECT_FALSE(gs.remove_item_from_group("item2", "groupB")); // item2 not in groupB
}

TEST(GroupedSetTest, RemoveItem) {
    TestGroupedSet gs;
    gs.add_item_to_group("item1", "groupA");
    gs.add_item_to_group("item1", "groupB");
    gs.add_item_to_group("item2", "groupA");
    gs.add_item("item3"); // Ungrouped item

    EXPECT_TRUE(gs.remove_item("item1"));
    EXPECT_FALSE(gs.item_exists("item1"));
    EXPECT_FALSE(gs.is_item_in_group("item1", "groupA"));
    EXPECT_FALSE(gs.is_item_in_group("item1", "groupB"));
    EXPECT_EQ(gs.items_in_group_count("groupA"), 1); // item2 still there
    EXPECT_EQ(gs.groups_for_item_count("item1"), 0);
    EXPECT_EQ(gs.size(), 2); // item2 and item3 remain

    EXPECT_TRUE(gs.remove_item("item3")); // Remove ungrouped item
    EXPECT_FALSE(gs.item_exists("item3"));
    EXPECT_EQ(gs.size(), 1);

    EXPECT_FALSE(gs.remove_item("itemNonExistent"));
}

TEST(GroupedSetTest, RemoveGroup) {
    TestGroupedSet gs;
    gs.add_item_to_group("item1", "groupA");
    gs.add_item_to_group("item1", "groupB"); // item1 in A and B
    gs.add_item_to_group("item2", "groupA"); // item2 in A
    gs.add_item_to_group("item3", "groupC"); // item3 in C

    EXPECT_TRUE(gs.remove_group("groupA"));
    EXPECT_FALSE(gs.group_exists("groupA"));
    EXPECT_FALSE(gs.is_item_in_group("item1", "groupA"));
    EXPECT_TRUE(gs.is_item_in_group("item1", "groupB")); // Still in groupB
    EXPECT_EQ(gs.groups_for_item_count("item1"), 1);
    EXPECT_FALSE(gs.is_item_in_group("item2", "groupA"));
    EXPECT_EQ(gs.groups_for_item_count("item2"), 0); // item2 was only in groupA
    EXPECT_EQ(gs.group_count(), 2); // groupB and groupC remain

    ItemSet expected_ungrouped_after_A_removed = {"item2"}; // item2 becomes ungrouped
    EXPECT_EQ(gs.get_ungrouped_items(), expected_ungrouped_after_A_removed);


    EXPECT_FALSE(gs.remove_group("groupNonExistent"));
}

TEST(GroupedSetTest, Clear) {
    TestGroupedSet gs;
    gs.add_item_to_group("item1", "groupA");
    gs.add_item_to_group("item2", "groupB");
    gs.clear();

    EXPECT_TRUE(gs.empty());
    EXPECT_EQ(gs.size(), 0);
    EXPECT_EQ(gs.group_count(), 0);
    EXPECT_FALSE(gs.item_exists("item1"));
    EXPECT_FALSE(gs.group_exists("groupA"));
}

TEST(GroupedSetTest, QueryMethods) {
    TestGroupedSet gs;
    gs.add_item_to_group("apple", "fruit");
    gs.add_item_to_group("banana", "fruit");
    gs.add_item_to_group("carrot", "vegetable");
    gs.add_item_to_group("apple", "red");
    gs.add_item_to_group("carrot", "orange");
    gs.add_item("brocolli"); // Ungrouped

    // get_all_items
    ItemSet expected_all = {"apple", "banana", "carrot", "brocolli"};
    EXPECT_EQ(gs.get_all_items(), expected_all);

    // get_all_groups
    std::vector<Group> expected_groups_vec = {"fruit", "vegetable", "red", "orange"};
    EXPECT_VECTORS_EQ_UNORDERED(gs.get_all_groups(), expected_groups_vec);

    // get_items_in_group
    ItemSet expected_fruit = {"apple", "banana"};
    EXPECT_EQ(gs.get_items_in_group("fruit"), expected_fruit);
    EXPECT_TRUE(gs.get_items_in_group("non_existent_group").empty());

    // get_groups_for_item
    GroupSet expected_apple_groups = {"fruit", "red"};
    EXPECT_EQ(gs.get_groups_for_item("apple"), expected_apple_groups);
    EXPECT_TRUE(gs.get_groups_for_item("non_existent_item").empty());
    EXPECT_TRUE(gs.get_groups_for_item("brocolli").empty());


    // get_ungrouped_items
    ItemSet expected_ungrouped = {"brocolli"};
    EXPECT_EQ(gs.get_ungrouped_items(), expected_ungrouped);
}

TEST(GroupedSetTest, AdvancedQueryMethods) {
    TestGroupedSet gs;
    // Common items for intersection/union tests
    gs.add_item_to_group("itemA", "group1");
    gs.add_item_to_group("itemA", "group2");
    gs.add_item_to_group("itemA", "group3");

    gs.add_item_to_group("itemB", "group1");
    gs.add_item_to_group("itemB", "group2");

    gs.add_item_to_group("itemC", "group1");

    gs.add_item_to_group("itemD", "group3");
    gs.add_item_to_group("itemE", "group4"); // Only in group4

    // get_items_in_all_groups
    std::vector<Group> g1_g2 = {"group1", "group2"};
    ItemSet expected_in_g1_g2 = {"itemA", "itemB"};
    EXPECT_EQ(gs.get_items_in_all_groups(g1_g2), expected_in_g1_g2);

    std::vector<Group> g1_g2_g3 = {"group1", "group2", "group3"};
    ItemSet expected_in_g1_g2_g3 = {"itemA"};
    EXPECT_EQ(gs.get_items_in_all_groups(g1_g2_g3), expected_in_g1_g2_g3);

    std::vector<Group> g1_g4 = {"group1", "group4"}; // No common items
    EXPECT_TRUE(gs.get_items_in_all_groups(g1_g4).empty());

    std::vector<Group> empty_groups_vec;
    EXPECT_TRUE(gs.get_items_in_all_groups(empty_groups_vec).empty());

    std::vector<Group> non_existent_group_vec = {"non_existent_group"};
    EXPECT_TRUE(gs.get_items_in_all_groups(non_existent_group_vec).empty());

    std::vector<Group> g1_non_existent_group_vec = {"group1", "non_existent_group"};
    EXPECT_TRUE(gs.get_items_in_all_groups(g1_non_existent_group_vec).empty());


    // get_items_in_any_group
    std::vector<Group> g2_g3 = {"group2", "group3"};
    ItemSet expected_in_g2_or_g3 = {"itemA", "itemB", "itemD"};
    EXPECT_EQ(gs.get_items_in_any_group(g2_g3), expected_in_g2_or_g3);

    std::vector<Group> g1_g4_any = {"group1", "group4"};
    ItemSet expected_in_g1_or_g4 = {"itemA", "itemB", "itemC", "itemE"};
    EXPECT_EQ(gs.get_items_in_any_group(g1_g4_any), expected_in_g1_or_g4);

    EXPECT_TRUE(gs.get_items_in_any_group(empty_groups_vec).empty());
    EXPECT_TRUE(gs.get_items_in_any_group(non_existent_group_vec).empty());
}

TEST(GroupedSetTest, CustomComparators) {
    struct CaseInsensitiveCompare {
        bool operator()(const std::string& a, const std::string& b) const {
            return std::lexicographical_compare(
                a.begin(), a.end(),
                b.begin(), b.end(),
                [](char c1, char c2) {
                    return std::tolower(c1) < std::tolower(c2);
                }
            );
        }
    };

    cpp_collections::GroupedSet<std::string, std::string, CaseInsensitiveCompare, CaseInsensitiveCompare> gs_ci;

    gs_ci.add_item_to_group("Apple", "Fruit");
    gs_ci.add_item_to_group("apple", "Red"); // Should be treated as same item "Apple" for item_to_groups
                                           // and new group "Red" for group_to_items

    EXPECT_TRUE(gs_ci.item_exists("APPLE"));
    EXPECT_TRUE(gs_ci.item_exists("apple"));
    EXPECT_EQ(gs_ci.size(), 1); // Only one unique item "Apple" due to CI compare

    EXPECT_TRUE(gs_ci.group_exists("FRUIT"));
    EXPECT_TRUE(gs_ci.group_exists("fruit"));
    EXPECT_EQ(gs_ci.group_count(), 2); // "Fruit" and "Red" are distinct if CI compare for groups is used correctly.

    // Check item's groups
    std::set<std::string, CaseInsensitiveCompare> expected_apple_groups_ci = {"Fruit", "Red"};
    EXPECT_EQ(gs_ci.get_groups_for_item("aPpLe"), expected_apple_groups_ci);

    // Check group's items
    std::set<std::string, CaseInsensitiveCompare> expected_fruit_items_ci = {"apple"}; // or "Apple"
    EXPECT_EQ(gs_ci.get_items_in_group("FRuiT").size(), 1);
    EXPECT_TRUE(gs_ci.get_items_in_group("FRuiT").count("APPLE"));
}

TEST(GroupedSetTest, EdgeCasesAndComplexScenarios) {
    TestGroupedSet gs;
    gs.add_item("item1"); // Ungrouped initially
    gs.add_item_to_group("item1", "groupA"); // Now grouped
    gs.remove_item_from_group("item1", "groupA"); // Now ungrouped again

    ItemSet expected_ungrouped = {"item1"};
    EXPECT_EQ(gs.get_ungrouped_items(), expected_ungrouped);
    EXPECT_EQ(gs.groups_for_item_count("item1"), 0);
    EXPECT_TRUE(gs.item_exists("item1")); // Still exists globally

    gs.add_item_to_group("item1", "groupB"); // Re-grouped
    EXPECT_FALSE(gs.get_ungrouped_items().count("item1"));

    // Remove group that makes an item ungrouped
    gs.remove_group("groupB");
    EXPECT_EQ(gs.get_ungrouped_items(), expected_ungrouped); // item1 becomes ungrouped again

    // Removing non-existent item from group
    EXPECT_FALSE(gs.remove_item_from_group("itemNonExistent", "groupA"));

    // Removing item from non-existent group
    EXPECT_FALSE(gs.remove_item_from_group("item1", "groupNonExistent"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

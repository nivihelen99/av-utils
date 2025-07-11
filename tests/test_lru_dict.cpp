#include "gtest/gtest.h"
#include "LRUDict.h" // Assuming LRUDict.h is in the include path or path is adjusted

#include <string>
#include <vector>
#include <algorithm> // For std::equal

// Helper to get LRUDict contents as a vector of pairs (MRU to LRU)
template<typename K, typename V>
std::vector<std::pair<K, V>> get_dict_contents(const cpp_collections::LRUDict<K, V>& dict) {
    std::vector<std::pair<K, V>> contents;
    for (const auto& p : dict) {
        contents.push_back(p);
    }
    return contents;
}

// Test fixture for LRUDict tests
class LRUDictTest : public ::testing::Test {
protected:
    cpp_collections::LRUDict<int, std::string> lru_3{3}; // Capacity 3
    cpp_collections::LRUDict<int, std::string> lru_1{1}; // Capacity 1
    cpp_collections::LRUDict<int, std::string> lru_0{0}; // Capacity 0
};

TEST_F(LRUDictTest, Construction) {
    EXPECT_EQ(lru_3.capacity(), 3);
    EXPECT_EQ(lru_3.size(), 0);
    EXPECT_TRUE(lru_3.empty());

    EXPECT_EQ(lru_1.capacity(), 1);
    EXPECT_EQ(lru_1.size(), 0);
    EXPECT_TRUE(lru_1.empty());

    EXPECT_EQ(lru_0.capacity(), 0);
    EXPECT_EQ(lru_0.size(), 0);
    EXPECT_TRUE(lru_0.empty());
}

TEST_F(LRUDictTest, InsertBasic) {
    auto result1 = lru_3.insert({1, "one"});
    EXPECT_TRUE(result1.second); // New insertion
    EXPECT_EQ(result1.first->first, 1);
    EXPECT_EQ(result1.first->second, "one");
    EXPECT_EQ(lru_3.size(), 1);
    EXPECT_FALSE(lru_3.empty());
    EXPECT_TRUE(lru_3.contains(1));
    EXPECT_EQ(lru_3.at(1), "one"); // Access moves to front, but it's already front

    auto result2 = lru_3.insert({1, "uno"}); // Existing key
    EXPECT_FALSE(result2.second); // Not a new insertion
    EXPECT_EQ(result2.first->first, 1);
    EXPECT_EQ(result2.first->second, "uno"); // Value updated
    EXPECT_EQ(lru_3.size(), 1);
    EXPECT_EQ(lru_3.at(1), "uno");
}

TEST_F(LRUDictTest, InsertOrderAndEviction) {
    lru_3.insert({1, "a"}); // MRU: {1,a}
    lru_3.insert({2, "b"}); // MRU: {2,b}, {1,a}
    lru_3.insert({3, "c"}); // MRU: {3,c}, {2,b}, {1,a} - Full

    std::vector<std::pair<int, std::string>> expected_order = {{3, "c"}, {2, "b"}, {1, "a"}};
    EXPECT_EQ(get_dict_contents(lru_3), expected_order);
    EXPECT_EQ(lru_3.size(), 3);

    lru_3.insert({4, "d"}); // Evicts {1,a}. New MRU: {4,d}
                            // Order: {4,d}, {3,c}, {2,b}
    EXPECT_EQ(lru_3.size(), 3);
    EXPECT_FALSE(lru_3.contains(1));
    EXPECT_TRUE(lru_3.contains(4));
    expected_order = {{4, "d"}, {3, "c"}, {2, "b"}};
    EXPECT_EQ(get_dict_contents(lru_3), expected_order);

    lru_3.insert({2, "updated_b"}); // Updates {2,b}, makes it MRU
                                  // Order: {2,updated_b}, {4,d}, {3,c}
    EXPECT_EQ(lru_3.size(), 3);
    EXPECT_EQ(lru_3.at(2), "updated_b"); // at() also moves to front
    expected_order = {{2, "updated_b"}, {4, "d"}, {3, "c"}};
    EXPECT_EQ(get_dict_contents(lru_3), expected_order);
}

TEST_F(LRUDictTest, AtAndUpdateLRU) {
    lru_3.insert({1, "a"});
    lru_3.insert({2, "b"});
    lru_3.insert({3, "c"}); // MRU->LRU: (3,c), (2,b), (1,a)

    EXPECT_EQ(lru_3.at(1), "a"); // Access 1, makes it MRU
                                 // Order: (1,a), (3,c), (2,b)
    std::vector<std::pair<int, std::string>> expected_order = {{1, "a"}, {3, "c"}, {2, "b"}};
    EXPECT_EQ(get_dict_contents(lru_3), expected_order);

    EXPECT_EQ(lru_3.at(3), "c"); // Access 3, makes it MRU
                                 // Order: (3,c), (1,a), (2,b)
    expected_order = {{3, "c"}, {1, "a"}, {2, "b"}};
    EXPECT_EQ(get_dict_contents(lru_3), expected_order);

    EXPECT_THROW(lru_3.at(100), std::out_of_range);
}

TEST_F(LRUDictTest, ConstAtNoLRUUpdate) {
    lru_3.insert({1, "a"});
    lru_3.insert({2, "b"});
    lru_3.insert({3, "c"}); // MRU->LRU: (3,c), (2,b), (1,a)

    const auto& const_lru_3 = lru_3;
    EXPECT_EQ(const_lru_3.at(1), "a");

    // Order should remain (3,c), (2,b), (1,a)
    std::vector<std::pair<int, std::string>> expected_order = {{3, "c"}, {2, "b"}, {1, "a"}};
    EXPECT_EQ(get_dict_contents(lru_3), expected_order);

    EXPECT_THROW(const_lru_3.at(100), std::out_of_range);
}


TEST_F(LRUDictTest, GetAndUpdateLRU) {
    lru_3.insert({1, "a"});
    lru_3.insert({2, "b"});
    lru_3.insert({3, "c"}); // MRU->LRU: (3,c), (2,b), (1,a)

    auto val_opt = lru_3.get(1);
    ASSERT_TRUE(val_opt.has_value());
    EXPECT_EQ(val_opt->get(), "a"); // Access 1, makes it MRU
                                  // Order: (1,a), (3,c), (2,b)
    std::vector<std::pair<int, std::string>> expected_order = {{1, "a"}, {3, "c"}, {2, "b"}};
    EXPECT_EQ(get_dict_contents(lru_3), expected_order);

    val_opt = lru_3.get(100);
    EXPECT_FALSE(val_opt.has_value());
}

TEST_F(LRUDictTest, ConstGetNoLRUUpdate) {
    lru_3.insert({1, "a"});
    lru_3.insert({2, "b"});
    lru_3.insert({3, "c"}); // MRU->LRU: (3,c), (2,b), (1,a)

    const auto& const_lru_3 = lru_3;
    auto val_opt = const_lru_3.get(1);
    ASSERT_TRUE(val_opt.has_value());
    EXPECT_EQ(val_opt->get(), "a");

    // Order should remain (3,c), (2,b), (1,a)
    std::vector<std::pair<int, std::string>> expected_order = {{3, "c"}, {2, "b"}, {1, "a"}};
    EXPECT_EQ(get_dict_contents(lru_3), expected_order);
}


TEST_F(LRUDictTest, PeekNoLRUUpdate) {
    lru_3.insert({1, "a"});
    lru_3.insert({2, "b"});
    lru_3.insert({3, "c"}); // MRU->LRU: (3,c), (2,b), (1,a)

    std::vector<std::pair<int, std::string>> initial_order = {{3, "c"}, {2, "b"}, {1, "a"}};

    auto val_opt = lru_3.peek(1);
    ASSERT_TRUE(val_opt.has_value());
    EXPECT_EQ(val_opt->get(), "a");

    // Order should remain (3,c), (2,b), (1,a)
    EXPECT_EQ(get_dict_contents(lru_3), initial_order);

    val_opt = lru_3.peek(100);
    EXPECT_FALSE(val_opt.has_value());
}


TEST_F(LRUDictTest, OperatorSquareBrackets) {
    lru_3[1] = "a"; // Insert
    lru_3[2] = "b"; // Insert
    lru_3[3] = "c"; // Insert. Order: (3,c), (2,b), (1,a)

    std::vector<std::pair<int, std::string>> expected_order = {{3, "c"}, {2, "b"}, {1, "a"}};
    EXPECT_EQ(get_dict_contents(lru_3), expected_order);

    lru_3[1] = "alpha"; // Access & Update. Order: (1,alpha), (3,c), (2,b)
    expected_order = {{1, "alpha"}, {3, "c"}, {2, "b"}};
    EXPECT_EQ(get_dict_contents(lru_3), expected_order);
    EXPECT_EQ(lru_3.size(), 3);

    lru_3[4] = "d"; // Insert, evicts (2,b). Order: (4,d), (1,alpha), (3,c)
    expected_order = {{4, "d"}, {1, "alpha"}, {3, "c"}};
    EXPECT_EQ(get_dict_contents(lru_3), expected_order);
    EXPECT_FALSE(lru_3.contains(2));
    EXPECT_EQ(lru_3.size(), 3);
}

TEST_F(LRUDictTest, Erase) {
    lru_3.insert({1, "a"});
    lru_3.insert({2, "b"});
    lru_3.insert({3, "c"}); // (3,c), (2,b), (1,a)

    EXPECT_TRUE(lru_3.erase(2)); // Erase middle. Order: (3,c), (1,a)
    EXPECT_EQ(lru_3.size(), 2);
    EXPECT_FALSE(lru_3.contains(2));
    std::vector<std::pair<int, std::string>> expected_order = {{3, "c"}, {1, "a"}};
    EXPECT_EQ(get_dict_contents(lru_3), expected_order);

    EXPECT_FALSE(lru_3.erase(100)); // Erase non-existent
    EXPECT_EQ(lru_3.size(), 2);

    EXPECT_TRUE(lru_3.erase(3)); // Erase MRU. Order: (1,a)
    EXPECT_EQ(lru_3.size(), 1);
    expected_order = {{1, "a"}};
    EXPECT_EQ(get_dict_contents(lru_3), expected_order);

    EXPECT_TRUE(lru_3.erase(1)); // Erase LRU (and only element)
    EXPECT_EQ(lru_3.size(), 0);
    EXPECT_TRUE(lru_3.empty());
}

TEST_F(LRUDictTest, EraseByIterator) {
    lru_3.insert({1, "a"});
    lru_3.insert({2, "b"});
    lru_3.insert({3, "c"}); // (3,c), (2,b), (1,a)

    auto it = lru_3.begin(); // Points to {3,c}
    ++it;                   // Points to {2,b}

    auto next_it = lru_3.erase(it); // Erase {2,b}
    EXPECT_EQ(lru_3.size(), 2);
    EXPECT_FALSE(lru_3.contains(2));
    ASSERT_NE(next_it, lru_3.end());
    EXPECT_EQ(next_it->first, 1); // Should point to {1,a}

    std::vector<std::pair<int, std::string>> expected_order = {{3, "c"}, {1, "a"}};
    EXPECT_EQ(get_dict_contents(lru_3), expected_order);

    // Erase MRU
    next_it = lru_3.erase(lru_3.begin());
    EXPECT_EQ(lru_3.size(), 1);
    ASSERT_NE(next_it, lru_3.end());
    EXPECT_EQ(next_it->first, 1);
    expected_order = {{1, "a"}};
    EXPECT_EQ(get_dict_contents(lru_3), expected_order);

    // Erase last element
    next_it = lru_3.erase(lru_3.begin());
    EXPECT_EQ(lru_3.size(), 0);
    EXPECT_EQ(next_it, lru_3.end());
}


TEST_F(LRUDictTest, Clear) {
    lru_3.insert({1, "a"});
    lru_3.insert({2, "b"});
    ASSERT_FALSE(lru_3.empty());
    ASSERT_EQ(lru_3.size(), 2);

    lru_3.clear();
    EXPECT_TRUE(lru_3.empty());
    EXPECT_EQ(lru_3.size(), 0);
    EXPECT_EQ(lru_3.capacity(), 3); // Capacity should remain
    EXPECT_FALSE(lru_3.contains(1));
}

TEST_F(LRUDictTest, ZeroCapacity) {
    EXPECT_EQ(lru_0.capacity(), 0);
    EXPECT_TRUE(lru_0.empty());

    auto result = lru_0.insert({1, "one"});
    EXPECT_FALSE(result.second); // Cannot insert
    EXPECT_EQ(result.first, lru_0.end());
    EXPECT_TRUE(lru_0.empty());

    EXPECT_THROW(lru_0[1] = "one", std::out_of_range);
    EXPECT_FALSE(lru_0.contains(1));
    EXPECT_TRUE(lru_0.empty());

    EXPECT_FALSE(lru_0.erase(1));
}

TEST_F(LRUDictTest, CapacityOne) {
    lru_1.insert({1, "a"}); // {1,a}
    EXPECT_EQ(lru_1.size(), 1);
    EXPECT_EQ(lru_1.at(1), "a");

    lru_1.insert({2, "b"}); // Evicts {1,a}. Now {2,b}
    EXPECT_EQ(lru_1.size(), 1);
    EXPECT_FALSE(lru_1.contains(1));
    EXPECT_TRUE(lru_1.contains(2));
    EXPECT_EQ(lru_1.at(2), "b");

    lru_1[2] = "beta"; // Update, still {2,beta}
    EXPECT_EQ(lru_1.at(2), "beta");
    EXPECT_EQ(lru_1.size(), 1);
}

TEST_F(LRUDictTest, TryEmplace) {
    auto result = lru_3.try_emplace(1, "one");
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->first, 1);
    EXPECT_EQ(lru_3.at(1), "one");

    result = lru_3.try_emplace(1, "another one"); // Key exists
    EXPECT_FALSE(result.second);
    EXPECT_EQ(result.first->first, 1);
    EXPECT_EQ(lru_3.at(1), "one"); // Value not updated by try_emplace if key exists

    // Fill it up
    lru_3.try_emplace(2, "two");
    lru_3.try_emplace(3, "three"); // (3,three), (2,two), (1,one)

    // Access 1 to make it MRU, 3 becomes LRU
    lru_3.at(1); // (1,one), (3,three), (2,two)

    result = lru_3.try_emplace(4, "four"); // Evicts 2
    EXPECT_TRUE(result.second);
    EXPECT_FALSE(lru_3.contains(2));
    EXPECT_TRUE(lru_3.contains(4));
    // Order: (4,four), (1,one), (3,three)
    std::vector<std::pair<int, std::string>> expected_order = {{4, "four"}, {1, "one"}, {3, "three"}};
    EXPECT_EQ(get_dict_contents(lru_3), expected_order);
}

TEST_F(LRUDictTest, InsertOrAssign) {
    auto result = lru_3.insert_or_assign(1, "one");
    EXPECT_TRUE(result.second); // Inserted
    EXPECT_EQ(result.first->first, 1);
    EXPECT_EQ(lru_3.at(1), "one");

    result = lru_3.insert_or_assign(1, "uno");
    EXPECT_FALSE(result.second); // Assigned
    EXPECT_EQ(result.first->first, 1);
    EXPECT_EQ(lru_3.at(1), "uno"); // Value updated

    // Fill it up
    lru_3.insert_or_assign(2, "dos");
    lru_3.insert_or_assign(3, "tres"); // (3,tres), (2,dos), (1,uno)

    // Access 1 to make it MRU, 3 becomes LRU
    lru_3.at(1); // (1,uno), (3,tres), (2,dos)

    result = lru_3.insert_or_assign(4, "cuatro"); // Evicts 2
    EXPECT_TRUE(result.second);
    EXPECT_FALSE(lru_3.contains(2));
    EXPECT_TRUE(lru_3.contains(4));
    // Order: (4,cuatro), (1,uno), (3,tres)
    std::vector<std::pair<int, std::string>> expected_order = {{4, "cuatro"}, {1, "uno"}, {3, "tres"}};
    EXPECT_EQ(get_dict_contents(lru_3), expected_order);
}

TEST_F(LRUDictTest, CopyConstructor) {
    lru_3.insert({1, "a"});
    lru_3.insert({2, "b"});
    lru_3.insert({3, "c"}); // MRU->LRU: (3,c), (2,b), (1,a)

    cpp_collections::LRUDict<int, std::string> lru_copy(lru_3);
    EXPECT_EQ(lru_copy.capacity(), 3);
    EXPECT_EQ(lru_copy.size(), 3);
    EXPECT_EQ(get_dict_contents(lru_copy), get_dict_contents(lru_3));

    // Ensure copy is deep and independent
    lru_copy.insert({4, "d"}); // Evicts 1 from copy
    EXPECT_TRUE(lru_3.contains(1)); // Original should still have 1
    EXPECT_FALSE(lru_copy.contains(1));
}

TEST_F(LRUDictTest, CopyAssignment) {
    lru_3.insert({1, "a"});
    lru_3.insert({2, "b"}); // (2,b), (1,a)

    cpp_collections::LRUDict<int, std::string> lru_copy_assign(1); // Different capacity initially
    lru_copy_assign.insert({100, "z"});

    lru_copy_assign = lru_3;
    EXPECT_EQ(lru_copy_assign.capacity(), 3);
    EXPECT_EQ(lru_copy_assign.size(), 2);
    EXPECT_EQ(get_dict_contents(lru_copy_assign), get_dict_contents(lru_3));

    // Ensure copy is deep and independent
    lru_copy_assign.insert({3, "c"});
    EXPECT_FALSE(lru_3.contains(3));
    EXPECT_TRUE(lru_copy_assign.contains(3));
}

TEST_F(LRUDictTest, MoveConstructor) {
    lru_3.insert({1, "a"});
    lru_3.insert({2, "b"});
    lru_3.insert({3, "c"});
    std::vector<std::pair<int, std::string>> expected_contents = get_dict_contents(lru_3);

    cpp_collections::LRUDict<int, std::string> lru_moved(std::move(lru_3));

    EXPECT_EQ(lru_moved.capacity(), 3);
    EXPECT_EQ(lru_moved.size(), 3);
    EXPECT_EQ(get_dict_contents(lru_moved), expected_contents);

    // Original lru_3 should be in a valid but empty/unspecified state
    EXPECT_EQ(lru_3.capacity(), 0); // Per current move constructor
    EXPECT_TRUE(lru_3.empty());
}


TEST_F(LRUDictTest, MoveAssignment) {
    lru_3.insert({1, "a"});
    lru_3.insert({2, "b"});
    lru_3.insert({3, "c"});
    std::vector<std::pair<int, std::string>> expected_contents = get_dict_contents(lru_3);

    cpp_collections::LRUDict<int, std::string> lru_move_assign(1);
    lru_move_assign.insert({100,"z"});

    lru_move_assign = std::move(lru_3);

    EXPECT_EQ(lru_move_assign.capacity(), 3);
    EXPECT_EQ(lru_move_assign.size(), 3);
    EXPECT_EQ(get_dict_contents(lru_move_assign), expected_contents);

    EXPECT_EQ(lru_3.capacity(), 0);
    EXPECT_TRUE(lru_3.empty());
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

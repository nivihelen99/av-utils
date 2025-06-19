#include "SlotMap.h" // Assuming SlotMap.h is in include/ and tests/ is at the same level
#include "gtest/gtest.h"
#include <string>
#include <vector> // For testing with vectors as values or similar

// Define a simple struct for testing
struct TestStruct {
    int id;
    std::string data;

    bool operator==(const TestStruct& other) const {
        return id == other.id && data == other.data;
    }
};

// Test fixture for SlotMap tests
class SlotMapTest : public ::testing::Test {
protected:
    utils::SlotMap<std::string> string_map;
    utils::SlotMap<int> int_map;
    utils::SlotMap<TestStruct> struct_map;
};

TEST_F(SlotMapTest, InitialState) {
    EXPECT_EQ(string_map.size(), 0);
    EXPECT_TRUE(string_map.empty());
    EXPECT_EQ(int_map.size(), 0);
    EXPECT_TRUE(int_map.empty());
    EXPECT_EQ(struct_map.size(), 0);
    EXPECT_TRUE(struct_map.empty());
}

TEST_F(SlotMapTest, BasicInsertAndGet) {
    auto key1 = string_map.insert("Hello");
    EXPECT_EQ(string_map.size(), 1);
    ASSERT_NE(string_map.get(key1), nullptr);
    EXPECT_EQ(*string_map.get(key1), "Hello");
    EXPECT_TRUE(string_map.contains(key1));

    auto key2 = string_map.insert("World");
    EXPECT_EQ(string_map.size(), 2);
    ASSERT_NE(string_map.get(key2), nullptr);
    EXPECT_EQ(*string_map.get(key2), "World");
    EXPECT_TRUE(string_map.contains(key2));

    // Check previous key still valid
    ASSERT_NE(string_map.get(key1), nullptr);
    EXPECT_EQ(*string_map.get(key1), "Hello");
}

TEST_F(SlotMapTest, InsertAndGetWithInts) {
    auto key1 = int_map.insert(100);
    EXPECT_EQ(int_map.size(), 1);
    ASSERT_NE(int_map.get(key1), nullptr);
    EXPECT_EQ(*int_map.get(key1), 100);

    auto key2 = int_map.insert(200);
    EXPECT_EQ(int_map.size(), 2);
    ASSERT_NE(int_map.get(key2), nullptr);
    EXPECT_EQ(*int_map.get(key2), 200);
}

TEST_F(SlotMapTest, InsertAndGetWithStructs) {
    TestStruct ts1{1, "Data1"};
    TestStruct ts2{2, "Data2"};

    auto key1 = struct_map.insert(ts1);
    EXPECT_EQ(struct_map.size(), 1);
    ASSERT_NE(struct_map.get(key1), nullptr);
    EXPECT_EQ(*struct_map.get(key1), ts1);

    auto key2 = struct_map.insert(ts2);
    EXPECT_EQ(struct_map.size(), 2);
    ASSERT_NE(struct_map.get(key2), nullptr);
    EXPECT_EQ(*struct_map.get(key2), ts2);
}

TEST_F(SlotMapTest, EraseAndGet) {
    auto key1 = string_map.insert("TestErase");
    EXPECT_EQ(string_map.size(), 1);
    EXPECT_TRUE(string_map.contains(key1));

    bool erased = string_map.erase(key1);
    EXPECT_TRUE(erased);
    EXPECT_EQ(string_map.size(), 0);
    EXPECT_FALSE(string_map.contains(key1));
    EXPECT_EQ(string_map.get(key1), nullptr);

    // Try erasing again
    bool erased_again = string_map.erase(key1);
    EXPECT_FALSE(erased_again); // Should fail as key is no longer valid
}

TEST_F(SlotMapTest, InsertAfterEraseReusesSlotAndIncrementsGeneration) {
    auto key1 = string_map.insert("First");
    uint32_t initial_index = key1.index;
    uint32_t initial_gen = key1.generation;

    string_map.erase(key1);
    EXPECT_FALSE(string_map.contains(key1));

    auto key2 = string_map.insert("Second"); // Should reuse slot from key1
    EXPECT_EQ(string_map.size(), 1);
    EXPECT_TRUE(string_map.contains(key2));
    ASSERT_NE(string_map.get(key2), nullptr);
    EXPECT_EQ(*string_map.get(key2), "Second");

    EXPECT_EQ(key2.index, initial_index); // Index should be reused
    EXPECT_EQ(key2.generation, initial_gen + 1); // Generation should be incremented
}

TEST_F(SlotMapTest, StaleKeyRetrieval) {
    auto key1 = string_map.insert("Original");
    utils::SlotMap<std::string>::Key stale_key = key1; // Copy the key

    string_map.erase(key1); // Erase the element, invalidating key1 and stale_key

    auto key2 = string_map.insert("NewData"); // This might reuse the slot

    EXPECT_FALSE(string_map.contains(stale_key));
    EXPECT_EQ(string_map.get(stale_key), nullptr);

    // Ensure new key is valid if it reused the slot
    if (key2.index == stale_key.index) {
        EXPECT_NE(key2.generation, stale_key.generation);
        EXPECT_TRUE(string_map.contains(key2));
        ASSERT_NE(string_map.get(key2), nullptr);
        EXPECT_EQ(*string_map.get(key2), "NewData");
    }
}

TEST_F(SlotMapTest, ContainsFunctionality) {
    auto key_valid = string_map.insert("Valid");
    EXPECT_TRUE(string_map.contains(key_valid));

    auto key_to_erase = string_map.insert("EraseMe");
    EXPECT_TRUE(string_map.contains(key_to_erase));
    string_map.erase(key_to_erase);
    EXPECT_FALSE(string_map.contains(key_to_erase)); // After erase

    // Stale key (generation mismatch)
    utils::SlotMap<std::string>::Key stale_key = key_to_erase; // Has old generation
    auto key_reused = string_map.insert("Reused");
    if (key_reused.index == stale_key.index) {
       EXPECT_FALSE(string_map.contains(stale_key));
    }

    utils::SlotMap<std::string>::Key non_existent_key = {999, 0}; // Index out of bounds
    EXPECT_FALSE(string_map.contains(non_existent_key));

    utils::SlotMap<std::string>::Key key_valid_gen_mismatch = {key_valid.index, key_valid.generation + 1};
    EXPECT_FALSE(string_map.contains(key_valid_gen_mismatch));
}

TEST_F(SlotMapTest, SizeAndEmpty) {
    EXPECT_TRUE(string_map.empty());
    EXPECT_EQ(string_map.size(), 0);

    auto key1 = string_map.insert("1");
    EXPECT_FALSE(string_map.empty());
    EXPECT_EQ(string_map.size(), 1);

    auto key2 = string_map.insert("2");
    EXPECT_EQ(string_map.size(), 2);

    string_map.erase(key1);
    EXPECT_EQ(string_map.size(), 1);

    string_map.erase(key2);
    EXPECT_TRUE(string_map.empty());
    EXPECT_EQ(string_map.size(), 0);
}

TEST_F(SlotMapTest, MultipleErasures) {
    auto k1 = string_map.insert("A");
    auto k2 = string_map.insert("B");
    auto k3 = string_map.insert("C");
    EXPECT_EQ(string_map.size(), 3);

    string_map.erase(k1);
    EXPECT_EQ(string_map.size(), 2);
    EXPECT_FALSE(string_map.contains(k1));
    EXPECT_TRUE(string_map.contains(k2));
    EXPECT_TRUE(string_map.contains(k3));

    string_map.erase(k3);
    EXPECT_EQ(string_map.size(), 1);
    EXPECT_FALSE(string_map.contains(k1));
    EXPECT_TRUE(string_map.contains(k2));
    EXPECT_FALSE(string_map.contains(k3));

    ASSERT_NE(string_map.get(k2), nullptr);
    EXPECT_EQ(*string_map.get(k2), "B");

    string_map.erase(k2);
    EXPECT_EQ(string_map.size(), 0);
    EXPECT_TRUE(string_map.empty());
    EXPECT_FALSE(string_map.contains(k2));
}

TEST_F(SlotMapTest, InsertEraseInsertSequence) {
    std::vector<utils::SlotMap<std::string>::Key> keys;
    for (int i = 0; i < 5; ++i) {
        keys.push_back(string_map.insert("Item " + std::to_string(i)));
    }
    EXPECT_EQ(string_map.size(), 5);

    // Erase some
    string_map.erase(keys[1]);
    string_map.erase(keys[3]);
    EXPECT_EQ(string_map.size(), 3);
    EXPECT_FALSE(string_map.contains(keys[1]));
    EXPECT_TRUE(string_map.contains(keys[0]));

    // Insert more
    auto k_new1 = string_map.insert("New Item 1");
    auto k_new2 = string_map.insert("New Item 2");
    EXPECT_EQ(string_map.size(), 5);
    EXPECT_TRUE(string_map.contains(k_new1));
    EXPECT_TRUE(string_map.contains(k_new2));

    // Check reused indices and new generations
    EXPECT_EQ(k_new1.index, keys[3].index); // Should reuse last erased slot first (LIFO from free_list)
    EXPECT_EQ(k_new1.generation, keys[3].generation + 1);

    EXPECT_EQ(k_new2.index, keys[1].index);
    EXPECT_EQ(k_new2.generation, keys[1].generation + 1);
}


TEST_F(SlotMapTest, GenerationIncrementDetails) {
    auto key1 = string_map.insert("gen_test_1");
    ASSERT_EQ(key1.generation, 0); // First insert in a new slot

    string_map.erase(key1); // Generation becomes 1 for this slot

    auto key2 = string_map.insert("gen_test_2"); // Reuses slot
    ASSERT_EQ(key2.index, key1.index);
    ASSERT_EQ(key2.generation, 1);

    string_map.erase(key2); // Generation becomes 2

    auto key3 = string_map.insert("gen_test_3"); // Reuses slot
    ASSERT_EQ(key3.index, key2.index);
    ASSERT_EQ(key3.generation, 2);
}


TEST_F(SlotMapTest, EmptyMapOperations) {
    utils::SlotMap<int>::Key dummy_key = {0, 0}; // A plausible key

    EXPECT_FALSE(int_map.contains(dummy_key));
    EXPECT_EQ(int_map.get(dummy_key), nullptr);
    EXPECT_FALSE(int_map.erase(dummy_key));

    utils::SlotMap<int>::Key invalid_key = {100, 0}; // Index out of bounds for empty map
    EXPECT_FALSE(int_map.contains(invalid_key));
    EXPECT_EQ(int_map.get(invalid_key), nullptr);
    EXPECT_FALSE(int_map.erase(invalid_key));
}


TEST_F(SlotMapTest, FillAndEmptyMultipleTimes) {
    for (int cycle = 0; cycle < 3; ++cycle) {
        std::vector<utils::SlotMap<int>::Key> current_keys;
        for (int i = 0; i < 10; ++i) {
            current_keys.push_back(int_map.insert(i * 10 + cycle));
        }
        ASSERT_EQ(int_map.size(), 10);

        for(const auto& k : current_keys) {
            ASSERT_TRUE(int_map.contains(k));
        }

        for (size_t i = 0; i < current_keys.size(); ++i) {
            bool erased = int_map.erase(current_keys[i]);
            ASSERT_TRUE(erased);
            ASSERT_FALSE(int_map.contains(current_keys[i]));
        }
        ASSERT_TRUE(int_map.empty());
        ASSERT_EQ(int_map.size(), 0);
    }
}

// This test is more conceptual for uint32_t max generation.
// It just checks if generation increments past 0, which is already covered.
// Actually hitting max would take too long.
TEST_F(SlotMapTest, GenerationIncrementsPastZero) {
    auto key = int_map.insert(1); // Gen 0
    EXPECT_EQ(key.generation, 0);
    int_map.erase(key); // Slot gen becomes 1

    key = int_map.insert(2); // Gen 1 for this key
    EXPECT_EQ(key.generation, 1);
    int_map.erase(key); // Slot gen becomes 2

    key = int_map.insert(3); // Gen 2 for this key
    EXPECT_EQ(key.generation, 2);
}

// main function is provided by GTest::gtest_main when linked,
// so it's not needed here for individual test file to be linkable
// with other tests into a single 'run_tests' executable or for ctest discovery.
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

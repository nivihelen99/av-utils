#include <gtest/gtest.h>
#include "slot_map_new.h" // Assuming it's in the parent directory's include folder
#include <string>
#include <vector>
#include <stdexcept> // For std::out_of_range
#include <algorithm> // For std::find_if
#include <set>       // For testing Key comparison operators

// Using namespace for convenience in test file
using namespace utils;

// Anonymous namespace for test-specific structures
namespace {

struct Entity {
    int id;
    std::string name;
    float health;

    Entity(int i, std::string n, float h) : id(i), name(std::move(n)), health(h) {}
    Entity() : id(0), name(""), health(0.0f) {}

    bool operator==(const Entity& other) const {
        return id == other.id && name == other.name && health == other.health;
    }
};

struct GameObject {
    std::string type;
    int x, y;
    bool active;

    GameObject(std::string t, int px, int py, bool a) : type(std::move(t)), x(px), y(py), active(a) {}
    GameObject() : type(""), x(0), y(0), active(false) {}

    bool operator==(const GameObject& other) const {
        return type == other.type && x == other.x && y == other.y && active == other.active;
    }
};

struct DestructorTestType {
    static int destructor_call_count;
    int id;
    bool moved_from; // To detect if moved

    DestructorTestType(int i) : id(i), moved_from(false) {}

    DestructorTestType(const DestructorTestType& other) : id(other.id), moved_from(false) {
        // Optional: std::cout << "DestructorTestType " << id << " copy constructed\n";
    }
    DestructorTestType(DestructorTestType&& other) noexcept : id(other.id), moved_from(false) {
        other.moved_from = true;
        // Optional: std::cout << "DestructorTestType " << id << " move constructed\n";
    }
    DestructorTestType& operator=(const DestructorTestType& other) {
        if (this != &other) {
            id = other.id;
            moved_from = false;
        }
        // Optional: std::cout << "DestructorTestType " << id << " copy assigned from " << other.id << "\n";
        return *this;
    }
    DestructorTestType& operator=(DestructorTestType&& other) noexcept {
        if (this != &other) {
            id = other.id;
            moved_from = false;
            other.moved_from = true;
        }
        // Optional: std::cout << "DestructorTestType " << id << " move assigned from " << other.id << "\n";
        return *this;
    }

    ~DestructorTestType() {
        if (!moved_from) {
            destructor_call_count++;
            // Optional: std::cout << "DestructorTestType " << id << " destructed. Count: " << destructor_call_count << "\n";
        } else {
            // Optional: std::cout << "DestructorTestType " << id << " (moved-from instance) destructed.\n";
        }
    }
};
int DestructorTestType::destructor_call_count = 0;

} // namespace

// Test fixture for SlotMap tests
class SlotMapNewTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset static counter before each test that uses DestructorTestType
        DestructorTestType::destructor_call_count = 0;
    }
};

TEST_F(SlotMapNewTest, BasicOperations) {
    SlotMap<int> map;
    ASSERT_EQ(map.size(), 0);
    ASSERT_TRUE(map.empty());
    SlotMap<int>::Key key1 = map.emplace(42);
    SlotMap<int>::Key key2 = map.emplace(100);
    SlotMap<int>::Key key3 = map.emplace(200);
    ASSERT_EQ(map.size(), 3);
    ASSERT_FALSE(map.empty());
    ASSERT_TRUE(key1.is_valid());
    ASSERT_TRUE(key2.is_valid());
    ASSERT_TRUE(key3.is_valid());
    int* val1_ptr = map.get(key1);
    ASSERT_NE(val1_ptr, nullptr);
    ASSERT_EQ(*val1_ptr, 42);
    ASSERT_EQ(map[key2], 100);
    ASSERT_EQ(map.at(key3), 200);
    SlotMap<int>::Key invalid_key_default;
    ASSERT_THROW(map.at(invalid_key_default), std::out_of_range);
    ASSERT_THROW(map.at(SlotMap<int>::INVALID_KEY), std::out_of_range);
    ASSERT_TRUE(map.contains(key1));
    ASSERT_FALSE(map.contains(invalid_key_default));
    ASSERT_FALSE(map.contains(SlotMap<int>::INVALID_KEY));
    ASSERT_TRUE(map.erase(key1));
    ASSERT_EQ(map.size(), 2);
    ASSERT_FALSE(map.contains(key1));
    ASSERT_EQ(map.get(key1), nullptr);
    ASSERT_THROW(map.at(key1), std::out_of_range);
    ASSERT_FALSE(map.erase(key1));
    ASSERT_FALSE(map.erase(invalid_key_default));
    ASSERT_FALSE(map.erase(SlotMap<int>::INVALID_KEY));
    SlotMap<int>::Key non_existent_key = {999, 0};
    ASSERT_FALSE(map.erase(non_existent_key));
    ASSERT_TRUE(map.contains(key2));
    ASSERT_EQ(map.at(key2), 100);
    ASSERT_TRUE(map.contains(key3));
    ASSERT_EQ(map.at(key3), 200);
}

TEST_F(SlotMapNewTest, GenerationSafety) {
    SlotMap<std::string> map;
    SlotMap<std::string>::Key key1 = map.emplace("Hello");
    ASSERT_NE(map.get(key1), nullptr);
    ASSERT_EQ(*map.get(key1), "Hello");
    ASSERT_TRUE(map.erase(key1));
    ASSERT_EQ(map.get(key1), nullptr);
    SlotMap<std::string>::Key key2 = map.emplace("World");
    ASSERT_NE(map.get(key2), nullptr);
    ASSERT_EQ(*map.get(key2), "World");
    if (key1.index == key2.index) {
        ASSERT_NE(key1.generation, key2.generation);
    }
    ASSERT_EQ(map.get(key1), nullptr);
    const std::string* val2_ptr = map.get(key2);
    ASSERT_NE(val2_ptr, nullptr);
    ASSERT_EQ(*val2_ptr, "World");
}

TEST_F(SlotMapNewTest, EmptyMapOperations) {
    SlotMap<int> map;
    ASSERT_TRUE(map.empty());
    ASSERT_EQ(map.size(), 0);
    SlotMap<int>::Key default_key;
    SlotMap<int>::Key specific_invalid_key = {0, 0};
    ASSERT_EQ(map.get(SlotMap<int>::INVALID_KEY), nullptr);
    ASSERT_EQ(map.get(default_key), nullptr);
    ASSERT_EQ(map.get(specific_invalid_key), nullptr);
    ASSERT_FALSE(map.erase(SlotMap<int>::INVALID_KEY));
    ASSERT_FALSE(map.erase(default_key));
    ASSERT_FALSE(map.erase(specific_invalid_key));
    ASSERT_FALSE(map.contains(SlotMap<int>::INVALID_KEY));
    ASSERT_FALSE(map.contains(default_key));
    ASSERT_FALSE(map.contains(specific_invalid_key));
    ASSERT_THROW(map.at(SlotMap<int>::INVALID_KEY), std::out_of_range);
    ASSERT_THROW(map.at(default_key), std::out_of_range);
    ASSERT_THROW(map.at(specific_invalid_key), std::out_of_range);
}

TEST_F(SlotMapNewTest, ComplexObjects) {
    SlotMap<Entity> entities;
    Entity player_initial(1, "Player", 100.0f);
    SlotMap<Entity>::Key player_key = entities.emplace(player_initial);
    Entity enemy_initial(2, "Enemy", 50.0f);
    SlotMap<Entity>::Key enemy_key = entities.emplace(enemy_initial);
    ASSERT_EQ(entities.size(), 2);
    Entity* player_ptr = entities.get(player_key);
    ASSERT_NE(player_ptr, nullptr);
    ASSERT_EQ(player_ptr->name, "Player");
    ASSERT_EQ(player_ptr->health, 100.0f);
    player_ptr->health -= 25.0f;
    ASSERT_EQ(entities.at(player_key).health, 75.0f);
    ASSERT_TRUE(entities.erase(enemy_key));
    ASSERT_EQ(entities.size(), 1);
    ASSERT_FALSE(entities.contains(enemy_key));
    Entity npc_initial(3, "NPC", 80.0f);
    SlotMap<Entity>::Key npc_key = entities.emplace(npc_initial);
    ASSERT_EQ(entities.size(), 2);
    ASSERT_TRUE(entities.contains(npc_key));
    ASSERT_EQ(entities.at(npc_key).name, "NPC");
    if (enemy_key.index == npc_key.index) {
        ASSERT_NE(enemy_key.generation, npc_key.generation);
    }
}

TEST_F(SlotMapNewTest, IteratorOperations) {
    SlotMap<GameObject> objects;
    std::vector<SlotMap<GameObject>::Key> keys;
    keys.push_back(objects.emplace("Cube", 0, 0, true));
    keys.push_back(objects.emplace("Sphere", 1, 1, true));
    keys.push_back(objects.emplace("Pyramid", 2, 2, false));
    keys.push_back(objects.emplace("Capsule", 3, 3, true));
    ASSERT_TRUE(objects.erase(keys[2]));
    keys[2] = objects.emplace("Cylinder", 4, 4, true);
    ASSERT_EQ(objects.size(), 3);
    size_t count = 0;
    std::vector<std::string> found_types_non_const;
    for (auto entry : objects) { // Changed from auto& to auto
        ASSERT_TRUE(entry.first.is_valid());
        ASSERT_TRUE(objects.contains(entry.first));
        found_types_non_const.push_back(entry.second.type);
        if (entry.second.type == "Cube") {
            entry.second.x = 10;
        }
        count++;
    }
    ASSERT_EQ(count, objects.size());
    ASSERT_EQ(objects.at(keys[0]).x, 10);
    ASSERT_NE(std::find(found_types_non_const.begin(), found_types_non_const.end(), "Cube"), found_types_non_const.end());
    ASSERT_NE(std::find(found_types_non_const.begin(), found_types_non_const.end(), "Sphere"), found_types_non_const.end());
    ASSERT_NE(std::find(found_types_non_const.begin(), found_types_non_const.end(), "Cylinder"), found_types_non_const.end());
    ASSERT_EQ(std::find(found_types_non_const.begin(), found_types_non_const.end(), "Pyramid"), found_types_non_const.end());
    const SlotMap<GameObject>& const_objects = objects;
    count = 0;
    std::vector<std::string> found_types_const;
    for (const auto& entry : const_objects) {
        ASSERT_TRUE(entry.first.is_valid());
        ASSERT_TRUE(const_objects.contains(entry.first));
        found_types_const.push_back(entry.second.type);
        count++;
    }
    ASSERT_EQ(count, const_objects.size());
    ASSERT_NE(std::find(found_types_const.begin(), found_types_const.end(), "Cube"), found_types_const.end());
    ASSERT_NE(std::find(found_types_const.begin(), found_types_const.end(), "Sphere"), found_types_const.end());
    ASSERT_NE(std::find(found_types_const.begin(), found_types_const.end(), "Cylinder"), found_types_const.end());
    SlotMap<int> empty_map;
    ASSERT_EQ(empty_map.begin(), empty_map.end());
    ASSERT_EQ(empty_map.cbegin(), empty_map.cend());
    count = 0;
    for (const auto& item : empty_map) { (void)item; count++; } // Changed from auto& to const auto&
    ASSERT_EQ(count, 0);
    SlotMap<int> map_to_clear;
    map_to_clear.emplace(1);
    map_to_clear.emplace(2);
    ASSERT_NE(map_to_clear.begin(), map_to_clear.end());
    map_to_clear.clear();
    ASSERT_EQ(map_to_clear.begin(), map_to_clear.end());
    ASSERT_EQ(map_to_clear.cbegin(), map_to_clear.cend());
    count = 0;
    for (const auto& item : map_to_clear) { (void)item; count++; } // Changed from auto& to const auto&
    ASSERT_EQ(count, 0);
}

TEST_F(SlotMapNewTest, ClearOperation) {
    SlotMap<int> map;
    SlotMap<int>::Key key1 = map.emplace(10);
    SlotMap<int>::Key key2 = map.emplace(20);
    SlotMap<int>::Key key3 = map.emplace(30);
    ASSERT_EQ(map.size(), 3);
    ASSERT_FALSE(map.empty());
    map.clear();
    ASSERT_EQ(map.size(), 0);
    ASSERT_TRUE(map.empty());
    ASSERT_EQ(map.get(key1), nullptr);
    ASSERT_EQ(map.get(key2), nullptr);
    ASSERT_EQ(map.get(key3), nullptr);
    ASSERT_FALSE(map.contains(key1));
    SlotMap<int>::Key key4 = map.emplace(40);
    ASSERT_TRUE(map.contains(key4));
    ASSERT_EQ(map.at(key4), 40);
    ASSERT_EQ(map.size(), 1);
    ASSERT_EQ(key4.index, 0);
}

TEST_F(SlotMapNewTest, ReserveAndCapacity) {
    SlotMap<int> map;
    size_t initial_capacity = map.capacity();
    ASSERT_LE(initial_capacity, 16);
    map.reserve(100);
    ASSERT_GE(map.capacity(), 100);
    for (int i = 0; i < 50; ++i) {
        map.emplace(i);
    }
    ASSERT_EQ(map.size(), 50);
    size_t capacity_after_50 = map.capacity();
    ASSERT_GE(capacity_after_50, 100);
    for (int i = 50; i < 100; ++i) {
        map.emplace(i);
    }
    ASSERT_EQ(map.size(), 100);
    if (capacity_after_50 >= 100) {
        ASSERT_EQ(map.capacity(), capacity_after_50);
    } else {
         ASSERT_GE(map.capacity(), 100);
    }
    map.shrink_to_fit();
    ASSERT_EQ(map.size(), 100);
    ASSERT_GT(map.max_size(), 0);
    SlotMap<int> map2;
    map2.emplace(1);
    size_t cap_before = map2.capacity();
    // Ensure cap_before is reasonable, otherwise the loop might be too long or short
    if (cap_before == 0) cap_before = 1; // Handle case where initial capacity is 0
    for(int i=0; i < (int)cap_before + 5; ++i) {
        map2.emplace(i + 2);
    }
    if (cap_before > 0) { // Only assert if there was a capacity to grow from.
        ASSERT_GT(map2.capacity(), cap_before);
    } else { // If initial capacity was 0, just check it's non-zero now.
        ASSERT_GT(map2.capacity(), 0);
    }
}

TEST_F(SlotMapNewTest, KeyValidity) {
    SlotMap<int> map;
    const SlotMap<int>::Key const_invalid_key = SlotMap<int>::INVALID_KEY;
    ASSERT_FALSE(const_invalid_key.is_valid());

    SlotMap<int>::Key default_key{}; // Default constructs to {0,0}
    // Key::is_valid() only checks index against INVALID_INDEX.
    // So {0,0} is "valid" in the sense that its index is not INVALID_INDEX.
    // This does not mean it points to a valid element in any specific map.
    ASSERT_TRUE(default_key.is_valid());
    // However, SlotMap::contains() uses a more robust check (is_valid_key).
    ASSERT_FALSE(map.contains(default_key));


    SlotMap<int>::Key k1 = map.emplace(10);
    ASSERT_TRUE(k1.is_valid()); // Structurally valid
    ASSERT_TRUE(map.contains(k1)); // Valid in this map

    map.erase(k1);
    ASSERT_TRUE(k1.is_valid()); // Structurally still valid (index != INVALID_INDEX)
    ASSERT_FALSE(map.contains(k1)); // But not valid in this map anymore
}

TEST_F(SlotMapNewTest, DestructorCalls) {
    DestructorTestType::destructor_call_count = 0; // Ensure clean state

    SlotMap<DestructorTestType> map;
    ASSERT_EQ(DestructorTestType::destructor_call_count, 0);

    SlotMap<DestructorTestType>::Key key1 = map.emplace(1);
    SlotMap<DestructorTestType>::Key key2 = map.emplace(2);
    ASSERT_EQ(DestructorTestType::destructor_call_count, 0); // No destructions yet

    map.erase(key1);
    ASSERT_EQ(DestructorTestType::destructor_call_count, 1); // Destructor for element 1

    // This emplace might reuse the slot from key1 if free_list is LIFO.
    // Placement new is used, so old object is already destroyed.
    SlotMap<DestructorTestType>::Key key3 = map.emplace(3);
    ASSERT_EQ(DestructorTestType::destructor_call_count, 1); // No new destructions

    map.erase(key2);
    ASSERT_EQ(DestructorTestType::destructor_call_count, 2); // Destructor for element 2

    map.clear(); // Element for key3 (value 3) should be destroyed
    ASSERT_EQ(DestructorTestType::destructor_call_count, 3);

    // Test destructor calls when SlotMap goes out of scope
    DestructorTestType::destructor_call_count = 0; // Reset for next part
    {
        SlotMap<DestructorTestType> local_map;
        local_map.emplace(100);
        local_map.emplace(200);
        ASSERT_EQ(DestructorTestType::destructor_call_count, 0);
    } // local_map goes out of scope
    ASSERT_EQ(DestructorTestType::destructor_call_count, 2); // Two elements (100, 200) destroyed
}


TEST_F(SlotMapNewTest, InsertMethod) {
    SlotMap<std::string> map;
    std::string s1 = "test_string";

    SlotMap<std::string>::Key key1 = map.insert(s1); // Lvalue insert
    ASSERT_NE(map.get(key1), nullptr);
    ASSERT_EQ(*(map.get(key1)), "test_string");
    ASSERT_EQ(map.at(key1), "test_string");

    SlotMap<std::string>::Key key2 = map.insert("another_string"); // Rvalue insert
    ASSERT_NE(map.get(key2), nullptr);
    ASSERT_EQ(*(map.get(key2)), "another_string");
    ASSERT_EQ(map.at(key2), "another_string");

    ASSERT_EQ(map.size(), 2);
}

TEST_F(SlotMapNewTest, ComparisonOperatorsForKey) {
    SlotMap<int>::Key k1{0,0}, k2{0,1}, k3{1,0}, k4{0,0};

    ASSERT_TRUE(k1 == k4);
    ASSERT_FALSE(k1 == k2);
    ASSERT_TRUE(k1 != k2);

    // Test operator<
    ASSERT_TRUE(k1 < k2); // Generation difference, same index
    ASSERT_TRUE(k1 < k3); // Index difference
    ASSERT_TRUE(k2 < k3); // Index difference (0 < 1)

    ASSERT_FALSE(k2 < k1); // k2 is not less than k1
    ASSERT_FALSE(k3 < k1); // k3 is not less than k1

    // Test with std::set (requires operator<)
    std::set<SlotMap<int>::Key> key_set;
    key_set.insert(k1);
    key_set.insert(k2);
    key_set.insert(k3);
    key_set.insert(k4); // k4 is same as k1, so set size should be 3

    ASSERT_EQ(key_set.size(), 3);
    ASSERT_TRUE(key_set.count(k1));
    ASSERT_TRUE(key_set.count(k2));
    ASSERT_TRUE(key_set.count(k3));
    ASSERT_TRUE(key_set.count(k4)); // Same as k1
}

TEST_F(SlotMapNewTest, MoveSemanticsSlotMap) {
    SlotMap<std::string> map1;
    SlotMap<std::string>::Key k1 = map1.emplace("Hello");
    map1.emplace("World");
    ASSERT_EQ(map1.size(), 2);

    // Test Move Construction
    SlotMap<std::string> map2 = std::move(map1);
    ASSERT_EQ(map2.size(), 2);
    ASSERT_NE(map2.get(k1), nullptr); // k1 should still be valid for map2
    ASSERT_EQ(*map2.get(k1), "Hello");

    // Standard guarantees that map1 is in a valid but unspecified state.
    // Often this means it's empty. Let's check common behavior.
    // ASSERT_TRUE(map1.empty()); // This depends on the specific std::vector move behavior.
    // For SlotMap, it defaults to std::vector's move, which should leave map1 empty.
     ASSERT_TRUE(map1.empty() || map1.size()==0);


    // Test Move Assignment
    SlotMap<std::string> map3;
    map3.emplace("Example");
    SlotMap<std::string>::Key k_map3 = map3.emplace("Data");
    ASSERT_EQ(map3.size(), 2);

    map1 = std::move(map3); // map1 was previously moved from, now assigned to
    ASSERT_EQ(map1.size(), 2);
    ASSERT_NE(map1.get(k_map3), nullptr);
    ASSERT_EQ(*map1.get(k_map3), "Data");
    // ASSERT_TRUE(map3.empty()); // map3 should be in a valid (likely empty) state.
    ASSERT_TRUE(map3.empty() || map3.size()==0);

}

// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

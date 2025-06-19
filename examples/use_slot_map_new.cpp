#include "slot_map_new.h"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <cassert>

using namespace utils;

// Test structures
struct Entity {
    int id;
    std::string name;
    float health;
    
    Entity(int i, const std::string& n, float h) : id(i), name(n), health(h) {}
    Entity() : id(0), name(""), health(0.0f) {}
};

struct GameObject {
    std::string type;
    int x, y;
    bool active;
    
    GameObject(const std::string& t, int px, int py) 
        : type(t), x(px), y(py), active(true) {}
};

// Test functions
void test_basic_operations() {
    std::cout << "=== Basic Operations Test ===\n";
    
    SlotMapNew<int> map;
    
    // Test insertion
    auto key1 = map.emplace(42);
    auto key2 = map.emplace(100);
    auto key3 = map.emplace(200);
    
    std::cout << "Inserted 3 elements, size: " << map.size() << "\n";
    
    // Test access
    if (auto* value = map.get(key1)) {
        std::cout << "key1 -> " << *value << "\n";
    }
    
    std::cout << "key2 -> " << map[key2] << "\n";
    std::cout << "key3 -> " << map.at(key3) << "\n";
    
    // Test contains
    std::cout << "Contains key1: " << map.contains(key1) << "\n";
    
    // Test erase
    bool erased = map.erase(key2);
    std::cout << "Erased key2: " << erased << ", new size: " << map.size() << "\n";
    
    // Test access to erased key
    if (auto* value = map.get(key2)) {
        std::cout << "ERROR: Should not be able to access erased key!\n";
    } else {
        std::cout << "Correctly unable to access erased key\n";
    }
    
    std::cout << "\n";
}

void test_generation_safety() {
    std::cout << "=== Generation Safety Test ===\n";
    
    SlotMapNew<std::string> map;
    
    auto key = map.emplace("Hello");
    std::cout << "Original key: index=" << key.index << ", gen=" << key.generation << "\n";
    
    // Store the key for later
    auto old_key = key;
    
    // Erase and reinsert
    map.erase(key);
    auto new_key = map.emplace("World");
    
    std::cout << "New key: index=" << new_key.index << ", gen=" << new_key.generation << "\n";
    
    // Try to access with old key (should fail)
    if (auto* value = map.get(old_key)) {
        std::cout << "ERROR: Old key should be invalid!\n";
    } else {
        std::cout << "Old key correctly invalidated\n";
    }
    
    // Access with new key (should work)
    if (auto* value = map.get(new_key)) {
        std::cout << "New key works: " << *value << "\n";
    }
    
    std::cout << "\n";
}

void test_complex_objects() {
    std::cout << "=== Complex Objects Test ===\n";
    
    SlotMapNew<Entity> entities;
    
    // Insert entities
    auto player = entities.emplace(1, "Player", 100.0f);
    auto enemy1 = entities.emplace(2, "Goblin", 50.0f);
    auto enemy2 = entities.emplace(3, "Dragon", 500.0f);
    
    std::cout << "Created " << entities.size() << " entities\n";
    
    // Access and modify
    if (auto* entity = entities.get(player)) {
        entity->health -= 25.0f;
        std::cout << entity->name << " health: " << entity->health << "\n";
    }
    
    // Remove enemy
    entities.erase(enemy1);
    std::cout << "After removing goblin, size: " << entities.size() << "\n";
    
    // Add new entity (should reuse slot)
    auto npc = entities.emplace(4, "Merchant", 75.0f);
    std::cout << "Added merchant, size: " << entities.size() << "\n";
    std::cout << "Merchant key: index=" << npc.index << ", gen=" << npc.generation << "\n";
    
    std::cout << "\n";
}

void test_iterator() {
    std::cout << "=== Iterator Test ===\n";
    
    SlotMapNew<GameObject> objects;
    
    // Add some objects
    objects.emplace("Player", 10, 20);
    auto enemy = objects.emplace("Enemy", 50, 60);
    objects.emplace("Pickup", 30, 40);
    
    // Remove one object
    objects.erase(enemy);
    
    // Add another
    objects.emplace("NPC", 70, 80);
    
    std::cout << "Active objects:\n";
    for (auto [key, obj] : objects) {
        std::cout << "  " << obj.type << " at (" << obj.x << ", " << obj.y 
                  << ") [key: " << key.index << "," << key.generation << "]\n";
    }
    
    std::cout << "\n";
}

void test_performance() {
    std::cout << "=== Performance Test ===\n";
    
    constexpr size_t NUM_OPERATIONS = 100000;
    SlotMapNew<int> map;
    map.reserve(NUM_OPERATIONS);
    
    std::vector<SlotMapNew<int>::Key> keys;
    keys.reserve(NUM_OPERATIONS);
    
    // Test insertion performance
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < NUM_OPERATIONS; ++i) {
        keys.push_back(map.emplace(static_cast<int>(i)));
    }
    
    auto mid = std::chrono::high_resolution_clock::now();
    
    // Test access performance
    int sum = 0;
    for (const auto& key : keys) {
        if (auto* value = map.get(key)) {
            sum += *value;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    auto insert_time = std::chrono::duration_cast<std::chrono::microseconds>(mid - start);
    auto access_time = std::chrono::duration_cast<std::chrono::microseconds>(end - mid);
    
    std::cout << "Inserted " << NUM_OPERATIONS << " elements in " 
              << insert_time.count() << " microseconds\n";
    std::cout << "Accessed " << NUM_OPERATIONS << " elements in " 
              << access_time.count() << " microseconds\n";
    std::cout << "Sum: " << sum << " (verification)\n";
    std::cout << "Final size: " << map.size() << "\n";
    
    std::cout << "\n";
}

void test_fragmentation_handling() {
    std::cout << "=== Fragmentation Handling Test ===\n";
    
    SlotMapNew<std::string> map;
    std::vector<SlotMapNew<std::string>::Key> keys;
    
    // Insert many elements
    for (int i = 0; i < 10; ++i) {
        keys.push_back(map.emplace("Item " + std::to_string(i)));
    }
    
    std::cout << "Initial size: " << map.size() << "\n";
    
    // Remove every other element
    for (size_t i = 1; i < keys.size(); i += 2) {
        map.erase(keys[i]);
    }
    
    std::cout << "After removing every other element: " << map.size() << "\n";
    
    // Add new elements (should reuse slots)
    std::vector<SlotMapNew<std::string>::Key> new_keys;
    for (int i = 0; i < 3; ++i) {
        new_keys.push_back(map.emplace("New Item " + std::to_string(i)));
    }
    
    std::cout << "After adding 3 new elements: " << map.size() << "\n";
    
    // Show which slots are being reused
    std::cout << "New element indices: ";
    for (const auto& key : new_keys) {
        std::cout << key.index << " ";
    }
    std::cout << "\n";
    
    std::cout << "\n";
}

void test_edge_cases() {
    std::cout << "=== Edge Cases Test ===\n";
    
    SlotMapNew<int> map;
    
    // Test empty map
    std::cout << "Empty map size: " << map.size() << "\n";
    std::cout << "Empty map is empty: " << map.empty() << "\n";
    
    // Test invalid key access
    SlotMapNew<int>::Key invalid_key{999, 0};
    if (auto* value = map.get(invalid_key)) {
        std::cout << "ERROR: Should not access invalid key!\n";
    } else {
        std::cout << "Invalid key correctly rejected\n";
    }
    
    // Test double erase
    auto key = map.emplace(42);
    bool first_erase = map.erase(key);
    bool second_erase = map.erase(key);
    
    std::cout << "First erase: " << first_erase << "\n";
    std::cout << "Second erase: " << second_erase << "\n";
    
    // Test clear
    map.emplace(1);
    map.emplace(2);
    map.emplace(3);
    std::cout << "Size before clear: " << map.size() << "\n";
    map.clear();
    std::cout << "Size after clear: " << map.size() << "\n";
    
    std::cout << "\n";
}

// Game-like usage example
void game_simulation_example() {
    std::cout << "=== Game Simulation Example ===\n";
    
    SlotMapNew<Entity> entities;
    
    // Create game entities
    auto player = entities.emplace(1, "Hero", 100.0f);
    auto goblin1 = entities.emplace(2, "Goblin", 30.0f);
    auto goblin2 = entities.emplace(3, "Goblin", 30.0f);
    auto treasure = entities.emplace(4, "Treasure", 1.0f);
    
    std::cout << "Game started with " << entities.size() << " entities\n";
    
    // Simulate combat
    if (auto* p = entities.get(player)) {
        p->health -= 15.0f;
        std::cout << p->name << " takes damage, health: " << p->health << "\n";
    }
    
    // Goblin dies
    entities.erase(goblin1);
    std::cout << "Goblin defeated! Entities remaining: " << entities.size() << "\n";
    
    // Spawn new enemy in the freed slot
    auto orc = entities.emplace(5, "Orc", 60.0f);
    std::cout << "Orc spawned! Entity count: " << entities.size() << "\n";
    std::cout << "Orc uses slot index: " << orc.index << "\n";
    
    // List all active entities
    std::cout << "Active entities:\n";
    for (auto [key, entity] : entities) {
        std::cout << "  " << entity.name << " (ID: " << entity.id 
                  << ", Health: " << entity.health << ")\n";
    }
    
    std::cout << "\n";
}

int main() {
    std::cout << "SlotMap Implementation Tests\n";
    std::cout << "============================\n\n";
    
    try {
        test_basic_operations();
        test_generation_safety();
        test_complex_objects();
        test_iterator();
        test_fragmentation_handling();
        test_edge_cases();
        test_performance();
        game_simulation_example();
        
        std::cout << "All tests completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

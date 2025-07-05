#include "sparse_set.h" // Assuming this path is correct relative to include dir
#include <iostream>
#include <vector>
#include <algorithm> // For std::shuffle
#include <random>    // For std::mt19937, std::random_device
#include <cassert>   // For assert

int main() {
    std::cout << "--- SparseSet Usage Example ---" << std::endl;

    // Create a SparseSet that can hold elements up to 999 (i.e., max_value_capacity = 1000)
    // Defaulting to uint32_t for element type and index type.
    cpp_collections::SparseSet<> game_entities(1000);

    // 1. Inserting elements (e.g., activating entities)
    std::cout << "\n1. Inserting entities:" << std::endl;
    game_entities.insert(10);
    game_entities.insert(250);
    game_entities.insert(5);
    game_entities.insert(800);

    std::cout << "Set size: " << game_entities.size() << std::endl; // Expected: 4

    // Trying to insert an existing element
    auto result = game_entities.insert(10);
    if (!result.second) {
        std::cout << "Entity " << *result.first << " was already present." << std::endl;
    }

    // 2. Checking for containment
    std::cout << "\n2. Checking entity status:" << std::endl;
    uint32_t entity_to_check = 250;
    if (game_entities.contains(entity_to_check)) {
        std::cout << "Entity " << entity_to_check << " is active." << std::endl;
    } else {
        std::cout << "Entity " << entity_to_check << " is NOT active." << std::endl;
    }

    entity_to_check = 100; // This entity was not inserted
    if (game_entities.contains(entity_to_check)) {
        std::cout << "Entity " << entity_to_check << " is active." << std::endl;
    } else {
        std::cout << "Entity " << entity_to_check << " is NOT active." << std::endl;
    }

    // Check out of range entity
    entity_to_check = 2000; // max_value_capacity is 1000
    if (game_entities.contains(entity_to_check)) {
        std::cout << "Entity " << entity_to_check << " is active (ERROR: should be out of range)." << std::endl;
    } else {
        std::cout << "Entity " << entity_to_check << " is not active (correctly identified as out of range or not present)." << std::endl;
    }


    // 3. Iterating over active entities
    // The order of iteration is the order in the dense array, not sorted by value.
    std::cout << "\n3. Active entities (iteration order):" << std::endl;
    for (uint32_t entity_id : game_entities) {
        std::cout << entity_id << " ";
    }
    std::cout << std::endl;

    // 4. Erasing elements (e.g., deactivating entities)
    std::cout << "\n4. Deactivating entity 250:" << std::endl;
    bool erased = game_entities.erase(250);
    if (erased) {
        std::cout << "Entity 250 deactivated." << std::endl;
    }
    std::cout << "Set size after erase: " << game_entities.size() << std::endl; // Expected: 3
    if (!game_entities.contains(250)) {
        std::cout << "Entity 250 is confirmed inactive." << std::endl;
    }

    // Try erasing a non-existent entity
    erased = game_entities.erase(100); // Was never there
    if (!erased) {
        std::cout << "Entity 100 was not found to deactivate." << std::endl;
    }

    // 5. Find an element
    std::cout << "\n5. Finding entity 5:" << std::endl;
    auto it = game_entities.find(5);
    if (it != game_entities.end()) {
        std::cout << "Found entity " << *it << "." << std::endl;
    } else {
        std::cout << "Entity 5 not found." << std::endl;
    }

    it = game_entities.find(250); // Was erased
    if (it == game_entities.end()) {
        std::cout << "Entity 250 (erased) correctly not found." << std::endl;
    }


    // 6. Simulating many additions and removals
    std::cout << "\n6. Simulating additions and removals:" << std::endl;
    std::vector<uint32_t> entity_ids;
    for (uint32_t i = 0; i < 500; ++i) { // Insert entities up to 499
        if (i % 3 == 0) entity_ids.push_back(i);
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(entity_ids.begin(), entity_ids.end(), g);

    cpp_collections::SparseSet<uint32_t> dynamic_set(600); // Max value 599
    int ops = 0;
    for (uint32_t id : entity_ids) {
        dynamic_set.insert(id);
        ops++;
        if (ops % 10 == 0 && !dynamic_set.empty()) {
            // Remove a random element from the ones currently in the set
            // This is a bit tricky without direct access to dense array elements by index easily
            // For demo, just remove the one we just added if it's not the only one
            if (dynamic_set.size() > 1 && ops % 20 == 0) { // Remove some, not all
                 dynamic_set.erase(id); // remove the one just added
            }
        }
    }
    std::cout << "After dynamic operations, set size: " << dynamic_set.size() << std::endl;
    std::cout << "First 5 elements in dynamic_set (iteration order): ";
    int print_count = 0;
    for(uint32_t id : dynamic_set) {
        std::cout << id << " ";
        print_count++;
        if (print_count >= 5) break;
    }
    std::cout << (print_count == 0 ? "(empty)" : "") << std::endl;


    // 7. Clear the set
    std::cout << "\n7. Clearing all entities:" << std::endl;
    game_entities.clear();
    std::cout << "Set size after clear: " << game_entities.size() << std::endl; // Expected: 0
    assert(game_entities.empty());

    std::cout << "\n--- SparseSet Example Finished ---" << std::endl;

    return 0;
}

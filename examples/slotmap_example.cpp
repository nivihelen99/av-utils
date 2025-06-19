#include "SlotMap.h" // Adjust path if needed, assuming SlotMap.h is in include/ and examples/ is at the same level as include/
#include <iostream>
#include <string>
#include <vector>

// Helper function to print key
void print_key(const utils::SlotMap<std::string>::Key& key) {
    std::cout << "Key(index: " << key.index << ", gen: " << key.generation << ")";
}

int main() {
    utils::SlotMap<std::string> map;

    std::cout << "Initial map size: " << map.size() << ", empty: " << map.empty() << std::endl;

    // Insert elements
    std::cout << "\n--- Inserting elements ---" << std::endl;
    utils::SlotMap<std::string>::Key key1 = map.insert("Hello");
    std::cout << "Inserted \"Hello\", got "; print_key(key1); std::cout << std::endl;

    utils::SlotMap<std::string>::Key key2 = map.insert("World");
    std::cout << "Inserted \"World\", got "; print_key(key2); std::cout << std::endl;

    utils::SlotMap<std::string>::Key key3 = map.insert("SlotMap");
    std::cout << "Inserted \"SlotMap\", got "; print_key(key3); std::cout << std::endl;

    std::cout << "Map size after inserts: " << map.size() << ", empty: " << map.empty() << std::endl;

    // Retrieve elements
    std::cout << "\n--- Retrieving elements ---" << std::endl;
    if (const std::string* val1 = map.get(key1)) {
        std::cout << "Value for key1: " << *val1 << std::endl;
    }
    if (const std::string* val2 = map.get(key2)) {
        std::cout << "Value for key2: " << *val2 << std::endl;
    }
    if (const std::string* val3 = map.get(key3)) {
        std::cout << "Value for key3: " << *val3 << std::endl;
    }

    // Check contains
    std::cout << "\n--- Checking contains ---" << std::endl;
    std::cout << "map.contains(key1): " << map.contains(key1) << std::endl;
    std::cout << "map.contains(key2): " << map.contains(key2) << std::endl;

    // Erase an element
    std::cout << "\n--- Erasing an element ---" << std::endl;
    std::cout << "Erasing key2 (World)..." << std::endl;
    bool erased = map.erase(key2);
    std::cout << "Erase successful: " << erased << std::endl;
    std::cout << "Map size after erase: " << map.size() << std::endl;

    std::cout << "map.contains(key1) after erasing key2: " << map.contains(key1) << std::endl;
    std::cout << "map.contains(key2) after erasing key2: " << map.contains(key2) << std::endl;

    // Try to get erased element
    std::cout << "\n--- Retrieving erased element ---" << std::endl;
    if (const std::string* val2_after_erase = map.get(key2)) {
        std::cout << "Value for key2 after erase: " << *val2_after_erase << " (Error, should not happen!)" << std::endl;
    } else {
        std::cout << "Value for key2 after erase: nullptr (Correct!)" << std::endl;
    }

    // Stale key (key2 has old generation)
    utils::SlotMap<std::string>::Key stale_key2 = key2; // generation is now outdated

    // Insert more elements (should reuse slot from key2)
    std::cout << "\n--- Inserting after erase (reuse) ---" << std::endl;
    utils::SlotMap<std::string>::Key key4 = map.insert("Reusable");
    std::cout << "Inserted \"Reusable\", got "; print_key(key4); std::cout << " (Note if index is same as key2's index)" << std::endl;

    std::cout << "Map size: " << map.size() << std::endl;

    if (const std::string* val4 = map.get(key4)) {
        std::cout << "Value for key4: " << *val4 << std::endl;
    }

    // Try to get with stale key
    std::cout << "\n--- Retrieving with stale key ---" << std::endl;
    if (const std::string* val_stale = map.get(stale_key2)) {
        std::cout << "Value for stale_key2: " << *val_stale << " (Error, should not happen!)" << std::endl;
    } else {
        std::cout << "Value for stale_key2: nullptr (Correct!)" << std::endl;
    }
    std::cout << "map.contains(stale_key2): " << map.contains(stale_key2) << " (Should be false)" << std::endl;
    std::cout << "map.contains(key4): " << map.contains(key4) << std::endl;


    std::cout << "\n--- Testing with more complex types (struct) ---" << std::endl;
    struct MyStruct {
        int id;
        std::string name;

        bool operator==(const MyStruct& other) const {
            return id == other.id && name == other.name;
        }
    };

    utils::SlotMap<MyStruct> struct_map;
    auto s_key1 = struct_map.insert({1, "StructA"});
    auto s_key2 = struct_map.insert({2, "StructB"});

    if(const MyStruct* s_val1 = struct_map.get(s_key1)) {
        std::cout << "StructA: id=" << s_val1->id << ", name=" << s_val1->name << std::endl;
    }
    struct_map.erase(s_key1);
    std::cout << "struct_map.contains(s_key1) after erase: " << struct_map.contains(s_key1) << std::endl;
    auto s_key3 = struct_map.insert({3, "StructC"}); // Potentially reuses s_key1's slot
     if(const MyStruct* s_val3 = struct_map.get(s_key3)) {
        std::cout << "StructC: id=" << s_val3->id << ", name=" << s_val3->name << std::endl;
    }
    std::cout << "Struct map size: " << struct_map.size() << std::endl;

    std::cout << "\n--- Done ---" << std::endl;

    return 0;
}

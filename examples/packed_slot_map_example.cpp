#include "PackedSlotMap.h" // Assuming PackedSlotMap.h is in the include path
#include <iostream>
#include <string>
#include <vector>
#include <cassert>

// A simple struct to store as values in the PackedSlotMap
struct MyObject {
    int id;
    std::string name;

    MyObject(int i, std::string n) : id(i), name(std::move(n)) {}

    // For printing
    friend std::ostream& operator<<(std::ostream& os, const MyObject& obj) {
        os << "MyObject{id: " << obj.id << ", name: \"" << obj.name << "\"}";
        return os;
    }
};

int main() {
    std::cout << "PackedSlotMap Example\n" << std::endl;

    PackedSlotMap<MyObject> psm;

    // 1. Insertion
    std::cout << "--- 1. Insertion ---" << std::endl;
    PackedSlotMap<MyObject>::Key key1 = psm.insert(MyObject(1, "Alice"));
    PackedSlotMap<MyObject>::Key key2 = psm.emplace(2, "Bob"); // Using emplace
    PackedSlotMap<MyObject>::Key key3 = psm.insert(MyObject(3, "Charlie"));
    PackedSlotMap<MyObject>::Key key4 = psm.insert(MyObject(4, "David"));

    std::cout << "Inserted 4 objects. Current size: " << psm.size() << std::endl;
    assert(psm.size() == 4);

    // 2. Retrieval
    std::cout << "\n--- 2. Retrieval ---" << std::endl;
    MyObject* obj_ptr = psm.get(key2);
    if (obj_ptr) {
        std::cout << "Retrieved by key2: " << *obj_ptr << std::endl;
        assert(obj_ptr->name == "Bob");
    } else {
        std::cout << "Failed to retrieve object by key2." << std::endl;
    }

    const MyObject* c_obj_ptr = psm.get(key3);
    if (c_obj_ptr) {
        std::cout << "Retrieved by key3 (const): " << *c_obj_ptr << std::endl;
        assert(c_obj_ptr->name == "Charlie");
    } else {
        std::cout << "Failed to retrieve object by key3 (const)." << std::endl;
    }

    // Modify retrieved object
    if(obj_ptr) {
        obj_ptr->name = "Robert";
        std::cout << "Modified key2 object's name to Robert." << std::endl;
        MyObject* updated_obj_ptr = psm.get(key2);
        assert(updated_obj_ptr && updated_obj_ptr->name == "Robert");
    }


    // 3. Iteration
    std::cout << "\n--- 3. Iteration ---" << std::endl;
    std::cout << "Iterating through all objects:" << std::endl;
    for (const MyObject& obj : psm) {
        std::cout << "  " << obj << std::endl;
    }
    // Also test const iteration
    std::cout << "Iterating using cbegin/cend:" << std::endl;
    for (auto it = psm.cbegin(); it != psm.cend(); ++it) {
        std::cout << "  " << *it << std::endl;
    }


    // 4. Erasure
    std::cout << "\n--- 4. Erasure ---" << std::endl;
    std::cout << "Erasing object with key2 (Bob/Robert)..." << std::endl;
    bool erased_key2 = psm.erase(key2);
    if (erased_key2) {
        std::cout << "Successfully erased object with key2. New size: " << psm.size() << std::endl;
        assert(psm.size() == 3);
    } else {
        std::cout << "Failed to erase object with key2." << std::endl;
    }

    std::cout << "Attempting to retrieve key2 after erasure: ";
    obj_ptr = psm.get(key2);
    if (obj_ptr) {
        std::cout << "Found " << *obj_ptr << " (ERROR, should be null)" << std::endl;
    } else {
        std::cout << "Not found (Correct)" << std::endl;
        assert(obj_ptr == nullptr);
    }
    assert(!psm.contains(key2));

    std::cout << "\nObjects after erasing key2:" << std::endl;
    for (const MyObject& obj : psm) {
        std::cout << "  " << obj << std::endl;
    }

    // Key stability check: key1, key3, key4 should still be valid if they weren't the one moved.
    // The element at key4 might have been moved into key2's old data slot.
    // Let's verify key1 and key3 are fine.
    std::cout << "\nChecking stability of other keys:" << std::endl;
    MyObject* obj1_after_erase = psm.get(key1);
    if (obj1_after_erase) {
        std::cout << "key1 still valid: " << *obj1_after_erase << std::endl;
        assert(obj1_after_erase->name == "Alice");
    } else {
        std::cout << "key1 became invalid (ERROR)" << std::endl;
    }

    MyObject* obj3_after_erase = psm.get(key3);
    if (obj3_after_erase) {
        std::cout << "key3 still valid: " << *obj3_after_erase << std::endl;
        assert(obj3_after_erase->name == "Charlie");
    } else {
        std::cout << "key3 became invalid (ERROR)" << std::endl;
    }

    // Let's see what key4 points to. If key2 was not the last data element,
    // key4's data would have been moved. But key4 itself (the handle) should still be valid.
    MyObject* obj4_after_erase = psm.get(key4);
     if (obj4_after_erase) {
        std::cout << "key4 still valid: " << *obj4_after_erase << std::endl;
        assert(obj4_after_erase->name == "David");
    } else {
        std::cout << "key4 became invalid (ERROR)" << std::endl;
    }


    std::cout << "\nErasing object with key4 (David)..." << std::endl;
    psm.erase(key4);
    std::cout << "Size after erasing key4: " << psm.size() << std::endl;
    assert(psm.size() == 2);
    assert(!psm.contains(key4));

    std::cout << "\nObjects after erasing key4:" << std::endl;
    for (const MyObject& obj : psm) {
        std::cout << "  " << obj << std::endl;
    }

    // 5. Contains
    std::cout << "\n--- 5. Contains ---" << std::endl;
    std::cout << "Contains key1? " << (psm.contains(key1) ? "Yes" : "No") << std::endl;
    assert(psm.contains(key1));
    std::cout << "Contains key2 (erased)? " << (psm.contains(key2) ? "Yes" : "No") << std::endl;
    assert(!psm.contains(key2));

    // 6. Clear and Empty
    std::cout << "\n--- 6. Clear and Empty ---" << std::endl;
    std::cout << "Is psm empty before clear? " << (psm.empty() ? "Yes" : "No") << std::endl;
    assert(!psm.empty());
    psm.clear();
    std::cout << "Cleared psm. Size: " << psm.size() << std::endl;
    assert(psm.size() == 0);
    std::cout << "Is psm empty after clear? " << (psm.empty() ? "Yes" : "No") << std::endl;
    assert(psm.empty());

    // 7. Reuse keys (generations)
    std::cout << "\n--- 7. Reuse keys (generations) ---" << std::endl;
    PackedSlotMap<MyObject>::Key key5 = psm.insert(MyObject(5, "Eve"));
    // key2's slot might be reused. If so, key5.slot_idx could be key2.slot_idx,
    // but key5.generation will be different from key2.generation.
    std::cout << "Inserted key5. slot_idx: " << key5.slot_idx << ", generation: " << key5.generation << std::endl;
    std::cout << "Old key2 was: slot_idx: " << key2.slot_idx << ", generation: " << key2.generation << std::endl;

    MyObject* obj5_ptr = psm.get(key5);
    if (obj5_ptr) {
        std::cout << "Retrieved by key5: " << *obj5_ptr << std::endl;
        assert(obj5_ptr->name == "Eve");
    }

    std::cout << "Attempting to retrieve using old key2 (should fail due to generation): ";
    if (psm.get(key2)) {
        std::cout << "Found " << *psm.get(key2) << " (ERROR, should be null due to generation)" << std::endl;
    } else {
        std::cout << "Not found (Correct)" << std::endl;
    }
    assert(!psm.contains(key2)); // contains checks generation too

    std::cout << "\nExample finished successfully." << std::endl;

    return 0;
}

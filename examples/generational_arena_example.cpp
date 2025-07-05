#include "GenerationalArena.h"
#include <iostream>
#include <string>
#include <vector>

// A simple struct to store in the arena
struct MyObject {
    int id;
    std::string name;
    float value;

    MyObject(int i, std::string n, float v) : id(i), name(std::move(n)), value(v) {
        // std::cout << "MyObject Constructor: id=" << id << ", name=" << name << std::endl;
    }

    // Example: Add a destructor to see it being called by the arena
    ~MyObject() {
        // std::cout << "MyObject Destructor: id=" << id << ", name=" << name << std::endl;
    }

    void print() const {
        std::cout << "MyObject { id: " << id << ", name: \"" << name << "\", value: " << value << " }" << std::endl;
    }
};

int main() {
    // Create a GenerationalArena for MyObject
    cpp_collections::GenerationalArena<MyObject> arena;

    std::cout << "Arena created. Initial size: " << arena.size() << ", capacity: " << arena.capacity() << std::endl;

    // Allocate some objects
    cpp_collections::ArenaHandle handle1 = arena.allocate(1, "ObjectOne", 10.5f);
    cpp_collections::ArenaHandle handle2 = arena.allocate(2, "ObjectTwo", 20.2f);
    cpp_collections::ArenaHandle handle3 = arena.allocate(3, "ObjectThree", 30.9f);

    std::cout << "\nAfter allocating 3 objects:" << std::endl;
    std::cout << "Arena size: " << arena.size() << ", capacity: " << arena.capacity() << std::endl;

    // Access and print objects using handles
    std::cout << "\nAccessing objects via handles:" << std::endl;
    if (MyObject* obj1 = arena.get(handle1)) {
        obj1->print();
    }
    if (const MyObject* obj2 = arena.get(handle2)) {
        obj2->print();
        // obj2->value = 22.2f; // Error: obj2 is const
    }
     if (MyObject* obj3_mutable = arena.get(handle3)) {
        obj3_mutable->value = 33.3f; // Modify value
        obj3_mutable->print();
    }


    // Iterate over objects in the arena
    std::cout << "\nIterating over objects in the arena:" << std::endl;
    for (const MyObject& obj : arena) {
        obj.print();
    }

    // Deallocate an object
    std::cout << "\nDeallocating ObjectTwo (handle2)..." << std::endl;
    arena.deallocate(handle2);
    std::cout << "Arena size after deallocating handle2: " << arena.size() << std::endl;

    // Try to access the deallocated object (should return nullptr)
    std::cout << "\nTrying to access deallocated handle2:" << std::endl;
    if (arena.get(handle2) == nullptr) {
        std::cout << "Access to handle2 failed (as expected after deallocation)." << std::endl;
    } else {
        std::cout << "ERROR: Access to handle2 succeeded (unexpected!)." << std::endl;
    }

    // Check validity of handles
    std::cout << "Is handle1 valid? " << (arena.is_valid(handle1) ? "Yes" : "No") << std::endl;
    std::cout << "Is handle2 valid? " << (arena.is_valid(handle2) ? "Yes" : "No") << std::endl;


    // Allocate a new object - it might reuse the slot from handle2
    std::cout << "\nAllocating a new object (ObjectFour)..." << std::endl;
    cpp_collections::ArenaHandle handle4 = arena.allocate(4, "ObjectFour", 40.0f);
    std::cout << "Arena size: " << arena.size() << ", capacity: " << arena.capacity() << std::endl;

    std::cout << "Details of handle4: index=" << handle4.index << ", generation=" << handle4.generation << std::endl;
    if (handle2.index == handle4.index) {
        std::cout << "ObjectFour (handle4) reused the slot of ObjectTwo (handle2)." << std::endl;
        std::cout << "handle2 generation: " << handle2.generation << ", handle4 generation: " << handle4.generation << std::endl;
    }


    // Access ObjectFour
    if (MyObject* obj4 = arena.get(handle4)) {
        obj4->print();
    }

    // Old handle2 should still be invalid
    std::cout << "\nIs old handle2 still valid after slot reuse? " << (arena.is_valid(handle2) ? "Yes (Error!)" : "No (Correct)") << std::endl;

    std::cout << "\nIterating again:" << std::endl;
    for (MyObject& obj : arena) { // Non-const iteration
        obj.value += 1.0f; // Modify objects during iteration
        obj.print();
    }

    std::cout << "\nObjects after modification during iteration:" << std::endl;
     if (MyObject* obj1 = arena.get(handle1)) obj1->print();
     if (MyObject* obj3 = arena.get(handle3)) obj3->print();
     if (MyObject* obj4 = arena.get(handle4)) obj4->print();


    // Test clear
    std::cout << "\nClearing the arena..." << std::endl;
    arena.clear();
    std::cout << "Arena size after clear: " << arena.size() << ", capacity: " << arena.capacity() << std::endl;
    std::cout << "Is handle1 valid after clear? " << (arena.is_valid(handle1) ? "Yes (Error!)" : "No (Correct)") << std::endl;

    // Test reserve
    cpp_collections::GenerationalArena<int> int_arena;
    std::cout << "\nTesting with int arena. Initial capacity: " << int_arena.capacity() << std::endl;
    int_arena.reserve(100);
    std::cout << "Capacity after reserving 100: " << int_arena.capacity() << std::endl;
    cpp_collections::ArenaHandle h_int = int_arena.allocate(123);
    if(int* val = int_arena.get(h_int)) {
        std::cout << "Allocated int: " << *val << std::endl;
    }
    std::cout << "Int arena size: " << int_arena.size() << ", capacity: " << int_arena.capacity() << std::endl;


    std::cout << "\nExample finished." << std::endl;

    return 0;
}

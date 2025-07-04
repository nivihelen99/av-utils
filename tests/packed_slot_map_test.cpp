#include "PackedSlotMap.h" // Adjust path as necessary
#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <algorithm> // For std::sort, std::find
#include <set>       // For checking uniqueness of iterated elements

// Helper to print test messages
void print_test_header(const std::string& test_name) {
    std::cout << "\n--- Testing: " << test_name << " ---" << std::endl;
}

struct TestObject {
    int id;
    std::string data;

    TestObject(int i = 0, std::string d = "") : id(i), data(std::move(d)) {}

    bool operator==(const TestObject& other) const {
        return id == other.id && data == other.data;
    }
    // Add operator< for std::set if TestObject itself is used in a set.
    // For these tests, we typically store pointers or keys.
    bool operator<(const TestObject& other) const {
        if (id != other.id) return id < other.id;
        return data < other.data;
    }
};

std::ostream& operator<<(std::ostream& os, const TestObject& obj) {
    os << "TestObject{id:" << obj.id << ", data:\"" << obj.data << "\"}";
    return os;
}


void test_construction_and_basic_properties() {
    print_test_header("Construction and Basic Properties");
    PackedSlotMap<TestObject> psm;
    assert(psm.empty());
    assert(psm.size() == 0);
    std::cout << "Default construction: PASSED" << std::endl;

    PackedSlotMap<int> psm_int;
    assert(psm_int.empty());
    assert(psm_int.size() == 0);
    std::cout << "Default construction (int): PASSED" << std::endl;
}

void test_insertion_and_retrieval() {
    print_test_header("Insertion and Retrieval");
    PackedSlotMap<TestObject> psm;

    TestObject obj1(1, "one");
    PackedSlotMap<TestObject>::Key key1 = psm.insert(obj1);
    assert(psm.size() == 1);
    assert(!psm.empty());
    assert(psm.contains(key1));

    const TestObject* p_obj1 = psm.get(key1);
    assert(p_obj1 != nullptr);
    assert(p_obj1->id == 1 && p_obj1->data == "one");
    std::cout << "Insert and get: PASSED" << std::endl;

    TestObject* p_mut_obj1 = psm.get(key1);
    assert(p_mut_obj1 != nullptr);
    p_mut_obj1->data = "first";
    const TestObject* p_updated_obj1 = psm.get(key1);
    assert(p_updated_obj1 != nullptr && p_updated_obj1->data == "first");
    std::cout << "Modify via get: PASSED" << std::endl;

    PackedSlotMap<TestObject>::Key key2 = psm.emplace(2, "two");
    assert(psm.size() == 2);
    assert(psm.contains(key2));
    const TestObject* p_obj2 = psm.get(key2);
    assert(p_obj2 != nullptr && p_obj2->id == 2 && p_obj2->data == "two");
    std::cout << "Emplace and get: PASSED" << std::endl;

    // Test get with invalid key
    PackedSlotMap<TestObject>::Key invalid_key = {key2.slot_idx + 100, key2.generation}; // unlikely to be valid
    assert(psm.get(invalid_key) == nullptr);
    PackedSlotMap<TestObject>::Key wrong_gen_key = {key1.slot_idx, key1.generation + 1};
    assert(psm.get(wrong_gen_key) == nullptr);
    std::cout << "Get with invalid/wrong-generation key: PASSED" << std::endl;
}

void test_erasure() {
    print_test_header("Erasure");
    PackedSlotMap<TestObject> psm;

    PackedSlotMap<TestObject>::Key key1 = psm.insert(TestObject(1, "one"));
    PackedSlotMap<TestObject>::Key key2 = psm.insert(TestObject(2, "two"));
    PackedSlotMap<TestObject>::Key key3 = psm.insert(TestObject(3, "three"));
    assert(psm.size() == 3);

    // Erase middle element
    bool erased = psm.erase(key2);
    assert(erased);
    assert(psm.size() == 2);
    assert(!psm.contains(key2));
    assert(psm.get(key2) == nullptr);
    assert(psm.contains(key1)); // key1 should still be valid
    assert(psm.get(key1) != nullptr && psm.get(key1)->id == 1);
    assert(psm.contains(key3)); // key3 should still be valid
    assert(psm.get(key3) != nullptr && psm.get(key3)->id == 3);
    std::cout << "Erase middle element: PASSED" << std::endl;

    // Erase first element (after previous erase, key1 might be first or key3 might have moved)
    // To be specific, let's re-get key1 and key3 to know their current values for sure.
    // Then erase key1
    erased = psm.erase(key1);
    assert(erased);
    assert(psm.size() == 1);
    assert(!psm.contains(key1));
    assert(psm.get(key1) == nullptr);
    assert(psm.contains(key3)); // key3 should still be valid
    assert(psm.get(key3) != nullptr && psm.get(key3)->id == 3);
    std::cout << "Erase first element (relative): PASSED" << std::endl;

    // Erase last remaining element
    erased = psm.erase(key3);
    assert(erased);
    assert(psm.size() == 0);
    assert(psm.empty());
    assert(!psm.contains(key3));
    assert(psm.get(key3) == nullptr);
    std::cout << "Erase last element: PASSED" << std::endl;

    // Attempt to erase already erased key
    erased = psm.erase(key1);
    assert(!erased);
    std::cout << "Erase already erased key: PASSED" << std::endl;

    // Attempt to erase from empty map
    PackedSlotMap<TestObject> psm_empty;
    PackedSlotMap<TestObject>::Key dummy_key = {0, 0};
    erased = psm_empty.erase(dummy_key);
    assert(!erased);
    std::cout << "Erase from empty map: PASSED" << std::endl;
}

void test_key_stability_and_generation() {
    print_test_header("Key Stability and Generation");
    PackedSlotMap<TestObject> psm;
    std::vector<PackedSlotMap<TestObject>::Key> keys;
    const int num_elements = 5;

    for (int i = 0; i < num_elements; ++i) {
        keys.push_back(psm.insert(TestObject(i, "obj" + std::to_string(i))));
    }
    assert(psm.size() == num_elements);

    // Erase key at index 2 (value id 2)
    PackedSlotMap<TestObject>::Key erased_key = keys[2];
    psm.erase(erased_key);
    assert(psm.size() == num_elements - 1);
    assert(!psm.contains(erased_key));
    assert(psm.get(erased_key) == nullptr);

    // Check other keys are still valid and point to correct data
    for (int i = 0; i < num_elements; ++i) {
        if (i == 2) continue; // Skip erased key
        assert(psm.contains(keys[i]));
        const TestObject* obj = psm.get(keys[i]);
        assert(obj != nullptr);
        assert(obj->id == i); // Data integrity
        assert(obj->data == "obj" + std::to_string(i));
    }
    std::cout << "Key stability after erasure: PASSED" << std::endl;

    // Re-insert to potentially reuse slot of keys[2]
    PackedSlotMap<TestObject>::Key new_key = psm.insert(TestObject(100, "new_obj"));
    assert(psm.size() == num_elements); // Back to original count

    // Check if the old erased_key is still invalid (due to generation)
    assert(!psm.contains(erased_key));
    assert(psm.get(erased_key) == nullptr);
    std::cout << "Old key invalid after slot reuse (generation check): PASSED" << std::endl;

    // Check new_key is valid
    assert(psm.contains(new_key));
    const TestObject* new_obj_ptr = psm.get(new_key);
    assert(new_obj_ptr != nullptr && new_obj_ptr->id == 100);

    // If new_key reused erased_key's slot_idx:
    if (new_key.slot_idx == erased_key.slot_idx) {
        assert(new_key.generation != erased_key.generation);
        std::cout << "Slot reused, generation incremented: PASSED" << std::endl;
    }
}

void test_iteration() {
    print_test_header("Iteration");
    PackedSlotMap<TestObject> psm;
    std::vector<TestObject> source_objects;
    const int num_elements = 5;

    for (int i = 0; i < num_elements; ++i) {
        source_objects.emplace_back(i, "iter_obj" + std::to_string(i));
        psm.insert(source_objects.back());
    }

    // Collect iterated elements
    std::vector<TestObject> iterated_objects;
    for (const TestObject& obj : psm) {
        iterated_objects.push_back(obj);
    }
    assert(iterated_objects.size() == num_elements);

    // Sort both to compare contents, as PackedSlotMap iteration order depends on erase/moves
    std::sort(source_objects.begin(), source_objects.end());
    std::sort(iterated_objects.begin(), iterated_objects.end());
    assert(iterated_objects == source_objects);
    std::cout << "Forward iteration content matches source: PASSED" << std::endl;

    // Test const iteration
    std::vector<TestObject> const_iterated_objects;
    const PackedSlotMap<TestObject>& const_psm = psm;
    for (const TestObject& obj : const_psm) {
        const_iterated_objects.push_back(obj);
    }
    std::sort(const_iterated_objects.begin(), const_iterated_objects.end());
    assert(const_iterated_objects == source_objects);
    std::cout << "Const forward iteration content matches source: PASSED" << std::endl;

    // Test iteration after erasures
    psm.erase(psm.insert(TestObject(99, "temp"))); // Insert and immediately get key to erase

    // Erase first original element (id 0)
    PackedSlotMap<TestObject>::Key key_to_erase_id0;
    bool found_id0 = false; // Find key for id 0
    // We need a way to get a key from a value, or iterate and find its key.
    // For this test, let's find it by iterating data_entries directly (if available and safe)
    // or by inserting known items and keeping their keys.
    // Re-setup for simpler key tracking for erasure:
    psm.clear();
    source_objects.clear();
    std::vector<PackedSlotMap<TestObject>::Key> keys_for_iter_test;
    for (int i = 0; i < num_elements; ++i) {
        source_objects.emplace_back(i, "iter_obj" + std::to_string(i));
        keys_for_iter_test.push_back(psm.insert(source_objects.back()));
    }

    psm.erase(keys_for_iter_test[0]); // Erase element with id 0
    psm.erase(keys_for_iter_test[num_elements -1]); // Erase element with id num_elements-1

    iterated_objects.clear();
    for (const TestObject& obj : psm) {
        iterated_objects.push_back(obj);
    }
    assert(iterated_objects.size() == num_elements - 2);

    // Check that remaining elements are correct
    std::set<int> remaining_ids;
    for(const auto& obj : iterated_objects) {
        remaining_ids.insert(obj.id);
    }
    for(int i = 1; i < num_elements - 1; ++i) { // IDs 1, 2, 3 for num_elements = 5
        assert(remaining_ids.count(i));
    }
    std::cout << "Iteration after erasures: PASSED" << std::endl;

    // Test empty iteration
    psm.clear();
    int count_empty_iter = 0;
    for (const TestObject& obj : psm) {
        (void)obj; // Suppress unused variable warning
        count_empty_iter++;
    }
    assert(count_empty_iter == 0);
    std::cout << "Iteration over empty map: PASSED" << std::endl;
}

void test_clear_and_capacity() {
    print_test_header("Clear and Capacity");
    PackedSlotMap<TestObject> psm;
    psm.insert(TestObject(1, "one"));
    psm.insert(TestObject(2, "two"));
    assert(psm.size() == 2);

    psm.clear();
    assert(psm.size() == 0);
    assert(psm.empty());
    // Check if keys are invalidated (they should be, as clear rebuilds slots)
    PackedSlotMap<TestObject>::Key old_key = {0,0}; // Assuming a key that might have been valid
    assert(!psm.contains(old_key));
    assert(psm.get(old_key) == nullptr);
    std::cout << "Clear method: PASSED" << std::endl;

    // Insert after clear
    PackedSlotMap<TestObject>::Key key_after_clear = psm.insert(TestObject(3, "three"));
    assert(psm.size() == 1);
    assert(psm.contains(key_after_clear));
    assert(psm.get(key_after_clear) != nullptr && psm.get(key_after_clear)->id == 3);
    std::cout << "Insert after clear: PASSED" << std::endl;

    // Capacity and Reserve (basic check, actual capacity logic is std::vector's)
    PackedSlotMap<int> psm_int;
    psm_int.reserve(100);
    assert(psm_int.capacity() >= 100); // Data capacity
    // slot_capacity might be different or not directly reservable via this main reserve.
    // For this test, data capacity is the primary concern.
    std::cout << "Reserve and capacity: PASSED (basic check)" << std::endl;
}

void test_edge_cases() {
    print_test_header("Edge Cases");
    PackedSlotMap<TestObject> psm;

    // Insert and erase many elements to trigger potential free list complexities
    const int num_ops = 1000;
    std::vector<PackedSlotMap<TestObject>::Key> many_keys;
    for (int i = 0; i < num_ops; ++i) {
        many_keys.push_back(psm.insert(TestObject(i, "stress" + std::to_string(i))));
    }
    assert(psm.size() == num_ops);

    // Erase about half of them, from various positions
    for (int i = 0; i < num_ops; i += 2) {
        psm.erase(many_keys[i]);
    }
    assert(psm.size() == num_ops / 2);

    // Check remaining elements
    for (int i = 1; i < num_ops; i += 2) {
        assert(psm.contains(many_keys[i]));
        const TestObject* obj = psm.get(many_keys[i]);
        assert(obj != nullptr && obj->id == i);
    }
    std::cout << "Stress insert/erase: PASSED" << std::endl;

    // Clear and reuse
    psm.clear();
    assert(psm.empty());
    PackedSlotMap<TestObject>::Key k = psm.insert(TestObject(1, "final"));
    assert(psm.size() == 1);
    assert(psm.contains(k) && psm.get(k)->id == 1);
    std::cout << "Clear and reuse: PASSED" << std::endl;
}


int main() {
    std::cout << "===== Running PackedSlotMap Tests =====" << std::endl;

    test_construction_and_basic_properties();
    test_insertion_and_retrieval();
    test_erasure();
    test_key_stability_and_generation();
    test_iteration();
    test_clear_and_capacity();
    test_edge_cases();

    std::cout << "\n===== All PackedSlotMap Tests PASSED =====" << std::endl;

    return 0;
}

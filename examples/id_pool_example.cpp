#include "id_pool.h"
#include <iostream>
#include <vector>
#include <iomanip> // For std::setw

void print_id_status(const IdPool& pool, Id id, const std::string& name) {
    std::cout << std::setw(15) << std::left << name << ": "
              << "Index = " << id.index
              << ", Gen = " << id.generation
              << ", Valid = " << (pool.is_valid(id) ? "true" : "false")
              << std::endl;
}

int main() {
    IdPool pool;

    std::cout << "Initial pool size: " << pool.size() << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    // Allocate some IDs
    std::cout << "Allocating initial IDs:" << std::endl;
    Id id1 = pool.allocate();
    print_id_status(pool, id1, "id1");

    Id id2 = pool.allocate();
    print_id_status(pool, id2, "id2");

    Id id3 = pool.allocate();
    print_id_status(pool, id3, "id3");

    std::cout << "Pool size after 3 allocations: " << pool.size() << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    // Release an ID
    std::cout << "Releasing id2:" << std::endl;
    pool.release(id2);
    print_id_status(pool, id1, "id1 (after id2 release)");
    print_id_status(pool, id2, "id2 (after release)"); // Expected: Valid = false
    print_id_status(pool, id3, "id3 (after id2 release)");
    std::cout << "Pool size after releasing id2: " << pool.size() << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    // Allocate a new ID - should reuse id2's slot
    std::cout << "Allocating id4 (expecting reuse of id2's slot):" << std::endl;
    Id id4 = pool.allocate();
    print_id_status(pool, id4, "id4");
    std::cout << "Pool size after allocating id4: " << pool.size() << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    // Demonstrate stale ID detection
    std::cout << "Checking status of original id2 (should be stale):" << std::endl;
    print_id_status(pool, id2, "original id2"); // Expected: Valid = false, Gen should mismatch id4's gen

    std::cout << "------------------------------------------" << std::endl;
    std::cout << "Current valid IDs in pool:" << std::endl;
    print_id_status(pool, id1, "id1");
    print_id_status(pool, id4, "id4"); // id4 took id2's slot with new generation
    print_id_status(pool, id3, "id3");
    std::cout << "------------------------------------------" << std::endl;

    // Allocate more IDs to show new indices being used
    std::cout << "Allocating id5 and id6:" << std::endl;
    Id id5 = pool.allocate();
    print_id_status(pool, id5, "id5");
    Id id6 = pool.allocate();
    print_id_status(pool, id6, "id6");
    std::cout << "Pool size: " << pool.size() << std::endl;
    std::cout << "------------------------------------------" << std::endl;


    // Release all IDs
    std::cout << "Releasing all current valid IDs (id1, id3, id4, id5, id6):" << std::endl;
    pool.release(id1);
    pool.release(id3);
    pool.release(id4);
    pool.release(id5);
    pool.release(id6);
    std::cout << "Pool size after releasing all: " << pool.size() << std::endl;
    print_id_status(pool, id1, "id1 (after mass release)");
    print_id_status(pool, id3, "id3 (after mass release)");
    print_id_status(pool, id4, "id4 (after mass release)");
    print_id_status(pool, id5, "id5 (after mass release)");
    print_id_status(pool, id6, "id6 (after mass release)");
    std::cout << "------------------------------------------" << std::endl;

    // Allocate again to see reuse with incremented generations
    std::cout << "Allocating new IDs to see reuse:" << std::endl;
    Id id7 = pool.allocate(); // Should reuse one of the released slots
    print_id_status(pool, id7, "id7");
    Id id8 = pool.allocate(); // Should reuse another slot
    print_id_status(pool, id8, "id8");
    std::cout << "Pool size: " << pool.size() << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    std::cout << "Attempting to release a stale ID (original id2) again:" << std::endl;
    size_t size_before_stale_release = pool.size();
    pool.release(id2); // id2 is stale (index 0, gen 0, but current gen for index 0 is higher)
    print_id_status(pool, id2, "original id2");
    std::cout << "Pool size after attempting stale release: " << pool.size()
              << (pool.size() == size_before_stale_release ? " (unchanged, correct)" : " (changed, INCORRECT)")
              << std::endl;

    return 0;
}

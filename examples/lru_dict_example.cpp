#include "LRUDict.h" // Assuming LRUDict.h is in the include path
#include <iostream>
#include <string>
#include <vector>

// Helper function to print the LRUDict content (MRU to LRU)
template<typename K, typename V>
void print_lru_dict(const cpp_collections::LRUDict<K, V>& lru_dict, const std::string& label) {
    std::cout << "--- " << label << " (Size: " << lru_dict.size()
              << ", Capacity: " << lru_dict.capacity() << ") ---\n";
    std::cout << "MRU -> LRU: ";
    for (const auto& pair : lru_dict) {
        std::cout << "{" << pair.first << ": " << pair.second << "} ";
    }
    std::cout << "\n-------------------------------------------\n" << std::endl;
}

int main() {
    std::cout << "=== LRUDict Example ===\n" << std::endl;

    // Create an LRUDict with capacity 3
    cpp_collections::LRUDict<int, std::string> cache(3);
    print_lru_dict(cache, "Initial Cache (Capacity 3)");

    // Insert some items
    cache.insert({1, "apple"});
    print_lru_dict(cache, "After inserting {1: apple}");

    cache.insert_or_assign(2, "banana");
    print_lru_dict(cache, "After inserting {2: banana}");

    cache.try_emplace(3, "cherry");
    print_lru_dict(cache, "After inserting {3: cherry}");

    // Cache is now full: {3:cherry} {2:banana} {1:apple} (MRU to LRU)

    // Access item 2, making it MRU
    std::cout << "Accessing key 2: " << cache.at(2) << std::endl;
    print_lru_dict(cache, "After accessing key 2");
    // Expected: {2:banana} {3:cherry} {1:apple}

    // Insert a new item, which should evict the LRU item (1: "apple")
    cache.insert({4, "date"});
    print_lru_dict(cache, "After inserting {4: date} (evicts {1:apple})");
    // Expected: {4:date} {2:banana} {3:cherry}

    // Check if an evicted item is still there
    if (cache.contains(1)) {
        std::cout << "Error: Key 1 should have been evicted." << std::endl;
    } else {
        std::cout << "Key 1 successfully evicted." << std::endl;
    }
    std::cout << std::endl;

    // Use operator[] to insert/update
    cache[5] = "elderberry"; // New item, evicts {3:cherry}
    print_lru_dict(cache, "After cache[5] = elderberry (evicts {3:cherry})");
    // Expected: {5:elderberry} {4:date} {2:banana}

    cache[4] = "dragonfruit"; // Update existing item, makes it MRU
    print_lru_dict(cache, "After cache[4] = dragonfruit (updates, makes MRU)");
    // Expected: {4:dragonfruit} {5:elderberry} {2:banana}

    // Using get (updates LRU)
    auto val_opt = cache.get(5);
    if (val_opt) {
        std::cout << "Got value for key 5: " << val_opt->get() << std::endl;
    }
    print_lru_dict(cache, "After get(5)");
    // Expected: {5:elderberry} {4:dragonfruit} {2:banana}

    // Using peek (does not update LRU)
    std::cout << "Peeking key 2..." << std::endl;
    auto peek_val_opt = cache.peek(2);
     if (peek_val_opt) {
        std::cout << "Peeked value for key 2: " << peek_val_opt->get() << std::endl;
    }
    print_lru_dict(cache, "After peek(2) - order should be unchanged");
    // Expected: {5:elderberry} {4:dragonfruit} {2:banana} (same as before peek)

    // Erase an item
    cache.erase(4);
    print_lru_dict(cache, "After erasing key 4");
    // Expected: {5:elderberry} {2:banana}

    // Test clear
    cache.clear();
    print_lru_dict(cache, "After clear()");
    std::cout << "Is cache empty? " << std::boolalpha << cache.empty() << std::endl;

    std::cout << "\n=== LRUDict with Capacity 0 Example ===\n" << std::endl;
    cpp_collections::LRUDict<int, std::string> zero_cap_cache(0);
    print_lru_dict(zero_cap_cache, "Initial Cache (Capacity 0)");

    auto insert_result = zero_cap_cache.insert({1, "one"});
    std::cout << "Insert {1, one} into zero-cap cache success? " << std::boolalpha << insert_result.second << std::endl;
    print_lru_dict(zero_cap_cache, "After trying to insert into zero-cap cache");

    try {
        zero_cap_cache[1] = "one_again";
    } catch (const std::out_of_range& e) {
        std::cout << "Caught expected exception for operator[] on zero-cap: " << e.what() << std::endl;
    }
    print_lru_dict(zero_cap_cache, "After trying operator[] on zero-cap cache");


    std::cout << "\n=== LRUDict Example Finished ===\n" << std::endl;

    return 0;
}

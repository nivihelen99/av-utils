#include "counting_bloom_filter.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    // Create a Counting Bloom Filter for strings
    // Expected insertions: around 1000 items
    // Desired false positive rate: 1% (0.01)
    // Using default uint8_t for counters
    cpp_utils::CountingBloomFilter<std::string> cbf(1000, 0.01);

    std::cout << "Counting Bloom Filter Example" << std::endl;
    std::cout << "-----------------------------" << std::endl;
    std::cout << "Initialized for ~1000 items, 1% FP rate." << std::endl;
    std::cout << "Calculated number of counters: " << cbf.numCounters() << std::endl;
    std::cout << "Calculated number of hash functions: " << cbf.numHashFunctions() << std::endl;
    std::cout << "Approximate memory usage: " << cbf.approxMemoryUsage() << " bytes" << std::endl;
    std::cout << std::endl;

    // Items to add
    std::vector<std::string> items_to_add = {"apple", "banana", "orange", "grape", "mango"};
    std::cout << "Adding items: ";
    for (const auto& item : items_to_add) {
        cbf.add(item);
        std::cout << "\"" << item << "\" ";
    }
    std::cout << std::endl << std::endl;

    // Check for added items
    std::cout << "Checking for items known to be present:" << std::endl;
    for (const auto& item : items_to_add) {
        std::cout << "Contains \"" << item << "\"? " << (cbf.contains(item) ? "Yes (or FP)" : "No") << std::endl;
    }
    std::cout << std::endl;

    // Check for items not added
    std::cout << "Checking for items known to be absent (might be False Positives):" << std::endl;
    std::vector<std::string> items_not_added = {"strawberry", "blueberry", "raspberry"};
    for (const auto& item : items_not_added) {
        std::cout << "Contains \"" << item << "\"? " << (cbf.contains(item) ? "Yes (FP)" : "No") << std::endl;
    }
    std::cout << std::endl;

    // Demonstrate removal
    std::string item_to_remove = "orange";
    std::cout << "Removing item: \"" << item_to_remove << "\"" << std::endl;
    if (cbf.remove(item_to_remove)) {
        std::cout << "\"" << item_to_remove << "\" was potentially removed." << std::endl;
    } else {
        std::cout << "\"" << item_to_remove << "\" was definitely not present or already fully removed." << std::endl;
    }
    std::cout << "Contains \"" << item_to_remove << "\" after removal? "
              << (cbf.contains(item_to_remove) ? "Yes (FP or not fully removed)" : "No") << std::endl;
    std::cout << std::endl;

    // Demonstrate multiple adds and removes
    std::string multi_item = "banana"; // "banana" was already added once
    std::cout << "Adding \"" << multi_item << "\" two more times." << std::endl;
    cbf.add(multi_item);
    cbf.add(multi_item);

    std::cout << "Contains \"" << multi_item << "\"? " << (cbf.contains(multi_item) ? "Yes" : "No") << std::endl;
    std::cout << "Removing \"" << multi_item << "\" once." << std::endl;
    cbf.remove(multi_item);
    std::cout << "Contains \"" << multi_item << "\" after one remove? "
              << (cbf.contains(multi_item) ? "Yes (still has counts)" : "No") << std::endl;

    std::cout << "Removing \"" << multi_item << "\" again." << std::endl;
    cbf.remove(multi_item);
    std::cout << "Contains \"" << multi_item << "\" after second remove? "
              << (cbf.contains(multi_item) ? "Yes (still has counts)" : "No") << std::endl;

    std::cout << "Removing \"" << multi_item << "\" a third time (original add + 2 more)." << std::endl;
    cbf.remove(multi_item);
    std::cout << "Contains \"" << multi_item << "\" after third remove? "
              << (cbf.contains(multi_item) ? "Yes (FP or error)" : "No") << std::endl;
    std::cout << std::endl;


    // Example with integer items and larger counter type
    cpp_utils::CountingBloomFilter<int, uint16_t> cbf_int(500, 0.001);
    std::cout << "--- Integer CBF Example (uint16_t counters) ---" << std::endl;
    cbf_int.add(12345);
    cbf_int.add(67890);
    cbf_int.add(12345); // Add 12345 again

    std::cout << "Contains 12345? " << (cbf_int.contains(12345) ? "Yes" : "No") << std::endl;
    std::cout << "Contains 67890? " << (cbf_int.contains(67890) ? "Yes" : "No") << std::endl;
    std::cout << "Contains 99999? " << (cbf_int.contains(99999) ? "Yes (FP)" : "No") << std::endl;

    cbf_int.remove(12345);
    std::cout << "Contains 12345 after one remove? " << (cbf_int.contains(12345) ? "Yes" : "No") << std::endl;
    cbf_int.remove(12345);
    std::cout << "Contains 12345 after second remove? " << (cbf_int.contains(12345) ? "Yes (FP)" : "No") << std::endl;


    return 0;
}

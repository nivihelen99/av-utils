#include "bloom_filter.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    // Example 1: Basic usage with integers
    std::cout << "--- Example 1: Integers ---" << std::endl;
    // Expecting to store around 1000 integers with a false positive rate of 1% (0.01)
    BloomFilter<int> bf_int(1000, 0.01);

    std::cout << "Bloom filter initialized for " << bf_int.expected_items_capacity() << " items." << std::endl;
    std::cout << "Calculated bit array size (m): " << bf_int.bit_array_size() << std::endl;
    std::cout << "Calculated number of hash functions (k): " << bf_int.number_of_hash_functions() << std::endl;

    // Add some numbers
    bf_int.add(10);
    bf_int.add(20);
    bf_int.add(30);
    std::cout << "Added 10, 20, 30. Approximate item count: " << bf_int.approximate_item_count() << std::endl;


    // Check for items
    int item_to_check1 = 20; // Was added
    int item_to_check2 = 40; // Was not added

    std::cout << "Checking for " << item_to_check1 << ": "
              << (bf_int.might_contain(item_to_check1) ? "Might be present" : "Definitely not present")
              << std::endl;

    std::cout << "Checking for " << item_to_check2 << ": "
              << (bf_int.might_contain(item_to_check2) ? "Might be present (False Positive?)" : "Definitely not present")
              << std::endl;

    // Example 2: Usage with strings and illustrating false positives
    std::cout << "\n--- Example 2: Strings and False Positives ---" << std::endl;
    // Smaller filter to make false positives more likely for demonstration
    BloomFilter<std::string> bf_str(50, 0.05); // 50 items, 5% FP rate

    std::cout << "Bloom filter initialized for " << bf_str.expected_items_capacity() << " strings." << std::endl;
    std::cout << "Calculated bit array size (m): " << bf_str.bit_array_size() << std::endl;
    std::cout << "Calculated number of hash functions (k): " << bf_str.number_of_hash_functions() << std::endl;

    std::vector<std::string> words_to_add = {"apple", "banana", "cherry", "date", "elderberry"};
    for (const auto& word : words_to_add) {
        bf_str.add(word);
    }
    std::cout << "Added " << bf_str.approximate_item_count() << " words." << std::endl;

    std::vector<std::string> words_to_check = {
        "apple",    // Added
        "banana",   // Added
        "grape",    // Not added
        "kiwi",     // Not added
        "lemon"     // Not added
    };

    std::cout << "\nChecking presence:" << std::endl;
    for (const auto& word : words_to_check) {
        bool present = bf_str.might_contain(word);
        bool actually_added = false;
        for(const auto& added_word : words_to_add) {
            if (word == added_word) {
                actually_added = true;
                break;
            }
        }

        std::cout << "Word: \"" << word << "\"" << std::endl;
        std::cout << "  Bloom Filter says: " << (present ? "Might be present" : "Definitely not present") << std::endl;
        if (present && !actually_added) {
            std::cout << "  This is a FALSE POSITIVE!" << std::endl;
        } else if (!present && actually_added) {
            // This should ideally not happen for a Bloom Filter (no false negatives)
            std::cout << "  This is a FALSE NEGATIVE! (Should not happen for Bloom Filters)" << std::endl;
        } else if (present && actually_added) {
             std::cout << "  Correctly identified as (possibly) present." << std::endl;
        } else { // !present && !actually_added
             std::cout << "  Correctly identified as not present." << std::endl;
        }
    }

    std::cout << "\n--- Notes on Bloom Filter results ---" << std::endl;
    std::cout << "* If 'might_contain' returns false: The item is DEFINITELY NOT in the set." << std::endl;
    std::cout << "* If 'might_contain' returns true: The item MIGHT BE in the set, or it could be a false positive." << std::endl;
    std::cout << "  The probability of a false positive is determined at construction." << std::endl;
    std::cout << "* Bloom filters do not store the items themselves, only their probabilistic presence." << std::endl;
    std::cout << "* Items cannot be removed from a standard Bloom filter." << std::endl;


    // Example 3: Filter with 0 expected items
    std::cout << "\n--- Example 3: Zero Expected Items ---" << std::endl;
    BloomFilter<int> bf_zero(0, 0.01);
    std::cout << "Bloom filter initialized for 0 items." << std::endl;
    std::cout << "Bit array size: " << bf_zero.bit_array_size() << ", Hash functions: " << bf_zero.number_of_hash_functions() << std::endl;
    std::cout << "Checking for 5 (before add): " << (bf_zero.might_contain(5) ? "Might be present" : "Definitely not present") << std::endl;
    bf_zero.add(5);
    std::cout << "Added 5. Approximate item count: " << bf_zero.approximate_item_count() << std::endl;
    std::cout << "Checking for 5 (after add): " << (bf_zero.might_contain(5) ? "Might be present" : "Definitely not present") << std::endl;
    std::cout << "Checking for 10 (after add): " << (bf_zero.might_contain(10) ? "Might be present" : "Definitely not present") << std::endl;


    return 0;
}

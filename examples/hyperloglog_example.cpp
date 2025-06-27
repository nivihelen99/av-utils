#include "hyperloglog.h" // Assuming hyperloglog.h is in include path (e.g. via -I../include)
#include <iostream>
#include <string>
#include <vector>
#include <random> // For generating random data

// A custom struct for demonstration
struct UserActivity {
    std::string userId;
    std::string pageVisited;

    // Custom hash for UserActivity
    struct Hash {
        size_t operator()(const UserActivity& ua) const {
            // A simple combination of std::hash for members
            size_t h1 = std::hash<std::string>{}(ua.userId);
            size_t h2 = std::hash<std::string>{}(ua.pageVisited);
            // Combine hashes (a common way, e.g., from Boost's hash_combine)
            return h1 ^ (h2 << 1);
        }
    };
};


int main() {
    std::cout << "HyperLogLog Example\n" << std::endl;

    // --- Basic Usage with std::string ---
    std::cout << "--- Basic String Example (p=10) ---" << std::endl;
    cpp_collections::HyperLogLog<std::string> hll_strings(10); // Precision p=10, m=1024 registers

    hll_strings.add("apple");
    hll_strings.add("banana");
    hll_strings.add("orange");
    hll_strings.add("apple"); // Duplicate
    hll_strings.add("grape");
    hll_strings.add("banana"); // Duplicate

    std::cout << "Added: apple, banana, orange, apple, grape, banana" << std::endl;
    std::cout << "Estimated unique strings: " << hll_strings.estimate() << " (Expected: 4)" << std::endl;
    std::cout << std::endl;

    // --- Usage with integers ---
    std::cout << "--- Integer Example (p=8, 32-bit hash) ---" << std::endl;
    cpp_collections::HyperLogLog<int, std::hash<int>, 32> hll_ints(8); // p=8, m=256

    const int num_int_to_add = 1000;
    const int int_range = 500; // Will result in duplicates
    std::mt19937 rng(12345); // Fixed seed for reproducible example
    std::uniform_int_distribution<int> distrib(1, int_range);

    std::cout << "Adding " << num_int_to_add << " integers (some duplicates, from range 1-" << int_range << ")..." << std::endl;
    for (int i = 0; i < num_int_to_add; ++i) {
        hll_ints.add(distrib(rng));
    }
    std::cout << "Estimated unique integers: " << hll_ints.estimate() << " (Expected: around " << int_range << ")" << std::endl;
    std::cout << std::endl;

    // --- Usage with custom struct and custom hash ---
    std::cout << "--- Custom Struct Example (UserActivity, p=12, 64-bit hash) ---" << std::endl;
    cpp_collections::HyperLogLog<UserActivity, UserActivity::Hash, 64> hll_custom(12); // p=12, m=4096

    hll_custom.add({"user1", "/home"});
    hll_custom.add({"user2", "/products"});
    hll_custom.add({"user1", "/home"});    // Duplicate
    hll_custom.add({"user3", "/home"});
    hll_custom.add({"user1", "/checkout"});
    hll_custom.add({"user2", "/products"}); // Duplicate

    std::cout << "Added UserActivity data..." << std::endl;
    std::cout << "Estimated unique UserActivities: " << hll_custom.estimate() << " (Expected: 4)" << std::endl;
    std::cout << std::endl;

    // --- Merging two HyperLogLog instances ---
    std::cout << "--- Merging Example (p=6) ---" << std::endl;
    cpp_collections::HyperLogLog<std::string> hll_part1(6); // p=6, m=64
    hll_part1.add("event_A_stream1");
    hll_part1.add("event_B_stream1");
    hll_part1.add("common_event");
    std::cout << "HLL Part 1 estimated unique: " << hll_part1.estimate() << " (Expected: 3)" << std::endl;

    cpp_collections::HyperLogLog<std::string> hll_part2(6);
    hll_part2.add("event_X_stream2");
    hll_part2.add("event_Y_stream2");
    hll_part2.add("common_event");
    std::cout << "HLL Part 2 estimated unique: " << hll_part2.estimate() << " (Expected: 3)" << std::endl;

    hll_part1.merge(hll_part2);
    std::cout << "After merging Part 2 into Part 1:" << std::endl;
    std::cout << "Merged HLL estimated unique: " << hll_part1.estimate()
              << " (Expected: 5: A, B, common, X, Y)" << std::endl;
    std::cout << std::endl;

    // --- Clearing an HLL instance ---
    std::cout << "--- Clearing Example ---" << std::endl;
    cpp_collections::HyperLogLog<int> hll_to_clear(8);
    hll_to_clear.add(10);
    hll_to_clear.add(20);
    std::cout << "Before clear, estimate: " << hll_to_clear.estimate() << std::endl;
    hll_to_clear.clear();
    std::cout << "After clear, estimate: " << hll_to_clear.estimate() << " (Expected: 0)" << std::endl;
    hll_to_clear.add(30);
    std::cout << "After adding one item post-clear, estimate: " << hll_to_clear.estimate() << " (Expected: 1)" << std::endl;


    std::cout << "\nExample finished." << std::endl;

    return 0;
}

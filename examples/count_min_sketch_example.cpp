#include "count_min_sketch.h" // Assuming it's in the include path via CMake
#include <iostream>
#include <string>
#include <vector>

int main() {
    // Create a Count-Min Sketch
    // Epsilon (error factor): 0.01 (estimate is within true_count +/- 0.01 * total_sum_counts)
    // Delta (error probability): 0.01 (99% confidence in the error bound)
    double epsilon = 0.01;
    double delta = 0.01;
    CountMinSketch<std::string> sketch(epsilon, delta);

    std::cout << "Count-Min Sketch created with:" << std::endl;
    std::cout << "  Epsilon (error factor): " << sketch.get_error_factor_epsilon() << std::endl;
    std::cout << "  Delta (error probability): " << sketch.get_error_probability_delta() << std::endl;
    std::cout << "  Width (counters per hash function): " << sketch.get_width() << std::endl;
    std::cout << "  Depth (number of hash functions): " << sketch.get_depth() << std::endl;
    std::cout << std::endl;

    // Items to add (simulating a stream of words)
    std::vector<std::string> stream = {
        "apple", "banana", "orange", "apple", "grape",
        "banana", "apple", "banana", "mango", "apple",
        "orange", "grape", "grape", "apple", "banana"
    };

    std::cout << "Adding items to the sketch:" << std::endl;
    for (const std::string& item : stream) {
        sketch.add(item); // Add with count 1
        // std::cout << "Added: " << item << std::endl;
    }
    std::cout << "Finished adding items." << std::endl << std::endl;

    // Items to estimate
    std::vector<std::string> items_to_estimate = {
        "apple", "banana", "orange", "grape", "mango", "pear" // "pear" was not added
    };

    std::cout << "Estimating item frequencies:" << std::endl;
    for (const std::string& item : items_to_estimate) {
        unsigned int estimated_count = sketch.estimate(item);
        std::cout << "  Item: \"" << item << "\", Estimated Freq: " << estimated_count << std::endl;
    }
    std::cout << std::endl;

    std::cout << "Explanation of results:" << std::endl;
    std::cout << "- Estimates are always >= true frequency." << std::endl;
    std::cout << "- Estimates can be higher than true frequency due to hash collisions." << std::endl;
    std::cout << "- The parameters epsilon and delta control the accuracy:" << std::endl;
    std::cout << "  With high probability (1 - delta), the error in estimation "
                 "(estimate - true_frequency) is at most epsilon * (total sum of all counts added)." << std::endl;
    std::cout << "- For items not added (like \"pear\"), the estimate might be > 0 due to collisions, "
                 "but it's typically low if the sketch is not overly full." << std::endl;

    // Example with integer keys
    CountMinSketch<int> int_sketch(0.05, 0.05); // Different params for variety
    std::cout << "\n--- Integer Key Example ---" << std::endl;
    int_sketch.add(101, 50);
    int_sketch.add(202, 75);
    int_sketch.add(101, 30); // Add more to 101, total 80

    std::cout << "Estimate for 101 (true 80): " << int_sketch.estimate(101) << std::endl;
    std::cout << "Estimate for 202 (true 75): " << int_sketch.estimate(202) << std::endl;
    std::cout << "Estimate for 303 (true 0): " << int_sketch.estimate(303) << std::endl;

    return 0;
}

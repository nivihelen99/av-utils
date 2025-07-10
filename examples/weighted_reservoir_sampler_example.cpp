#include "WeightedReservoirSampler.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip> // For std::fixed and std::setprecision

// Helper to print a sample
template<typename T>
void print_sample(const std::string& title, const std::vector<T>& sample, size_t k) {
    std::cout << title << " (k=" << k << ", actual size=" << sample.size() << "):" << std::endl;
    if (sample.empty()) {
        std::cout << "  [Empty]" << std::endl;
    } else {
        std::cout << "  [";
        for (size_t i = 0; i < sample.size(); ++i) {
            std::cout << sample[i] << (i == sample.size() - 1 ? "" : ", ");
        }
        std::cout << "]" << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    // Basic example with integers
    std::cout << "--- Basic Integer Example ---" << std::endl;
    cpp_utils::WeightedReservoirSampler<int> sampler1(3);
    sampler1.add(1, 10.0); // Item, Weight
    sampler1.add(2, 1.0);
    sampler1.add(3, 1.0);
    sampler1.add(4, 100.0); // High weight, likely to be included and kick out a low weight item
    sampler1.add(5, 1.0);
    sampler1.add(6, 0.5); // Very low weight
    sampler1.add(7, 90.0); // High weight

    print_sample("Sample 1 (integers)", sampler1.get_sample(), sampler1.capacity());

    // Example with strings
    std::cout << "--- String Example ---" << std::endl;
    cpp_utils::WeightedReservoirSampler<std::string> sampler2(2, 12345); // Fixed seed for reproducible example output
    sampler2.add("apple", 50.0);
    sampler2.add("banana", 5.0);
    sampler2.add("cherry", 1.0); // Low weight, likely replaced
    sampler2.add("date", 60.0);   // High weight
    sampler2.add("elderberry", 0.1); // Very low

    print_sample("Sample 2 (strings)", sampler2.get_sample(), sampler2.capacity());

    // Example with k=0
    std::cout << "--- k=0 Example ---" << std::endl;
    cpp_utils::WeightedReservoirSampler<int> sampler_k0(0);
    sampler_k0.add(100, 1000.0);
    sampler_k0.add(200, 1000.0);
    print_sample("Sample k=0", sampler_k0.get_sample(), sampler_k0.capacity());

    // Example with non-positive weights
    std::cout << "--- Non-positive Weights Example ---" << std::endl;
    cpp_utils::WeightedReservoirSampler<int> sampler_npw(2);
    sampler_npw.add(1, 10.0);
    sampler_npw.add(2, 0.0);    // Should be ignored
    sampler_npw.add(3, -5.0);   // Should be ignored
    sampler_npw.add(4, 20.0);
    sampler_npw.add(5, 1.0);    // Might be kicked out by 1 or 4
    print_sample("Sample with non-positive weights", sampler_npw.get_sample(), sampler_npw.capacity());
    // Expected: 1 and 4, or 1 and 5, or 4 and 5. 2 and 3 should not be present.

    // Demonstration of statistical nature (very basic)
    // For a more rigorous test, one would run many times and count frequencies.
    std::cout << "--- Statistical Tendency Demonstration (Basic) ---" << std::endl;
    const int num_trials = 10000;
    const int k_stat = 1;
    int item_A_count = 0;
    int item_B_count = 0;
    int item_C_count = 0;

    // Item A has weight 90, B has 9, C has 1. Total weight 100.
    // In a k=1 sample, A should appear ~90% of time, B ~9%, C ~1%
    // This is a simplification; reservoir sampling probabilities are more complex,
    // but the tendency for higher weights to be selected should be clear.

    for (int i = 0; i < num_trials; ++i) {
        cpp_utils::WeightedReservoirSampler<char> sampler_stat(k_stat, i); // Vary seed
        sampler_stat.add('A', 90.0);
        sampler_stat.add('B', 9.0);
        sampler_stat.add('C', 1.0);
        // Add more low-weight items to ensure the reservoir logic is stressed
        sampler_stat.add('D', 0.1);
        sampler_stat.add('E', 0.1);


        std::vector<char> sample = sampler_stat.get_sample();
        if (!sample.empty()) {
            if (sample[0] == 'A') item_A_count++;
            else if (sample[0] == 'B') item_B_count++;
            else if (sample[0] == 'C') item_C_count++;
        }
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "After " << num_trials << " trials (k=" << k_stat << "):" << std::endl;
    std::cout << "  Item 'A' (weight 90.0) selected: " << item_A_count << " times (" << (double)item_A_count / num_trials * 100.0 << "%)" << std::endl;
    std::cout << "  Item 'B' (weight 9.0) selected: " << item_B_count << " times (" << (double)item_B_count / num_trials * 100.0 << "%)" << std::endl;
    std::cout << "  Item 'C' (weight 1.0) selected: " << item_C_count << " times (" << (double)item_C_count / num_trials * 100.0 << "%)" << std::endl;
    std::cout << "Note: These percentages demonstrate tendency, not exact probabilities for this specific algorithm without full analysis." << std::endl;


    std::cout << "\n--- Example with many items and small k ---" << std::endl;
    cpp_utils::WeightedReservoirSampler<int> sampler_many(5);
    for (int i = 0; i < 100; ++i) {
        // Give items 0-49 lower weights, 50-99 higher weights
        double weight = (i < 50) ? (rand() % 10 + 1.0) : (rand() % 50 + 50.0);
        sampler_many.add(i, weight);
    }
    std::vector<int> final_sample_many = sampler_many.get_sample();
    print_sample("Sample from 100 items", final_sample_many, sampler_many.capacity());
    std::cout << "Observe if the sample tends to contain items with higher original indices (50-99)." << std::endl;


    return 0;
}

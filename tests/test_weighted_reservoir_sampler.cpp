#define CATCH_CONFIG_MAIN // Provides main() - only do this in one cpp file
#include "catch.hpp"

#include "WeightedReservoirSampler.h"
#include <string>
#include <vector>
#include <map> // For frequency counting in statistical test
#include <iostream> // For debug prints if needed

TEST_CASE("WeightedReservoirSampler Basic Initialization", "[WeightedReservoirSampler]") {
    cpp_utils::WeightedReservoirSampler<int> sampler_empty(0);
    REQUIRE(sampler_empty.capacity() == 0);
    REQUIRE(sampler_empty.sample_size() == 0);
    REQUIRE(sampler_empty.get_sample().empty());

    cpp_utils::WeightedReservoirSampler<std::string> sampler_k5(5);
    REQUIRE(sampler_k5.capacity() == 5);
    REQUIRE(sampler_k5.sample_size() == 0);
    REQUIRE(sampler_k5.get_sample().empty());
    REQUIRE(sampler_k5.empty());
}

TEST_CASE("WeightedReservoirSampler Adding Items Below Capacity", "[WeightedReservoirSampler]") {
    cpp_utils::WeightedReservoirSampler<int> sampler(3);
    sampler.add(10, 1.0);
    REQUIRE(sampler.sample_size() == 1);
    REQUIRE(sampler.get_sample().size() == 1);
    REQUIRE(sampler.get_sample()[0] == 10);

    sampler.add(20, 2.0);
    REQUIRE(sampler.sample_size() == 2);
    REQUIRE(sampler.get_sample().size() == 2);
    // Order in multiset is by key, not insertion. get_sample() extracts from multiset.
    // We cannot guarantee order of 10, 20 in the returned vector without knowing keys.

    sampler.add(30, 0.5);
    REQUIRE(sampler.sample_size() == 3);
    REQUIRE(sampler.get_sample().size() == 3);
    REQUIRE_FALSE(sampler.empty());

    // Verify all three items are present (order might vary due to keys)
    std::vector<int> sample = sampler.get_sample();
    std::sort(sample.begin(), sample.end());
    REQUIRE(sample[0] == 10);
    REQUIRE(sample[1] == 20);
    REQUIRE(sample[2] == 30);
}

TEST_CASE("WeightedReservoirSampler Adding Items At and Beyond Capacity", "[WeightedReservoirSampler]") {
    // Use a fixed seed for reproducibility of keys to make this test deterministic.
    // Note: This makes the test dependent on the exact behavior of std::mt19937 and key generation.
    // For a more robust test of logic, one might need to mock key generation or test statistically.
    unsigned int fixed_seed = 123;
    cpp_utils::WeightedReservoirSampler<int, double> sampler(2, fixed_seed);

    // Item, Weight. Keys are u^(1/w) or exp(log(u)/w). Higher key is preferred.
    // Smaller weight -> larger 1/w -> potentially larger key if u is not too small.
    // Larger weight -> smaller 1/w -> potentially smaller key unless u is very close to 1.
    // Let's trace exp(log(u)/w):
    //  - log(u) is negative.
    //  - For larger w, log(u)/w is a smaller negative number (closer to 0). exp(closer_to_0) is closer to 1. (Higher key)
    //  - For smaller w, log(u)/w is a larger negative number. exp(larger_neg) is closer to 0. (Lower key)
    // So, higher weight -> higher key. This means items with higher weight are preferred.

    sampler.add(1, 1.0); // item, weight
    sampler.add(2, 10.0); // item, weight. Higher weight, should have higher key.
    REQUIRE(sampler.sample_size() == 2);

    std::vector<int> s1 = sampler.get_sample();
    std::sort(s1.begin(), s1.end());
    REQUIRE(s1[0] == 1);
    REQUIRE(s1[1] == 2);

    // Add item 3 with weight 0.1 (very low, should not replace 1 or 2)
    sampler.add(3, 0.1);
    REQUIRE(sampler.sample_size() == 2);
    std::vector<int> s2 = sampler.get_sample();
    std::sort(s2.begin(), s2.end());
    REQUIRE(s2[0] == 1); // Expect 1 and 2 to remain
    REQUIRE(s2[1] == 2);

    // Add item 4 with weight 100.0 (very high, should replace item with lower weight/key)
    // Item 1 (weight 1.0) should be replaced.
    sampler.add(4, 100.0);
    REQUIRE(sampler.sample_size() == 2);
    std::vector<int> s3 = sampler.get_sample();
    std::sort(s3.begin(), s3.end());
    // Expected: 2 (weight 10.0) and 4 (weight 100.0)
    bool found2 = false, found4 = false;
    for(int val : s3) {
        if (val == 2) found2 = true;
        if (val == 4) found4 = true;
    }
    REQUIRE(found2);
    REQUIRE(found4);

    // Add item 5 with weight 20.0.
    // This should replace item 2 (weight 10.0) but not 4 (weight 100.0)
    sampler.add(5, 20.0);
    REQUIRE(sampler.sample_size() == 2);
    std::vector<int> s4 = sampler.get_sample();
    std::sort(s4.begin(), s4.end());
    bool found5 = false;
    found4 = false; // reset
     for(int val : s4) {
        if (val == 5) found5 = true;
        if (val == 4) found4 = true;
    }
    REQUIRE(found5); // Expect 5 (weight 20.0)
    REQUIRE(found4); // Expect 4 (weight 100.0)
}

TEST_CASE("WeightedReservoirSampler Handling of Non-Positive Weights", "[WeightedReservoirSampler]") {
    cpp_utils::WeightedReservoirSampler<int> sampler(2);
    sampler.add(1, 10.0);
    REQUIRE(sampler.sample_size() == 1);

    sampler.add(2, 0.0); // Zero weight, should be ignored
    REQUIRE(sampler.sample_size() == 1);
    REQUIRE(sampler.get_sample()[0] == 1);


    sampler.add(3, -5.5); // Negative weight, should be ignored
    REQUIRE(sampler.sample_size() == 1);
    REQUIRE(sampler.get_sample()[0] == 1);

    sampler.add(4, 20.0);
    REQUIRE(sampler.sample_size() == 2);

    std::vector<int> sample = sampler.get_sample();
    bool found1 = false, found4 = false, found2 = false, found3 = false;
    for(int val : sample) {
        if (val == 1) found1 = true;
        if (val == 4) found4 = true;
        if (val == 2) found2 = true; // Should not be found
        if (val == 3) found3 = true; // Should not be found
    }
    REQUIRE(found1);
    REQUIRE(found4);
    REQUIRE_FALSE(found2);
    REQUIRE_FALSE(found3);
}

TEST_CASE("WeightedReservoirSampler Clear and Empty Operations", "[WeightedReservoirSampler]") {
    cpp_utils::WeightedReservoirSampler<std::string> sampler(3);
    sampler.add("apple", 1.0);
    sampler.add("banana", 1.0);
    REQUIRE_FALSE(sampler.empty());
    REQUIRE(sampler.sample_size() == 2);

    sampler.clear();
    REQUIRE(sampler.empty());
    REQUIRE(sampler.sample_size() == 0);
    REQUIRE(sampler.get_sample().empty());

    // Add again after clearing
    sampler.add("cherry", 2.0);
    REQUIRE_FALSE(sampler.empty());
    REQUIRE(sampler.sample_size() == 1);
    REQUIRE(sampler.get_sample()[0] == "cherry");
}

TEST_CASE("WeightedReservoirSampler Move Semantics for Items", "[WeightedReservoirSampler]") {
    cpp_utils::WeightedReservoirSampler<std::string> sampler(1);
    std::string s1 = "movable_string_1";
    std::string s2 = "movable_string_2";

    // Add s1 via rvalue reference
    sampler.add(std::move(s1), 10.0);
    REQUIRE(s1.empty()); // Check if s1 was moved from
    REQUIRE(sampler.sample_size() == 1);
    REQUIRE(sampler.get_sample()[0] == "movable_string_1");

    // Try to replace with s2 (higher weight implies higher key)
    sampler.add(std::move(s2), 100.0);
    REQUIRE(s2.empty()); // Check if s2 was moved from
    REQUIRE(sampler.sample_size() == 1);
    REQUIRE(sampler.get_sample()[0] == "movable_string_2");
}


TEST_CASE("WeightedReservoirSampler Statistical Distribution Tendency", "[WeightedReservoirSampler]") {
    const int num_trials = 20000; // Increased trials for better statistical significance
    const size_t k = 1;
    std::map<char, int> counts;
    // Item A: high weight, Item B: medium, Item C: low
    // With A-ExpJ, higher weight means higher key on average, so more likely to be kept.
    double weightA = 90.0;
    double weightB = 9.0;
    double weightC = 1.0;

    // For a single slot (k=1) and items added one by one, the probability of item i being selected
    // is not simply w_i / sum(w_j) as in weighted random sampling from a fixed list.
    // However, we expect a strong bias towards higher weight items.

    for (int i = 0; i < num_trials; ++i) {
        // Use different seed per trial to ensure varied random key generation
        cpp_utils::WeightedReservoirSampler<char> sampler(k, i);
        sampler.add('A', weightA);
        sampler.add('B', weightB);
        sampler.add('C', weightC);
        // Add a few more items with very low weights to ensure reservoir processes them
        sampler.add('X', 0.01);
        sampler.add('Y', 0.001);

        std::vector<char> sample = sampler.get_sample();
        if (!sample.empty()) {
            REQUIRE(sample.size() == k);
            counts[sample[0]]++;
        }
    }

    // std::cout << "Statistical Test Counts (k=1):" << std::endl;
    // std::cout << "  A (w=" << weightA << "): " << counts['A'] << " (" << (double)counts['A']/num_trials*100 << "%)" << std::endl;
    // std::cout << "  B (w=" << weightB << "): " << counts['B'] << " (" << (double)counts['B']/num_trials*100 << "%)" << std::endl;
    // std::cout << "  C (w=" << weightC << "): " << counts['C'] << " (" << (double)counts['C']/num_trials*100 << "%)" << std::endl;
    // std::cout << "  X (w=0.01): " << counts['X'] << std::endl;
    // std::cout << "  Y (w=0.001): " << counts['Y'] << std::endl;


    // Check general tendency: A > B > C in counts.
    // And X, Y should be very low or zero.
    REQUIRE(counts['A'] > counts['B']);
    REQUIRE(counts['B'] > counts['C']);

    // Check that A is selected significantly more often
    // These are rough checks for tendency, not strict probabilities.
    // The exact probabilities for A-ExpJ are complex.
    // Item A should be dominant.
    REQUIRE(counts['A'] > num_trials * 0.70); // Expect A to be in a high percentage of samples.

    // Item C should be rare.
    REQUIRE(counts['C'] < num_trials * 0.15); // Expect C to be in a low percentage.

    // Items X and Y should be extremely rare or absent
    REQUIRE(counts['X'] < num_trials * 0.05);
    REQUIRE(counts['Y'] < num_trials * 0.05);


    // Test with k=2
    const size_t k2 = 2;
    std::map<char, int> counts_k2;
    for (int i = 0; i < num_trials; ++i) {
        cpp_utils::WeightedReservoirSampler<char> sampler_k2(k2, num_trials + i); // Vary seed
        sampler_k2.add('A', weightA); // 90
        sampler_k2.add('B', weightB); // 9
        sampler_k2.add('C', weightC); // 1
        sampler_k2.add('D', 30.0);    // medium-high
        sampler_k2.add('E', 0.5);     // low

        std::vector<char> sample = sampler_k2.get_sample();
        if (sample.size() == k2) {
            for (char c : sample) {
                counts_k2[c]++;
            }
        }
    }
    // std::cout << "Statistical Test Counts (k=2):" << std::endl;
    // std::cout << "  A (w=" << weightA << "): " << counts_k2['A'] << " (" << (double)counts_k2['A']/num_trials*100 << "%)" << std::endl;
    // std::cout << "  B (w=" << weightB << "): " << counts_k2['B'] << " (" << (double)counts_k2['B']/num_trials*100 << "%)" << std::endl;
    // std::cout << "  C (w=" << weightC << "): " << counts_k2['C'] << " (" << (double)counts_k2['C']/num_trials*100 << "%)" << std::endl;
    // std::cout << "  D (w=30.0): " << counts_k2['D'] << " (" << (double)counts_k2['D']/num_trials*100 << "%)" << std::endl;
    // std::cout << "  E (w=0.5): " << counts_k2['E'] << " (" << (double)counts_k2['E']/num_trials*100 << "%)" << std::endl;

    // Expect A and D to be the most frequent.
    REQUIRE(counts_k2['A'] > num_trials * 0.85); // A should be in almost all k=2 samples
    REQUIRE(counts_k2['D'] > num_trials * 0.50); // D should be frequent as well

    // B should be less frequent than D
    REQUIRE(counts_k2['D'] > counts_k2['B']);

    // C and E should be infrequent
    REQUIRE(counts_k2['C'] < num_trials * 0.20);
    REQUIRE(counts_k2['E'] < num_trials * 0.20);
}

TEST_CASE("WeightedReservoirSampler with many items", "[WeightedReservoirSampler]") {
    cpp_utils::WeightedReservoirSampler<int> sampler(10);
    for(int i = 0; i < 1000; ++i) {
        sampler.add(i, static_cast<double>(rand() % 100 + 1));
    }
    REQUIRE(sampler.sample_size() == 10);
    std::vector<int> sample = sampler.get_sample();
    REQUIRE(sample.size() == 10);
    // Further checks could involve verifying properties of the sampled items,
    // e.g. if their original weights (if stored/retrievable) are generally higher.
}

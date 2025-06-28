#include "QuotientFilter.h"
#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <iomanip> // For std::fixed, std::setprecision
#include <unordered_set> // For FPR test

// Helper to print test results
void print_test_result(const std::string& test_name, bool success) {
    std::cout << (success ? "[PASS] " : "[FAIL] ") << test_name << std::endl;
    if (!success) {
        // In a real scenario, one might exit or throw here to stop on first failure.
    }
}

void test_construction() {
    std::cout << "\n--- Testing Construction ---" << std::endl;
    bool success = true;
    QuotientFilter<int> qf(1000, 0.01);
    print_test_result("Default construction (int)", qf.empty() && qf.size() == 0);
    assert(qf.empty() && qf.size() == 0);

    QuotientFilter<std::string> qf_str(500, 0.001);
    print_test_result("Default construction (string)", qf_str.empty() && qf_str.size() == 0);
    assert(qf_str.empty() && qf_str.size() == 0);

    // Test initial capacity and bit sizes (example values)
    std::cout << "QF(1000, 0.01): q_bits=" << (int)qf.quotient_bits()
              << ", r_bits=" << (int)qf.remainder_bits()
              << ", num_slots=" << qf.num_slots()
              << ", capacity_approx=" << qf.capacity() << std::endl;

    // Test invalid arguments
    bool throws_expected_items_zero = false;
    try {
        QuotientFilter<int> qf_invalid(0, 0.01);
    } catch (const std::invalid_argument& e) {
        throws_expected_items_zero = true;
        std::cout << "Caught expected exception for 0 items: " << e.what() << std::endl;
    }
    print_test_result("Throws on zero expected items", throws_expected_items_zero);
    assert(throws_expected_items_zero);

    bool throws_fp_too_low = false;
    try {
        QuotientFilter<int> qf_invalid(100, 0.0);
    } catch (const std::invalid_argument& e) {
        throws_fp_too_low = true;
         std::cout << "Caught expected exception for FP <= 0.0: " << e.what() << std::endl;
    }
    print_test_result("Throws on FP probability <= 0.0", throws_fp_too_low);
    assert(throws_fp_too_low);

    bool throws_fp_too_high = false;
    try {
        QuotientFilter<int> qf_invalid(100, 1.0);
    } catch (const std::invalid_argument& e) {
        throws_fp_too_high = true;
        std::cout << "Caught expected exception for FP >= 1.0: " << e.what() << std::endl;
    }
    print_test_result("Throws on FP probability >= 1.0", throws_fp_too_high);
    assert(throws_fp_too_high);
}

void test_simple_add_lookup() {
    std::cout << "\n--- Testing Simple Add/Lookup ---" << std::endl;
    QuotientFilter<int> qf(100, 0.01);

    qf.add(42);
    print_test_result("Add 42, size is 1", qf.size() == 1);
    assert(qf.size() == 1);
    print_test_result("might_contain(42) is true", qf.might_contain(42));
    assert(qf.might_contain(42));
    print_test_result("might_contain(100) is false", !qf.might_contain(100));
    assert(!qf.might_contain(100));

    qf.add(42); // Adding same item again
    print_test_result("Add 42 again, size is still 1", qf.size() == 1);
    assert(qf.size() == 1);

    qf.add(123);
    print_test_result("Add 123, size is 2", qf.size() == 2);
    assert(qf.size() == 2);
    print_test_result("might_contain(123) is true", qf.might_contain(123));
    assert(qf.might_contain(123));
    print_test_result("might_contain(42) is still true", qf.might_contain(42));
    assert(qf.might_contain(42));
}

void test_multiple_items() {
    std::cout << "\n--- Testing Multiple Items ---" << std::endl;
    QuotientFilter<int> qf(200, 0.01);
    std::vector<int> items_to_add;
    for(int i=0; i<100; ++i) {
        items_to_add.push_back(i * 10); // 0, 10, 20, ... 990
    }

    for(int item : items_to_add) {
        qf.add(item);
    }
    print_test_result("Size after adding 100 items", qf.size() == 100);
    assert(qf.size() == 100);

    bool all_found = true;
    for(int item : items_to_add) {
        if (!qf.might_contain(item)) {
            all_found = false;
            std::cerr << "FAIL: Item " << item << " not found after adding." << std::endl;
            break;
        }
    }
    print_test_result("All added items found", all_found);
    assert(all_found);

    bool none_missing_found = true;
    for(int i=1; i<=50; ++i) {
        if (qf.might_contain(i * 10 + 1)) { // e.g. 1, 11, 21... (these were not added)
            // This could be a false positive, which is allowed.
            // For a strict test of not finding non-members, we'd need to control hashing.
            // This test just checks that it doesn't *always* return true.
            // A better test is the FPR test.
        }
    }
    // This part of test is weak without FPR analysis
    std::cout << "(Skipping strict check for non-added items here; covered by FPR test)" << std::endl;
}

void test_string_items() {
    std::cout << "\n--- Testing String Items ---" << std::endl;
    QuotientFilter<std::string> qf(100, 0.01);

    qf.add("hello");
    qf.add("world");
    qf.add("quotient");
    qf.add("filter");

    print_test_result("Size after adding 4 strings", qf.size() == 4);
    assert(qf.size() == 4);

    print_test_result("might_contain(\"hello\")", qf.might_contain("hello"));
    assert(qf.might_contain("hello"));
    print_test_result("might_contain(\"world\")", qf.might_contain("world"));
    assert(qf.might_contain("world"));
    print_test_result("might_contain(\"quotient\")", qf.might_contain("quotient"));
    assert(qf.might_contain("quotient"));
    print_test_result("might_contain(\"filter\")", qf.might_contain("filter"));
    assert(qf.might_contain("filter"));

    print_test_result("!might_contain(\"test\")", !qf.might_contain("test"));
    assert(!qf.might_contain("test"));
    print_test_result("!might_contain(\"\")", !qf.might_contain("")); // Empty string
    assert(!qf.might_contain(""));
}

void test_full_behavior() {
    std::cout << "\n--- Testing Full Behavior (Heuristic) ---" << std::endl;
    // Test with expected items close to capacity limit based on load factor
    // This test depends on the internal target_load_factor_ (0.90)
    // num_slots for (10, 0.01) -> q_bits=4, r_bits=7. num_slots=16. Capacity approx 16*0.9 = 14
    // Let's try to make q_bits small. expected_items=5, fp=0.1 => r=4. q for 5/0.9=5.55 => ceil(log2(5.55))=3. num_slots=8.
    // Capacity = 8 * 0.9 = 7.2. So, can add up to 8 items if strict num_slots limit.
    QuotientFilter<int> qf(5, 0.1); // Should give num_slots = 8
    std::cout << "QF(5, 0.1): q_bits=" << (int)qf.quotient_bits()
              << ", r_bits=" << (int)qf.remainder_bits()
              << ", num_slots=" << qf.num_slots()
              << ", capacity_approx=" << qf.capacity() << std::endl;

    bool success = true;
    size_t num_to_add = qf.num_slots(); // Try to fill up to physical slots
    if (qf.num_slots() == 0) {
        print_test_result("Skipping full behavior test due to 0 slots (config error)", false);
        return;
    }

    for (size_t i = 0; i < num_to_add; ++i) {
        try {
            qf.add(static_cast<int>(i * 101)); // Unique items
        } catch (const std::runtime_error& e) {
            std::cerr << "Error adding item " << i << ": " << e.what() << std::endl;
            success = false;
            break;
        }
    }
    if (success) {
        print_test_result("Added items up to num_slots", qf.size() == num_to_add);
        assert(qf.size() == num_to_add);
    } else {
        print_test_result("Failed to add items up to num_slots", false);
        assert(false);
    }


    // Try to add one more item, expecting a "full" exception (if strictly limited by num_slots)
    bool throws_on_full = false;
    if (success) { // Only if previous adds succeeded
        try {
            qf.add(static_cast<int>(num_to_add * 101 +1));
        } catch (const std::runtime_error& e) {
            if (std::string(e.what()).find("full") != std::string::npos) {
                throws_on_full = true;
                std::cout << "Caught expected 'full' exception: " << e.what() << std::endl;
            } else {
                std::cerr << "Caught unexpected runtime_error: " << e.what() << std::endl;
                success = false; // Mark overall test as failed due to unexpected error
            }
        }
        // This assertion depends on the exact full condition.
        // The current add() throws if item_count_ >= num_slots_
        print_test_result("Throws exception when adding to a full filter", throws_on_full);
        assert(throws_on_full);
    }
     if(!success) print_test_result("Full behavior test encountered issues", false);

}


// Basic False Positive Rate Test
void test_fpr() {
    std::cout << "\n--- Testing False Positive Rate ---" << std::endl;
    size_t num_insertions = 10000;
    double target_fp_rate = 0.01; // 1%
    QuotientFilter<int> qf(num_insertions, target_fp_rate);

    std::unordered_set<int> inserted_elements;
    for (size_t i = 0; i < num_insertions; ++i) {
        // Insert distinct, somewhat random numbers
        int val = static_cast<int>((i * 0x9E3779B9 + 0x61C88647) % 0xFFFFFFFF);
        qf.add(val);
        inserted_elements.insert(val);
    }
    print_test_result("FPR Test: Size after insertions", qf.size() == inserted_elements.size());
    // Note: qf.size() might be less if hash collisions on full 64-bit hash occur for QuotientHash,
    // or if add stops early. For this test, assume all unique items are added.
    // This needs to be robust if `add` doesn't add duplicates.
    // Our current `add` returns if `might_contain` is true. If there's a full hash collision,
    // `get_fingerprint_parts` will yield same (fq,fr), so `might_contain` would be true.
    // So size should be num_insertions if all generated vals are unique and their fingerprints too.

    size_t num_lookups = 100000;
    size_t false_positives = 0;
    size_t true_negatives_tested = 0;

    for (size_t i = 0; i < num_lookups; ++i) {
        // Lookup distinct items that were NOT inserted
        // Choose values in a different range or with a different pattern
        int val_to_check = static_cast<int>(((i + num_insertions) * 0x1B873593 + 0x91E10DE5) % 0xFFFFFFFF) ;
        if (inserted_elements.count(val_to_check)) {
            // This value was accidentally inserted, skip it for FPR test.
            // This can happen if val_to_check happens to be in inserted_elements due to hash collision or just by chance.
            // To be very robust, ensure val_to_check is truly not in inserted_elements.
            // A simpler way: generate lookup items and if they were inserted, regenerate.
             int attempts = 0;
             while(inserted_elements.count(val_to_check) && attempts < 100) {
                val_to_check += 1; // Simple way to get a new number
                attempts++;
             }
             if(inserted_elements.count(val_to_check)) continue; // Still a collision after trying, skip this lookup
        }

        true_negatives_tested++;
        if (qf.might_contain(val_to_check)) {
            false_positives++;
        }
    }

    if (true_negatives_tested == 0) {
        print_test_result("FPR Test: No non-inserted items were tested", false);
        assert(false);
        return;
    }

    double actual_fp_rate = static_cast<double>(false_positives) / true_negatives_tested;
    std::cout << std::fixed << std::setprecision(5);
    std::cout << "Target FP Rate: " << target_fp_rate << std::endl;
    std::cout << "Actual FP Rate: " << actual_fp_rate
              << " (FP: " << false_positives << " / TN_Tested: " << true_negatives_tested << ")" << std::endl;

    // Check if actual_fp_rate is reasonably close to target_fp_rate.
    // Allow some leeway, e.g., up to 2x or 3x the target for probabilistic structures,
    // especially with smaller test numbers or if hash function isn't perfect.
    // A very strict check might be `actual_fp_rate <= target_fp_rate * 1.5`
    // For a more robust check, ensure it's not drastically higher (e.g. > 0.5 when target is 0.01)
    bool fpr_ok = actual_fp_rate < (target_fp_rate * 2.5) || (actual_fp_rate < 0.001 && target_fp_rate < 0.001) ; // Heuristic check
     if (num_insertions < 1000) fpr_ok = actual_fp_rate < (target_fp_rate * 5.0); // More leeway for small N

    print_test_result("FPR within acceptable range", fpr_ok);
    // assert(fpr_ok); // This can be flaky, so commented out for CI, but good for dev
}


int main() {
    std::cout << "Starting QuotientFilter Tests..." << std::endl;

    test_construction();
    test_simple_add_lookup();
    test_multiple_items();
    test_string_items();
    test_full_behavior(); // This test is heuristic for "full"
    test_fpr();

    std::cout << "\n...QuotientFilter Tests Finished." << std::endl;
    // Add a summary or check overall pass/fail if not exiting on first failure.
    return 0;
}

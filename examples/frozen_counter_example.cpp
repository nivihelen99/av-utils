#include "FrozenCounter.h"
#include "counter.h" // For creating a Counter to convert from
#include <iostream>
#include <vector>
#include <string>
#include <cassert>

void print_line() {
    std::cout << "----------------------------------------\n";
}

template<typename K, typename C, typename A>
void print_frozen_counter(const cpp_collections::FrozenCounter<K, C, A>& fc, const std::string& name) {
    std::cout << "FrozenCounter '" << name << "':\n";
    if (fc.empty()) {
        std::cout << "  (empty)\n";
    } else {
        for (const auto& pair : fc) {
            std::cout << "  " << pair.first << ": " << pair.second << "\n";
        }
    }
    std::cout << "  Total unique elements: " << fc.size() << "\n";
    std::cout << "  Sum of all counts: " << fc.total() << "\n";
    print_line();
}

int main() {
    // 1. Construction from initializer_list
    cpp_collections::FrozenCounter<std::string> fc1 = {
        {"apple", 3}, {"banana", 2}, {"apple", 2}, {"orange", 1}, {"banana", 3}, {"grape", 0}, {"plum", -2}
    };
    print_frozen_counter(fc1, "fc1 (from initializer_list)");
    // Expected: apple: 5, banana: 5, orange: 1. grape and plum ignored (count <=0)
    // Total unique: 3, Sum of counts: 11

    assert(fc1.count("apple") == 5);
    assert(fc1["banana"] == 5); // Test operator[]
    assert(fc1.count("orange") == 1);
    assert(fc1.count("grape") == 0); // Not present
    assert(fc1.contains("apple"));
    assert(!fc1.contains("grape"));
    assert(fc1.size() == 3);
    assert(fc1.total() == 11);

    // 2. Construction from a vector of pairs
    std::vector<std::pair<char, int>> char_counts_vec = {
        {'a', 1}, {'b', 2}, {'a', 3}, {'c', 4}, {'b', -1}
    };
    cpp_collections::FrozenCounter<char> fc2(char_counts_vec.begin(), char_counts_vec.end());
    print_frozen_counter(fc2, "fc2 (from vector)");
    // Expected: a: 4, c: 4. b ignored.
    // Total unique: 2, Sum of counts: 8
    assert(fc2.count('a') == 4);
    assert(fc2.count('b') == 0);
    assert(fc2.count('c') == 4);
    assert(fc2.size() == 2);
    assert(fc2.total() == 8);

    // 3. Construction from an existing Counter (global namespace)
    ::Counter<int> mutable_counter; // Use global Counter
    mutable_counter.add(10, 3);
    mutable_counter.add(20, 5);
    mutable_counter.add(10, 2);
    mutable_counter.add(30, 0); // Should be ignored by FrozenCounter
    mutable_counter.add(40, -1); // Should be ignored

    std::cout << "Source mutable_counter:\n";
    for(const auto& p : mutable_counter) {
        std::cout << "  " << p.first << ": " << p.second << "\n";
    }
    print_line();

    cpp_collections::FrozenCounter<int> fc3(mutable_counter);
    print_frozen_counter(fc3, "fc3 (from mutable Counter)");
    // Expected: 10: 5, 20: 5
    // Total unique: 2, Sum of counts: 10
    assert(fc3.count(10) == 5);
    assert(fc3.count(20) == 5);
    assert(fc3.count(30) == 0);
    assert(fc3.count(40) == 0);
    assert(fc3.size() == 2);
    assert(fc3.total() == 10);

    // 4. Empty FrozenCounter
    cpp_collections::FrozenCounter<double> fc4;
    print_frozen_counter(fc4, "fc4 (empty)");
    assert(fc4.empty());
    assert(fc4.size() == 0);
    assert(fc4.total() == 0);

    // 5. Most common elements
    std::cout << "Most common elements in fc1:\n";
    auto common_fc1 = fc1.most_common(); // Get all
    for (const auto& p : common_fc1) {
        std::cout << "  " << p.first << ": " << p.second << "\n";
    }
    // Expected: apple:5, banana:5 (order depends on key comparison for ties), orange:1
    // Default std::less<std::string> -> "apple" before "banana"
    assert(common_fc1.size() == 3);
    assert(common_fc1[0].first == "apple" || common_fc1[0].first == "banana"); // Could be either if counts are same
    assert(common_fc1[0].second == 5);
    assert(common_fc1[1].first == "banana" || common_fc1[1].first == "apple");
    assert(common_fc1[1].second == 5);
    assert(common_fc1[2].first == "orange");
    assert(common_fc1[2].second == 1);


    std::cout << "Top 1 most common in fc1:\n";
    auto top_1_fc1 = fc1.most_common(1);
    for (const auto& p : top_1_fc1) {
        std::cout << "  " << p.first << ": " << p.second << "\n";
    }
    assert(top_1_fc1.size() == 1);
    assert(top_1_fc1[0].second == 5); // apple or banana

    // 6. Iteration
    std::cout << "Iterating fc3 (sorted by key):\n";
    for(const auto& item : fc3) { // fc3 has {10:5, 20:5}
        std::cout << "  Key: " << item.first << ", Count: " << item.second << std::endl;
    }
    // Expected: 10:5, then 20:5 (due to default std::less<int>)
    auto it_fc3 = fc3.begin();
    assert(it_fc3->first == 10 && it_fc3->second == 5);
    ++it_fc3;
    assert(it_fc3->first == 20 && it_fc3->second == 5);
    ++it_fc3;
    assert(it_fc3 == fc3.end());


    // 7. Comparison
    cpp_collections::FrozenCounter<std::string> fc1_copy = {
        {"apple", 5}, {"banana", 5}, {"orange", 1}
    };
    cpp_collections::FrozenCounter<std::string> fc1_different_order = {
        {"orange", 1}, {"banana", 5}, {"apple", 5}
    };
    cpp_collections::FrozenCounter<std::string> fc1_different_counts = {
        {"apple", 5}, {"banana", 4}, {"orange", 1}
    };

    assert(fc1 == fc1_copy);
    std::cout << "fc1 == fc1_copy: True\n";
    assert(fc1 == fc1_different_order); // Should be true as internal representation is sorted
    std::cout << "fc1 == fc1_different_order: True\n";
    assert(fc1 != fc1_different_counts);
    std::cout << "fc1 != fc1_different_counts: True\n";
    print_line();

    // 8. Hashing (demonstrate it compiles with std::unordered_map)
    std::unordered_map<cpp_collections::FrozenCounter<std::string>, int> map_of_fcs;
    map_of_fcs[fc1] = 100;
    map_of_fcs[fc1_different_order] = 200; // Should overwrite fc1 if hashes and equality are correct

    std::cout << "map_of_fcs[fc1]: " << map_of_fcs[fc1] << " (should be 200 if hash is good)\n";
    assert(map_of_fcs.size() == 1); // fc1 and fc1_different_order should be treated as same key
    assert(map_of_fcs[fc1] == 200);
    print_line();

    std::cout << "FrozenCounter examples completed successfully!\n";
    return 0;
}

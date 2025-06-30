#include "multiset_counter.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <set> // For std::multiset example

// Helper function to print a multiset (vector<T>)
template <typename T>
void print_multiset(const std::vector<T>& ms) {
    std::cout << "{";
    for (size_t i = 0; i < ms.size(); ++i) {
        std::cout << ms[i] << (i == ms.size() - 1 ? "" : ", ");
    }
    std::cout << "}";
}

// Helper function to print most_common results
template <typename T>
void print_most_common(const std::vector<std::pair<const std::vector<T>, int>>& common_items) {
    for (const auto& pair : common_items) {
        print_multiset(pair.first);
        std::cout << ": " << pair.second << std::endl;
    }
}

int main() {
    std::cout << "=== MultisetCounter Basic Example (std::string) ===" << std::endl;

    cpp_collections::multiset_counter<std::string> mc_str;

    // Add some multisets
    mc_str.add({"apple", "banana"});
    mc_str.add({"banana", "apple"}); // Same as above
    mc_str.add({"apple", "orange"});
    mc_str.add({"apple", "banana", "apple"}); // Different from {"apple", "banana"}
    mc_str.add({"grape"});

    std::cout << "Count of {'apple', 'banana'}: " << mc_str.count({"banana", "apple"}) << std::endl; // Expected: 2
    std::cout << "Count of {'apple', 'orange'}: " << mc_str[{"apple", "orange"}] << std::endl;       // Expected: 1
    std::cout << "Count of {'grape'}: " << mc_str.count({"grape"}) << std::endl;                 // Expected: 1
    std::cout << "Count of {'apple', 'banana', 'apple'}: " << mc_str.count({"apple", "apple", "banana"}) << std::endl; // Expected: 1
    std::cout << "Count of {'kiwi'}: " << mc_str.count({"kiwi"}) << std::endl;                     // Expected: 0

    std::cout << "\nTotal unique multisets: " << mc_str.size() << std::endl; // Expected: 4
    std::cout << "Total items counted (sum of counts): " << mc_str.total() << std::endl; // Expected: 2+1+1+1 = 5

    std::cout << "\nMost common (all):" << std::endl;
    print_most_common(mc_str.most_common());

    std::cout << "\nMost common (top 2):" << std::endl;
    print_most_common(mc_str.most_common(2));

    std::cout << "\nIterating through the counter:" << std::endl;
    for (const auto& pair : mc_str) { // Uses cbegin()/cend()
        print_multiset(pair.first);
        std::cout << ": " << pair.second << std::endl;
    }

    std::cout << "\n=== Using Initializer List Constructor ===" << std::endl;
    cpp_collections::multiset_counter<int> mc_init = {
        {1, 2, 3}, {3, 2, 1}, {1, 1, 2}, {1, 2, 3}
    };
    std::cout << "Count of {1, 2, 3} after init: " << mc_init.count({1, 2, 3}) << std::endl; // Expected: 3
    std::cout << "Count of {1, 1, 2} after init: " << mc_init.count({1, 1, 2}) << std::endl; // Expected: 1

    std::cout << "\n=== Example with std::list and custom comparator (descending order) ===" << std::endl;
    cpp_collections::multiset_counter<int, std::greater<int>> mc_list_custom_comp;
    std::list<int> l1 = {5, 1, 3}; // Canonical with std::greater: {5, 3, 1}
    std::vector<int> v1 = {3, 1, 5}; // Canonical with std::greater: {5, 3, 1}
    std::multiset<int, std::greater<int>> ms1_custom = {1, 5, 3}; // Already sorted by std::greater

    mc_list_custom_comp.add(l1);
    mc_list_custom_comp.add(v1);
    mc_list_custom_comp.add(ms1_custom);

    std::cout << "Count of {1, 3, 5} (using default vector for query): ";
    // For query, it will use its internal comparator (std::greater)
    print_multiset<int>({1, 3, 5});
    std::cout << " is " << mc_list_custom_comp.count({1, 3, 5}) << std::endl; // Expected: 3

    std::cout << "\nMost common for custom comparator:" << std::endl;
    // The keys (pair.first) will be sorted according to std::greater<int>
    print_most_common(mc_list_custom_comp.most_common());


    std::cout << "\n=== Anagram Example ===" << std::endl;
    cpp_collections::multiset_counter<char> anagram_counter;
    std::string word1 = "listen";
    std::string word2 = "silent";
    std::string word3 = "enlist";
    std::string word4 = "banana";

    // Convert strings to vector<char> for the counter
    // The generic add overload handles std::string directly if T is char
    anagram_counter.add(std::vector<char>(word1.begin(), word1.end()));
    anagram_counter.add(std::vector<char>(word2.begin(), word2.end()));
    anagram_counter.add(std::vector<char>(word3.begin(), word3.end()));
    anagram_counter.add(std::vector<char>(word4.begin(), word4.end()));

    // Or using the generic add for string (if T is char)
    // anagram_counter.add(word1); // This would also work

    std::cout << "Are 'listen' and 'silent' anagrams (same multiset of chars)?" << std::endl;
    std::vector<char> listen_chars(word1.begin(), word1.end());
    std::vector<char> silent_chars(word2.begin(), word2.end());

    // Canonical forms will be the same if they are anagrams
    std::sort(listen_chars.begin(), listen_chars.end());
    std::sort(silent_chars.begin(), silent_chars.end());

    std::cout << "Canonical for 'listen': "; print_multiset(listen_chars); std::cout << std::endl;
    std::cout << "Canonical for 'silent': "; print_multiset(silent_chars); std::cout << std::endl;

    std::cout << "Count of char multiset for 'listen': " << anagram_counter.count(std::vector<char>(word1.begin(), word1.end())) << std::endl; // Expected: 3
    std::cout << "Count of char multiset for 'banana': " << anagram_counter.count(std::vector<char>(word4.begin(), word4.end())) << std::endl; // Expected: 1

    std::cout << "\nClearing mc_str and checking size/empty:" << std::endl;
    mc_str.clear();
    std::cout << "Size after clear: " << mc_str.size() << std::endl;         // Expected: 0
    std::cout << "Empty after clear: " << std::boolalpha << mc_str.empty() << std::endl; // Expected: true
    std::cout << "Total after clear: " << mc_str.total() << std::endl;       // Expected: 0

    return 0;
}

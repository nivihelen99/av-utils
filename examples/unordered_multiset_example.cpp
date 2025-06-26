#include "unordered_multiset.h" // Assuming this path works from the build system
#include <iostream>
#include <string>
#include <vector>

// Helper function to print the contents of the multiset
template<typename T, typename Hash, typename KeyEqual>
void print_multiset_details(const cpp_utils::UnorderedMultiset<T, Hash, KeyEqual>& ms, const std::string& name) {
    std::cout << "--- Details for multiset: " << name << " ---" << std::endl;
    if (ms.empty()) {
        std::cout << name << " is empty." << std::endl;
        std::cout << "Total size: " << ms.size() << std::endl;
        return;
    }

    std::cout << "Total size: " << ms.size() << std::endl;
    std::cout << "Unique elements and their counts:" << std::endl;
    // Iterating over unique elements and their counts
    for (const auto& pair : ms) { // pair.first is the element, pair.second is its count
        std::cout << "- Element: '" << pair.first << "', Count: " << pair.second << std::endl;
    }
    std::cout << "----------------------------------------" << std::endl;
}


int main() {
    std::cout << "=== UnorderedMultiset Example ===" << std::endl;

    // 1. Basic usage with integers
    std::cout << "\n--- Integer Multiset Example ---" << std::endl;
    cpp_utils::UnorderedMultiset<int> int_ms;

    int_ms.insert(10);
    int_ms.insert(20);
    int_ms.insert(10); // Duplicate
    int_ms.insert(30);
    int_ms.insert(10); // Another duplicate
    int_ms.insert(25);

    print_multiset_details(int_ms, "int_ms after insertions");

    std::cout << "Count of 10: " << int_ms.count(10) << std::endl; // Expected: 3
    std::cout << "Count of 20: " << int_ms.count(20) << std::endl; // Expected: 1
    std::cout << "Count of 50 (non-existent): " << int_ms.count(50) << std::endl; // Expected: 0

    std::cout << "Contains 20? " << (int_ms.contains(20) ? "Yes" : "No") << std::endl;
    std::cout << "Contains 50? " << (int_ms.contains(50) ? "Yes" : "No") << std::endl;

    std::cout << "\nErasing one instance of 10..." << std::endl;
    int_ms.erase(10);
    print_multiset_details(int_ms, "int_ms after erasing one 10");

    std::cout << "\nErasing all instances of 10..." << std::endl;
    int_ms.erase_all(10);
    print_multiset_details(int_ms, "int_ms after erasing all 10s");

    std::cout << "\nClearing the multiset..." << std::endl;
    int_ms.clear();
    print_multiset_details(int_ms, "int_ms after clear");

    // 2. Usage with strings - a simple word frequency counter
    std::cout << "\n--- String Multiset Example (Word Frequency) ---" << std::endl;
    cpp_utils::UnorderedMultiset<std::string> word_freq_ms;
    std::vector<std::string> words = {
        "hello", "world", "hello", "c++", "multiset", "world", "hello", "example"
    };

    std::cout << "Adding words to multiset: ";
    for (const auto& word : words) {
        std::cout << word << " ";
        word_freq_ms.insert(word);
    }
    std::cout << std::endl;

    print_multiset_details(word_freq_ms, "word_freq_ms");

    std::cout << "Frequency of 'hello': " << word_freq_ms.count("hello") << std::endl;
    std::cout << "Frequency of 'world': " << word_freq_ms.count("world") << std::endl;
    std::cout << "Frequency of 'python': " << word_freq_ms.count("python") << std::endl;

    // 3. Swapping multisets
    std::cout << "\n--- Swap Example ---" << std::endl;
    cpp_utils::UnorderedMultiset<int> ms1;
    ms1.insert(1); ms1.insert(1); ms1.insert(2);

    cpp_utils::UnorderedMultiset<int> ms2;
    ms2.insert(100); ms2.insert(200); ms2.insert(200); ms2.insert(200);

    std::cout << "Before swap:" << std::endl;
    print_multiset_details(ms1, "ms1");
    print_multiset_details(ms2, "ms2");

    ms1.swap(ms2); // Member swap

    std::cout << "\nAfter member swap:" << std::endl;
    print_multiset_details(ms1, "ms1 (now has ms2's content)");
    print_multiset_details(ms2, "ms2 (now has ms1's content)");

    // Using non-member swap (std::swap or the custom one)
    swap(ms1, ms2); // Should call cpp_utils::swap or fall back to std::swap if ADL works

    std::cout << "\nAfter non-member swap (back to original):" << std::endl;
    print_multiset_details(ms1, "ms1");
    print_multiset_details(ms2, "ms2");


    std::cout << "\n=== Example Finished ===" << std::endl;
    return 0;
}

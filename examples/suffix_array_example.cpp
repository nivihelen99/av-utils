#include <iostream>
#include <string>
#include <vector>
#include "SuffixArray.h" // Assuming SuffixArray.h is in include path set by CMake

void print_occurrences(const std::string& text, const std::string& pattern, const std::vector<int>& occurrences) {
    std::cout << "In text \"" << text << "\":\n";
    std::cout << "  Pattern \"" << pattern << "\" occurs " << occurrences.size() << " times at indices: ";
    if (occurrences.empty()) {
        std::cout << "(none)";
    } else {
        for (size_t i = 0; i < occurrences.size(); ++i) {
            std::cout << occurrences[i] << (i == occurrences.size() - 1 ? "" : ", ");
        }
    }
    std::cout << std::endl;
}

int main() {
    std::string text = "abracadabra";
    std::cout << "Original text: \"" << text << "\"" << std::endl;
    std::cout << "Length of text: " << text.length() << std::endl;

    // Create a Suffix Array
    SuffixArray sa(text);

    // Get the raw suffix array (sorted indices)
    const std::vector<int>& sorted_indices = sa.get_array();
    std::cout << "\nSuffix Array (sorted indices of suffixes): ";
    for (size_t i = 0; i < sorted_indices.size(); ++i) {
        std::cout << sorted_indices[i] << (i == sorted_indices.size() - 1 ? "" : ", ");
    }
    std::cout << std::endl;

    std::cout << "\n--- Corresponding Suffixes (in lexicographical order) ---" << std::endl;
    for (int index : sorted_indices) {
        if (static_cast<size_t>(index) < text.length()) { // Basic sanity check
             std::cout << "Index " << index << ": \"" << text.substr(index) << "\"" << std::endl;
        } else {
             std::cout << "Index " << index << ": [Error: index out of bounds]" << std::endl;
        }
    }
    std::cout << "---------------------------------------------------------" << std::endl;


    // --- Count occurrences ---
    std::cout << "\n--- Counting Occurrences ---" << std::endl;
    std::string pattern1 = "abr";
    size_t count1 = sa.count_occurrences(pattern1);
    std::cout << "Pattern \"" << pattern1 << "\" occurs " << count1 << " times." << std::endl;

    std::string pattern2 = "a";
    size_t count2 = sa.count_occurrences(pattern2);
    std::cout << "Pattern \"" << pattern2 << "\" occurs " << count2 << " times." << std::endl;

    std::string pattern3 = "xyz"; // Does not exist
    size_t count3 = sa.count_occurrences(pattern3);
    std::cout << "Pattern \"" << pattern3 << "\" occurs " << count3 << " times." << std::endl;

    std::string pattern4 = "abracadabra"; // The whole string
    size_t count4 = sa.count_occurrences(pattern4);
    std::cout << "Pattern \"" << pattern4 << "\" occurs " << count4 << " times." << std::endl;

    std::string pattern5 = ""; // Empty pattern
    size_t count5 = sa.count_occurrences(pattern5);
    std::cout << "Pattern \"" << pattern5 << "\" (empty) occurs " << count5 << " times." << std::endl;


    // --- Find occurrences (get actual indices) ---
    std::cout << "\n--- Finding Occurrences (sorted by index) ---" << std::endl;
    std::vector<int> occurrences1 = sa.find_occurrences(pattern1);
    print_occurrences(text, pattern1, occurrences1);

    std::vector<int> occurrences2 = sa.find_occurrences(pattern2);
    print_occurrences(text, pattern2, occurrences2);

    std::vector<int> occurrences3 = sa.find_occurrences(pattern3);
    print_occurrences(text, pattern3, occurrences3);

    std::vector<int> occurrences4 = sa.find_occurrences(pattern4);
    print_occurrences(text, pattern4, occurrences4);

    std::vector<int> occurrences5 = sa.find_occurrences(pattern5); // Empty pattern
    print_occurrences(text, pattern5, occurrences5);


    std::string pattern_ra = "ra";
    std::vector<int> occurrences_ra = sa.find_occurrences(pattern_ra);
    print_occurrences(text, pattern_ra, occurrences_ra);


    std::cout << "\n--- Example with a longer text: \"mississippi river\" ---" << std::endl;
    std::string long_text = "mississippi river";
    SuffixArray sa_long(long_text);
    std::cout << "Original text: \"" << long_text << "\"" << std::endl;

    std::string p_issi = "issi";
    print_occurrences(long_text, p_issi, sa_long.find_occurrences(p_issi));
    std::cout << "  Count for \"" << p_issi << "\": " << sa_long.count_occurrences(p_issi) << std::endl;

    std::string p_i = "i";
    print_occurrences(long_text, p_i, sa_long.find_occurrences(p_i));
    std::cout << "  Count for \"" << p_i << "\": " << sa_long.count_occurrences(p_i) << std::endl;

    std::string p_river = "river";
    print_occurrences(long_text, p_river, sa_long.find_occurrences(p_river));
    std::cout << "  Count for \"" << p_river << "\": " << sa_long.count_occurrences(p_river) << std::endl;

    std::string p_space = " ";
    print_occurrences(long_text, p_space, sa_long.find_occurrences(p_space));
    std::cout << "  Count for \"" << p_space << "\": " << sa_long.count_occurrences(p_space) << std::endl;

    std::cout << "\nSuffix arrays are powerful for various stringology problems," << std::endl;
    std::cout << "such as finding the longest common substring, longest repeated substring," << std::endl;
    std::cout << "and more, especially when combined with an LCP (Longest Common Prefix) array." << std::endl;
    std::cout << "This implementation provides basic suffix array construction and search." << std::endl;

    return 0;
}

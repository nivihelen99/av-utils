#include "FrozenSet.h"
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

// Custom struct for demonstration
struct Book {
    std::string title;
    std::string author;

    Book(std::string t, std::string a) : title(std::move(t)), author(std::move(a)) {}

    // Comparison for sorting within FrozenSet
    bool operator<(const Book& other) const {
        if (author < other.author) return true;
        if (author > other.author) return false;
        return title < other.title;
    }

    // Equality (mainly for demonstration/easy checking)
    bool operator==(const Book& other) const {
        return title == other.title && author == other.author;
    }
};

// Hash specialization for Book to be used in FrozenSet (if Book itself was the Key)
// and for FrozenSet<Book> to be used as a key in unordered_map.
namespace std {
    template<> struct hash<Book> {
        size_t operator()(const Book& b) const {
            return std::hash<std::string>()(b.author) ^
                   (std::hash<std::string>()(b.title) << 1);
        }
    };
} // namespace std

// Custom comparator for strings (case-insensitive)
struct CaseInsensitiveStringCompare {
    bool operator()(const std::string& a, const std::string& b) const {
        return std::lexicographical_compare(
            a.begin(), a.end(),
            b.begin(), b.end(),
            [](char c1, char c2) {
                return std::tolower(c1) < std::tolower(c2);
            }
        );
    }
};


int main() {
    std::cout << "--- Basic FrozenSet of Integers ---" << std::endl;
    cpp_collections::FrozenSet<int> fs_ints = {5, 1, 8, 3, 5, 1, 9};

    std::cout << "Size: " << fs_ints.size() << std::endl; // Expected: 5 (duplicates removed)
    std::cout << "Elements: ";
    for (int val : fs_ints) {
        std::cout << val << " "; // Expected: 1 3 5 8 9 (sorted)
    }
    std::cout << std::endl;

    std::cout << "Contains 5? " << (fs_ints.contains(5) ? "Yes" : "No") << std::endl;
    std::cout << "Contains 10? " << (fs_ints.contains(10) ? "Yes" : "No") << std::endl;

    auto it_find = fs_ints.find(8);
    if (it_find != fs_ints.end()) {
        std::cout << "Found 8: " << *it_find << std::endl;
    }

    std::cout << "\n--- FrozenSet of Strings ---" << std::endl;
    std::vector<std::string> str_vector = {"banana", "apple", "cherry", "apple", "date"};
    cpp_collections::FrozenSet<std::string> fs_strings(str_vector.begin(), str_vector.end());

    std::cout << "Elements: ";
    for (const auto& s : fs_strings) {
        std::cout << s << " "; // Expected: apple banana cherry date
    }
    std::cout << std::endl;

    std::cout << "\n--- FrozenSet with Custom Objects (Books) ---" << std::endl;
    cpp_collections::FrozenSet<Book> fs_books = {
        {"The Lord of the Rings", "J.R.R. Tolkien"},
        {"Pride and Prejudice", "Jane Austen"},
        {"The Hobbit", "J.R.R. Tolkien"},
        {"Pride and Prejudice", "Jane Austen"} // Duplicate
    };

    std::cout << "Favorite Authors & Books (sorted by author, then title):" << std::endl;
    for (const auto& book : fs_books) {
        std::cout << " - " << book.author << ", \"" << book.title << "\"" << std::endl;
    }
    // Expected:
    // - Jane Austen, "Pride and Prejudice"
    // - J.R.R. Tolkien, "The Hobbit"
    // - J.R.R. Tolkien, "The Lord of the Rings"

    std::cout << "\n--- FrozenSet Comparison ---" << std::endl;
    cpp_collections::FrozenSet<int> fs_a = {1, 2, 3};
    cpp_collections::FrozenSet<int> fs_b = {3, 2, 1}; // Same elements, different init order
    cpp_collections::FrozenSet<int> fs_c = {1, 2, 4};

    std::cout << "fs_a == fs_b? " << (fs_a == fs_b ? "Yes" : "No") << std::endl; // Expected: Yes
    std::cout << "fs_a == fs_c? " << (fs_a == fs_c ? "Yes" : "No") << std::endl; // Expected: No
    std::cout << "fs_a < fs_c? " << (fs_a < fs_c ? "Yes" : "No") << std::endl;   // Expected: Yes

    std::cout << "\n--- FrozenSet Hashing (Usage as Map Key) ---" << std::endl;
    std::unordered_map<cpp_collections::FrozenSet<std::string>, std::string> anagram_groups;

    cpp_collections::FrozenSet<std::string> group1 = {"eat", "tea", "ate"};
    cpp_collections::FrozenSet<std::string> group2 = {"tan", "nat"};
    cpp_collections::FrozenSet<std::string> group3 = {"bat"};
    // Another way to define group1, should result in the same FrozenSet
    cpp_collections::FrozenSet<std::string> group1_alt = {"ate", "eat", "tea"};


    anagram_groups[group1] = "Group A (eat, tea, ate)";
    anagram_groups[group2] = "Group B (tan, nat)";
    anagram_groups[group3] = "Group C (bat)";

    std::cout << "Lookup group1: " << anagram_groups[group1] << std::endl;
    std::cout << "Lookup group1_alt (should be same as group1): " << anagram_groups[group1_alt] << std::endl;

    if (anagram_groups.count(group2)) {
        std::cout << "Found group2: " << anagram_groups.at(group2) << std::endl;
    }

    std::cout << "Number of unique groups in map: " << anagram_groups.size() << std::endl; // Expected: 3

    std::cout << "\n--- FrozenSet with Custom Comparator (Case-Insensitive Strings) ---" << std::endl;
    cpp_collections::FrozenSet<std::string, CaseInsensitiveStringCompare> fs_ci_strings =
        {"Apple", "banana", "CHERRY", "apple"};

    std::cout << "Case-insensitive elements: ";
    for (const auto& s : fs_ci_strings) {
        std::cout << s << " "; // Order might be "Apple", "banana", "CHERRY" or "apple", "banana", "CHERRY"
                               // depending on stability of sort/unique for equivalent items.
                               // What's guaranteed is that only one "apple"/"Apple" variant is stored.
    }
    std::cout << std::endl;
    std::cout << "Size: " << fs_ci_strings.size() << std::endl; // Expected: 3 ("Apple" and "apple" are equiv)
    std::cout << "Contains 'apple'? " << (fs_ci_strings.contains("apple") ? "Yes" : "No") << std::endl;
    std::cout << "Contains 'APPLE'? " << (fs_ci_strings.contains("APPLE") ? "Yes" : "No") << std::endl;
    std::cout << "Contains 'Banana'? " << (fs_ci_strings.contains("Banana") ? "Yes" : "No") << std::endl;


    // Demonstrating that FrozenSets with custom comparators, if their stored representations differ,
    // are different keys in a hash map, even if their elements are "equivalent" by the custom comparator.
    std::unordered_map<cpp_collections::FrozenSet<std::string, CaseInsensitiveStringCompare>, int> ci_map;
    cpp_collections::FrozenSet<std::string, CaseInsensitiveStringCompare> ci_key1 = {"Hello", "World"}; // stores "Hello", "World"
    cpp_collections::FrozenSet<std::string, CaseInsensitiveStringCompare> ci_key2 = {"hello", "world"}; // stores "hello", "world"

    // ci_key1 and ci_key2 are NOT equal because their internal data_ vectors differ.
    // The hash function for FrozenSet hashes the actual stored elements.
    std::cout << "ci_key1 == ci_key2 ? " << (ci_key1 == ci_key2 ? "Yes" : "No") << std::endl; // Expected: No

    ci_map[ci_key1] = 10;
    ci_map[ci_key2] = 20; // This will be a separate entry

    std::cout << "ci_map size: " << ci_map.size() << std::endl; // Expected: 2
    std::cout << "Value for {\"Hello\", \"World\"}: " << ci_map.at(ci_key1) << std::endl;
    std::cout << "Value for {\"hello\", \"world\"}: " << ci_map.at(ci_key2) << std::endl;


    return 0;
}

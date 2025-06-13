#include "counter.h"

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <random>
#include <iomanip>

// Helper function to print counter contents
template<typename T>
void print_counter(const Counter<T>& counter, const std::string& title = "") {
    if (!title.empty()) {
        std::cout << "\n=== " << title << " ===\n";
    }
    
    if (counter.empty()) {
        std::cout << "Counter is empty\n";
        return;
    }
    
    std::cout << "Size: " << counter.size() << ", Total: " << counter.total() << "\n";
    for (const auto& [key, count] : counter) {
        std::cout << "  " << key << ": " << count << "\n";
    }
}

// Example 1: Basic Operations
void example_basic_operations() {
    std::cout << "\n🚀 EXAMPLE 1: Basic Operations\n";
    std::cout << "================================\n";
    
    // Different ways to create counters
    Counter<std::string> words1;
    Counter<std::string> words2{"apple", "banana", "apple", "cherry"};
    Counter<std::string> words3{{"apple", 3}, {"banana", 2}, {"cherry", 1}};
    
    print_counter(words2, "Created from initializer list");
    print_counter(words3, "Created from key-value pairs");
    
    // Basic operations
    words1.add("hello");
    words1.add("world", 2);
    words1.add("hello");
    
    std::cout << "\nBasic add operations:\n";
    std::cout << "words1['hello']: " << words1["hello"] << "\n";
    std::cout << "words1.count('world'): " << words1.count("world") << "\n";
    std::cout << "words1.contains('missing'): " << std::boolalpha << words1.contains("missing") << "\n";
    
    // Subtraction
    words1.subtract("world");
    std::cout << "After subtract('world', 1): " << words1["world"] << "\n";
    
    words1.subtract("world", 2); // This will remove the key completely
    std::cout << "After subtract('world', 2): " << words1["world"] << " (removed)\n";
    
    print_counter(words1, "Final state of words1");
}

// Example 2: Iterator-based Construction and Range Operations
void example_iterators_and_ranges() {
    std::cout << "\n🔄 EXAMPLE 2: Iterators and Range Operations\n";
    std::cout << "===========================================\n";
    
    // Create from vector
    std::vector<char> letters{'a', 'b', 'a', 'c', 'b', 'a', 'd'};
    Counter<char> char_counter(letters.begin(), letters.end());
    
    print_counter(char_counter, "Counter from vector");
    
    // Range-based iteration
    std::cout << "\nUsing range-based for loop:\n";
    for (const auto& [letter, count] : char_counter) {
        std::cout << "  '" << letter << "' appears " << count << " times\n";
    }
    
    // STL algorithm compatibility
    std::cout << "\nUsing STL algorithms:\n";
    auto max_element = std::max_element(char_counter.begin(), char_counter.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    if (max_element != char_counter.end()) {
        std::cout << "Most frequent: '" << max_element->first 
                  << "' with " << max_element->second << " occurrences\n";
    }
}

// Example 3: most_common() and Sorting
void example_most_common() {
    std::cout << "\n📊 EXAMPLE 3: most_common() and Sorting\n";
    std::cout << "======================================\n";
    
    // Create a counter with random data
    Counter<int> numbers;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);
    
    // Add random numbers
    for (int i = 0; i < 50; ++i) {
        numbers.add(dis(gen));
    }
    
    print_counter(numbers, "Random number frequencies");
    
    // Get top 3 most common
    auto top3 = numbers.most_common(3);
    std::cout << "\nTop 3 most common numbers:\n";
    for (size_t i = 0; i < top3.size(); ++i) {
        std::cout << "  #" << (i+1) << ": " << top3[i].first 
                  << " (appears " << top3[i].second << " times)\n";
    }
    
    // Get all items sorted
    auto all_sorted = numbers.most_common();
    std::cout << "\nAll numbers by frequency:\n";
    for (const auto& [num, count] : all_sorted) {
        std::cout << "  " << num << ": " << std::string(count, '*') << " (" << count << ")\n";
    }
}

// Example 4: Arithmetic Operations
void example_arithmetic() {
    std::cout << "\n➕ EXAMPLE 4: Arithmetic Operations\n";
    std::cout << "==================================\n";
    
    Counter<std::string> inventory1{{"apples", 5}, {"bananas", 3}, {"oranges", 2}};
    Counter<std::string> inventory2{{"apples", 2}, {"bananas", 1}, {"grapes", 4}};
    
    print_counter(inventory1, "Inventory 1");
    print_counter(inventory2, "Inventory 2");
    
    // Addition
    auto combined = inventory1 + inventory2;
    print_counter(combined, "Combined (inventory1 + inventory2)");
    
    // Subtraction
    auto difference = inventory1 - inventory2;
    print_counter(difference, "Difference (inventory1 - inventory2)");
    
    // In-place operations
    inventory1 += inventory2;
    print_counter(inventory1, "Inventory1 after += inventory2");
    
    // Comparison
    Counter<std::string> inventory3 = inventory2;
    std::cout << "\ninventory2 == inventory3: " << std::boolalpha << (inventory2 == inventory3) << "\n";
    std::cout << "inventory1 == inventory2: " << std::boolalpha << (inventory1 == inventory2) << "\n";
}

// Example 5: Set Operations
void example_set_operations() {
    std::cout << "\n🔗 EXAMPLE 5: Set Operations\n";
    std::cout << "===========================\n";
    
    Counter<char> set1{{'a', 3}, {'b', 2}, {'c', 1}};
    Counter<char> set2{{'b', 1}, {'c', 4}, {'d', 2}};
    
    print_counter(set1, "Set 1");
    print_counter(set2, "Set 2");
    
    // Intersection (minimum counts)
    auto intersection = set1.intersection(set2);
    print_counter(intersection, "Intersection (min counts)");
    
    // Union (maximum counts)
    auto union_result = set1.union_with(set2);
    print_counter(union_result, "Union (max counts)");
}

// Example 6: Filtering Operations
void example_filtering() {
    std::cout << "\n🔍 EXAMPLE 6: Filtering Operations\n";
    std::cout << "=================================\n";
    
    Counter<int> mixed{-2, 3, -1, 0, 5, -3, 2};
    mixed.add(-2, 2); // Make some negative counts
    mixed.subtract(3, 5); // Make 3 negative
    
    print_counter(mixed, "Mixed positive/negative counter");
    
    // Filter positive values
    auto positive = mixed.positive();
    print_counter(positive, "Positive values only");
    
    // Filter negative values  
    auto negative = mixed.negative();
    print_counter(negative, "Negative values only");
    
    // Custom filter: even numbers with count > 1
    auto even_frequent = mixed.filter([](int key, int count) {
        return key % 2 == 0 && count > 1;
    });
    print_counter(even_frequent, "Even numbers with count > 1");
}

// Example 7: Advanced Usage with Custom Types
class Person {
public:
    std::string name;
    int age;
    
    Person(std::string n, int a) : name(std::move(n)), age(a) {}

    // Default constructor
    Person() = default;

    // For sorting and tie-breaking in Counter::most_common
    bool operator<(const Person& other) const {
        if (name != other.name) {
            return name < other.name;
        }
        return age < other.age;
    }
    
    bool operator==(const Person& other) const {
        return name == other.name && age == other.age;
    }
};

// Custom hash function for Person
struct PersonHash {
    std::size_t operator()(const Person& p) const {
        return std::hash<std::string>{}(p.name) ^ (std::hash<int>{}(p.age) << 1);
    }
};

// Custom output operator
std::ostream& operator<<(std::ostream& os, const Person& p) {
    return os << p.name << "(" << p.age << ")";
}

void example_custom_types() {
    std::cout << "\n👥 EXAMPLE 7: Custom Types with Custom Hash\n";
    std::cout << "==========================================\n";
    
    Counter<Person, PersonHash> people;
    
    people.add(Person{"Alice", 25});
    people.add(Person{"Bob", 30}, 2);
    people.add(Person{"Alice", 25});
    people.add(Person{"Charlie", 35});
    
    std::cout << "People counter:\n";
    for (const auto& [person, count] : people) {
        std::cout << "  " << person << ": " << count << "\n";
    }
    
    std::cout << "\nMost popular people:\n";
    auto popular = people.most_common(2);
    for (size_t i = 0; i < popular.size(); ++i) {
        std::cout << "  #" << (i+1) << ": " << popular[i].first 
                  << " (" << popular[i].second << " mentions)\n";
    }
}

// Example 8: Performance and Memory Management
void example_performance() {
    std::cout << "\n⚡ EXAMPLE 8: Performance and Memory Management\n";
    std::cout << "==============================================\n";
    
    Counter<int> big_counter;
    
    // Reserve space for better performance
    big_counter.reserve(1000);
    
    std::cout << "Initial bucket count: " << big_counter.bucket_count() << "\n";
    std::cout << "Initial load factor: " << std::fixed << std::setprecision(2) 
              << big_counter.load_factor() << "\n";
    
    // Add many elements
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    for (int i = 0; i < 500; ++i) {
        big_counter.add(dis(gen));
    }
    
    std::cout << "After adding 500 elements:\n";
    std::cout << "  Size: " << big_counter.size() << "\n";
    std::cout << "  Total count: " << big_counter.total() << "\n";
    std::cout << "  Bucket count: " << big_counter.bucket_count() << "\n";
    std::cout << "  Load factor: " << std::fixed << std::setprecision(2) 
              << big_counter.load_factor() << "\n";
    
    // Access internal data
    const auto& data = big_counter.data();
    std::cout << "  Direct data access size: " << data.size() << "\n";
}

// Example 9: Text Analysis Use Case
void example_text_analysis() {
    std::cout << "\n📝 EXAMPLE 9: Text Analysis Use Case\n";
    std::cout << "===================================\n";
    
    std::string text = "the quick brown fox jumps over the lazy dog the fox is quick";
    
    // Word frequency
    Counter<std::string> word_freq;
    std::istringstream iss(text);
    std::string word;
    
    while (iss >> word) {
        word_freq.add(word);
    }
    
    print_counter(word_freq, "Word frequencies");
    
    // Character frequency
    Counter<char> char_freq;
    for (char c : text) {
        if (c != ' ') { // Skip spaces
            char_freq.add(c);
        }
    }
    
    std::cout << "\nCharacter frequencies (excluding spaces):\n";
    auto char_sorted = char_freq.most_common();
    for (const auto& [ch, count] : char_sorted) {
        std::cout << "  '" << ch << "': " << count << "\n";
    }
    
    // Find unique words (appear exactly once)
    auto unique_words = word_freq.filter([](const std::string&, int count) {
        return count == 1;
    });
    
    std::cout << "\nWords that appear exactly once:\n";
    for (const auto& [word, count] : unique_words) {
        std::cout << "  " << word << "\n";
    }
}

// Example 10: Move Semantics and Efficiency
void example_move_semantics() {
    std::cout << "\n🚀 EXAMPLE 10: Move Semantics and Efficiency\n";
    std::cout << "===========================================\n";
    
    Counter<std::string> counter;
    
    // Demonstrate move semantics
    std::string temp = "expensive_to_copy_string_with_long_content";
    
    std::cout << "Adding string via copy: ";
    counter.add(temp);
    std::cout << "temp still valid: '" << temp << "'\n";
    
    std::cout << "Adding string via move: ";
    counter.add(std::move(temp));
    std::cout << "temp after move: '" << temp << "' (moved-from state)\n";
    
    print_counter(counter, "Counter after move operations");
    
    // Demonstrate non-const operator[]
    Counter<std::string> modifiable;
    modifiable["direct"] = 5;  // Direct assignment
    modifiable["access"] += 3; // Increment
    
    print_counter(modifiable, "Counter with direct modifications");
}

int main() {
    std::cout << "🎯 Counter<T> Comprehensive Examples\n";
    std::cout << "====================================\n";
    
    try {
        example_basic_operations();
        example_iterators_and_ranges();
        example_most_common();
        example_arithmetic();
        example_set_operations();
        example_filtering();
        example_custom_types();
        example_performance();
        example_text_analysis();
        example_move_semantics();
        
        std::cout << "\n✅ All examples completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

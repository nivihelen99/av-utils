
// Example usage and demonstrations
#include <iostream>
#include <vector>
#include <string>
#include <memory>

#include "default_dict.h"
using namespace std_ext;


void demonstrate_basic_usage() {
    std::cout << "=== Basic Usage ===\n";
    
    // Integer defaultdict with zero default
    std_ext::defaultdict<std::string, int> counter(std_ext::zero_factory<int>());
    
    counter["apple"] += 5;
    counter["banana"] += 3;
    counter["cherry"]; // Creates with default value 0
    
    std::cout << "apple: " << counter["apple"] << "\n";
    std::cout << "banana: " << counter["banana"] << "\n";
    std::cout << "cherry: " << counter["cherry"] << "\n";
    std::cout << "unknown: " << counter["unknown"] << "\n"; // Auto-creates with 0
    std::cout << "Size: " << counter.size() << "\n\n";
}

void demonstrate_vector_defaultdict() {
    std::cout << "=== Vector DefaultDict ===\n";
    
    // Vector defaultdict for grouping
    std_ext::defaultdict<std::string, std::vector<int>> groups(
        std_ext::default_factory<std::vector<int>>()
    );
    
    groups["evens"].push_back(2);
    groups["evens"].push_back(4);
    groups["odds"].push_back(1);
    groups["odds"].push_back(3);
    groups["empty"]; // Creates empty vector
    
    for (const auto& [key, vec] : groups) {
        std::cout << key << ": ";
        for (int val : vec) {
            std::cout << val << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

void demonstrate_custom_factory() {
    std::cout << "=== Custom Factory ===\n";
    
    // Custom default factory
    auto string_factory = []() { return std::string("DEFAULT"); };
    
    std_ext::defaultdict<int, std::string> dict(string_factory);
    
    dict[1] = "one";
    dict[2] = "two";
    
    std::cout << "dict[1]: " << dict[1] << "\n";
    std::cout << "dict[999]: " << dict[999] << "\n"; // Gets "DEFAULT"
    std::cout << "\n";
}

void demonstrate_stl_compatibility() {
    std::cout << "=== STL Compatibility ===\n";
    
    std_ext::defaultdict<std::string, int> dict(std_ext::zero_factory<int>());
    
    // Initialize with data
    dict = {{"a", 1}, {"b", 2}, {"c", 3}};
    
    // STL algorithms work
    auto it = std::find_if(dict.begin(), dict.end(),
        [](const auto& pair) { return pair.second > 1; });
    
    if (it != dict.end()) {
        std::cout << "Found: " << it->first << " -> " << it->second << "\n";
    }
    
    // Range-based for loop
    std::cout << "All entries: ";
    for (const auto& [key, value] : dict) {
        std::cout << key << ":" << value << " ";
    }
    std::cout << "\n\n";
}

void demonstrate_memory_management() {
    std::cout << "=== Memory Management ===\n";
    
    // Custom allocator example (using default allocator for demo)
    using AllocType = std::allocator<std::pair<const std::string, std::unique_ptr<int>>>;
    
    auto ptr_factory = []() { return std::make_unique<int>(42); };
    
    std_ext::defaultdict<std::string, std::unique_ptr<int>, 
                        decltype(ptr_factory), std::hash<std::string>, 
                        std::equal_to<std::string>, AllocType> smart_dict(ptr_factory);
    
    smart_dict["test"]; // Creates unique_ptr with value 42
    
    std::cout << "smart_dict[\"test\"]: " << *smart_dict["test"] << "\n";
    std::cout << "smart_dict[\"new\"]: " << *smart_dict["new"] << "\n";
    std::cout << "\n";
}

int main() {
    demonstrate_basic_usage();
    demonstrate_vector_defaultdict();
    demonstrate_custom_factory();
    demonstrate_stl_compatibility();
    demonstrate_memory_management();
    return 0;
}

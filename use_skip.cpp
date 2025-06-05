#include "skiplist.h"

// Example usage and test cases
int main() {
    SkipList<int> skipList;
    
    std::cout << "=== Inserting values ===" << std::endl;
    std::vector<int> values = {3, 6, 7, 9, 12, 19, 17, 26, 21, 25};
    
    for (int val : values) {
        skipList.insert(val);
    }
    
    skipList.display();
    
    std::cout << "=== Search operations ===" << std::endl;
    std::cout << "Search 19: " << (skipList.search(19) ? "Found" : "Not found") << std::endl;
    std::cout << "Search 15: " << (skipList.search(15) ? "Found" : "Not found") << std::endl;
    
    std::cout << "\n=== Skip list size ===" << std::endl;
    std::cout << "Size: " << skipList.size() << std::endl;
    
    std::cout << "\n=== All values in order ===" << std::endl;
    skipList.printValues();
    
    std::cout << "\n=== K-th element queries ===" << std::endl;
    try {
        std::cout << "3rd smallest (0-indexed): " << skipList.kthElement(3) << std::endl;
        std::cout << "5th smallest (0-indexed): " << skipList.kthElement(5) << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    
    std::cout << "\n=== Range query [10, 20] ===" << std::endl;
    auto rangeResult = skipList.rangeQuery(10, 20);
    std::cout << "Values in range [10, 20]: ";
    for (int val : rangeResult) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    std::cout << "\n=== Deletion operations ===" << std::endl;
    skipList.remove(19);
    skipList.remove(15); // Should fail
    
    skipList.display();
    skipList.printValues();

    std::cout << "\n\n=== Testing SkipList<std::string> ===" << std::endl;
    SkipList<std::string> stringSkipList;

    std::cout << "--- Inserting strings ---" << std::endl;
    stringSkipList.insert("apple");
    stringSkipList.insert("banana");
    stringSkipList.insert("cherry");
    stringSkipList.insert("date");
    stringSkipList.insert("fig");

    stringSkipList.display();
    stringSkipList.printValues();

    std::cout << "--- Search operations (string) ---" << std::endl;
    std::cout << "Search 'cherry': " << (stringSkipList.search("cherry") ? "Found" : "Not found") << std::endl;
    std::cout << "Search 'grape': " << (stringSkipList.search("grape") ? "Found" : "Not found") << std::endl;

    std::cout << "--- String skip list size ---" << std::endl;
    std::cout << "Size: " << stringSkipList.size() << std::endl;

    std::cout << "--- K-th element (string) ---" << std::endl;
    try {
        std::cout << "1st smallest (0-indexed): " << stringSkipList.kthElement(1) << std::endl; // Should be banana
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }

    std::cout << "--- Range query ['banana', 'fig'] (string) ---" << std::endl;
    auto stringRangeResult = stringSkipList.rangeQuery("banana", "fig");
    std::cout << "Values in range ['banana', 'fig']: ";
    for (const std::string& val : stringRangeResult) {
        std::cout << "\"" << val << "\" ";
    }
    std::cout << std::endl;


    std::cout << "--- Deletion operations (string) ---" << std::endl;
    stringSkipList.remove("banana");
    stringSkipList.remove("grape"); // Should fail

    stringSkipList.display();
    stringSkipList.printValues();

    std::cout << "\n\n=== Bulk Operations Test (int) ===" << std::endl;
    SkipList<int> bulkIntList;
    std::vector<int> int_bulk_values = {50, 10, 30, 20, 60, 40, 30}; // Includes duplicates, unsorted
    std::cout << "--- Bulk Insert (int) ---" << std::endl;
    std::cout << "Inserting: ";
    for(int v : int_bulk_values) std::cout << v << " ";
    std::cout << std::endl;
    bulkIntList.insert_bulk(int_bulk_values);
    bulkIntList.display();
    bulkIntList.printValues();
    std::cout << "Size: " << bulkIntList.size() << std::endl;

    std::cout << "--- Bulk Insert (int) with empty vector ---" << std::endl;
    bulkIntList.insert_bulk({}); // Empty vector
    bulkIntList.display(); // Should be unchanged
    bulkIntList.printValues();


    std::vector<int> int_remove_values = {30, 70, 10, 30, 5}; // Remove existing, non-existing, duplicate
    std::cout << "--- Bulk Remove (int) ---" << std::endl;
    std::cout << "Removing: ";
    for(int v : int_remove_values) std::cout << v << " ";
    std::cout << std::endl;
    size_t removed_count_int = bulkIntList.remove_bulk(int_remove_values);
    std::cout << "Successfully removed " << removed_count_int << " items." << std::endl;
    bulkIntList.display();
    bulkIntList.printValues();
    std::cout << "Size: " << bulkIntList.size() << std::endl;

    std::cout << "--- Bulk Remove (int) with empty vector ---" << std::endl;
    removed_count_int = bulkIntList.remove_bulk({}); // Empty vector
    std::cout << "Successfully removed " << removed_count_int << " items." << std::endl;
    bulkIntList.display(); // Should be unchanged
    bulkIntList.printValues();


    std::cout << "\n\n=== Bulk Operations Test (std::string) ===" << std::endl;
    SkipList<std::string> bulkStringList;
    std::vector<std::string> string_bulk_values = {"orange", "apple", "pear", "banana", "apple"};
    std::cout << "--- Bulk Insert (string) ---" << std::endl;
    std::cout << "Inserting: ";
    for(const auto& s : string_bulk_values) std::cout << "\"" << s << "\" ";
    std::cout << std::endl;
    bulkStringList.insert_bulk(string_bulk_values);
    bulkStringList.display();
    bulkStringList.printValues();
    std::cout << "Size: " << bulkStringList.size() << std::endl;

    std::cout << "--- Bulk Insert (string) with empty vector ---" << std::endl;
    bulkStringList.insert_bulk({}); // Empty vector
    bulkStringList.display(); // Should be unchanged
    bulkStringList.printValues();

    std::vector<std::string> string_remove_values = {"apple", "grape", "pear", "fig", "apple"};
    std::cout << "--- Bulk Remove (string) ---" << std::endl;
    std::cout << "Removing: ";
    for(const auto& s : string_remove_values) std::cout << "\"" << s << "\" ";
    std::cout << std::endl;
    size_t removed_count_string = bulkStringList.remove_bulk(string_remove_values);
    std::cout << "Successfully removed " << removed_count_string << " items." << std::endl;
    bulkStringList.display();
    bulkStringList.printValues();
    std::cout << "Size: " << bulkStringList.size() << std::endl;
    
    std::cout << "--- Bulk Remove (string) with empty vector ---" << std::endl;
    removed_count_string = bulkStringList.remove_bulk({}); // Empty vector
    std::cout << "Successfully removed " << removed_count_string << " items." << std::endl;
    bulkStringList.display(); // Should be unchanged
    bulkStringList.printValues();

    return 0;
}

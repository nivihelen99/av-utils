#include "skiplist.h"

// To compile this example using standard library malloc/free for SkipList nodes
// instead of the default custom memory pool, define the SKIPLIST_USE_STD_ALLOC macro.
// For example, with g++:
// g++ use_skip.cpp -o use_skip_std_alloc -std=c++17 -DSKIPLIST_USE_STD_ALLOC -I../include
//
// To compile with the default memory pool:
// g++ use_skip.cpp -o use_skip_mem_pool -std=c++17 -I../include

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


    std::cout << "\n\n=== Iterator Test (int) ===" << std::endl;
    SkipList<int> iter_list;
    iter_list.insert(1);
    iter_list.insert(5);
    iter_list.insert(2);
    iter_list.insert(8);
    iter_list.insert(3);

    std::cout << "Initial list for iterator tests:" << std::endl;
    iter_list.display();

    std::cout << "Iterating using begin()/end(): ";
    for (SkipList<int>::iterator it = iter_list.begin(); it != iter_list.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    std::cout << "Iterating using range-based for: ";
    for (int val : iter_list) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    std::cout << "Iterating using cbegin()/cend(): ";
    const auto& const_iter_list = iter_list;
    for (SkipList<int>::const_iterator cit = const_iter_list.cbegin(); cit != const_iter_list.cend(); ++cit) {
        std::cout << *cit << " ";
    }
    std::cout << std::endl;

    std::cout << "Iterating using range-based for (const): ";
    for (int val : const_iter_list) {
        std::cout << val << " ";
    }
    std::cout << std::endl;


    std::cout << "\n\n=== Key-Value Pair Test (std::pair<const int, std::string>) ===" << std::endl;
    SkipList<std::pair<const int, std::string>> kv_list;

    std::cout << "--- Inserting key-value pairs ---" << std::endl;
    kv_list.insert({10, "apple"});
    kv_list.insert({5, "banana"});
    kv_list.insert({20, "cherry"});
    std::cout << "Attempting to insert duplicate key 5 (banana should remain):" << std::endl;
    kv_list.insert({5, "orange"}); // Test duplicate key insertion

    kv_list.display();
    kv_list.printValues();

    std::cout << "--- Search operations (key-value) ---" << std::endl;
    std::cout << "Search for key 5: " << (kv_list.search({5, ""}) ? "Found" : "Not found") << std::endl;
    std::cout << "Search for key 15: " << (kv_list.search({15, ""}) ? "Found" : "Not found") << std::endl;

    std::cout << "--- Iterating through key-value pairs ---" << std::endl;
    std::cout << "Pairs: ";
    for (const auto& kv_pair : kv_list) {
        std::cout << "<" << kv_pair.first << ":" << kv_pair.second << "> ";
    }
    std::cout << std::endl;

    std::cout << "--- Remove operation (key-value) ---" << std::endl;
    std::cout << "Removing key 5: " << (kv_list.remove({5, ""}) ? "Removed" : "Not removed") << std::endl;
    kv_list.display();
    std::cout << "Removing key 15 (non-existent): " << (kv_list.remove({15, ""}) ? "Removed" : "Not removed") << std::endl;
    kv_list.display();


    std::cout << "--- Range query for keys [7, 25] (key-value) ---" << std::endl;
    // For rangeQuery, T is std::pair<const int, std::string>.
    // So minVal and maxVal should be of this type. Key part is used for comparison.
    auto kv_range_result = kv_list.rangeQuery({7, ""}, {25, ""});
    std::cout << "Values in range: ";
    for (const auto& p : kv_range_result) {
        std::cout << "<" << p.first << ":" << p.second << "> ";
    }
    std::cout << std::endl;

    return 0;
}

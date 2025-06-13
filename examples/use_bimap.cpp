
#include "bimap.h"
#include <iostream>     // For std::cout, std::endl
#include <string>       // For std::string
#include <vector>       // For std::vector
#include <algorithm>    // For std::find_if, std::count_if, std::for_each, std::transform
#include <utility>      // For std::pair, std::move, std::piecewise_construct, std::forward_as_tuple
#include <map>          // For std::map (used in some examples if we were to collect results)

// Example usage demonstrating STL algorithm compatibility
int main() {
    BiMap<std::string, int> user_ids;
    
    // Insert using various methods
    user_ids.insert("alice", 1001);
    user_ids.insert({"bob", 1002});
    user_ids.insert({{"charlie", 1003}, {"dave", 1004}});
    
    std::cout << "=== Basic Usage ===" << std::endl;
    std::cout << "alice's ID: " << user_ids.at_left("alice") << std::endl;
    std::cout << "User 1002: " << user_ids.at_right(1002) << std::endl;
    
    std::cout << "\n=== STL Algorithm Examples ===" << std::endl;
    
    // Use std::find_if with left view
    auto left_view = user_ids.left();
    auto it = std::find_if(left_view.begin(), left_view.end(),
        [](const auto& pair) { return pair.second > 1002; });
    
    if (it != left_view.end()) {
        std::cout << "First user with ID > 1002: " << it->first << " -> " << it->second << std::endl;
    }
    
    // Use std::count_if
    auto count = std::count_if(left_view.begin(), left_view.end(),
        [](const auto& pair) { return pair.first.length() > 4; });
    std::cout << "Users with names longer than 4 chars: " << count << std::endl;
    
    // Use std::for_each
    std::cout << "\nAll users (using std::for_each on left view):" << std::endl;
    std::for_each(left_view.begin(), left_view.end(),
        [](const auto& pair) {
            std::cout << "  " << pair.first << " -> " << pair.second << std::endl;
        });
    
    // Use std::for_each on right view
    std::cout << "\nAll users (using std::for_each on right view):" << std::endl;
    auto right_view = user_ids.right();
    std::for_each(right_view.begin(), right_view.end(),
        [](const auto& pair) {
            std::cout << "  " << pair.first << " -> " << pair.second << std::endl;
        });
    
    // Use std::transform to collect all usernames
    std::vector<std::string> usernames;
    std::transform(left_view.begin(), left_view.end(), std::back_inserter(usernames),
        [](const auto& pair) { return pair.first; });
    
    std::cout << "\nUsernames collected with std::transform: ";
    for (const auto& name : usernames) {
        std::cout << name << " ";
    }
    std::cout << std::endl;
    
    // Range-based for loops work too!
    std::cout << "\nUsing range-based for loop on left view:" << std::endl;
    for (const auto& [username, id] : user_ids.left()) {
        std::cout << "  " << username << " has ID " << id << std::endl;
    }
    
    std::cout << "\nUsing range-based for loop on right view:" << std::endl;
    for (const auto& [id, username] : user_ids.right()) {
        std::cout << "  ID " << id << " belongs to " << username << std::endl;
    }
    
    // Default iteration (left view)
    std::cout << "\nDefault iteration (same as left view):" << std::endl;
    for (const auto& pair : user_ids) {
        std::cout << "  " << pair.first << " -> " << pair.second << std::endl;
    }

    std::cout << "\n=== Emplace Example ===" << std::endl;
    BiMap<std::string, int> product_codes;
    // Emplace new items
    auto result1_emplace = product_codes.emplace("laptop", 2001); // Changed var name
    if (result1_emplace.second) {
        std::cout << "Emplaced: " << result1_emplace.first->first << " -> " << result1_emplace.first->second << std::endl;
    }
    product_codes.emplace(std::piecewise_construct,
                        std::forward_as_tuple("monitor"),
                        std::forward_as_tuple(2002));
    std::cout << "Monitor code: " << product_codes.at_left("monitor") << std::endl;

    // Try to emplace duplicate left key
    auto result2_emplace = product_codes.emplace("laptop", 2003); // Changed var name
    if (!result2_emplace.second) {
        std::cout << "Failed to emplace laptop again, existing: "
                  << result2_emplace.first->first << " -> " << result2_emplace.first->second << std::endl;
    }
    // Try to emplace item where right value would conflict
     auto result3_emplace = product_codes.emplace("keyboard", 2001); // Changed var name
    if (!result3_emplace.second && result3_emplace.first == product_codes.left_end()) {
         std::cout << "Failed to emplace keyboard with code 2001 (code already used by "
                   << product_codes.at_right(2001) << ")" << std::endl;
    }
    std::cout << "Product codes size: " << product_codes.size() << std::endl;

    std::cout << "\n=== Try Emplace Example ===" << std::endl;
    BiMap<int, std::string> error_codes;
    // Try emplace new items
    auto tr_result1 = error_codes.try_emplace_left(404, "Not Found");
    if (tr_result1.second) {
        std::cout << "Try-emplaced: " << tr_result1.first->first << " -> " << tr_result1.first->second << std::endl;
    }
    // Key exists
    auto tr_result2 = error_codes.try_emplace_left(404, "File Not Found");
    if (!tr_result2.second) {
         std::cout << "Failed to try-emplace 404 again, existing: "
                   << tr_result2.first->first << " -> " << tr_result2.first->second << std::endl;
    }
    // Value would conflict
    error_codes.insert(500, "Server Error"); // Make sure 500 is present for next test
    auto tr_result3 = error_codes.try_emplace_left(403, "Server Error"); // 403 is new, "Server Error" is not
     if (!tr_result3.second && tr_result3.first == error_codes.left_end()) {
         std::cout << "Failed to try-emplace 403 with 'Server Error' (value already used by "
                   << error_codes.at_right("Server Error") << ")" << std::endl;
    }
    std::cout << "Error codes:" << std::endl;
    for(const auto& [code, msg] : error_codes.left()){ // C++17 structured binding
        std::cout << "  " << code << ": " << msg << std::endl;
    }

    std::cout << "\n=== Swap Example ===" << std::endl;
    BiMap<std::string, int> map1, map2;
    map1.insert("one", 1);
    map1.insert("two", 2);
    map2.insert("three", 3);

    std::cout << "Map1 before swap: size " << map1.size();
    if(map1.contains_left("one")) std::cout << ", one -> " << map1.at_left("one");
    std::cout << std::endl;
    std::cout << "Map2 before swap: size " << map2.size();
    if(map2.contains_left("three")) std::cout << ", three -> " << map2.at_left("three");
    std::cout << std::endl;

    map1.swap(map2); // Member swap
    std::cout << "Map1 after member swap with map2: size " << map1.size();
    if(map1.contains_left("three")) std::cout << ", three -> " << map1.at_left("three");
    std::cout << std::endl;
    std::cout << "Map2 after member swap with map1: size " << map2.size();
    if(map2.contains_left("one")) std::cout << ", one -> " << map2.at_left("one");
    std::cout << std::endl;

    using std::swap;
    swap(map1, map2); // Non-member swap (ADL)
    std::cout << "Map1 after std::swap: size " << map1.size();
    if(map1.contains_left("one")) std::cout << ", one -> " << map1.at_left("one");
    std::cout << std::endl;

    std::cout << "\n=== Comparison Example ===" << std::endl;
    BiMap<std::string, int> cmp_map1, cmp_map2, cmp_map3;
    cmp_map1.insert("apple", 1);
    cmp_map1.insert("banana", 2);
    cmp_map2.insert("apple", 1);
    cmp_map2.insert("banana", 2);
    cmp_map3.insert("apple", 1);
    cmp_map3.insert("cherry", 3);

    if (cmp_map1 == cmp_map2) {
        std::cout << "cmp_map1 is equal to cmp_map2" << std::endl;
    }
    if (cmp_map1 != cmp_map3) {
        std::cout << "cmp_map1 is not equal to cmp_map3" << std::endl;
    }

    std::cout << "\n=== Move Semantics Note ===" << std::endl;
    std::cout << "BiMap now supports move semantics for insert and insert_or_assign operations," << std::endl;
    std::cout << "e.g., bimap.insert(std::move(my_left_obj), std::move(my_right_obj));" << std::endl;
    std::cout << "This can improve performance for types expensive to copy." << std::endl;
    
    return 0;
}

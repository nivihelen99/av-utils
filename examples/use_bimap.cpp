
#include "bimap.h"

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
    
    return 0;
}

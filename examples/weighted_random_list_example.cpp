#include "WeightedRandomList.h" // Assuming this path will be set up by CMake
#include <iostream>
#include <string>
#include <vector>
#include <map>

// A simple helper to print the list contents
template<typename T, typename WeightType>
void print_list_details(const cpp_utils::WeightedRandomList<T, WeightType>& list) {
    if (list.empty()) {
        std::cout << "List is empty." << std::endl;
        return;
    }
    std::cout << "List contents (size: " << list.size() << ", total weight: " << list.total_weight() << "):" << std::endl;
    for (typename cpp_utils::WeightedRandomList<T, WeightType>::size_type i = 0; i < list.size(); ++i) {
        auto entry = list.get_entry(i);
        std::cout << "  Index " << i << ": Value = \"" << entry.first
                  << "\", Weight = " << entry.second << std::endl;
    }
}

int main() {
    std::cout << "--- WeightedRandomList Example ---" << std::endl;

    // 1. Create a WeightedRandomList
    cpp_utils::WeightedRandomList<std::string> wr_list;
    // Or: cpp_utils::WeightedRandomList<std::string, long long> wr_list_explicit;


    // 2. Add elements with weights
    std::cout << "\n--- Adding elements ---" << std::endl;
    wr_list.push_back("apple", 10);
    wr_list.push_back("banana", 20);
    wr_list.push_back("cherry", 70);
    wr_list.push_back("date", 0); // Zero weight, should not be selected

    print_list_details(wr_list);

    // 3. Fetch elements randomly
    std::cout << "\n--- Random selections (10000 draws) ---" << std::endl;
    if (wr_list.empty() || wr_list.total_weight() == 0) {
        std::cout << "Cannot draw randomly, list is empty or has no total weight." << std::endl;
    } else {
        std::map<std::string, int> counts;
        const int num_draws = 10000;
        for (int i = 0; i < num_draws; ++i) {
            auto random_item_opt = wr_list.get_random();
            if (random_item_opt) {
                counts[random_item_opt.value().get()]++;
            }
        }

        for (const auto& pair : counts) {
            double percentage = static_cast<double>(pair.second) / num_draws * 100.0;
            std::cout << "\"" << pair.first << "\": " << pair.second << " times (" << percentage << "%)" << std::endl;
        }
        std::cout << "(Note: 'date' should have 0 draws or not appear if its weight is 0)." << std::endl;
    }

    // 4. Update weights
    std::cout << "\n--- Updating weights ---" << std::endl;
    std::cout << "Updating weight of 'apple' (index 0) from 10 to 50." << std::endl;
    wr_list.update_weight(0, 50); // apple's index is 0

    std::cout << "Updating weight of 'cherry' (index 2) from 70 to 10." << std::endl;
    wr_list.update_weight(2, 10); // cherry's index is 2

    print_list_details(wr_list);

    std::cout << "\n--- Random selections after weight update (10000 draws) ---" << std::endl;
    if (wr_list.empty() || wr_list.total_weight() == 0) {
        std::cout << "Cannot draw randomly, list is empty or has no total weight." << std::endl;
    } else {
        std::map<std::string, int> counts_updated;
        const int num_draws_updated = 10000;
        for (int i = 0; i < num_draws_updated; ++i) {
            auto random_item_opt = wr_list.get_random();
            if (random_item_opt) {
                counts_updated[random_item_opt.value().get()]++;
            }
        }
        for (const auto& pair : counts_updated) {
            double percentage = static_cast<double>(pair.second) / num_draws_updated * 100.0;
            std::cout << "\"" << pair.first << "\": " << pair.second << " times (" << percentage << "%)" << std::endl;
        }
    }

    // 5. Using operator[] and at()
    std::cout << "\n--- Direct access using at() and operator[] ---" << std::endl;
    try {
        std::cout << "Element at index 1 (using at()): " << wr_list.at(1) << std::endl;
        std::cout << "Element at index 0 (using operator[]): " << wr_list[0] << std::endl;
        // Accessing out of bounds
        // std::cout << "Element at index 10 (will throw): " << wr_list.at(10) << std::endl;
    } catch (const std::out_of_range& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }

    // 6. Modifying an element through mutable random access
    std::cout << "\n--- Modifying element via get_random_mut() ---" << std::endl;
    auto item_to_modify_opt = wr_list.get_random_mut();
    if (item_to_modify_opt) {
        std::string& item_ref = item_to_modify_opt.value().get();
        std::cout << "Randomly selected item to modify: " << item_ref << std::endl;
        item_ref = "MODIFIED_" + item_ref;
        std::cout << "Item after modification: " << item_ref << std::endl;
    }
    print_list_details(wr_list);


    // 7. Clearing the list
    std::cout << "\n--- Clearing the list ---" << std::endl;
    wr_list.clear();
    print_list_details(wr_list);
    std::cout << "Is list empty? " << (wr_list.empty() ? "Yes" : "No") << std::endl;

    // 8. Example with zero total weight initially then adding positive weight
    std::cout << "\n--- Example with initial zero total weight ---" << std::endl;
    cpp_utils::WeightedRandomList<int> int_list;
    int_list.push_back(100, 0);
    int_list.push_back(200, 0);
    print_list_details(int_list);
    std::cout << "Attempting random draw (should be nullopt or handled gracefully):" << std::endl;
    auto random_int_opt = int_list.get_random();
    if (!random_int_opt) {
        std::cout << "  Correctly received no item (nullopt)." << std::endl;
    } else {
        std::cout << "  Unexpectedly received item: " << random_int_opt.value().get() << std::endl;
    }

    std::cout << "Adding an item with positive weight:" << std::endl;
    int_list.push_back(300, 5);
    print_list_details(int_list);
    random_int_opt = int_list.get_random();
    if (random_int_opt) {
        std::cout << "  Randomly selected item: " << random_int_opt.value().get() << " (should be 300)" << std::endl;
    }


    std::cout << "\n--- Example End ---" << std::endl;

    return 0;
}

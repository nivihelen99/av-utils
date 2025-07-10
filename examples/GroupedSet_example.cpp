#include "GroupedSet.h" // Assuming GroupedSet.h is in the include path
#include <iostream>
#include <string>
#include <vector>

// Helper function to print a set
template<typename T, typename Compare>
void print_set(const std::set<T, Compare>& s, const std::string& label) {
    std::cout << label << ": { ";
    for (const auto& item : s) {
        std::cout << item << " ";
    }
    std::cout << "}" << std::endl;
}

// Helper function to print a vector
template<typename T>
void print_vector(const std::vector<T>& v, const std::string& label) {
    std::cout << label << ": [ ";
    for (const auto& item : v) {
        std::cout << item << " ";
    }
    std::cout << "]" << std::endl;
}

int main() {
    // Create a GroupedSet with string items and string group IDs
    cpp_collections::GroupedSet<std::string, std::string> asset_manager;

    // Add items
    asset_manager.add_item("Laptop01");
    asset_manager.add_item("Laptop02");
    asset_manager.add_item("Server01");
    asset_manager.add_item("Server02");
    asset_manager.add_item("Desktop01");
    asset_manager.add_item("Switch01"); // Initially ungrouped

    std::cout << "Initial state:" << std::endl;
    print_set(asset_manager.get_all_items(), "All items");
    std::cout << "Total items: " << asset_manager.size() << std::endl;
    std::cout << "Is empty? " << (asset_manager.empty() ? "Yes" : "No") << std::endl;
    std::cout << "Group count: " << asset_manager.group_count() << std::endl;
    std::cout << std::endl;

    // Add items to groups
    std::cout << "Adding items to groups..." << std::endl;
    asset_manager.add_item_to_group("Laptop01", "HR");
    asset_manager.add_item_to_group("Laptop02", "Engineering");
    asset_manager.add_item_to_group("Server01", "Engineering");
    asset_manager.add_item_to_group("Server01", "DataCenterA"); // Server01 in two groups
    asset_manager.add_item_to_group("Server02", "DataCenterB");
    asset_manager.add_item_to_group("Desktop01", "HR");
    asset_manager.add_item_to_group("Desktop01", "Finance"); // Desktop01 in two groups

    // Check item existence
    std::cout << "Item 'Laptop01' exists: " << (asset_manager.item_exists("Laptop01") ? "Yes" : "No") << std::endl;
    std::cout << "Item 'Projector01' exists: " << (asset_manager.item_exists("Projector01") ? "Yes" : "No") << std::endl;

    // Check group existence
    std::cout << "Group 'HR' exists: " << (asset_manager.group_exists("HR") ? "Yes" : "No") << std::endl;
    std::cout << "Group 'Marketing' exists: " << (asset_manager.group_exists("Marketing") ? "Yes" : "No") << std::endl;
    std::cout << std::endl;

    // Querying
    std::cout << "Querying groups and items:" << std::endl;
    print_vector(asset_manager.get_all_groups(), "All groups");
    print_set(asset_manager.get_items_in_group("HR"), "Items in HR");
    print_set(asset_manager.get_items_in_group("Engineering"), "Items in Engineering");
    print_set(asset_manager.get_items_in_group("DataCenterA"), "Items in DataCenterA");
    print_set(asset_manager.get_items_in_group("Marketing"), "Items in Marketing (non-existent)");

    print_set(asset_manager.get_groups_for_item("Server01"), "Groups for Server01");
    print_set(asset_manager.get_groups_for_item("Laptop02"), "Groups for Laptop02");
    print_set(asset_manager.get_groups_for_item("Switch01"), "Groups for Switch01 (ungrouped)");

    std::cout << "Is 'Laptop01' in 'HR'? " << (asset_manager.is_item_in_group("Laptop01", "HR") ? "Yes" : "No") << std::endl;
    std::cout << "Is 'Laptop01' in 'Engineering'? " << (asset_manager.is_item_in_group("Laptop01", "Engineering") ? "Yes" : "No") << std::endl;
    std::cout << std::endl;

    // Counts
    std::cout << "Counts:" << std::endl;
    std::cout << "Total items: " << asset_manager.size() << std::endl;
    std::cout << "Group count: " << asset_manager.group_count() << std::endl;
    std::cout << "Items in 'HR' count: " << asset_manager.items_in_group_count("HR") << std::endl;
    std::cout << "Groups for 'Server01' count: " << asset_manager.groups_for_item_count("Server01") << std::endl;
    std::cout << std::endl;

    // Advanced queries
    std::cout << "Advanced queries:" << std::endl;
    std::vector<std::string> eng_dc_groups = {"Engineering", "DataCenterA"};
    print_set(asset_manager.get_items_in_all_groups(eng_dc_groups), "Items in ALL (Engineering, DataCenterA)");

    std::vector<std::string> hr_fin_groups = {"HR", "Finance"};
    print_set(asset_manager.get_items_in_all_groups(hr_fin_groups), "Items in ALL (HR, Finance)");

    std::vector<std::string> any_hr_eng = {"HR", "Engineering"};
    print_set(asset_manager.get_items_in_any_group(any_hr_eng), "Items in ANY (HR, Engineering)");

    print_set(asset_manager.get_ungrouped_items(), "Ungrouped items");
    std::cout << std::endl;

    // Removals
    std::cout << "Demonstrating removals:" << std::endl;
    std::cout << "Removing 'Laptop01' from 'HR'..." << std::endl;
    asset_manager.remove_item_from_group("Laptop01", "HR");
    print_set(asset_manager.get_items_in_group("HR"), "Items in HR after removing Laptop01");
    print_set(asset_manager.get_groups_for_item("Laptop01"), "Groups for Laptop01 after removing from HR");
    print_set(asset_manager.get_ungrouped_items(), "Ungrouped items after Laptop01 removed from HR"); // Laptop01 should now be ungrouped
    std::cout << std::endl;

    std::cout << "Removing 'Server01' (item) completely..." << std::endl;
    asset_manager.remove_item("Server01");
    print_set(asset_manager.get_all_items(), "All items after removing Server01");
    print_set(asset_manager.get_items_in_group("Engineering"), "Items in Engineering after removing Server01");
    print_set(asset_manager.get_items_in_group("DataCenterA"), "Items in DataCenterA after removing Server01");
    std::cout << "Item 'Server01' exists: " << (asset_manager.item_exists("Server01") ? "Yes" : "No") << std::endl;
    std::cout << std::endl;

    std::cout << "Removing 'Finance' (group) completely..." << std::endl;
    asset_manager.remove_group("Finance");
    print_vector(asset_manager.get_all_groups(), "All groups after removing Finance");
    print_set(asset_manager.get_groups_for_item("Desktop01"), "Groups for Desktop01 after removing Finance group");
    std::cout << "Group 'Finance' exists: " << (asset_manager.group_exists("Finance") ? "Yes" : "No") << std::endl;
    std::cout << std::endl;

    // Clear everything
    std::cout << "Clearing the GroupedSet..." << std::endl;
    asset_manager.clear();
    std::cout << "Total items after clear: " << asset_manager.size() << std::endl;
    std::cout << "Is empty after clear? " << (asset_manager.empty() ? "Yes" : "No") << std::endl;
    print_vector(asset_manager.get_all_groups(), "All groups after clear");

    return 0;
}

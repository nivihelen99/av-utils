#include "weighted_set.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip> // For std::fixed and std::setprecision

void print_line() {
    std::cout << "----------------------------------------\n";
}

int main() {
    std::cout << "WeightedSet Example\n";
    print_line();

    // Create a WeightedSet with strings as keys and doubles as weights
    cpp_collections::WeightedSet<std::string, double> loot_table;

    // Add items with weights
    std::cout << "Adding items to the loot table...\n";
    loot_table.add("Sword", 10.0);
    loot_table.add("Shield", 10.0);
    loot_table.add("Potion", 25.0);
    loot_table.add("Gold Coin", 50.0);
    loot_table.add("Rare Gem", 5.0);

    std::cout << "Initial loot table (size: " << loot_table.size() << ", total weight: " << loot_table.total_weight() << "):\n";
    for (const auto& item : loot_table) {
        std::cout << "  Item: " << std::setw(10) << std::left << item.first
                  << " Weight: " << item.second << "\n";
    }
    print_line();

    // Demonstrate get_weight and contains
    std::cout << "Weight of Potion: " << loot_table.get_weight("Potion") << "\n";
    std::cout << "Weight of Dagger (not present): " << loot_table.get_weight("Dagger") << "\n";
    std::cout << "Contains Shield? " << std::boolalpha << loot_table.contains("Shield") << "\n";
    std::cout << "Contains Armor? " << std::boolalpha << loot_table.contains("Armor") << "\n";
    print_line();

    // Demonstrate updating an item's weight
    std::cout << "Updating weight of Gold Coin to 60.0...\n";
    loot_table.add("Gold Coin", 60.0);
    std::cout << "Weight of Gold Coin after update: " << loot_table.get_weight("Gold Coin") << "\n";
    std::cout << "Total weight after update: " << loot_table.total_weight() << "\n";
    print_line();

    // Demonstrate removing an item
    std::cout << "Removing Shield...\n";
    loot_table.remove("Shield");
    std::cout << "Contains Shield after removal? " << std::boolalpha << loot_table.contains("Shield") << "\n";
    std::cout << "Loot table size after removal: " << loot_table.size() << "\n";
    std::cout << "Total weight after removal: " << loot_table.total_weight() << "\n";
    print_line();

    // Demonstrate adding an item with zero weight (should effectively not add or remove if present)
    std::cout << "Trying to add 'Scroll' with 0.0 weight...\n";
    loot_table.add("Scroll", 0.0);
    std::cout << "Contains Scroll? " << std::boolalpha << loot_table.contains("Scroll") << "\n";
    std::cout << "Size: " << loot_table.size() << "\n";
    print_line();

    // Demonstrate sampling
    std::cout << "Sampling 10 items from the loot table:\n";
    if (loot_table.empty() || loot_table.total_weight() <= 0) {
        std::cout << "Cannot sample, loot table is effectively empty.\n";
    } else {
        std::map<std::string, int> sample_counts;
        const int num_samples = 100000; // Increased samples for better statistical view
        std::cout << "Performing " << num_samples << " samples to see distribution...\n";
        for (int i = 0; i < num_samples; ++i) {
            try {
                sample_counts[loot_table.sample()]++;
            } catch (const std::out_of_range& e) {
                std::cerr << "Error during sampling: " << e.what() << std::endl;
                break;
            }
        }

        std::cout << "\nSampled item counts (out of " << num_samples << " samples):\n";
        for (const auto& pair : sample_counts) {
            double percentage = static_cast<double>(pair.second) / num_samples * 100.0;
            std::cout << "  Item: " << std::setw(10) << std::left << pair.first
                      << " Count: " << std::setw(7) << std::right << pair.second
                      << " (" << std::fixed << std::setprecision(2) << percentage << "%)\n";
        }

        std::cout << "\nExpected distribution based on current weights:\n";
        double current_total_weight = loot_table.total_weight();
        if (current_total_weight > 0) {
            for(const auto& item : loot_table) {
                double expected_percentage = (item.second / current_total_weight) * 100.0;
                 std::cout << "  Item: " << std::setw(10) << std::left << item.first
                           << " Expected: (" << std::fixed << std::setprecision(2) << expected_percentage << "%)\n";
            }
        }
    }
    print_line();

    // Test with integer keys and weights
    cpp_collections::WeightedSet<int, int> number_set;
    number_set.add(1, 10);
    number_set.add(2, 20);
    number_set.add(3, 70);

    std::cout << "Number set (total weight: " << number_set.total_weight() << "):\n";
     for (const auto& item : number_set) {
        std::cout << "  Item: " << item.first << " Weight: " << item.second << "\n";
    }
    std::cout << "Sampling 5 numbers:\n";
    if(!number_set.empty() && number_set.total_weight() > 0) {
        for (int i = 0; i < 5; ++i) {
            std::cout << "  Sampled: " << number_set.sample() << "\n";
        }
    } else {
        std::cout << "Cannot sample from number_set.\n";
    }
    print_line();

    // Test construction with initializer list
    cpp_collections::WeightedSet<char, unsigned int> char_set = {
        {'a', 100}, {'b', 50}, {'c', 25}, {'d', 5}
    };
    std::cout << "Character set from initializer list (total weight: " << char_set.total_weight() << "):\n";
    for (const auto& item : char_set) {
        std::cout << "  Char: " << item.first << " Weight: " << item.second << "\n";
    }
     if(!char_set.empty() && char_set.total_weight() > 0) {
        std::cout << "Sampled char: " << char_set.sample() << "\n";
    }
    print_line();

    std::cout << "Example finished.\n";
    return 0;
}

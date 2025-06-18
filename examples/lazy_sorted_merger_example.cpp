#include <iostream>
#include <vector>
#include <string>
#include <algorithm> // For std::sort with custom comparators if needed for setup
#include "../include/lazy_sorted_merger.h" // Adjust path if necessary

// Example 1: Merging three sorted vectors of integers
void example_merge_integers() {
    std::cout << "Example 1: Merging three sorted vectors of integers" << std::endl;
    std::vector<int> vec1 = {1, 5, 10, 15};
    std::vector<int> vec2 = {2, 6, 11, 16};
    std::vector<int> vec3 = {3, 7, 12, 17};

    std::vector<std::pair<std::vector<int>::const_iterator, std::vector<int>::const_iterator>> sources;
    sources.push_back({vec1.begin(), vec1.end()});
    sources.push_back({vec2.begin(), vec2.end()});
    sources.push_back({vec3.begin(), vec3.end()});

    auto merger = lazy_merge(sources);

    std::cout << "Merged integers: ";
    while (merger.hasNext()) {
        if (auto val = merger.next()) {
            std::cout << *val << " ";
        }
    }
    std::cout << std::endl << std::endl;
}

// Example 2: Merging with a custom comparator (descending order)
void example_merge_custom_comparator() {
    std::cout << "Example 2: Merging with a custom comparator (descending order)" << std::endl;
    std::vector<int> vec_a = {15, 10, 5, 1}; // Sorted descending
    std::vector<int> vec_b = {16, 11, 6, 2}; // Sorted descending

    std::vector<std::pair<std::vector<int>::const_iterator, std::vector<int>::const_iterator>> sources;
    sources.push_back({vec_a.begin(), vec_a.end()});
    sources.push_back({vec_b.begin(), vec_b.end()});

    auto merger = lazy_merge(sources, std::greater<int>());

    std::cout << "Merged integers (descending): ";
    while (merger.hasNext()) {
        if (auto val = merger.next()) {
            std::cout << *val << " ";
        }
    }
    std::cout << std::endl << std::endl;
}

// Example 3: Merging with a custom struct
struct Product {
    std::string name;
    double price;

    // For easy printing
    friend std::ostream& operator<<(std::ostream& os, const Product& p) {
        os << "Product{name=\"" << p.name << "\", price=" << p.price << "}";
        return os;
    }
};

// Custom comparator for Product based on price (ascending)
struct ProductPriceComparator {
    bool operator()(const Product& a, const Product& b) const {
        return a.price < b.price;
    }
};

void example_merge_custom_struct() {
    std::cout << "Example 3: Merging with a custom struct (Product by price)" << std::endl;
    std::vector<Product> catalog1 = {
        {"Apple", 0.5}, {"Banana", 0.25}, {"Orange", 0.75}
    };
    std::vector<Product> catalog2 = {
        {"Milk", 1.5}, {"Bread", 1.25}, {"Butter", 2.75}
    };
    std::vector<Product> catalog3 = {
        {"Grape", 1.0}, {"Pineapple", 2.0}
    };

    // Ensure catalogs are sorted by price for the comparator
    std::sort(catalog1.begin(), catalog1.end(), ProductPriceComparator());
    std::sort(catalog2.begin(), catalog2.end(), ProductPriceComparator());
    std::sort(catalog3.begin(), catalog3.end(), ProductPriceComparator());


    std::vector<std::pair<std::vector<Product>::const_iterator, std::vector<Product>::const_iterator>> sources;
    sources.push_back({catalog1.begin(), catalog1.end()});
    sources.push_back({catalog2.begin(), catalog2.end()});
    sources.push_back({catalog3.begin(), catalog3.end()});

    auto merger = lazy_merge(sources, ProductPriceComparator());

    std::cout << "Merged Products (by price):" << std::endl;
    while (merger.hasNext()) {
        if (auto val = merger.next()) {
            std::cout << "  " << *val << std::endl;
        }
    }
    std::cout << std::endl;
}


int main() {
    example_merge_integers();
    example_merge_custom_comparator();
    example_merge_custom_struct();

    return 0;
}

#include "ribbon_filter.h"
#include <iostream>
#include <vector>
#include <string>

int main() {
    // 1. Create a filter, specifying the expected number of items.
    size_t expected_item_count = 1000;
    RibbonFilter<std::string> filter(expected_item_count);

    // 2. Add items. These are stored temporarily.
    std::vector<std::string> my_items = {"apple", "banana", "cherry", "date"};
    for (const auto& item : my_items) {
        filter.add(item);
    }
    // ... add more items ...
    filter.add("elderberry");
    filter.add("fig");
    filter.add("grapefruit");


    // 3. Build the filter. This is a crucial step.
    bool build_ok = filter.build();
    if (!build_ok) {
        std::cerr << "Filter construction failed! This can happen if the filter is too full "
                  << "or due to unlucky hash collisions making the item graph unpeelable." << std::endl;
        return 1;
    }
    std::cout << "Filter built successfully. Items in filter: " << filter.size() << std::endl;
    std::cout << "Filter capacity (slots): " << filter.capacity_slots() << std::endl;
    std::cout << "Filter is built: " << (filter.is_built() ? "Yes" : "No") << std::endl;


    // 4. Check for membership.
    std::cout << "Contains 'apple'? " << (filter.might_contain("apple") ? "Yes" : "No") << std::endl; // Expected: Yes
    std::cout << "Contains 'banana'? " << (filter.might_contain("banana") ? "Yes" : "No") << std::endl; // Expected: Yes
    std::cout << "Contains 'cherry'? " << (filter.might_contain("cherry") ? "Yes" : "No") << std::endl; // Expected: Yes
    std::cout << "Contains 'date'? " << (filter.might_contain("date") ? "Yes" : "No") << std::endl; // Expected: Yes
    std::cout << "Contains 'elderberry'? " << (filter.might_contain("elderberry") ? "Yes" : "No") << std::endl; // Expected: Yes
    std::cout << "Contains 'fig'? " << (filter.might_contain("fig") ? "Yes" : "No") << std::endl; // Expected: Yes
    std::cout << "Contains 'grapefruit'? " << (filter.might_contain("grapefruit") ? "Yes" : "No") << std::endl; // Expected: Yes

    std::cout << "Contains 'grape'? " << (filter.might_contain("grape") ? "Yes (False Positive?)" : "No") << std::endl; // Expected: No (or Yes if FP)
    std::cout << "Contains 'honeydew'? " << (filter.might_contain("honeydew") ? "Yes (False Positive?)" : "No") << std::endl; // Expected: No (or Yes if FP)
    std::cout << "Contains 'kiwi'? " << (filter.might_contain("kiwi") ? "Yes (False Positive?)" : "No") << std::endl; // Expected: No (or Yes if FP)


    // Example with integers and custom fingerprint type (uint32_t)
    RibbonFilter<int, uint32_t> int_filter(500);
    for (int i = 0; i < 500; ++i) {
        int_filter.add(i * 10);
    }

    std::cout << "\nBuilding integer filter..." << std::endl;
    if (int_filter.build()) {
        std::cout << "Int filter built successfully. Items: " << int_filter.size() << std::endl;
        std::cout << "Int filter capacity (slots): " << int_filter.capacity_slots() << std::endl;
        std::cout << "Int filter is built: " << (int_filter.is_built() ? "Yes" : "No") << std::endl;

        std::cout << "Int filter contains 100? " << (int_filter.might_contain(100) ? "Yes" : "No") << std::endl; // Expected: Yes (10 * 10)
        std::cout << "Int filter contains 4990? " << (int_filter.might_contain(4990) ? "Yes" : "No") << std::endl; // Expected: Yes (499 * 10)
        std::cout << "Int filter contains 101? " << (int_filter.might_contain(101) ? "Yes (FP?)" : "No") << std::endl; // Expected: No (or Yes if FP)
        std::cout << "Int filter contains 5000? " << (int_filter.might_contain(5000) ? "Yes (FP?)" : "No") << std::endl; // Expected: No (or Yes if FP)
    } else {
        std::cerr << "Integer filter construction failed!" << std::endl;
    }

    // Example with const char*
    RibbonFilter<const char*> cstr_filter(10);
    cstr_filter.add("hello");
    cstr_filter.add("world");

    std::cout << "\nBuilding const char* filter..." << std::endl;
    if (cstr_filter.build()) {
        std::cout << "const char* filter built successfully. Items: " << cstr_filter.size() << std::endl;
        std::cout << "const char* filter contains 'hello'? " << (cstr_filter.might_contain("hello") ? "Yes" : "No") << std::endl;
        std::cout << "const char* filter contains 'test'? " << (cstr_filter.might_contain("test") ? "Yes (FP?)" : "No") << std::endl;
    } else {
        std::cerr << "const char* filter construction failed!" << std::endl;
    }

    // Example of a filter that is expected to fail building (too many items for capacity)
    // Sized for 10, trying to add 100. Should almost certainly fail.
    std::cout << "\nTesting build failure scenario..." << std::endl;
    RibbonFilter<int> fail_filter(10);
    for(int i=0; i<100; ++i) {
        fail_filter.add(i);
    }
    if (!fail_filter.build()) {
        std::cout << "Build failure test: Filter construction failed as expected." << std::endl;
        std::cout << "Build failure test: Is built? " << (fail_filter.is_built() ? "Yes" : "No") << std::endl;
        std::cout << "Build failure test: Size? " << fail_filter.size() << std::endl; // Should be 0
        std::cout << "Build failure test: might_contain(5)? " << (fail_filter.might_contain(5) ? "Yes" : "No") << std::endl; // Should be No
    } else {
        std::cerr << "Build failure test: Filter construction unexpectedly succeeded." << std::endl;
    }

    // Test adding after build
    std::cout << "\nTesting add after build..." << std::endl;
    RibbonFilter<std::string> add_after_build_filter(5);
    add_after_build_filter.add("item1");
    add_after_build_filter.build();
    try {
        add_after_build_filter.add("item2");
        std::cerr << "Error: Adding item after build did not throw an exception." << std::endl;
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected exception when adding after build: " << e.what() << std::endl;
    }


    return 0;
}

#include "chain_map.hpp" // Assuming chain_map.hpp is in the same directory or include path
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <stdexcept> // For std::out_of_range

// Helper to print test case names
void print_test_name(const std::string& name) {
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Running test: " << name << std::endl;
    std::cout << "----------------------------------------" << std::endl;
}

// Test functions will be added here later.
// For example:
// void test_default_constructor() {
//     print_test_name("Default Constructor");
//     ChainMap<std::string, int> cm;
//     assert(cm.empty()); // This needs ChainMap::empty() to be implemented
//     assert(cm.size() == 0); // This needs ChainMap::size() for unique keys
//     // assert(cm.get_maps().empty()); // This needs ChainMap::get_maps()
//     std::cout << "Default Constructor tests passed." << std::endl;
// }

void test_default_constructor() {
    print_test_name("Default Constructor");
    ChainMap<std::string, int> cm;
    assert(cm.empty()); // Relies on ChainMap::empty()
    assert(cm.size() == 0); // Relies on ChainMap::size() for unique keys
    assert(cm.get_maps().empty()); // Relies on ChainMap::get_maps()

    // Test const versions
    const ChainMap<std::string, int> const_cm;
    assert(const_cm.empty());
    assert(const_cm.size() == 0);
    assert(const_cm.get_maps().empty());

    std::cout << "Default Constructor tests passed." << std::endl;
}

void test_single_map_constructor() {
    print_test_name("Single Map Constructor");
    std::map<std::string, int> m1 = {{"a", 1}, {"b", 2}};
    ChainMap<std::string, int, std::map<std::string, int>> cm(&m1);

    assert(!cm.empty());
    assert(cm.size() == 2);
    assert(cm.get_maps().size() == 1);
    assert(cm.get_maps()[0] == &m1);
    assert(cm.contains("a"));
    assert(cm.contains("b"));
    assert(!cm.contains("c"));
    assert(cm.at("a") == 1);

    // Test with nullptr
    std::map<std::string, int>* null_map_ptr = nullptr;
    ChainMap<std::string, int, std::map<std::string, int>> cm_null(null_map_ptr);
    assert(cm_null.empty()); // Constructor should skip nullptrs
    assert(cm_null.size() == 0);
    assert(cm_null.get_maps().empty());

    std::cout << "Single Map Constructor tests passed." << std::endl;
}

void test_initializer_list_constructor() {
    print_test_name("Initializer List Constructor");
    std::unordered_map<int, std::string> map1 = {{1, "one"}, {2, "two"}};
    std::unordered_map<int, std::string> map2 = {{3, "three"}, {2, "deux"}}; // "deux" for key 2 will be hidden
    std::unordered_map<int, std::string>* map3_ptr = nullptr;

    ChainMap<int, std::string> cm = {&map1, &map2, map3_ptr}; // map3_ptr should be skipped

    assert(!cm.empty());
    assert(cm.get_maps().size() == 2); // map1, map2
    assert(cm.size() == 3); // Unique keys: 1, 2, 3
    assert(cm.contains(1));
    assert(cm.contains(2));
    assert(cm.contains(3));
    assert(!cm.contains(4));
    assert(cm.at(1) == "one");
    assert(cm.at(2) == "two"); // From map1 (higher priority)
    assert(cm.at(3) == "three");

    ChainMap<int, std::string> cm_empty_list({});
    assert(cm_empty_list.empty());
    assert(cm_empty_list.size() == 0);

    std::cout << "Initializer List Constructor tests passed." << std::endl;
}

void test_variadic_constructor() {
    print_test_name("Variadic Constructor");
    std::map<std::string, double> user_prefs = {{"timeout", 10.5}, {"retries", 3.0}};
    std::map<std::string, double> system_defaults = {{"timeout", 5.0}, {"buffer_size", 4096.0}, {"retries", 5.0}};

    ChainMap<std::string, double, std::map<std::string, double>> config(user_prefs, system_defaults);

    assert(!config.empty());
    assert(config.get_maps().size() == 2);
    assert(config.get_maps()[0] == &user_prefs);
    assert(config.get_maps()[1] == &system_defaults);

    assert(config.size() == 3); // "timeout", "retries", "buffer_size"
    assert(config.contains("timeout"));
    assert(config.contains("retries"));
    assert(config.contains("buffer_size"));
    assert(!config.contains("non_existent"));

    assert(config.at("timeout") == 10.5); // From user_prefs
    assert(config.at("retries") == 3.0);   // From user_prefs
    assert(config.at("buffer_size") == 4096.0); // From system_defaults

    // Test const version
    const ChainMap<std::string, double, std::map<std::string, double>> const_config(user_prefs, system_defaults);
    assert(const_config.at("timeout") == 10.5);


    // Test with a single map
    ChainMap<std::string, double, std::map<std::string, double>> single_map_config(user_prefs);
    assert(single_map_config.get_maps().size() == 1);
    assert(single_map_config.at("timeout") == 10.5);
    assert(single_map_config.size() == 2);


    std::cout << "Variadic Constructor tests passed." << std::endl;
}

void test_access_and_lookup() {
    print_test_name("Access and Lookup (at, get, [], contains)");
    std::map<std::string, int> m1 = {{"apple", 10}, {"banana", 20}};
    std::map<std::string, int> m2 = {{"banana", 200}, {"cherry", 30}}; // "banana" in m1 takes precedence
    std::map<std::string, int> m3 = {}; // Empty map for insertion tests

    ChainMap<std::string, int, std::map<std::string, int>> cm(&m3, &m1, &m2);

    // Test contains()
    assert(cm.contains("apple"));
    assert(cm.contains("banana"));
    assert(cm.contains("cherry"));
    assert(!cm.contains("date"));

    // Test at() and get() (const access)
    assert(cm.at("apple") == 10);   // From m1 (via m3)
    assert(cm.get("apple") == 10);
    assert(cm.at("banana") == 20);  // From m1 (via m3) - m3 is first, then m1
    assert(cm.get("banana") == 20);
    assert(cm.at("cherry") == 30);  // From m2
    assert(cm.get("cherry") == 30);

    // Test at() for missing key
    bool caught_out_of_range = false;
    try {
        cm.at("date");
    } catch (const std::out_of_range& e) {
        caught_out_of_range = true;
    }
    assert(caught_out_of_range);

    caught_out_of_range = false;
    try {
        cm.get("date");
    } catch (const std::out_of_range& e) {
        caught_out_of_range = true;
    }
    assert(caught_out_of_range);

    // Test non-const at()
    cm.at("apple") = 15; // Modifies m1's "apple" through m3's view if m3 doesn't have it
                         // This is tricky: at() modifies in-place. If "apple" was only in m1, m1 is modified.
                         // Since m3 is the first map, if "apple" is not in m3, at() should find it in m1.
    assert(m1.at("apple") == 15); // m1 should be modified
    assert(cm.at("apple") == 15);
    assert(m3.count("apple") == 0); // at() does not insert into the first map if found elsewhere.

    // Test operator[] for access
    assert(cm["apple"] == 15); // From m1
    assert(cm["cherry"] == 30); // From m2

    // Test operator[] for insertion
    cm["date"] = 40; // Should insert into m3 (the first map)
    assert(m3.count("date") == 1);
    assert(m3.at("date") == 40);
    assert(cm.contains("date"));
    assert(cm["date"] == 40);
    assert(cm.size() == 4); // apple, banana, cherry, date

    // Test operator[] when key exists in a later map but not first
    // cm["banana"] is 20 (from m1). If we assign cm["banana"] = 25
    // it should modify m3.
    cm["banana"] = 25;
    assert(m3.count("banana") == 1);
    assert(m3.at("banana") == 25);
    assert(m1.at("banana") == 20); // m1 remains unchanged if operator[] promotes/writes to first map
    assert(cm["banana"] == 25); // Should now return from m3
    assert(cm.at("banana")==25); // at should also find it in m3 now.

    // Test operator[] on a const ChainMap (should not compile if non-const operator[] is only one)
    // To test this, we'd need a const ChainMap.
    // const auto& const_cm = cm;
    // assert(const_cm.at("apple") == 15); // OK: const at()
    // assert(const_cm.get("apple") == 15); // OK: get() is const
    // Value an_apple = const_cm["apple"]; // This line should fail to compile if there's no const operator[]
    // For now, we assume ChainMap's operator[] is primarily for non-const "upsert" behavior.
    // Standard map operator[] is non-const.
    // If a const operator[] is desired, it would typically behave like at().

    std::cout << "Access and Lookup tests passed." << std::endl;
}

void test_modification_operations() {
    print_test_name("Modification Operations (insert, erase, clear, layers)");
    std::map<std::string, int> m1 = {{"a", 1}, {"b", 2}};
    std::map<std::string, int> m2 = {{"b", 20}, {"c", 3}};
    std::map<std::string, int> m_writable;

    ChainMap<std::string, int, std::map<std::string, int>> cm(&m_writable, &m1);

    // Test insert() - should insert into m_writable
    cm.insert("d", 4);
    assert(m_writable.count("d") == 1 && m_writable.at("d") == 4);
    assert(cm.contains("d") && cm.at("d") == 4);
    assert(cm.size() == 3); // m_writable={d:4}, m1={a:1, b:2}. Unique: a,b,d.
    assert(m1.count("d") == 0); // m1 not affected

    // Update existing key (in m_writable)
    cm.insert("d", 40);
    assert(m_writable.at("d") == 40 && cm.at("d") == 40);

    // Insert a key that exists in m1 - should add to m_writable and shadow m1's "a"
    cm.insert("a", 10);
    assert(m_writable.count("a") == 1 && m_writable.at("a") == 10);
    assert(m1.at("a") == 1); // m1 not affected
    assert(cm.at("a") == 10); // Now sees m_writable's "a"
    assert(cm.size() == 3); // Still a, b, d (unique keys)

    // Test erase() - should only erase from m_writable
    size_t erased_count = cm.erase("a"); // Erase "a" from m_writable
    assert(erased_count == 1);
    assert(m_writable.count("a") == 0);
    assert(cm.contains("a")); // Still sees "a" from m1
    assert(cm.at("a") == 1);   // Sees m1's "a" again
    assert(cm.size() == 3); // a (from m1), b, d.

    erased_count = cm.erase("b"); // "b" is not in m_writable
    assert(erased_count == 0);    // Nothing erased from m_writable
    assert(cm.contains("b"));     // "b" from m1 still there
    assert(cm.at("b") == 2);

    erased_count = cm.erase("non_existent");
    assert(erased_count == 0);

    // Test prepend_layer()
    cm.prepend_layer(&m2); // m2, m_writable, m1
    assert(cm.get_maps().size() == 3);
    assert(cm.get_maps()[0] == &m2);
    assert(cm.at("c") == 3);  // From m2
    assert(cm.at("b") == 20); // From m2 (shadows m1's and m_writable's if any)
    assert(cm.at("a") == 1);  // From m1 (as m_writable doesn't have "a", m2 doesn't have "a")
    assert(cm.size() == 4); // a,b,c,d (d from m_writable, a from m1, b,c from m2)
                            // m_writable: {d:40}, m1: {a:1, b:2}, m2: {b:20, c:3}
                            // Visible: c (m2), b (m2), d (m_writable), a (m1)

    // Test add_layer()
    std::map<std::string, int> m_lowest = {{"z", 100}, {"a", 1000}};
    cm.add_layer(&m_lowest); // m2, m_writable, m1, m_lowest
    assert(cm.get_maps().size() == 4);
    assert(cm.get_maps()[3] == &m_lowest);
    assert(cm.contains("z"));
    assert(cm.at("z") == 100);
    assert(cm.at("a") == 1); // Still sees m1's "a" as m2, m_writable don't have it.
    assert(cm.size() == 5); // a,b,c,d,z

    // Test clear()
    cm.clear();
    assert(cm.empty());
    assert(cm.get_maps().empty());
    assert(cm.size() == 0);
    assert(!cm.contains("a"));

    // Test operations on an empty ChainMap after clear
    bool caught_error = false;
    try {
        cm.at("a");
    } catch (const std::out_of_range&) {
        caught_error = true;
    }
    assert(caught_error);

    caught_error = false;
    try {
        cm.insert("x",1); // get_writable_map will throw
    } catch (const std::logic_error&) {
        caught_error = true;
    }
    assert(caught_error);

    std::cout << "Modification Operations tests passed." << std::endl;
}

void test_iteration_and_views() {
    print_test_name("Iteration (begin, end) and Views (keys, values, items)");
    std::map<std::string, int> m1 = {{"apple", 10}, {"banana", 20}};
    std::map<std::string, int> m2 = {{"cherry", 30}, {"banana", 200}}; // banana in m1 hides this one
    std::map<std::string, int> m_empty = {};
    std::map<std::string, int>* m_null = nullptr;

    ChainMap<std::string, int, std::map<std::string, int>> cm(&m1, &m_empty, &m2, m_null);
    // Expected iteration order: apple (m1), banana (m1), cherry (m2)

    std::vector<std::string> expected_keys = {"apple", "banana", "cherry"};
    std::vector<int> expected_values = {10, 20, 30};
    std::vector<std::pair<const std::string, int>> expected_items = {
        {"apple", 10}, {"banana", 20}, {"cherry", 30}
    };

    // Test iterator (range-based for loop)
    std::vector<std::pair<const std::string, int>> iterated_items;
    for (const auto& item : cm) {
        iterated_items.push_back(item);
    }
    assert(iterated_items.size() == 3);
    assert(iterated_items == expected_items);

    // Test const_iterator (range-based for loop on const ChainMap)
    const auto& const_cm = cm;
    std::vector<std::pair<const std::string, int>> const_iterated_items;
    for (const auto& item : const_cm) {
        const_iterated_items.push_back(item);
    }
    assert(const_iterated_items.size() == 3);
    assert(const_iterated_items == expected_items);

    // Test keys()
    std::vector<std::string> keys_vec = cm.keys();
    assert(keys_vec == expected_keys);

    // Test values()
    std::vector<int> values_vec = cm.values();
    assert(values_vec == expected_values);

    // Test items()
    std::vector<std::pair<const std::string, int>> items_vec = cm.items();
    assert(items_vec.size() == expected_items.size());
    for(size_t i=0; i < items_vec.size(); ++i) {
        assert(items_vec[i].first == expected_items[i].first);
        assert(items_vec[i].second == expected_items[i].second);
    }


    // Test with an empty ChainMap
    ChainMap<std::string, int, std::map<std::string, int>> empty_cm;
    assert(empty_cm.begin() == empty_cm.end());
    assert(empty_cm.keys().empty());
    assert(empty_cm.values().empty());
    assert(empty_cm.items().empty());
    int count = 0;
    for (const auto& item : empty_cm) { count++; (void)item; } // Suppress unused warning
    assert(count == 0);

    // Test ChainMap with only null maps initially
    ChainMap<std::string, int, std::map<std::string, int>> cm_all_null(m_null, m_null);
    assert(cm_all_null.begin() == cm_all_null.end());
    assert(cm_all_null.keys().empty());

    // Test iterators after modification
    std::map<std::string, int> mod_m1 = {{"x", 100}};
    std::map<std::string, int> mod_m2 = {{"y", 200}};
    ChainMap<std::string, int, std::map<std::string, int>> mod_cm(&mod_m1);
    mod_cm.add_layer(&mod_m2); // mod_m1, mod_m2

    std::vector<std::pair<const std::string, int>> mod_expected_items = {{"x",100}, {"y",200}};
    iterated_items.clear();
    for(const auto& item : mod_cm) {
        iterated_items.push_back(item);
    }
    assert(iterated_items == mod_expected_items);


    std::cout << "Iteration and Views tests passed." << std::endl;
}

void test_requirements_example() {
    print_test_name("Requirements Example Usage");

    std::unordered_map<std::string, std::string> user = {{"theme", "dark"}};
    std::unordered_map<std::string, std::string> system_cfg = {{"theme", "light"}, {"lang", "en"}}; // Renamed
    std::unordered_map<std::string, std::string> defaults = {{"theme", "default"}, {"lang", "en"}, {"timezone", "UTC"}};

    // Using the variadic constructor which takes references
    // Default MapType is std::unordered_map<std::string, std::string>
    ChainMap<std::string, std::string> config(user, system_cfg, defaults);

    // Initial lookups
    assert(config.get("theme") == "dark");
    assert(config.get("lang") == "en");     // From system_cfg
    assert(config.get("timezone") == "UTC"); // From defaults
    assert(config.contains("theme"));

    // Test throwing for a missing key with get
    bool caught_ex = false;
    try {
        config.get("nonexistent_key");
    } catch (const std::out_of_range&) {
        caught_ex = true;
    }
    assert(caught_ex);

    // Insert into the first map (user)
    config.insert("lang", "fr");
    assert(user.count("lang") == 1 && user.at("lang") == "fr");
    assert(config.get("lang") == "fr"); // Now from user map

    // Verify other maps are not affected by insert
    assert(system_cfg.count("lang") == 1 && system_cfg.at("lang") == "en");
    assert(defaults.count("lang") == 1 && defaults.at("lang") == "en");


    // Test iteration and expected items after modification
    // Expected order due to ChainMap iteration logic (user map first, then system_cfg, then defaults, skipping visited keys)
    std::vector<std::pair<const std::string, std::string>> expected_items;
    // Keys from user map (order within map itself might vary for unordered_map, but ChainMap processes map by map)
    // Assuming "theme" comes before "lang" in user map's internal order for this test, or vice-versa.
    // The important part is that all of user's unique keys come first.
    if (user.find("theme")->second == config.at("theme")) { // Check if theme from user is visible
         expected_items.push_back({"theme", "dark"});
         expected_items.push_back({"lang", "fr"});
    } else { // lang from user is visible
         expected_items.push_back({"lang", "fr"});
         expected_items.push_back({"theme", "dark"});
    }
    // Then unique keys from system_cfg (none in this case as "theme" and "lang" are in user)
    // Then unique keys from defaults
    expected_items.push_back({"timezone", "UTC"});


    std::vector<std::pair<const std::string, std::string>> actual_items;
    for (const auto& item : config) {
        actual_items.push_back(item);
    }

    assert(actual_items.size() == expected_items.size());

    // To handle varying order from unordered_map within each layer, convert to sets for comparison
    // or sort them if keys are comparable and order is deterministic from ChainMap logic.
    // Given ChainMap's iterator processes map by map, and for std::unordered_map elements are unordered,
    // the exact order of items from *within* the same map layer isn't guaranteed.
    // However, the example `expected_items` implies a fixed order.
    // The current ChainMap iterator yields all items from the first map, then all *new* items from the second, etc.
    // The order of items *within* `user` map is not fixed.
    // So, for `expected_items`, `{"theme", "dark"}` and `{"lang", "fr"}` could be swapped.

    // Let's sort both actual and expected items before comparison to make the test robust
    // to internal ordering of std::unordered_map.
    auto sort_pairs = [](std::vector<std::pair<const std::string, std::string>>& vec) {
        std::sort(vec.begin(), vec.end(), [](const auto& a, const auto& b){
            return a.first < b.first;
        });
    };

    sort_pairs(actual_items);
    sort_pairs(expected_items); // Ensure expected_items is also sorted by key for fair comparison

    assert(actual_items == expected_items);

    std::cout << "Requirements Example Usage tests passed." << std::endl;
}


int main() {
    std::cout << "Starting ChainMap tests..." << std::endl;

    // Call test functions here
    test_default_constructor();
    test_single_map_constructor();
    test_initializer_list_constructor();
    test_variadic_constructor();
    test_access_and_lookup();
    test_modification_operations();
    test_iteration_and_views();
    test_requirements_example();
    // ... other test functions ...

    std::cout << "========================================" << std::endl;
    std::cout << "All ChainMap tests completed." << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}

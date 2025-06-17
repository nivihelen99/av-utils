#include <iostream>
#include <string>
#include <vector> // For std::vector to show eviction callback storing items
#include "lru_cache.h" // Assuming lru_cache.h is accessible via include paths

// A simple struct to use as a value in the cache
struct ComplexData {
    int id;
    std::string name;
    double value;

    // For easy printing
    friend std::ostream& operator<<(std::ostream& os, const ComplexData& data) {
        os << "ID: " << data.id << ", Name: "" << data.name << "", Value: " << data.value;
        return os;
    }
};

int main() {
    // --- Example 1: Basic usage with int keys and string values ---
    std::cout << "--- Example 1: Basic Integer Keys, String Values ---" << '\n';
    LRUCache<int, std::string> cache1(3); // Cache with max size 3

    cache1.put(1, "Apple");
    cache1.put(2, "Banana");
    cache1.put(3, "Cherry");

    std::cout << "Cache size: " << cache1.size() << '\n'; // Expected: 3

    if (auto val = cache1.get(2)) {
        std::cout << "Got key 2: " << val.value() << '\n'; // Expected: Banana
    } else {
        std::cout << "Key 2 not found." << '\n';
    }

    // Adding a new item, "Date", should evict the LRU item (1, "Apple")
    // because "Banana" (key 2) was most recently accessed.
    // Current order (MRU to LRU): 2, 3, 1
    cache1.put(4, "Date");
    std::cout << "Cache size after adding 4 ('Date'): " << cache1.size() << '\n'; // Expected: 3

    std::cout << "Contains key 1? " << (cache1.contains(1) ? "Yes" : "No") << '\n'; // Expected: No
    std::cout << "Contains key 4? " << (cache1.contains(4) ? "Yes" : "No") << '\n'; // Expected: Yes

    if (auto val = cache1.get(1)) {
        std::cout << "Got key 1: " << val.value() << '\n';
    } else {
        std::cout << "Key 1 not found (evicted)." << '\n'; // Expected
    }
     if (auto val = cache1.get(3)) { // Accessing 3 makes it MRU
        std::cout << "Got key 3: " << val.value() << '\n'; // Expected: Cherry
    }

    // Cache order (MRU to LRU): 3, 4, 2
    cache1.put(5, "Elderberry"); // Evicts 2 ("Banana")
    std::cout << "Contains key 2 after adding 5 ('Elderberry')? " << (cache1.contains(2) ? "Yes" : "No") << '\n'; // Expected: No

    std::cout << "Current cache items (order might vary due to internal map):" << '\n';
    if(auto v = cache1.get(5)) std::cout << "Key 5: " << v.value() << '\n';
    if(auto v = cache1.get(3)) std::cout << "Key 3: " << v.value() << '\n';
    if(auto v = cache1.get(4)) std::cout << "Key 4: " << v.value() << '\n';


    // --- Example 2: String keys and custom struct values with Eviction Callback ---
    std::cout << "\n--- Example 2: String Keys, Custom Struct Values, Eviction Callback ---" << '\n';

    std::vector<std::pair<std::string, ComplexData>> evicted_items;
    auto eviction_logger =
        [&evicted_items](const std::string& key, const ComplexData& value) {
        std::cout << "Eviction callback: Key "" << key << "" with value {" << value << "} was evicted." << '\n';
        evicted_items.push_back({key, value});
    };

    LRUCache<std::string, ComplexData> cache2(2, eviction_logger);

    cache2.put("alpha", {1, "Object Alpha", 10.5});
    cache2.put("beta",  {2, "Object Beta",  20.2});

    if (auto val = cache2.get("alpha")) { // alpha becomes MRU
        std::cout << "Got key 'alpha': " << val.value() << '\n';
    }

    // Order: alpha (MRU), beta (LRU)
    cache2.put("gamma", {3, "Object Gamma", 30.9}); // Evicts "beta"
    // Order: gamma (MRU), alpha (LRU)
    cache2.put("delta", {4, "Object Delta", 40.4}); // Evicts "alpha"

    std::cout << "Cache size: " << cache2.size() << '\n'; // Expected: 2
    std::cout << "Contains 'alpha'? " << (cache2.contains("alpha") ? "Yes" : "No") << '\n'; // Expected: No
    std::cout << "Contains 'beta'? "  << (cache2.contains("beta") ? "Yes" : "No") << '\n';  // Expected: No
    std::cout << "Contains 'gamma'? " << (cache2.contains("gamma") ? "Yes" : "No") << '\n'; // Expected: Yes
    std::cout << "Contains 'delta'? " << (cache2.contains("delta") ? "Yes" : "No") << '\n'; // Expected: Yes

    std::cout << "\nEvicted items log:" << '\n';
    for (const auto& item : evicted_items) {
        std::cout << "- Key: " << item.first << ", Value: { " << item.second << " }" << '\n';
    }
    // Expected evicted items: beta, then alpha.

    cache2.erase("gamma");
    std::cout << "\nCache size after erasing 'gamma': " << cache2.size() << '\n'; // Expected: 1
    std::cout << "Contains 'gamma' after erase? " << (cache2.contains("gamma") ? "Yes" : "No") << '\n'; // Expected: No

    cache2.clear();
    std::cout << "\nCache size after clear: " << cache2.size() << '\n'; // Expected: 0
    std::cout << "Is cache empty after clear? " << (cache2.empty() ? "Yes" : "No") << '\n'; // Expected: Yes

    // Eviction callback is not called for erase or clear.
    std::cout << "Total items in eviction log after erase/clear: " << evicted_items.size() << '\n'; // Expected: 2

    std::cout << "\n--- Example Finished ---" << '\n';

    return 0;
}

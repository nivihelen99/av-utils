#include "../include/value_index_map.h"
#include <iostream>
#include <string>
#include <vector>
#include <cassert> // For assert()

// A custom struct to demonstrate usage with user-defined types
struct Port {
    std::string name;
    int speed_gbps;

    bool operator==(const Port& other) const {
        return name == other.name && speed_gbps == other.speed_gbps;
    }
};

// Custom hash for Port struct
struct PortHash {
    std::size_t operator()(const Port& p) const {
        return std::hash<std::string>()(p.name) ^ (std::hash<int>()(p.speed_gbps) << 1);
    }
};

int main() {
    std::cout << "--- ValueIndexMap Example ---" << std::endl;

    // Example 1: Using std::string
    std::cout << "\n--- Example 1: std::string ---" << std::endl;
    ValueIndexMap<std::string> str_map;

    std::cout << "Inserting 'apple', 'banana', 'cherry'..." << std::endl;
    size_t idx_apple = str_map.insert("apple");
    size_t idx_banana = str_map.insert("banana");
    size_t idx_cherry = str_map.insert("cherry");

    std::cout << "'apple' inserted at index: " << idx_apple << std::endl;
    std::cout << "'banana' inserted at index: " << idx_banana << std::endl;
    std::cout << "'cherry' inserted at index: " << idx_cherry << std::endl;

    std::cout << "Current map size: " << str_map.size() << std::endl;

    std::cout << "Index of 'banana': " << str_map.index_of("banana").value_or(-1) << std::endl;
    if (const std::string* val = str_map.value_at(idx_apple)) {
        std::cout << "Value at index " << idx_apple << ": " << *val << std::endl;
    }

    std::cout << "Iterating through map:" << std::endl;
    for (const auto& str_val : str_map) {
        std::cout << "  Value: " << str_val << " (Index: " << str_map.index_of(str_val).value() << ")" << std::endl;
    }

    // Example 2: Using integers
    std::cout << "\n--- Example 2: int ---" << std::endl;
    ValueIndexMap<int> int_map;
    int_map.insert(1001);
    int_map.insert(2002);
    int_map.insert(1001); // Idempotent

    std::cout << "Integer map size: " << int_map.size() << std::endl; // Should be 2
    std::cout << "Index of 2002: " << int_map.index_of(2002).value_or(-1) << std::endl;

    // Example 3: Using Custom Struct (Port) with Custom Hash
    std::cout << "\n--- Example 3: Custom Struct (Port) ---" << std::endl;
    ValueIndexMap<Port, PortHash> port_map;

    Port p1 = {"eth0/1", 100};
    Port p2 = {"eth0/2", 40};
    Port p3 = {"eth0/1", 100}; // Same as p1

    size_t idx_p1 = port_map.insert(p1);
    size_t idx_p2 = port_map.insert(p2);
    size_t idx_p3 = port_map.insert(p3); // Idempotent

    std::cout << "Port map size: " << port_map.size() << std::endl; // Should be 2
    std::cout << "Index of Port{'eth0/1', 100}: " << port_map.index_of(p1).value_or(-1) << std::endl;
    assert(idx_p1 == idx_p3);

    if (const Port* port_val = port_map.value_at(idx_p2)) {
        std::cout << "Value at index " << idx_p2 << ": { Name: " << port_val->name
                  << ", Speed: " << port_val->speed_gbps << "Gbps }" << std::endl;
    }

    // Example 4: Serialization and Deserialization
    std::cout << "\n--- Example 4: Serialization/Deserialization ---" << std::endl;
    const std::vector<Port>& serialized_ports = port_map.get_values_for_serialization();
    std::cout << "Serialized data contains " << serialized_ports.size() << " ports." << std::endl;

    ValueIndexMap<Port, PortHash> new_port_map(serialized_ports); // Deserialize
    std::cout << "Deserialized port map size: " << new_port_map.size() << std::endl;
    assert(new_port_map.size() == port_map.size());
    assert(new_port_map.index_of(p1) == port_map.index_of(p1));
    assert(new_port_map.index_of(p2) == port_map.index_of(p2));
    std::cout << "Deserialization successful and data matches." << std::endl;


    // Example 5: Erase
    std::cout << "\n--- Example 5: Erase ---" << std::endl;
    std::cout << "Original str_map elements: ";
    for(const auto& s : str_map) { std::cout << s << "(" << str_map.index_of(s).value() << ") "; }
    std::cout << std::endl;

    std::cout << "Erasing 'banana' (index " << str_map.index_of("banana").value() << ")" << std::endl;
    str_map.erase("banana"); // 'cherry' (idx 2) will move to 'banana's slot (idx 1)

    std::cout << "str_map after erasing 'banana':" << std::endl;
    for (const auto& str_val : str_map) {
        std::cout << "  Value: " << str_val << " (Index: " << str_map.index_of(str_val).value() << ")" << std::endl;
    }
    assert(str_map.size() == 2);
    assert(str_map.index_of("apple").value() == 0);
    assert(str_map.index_of("cherry").value() == 1); // Cherry moved

    // Example 6: Seal
    std::cout << "\n--- Example 6: Seal ---" << std::endl;
    std::cout << "Sealing str_map..." << std::endl;
    str_map.seal();
    std::cout << "Is str_map sealed? " << (str_map.is_sealed() ? "Yes" : "No") << std::endl;

    std::cout << "Attempting to insert 'date' into sealed map..." << std::endl;
    try {
        str_map.insert("date");
    } catch (const std::logic_error& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }
    assert(str_map.size() == 2); // Size should not change

    std::cout << "\n--- ValueIndexMap Example End ---" << std::endl;
    return 0;
}

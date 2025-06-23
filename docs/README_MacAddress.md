# `MacAddress` Class

## Overview

The `MacAddress.h` header provides a robust C++ class, `MacAddress`, for representing and manipulating MAC-48 (Media Access Control) addresses. It offers a comprehensive suite of features including parsing from various string formats, conversion to strings, generation of special MAC addresses (random, broadcast, zero), type checking (unicast, multicast, broadcast, local/universal administration), and comparison.

The class is designed for ease of use, integrating with standard C++ idioms like stream operators, iterators, and hashing for use in STL containers.

## Features

-   **Multiple Construction Options:** Create `MacAddress` objects from strings, byte arrays, individual octets, or initializer lists.
-   **Flexible Parsing:** Parses MAC addresses from common formats:
    -   Colon-separated (e.g., `0A:1B:2C:3D:4E:5F`)
    -   Hyphen-separated (e.g., `0A-1B-2C-3D-4E-5F`)
    -   Dot-separated (e.g., `0A.1B.2C.3D.4E.5F` or Cisco-style `0a1b.2c3d.4e5f`)
    -   No separator (e.g., `0A1B2C3D4E5F`)
-   **Versatile String Conversion:** Convert MAC addresses to strings with customizable separators and case.
-   **Special MAC Address Generation:**
    -   `MacAddress::random()`: Generates a random, locally administered, unicast MAC.
    -   `MacAddress::broadcast()`: Provides the broadcast MAC (`FF:FF:FF:FF:FF:FF`).
    -   `MacAddress::zero()`: Provides the zero MAC (`00:00:00:00:00:00`).
-   **Type Checking:**
    -   `isBroadcast()`, `isMulticast()`, `isUnicast()`
    -   `isLocallyAdministered()`, `isUniversallyAdministered()`
    -   `isZero()`, `isValid()`
-   **Component Access:**
    -   `getOUI()`: Retrieves the 3-byte Organizationally Unique Identifier.
    -   `getNIC()`: Retrieves the 3-byte Network Interface Controller specific part.
-   **Integer Conversion:** `toUInt64()` and `fromUInt64()`.
-   **Standard Compliance:**
    -   Overloaded comparison operators (`==`, `!=`, `<`, `>`, `<=`, `>=`).
    -   Stream insertion (`<<`) and extraction (`>>`) operators.
    -   Iterators (`begin`, `end`) for octet access.
    -   `std::hash<MacAddress>` specialization for use with `std::unordered_map`, `std::unordered_set`, etc.

## Public Interface Highlights

### Constructors

```cpp
MacAddress();                               // Zero MAC: 00:00:00:00:00:00
MacAddress(std::string_view mac_str);       // Parses string, throws on error
MacAddress(const std::array<uint8_t, 6>& octets);
MacAddress(uint8_t o1, uint8_t o2, uint8_t o3, uint8_t o4, uint8_t o5, uint8_t o6);
MacAddress({0x0A, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F}); // From initializer_list
```

### Static Factory Methods

```cpp
static MacAddress MacAddress::fromString(std::string_view mac_str);
static MacAddress MacAddress::fromBytes(const uint8_t* bytes);
static MacAddress MacAddress::random();
static MacAddress MacAddress::broadcast();
static MacAddress MacAddress::zero();
```

### String Conversion

```cpp
std::string toString(char separator = ':') const;      // Uppercase hex
std::string toStringLower(char separator = ':') const; // Lowercase hex
std::string toCiscoFormat() const;                     // xxxx.xxxx.xxxx (lowercase)
std::string toWindowsFormat() const;                   // XX-XX-XX-XX-XX-XX
std::string toUnixFormat() const;                      // XX:XX:XX:XX:XX:XX
```

### Accessors and Utility

```cpp
const std::array<uint8_t, 6>& getOctets() const noexcept;
uint8_t operator[](size_t index) const; // With bounds checking via at()
bool isBroadcast() const noexcept;
bool isMulticast() const noexcept;
// ... and other utility methods
```

## Usage Examples

### Creating and Displaying MAC Addresses

```cpp
#include "MacAddress.h"
#include <iostream>
#include <vector>

int main() {
    // From string
    try {
        MacAddress mac1("0A:1B:2C:3D:4E:5F");
        std::cout << "MAC1 (Unix format): " << mac1.toUnixFormat() << std::endl;
        std::cout << "MAC1 (Windows format): " << mac1.toWindowsFormat() << std::endl;
        std::cout << "MAC1 (Cisco format): " << mac1.toCiscoFormat() << std::endl;
        std::cout << "MAC1 (lowercase, no separator): " << mac1.toStringLower(' ') << std::endl;


        MacAddress mac2("0a-1b-2c-3d-4e-5f"); // Hyphen, lowercase
        std::cout << "MAC2: " << mac2 << std::endl; // Uses default operator<< (colon, uppercase)

        MacAddress mac3("0A1B.2C3D.4E5F"); // Cisco format
        std::cout << "MAC3: " << mac3 << std::endl;

        MacAddress mac4("0A1B2C3D4E5F"); // No separator
        std::cout << "MAC4: " << mac4 << std::endl;

    } catch (const std::invalid_argument& e) {
        std::cerr << "Error creating MAC address: " << e.what() << std::endl;
    }

    // From individual octets
    MacAddress mac_octets(0x01, 0x23, 0x45, 0x67, 0x89, 0xAB);
    std::cout << "MAC from octets: " << mac_octets << std::endl;

    // From initializer list
    MacAddress mac_init({0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54});
    std::cout << "MAC from initializer list: " << mac_init << std::endl;

    // Special MAC addresses
    MacAddress r_mac = MacAddress::random();
    std::cout << "Random MAC: " << r_mac << std::endl;
    std::cout << "  Is locally administered? " << std::boolalpha << r_mac.isLocallyAdministered() << std::endl;
    std::cout << "  Is unicast? " << std::boolalpha << r_mac.isUnicast() << std::endl;

    MacAddress b_mac = MacAddress::broadcast();
    std::cout << "Broadcast MAC: " << b_mac << std::endl;
    std::cout << "  Is broadcast? " << std::boolalpha << b_mac.isBroadcast() << std::endl;

    MacAddress z_mac = MacAddress::zero();
    std::cout << "Zero MAC: " << z_mac << std::endl;
    std::cout << "  Is zero? " << std::boolalpha << z_mac.isZero() << std::endl;
    std::cout << "  Is valid? " << std::boolalpha << z_mac.isValid() << std::endl; // false

    return 0;
}
```

### Parsing and Properties

```cpp
#include "MacAddress.h"
#include <iostream>

int main() {
    std::string mac_str = "01:00:5E:7F:FF:FA"; // Example multicast MAC
    try {
        MacAddress mac = MacAddress::fromString(mac_str);
        std::cout << "Parsed MAC: " << mac << std::endl;
        std::cout << "  Is multicast? " << std::boolalpha << mac.isMulticast() << std::endl;        // true
        std::cout << "  Is unicast? " << std::boolalpha << mac.isUnicast() << std::endl;          // false
        std::cout << "  Is universally administered? " << std::boolalpha << mac.isUniversallyAdministered() << std::endl; // true (01 is not locally administered)

        uint32_t oui = mac.getOUI();
        uint32_t nic = mac.getNIC();
        std::cout << "  OUI: " << std::hex << oui << std::endl; // 01005E
        std::cout << "  NIC: " << std::hex << nic << std::endl; // 7FFFFA

        uint64_t mac_val = mac.toUInt64();
        std::cout << "  As uint64_t: 0x" << std::hex << mac_val << std::endl;

        MacAddress mac_from_val = MacAddress::fromUInt64(mac_val);
        std::cout << "  From uint64_t: " << mac_from_val << std::endl;
        std::cout << "  Matches original? " << std::boolalpha << (mac == mac_from_val) << std::endl;

    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    // Input stream operator
    MacAddress input_mac;
    std::cout << "Enter a MAC address (e.g., 11:22:33:44:55:66): ";
    std::cin >> input_mac;
    if (std::cin.good()) {
        std::cout << "You entered: " << input_mac << std::endl;
    } else {
        std::cout << "Invalid MAC address input." << std::endl;
    }

    return 0;
}
```

### Using with STL Containers

```cpp
#include "MacAddress.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_set>

int main() {
    std::vector<MacAddress> macs = {
        MacAddress("00:00:00:AA:BB:CC"),
        MacAddress("00:00:00:11:22:33"),
        MacAddress("00:00:00:AA:BB:CC") // Duplicate for set testing
    };

    // Sorting (uses operator<)
    std::sort(macs.begin(), macs.end());
    std::cout << "Sorted MACs:" << std::endl;
    for (const auto& m : macs) {
        std::cout << "  " << m << std::endl;
    }

    // Using in an unordered_set (uses std::hash<MacAddress> and operator==)
    std::unordered_set<MacAddress> mac_set;
    mac_set.insert(MacAddress("DE:AD:BE:EF:00:01"));
    mac_set.insert(MacAddress("DE:AD:BE:EF:00:02"));
    mac_set.insert(MacAddress("DE:AD:BE:EF:00:01")); // This will not be inserted again

    std::cout << "\nMACs in unordered_set (size " << mac_set.size() << "):" << std::endl;
    for (const auto& m : mac_set) {
        std::cout << "  " << m << std::endl;
    }

    // Iterating over octets
    MacAddress my_mac("01:02:03:04:05:06");
    std::cout << "\nOctets of " << my_mac << ": ";
    for (uint8_t octet : my_mac) {
        std::cout << std::hex << static_cast<int>(octet) << " ";
    }
    std::cout << std::dec << std::endl;

    return 0;
}
```

## Dependencies
- `<array>`, `<functional>`, `<iomanip>`, `<iostream>`, `<random>`, `<regex>`, `<sstream>`, `<stdexcept>`, `<string>`, `<string_view>`

This class provides a comprehensive and modern C++ interface for working with MAC addresses.

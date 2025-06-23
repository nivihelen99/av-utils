# MAC Address Parsing and Formatting (`mac_parse.h`)

## Overview

The `mac_parse.h` header provides a set of C-style/procedural functions for parsing MAC address strings into a `MAC_ADR` struct and for formatting `MAC_ADR` structs back into various string representations. It supports common MAC address formats and offers control over output casing and prefixing.

This utility is suitable for contexts where a lightweight, function-based approach to MAC address manipulation is preferred over an object-oriented one.

## Core Components

### `MAC_ADR` Struct

```c++
#define MAC_ADDR_SZ 6
struct MAC_ADR {
  unsigned char macAdr[MAC_ADDR_SZ]; // Stores 6 octets of the MAC address
};
#define MAC_ADR_LEN sizeof(MAC_ADR)
```

### `MacFormat` Enum

Defines the desired string output format:

```c++
enum class MacFormat {
   COLON_SEPARATED,  // e.g., AA:BB:CC:DD:EE:FF
   HYPHEN_SEPARATED, // e.g., AA-BB-CC-DD-EE-FF
   DOT_SEPARATED,    // e.g., AA.BB.CC.DD.EE.FF
   DOTTED_QUAD,      // e.g., AAAA.BBBB.CCCC (Cisco style)
   NO_SEPARATOR      // e.g., AABBCCDDEEFF
};
```

## Key Functions

### Parsing

-   **`std::optional<MAC_ADR> parseMAC(const std::string& mac_str)`**
    -   The main parsing function. It attempts to automatically detect the format of the input `mac_str` (colon, hyphen, dot-separated, or no separator) and parse it into a `MAC_ADR` struct.
    -   Returns a `std::optional<MAC_ADR>` which contains the parsed MAC address if successful, or `std::nullopt` if parsing fails.

-   **`std::optional<MAC_ADR> parseMAC_WithSeparator(const std::string& mac_str, char separator)`**
    -   Parses MAC address strings that use a known `separator` (':', '-', or '.').
    -   Supports `XX:XX:XX:XX:XX:XX`, `XX-XX-XX-XX-XX-XX`, `XXXX.XXXX.XXXX` (Cisco), and `XX.XX.XX.XX.XX.XX`.

-   **`std::optional<MAC_ADR> parseMAC_WithoutSeparator(const std::string& mac_str)`**
    -   Parses MAC address strings that consist of 12 hexadecimal characters without any separators (e.g., `AABBCCDDEEFF`).

### Formatting

-   **`std::string macToString(const MAC_ADR& mac, MacFormat format = MacFormat::COLON_SEPARATED, bool uppercase = true, bool add_0x = false)`**
    -   Converts a `MAC_ADR` struct to its string representation.
    -   `format`: Specifies the output format using the `MacFormat` enum.
    -   `uppercase`: If `true`, uses uppercase hexadecimal characters (A-F); otherwise, lowercase (a-f).
    -   `add_0x`: If `true`, prefixes the first octet (or first group in `DOTTED_QUAD`, or the whole string in `NO_SEPARATOR`) with "0x".

-   **Convenience Formatting Functions:**
    These are wrappers around `macToString` for common formats:
    -   `std::string macToColonString(const MAC_ADR& mac, bool uppercase = true, bool add_0x = false)`
    -   `std::string macToHyphenString(const MAC_ADR& mac, bool uppercase = true, bool add_0x = false)`
    -   `std::string macToDottedQuadString(const MAC_ADR& mac, bool uppercase = true, bool add_0x = false)`
    -   `std::string macToPlainString(const MAC_ADR& mac, bool uppercase = true, bool add_0x = false)`

-   **`std::string formatMAC(const MAC_ADR& mac, char separator = ':', bool lowercase = false)`**
    -   A simpler formatting function that takes a specific `separator` character and a `lowercase` flag.

### Helper

-   **`bool isInputLowercase(const std::string& mac_str)`**
    -   Checks if the input MAC string predominantly uses lowercase hexadecimal characters. This can be useful for preserving case when re-formatting.

## Usage Examples

(Examples are based on `examples/use_mac_parser.cpp`)

### Parsing MAC Strings

```cpp
#include "mac_parse.h"
#include <iostream>
#include <string>
#include <vector>
#include <optional> // For std::optional

// Helper to print MAC_ADR
void print_mac(const std::optional<MAC_ADR>& mac_opt) {
    if (mac_opt) {
        std::cout << "Parsed: ";
        for (size_t i = 0; i < MAC_ADDR_SZ; ++i) {
            printf("%02X", mac_opt->macAdr[i]);
            if (i < MAC_ADDR_SZ - 1) std::cout << ":";
        }
        std::cout << std::endl;
    } else {
        std::cout << "Failed to parse MAC address." << std::endl;
    }
}

int main() {
    std::vector<std::string> test_macs = {
        "01:23:45:67:89:AB",
        "cd-ef-01-23-45-67",
        "ABCD.EF01.2345",    // Dotted quad
        "ab.cd.ef.01.23.45", // Dot separated
        "0123456789ab",      // No separator
        "invalid-mac",
        "00:11:22:33:44:XX"  // Invalid hex
    };

    for (const auto& mac_str : test_macs) {
        std::cout << "Input: \"" << mac_str << "\"" << std::endl;
        std::optional<MAC_ADR> parsed = parseMAC(mac_str);
        print_mac(parsed);
    }

    return 0;
}
```

### Formatting `MAC_ADR` to String

```cpp
#include "mac_parse.h"
#include <iostream>

int main() {
    MAC_ADR my_mac = {{0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33}};

    std::cout << "Original MAC: AA:BB:CC:11:22:33" << std::endl;

    // Using macToString
    std::cout << "Colon (uppercase):     " << macToString(my_mac, MacFormat::COLON_SEPARATED, true) << std::endl;
    std::cout << "Colon (lowercase):     " << macToString(my_mac, MacFormat::COLON_SEPARATED, false) << std::endl;
    std::cout << "Hyphen (uppercase):    " << macToString(my_mac, MacFormat::HYPHEN_SEPARATED, true) << std::endl;
    std::cout << "Dotted Quad (Cisco):   " << macToString(my_mac, MacFormat::DOTTED_QUAD, true) << std::endl;
    std::cout << "No Separator:          " << macToString(my_mac, MacFormat::NO_SEPARATOR, true) << std::endl;
    std::cout << "Colon (lowercase, 0x): " << macToString(my_mac, MacFormat::COLON_SEPARATED, false, true) << std::endl;
    std::cout << "Plain (uppercase, 0x): " << macToString(my_mac, MacFormat::NO_SEPARATOR, true, true) << std::endl;


    // Using convenience functions
    std::cout << "macToColonString():      " << macToColonString(my_mac) << std::endl;
    std::cout << "macToHyphenString(lower):" << macToHyphenString(my_mac, false) << std::endl;
    std::cout << "macToDottedQuadString(): " << macToDottedQuadString(my_mac) << std::endl;

    // Using formatMAC
    std::cout << "formatMAC (hyphen, lower): " << formatMAC(my_mac, '-', true) << std::endl;

    return 0;
}
```

## Dependencies
- `<string>`, `<vector>`, `<optional>`, `<regex>`, `<iomanip>`, `<sstream>`

This header provides a straightforward, functional approach for basic MAC address string manipulation in C++.

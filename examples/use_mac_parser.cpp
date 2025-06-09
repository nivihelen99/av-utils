#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include "mac_parse.h"

// Helper function to print MAC_ADR (optional)
void printMacAddress(const std::optional<MAC_ADR>& mac_opt) {
    if (mac_opt) {
        std::cout << "Parsed MAC Address: ";
        for (size_t i = 0; i < MAC_ADDR_SZ; ++i) {
            printf("%02X", mac_opt->macAdr[i]);
            if (i < MAC_ADDR_SZ - 1) {
                std::cout << ":";
            }
        }
        std::cout << std::endl;
    } else {
        std::cout << "Failed to parse MAC address." << std::endl;
    }
}

int main() {
    std::cout << "--- MAC Address Parsing Demonstrations ---" << std::endl;

    std::vector<std::string> mac_strings_to_parse = {
        "01:23:45:67:89:AB",
        "01-23-45-67-89-AB",
        "0123.4567.89AB",
        "01.23.45.67.89.AB",
        "0123456789AB",
        "invalid-mac-string", // Invalid case
        "01:23:45:67:89:XY" // Invalid hex char
    };

    for (const auto& mac_str : mac_strings_to_parse) {
        std::cout << "\nParsing MAC string: \"" << mac_str << "\"" << std::endl;
        std::optional<MAC_ADR> parsed_mac = parseMAC(mac_str);
        printMacAddress(parsed_mac);
    }

    std::cout << "\n--- MAC Address Formatting Demonstrations ---" << std::endl;

    MAC_ADR sample_mac = {{0x01, 0x23, 0x45, 0x67, 0x89, 0xAB}};
    std::cout << "Sample MAC_ADR: ";
    for (size_t i = 0; i < MAC_ADDR_SZ; ++i) {
        printf("%02X", sample_mac.macAdr[i]);
        if (i < MAC_ADDR_SZ - 1) {
            std::cout << ":";
        }
    }
    std::cout << std::endl;


    std::cout << "\nFormatting with MacFormat::COLON_SEPARATED:" << std::endl;
    std::cout << "Default: " << macToString(sample_mac, MacFormat::COLON_SEPARATED) << std::endl;
    std::cout << "Uppercase: " << macToString(sample_mac, MacFormat::COLON_SEPARATED, true) << std::endl;
    std::cout << "With 0x: " << macToString(sample_mac, MacFormat::COLON_SEPARATED, false, true) << std::endl;
    std::cout << "Uppercase & 0x: " << macToString(sample_mac, MacFormat::COLON_SEPARATED, true, true) << std::endl;

    std::cout << "\nFormatting with MacFormat::HYPHEN_SEPARATED:" << std::endl;
    std::cout << "Default: " << macToString(sample_mac, MacFormat::HYPHEN_SEPARATED) << std::endl;
    std::cout << "Uppercase: " << macToString(sample_mac, MacFormat::HYPHEN_SEPARATED, true) << std::endl;
    std::cout << "With 0x: " << macToString(sample_mac, MacFormat::HYPHEN_SEPARATED, false, true) << std::endl;
    std::cout << "Uppercase & 0x: " << macToString(sample_mac, MacFormat::HYPHEN_SEPARATED, true, true) << std::endl;

    std::cout << "\nFormatting with MacFormat::DOT_SEPARATED:" << std::endl;
    std::cout << "Default: " << macToString(sample_mac, MacFormat::DOT_SEPARATED) << std::endl;
    std::cout << "Uppercase: " << macToString(sample_mac, MacFormat::DOT_SEPARATED, true) << std::endl;
    // Note: add_0x is not typically used with DOT_SEPARATED for MACs, but testing for completeness
    std::cout << "With 0x (non-standard): " << macToString(sample_mac, MacFormat::DOT_SEPARATED, false, true) << std::endl;


    std::cout << "\nFormatting with MacFormat::DOTTED_QUAD:" << std::endl;
    std::cout << "Default: " << macToString(sample_mac, MacFormat::DOTTED_QUAD) << std::endl;
    std::cout << "Uppercase: " << macToString(sample_mac, MacFormat::DOTTED_QUAD, true) << std::endl;
     // Note: add_0x is not typically used with DOTTED_QUAD for MACs
    std::cout << "With 0x (non-standard): " << macToString(sample_mac, MacFormat::DOTTED_QUAD, false, true) << std::endl;

    std::cout << "\nFormatting with MacFormat::NO_SEPARATOR:" << std::endl;
    std::cout << "Default: " << macToString(sample_mac, MacFormat::NO_SEPARATOR) << std::endl;
    std::cout << "Uppercase: " << macToString(sample_mac, MacFormat::NO_SEPARATOR, true) << std::endl;
    // Note: add_0x is not typically used with NO_SEPARATOR for MACs
    std::cout << "With 0x (non-standard): " << macToString(sample_mac, MacFormat::NO_SEPARATOR, false, true) << std::endl;


    std::cout << "\n--- Handling Invalid MAC Strings ---" << std::endl;
    std::string invalid_str1 = "00:11:22:33:44:XX"; // Invalid character
    std::cout << "Parsing invalid string: \"" << invalid_str1 << "\"" << std::endl;
    std::optional<MAC_ADR> result1 = parseMAC(invalid_str1);
    if (!result1) {
        std::cout << "Correctly identified as invalid." << std::endl;
    } else {
        std::cout << "Incorrectly parsed as valid: ";
        printMacAddress(result1);
    }

    std::string invalid_str2 = "00:11:22:33:44:55:66"; // Too long
    std::cout << "Parsing invalid string (too long): \"" << invalid_str2 << "\"" << std::endl;
    std::optional<MAC_ADR> result2 = parseMAC(invalid_str2);
    if (!result2) {
        std::cout << "Correctly identified as invalid." << std::endl;
    } else {
        std::cout << "Incorrectly parsed as valid: ";
        printMacAddress(result2);
    }

    std::string invalid_str3 = "0011223344"; // Too short for no-separator
    std::cout << "Parsing invalid string (too short): \"" << invalid_str3 << "\"" << std::endl;
    std::optional<MAC_ADR> result3 = parseMAC(invalid_str3);
     if (!result3) {
        std::cout << "Correctly identified as invalid." << std::endl;
    } else {
        std::cout << "Incorrectly parsed as valid: ";
        printMacAddress(result3);
    }


    std::cout << "\nDemonstration complete." << std::endl;

    return 0;
}

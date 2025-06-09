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

int test1()
{
    std::vector<std::string> test_macs = {
        "AA:BB:CC:DD:EE:FF",
        "aa:bb:cc:dd:ee:ff",
        "AA-BB-CC-DD-EE-FF",
        "aa-bb-cc-dd-ee-ff", 
        "AABB.CCDD.EEFF",
        "aabb.ccdd.eeff",
        "AA.BB.CC.DD.EE.FF",
        "aa.bb.cc.dd.ee.ff",
        "AABBCCDDEEFF",
        "aabbccddeeff",
        "12:34:56:aB:Cd:Ef",  // Mixed case
        "invalid_mac"
    };
    
    for (const auto& mac_str : test_macs)
    {
        auto result = parseMAC(mac_str);
        if (result)
        {
            bool input_lowercase = isInputLowercase(mac_str);
            std::string formatted = formatMAC(*result, ':', input_lowercase);
            std::cout << "Parsed '" << mac_str << "' -> " << formatted << std::endl;
        }
        else
        {
            std::cout << "Failed to parse: " << mac_str << std::endl;
        }
    }
    
    return 0;
}

int test2()
{
    // Example MAC address: AA:BB:CC:DD:EE:FF
    MAC_ADR mac = {};
    mac.macAdr[0] = 0xAA;
    mac.macAdr[1] = 0xBB;
    mac.macAdr[2] = 0xCC;
    mac.macAdr[3] = 0xDD;
    mac.macAdr[4] = 0xEE;
    mac.macAdr[5] = 0xFF;
    
    std::cout << "=== Different Formats ===" << std::endl;
    std::cout << "Colon:        " << macToString(mac, MacFormat::COLON_SEPARATED) << std::endl;
    std::cout << "Hyphen:       " << macToString(mac, MacFormat::HYPHEN_SEPARATED) << std::endl;
    std::cout << "Dot:          " << macToString(mac, MacFormat::DOT_SEPARATED) << std::endl;
    std::cout << "Dotted Quad:  " << macToString(mac, MacFormat::DOTTED_QUAD) << std::endl;
    std::cout << "No Separator: " << macToString(mac, MacFormat::NO_SEPARATOR) << std::endl;
    
    std::cout << "\n=== Case Variations ===" << std::endl;
    std::cout << "Uppercase:    " << macToString(mac, MacFormat::COLON_SEPARATED, true) << std::endl;
    std::cout << "Lowercase:    " << macToString(mac, MacFormat::COLON_SEPARATED, false) << std::endl;
    
    std::cout << "\n=== With 0x Prefix (only first octet) ===" << std::endl;
    std::cout << "Colon + 0x:   " << macToString(mac, MacFormat::COLON_SEPARATED, true, true) << std::endl;
    std::cout << "Hyphen + 0x:  " << macToString(mac, MacFormat::HYPHEN_SEPARATED, false, true) << std::endl;
    std::cout << "Plain + 0x:   " << macToString(mac, MacFormat::NO_SEPARATOR, true, true) << std::endl;
    std::cout << "Quad + 0x:    " << macToString(mac, MacFormat::DOTTED_QUAD, false, true) << std::endl;
    
    std::cout << "\n=== Convenience Functions ===" << std::endl;
    std::cout << "macToColonString():      " << macToColonString(mac) << std::endl;
    std::cout << "macToHyphenString():     " << macToHyphenString(mac, false) << std::endl;
    std::cout << "macToDottedQuadString(): " << macToDottedQuadString(mac) << std::endl;
    std::cout << "macToPlainString():      " << macToPlainString(mac, true, true) << std::endl;
    
    return 0;
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

    test1();
    test2();
    
    std::cout << "\nDemonstration complete." << std::endl;

    return 0;
}

#include <gtest/gtest.h>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <chrono>
#include "MacAddress.h" // Include your MacAddress header

class MacAddressTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common test MAC addresses
        test_mac_str_ = "AA:BB:CC:DD:EE:FF";
        test_mac_bytes_ = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
        broadcast_mac_ = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        zero_mac_ = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        multicast_mac_ = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x01}; // Multicast bit set
        locally_administered_mac_ = {0x02, 0x11, 0x22, 0x33, 0x44, 0x55}; // Locally administered
    }

    std::string test_mac_str_;
    MacAddress::MacArray test_mac_bytes_;
    MacAddress::MacArray broadcast_mac_;
    MacAddress::MacArray zero_mac_;
    MacAddress::MacArray multicast_mac_;
    MacAddress::MacArray locally_administered_mac_;
    MacAddress::MacArray universally_admin_test_mac_bytes_ = {0x00, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
};

// Test Constructors
TEST_F(MacAddressTest, DefaultConstructor) {
    MacAddress mac;
    EXPECT_TRUE(mac.isZero());
    EXPECT_EQ(mac.toString(), "00:00:00:00:00:00");
}

TEST_F(MacAddressTest, ArrayConstructor) {
    MacAddress mac(test_mac_bytes_);
    EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
    
    for (size_t i = 0; i < MacAddress::MAC_LENGTH; ++i) {
        EXPECT_EQ(mac[i], test_mac_bytes_[i]);
    }
}

TEST_F(MacAddressTest, StringConstructor) {
    MacAddress mac(test_mac_str_);
    EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(MacAddressTest, InvalidStringConstructor) {
    EXPECT_THROW(MacAddress("invalid"), std::invalid_argument);
    EXPECT_THROW(MacAddress("AA:BB:CC:DD:EE"), std::invalid_argument);
    EXPECT_THROW(MacAddress("AA:BB:CC:DD:EE:GG"), std::invalid_argument);
}

TEST_F(MacAddressTest, IndividualOctetConstructor) {
    MacAddress mac(0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF);
    EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(MacAddressTest, CopyConstructor) {
    MacAddress original(test_mac_bytes_);
    MacAddress copy(original);
    
    EXPECT_EQ(original, copy);
    EXPECT_EQ(copy.toString(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(MacAddressTest, MoveConstructor) {
    MacAddress original(test_mac_bytes_);
    MacAddress moved(std::move(original));
    
    EXPECT_EQ(moved.toString(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(MacAddressTest, AssignmentOperator) {
    MacAddress mac1(test_mac_bytes_);
    MacAddress mac2;
    
    mac2 = mac1;
    EXPECT_EQ(mac1, mac2);
}

// Test Factory Methods
TEST_F(MacAddressTest, FromStringFactory) {
    MacAddress mac = MacAddress::fromString("AA:BB:CC:DD:EE:FF");
    EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(MacAddressTest, FromBytesFactory) {
    uint8_t bytes[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    MacAddress mac = MacAddress::fromBytes(bytes);
    EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(MacAddressTest, FromBytesFactoryNullPointer) {
    EXPECT_THROW(MacAddress::fromBytes(nullptr), std::invalid_argument);
}

TEST_F(MacAddressTest, RandomFactory) {
    MacAddress mac1 = MacAddress::random();
    MacAddress mac2 = MacAddress::random();
    
    // Random MACs should be different (very high probability)
    EXPECT_NE(mac1, mac2);
    
    // Random MAC should be locally administered and unicast
    EXPECT_TRUE(mac1.isLocallyAdministered());
    EXPECT_TRUE(mac1.isUnicast());
}

TEST_F(MacAddressTest, BroadcastFactory) {
    MacAddress mac = MacAddress::broadcast();
    EXPECT_TRUE(mac.isBroadcast());
    EXPECT_EQ(mac.toString(), "FF:FF:FF:FF:FF:FF");
}

TEST_F(MacAddressTest, ZeroFactory) {
    MacAddress mac = MacAddress::zero();
    EXPECT_TRUE(mac.isZero());
    EXPECT_EQ(mac.toString(), "00:00:00:00:00:00");
}

// Test String Parsing
TEST_F(MacAddressTest, ParseColonSeparated) {
    MacAddress mac;
    EXPECT_TRUE(mac.parse("AA:BB:CC:DD:EE:FF"));
    EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
    
    EXPECT_TRUE(mac.parse("aa:bb:cc:dd:ee:ff"));
    EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
    
    EXPECT_TRUE(mac.parse("A:B:C:D:E:F"));
    EXPECT_EQ(mac.toString(), "0A:0B:0C:0D:0E:0F");
}

TEST_F(MacAddressTest, ParseHyphenSeparated) {
    MacAddress mac;
    EXPECT_TRUE(mac.parse("AA-BB-CC-DD-EE-FF"));
    EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(MacAddressTest, ParseCiscoFormat) {
    MacAddress mac;
    EXPECT_TRUE(mac.parse("AABB.CCDD.EEFF"));
    EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
    
    EXPECT_TRUE(mac.parse("aabb.ccdd.eeff"));
    EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(MacAddressTest, ParseNoSeparator) {
    MacAddress mac;
    EXPECT_TRUE(mac.parse("AABBCCDDEEFF"));
    EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
    
    EXPECT_TRUE(mac.parse("aabbccddeeff"));
    EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(MacAddressTest, ParseWithWhitespace) {
    MacAddress mac;
    EXPECT_TRUE(mac.parse(" AA:BB:CC:DD:EE:FF "));
    EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
    
    EXPECT_TRUE(mac.parse("AA: BB :CC: DD: EE :FF"));
    EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(MacAddressTest, ParseInvalidFormats) {
    MacAddress mac;
    EXPECT_FALSE(mac.parse(""));
    EXPECT_FALSE(mac.parse("AA:BB:CC:DD:EE"));
    EXPECT_FALSE(mac.parse("AA:BB:CC:DD:EE:FF:GG"));
    EXPECT_FALSE(mac.parse("GG:BB:CC:DD:EE:FF"));
    EXPECT_FALSE(mac.parse("AA::BB:CC:DD:EE:FF"));
    EXPECT_FALSE(mac.parse("AABBCCDDEEF"));
    EXPECT_FALSE(mac.parse("AABBCCDDEEFFF"));
}

// Test Accessors
TEST_F(MacAddressTest, ArrayAccess) {
    MacAddress mac(test_mac_bytes_);
    
    for (size_t i = 0; i < MacAddress::MAC_LENGTH; ++i) {
        EXPECT_EQ(mac[i], test_mac_bytes_[i]);
    }
}

TEST_F(MacAddressTest, ArrayAccessOutOfBounds) {
    MacAddress mac(test_mac_bytes_);
    EXPECT_THROW(mac[MacAddress::MAC_LENGTH], std::out_of_range);
}

TEST_F(MacAddressTest, AtMethod) {
    MacAddress mac(test_mac_bytes_);
    
    for (size_t i = 0; i < MacAddress::MAC_LENGTH; ++i) {
        EXPECT_EQ(mac.at(i), test_mac_bytes_[i]);
    }
    
    EXPECT_THROW(mac.at(MacAddress::MAC_LENGTH), std::out_of_range);
}

TEST_F(MacAddressTest, ModifyThroughAccess) {
    MacAddress mac;
    mac[0] = 0xAA;
    mac.at(1) = 0xBB;
    
    EXPECT_EQ(mac[0], 0xAA);
    EXPECT_EQ(mac.at(1), 0xBB);
}

TEST_F(MacAddressTest, GetOctets) {
    MacAddress mac(test_mac_bytes_);
    const auto& octets = mac.getOctets();
    
    for (size_t i = 0; i < MacAddress::MAC_LENGTH; ++i) {
        EXPECT_EQ(octets[i], test_mac_bytes_[i]);
    }
}

// Test String Conversion Methods
TEST_F(MacAddressTest, ToStringDefault) {
    MacAddress mac(test_mac_bytes_);
    EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(MacAddressTest, ToStringCustomSeparator) {
    MacAddress mac(test_mac_bytes_);
    EXPECT_EQ(mac.toString('-'), "AA-BB-CC-DD-EE-FF");
    EXPECT_EQ(mac.toString('|'), "AA|BB|CC|DD|EE|FF");
}

TEST_F(MacAddressTest, ToStringLower) {
    MacAddress mac(test_mac_bytes_);
    EXPECT_EQ(mac.toStringLower(), "aa:bb:cc:dd:ee:ff");
    EXPECT_EQ(mac.toStringLower('-'), "aa-bb-cc-dd-ee-ff");
}

TEST_F(MacAddressTest, ToCiscoFormat) {
    MacAddress mac(test_mac_bytes_);
    EXPECT_EQ(mac.toCiscoFormat(), "aabb.ccdd.eeff");
}

TEST_F(MacAddressTest, ToWindowsFormat) {
    MacAddress mac(test_mac_bytes_);
    EXPECT_EQ(mac.toWindowsFormat(), "AA-BB-CC-DD-EE-FF");
}

TEST_F(MacAddressTest, ToUnixFormat) {
    MacAddress mac(test_mac_bytes_);
    EXPECT_EQ(mac.toUnixFormat(), "AA:BB:CC:DD:EE:FF");
}

// Test Utility Methods
TEST_F(MacAddressTest, IsValid) {
    MacAddress zero_mac(zero_mac_);
    MacAddress valid_mac(test_mac_bytes_);
    
    EXPECT_FALSE(zero_mac.isValid());
    EXPECT_TRUE(valid_mac.isValid());
}

TEST_F(MacAddressTest, IsZero) {
    MacAddress zero_mac(zero_mac_);
    MacAddress non_zero_mac(test_mac_bytes_);
    
    EXPECT_TRUE(zero_mac.isZero());
    EXPECT_FALSE(non_zero_mac.isZero());
}

TEST_F(MacAddressTest, IsBroadcast) {
    MacAddress broadcast_mac(broadcast_mac_);
    MacAddress non_broadcast_mac(test_mac_bytes_);
    
    EXPECT_TRUE(broadcast_mac.isBroadcast());
    EXPECT_FALSE(non_broadcast_mac.isBroadcast());
}

TEST_F(MacAddressTest, IsMulticast) {
    MacAddress multicast_mac(multicast_mac_);
    MacAddress unicast_mac(test_mac_bytes_);
    
    EXPECT_TRUE(multicast_mac.isMulticast());
    EXPECT_FALSE(unicast_mac.isMulticast());
}

TEST_F(MacAddressTest, IsUnicast) {
    MacAddress multicast_mac(multicast_mac_);
    MacAddress unicast_mac(test_mac_bytes_);
    
    EXPECT_FALSE(multicast_mac.isUnicast());
    EXPECT_TRUE(unicast_mac.isUnicast());
}

TEST_F(MacAddressTest, IsLocallyAdministered) {
    MacAddress locally_admin_mac(locally_administered_mac_);
    MacAddress universally_admin_mac(universally_admin_test_mac_bytes_);
    
    EXPECT_TRUE(locally_admin_mac.isLocallyAdministered());
    EXPECT_FALSE(universally_admin_mac.isLocallyAdministered());
}

TEST_F(MacAddressTest, IsUniversallyAdministered) {
    MacAddress locally_admin_mac(locally_administered_mac_);
    MacAddress universally_admin_mac(universally_admin_test_mac_bytes_);
    
    EXPECT_FALSE(locally_admin_mac.isUniversallyAdministered());
    EXPECT_TRUE(universally_admin_mac.isUniversallyAdministered());
}

TEST_F(MacAddressTest, GetOUI) {
    MacAddress mac(test_mac_bytes_);
    uint32_t expected_oui = (0xAA << 16) | (0xBB << 8) | 0xCC;
    EXPECT_EQ(mac.getOUI(), expected_oui);
}

TEST_F(MacAddressTest, GetNIC) {
    MacAddress mac(test_mac_bytes_);
    uint32_t expected_nic = (0xDD << 16) | (0xEE << 8) | 0xFF;
    EXPECT_EQ(mac.getNIC(), expected_nic);
}

TEST_F(MacAddressTest, ToUInt64) {
    MacAddress mac(test_mac_bytes_);
    uint64_t expected = 0xAABBCCDDEEFFULL;
    EXPECT_EQ(mac.toUInt64(), expected);
}

TEST_F(MacAddressTest, FromUInt64) {
    uint64_t value = 0xAABBCCDDEEFFULL;
    MacAddress mac = MacAddress::fromUInt64(value);
    EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(MacAddressTest, UInt64RoundTrip) {
    MacAddress original(test_mac_bytes_);
    uint64_t value = original.toUInt64();
    MacAddress restored = MacAddress::fromUInt64(value);
    EXPECT_EQ(original, restored);
}

// Test Iterators
TEST_F(MacAddressTest, RangeBasedForLoop) {
    MacAddress mac(test_mac_bytes_);
    size_t index = 0;
    
    for (uint8_t octet : mac) {
        EXPECT_EQ(octet, test_mac_bytes_[index++]);
    }
    EXPECT_EQ(index, MacAddress::MAC_LENGTH);
}

TEST_F(MacAddressTest, STLAlgorithms) {
    MacAddress mac(test_mac_bytes_);
    
    // Test std::find
    auto it = std::find(mac.begin(), mac.end(), 0xCC);
    EXPECT_NE(it, mac.end());
    EXPECT_EQ(*it, 0xCC);
    
    // Test std::count
    MacAddress all_zeros;
    EXPECT_EQ(std::count(all_zeros.begin(), all_zeros.end(), 0), 6);
}

// Test Comparison Operators
TEST_F(MacAddressTest, EqualityOperator) {
    MacAddress mac1(test_mac_bytes_);
    MacAddress mac2(test_mac_bytes_);
    MacAddress mac3(zero_mac_);
    
    EXPECT_TRUE(mac1 == mac2);
    EXPECT_FALSE(mac1 == mac3);
}

TEST_F(MacAddressTest, InequalityOperator) {
    MacAddress mac1(test_mac_bytes_);
    MacAddress mac2(zero_mac_);
    
    EXPECT_TRUE(mac1 != mac2);
    EXPECT_FALSE(mac1 != mac1);
}

TEST_F(MacAddressTest, LessThanOperator) {
    MacAddress mac1({0x00, 0x11, 0x22, 0x33, 0x44, 0x55});
    MacAddress mac2({0x00, 0x11, 0x22, 0x33, 0x44, 0x56});
    
    EXPECT_TRUE(mac1 < mac2);
    EXPECT_FALSE(mac2 < mac1);
}

TEST_F(MacAddressTest, ComparisonOperators) {
    MacAddress mac1({0x00, 0x11, 0x22, 0x33, 0x44, 0x55});
    MacAddress mac2({0x00, 0x11, 0x22, 0x33, 0x44, 0x56});
    MacAddress mac3(mac1);
    
    EXPECT_TRUE(mac1 < mac2);
    EXPECT_TRUE(mac1 <= mac2);
    EXPECT_TRUE(mac1 <= mac3);
    EXPECT_TRUE(mac2 > mac1);
    EXPECT_TRUE(mac2 >= mac1);
    EXPECT_TRUE(mac1 >= mac3);
}

// Test Stream Operators
TEST_F(MacAddressTest, OutputStreamOperator) {
    MacAddress mac(test_mac_bytes_);
    std::ostringstream oss;
    oss << mac;
    EXPECT_EQ(oss.str(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(MacAddressTest, InputStreamOperator) {
    std::istringstream iss("AA:BB:CC:DD:EE:FF");
    MacAddress mac;
    iss >> mac;
    
    EXPECT_FALSE(iss.fail());
    EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(MacAddressTest, InputStreamOperatorInvalid) {
    std::istringstream iss("invalid_mac");
    MacAddress mac;
    iss >> mac;
    
    EXPECT_TRUE(iss.fail());
}

// Test Hash Function
TEST_F(MacAddressTest, HashFunction) {
    MacAddress mac1(test_mac_bytes_);
    MacAddress mac2(test_mac_bytes_);
    MacAddress mac3(zero_mac_);
    
    std::hash<MacAddress> hasher;
    
    EXPECT_EQ(hasher(mac1), hasher(mac2));
    EXPECT_NE(hasher(mac1), hasher(mac3));
}

TEST_F(MacAddressTest, UnorderedSet) {
    std::unordered_set<MacAddress> mac_set;
    
    MacAddress mac1(test_mac_bytes_);
    MacAddress mac2(zero_mac_);
    MacAddress mac3(broadcast_mac_);
    
    mac_set.insert(mac1);
    mac_set.insert(mac2);
    mac_set.insert(mac3);
    mac_set.insert(mac1); // Duplicate
    
    EXPECT_EQ(mac_set.size(), 3);
    EXPECT_NE(mac_set.find(mac1), mac_set.end());
    EXPECT_NE(mac_set.find(mac2), mac_set.end());
    EXPECT_NE(mac_set.find(mac3), mac_set.end());
}

TEST_F(MacAddressTest, UnorderedMap) {
    std::unordered_map<MacAddress, std::string> mac_map;
    
    MacAddress mac1(test_mac_bytes_);
    MacAddress mac2(zero_mac_);
    
    mac_map[mac1] = "Test Device";
    mac_map[mac2] = "Unknown Device";
    
    EXPECT_EQ(mac_map[mac1], "Test Device");
    EXPECT_EQ(mac_map[mac2], "Unknown Device");
    EXPECT_EQ(mac_map.size(), 2);
}

// Test Edge Cases and Error Handling
TEST_F(MacAddressTest, LargeHexValues) {
    MacAddress mac("FF:FF:FF:FF:FF:FF");
    EXPECT_TRUE(mac.isBroadcast());
    
    for (size_t i = 0; i < MacAddress::MAC_LENGTH; ++i) {
        EXPECT_EQ(mac[i], 0xFF);
    }
}

TEST_F(MacAddressTest, MixedCaseInput) {
    MacAddress mac1("Aa:Bb:Cc:Dd:Ee:Ff");
    MacAddress mac2("AA:BB:CC:DD:EE:FF");
    
    EXPECT_EQ(mac1, mac2);
}

TEST_F(MacAddressTest, SingleDigitHex) {
    MacAddress mac("1:2:3:4:5:6");
    EXPECT_EQ(mac.toString(), "01:02:03:04:05:06");
}

// Performance Tests (Basic)
TEST_F(MacAddressTest, PerformanceConstruction) {
    const int iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        MacAddress mac("AA:BB:CC:DD:EE:FF");
        (void)mac; // Suppress unused variable warning
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // This should complete reasonably quickly (adjust threshold as needed)
    EXPECT_LT(duration.count(), 100000); // Less than 100ms for 10k constructions
}

// Integration Test
TEST_F(MacAddressTest, CompleteWorkflow) {
    // Parse various formats
    std::vector<std::string> mac_strings = {
        "AA:BB:CC:DD:EE:FF",
        "aa-bb-cc-dd-ee-ff",
        "AABB.CCDD.EEFF",
        "aabbccddeeff"
    };
    
    for (const auto& mac_str : mac_strings) {
        MacAddress mac(mac_str);
        
        // Verify parsing
        EXPECT_EQ(mac.toString(), "AA:BB:CC:DD:EE:FF");
        
        // Test properties
        EXPECT_TRUE(mac.isValid());
        EXPECT_FALSE(mac.isZero());
        EXPECT_FALSE(mac.isBroadcast());
        EXPECT_TRUE(mac.isUnicast());
        EXPECT_TRUE(mac.isLocallyAdministered());
        
        // Test conversions
        uint64_t value = mac.toUInt64();
        MacAddress restored = MacAddress::fromUInt64(value);
        EXPECT_EQ(mac, restored);
        
        // Test different output formats
        EXPECT_EQ(mac.toWindowsFormat(), "AA-BB-CC-DD-EE-FF");
        EXPECT_EQ(mac.toCiscoFormat(), "aabb.ccdd.eeff");
        EXPECT_EQ(mac.toStringLower(), "aa:bb:cc:dd:ee:ff");
    }
}

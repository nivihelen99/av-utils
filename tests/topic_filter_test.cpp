#include "gtest/gtest.h"
#include "topic_filter.h" // Assuming this will be found by include paths
#include <iostream>
#include <vector>
#include <string>
#include <chrono> // For performance test

// Using namespace for convenience in test file
using namespace aos_utils;

TEST(TopicFilterTest, BasicFunctionality)
{
    TopicFilter filter;

    // Test exact matches
    filter.addExactMatch("VLAN_1000");
    filter.addExactMatch("PORT_CHANNEL_42");

    EXPECT_TRUE(filter.match("VLAN_1000"));
    EXPECT_TRUE(filter.match("PORT_CHANNEL_42"));
    EXPECT_FALSE(filter.match("VLAN_1001"));

    // Test prefix matches
    filter.addPrefixMatch("Ethernet*"); // Becomes "Ethernet"
    filter.addPrefixMatch("PortChannel"); // Stays "PortChannel"

    EXPECT_TRUE(filter.match("Ethernet0"));
    EXPECT_TRUE(filter.match("Ethernet1/1/1"));
    EXPECT_TRUE(filter.match("PortChannel1"));
    EXPECT_FALSE(filter.match("FastEthernet0")); // Does not start with "PortChannel"
    EXPECT_FALSE(filter.match("Port")); // Does not match "PortChannel" fully

    // Test range matches
    filter.addRangeMatch("VLAN", 1, 100); // Becomes "VLAN_"
    filter.addRangeMatch("Interface", 1000, 2000); // Becomes "Interface_"

    EXPECT_TRUE(filter.match("VLAN_1"));
    EXPECT_TRUE(filter.match("VLAN_100"));
    EXPECT_FALSE(filter.match("VLAN_101"));
    EXPECT_TRUE(filter.match("Interface_1500"));
    EXPECT_FALSE(filter.match("Interface_2001"));
    EXPECT_FALSE(filter.match("VLAN_0")); // Below range
    EXPECT_FALSE(filter.match("VLAN_")); // No number
    EXPECT_FALSE(filter.match("VLAN_abc")); // Not a number
}

TEST(TopicFilterTest, RegexFunctionality)
{
    TopicFilter filter;

    // Test regex full match patterns
    filter.addRegexMatch(R"(VLAN_[0-9]+)", TopicFilter::RegexMode::MATCH);
    filter.addRegexMatch(R"(Ethernet[0-9]+/[0-9]+)", TopicFilter::RegexMode::MATCH);
    filter.addRegexMatch(R"(PortChannel[0-9]{1,3})", TopicFilter::RegexMode::MATCH);

    // Test VLAN regex
    EXPECT_TRUE(filter.match("VLAN_1"));
    EXPECT_TRUE(filter.match("VLAN_1234"));
    EXPECT_FALSE(filter.match("VLAN_"));
    EXPECT_FALSE(filter.match("VLAN_abc"));
    EXPECT_FALSE(filter.match("VLAN_1_extra")); // Full match, so extra text fails

    // Test Ethernet regex
    EXPECT_TRUE(filter.match("Ethernet1/1"));
    EXPECT_TRUE(filter.match("Ethernet99/255"));
    EXPECT_FALSE(filter.match("Ethernet1"));
    EXPECT_FALSE(filter.match("Ethernet1/1/1"));

    // Test PortChannel regex (1-3 digits)
    EXPECT_TRUE(filter.match("PortChannel1"));
    EXPECT_TRUE(filter.match("PortChannel999"));
    EXPECT_FALSE(filter.match("PortChannel1000")); // 4 digits
    EXPECT_FALSE(filter.match("PortChannel"));

    // Test regex search patterns
    filter.addRegexMatch(R"([A-Z]+_[0-9]+)", TopicFilter::RegexMode::SEARCH);

    EXPECT_TRUE(filter.match("prefix_ABC_123_suffix")); // Search finds ABC_123
    EXPECT_TRUE(filter.match("XYZ_456"));
    EXPECT_FALSE(filter.match("abc_123")); // Lowercase doesn't match [A-Z]+

    // Test case-insensitive regex
    filter.addRegexMatch(
        R"(user_[a-z]+)", TopicFilter::RegexMode::MATCH, std::regex_constants::icase);

    EXPECT_TRUE(filter.match("user_john"));
    EXPECT_TRUE(filter.match("USER_JOHN")); // Case insensitive
    EXPECT_TRUE(filter.match("User_John"));
}

TEST(TopicFilterTest, EdgeCases)
{
    TopicFilter filter;

    // Test invalid regex patterns
    EXPECT_THROW(filter.addRegexMatch("[invalid"), std::invalid_argument);

    // Test empty regex pattern
    EXPECT_THROW(filter.addRegexMatch(""), std::invalid_argument);

    // Test empty exact match pattern
    EXPECT_THROW(filter.addExactMatch(""), std::invalid_argument);

    // Test empty prefix match pattern
    EXPECT_THROW(filter.addPrefixMatch(""), std::invalid_argument);

    // Test empty prefix for range match
    EXPECT_THROW(filter.addRangeMatch("", 0, 1), std::invalid_argument);

    // Test invalid range (start > end)
    EXPECT_THROW(filter.addRangeMatch("PREFIX", 10, 0), std::invalid_argument);


    // Test complex regex patterns
    filter.addRegexMatch(R"(^(VLAN|INTERFACE)_([0-9]{1,4})_(CONFIG|STATUS)$)",
                         TopicFilter::RegexMode::MATCH);

    EXPECT_TRUE(filter.match("VLAN_1_CONFIG"));
    EXPECT_TRUE(filter.match("INTERFACE_9999_STATUS"));
    EXPECT_FALSE(filter.match("VLAN_12345_CONFIG")); // Too many digits
    EXPECT_FALSE(filter.match("VLAN_1_INVALID"));   // Invalid suffix
}

// This is a longer running test, but provides a basic performance check.
TEST(TopicFilterTest, PerformanceTest)
{
    TopicFilter filter;

    const int NUM_RULES = 1000;
    // Estimate NUM_RULES / 10 for regex match rules, 0 for search rules in this test
    filter.reserve(NUM_RULES, NUM_RULES, NUM_RULES, NUM_RULES / 10, 0);

    // Add different types of rules
    for (int i = 0; i < NUM_RULES; ++i)
    {
        filter.addExactMatch("EXACT_" + std::to_string(i));
        filter.addPrefixMatch("PREFIX_" + std::to_string(i)); // Already handles '*' if any
        filter.addRangeMatch("RANGE_" + std::to_string(i), i * 100, (i + 1) * 100 -1); // Ensure end is inclusive

        // Add some regex rules
        if (i % 10 == 0)
        {
            filter.addRegexMatch(R"(REGEX_)" + std::to_string(i) + R"(_[0-9]+)",
                                 TopicFilter::RegexMode::MATCH);
        }
    }

    filter.optimize();

    const int NUM_TESTS = 10000;
    std::vector<std::string> test_keys;
    test_keys.reserve(NUM_TESTS);

    for (int i = 0; i < NUM_TESTS; ++i)
    {
        switch (i % 4)
        {
        case 0:
            test_keys.push_back("EXACT_" + std::to_string(i % NUM_RULES));
            break;
        case 1:
            test_keys.push_back("PREFIX_" + std::to_string(i % NUM_RULES) + "_suffix");
            break;
        case 2:
            test_keys.push_back("RANGE_" + std::to_string(i % NUM_RULES) + "_" +
                                std::to_string((i % NUM_RULES) * 100 + 50));
            break;
        case 3:
            // Ensure this regex key is one that was added
            test_keys.push_back("REGEX_" + std::to_string((i % (NUM_RULES / 10)) * 10) + "_123");
            break;
        }
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    int matches = 0;
    for (const auto& key : test_keys)
    {
        if (filter.match(key))
        {
            ++matches;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    TopicFilter::Statistics stats = filter.getStatistics();

    std::cout << "Performance Test Results (Gtest):\n";
    std::cout << "  Total rules: " << stats.total_rules << "\n";
    std::cout << "    Exact: " << stats.exact_rules << "\n";
    std::cout << "    Prefix: " << stats.prefix_rules << "\n";
    std::cout << "    Range: " << stats.range_rules << "\n";
    std::cout << "    Regex (match): " << stats.regex_match_rules << "\n";
    std::cout << "    Regex (search): " << stats.regex_search_rules << "\n";
    std::cout << "  Tests: " << NUM_TESTS << "\n";
    std::cout << "  Matches found: " << matches << "\n";
    std::cout << "  Time: " << duration.count() << " microseconds\n";
    if (NUM_TESTS > 0) {
        std::cout << "  Average: " << static_cast<double>(duration.count()) / NUM_TESTS << " \xC2\xB5s per match call\n";
    }
    // Basic check to ensure some matches were found, indicating test ran as expected.
    // The exact number of matches can be sensitive to test data generation.
    EXPECT_GT(matches, 0);
    // Also check that not all keys matched, implying filter is selective.
    if (NUM_TESTS > 0) { // Avoid division by zero or issues if NUM_TESTS is 0
        EXPECT_LT(matches, NUM_TESTS);
    }
}

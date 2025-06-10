
#include <iostream>
#include <chrono>
#include "topic_filter.h"

using namespace aos_utils;

// Test and benchmark code
class TopicFilterTester
{
   public:
   static void runAllTests()
   {
      std::cout << "Running TopicFilter tests (with Regex support)...\n";

      testBasicFunctionality();
      testRegexFunctionality();
      testEdgeCases();
      testPerformance();

      std::cout << "All tests passed!\n";
   }

   private:
   static void testBasicFunctionality()
   {
      std::cout << "Testing basic functionality...\n";

      TopicFilter filter;

      // Test exact matches
      filter.addExactMatch("VLAN_1000");
      filter.addExactMatch("PORT_CHANNEL_42");

      assert(filter.match("VLAN_1000"));
      assert(filter.match("PORT_CHANNEL_42"));
      assert(!filter.match("VLAN_1001"));

      // Test prefix matches
      filter.addPrefixMatch("Ethernet*");
      filter.addPrefixMatch("PortChannel");

      assert(filter.match("Ethernet0"));
      assert(filter.match("Ethernet1/1/1"));
      assert(filter.match("PortChannel1"));
      assert(!filter.match("FastEthernet0"));

      // Test range matches
      filter.addRangeMatch("VLAN", 1, 100);
      filter.addRangeMatch("Interface", 1000, 2000);

      assert(filter.match("VLAN_1"));
      assert(filter.match("VLAN_100"));
      assert(!filter.match("VLAN_101"));
      assert(filter.match("Interface_1500"));
      assert(!filter.match("Interface_2001"));

      std::cout << "Basic functionality tests passed.\n";
   }

   static void testRegexFunctionality()
   {
      std::cout << "Testing regex functionality...\n";

      TopicFilter filter;

      // Test regex full match patterns
      filter.addRegexMatch(R"(VLAN_[0-9]+)", TopicFilter::RegexMode::MATCH);
      filter.addRegexMatch(R"(Ethernet[0-9]+/[0-9]+)", TopicFilter::RegexMode::MATCH);
      filter.addRegexMatch(R"(PortChannel[0-9]{1,3})", TopicFilter::RegexMode::MATCH);

      // Test VLAN regex
      assert(filter.match("VLAN_1"));
      assert(filter.match("VLAN_1234"));
      assert(!filter.match("VLAN_"));
      assert(!filter.match("VLAN_abc"));
      assert(!filter.match("VLAN_1_extra")); // Full match, so extra text fails

      // Test Ethernet regex
      assert(filter.match("Ethernet1/1"));
      assert(filter.match("Ethernet99/255"));
      assert(!filter.match("Ethernet1"));
      assert(!filter.match("Ethernet1/1/1"));

      // Test PortChannel regex (1-3 digits)
      assert(filter.match("PortChannel1"));
      assert(filter.match("PortChannel999"));
      assert(!filter.match("PortChannel1000")); // 4 digits
      assert(!filter.match("PortChannel"));

      // Test regex search patterns
      filter.addRegexMatch(R"([A-Z]+_[0-9]+)", TopicFilter::RegexMode::SEARCH);

      assert(filter.match("prefix_ABC_123_suffix")); // Search finds ABC_123
      assert(filter.match("XYZ_456"));
      assert(!filter.match("abc_123")); // Lowercase doesn't match [A-Z]+

      // Test case-insensitive regex
      filter.addRegexMatch(
         R"(user_[a-z]+)", TopicFilter::RegexMode::MATCH, std::regex_constants::icase);

      assert(filter.match("user_john"));
      assert(filter.match("USER_JOHN")); // Case insensitive
      assert(filter.match("User_John"));

      std::cout << "Regex functionality tests passed.\n";
   }

   static void testEdgeCases()
   {
      std::cout << "Testing edge cases...\n";

      TopicFilter filter;

      // Test invalid regex patterns
      try
      {
         filter.addRegexMatch("[invalid"); // Missing closing bracket
         assert(false && "Should throw exception for invalid regex");
      }
      catch (const std::invalid_argument&)
      {
         // Expected
      }

      // Test empty regex pattern
      try
      {
         filter.addRegexMatch("");
         assert(false && "Should throw exception for empty regex");
      }
      catch (const std::invalid_argument&)
      {
         // Expected
      }

      // Test complex regex patterns
      filter.addRegexMatch(R"(^(VLAN|INTERFACE)_([0-9]{1,4})_(CONFIG|STATUS)$)",
                           TopicFilter::RegexMode::MATCH);

      assert(filter.match("VLAN_1_CONFIG"));
      assert(filter.match("INTERFACE_9999_STATUS"));
      assert(!filter.match("VLAN_12345_CONFIG")); // Too many digits
      assert(!filter.match("VLAN_1_INVALID")); // Invalid suffix

      std::cout << "Edge case tests passed.\n";
   }

   static void testPerformance()
   {
      std::cout << "Testing performance (including regex)...\n";

      TopicFilter filter;

      const int NUM_RULES = 1000; // Reduced for regex performance
      // Estimate NUM_RULES / 10 for regex match rules, 0 for search rules in this test
      filter.reserve(NUM_RULES, NUM_RULES, NUM_RULES, NUM_RULES / 10, 0);

      // Add different types of rules
      for (int i = 0; i < NUM_RULES; ++i)
      {
         filter.addExactMatch("EXACT_" + std::to_string(i));
         filter.addPrefixMatch("PREFIX_" + std::to_string(i));
         filter.addRangeMatch("RANGE_" + std::to_string(i), i * 100, (i + 1) * 100);

         // Add some regex rules (fewer due to performance cost)
         if (i % 10 == 0)
         {
            filter.addRegexMatch(R"(REGEX_)" + std::to_string(i) + R"(_[0-9]+)",
                                 TopicFilter::RegexMode::MATCH);
         }
      }

      filter.optimize();

      // Performance test
      const int NUM_TESTS = 10000; // Reduced due to regex overhead
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
               test_keys.push_back("REGEX_" + std::to_string((i % (NUM_RULES / 10)) * 10) + "_123");
               break;
         }
      }

      auto start = std::chrono::high_resolution_clock::now();

      int matches = 0;
      for (const auto& key : test_keys)
      {
         if (filter.match(key))
         {
            ++matches;
         }
      }

      auto end = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

      auto stats = filter.getStatistics();

      std::cout << "Performance test results:\n";
      std::cout << "  Total rules: " << stats.total_rules << "\n";
      std::cout << "    Exact: " << stats.exact_rules << "\n";
      std::cout << "    Prefix: " << stats.prefix_rules << "\n";
      std::cout << "    Range: " << stats.range_rules << "\n";
      std::cout << "    Regex (match): " << stats.regex_match_rules << "\n";
      std::cout << "    Regex (search): " << stats.regex_search_rules << "\n";
      std::cout << "  Tests: " << NUM_TESTS << "\n";
      std::cout << "  Matches: " << matches << "\n";
      std::cout << "  Time: " << duration.count() << " microseconds\n";
      std::cout << "  Average: " << (double) duration.count() / NUM_TESTS << " �s per match\n";

      std::cout << "Performance test completed.\n";
   }
};

// Example usage
int main()
{
   std::cout << "=== TopicFilter with Regex Support Demo ===\n\n";

   // Run comprehensive tests
   TopicFilterTester::runAllTests();

   std::cout << "\n=== Example Usage with Regex ===\n";

   TopicFilter filter;

   // Traditional rules
   filter.addExactMatch("SYSTEM_RELOAD");
   filter.addPrefixMatch("Ethernet*");
   filter.addRangeMatch("VLAN", 1, 4094);

   // Regex rules for complex patterns
   filter.addRegexMatch(R"(Ethernet[0-9]+/[0-9]+/[0-9]+)", TopicFilter::RegexMode::MATCH);
   filter.addRegexMatch(R"(PortChannel[0-9]{1,3})", TopicFilter::RegexMode::MATCH);
   filter.addRegexMatch(R"(INTERFACE_[A-Z]+_[0-9]+_(UP|DOWN))", TopicFilter::RegexMode::MATCH);
   filter.addRegexMatch(R"(ERROR|WARN)", TopicFilter::RegexMode::SEARCH); // Search anywhere

   // Case-insensitive pattern for user events
   filter.addRegexMatch(
      R"(user_[a-z]+_login)", TopicFilter::RegexMode::MATCH, std::regex_constants::icase);

   filter.optimize();

   std::vector<std::string> test_keys = {
      "SYSTEM_RELOAD", // Exact match
      "Ethernet0", // Prefix match
      "Ethernet1/2/3", // Regex match (full format)
      "VLAN_100", // Range match
      "PortChannel42", // Regex match
      "INTERFACE_GIG_1_UP", // Regex match
      "INTERFACE_GIG_1_DOWN", // Regex match
      "some_ERROR_message", // Regex search
      "warning_WARN_alert", // Regex search
      "user_john_login", // Regex match (case insensitive)
      "USER_JANE_LOGIN", // Regex match (case insensitive)
      "Ethernet1/2", // No match (incomplete format)
      "PortChannel1000", // No match (too many digits)
      "random_key", // No match
      "INTERFACE_GIG_1_UNKNOWN" // No match (invalid status)
   };

   std::cout << "Testing various patterns:\n";
   for (const auto& key : test_keys)
   {
      bool matches = filter.match(key);
      std::cout << "  " << key << ": " << (matches ? "MATCH" : "NO MATCH") << "\n";
   }

   auto stats = filter.getStatistics();
   std::cout << "\nFilter statistics:\n";
   std::cout << "  Total rules: " << stats.total_rules << "\n";
   std::cout << "  Exact matches: " << stats.exact_rules << "\n";
   std::cout << "  Prefix matches: " << stats.prefix_rules << "\n";
   std::cout << "  Range matches: " << stats.range_rules << "\n";
   std::cout << "  Regex full matches: " << stats.regex_match_rules << "\n";
   std::cout << "  Regex search patterns: " << stats.regex_search_rules << "\n";

   return 0;
}

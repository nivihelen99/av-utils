#include "gtest/gtest.h"
#include "gmock/gmock.h" // Added for GMock features
#include "../include/arp_cache.h" // Adjust path as needed
#include <vector>
#include <array>
#include <iostream> // For std::cerr for warnings, if not captured by test

// MockARPCache to allow mocking of virtual methods from ARPCache
class MockARPCache : public ARPCache {
public:
    // Constructor must call the base class constructor
    MockARPCache(const mac_addr_t& dev_mac) : ARPCache(dev_mac) {}

    // Mock for the virtual send_arp_request method (must be virtual in ARPCache)
    MOCK_METHOD(void, send_arp_request, (uint32_t ip), (override));

    // Mock for the virtual log_ip_conflict method (must be virtual in ARPCache)
    MOCK_METHOD(void, log_ip_conflict, (uint32_t ip, const mac_addr_t& existing_mac, const mac_addr_t& new_mac), (override));
};

TEST(ARPCacheTest, AddAndLookup) {
    mac_addr_t dev_mac = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    MockARPCache cache(dev_mac); // Use MockARPCache

    uint32_t ip1 = 0xC0A80101; // 192.168.1.1
    mac_addr_t mac1 = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01};
    mac_addr_t mac_out;

    // Test lookup miss - Expect send_arp_request to be called
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    ASSERT_FALSE(cache.lookup(ip1, mac_out));

    // Test add_entry and lookup hit
    cache.add_entry(ip1, mac1);
    // No send_arp_request expected here as it should be a hit
    EXPECT_CALL(cache, send_arp_request(testing::_)).Times(0); // Ensure no unexpected calls
    ASSERT_TRUE(cache.lookup(ip1, mac_out));
    ASSERT_EQ(mac_out, mac1);
    testing::Mock::VerifyAndClearExpectations(&cache); // Clear expectations for this part
}

TEST(ARPCacheTest, GratuitousARP) {
    mac_addr_t dev_mac = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    MockARPCache cache(dev_mac); // Use MockARPCache to check log_ip_conflict

    uint32_t ip1 = 0xC0A80101; // 192.168.1.1
    mac_addr_t mac1 = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01};
    mac_addr_t mac2 = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x02}; // Different MAC

    cache.add_entry(ip1, mac1); // Initial entry

    // Adding same IP, same MAC - no conflict log expected
    EXPECT_CALL(cache, log_ip_conflict(testing::_, testing::_, testing::_)).Times(0);
    cache.add_entry(ip1, mac1);
    testing::Mock::VerifyAndClearExpectations(&cache);

    // Adding same IP, different MAC - conflict log expected
    EXPECT_CALL(cache, log_ip_conflict(ip1, mac1, mac2)).Times(1);
    cache.add_entry(ip1, mac2);

    mac_addr_t mac_out;
    ASSERT_TRUE(cache.lookup(ip1, mac_out)); // Should now find mac2
    ASSERT_EQ(mac_out, mac2);
    testing::Mock::VerifyAndClearExpectations(&cache);
}

TEST(ARPCacheTest, ProxyARP) {
    mac_addr_t dev_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    MockARPCache cache(dev_mac); // Use mock to check send_arp_request calls

    uint32_t proxy_ip_prefix = 0xC0A80A00; // 192.168.10.0
    uint32_t proxy_subnet_mask = 0xFFFFFF00; // 255.255.255.0
    cache.add_proxy_subnet(proxy_ip_prefix, proxy_subnet_mask);

    uint32_t ip_in_proxy_subnet = 0xC0A80A05; // 192.168.10.5
    uint32_t ip_not_in_proxy_subnet = 0xC0A80B05; // 192.168.11.5
    mac_addr_t mac_out;

    // Test lookup for IP in proxy subnet (not in cache) - no ARP request expected for proxy
    EXPECT_CALL(cache, send_arp_request(testing::_)).Times(0);
    ASSERT_TRUE(cache.lookup(ip_in_proxy_subnet, mac_out));
    ASSERT_EQ(mac_out, dev_mac);
    testing::Mock::VerifyAndClearExpectations(&cache);

    // Verify it was added to cache, so still no ARP request
    EXPECT_CALL(cache, send_arp_request(testing::_)).Times(0);
    ASSERT_TRUE(cache.lookup(ip_in_proxy_subnet, mac_out));
    ASSERT_EQ(mac_out, dev_mac);
    testing::Mock::VerifyAndClearExpectations(&cache);

    // Test lookup for IP not in proxy subnet (not in cache) - ARP request expected
    EXPECT_CALL(cache, send_arp_request(ip_not_in_proxy_subnet)).Times(1);
    ASSERT_FALSE(cache.lookup(ip_not_in_proxy_subnet, mac_out));
    testing::Mock::VerifyAndClearExpectations(&cache);
}

// Note: The failover tests below are still somewhat conceptual without
// direct state manipulation helpers in ARPCache (e.g., force_set_state, force_set_probe_count)
// or precise time control for aging. We test what's possible via the public API.

TEST(ARPCacheTest, FastFailoverInLookupIfStale) {
    mac_addr_t dev_mac = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    MockARPCache cache(dev_mac);

    uint32_t ip1 = 0xC0A80101;
    mac_addr_t mac1_primary = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01};
    mac_addr_t mac2_backup = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x02};
    mac_addr_t mac_out;

    cache.add_entry(ip1, mac1_primary);
    cache.add_backup_mac(ip1, mac2_backup);

    ASSERT_TRUE(cache.lookup(ip1, mac_out) && mac_out == mac1_primary);

    GTEST_LOG_(INFO) << "FastFailoverInLookupIfStale test is conceptual for forcing STALE state.";
}


TEST(ARPCacheTest, FailoverInAgeEntriesAfterMaxProbes) {
    mac_addr_t dev_mac = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    MockARPCache cache(dev_mac); // Use MockARPCache for expectations

    uint32_t ip1 = 0xC0A80102; // Using a different IP to avoid interference from other tests
    mac_addr_t mac1_primary_dummy = {}; // Will be written to by lookup, actual value not critical for this test setup
    mac_addr_t mac1_backup  = {0x00, 0x11, 0x22, 0x33, 0x44, 0xBB};
    mac_addr_t mac_out;

    // Add backup MAC. If ip1 doesn't have an entry yet, add_backup_mac handles it gracefully.
    cache.add_backup_mac(ip1, mac1_backup);

    // 1. Initial lookup for a non-existent IP. This should:
    //    - Create an INCOMPLETE entry for ip1.
    //    - Call send_arp_request once for this initial attempt.
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    ASSERT_FALSE(cache.lookup(ip1, mac1_primary_dummy));
    testing::Mock::VerifyAndClearExpectations(&cache);

    // Ensure no IP conflict logs are expected during probing of this entry
    EXPECT_CALL(cache, log_ip_conflict(testing::_, testing::_, testing::_)).Times(0);

    // 2. Simulate subsequent probes via age_entries.
    //    The entry for ip1 is now INCOMPLETE. ARPCache::MAX_PROBES is the total number of probes.
    //    One probe was already sent by the initial lookup.
    //    So, expect (ARPCache::MAX_PROBES - 1) more calls to send_arp_request via age_entries.
    int remaining_probes = ARPCache::MAX_PROBES - 1;
    if (remaining_probes < 0) remaining_probes = 0;

    if (remaining_probes > 0) {
        EXPECT_CALL(cache, send_arp_request(ip1)).Times(remaining_probes);
    }
    // else if MAX_PROBES is 1, no more calls from age_entries before failover/erase.

    auto test_time = std::chrono::steady_clock::now();
    // Advance time slightly to ensure the first age_entries call processes the INCOMPLETE entry
    // if its timestamp was set to 'now' by the preceding lookup.
    test_time += ARPCache::PROBE_RETRANSMIT_INTERVAL + std::chrono::milliseconds(10);

    for (int i = 0; i < remaining_probes; ++i) {
        cache.age_entries(test_time); // Use parameterized version
        // Advance time for the next probe interval
        test_time += ARPCache::PROBE_RETRANSMIT_INTERVAL + std::chrono::milliseconds(10);
    }
    testing::Mock::VerifyAndClearExpectations(&cache);

    // 3. The next call to age_entries.
    //    At this stage, probe_count should be ARPCache::MAX_PROBES.
    //    This call to age_entries, using the advanced test_time, should find the entry
    //    INCOMPLETE, timed out, and with probe_count at MAX_PROBES, triggering failover.
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(0);
    // std::cerr << "GTEST_NOTE: Expecting an INFO log for failover (ARP age_entries) on next age_entries(" << test_time.time_since_epoch().count() << ") call." << std::endl;
    cache.age_entries(test_time); // Use parameterized version for the failover-triggering call
    testing::Mock::VerifyAndClearExpectations(&cache);

    // 4. Verify failover: Looking up ip1 should now yield the backup MAC and be REACHABLE.
    //    The lookup itself should not trigger send_arp_request now.
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(0);
    ASSERT_TRUE(cache.lookup(ip1, mac_out)) << "Lookup failed after expected failover.";
    ASSERT_EQ(mac_out, mac1_backup) << "MAC address did not match backup MAC after failover.";

    // Optional: Verify the state is REACHABLE (needs a getter or friend class for ARPEntry state)
    // EXPECT_EQ(get_arp_entry_state_for_test(cache, ip1), ARPCache::ARPState::REACHABLE);
    testing::Mock::VerifyAndClearExpectations(&cache);
}

// Note: Some tests above are marked as conceptual or requiring ARPCache modifications
// for full testability (e.g., forcing entry states, mocking non-virtual methods,
// or checking stderr output without a test framework's capture abilities).
// The existing assertions primarily check the cache's state after operations.

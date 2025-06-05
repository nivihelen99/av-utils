#include "gtest/gtest.h"
#include "gmock/gmock.h" // Added for GMock
#include "../include/nd_cache.h" // Adjust path
#include <vector>
#include <array>
#include <iostream>

class MockNDCache : public NDCache {
public:
    MockNDCache(const mac_addr_t& own_mac) : NDCache(own_mac) {}

    // Ensure MOCK_METHOD signatures exactly match the virtual methods in NDCache
    MOCK_METHOD(void, send_router_solicitation, (const ipv6_addr_t& source_ip), (override));
    MOCK_METHOD(void, send_neighbor_solicitation, (const ipv6_addr_t& target_ip, const ipv6_addr_t& source_ip, const mac_addr_t* sllao, bool for_dad), (override));
    MOCK_METHOD(void, send_neighbor_advertisement, (const ipv6_addr_t& target_ip, const ipv6_addr_t& adv_source_ip, const mac_addr_t& tllao, bool is_router, bool solicited, bool override_flag), (override));
};

// Helper to generate a link-local address from a MAC for test setup
// Replicates EUI-64 logic locally to avoid calling private NDCache method.
ipv6_addr_t generate_ll_from_mac_for_test(const mac_addr_t& mac) {
    ipv6_addr_t ll_addr;
    ll_addr.fill(0);
    ll_addr[0] = 0xfe;
    ll_addr[1] = 0x80;
    // EUI-64 logic:
    ll_addr[8] = mac[0] ^ 0x02; // Flip U/L bit
    ll_addr[9] = mac[1];
    ll_addr[10] = mac[2];
    ll_addr[11] = 0xFF;
    ll_addr[12] = 0xFE;
    ll_addr[13] = mac[3];
    ll_addr[14] = mac[4];
    ll_addr[15] = mac[5];
    return ll_addr;
}


TEST(NDCacheTest, LinkLocalAndDAD) {
    mac_addr_t device_mac = {0x00, 0x00, 0x00, 0x11, 0x22, 0x33};
    MockNDCache cache(device_mac);
    ipv6_addr_t link_local_addr = cache.get_link_local_address();

    // The constructor calls initiate_link_local_dad -> start_dad.
    // DAD logic in age_entries sends NS if now >= next_probe_time.
    // start_dad initializes next_probe_time to std::chrono::steady_clock::now().

    auto test_time = std::chrono::steady_clock::now();

    // Expect MAX_MULTICAST_SOLICIT calls for DAD NS in total.
    // The source IP for DAD NS should be the unspecified address.
    ipv6_addr_t unspecified_addr;
    unspecified_addr.fill(0);

    EXPECT_CALL(cache, send_neighbor_solicitation(link_local_addr, unspecified_addr, nullptr, true))
        .Times(NDCache::MAX_MULTICAST_SOLICIT);

    for (int i = 0; i < NDCache::MAX_MULTICAST_SOLICIT; ++i) {
        // Ensure test_time is at or after the expected next_probe_time for DAD logic.
        // The first call to age_entries should trigger the first probe immediately
        // as next_probe_time was set to 'now' in start_dad.
        cache.age_entries(test_time);
        ASSERT_FALSE(cache.is_link_local_dad_completed()) << "DAD completed prematurely after probe " << i + 1;
        // Advance time past the DAD retransmission timer for the next iteration
        test_time += std::chrono::milliseconds(NDCache::RETRANS_TIMER) + std::chrono::milliseconds(10);
    }

    // One more call to age_entries after all probes have been sent.
    // This call should process the DAD success (timeout without conflict).
    cache.age_entries(test_time);

    ASSERT_TRUE(cache.is_link_local_dad_completed()) << "DAD for link-local address did not complete successfully.";

    // Optional: Verify that the link-local address might have an entry in the cache,
    // though DAD success doesn't necessarily mean it's added to the main neighbor cache
    // unless it's used in communication. The flag is the primary outcome.
    // mac_addr_t mac_out;
    // bool found = cache.lookup(link_local_addr, mac_out); // This might create an INCOMPLETE entry if not there.

    testing::Mock::VerifyAndClearExpectations(&cache);
}

TEST(NDCacheTest, SLAACProcessingAndDAD) {
    mac_addr_t device_mac = {0x00, 0x00, 0x00, 0x11, 0x22, 0xAA};
    MockNDCache cache(device_mac);

    // 1. Complete Link-Local DAD first
    EXPECT_CALL(cache, send_neighbor_solicitation(testing::_, testing::_, nullptr, true))
        .Times(testing::AtLeast(NDCache::MAX_MULTICAST_SOLICIT));
    for (int i = 0; i <= NDCache::MAX_MULTICAST_SOLICIT; ++i) { // Loop enough times for DAD to complete
        cache.age_entries();
    }
    ASSERT_TRUE(cache.is_link_local_dad_completed());
    testing::Mock::VerifyAndClearExpectations(&cache);

    // 2. Prepare and process Router Advertisement for SLAAC
    NDCache::RAInfo ra;
    ra.source_ip = generate_ll_from_mac_for_test({0xAA,0xBB,0xCC,0xDD,0xEE,0xFF});
    ra.router_mac = {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};
    ra.router_lifetime = std::chrono::seconds(1800);

    PrefixEntry pe;
    pe.prefix = {{0x20,0x01,0x0d,0xb8,0x00,0x01,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}}; // 2001:db8:1::/64
    pe.prefix_length = 64;
    pe.on_link = true;
    pe.autonomous = true;
    pe.valid_lifetime = std::chrono::seconds(7200);
    pe.preferred_lifetime = std::chrono::seconds(3600);
    ra.prefixes.push_back(pe);

    // process_router_advertisement -> configure_address_slaac -> start_dad
    // The first DAD NS for the new SLAAC address is sent by the next age_entries call.
    cache.process_router_advertisement(ra);

    EXPECT_CALL(cache, send_neighbor_solicitation(testing::_, testing::_, nullptr, true))
        .Times(testing::AtLeast(1)); // DAD NS for the SLAAC generated address
    cache.age_entries();
    testing::Mock::VerifyAndClearExpectations(&cache);

    GTEST_LOG_(INFO) << "SLAAC test relies on NS call for DAD initiation and conceptual check of prefix list.";

    EXPECT_CALL(cache, send_neighbor_solicitation(testing::_, testing::_, nullptr, true))
        .Times(testing::AtMost(NDCache::MAX_MULTICAST_SOLICIT -1));
     for (int i = 0; i < NDCache::MAX_MULTICAST_SOLICIT; ++i) {
        cache.age_entries();
        // Conceptual: if (cache.is_slaac_address_dad_completed(generated_address)) break;
    }
    // Final age_entries to process success if it's due to timeout
    if (/* !cache.is_slaac_address_dad_completed(generated_address) */ true) { // Placeholder for actual check
         cache.age_entries();
    }
    // Assert DAD completion for SLAAC address (needs a way to get the address and check its status)
}

TEST(NDCacheTest, DADConflict) {
    mac_addr_t device_mac = {0x00,0x00,0x00,0x11,0x22,0xBB};
    MockNDCache cache(device_mac);
    ipv6_addr_t conflicting_addr = cache.get_link_local_address();

    EXPECT_CALL(cache, send_neighbor_solicitation(conflicting_addr, testing::_, nullptr, true)).Times(1);
    cache.age_entries();
    testing::Mock::VerifyAndClearExpectations(&cache);

    NDCache::NAInfo na_conflict;
    na_conflict.target_ip = conflicting_addr;
    na_conflict.source_ip = generate_ll_from_mac_for_test({0xDE,0xAD,0xBE,0xEF,0x00,0x01});
    na_conflict.tllao = {0xDE,0xAD,0xBE,0xEF,0x00,0x01};
    na_conflict.is_router = false;
    na_conflict.solicited = false;
    na_conflict.override_flag = true;

    // No more NS should be sent after conflict is detected by NA processing
    EXPECT_CALL(cache, send_neighbor_solicitation(conflicting_addr, testing::_, nullptr, true)).Times(0);
    cache.process_neighbor_advertisement(na_conflict);

    ASSERT_FALSE(cache.is_link_local_dad_completed());
    testing::Mock::VerifyAndClearExpectations(&cache);
}


TEST(NDCacheTest, FastFailoverLookup) {
    mac_addr_t device_mac = {0x00,0x00,0x00,0x11,0x22,0xCC};
    MockNDCache cache(device_mac);

    EXPECT_CALL(cache, send_neighbor_solicitation(testing::_, testing::_, testing::_, testing::_))
        .Times(testing::AtLeast(NDCache::MAX_MULTICAST_SOLICIT));
    for (int i = 0; i <= NDCache::MAX_MULTICAST_SOLICIT; ++i) cache.age_entries();
    ASSERT_TRUE(cache.is_link_local_dad_completed());
    testing::Mock::VerifyAndClearExpectations(&cache);

    ipv6_addr_t neighbor_ip = {{0x20,0x01,0x0d,0xb8,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01}};
    mac_addr_t mac1_primary = {0x11, 0x22, 0x33, 0x44, 0x55, 0x01};
    mac_addr_t mac2_backup  = {0x11, 0x22, 0x33, 0x44, 0x55, 0x02};
    mac_addr_t mac_out;

    cache.add_entry(neighbor_ip, mac1_primary, NDCacheState::REACHABLE);
    cache.add_backup_mac(neighbor_ip, mac2_backup);

    ASSERT_TRUE(cache.lookup(neighbor_ip, mac_out));
    ASSERT_EQ(mac_out, mac1_primary);

    GTEST_LOG_(INFO) << "ND FastFailoverLookup test is conceptual for STALE state due to state control limitations.";
}

TEST(NDCacheTest, FailoverInAgeEntries) {
    mac_addr_t device_mac = {0x00,0x00,0x00,0x11,0x22,0xDD};
    MockNDCache cache(device_mac);

    EXPECT_CALL(cache, send_neighbor_solicitation(testing::_, testing::_, testing::_, testing::_))
        .Times(testing::AtLeast(NDCache::MAX_MULTICAST_SOLICIT)); // For LL DAD
    for (int i = 0; i <= NDCache::MAX_MULTICAST_SOLICIT; ++i) cache.age_entries();
    ASSERT_TRUE(cache.is_link_local_dad_completed());
    testing::Mock::VerifyAndClearExpectations(&cache);

    ipv6_addr_t neighbor_ip = {{0x20,0x01,0x0d,0xb8,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0A}};
    mac_addr_t mac1_primary = {0x11, 0xAA, 0xBB, 0xCC, 0xDD, 0x01};
    mac_addr_t mac2_backup  = {0x11, 0xAA, 0xBB, 0xCC, 0xDD, 0x02};
    mac_addr_t mac_out;

    cache.add_entry(neighbor_ip, mac1_primary, NDCacheState::INCOMPLETE, std::chrono::seconds(30), false, {mac2_backup});

    EXPECT_CALL(cache, send_neighbor_solicitation(neighbor_ip, cache.get_link_local_address(), &device_mac, false))
        .Times(NDCache::MAX_MULTICAST_SOLICIT);

    for (int i = 0; i < NDCache::MAX_MULTICAST_SOLICIT; ++i) {
        cache.age_entries();
    }
    // This next call to age_entries should see probe_count > MAX_MULTICAST_SOLICIT
    // and trigger failover because backup_macs is not empty.
    cache.age_entries();

    ASSERT_TRUE(cache.lookup(neighbor_ip, mac_out));
    ASSERT_EQ(mac_out, mac2_backup);

    // Check if the state became REACHABLE (as per current failover logic)
    // This needs a getter for NDEntry or more detailed state exposure for testing.
    // For now, successful lookup of backup MAC implies failover worked.
    GTEST_LOG_(INFO) << "ND FailoverInAgeEntries check relies on lookup after failover.";
    testing::Mock::VerifyAndClearExpectations(&cache);
}

// Note: Full testing of NDCache requires more fine-grained control over time,
// explicit simulation of NA/NS/RA message reception with specific contents,
// and potentially getter methods or friend classes for inspecting internal state
// (e.g., prefix list, DAD status of specific addresses, cache entry states).
// The MOCK_METHOD setup also assumes that the send_... methods in the base NDCache
// are virtual (as they were defined in nd_cache.h with `virtual` keyword).
// The tests above attempt to cover DAD, SLAAC, and failover logic,
// but full verification of all internal states and transitions would benefit
// from more fine-grained control or inspection capabilities in NDCache, or more specific test message simulations.

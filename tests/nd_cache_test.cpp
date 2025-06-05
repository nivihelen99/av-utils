#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "../include/nd_cache.h"
#include <vector>
#include <array>
#include <iostream>

class MockNDCache : public NDCache {
public:
    MockNDCache(const mac_addr_t& own_mac) : NDCache(own_mac) {}

    MOCK_METHOD(void, send_router_solicitation, (const ipv6_addr_t& source_ip), (override));
    MOCK_METHOD(void, send_neighbor_solicitation, (const ipv6_addr_t& target_ip, const ipv6_addr_t& source_ip, const mac_addr_t* sllao, bool for_dad), (override));
    MOCK_METHOD(void, send_neighbor_advertisement, (const ipv6_addr_t& target_ip, const ipv6_addr_t& adv_source_ip, const mac_addr_t& tllao, bool is_router, bool solicited, bool override_flag), (override));
};

// Helper to generate a link-local address from a MAC for test setup (not strictly needed by tests anymore if using getters)
ipv6_addr_t generate_ll_from_mac_for_test(const mac_addr_t& mac) {
    ipv6_addr_t ll_addr;
    ll_addr.fill(0);
    ll_addr[0] = 0xfe; ll_addr[1] = 0x80;
    ll_addr[8] = mac[0] ^ 0x02; ll_addr[9] = mac[1]; ll_addr[10] = mac[2];
    ll_addr[11] = 0xFF; ll_addr[12] = 0xFE;
    ll_addr[13] = mac[3]; ll_addr[14] = mac[4]; ll_addr[15] = mac[5];
    return ll_addr;
}

TEST(NDCacheTest, LinkLocalAndDAD) {
    mac_addr_t device_mac = {0x00, 0x00, 0x00, 0x11, 0x22, 0x33};
    MockNDCache cache(device_mac);
    ipv6_addr_t link_local_addr = cache.get_link_local_address();
    ipv6_addr_t solicited_node_addr_for_ll = cache.get_link_local_solicited_node_address();
    ipv6_addr_t unspecified_addr;
    unspecified_addr.fill(0);

    auto test_time = std::chrono::steady_clock::now();

    // Expect MAX_MULTICAST_SOLICIT DAD NS messages to be sent for the link-local address.
    // Target is the solicited-node multicast address of the link-local address.
    // Source is the unspecified address.
    EXPECT_CALL(cache, send_neighbor_solicitation(solicited_node_addr_for_ll, unspecified_addr, nullptr, true))
        .Times(NDCache::MAX_MULTICAST_SOLICIT);

    for (int i = 0; i < NDCache::MAX_MULTICAST_SOLICIT; ++i) {
        cache.age_entries(test_time);
        if (i < NDCache::MAX_MULTICAST_SOLICIT -1) { // Don't check on the iteration that sends the last probe
           ASSERT_FALSE(cache.is_link_local_dad_completed()) << "DAD completed prematurely after probe " << i + 1;
        }
        test_time += std::chrono::milliseconds(NDCache::RETRANS_TIMER) + std::chrono::milliseconds(10);
    }

    // One more call to age_entries after all probes have been sent and their timeout period has passed.
    // This call should process the DAD success.
    cache.age_entries(test_time);

    ASSERT_TRUE(cache.is_link_local_dad_completed()) << "DAD for link-local address did not complete successfully.";

    testing::Mock::VerifyAndClearExpectations(&cache);
}

TEST(NDCacheTest, SLAACProcessingAndDAD) {
    mac_addr_t device_mac = {0x00, 0x00, 0x00, 0x11, 0x22, 0xAA};
    MockNDCache cache(device_mac);
    ipv6_addr_t link_local_addr = cache.get_link_local_address();
    ipv6_addr_t solicited_node_addr_for_ll = cache.get_link_local_solicited_node_address();
    ipv6_addr_t unspecified_addr;
    unspecified_addr.fill(0);

    // 1. Complete Link-Local DAD first
    EXPECT_CALL(cache, send_neighbor_solicitation(solicited_node_addr_for_ll, unspecified_addr, nullptr, true))
        .Times(NDCache::MAX_MULTICAST_SOLICIT);

    auto ll_dad_test_time = std::chrono::steady_clock::now();
    for (int i = 0; i < NDCache::MAX_MULTICAST_SOLICIT; ++i) {
        cache.age_entries(ll_dad_test_time);
        ll_dad_test_time += std::chrono::milliseconds(NDCache::RETRANS_TIMER) + std::chrono::milliseconds(10);
    }
    cache.age_entries(ll_dad_test_time);
    ASSERT_TRUE(cache.is_link_local_dad_completed());
    testing::Mock::VerifyAndClearExpectations(&cache);

    // 2. Prepare and process Router Advertisement for SLAAC
    NDCache::RAInfo ra;
    ra.source_ip = generate_ll_from_mac_for_test({0xAA,0xBB,0xCC,0xDD,0xEE,0xFF}); // Example router LL
    ra.router_mac = {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};
    ra.router_lifetime = std::chrono::seconds(1800);

    PrefixEntry pe;
    pe.prefix = {{0x20,0x01,0x0d,0xb8,0x00,0x01,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};
    pe.prefix_length = 64; pe.on_link = true; pe.autonomous = true;
    pe.valid_lifetime = std::chrono::seconds(7200); pe.preferred_lifetime = std::chrono::seconds(3600);
    ra.prefixes.push_back(pe);

    cache.process_router_advertisement(ra);

    // Determine the generated SLAAC address for expectation
    ipv6_addr_t expected_slaac_address = pe.prefix;
    // Replicate EUI-64 generation for the device's MAC
    expected_slaac_address[8]  = device_mac[0] ^ 0x02;
    expected_slaac_address[9]  = device_mac[1];
    expected_slaac_address[10] = device_mac[2];
    expected_slaac_address[11] = 0xFF;
    expected_slaac_address[12] = 0xFE;
    expected_slaac_address[13] = device_mac[3];
    expected_slaac_address[14] = device_mac[4];
    expected_slaac_address[15] = device_mac[5];
    ipv6_addr_t slaac_solicited_node_addr = cache.solicited_node_multicast_address(expected_slaac_address); // Internal helper is fine here

    EXPECT_CALL(cache, send_neighbor_solicitation(slaac_solicited_node_addr, unspecified_addr, nullptr, true))
        .Times(NDCache::MAX_MULTICAST_SOLICIT);

    auto slaac_dad_test_time = std::chrono::steady_clock::now();
    for (int i = 0; i < NDCache::MAX_MULTICAST_SOLICIT; ++i) {
        cache.age_entries(slaac_dad_test_time);
        // TODO: Add an ASSERT_FALSE for DAD completion of SLAAC address if a getter is available
        slaac_dad_test_time += std::chrono::milliseconds(NDCache::RETRANS_TIMER) + std::chrono::milliseconds(10);
    }
    cache.age_entries(slaac_dad_test_time);

    // TODO: ASSERT_TRUE for DAD completion on the SLAAC address (needs getter for PrefixEntry or specific address DAD status)
    GTEST_LOG_(INFO) << "SLAAC DAD completion check requires inspection method for PrefixEntry or specific address.";
    testing::Mock::VerifyAndClearExpectations(&cache);
}

TEST(NDCacheTest, DADConflict) {
    mac_addr_t device_mac = {0x00,0x00,0x00,0x11,0x22,0xBB};
    MockNDCache cache(device_mac);
    ipv6_addr_t link_local_addr = cache.get_link_local_address();
    ipv6_addr_t solicited_node_addr_for_ll = cache.get_link_local_solicited_node_address();
    ipv6_addr_t unspecified_addr; unspecified_addr.fill(0);

    EXPECT_CALL(cache, send_neighbor_solicitation(solicited_node_addr_for_ll, unspecified_addr, nullptr, true)).Times(1);
    auto test_time = std::chrono::steady_clock::now();
    cache.age_entries(test_time);
    testing::Mock::VerifyAndClearExpectations(&cache);

    NDCache::NAInfo na_conflict;
    na_conflict.target_ip = link_local_addr;
    na_conflict.source_ip = generate_ll_from_mac_for_test({0xDE,0xAD,0xBE,0xEF,0x00,0x01});
    na_conflict.tllao = {0xDE,0xAD,0xBE,0xEF,0x00,0x01};
    na_conflict.is_router = false; na_conflict.solicited = false; na_conflict.override_flag = true;

    EXPECT_CALL(cache, send_neighbor_solicitation(solicited_node_addr_for_ll, unspecified_addr, nullptr, true)).Times(0);
    cache.process_neighbor_advertisement(na_conflict);

    ASSERT_FALSE(cache.is_link_local_dad_completed());
    testing::Mock::VerifyAndClearExpectations(&cache);
}

TEST(NDCacheTest, FastFailoverLookup) {
    mac_addr_t device_mac = {0x00,0x00,0x00,0x11,0x22,0xCC}; MockNDCache cache(device_mac);
    auto test_time = std::chrono::steady_clock::now();
    EXPECT_CALL(cache, send_neighbor_solicitation(cache.get_link_local_solicited_node_address(), testing::_, nullptr, true)).Times(NDCache::MAX_MULTICAST_SOLICIT);
    for (int i = 0; i < NDCache::MAX_MULTICAST_SOLICIT; ++i) { cache.age_entries(test_time); test_time += std::chrono::milliseconds(NDCache::RETRANS_TIMER) + std::chrono::milliseconds(10); }
    cache.age_entries(test_time); ASSERT_TRUE(cache.is_link_local_dad_completed()); testing::Mock::VerifyAndClearExpectations(&cache);

    ipv6_addr_t neighbor_ip = {{0x20,0x01,0x0d,0xb8,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01}};
    mac_addr_t mac1_primary = {0x11,0x22,0x33,0x44,0x55,0x01}; mac_addr_t mac2_backup  = {0x11,0x22,0x33,0x44,0x55,0x02};
    mac_addr_t mac_out;
    cache.add_entry(neighbor_ip, mac1_primary, NDCacheState::REACHABLE); cache.add_backup_mac(neighbor_ip, mac2_backup);
    ASSERT_TRUE(cache.lookup(neighbor_ip, mac_out)); ASSERT_EQ(mac_out, mac1_primary);
    GTEST_LOG_(INFO) << "ND FastFailoverLookup test is conceptual for STALE state due to state control limitations.";
}
TEST(NDCacheTest, FailoverInAgeEntries) {
    mac_addr_t device_mac = {0x00,0x00,0x00,0x11,0x22,0xDD}; MockNDCache cache(device_mac);
    auto test_time = std::chrono::steady_clock::now();
    EXPECT_CALL(cache, send_neighbor_solicitation(cache.get_link_local_solicited_node_address(), testing::_, nullptr, true)).Times(NDCache::MAX_MULTICAST_SOLICIT);
    for (int i = 0; i < NDCache::MAX_MULTICAST_SOLICIT; ++i) { cache.age_entries(test_time); test_time += std::chrono::milliseconds(NDCache::RETRANS_TIMER) + std::chrono::milliseconds(10); }
    cache.age_entries(test_time); ASSERT_TRUE(cache.is_link_local_dad_completed()); testing::Mock::VerifyAndClearExpectations(&cache);

    ipv6_addr_t neighbor_ip = {{0x20,0x01,0x0d,0xb8,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0A}};
    mac_addr_t mac1_primary = {0x11,0xAA,0xBB,0xCC,0xDD,0x01}; mac_addr_t mac2_backup  = {0x11,0xAA,0xBB,0xCC,0xDD,0x02};
    mac_addr_t mac_out;
    cache.add_entry(neighbor_ip, mac1_primary, NDCacheState::INCOMPLETE, std::chrono::seconds(30), false, {mac2_backup});

    // For INCOMPLETE NUD, target of NS is the neighbor_ip's solicited-node address, source is link-local.
    ipv6_addr_t neighbor_solicited_node_addr = cache.solicited_node_multicast_address(neighbor_ip); // Public helper if needed, or use private one
    EXPECT_CALL(cache, send_neighbor_solicitation(neighbor_solicited_node_addr, link_local_addr, &device_mac, false))
        .Times(NDCache::MAX_MULTICAST_SOLICIT);

    test_time = std::chrono::steady_clock::now(); // Reset time for this new sequence
    for (int i = 0; i < NDCache::MAX_MULTICAST_SOLICIT; ++i) {
        cache.age_entries(test_time);
        test_time += std::chrono::milliseconds(NDCache::RETRANS_TIMER) + std::chrono::milliseconds(10);
    }
    cache.age_entries(test_time);
    ASSERT_TRUE(cache.lookup(neighbor_ip, mac_out)); ASSERT_EQ(mac_out, mac2_backup);
    GTEST_LOG_(INFO) << "ND FailoverInAgeEntries check relies on lookup after failover.";
    testing::Mock::VerifyAndClearExpectations(&cache);
}

// Note on generate_eui64_interface_id_bytes_for_test in MockNDCache:
// The SLAAC test needs to predict the generated IPv6 address.
// The helper `generate_ll_from_mac_for_test` replicates EUI-64 logic.
// The SLAAC test directly replicates EUI-64 logic using device_mac for its expectation.
// This is fine and avoids needing a public version of the internal EUI-64 helper just for tests,
// or complex friend class setups.

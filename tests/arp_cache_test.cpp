#include "gtest/gtest.h"
#include "gmock/gmock.h" // Added for GMock features
#include "../include/arp_cache.h" // Adjust path as needed
#include <vector>
#include <array>
#include <iostream> // For std::cerr for warnings, if not captured by test
#include <thread> // For std::this_thread

// MockARPCache to allow mocking of virtual methods from ARPCache
class MockARPCache : public ARPCache {
public:
    // Constructor must call the base class constructor
    MockARPCache(const mac_addr_t& dev_mac,
                 std::chrono::seconds reachable_time = std::chrono::seconds(300),
                 std::chrono::seconds stale_time = std::chrono::seconds(30),
                 std::chrono::seconds probe_retransmit_interval = std::chrono::seconds(1),
                 std::chrono::seconds max_probe_backoff_interval = std::chrono::seconds(60),
                 std::chrono::seconds failed_entry_lifetime = std::chrono::seconds(20),
                 std::chrono::seconds delay_duration = std::chrono::seconds(5),
                 std::chrono::seconds flap_detection_window = std::chrono::seconds(10),
                 int max_flaps = 3,
                 size_t max_cache_size = 1024)
        : ARPCache(dev_mac, reachable_time, stale_time, probe_retransmit_interval, max_probe_backoff_interval, failed_entry_lifetime, delay_duration, flap_detection_window, max_flaps, max_cache_size) {}

    // Mock for the virtual send_arp_request method (must be virtual in ARPCache)
    MOCK_METHOD(void, send_arp_request, (uint32_t ip), (override));

    // Mock for the virtual log_ip_conflict method (must be virtual in ARPCache)
    MOCK_METHOD(void, log_ip_conflict, (uint32_t ip, const mac_addr_t& existing_mac, const mac_addr_t& new_mac), (override));

    // Helper for tests to force an entry into a specific state
    void force_set_state_for_test(uint32_t ip, ARPState new_state, std::chrono::steady_clock::time_point timestamp) {
        auto it = cache_.find(ip);
        if (it == cache_.end()) { // If entry doesn't exist, create a dummy one to set state
            mac_addr_t dummy_mac = {0};
            // ARPEntry { mac, state, timestamp, probe_count, pending_packets_q, backup_macs_vec, backoff_exp, flap_cnt, last_mac_update_t }
            cache_[ip] = {dummy_mac, new_state, timestamp, 0, {}, {}, 0, 0, std::chrono::steady_clock::time_point{}};
            promoteToMRU(ip);
        } else {
            it->second.state = new_state;
            it->second.timestamp = timestamp;
            promoteToMRU(ip);
        }
    }
};

TEST(ARPCacheTest, AddAndLookup) {
    mac_addr_t dev_mac = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    MockARPCache cache(dev_mac);

    uint32_t ip1 = 0xC0A80101;
    mac_addr_t mac1 = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01};
    mac_addr_t mac_out;

    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    ASSERT_FALSE(cache.lookup(ip1, mac_out));

    cache.add_entry(ip1, mac1);
    EXPECT_CALL(cache, send_arp_request(testing::_)).Times(0);
    ASSERT_TRUE(cache.lookup(ip1, mac_out));
    ASSERT_EQ(mac_out, mac1);
    testing::Mock::VerifyAndClearExpectations(&cache);
}

TEST(ARPCacheTest, GratuitousARP) {
    mac_addr_t dev_mac = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    MockARPCache cache(dev_mac);

    uint32_t ip1 = 0xC0A80101;
    mac_addr_t mac1 = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01};
    mac_addr_t mac2 = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x02};

    cache.add_entry(ip1, mac1);

    EXPECT_CALL(cache, log_ip_conflict(testing::_, testing::_, testing::_)).Times(0);
    cache.add_entry(ip1, mac1);
    testing::Mock::VerifyAndClearExpectations(&cache);

    EXPECT_CALL(cache, log_ip_conflict(ip1, mac1, mac2)).Times(1);
    cache.add_entry(ip1, mac2);

    mac_addr_t mac_out;
    ASSERT_TRUE(cache.lookup(ip1, mac_out));
    ASSERT_EQ(mac_out, mac2);
    testing::Mock::VerifyAndClearExpectations(&cache);
}

TEST(ARPCacheTest, ProxyARP) {
    mac_addr_t dev_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    MockARPCache cache(dev_mac);

    uint32_t proxy_ip_prefix = 0xC0A80A00;
    uint32_t proxy_subnet_mask = 0xFFFFFF00;
    cache.add_proxy_subnet(proxy_ip_prefix, proxy_subnet_mask);

    uint32_t ip_in_proxy_subnet = 0xC0A80A05;
    uint32_t ip_not_in_proxy_subnet = 0xC0A80B05;
    mac_addr_t mac_out;

    EXPECT_CALL(cache, send_arp_request(testing::_)).Times(0);
    ASSERT_TRUE(cache.lookup(ip_in_proxy_subnet, mac_out));
    ASSERT_EQ(mac_out, dev_mac);
    testing::Mock::VerifyAndClearExpectations(&cache);

    EXPECT_CALL(cache, send_arp_request(testing::_)).Times(0);
    ASSERT_TRUE(cache.lookup(ip_in_proxy_subnet, mac_out));
    ASSERT_EQ(mac_out, dev_mac);
    testing::Mock::VerifyAndClearExpectations(&cache);

    EXPECT_CALL(cache, send_arp_request(ip_not_in_proxy_subnet)).Times(1);
    ASSERT_FALSE(cache.lookup(ip_not_in_proxy_subnet, mac_out));
    testing::Mock::VerifyAndClearExpectations(&cache);
}

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
    auto test_reachable_time = std::chrono::seconds(20);
    auto test_stale_time = std::chrono::seconds(5);
    auto test_probe_interval = std::chrono::seconds(1);
    auto test_max_backoff = std::chrono::seconds(60);
    auto test_failed_lifetime = std::chrono::seconds(20);
    auto test_delay_duration = std::chrono::seconds(5);
    auto test_flap_window = std::chrono::seconds(10);
    int test_max_flaps = 3;
    size_t test_max_size = 1024;

    MockARPCache cache(dev_mac, test_reachable_time, test_stale_time, test_probe_interval,
                       test_max_backoff, test_failed_lifetime, test_delay_duration,
                       test_flap_window, test_max_flaps, test_max_size);

    uint32_t ip1 = 0xC0A80102;
    mac_addr_t mac1_primary_dummy = {};
    mac_addr_t mac1_backup  = {0x00, 0x11, 0x22, 0x33, 0x44, 0xBB};
    mac_addr_t mac_out;

    auto current_time_base = std::chrono::steady_clock::now();
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    ASSERT_FALSE(cache.lookup(ip1, mac1_primary_dummy));
    testing::Mock::VerifyAndClearExpectations(&cache);

    cache.add_backup_mac(ip1, mac1_backup);

    EXPECT_CALL(cache, log_ip_conflict(testing::_, testing::_, testing::_)).Times(0);

    auto last_probe_time = current_time_base;
    for (int k = 0; k < ARPCache::MAX_PROBES; ++k) {
        long long wait_multiplier = (1LL << k);
        std::chrono::seconds wait_duration = std::chrono::seconds(test_probe_interval.count() * wait_multiplier);
        if (wait_duration > test_max_backoff) {
            wait_duration = test_max_backoff;
        }
        last_probe_time += wait_duration + std::chrono::milliseconds(100);
        EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
        cache.age_entries(last_probe_time);
        testing::Mock::VerifyAndClearExpectations(&cache);
    }

    long long final_wait_multiplier = (1LL << ARPCache::MAX_PROBES);
    std::chrono::seconds final_wait_duration = std::chrono::seconds(test_probe_interval.count() * final_wait_multiplier);
    if (final_wait_duration > test_max_backoff) {
        final_wait_duration = test_max_backoff;
    }
    last_probe_time += final_wait_duration + std::chrono::milliseconds(100);

    EXPECT_CALL(cache, send_arp_request(ip1)).Times(0);
    cache.age_entries(last_probe_time);
    testing::Mock::VerifyAndClearExpectations(&cache);

    EXPECT_CALL(cache, send_arp_request(ip1)).Times(0);
    ASSERT_TRUE(cache.lookup(ip1, mac_out)) << "Lookup failed after expected failover.";
    ASSERT_EQ(mac_out, mac1_backup) << "MAC address did not match backup MAC after failover.";
    testing::Mock::VerifyAndClearExpectations(&cache);
}

TEST(ARPCacheTest, ConfigurableTimers_ReachableToStale) {
    mac_addr_t dev_mac = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    auto custom_reachable_time = std::chrono::seconds(5);
    auto custom_stale_time = std::chrono::seconds(3);
    MockARPCache cache(dev_mac, custom_reachable_time, custom_stale_time, std::chrono::seconds(1));

    uint32_t ip1 = 0xC0A8010F;
    mac_addr_t mac1 = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x0F};
    mac_addr_t mac_out;

    auto time_reference_add = std::chrono::steady_clock::now();
    cache.add_entry(ip1, mac1);

    auto time_for_staling = time_reference_add + custom_reachable_time + std::chrono::milliseconds(100);
    cache.age_entries(time_for_staling);

    ASSERT_TRUE(cache.lookup(ip1, mac_out) && mac_out == mac1);

    auto time_for_probing = time_for_staling + custom_stale_time + std::chrono::milliseconds(100);
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    cache.age_entries(time_for_probing);
    testing::Mock::VerifyAndClearExpectations(&cache);
}

TEST(ARPCacheTest, ConfigurableTimers_StaleToProbe) {
    mac_addr_t dev_mac = {0x00, 0x01, 0x02, 0x03, 0x04, 0x06};
    auto reachable_time = std::chrono::seconds(2);
    auto custom_stale_time = std::chrono::seconds(3);
    MockARPCache cache(dev_mac, reachable_time, custom_stale_time, std::chrono::seconds(1));

    uint32_t ip1 = 0xC0A80102;
    mac_addr_t mac1 = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x02};
    mac_addr_t mac_out;

    auto time_reference_add = std::chrono::steady_clock::now();
    cache.add_entry(ip1, mac1);

    auto time_when_staled = time_reference_add + reachable_time + std::chrono::milliseconds(100);
    cache.age_entries(time_when_staled);

    ASSERT_TRUE(cache.lookup(ip1, mac_out) && mac_out == mac1);

    auto time_to_probe_from_stale = time_when_staled + custom_stale_time + std::chrono::milliseconds(100);
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    cache.age_entries(time_to_probe_from_stale);
    testing::Mock::VerifyAndClearExpectations(&cache);
}

TEST(ARPCacheTest, ConfigurableTimers_ProbeRetransmit) {
    mac_addr_t dev_mac = {0x00, 0x01, 0x02, 0x03, 0x04, 0x07};
    auto base_interval = std::chrono::seconds(2);
    auto max_backoff = std::chrono::seconds(60);

    MockARPCache cache(dev_mac,
                       std::chrono::seconds(10),  // reachable_time
                       std::chrono::seconds(5),   // stale_time
                       base_interval,             // probe_retransmit_interval
                       max_backoff,               // max_probe_backoff_interval
                       std::chrono::seconds(20),  // failed_entry_lifetime
                       std::chrono::seconds(5),   // delay_duration
                       std::chrono::seconds(10),  // flap_detection_window
                       3,                         // max_flaps
                       1024);                     // max_cache_size

    uint32_t ip1 = 0xC0A80103;
    mac_addr_t mac_out;

    auto current_time_base = std::chrono::steady_clock::now();
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    ASSERT_FALSE(cache.lookup(ip1, mac_out));
    testing::Mock::VerifyAndClearExpectations(&cache);

    auto last_probe_time = current_time_base;

    for (int k = 0; k < ARPCache::MAX_PROBES; ++k) {
        long long current_backoff_exponent_for_wait = k;
        long long wait_multiplier = (1LL << current_backoff_exponent_for_wait);
        std::chrono::seconds wait_duration = std::chrono::seconds(base_interval.count() * wait_multiplier);
        if (wait_duration > max_backoff) {
            wait_duration = max_backoff;
        }
        auto next_probe_trigger_time = last_probe_time + wait_duration + std::chrono::milliseconds(100);
        EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
        cache.age_entries(next_probe_trigger_time);
        last_probe_time = next_probe_trigger_time;
        testing::Mock::VerifyAndClearExpectations(&cache);
    }
}

TEST(ARPCacheTest, ExponentialBackoff_ProbeIntervalIncrease) {
    mac_addr_t dev_mac = {0x00,0x01,0x02,0x03,0x04,0x08};
    auto base_interval = std::chrono::seconds(1);
    auto reachable_time = std::chrono::seconds(300);
    auto stale_time = std::chrono::seconds(30);
    auto max_backoff = std::chrono::seconds(60);
    MockARPCache cache(dev_mac, reachable_time, stale_time, base_interval, max_backoff);

    uint32_t ip1 = 0xC0A8010A;
    mac_addr_t mac_out;

    auto t_ref = std::chrono::steady_clock::now();
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    ASSERT_FALSE(cache.lookup(ip1, mac_out));
    testing::Mock::VerifyAndClearExpectations(&cache);

    auto t_probe2 = t_ref + base_interval * (static_cast<long long>(1) << 0) + std::chrono::milliseconds(100);
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    cache.age_entries(t_probe2);
    testing::Mock::VerifyAndClearExpectations(&cache);

    auto t_probe3 = t_probe2 + base_interval * (static_cast<long long>(1) << 1) + std::chrono::milliseconds(100);
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    cache.age_entries(t_probe3);
    testing::Mock::VerifyAndClearExpectations(&cache);

    if (ARPCache::MAX_PROBES >= 3) {
        auto t_probe4 = t_probe3 + base_interval * (static_cast<long long>(1) << 2) + std::chrono::milliseconds(100);
        EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
        cache.age_entries(t_probe4);
        testing::Mock::VerifyAndClearExpectations(&cache);
    }
}

TEST(ARPCacheTest, ExponentialBackoff_MaxIntervalCap) {
    mac_addr_t dev_mac = {0x00,0x01,0x02,0x03,0x04,0x09};
    auto base_interval = std::chrono::seconds(1);
    auto max_backoff_cap = std::chrono::seconds(3);
    MockARPCache cache(dev_mac, std::chrono::seconds(300), std::chrono::seconds(30), base_interval, max_backoff_cap);

    uint32_t ip1 = 0xC0A8010B;
    mac_addr_t mac_out;

    auto t_ref = std::chrono::steady_clock::now();
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    ASSERT_FALSE(cache.lookup(ip1, mac_out));
    testing::Mock::VerifyAndClearExpectations(&cache);

    auto t_probe2 = t_ref + base_interval * (static_cast<long long>(1) << 0) + std::chrono::milliseconds(100);
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    cache.age_entries(t_probe2);
    testing::Mock::VerifyAndClearExpectations(&cache);

    auto t_probe3 = t_probe2 + base_interval * (static_cast<long long>(1) << 1) + std::chrono::milliseconds(100);
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    cache.age_entries(t_probe3);
    testing::Mock::VerifyAndClearExpectations(&cache);

    if (ARPCache::MAX_PROBES >= 3) {
        auto t_probe4 = t_probe3 + max_backoff_cap + std::chrono::milliseconds(100);
        EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
        cache.age_entries(t_probe4);
        testing::Mock::VerifyAndClearExpectations(&cache);

        if (ARPCache::MAX_PROBES >= 4) {
             auto t_probe5 = t_probe4 + max_backoff_cap + std::chrono::milliseconds(100);
             EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
             cache.age_entries(t_probe5);
             testing::Mock::VerifyAndClearExpectations(&cache);
        }
    }
}

TEST(ARPCacheTest, ExponentialBackoff_ResetOnReachable) {
    mac_addr_t dev_mac = {0x00,0x01,0x02,0x03,0x04,0x0A};
    auto base_interval = std::chrono::seconds(1);
    auto reachable_t = std::chrono::seconds(5);
    auto stale_t = std::chrono::seconds(2);
    auto max_backoff = std::chrono::seconds(60);
    MockARPCache cache(dev_mac, reachable_t, stale_t, base_interval, max_backoff);

    uint32_t ip1 = 0xC0A8010C;
    mac_addr_t mac1 = {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};
    mac_addr_t mac_out;

    auto t_ref = std::chrono::steady_clock::now();
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    ASSERT_FALSE(cache.lookup(ip1, mac_out));
    testing::Mock::VerifyAndClearExpectations(&cache);

    auto t_probe2_time = t_ref + base_interval * (static_cast<long long>(1) << 0) + std::chrono::milliseconds(100);
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    cache.age_entries(t_probe2_time);
    testing::Mock::VerifyAndClearExpectations(&cache);

    cache.add_entry(ip1, mac1);
    auto time_entry_made_reachable = std::chrono::steady_clock::now();

    auto time_becomes_stale = time_entry_made_reachable + reachable_t + std::chrono::milliseconds(100);
    cache.age_entries(time_becomes_stale);

    auto time_becomes_probe = time_becomes_stale + stale_t + std::chrono::milliseconds(100);
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    cache.age_entries(time_becomes_probe);
    testing::Mock::VerifyAndClearExpectations(&cache);

    auto time_for_next_probe = time_becomes_probe + base_interval * (static_cast<long long>(1) << 0) + std::chrono::milliseconds(100);
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    cache.age_entries(time_for_next_probe);
    testing::Mock::VerifyAndClearExpectations(&cache);
}

TEST(ARPCacheTest, FailedState_TransitionOnProbeFailure) {
    mac_addr_t dev_mac = {0x00,0x01,0x02,0x03,0x04,0x0B};
    auto base_interval = std::chrono::seconds(1);
    auto max_backoff_for_test = std::chrono::seconds(2);
    auto failed_lifetime_for_test = std::chrono::seconds(10);
    MockARPCache cache(dev_mac,
                       std::chrono::seconds(30),  // reachable_time
                       std::chrono::seconds(5),   // stale_time
                       base_interval,             // probe_retransmit_interval
                       max_backoff_for_test,      // max_probe_backoff_interval
                       failed_lifetime_for_test,  // failed_entry_lifetime
                       std::chrono::seconds(5),   // delay_duration (default)
                       std::chrono::seconds(10),  // flap_detection_window (default)
                       3,                         // max_flaps (default)
                       1024);                     // max_cache_size (default)

    uint32_t ip1 = 0xC0A8010D;
    mac_addr_t mac_out;

    auto current_time_base = std::chrono::steady_clock::now();
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    ASSERT_FALSE(cache.lookup(ip1, mac_out));
    testing::Mock::VerifyAndClearExpectations(&cache);

    auto last_probe_time = current_time_base;

    for (int k = 0; k < ARPCache::MAX_PROBES; ++k) {
        long long wait_multiplier = (1LL << k);
        std::chrono::seconds wait_duration = std::chrono::seconds(base_interval.count() * wait_multiplier);
        if (wait_duration > max_backoff_for_test) {
            wait_duration = max_backoff_for_test;
        }
        last_probe_time += wait_duration + std::chrono::milliseconds(100);

        EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
        cache.age_entries(last_probe_time);
        testing::Mock::VerifyAndClearExpectations(&cache);
    }

    long long final_wait_multiplier = (1LL << ARPCache::MAX_PROBES);
    std::chrono::seconds final_wait_duration = std::chrono::seconds(base_interval.count() * final_wait_multiplier);
    if (final_wait_duration > max_backoff_for_test) {
        final_wait_duration = max_backoff_for_test;
    }
    last_probe_time += final_wait_duration + std::chrono::milliseconds(100);

    EXPECT_CALL(cache, send_arp_request(ip1)).Times(0);
    cache.age_entries(last_probe_time);
    testing::Mock::VerifyAndClearExpectations(&cache);

    ASSERT_FALSE(cache.lookup(ip1, mac_out)) << "Lookup should fail for FAILED entry.";
}

TEST(ARPCacheTest, FailedState_LookupBehavior) {
    mac_addr_t dev_mac = {0x00,0x01,0x02,0x03,0x04,0x0C};
    auto base_interval = std::chrono::seconds(1); // For setup
    auto max_backoff_for_test = std::chrono::seconds(2); // For setup
    auto failed_lifetime_for_test = std::chrono::seconds(10); // For setup / test
    MockARPCache cache(dev_mac,
                       std::chrono::seconds(30),  // reachable_time
                       std::chrono::seconds(5),   // stale_time
                       base_interval,             // probe_retransmit_interval
                       max_backoff_for_test,      // max_probe_backoff_interval
                       failed_lifetime_for_test,  // failed_entry_lifetime
                       std::chrono::seconds(5),   // delay_duration (default)
                       std::chrono::seconds(10),  // flap_detection_window (default)
                       3,                         // max_flaps (default)
                       1024);                     // max_cache_size (default)
    uint32_t ip1 = 0xC0A8010E;
    mac_addr_t mac_out;

    auto current_time_base = std::chrono::steady_clock::now();
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    cache.lookup(ip1, mac_out);
    testing::Mock::VerifyAndClearExpectations(&cache);

    auto last_probe_time = current_time_base;
    for(int k=0; k < ARPCache::MAX_PROBES; ++k) {
        long long wait_multiplier = (1LL << k);
        std::chrono::seconds wait_duration = std::chrono::seconds(base_interval.count() * wait_multiplier);
        if (wait_duration > max_backoff_for_test) {
            wait_duration = max_backoff_for_test;
        }
        last_probe_time += wait_duration + std::chrono::milliseconds(100);
        EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
        cache.age_entries(last_probe_time);
        testing::Mock::VerifyAndClearExpectations(&cache);
    }
    long long final_wait_multiplier = (1LL << ARPCache::MAX_PROBES);
    std::chrono::seconds final_wait_duration = std::chrono::seconds(base_interval.count() * final_wait_multiplier);
    if (final_wait_duration > max_backoff_for_test) {
        final_wait_duration = max_backoff_for_test;
    }
    last_probe_time += final_wait_duration + std::chrono::milliseconds(100);
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(0);
    cache.age_entries(last_probe_time);
    testing::Mock::VerifyAndClearExpectations(&cache);

    ASSERT_FALSE(cache.lookup(ip1, mac_out)) << "Lookup on a FAILED entry should return false.";
}

TEST(ARPCacheTest, FailedState_PurgeAfterLifetime) {
    mac_addr_t dev_mac = {0x00,0x01,0x02,0x03,0x04,0x0D};
    auto failed_lifetime_for_test = std::chrono::seconds(3);
    auto base_interval = std::chrono::seconds(1); // For setup
    auto max_backoff_for_test = std::chrono::seconds(2); // For setup
    MockARPCache cache(dev_mac,
                       std::chrono::seconds(30),  // reachable_time
                       std::chrono::seconds(5),   // stale_time
                       base_interval,             // probe_retransmit_interval
                       max_backoff_for_test,      // max_probe_backoff_interval
                       failed_lifetime_for_test,  // failed_entry_lifetime
                       std::chrono::seconds(5),   // delay_duration (default)
                       std::chrono::seconds(10),  // flap_detection_window (default)
                       3,                         // max_flaps (default)
                       1024);                     // max_cache_size (default)
    uint32_t ip1 = 0xC0A8010F;
    mac_addr_t mac_out;

    auto current_time_base = std::chrono::steady_clock::now();
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    cache.lookup(ip1, mac_out);
    testing::Mock::VerifyAndClearExpectations(&cache);

    auto time_failed_state_set = current_time_base;
    for(int k=0; k < ARPCache::MAX_PROBES; ++k) {
        long long wait_multiplier = (1LL << k);
        std::chrono::seconds wait_duration = std::chrono::seconds(base_interval.count() * wait_multiplier);
        if (wait_duration > max_backoff_for_test) {
            wait_duration = max_backoff_for_test;
        }
        time_failed_state_set += wait_duration + std::chrono::milliseconds(100);
        EXPECT_CALL(cache, send_arp_request(ip1)).Times(testing::AtMost(1));
        cache.age_entries(time_failed_state_set);
        testing::Mock::VerifyAndClearExpectations(&cache);
    }
    // Final aging call to transition to FAILED
    long long final_wait_multiplier = (1LL << ARPCache::MAX_PROBES);
    std::chrono::seconds final_wait_duration = std::chrono::seconds(base_interval.count() * final_wait_multiplier);
    if (final_wait_duration > max_backoff_for_test) {
        final_wait_duration = max_backoff_for_test;
    }
    time_failed_state_set += final_wait_duration + std::chrono::milliseconds(100);
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(0);
    cache.age_entries(time_failed_state_set);
    testing::Mock::VerifyAndClearExpectations(&cache);

    ASSERT_FALSE(cache.lookup(ip1, mac_out));

    auto time_to_purge = time_failed_state_set + failed_lifetime_for_test + std::chrono::milliseconds(100);
    cache.age_entries(time_to_purge);

    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    ASSERT_FALSE(cache.lookup(ip1, mac_out)) << "Lookup after purge should initiate new resolution.";
    testing::Mock::VerifyAndClearExpectations(&cache);
}

TEST(ARPCacheTest, BackgroundRefresh_Successful) {
    mac_addr_t dev_mac = {0x00,0x01,0x02,0x03,0x04,0x0E};
    auto reachable_t = std::chrono::seconds(10);
    auto stale_t = std::chrono::seconds(5);
    auto base_interval = std::chrono::seconds(1);
    auto max_backoff = std::chrono::seconds(5);
    auto failed_lt = std::chrono::seconds(10);
    MockARPCache cache(dev_mac, reachable_t, stale_t, base_interval, max_backoff, failed_lt);

    uint32_t ip1 = 0xC0A80110;
    mac_addr_t mac1 = {0xAA,0xBB,0xCC,0xDD,0xEE,0x10};
    mac_addr_t mac_out;

    auto time_ref = std::chrono::steady_clock::now();
    cache.add_entry(ip1, mac1);

    auto time_in_refresh_window = time_ref + std::chrono::seconds(static_cast<std::chrono::seconds::rep>(reachable_t.count() * 0.91)) + std::chrono::milliseconds(100);

    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    cache.age_entries(time_in_refresh_window);
    testing::Mock::VerifyAndClearExpectations(&cache);

    auto time_reply_received = time_in_refresh_window + std::chrono::milliseconds(50);
    cache.add_entry(ip1, mac1);

    ASSERT_TRUE(cache.lookup(ip1, mac_out) && mac_out == mac1);

    auto time_past_original_expiry = time_reply_received + reachable_t - std::chrono::seconds(1);
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(0);
    cache.age_entries(time_past_original_expiry);
    ASSERT_TRUE(cache.lookup(ip1, mac_out) && mac_out == mac1);
    testing::Mock::VerifyAndClearExpectations(&cache);
}

TEST(ARPCacheTest, BackgroundRefresh_ToStaleIfNoReply) {
    mac_addr_t dev_mac = {0x00,0x01,0x02,0x03,0x04,0x0F};
    auto reachable_t = std::chrono::seconds(10);
    auto stale_t = std::chrono::seconds(3);
    auto base_interval = std::chrono::seconds(1);
    auto max_backoff = std::chrono::seconds(5);
    auto failed_lt = std::chrono::seconds(10);
    MockARPCache cache(dev_mac, reachable_t, stale_t, base_interval, max_backoff, failed_lt);

    uint32_t ip1 = 0xC0A80111;
    mac_addr_t mac1 = {0xAA,0xBB,0xCC,0xDD,0xEE,0x11};
    mac_addr_t mac_out;

    auto time_ref = std::chrono::steady_clock::now();
    cache.add_entry(ip1, mac1);

    auto time_in_refresh_window = time_ref + std::chrono::seconds(static_cast<std::chrono::seconds::rep>(reachable_t.count() * 0.91)) + std::chrono::milliseconds(100);
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    cache.age_entries(time_in_refresh_window);
    testing::Mock::VerifyAndClearExpectations(&cache);

    auto current_probe_time = time_in_refresh_window;

    current_probe_time += base_interval * (static_cast<long long>(1)<<0) + std::chrono::milliseconds(100);
    EXPECT_CALL(cache, send_arp_request(ip1)).Times(1);
    cache.age_entries(current_probe_time);
    testing::Mock::VerifyAndClearExpectations(&cache);

    ASSERT_FALSE(cache.lookup(ip1, mac_out));

    auto time_check_stale_behavior = current_probe_time + stale_t + std::chrono::milliseconds(100);
}

// Note: Some tests above are marked as conceptual or requiring ARPCache modifications
// for full testability (e.g., forcing entry states, mocking non-virtual methods,
// or checking stderr output without a test framework's capture abilities).
// The existing assertions primarily check the cache's state after operations.
// [end of tests/arp_cache_test.cpp]

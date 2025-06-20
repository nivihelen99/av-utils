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
                 size_t max_cache_size = 1024,
                 ConflictPolicy conflict_pol = ConflictPolicy::UPDATE_EXISTING,
                 GratuitousArpPolicy garp_pol = GratuitousArpPolicy::PROCESS,
                 std::chrono::milliseconds gratuitous_arp_min_interval = std::chrono::milliseconds(1000))
        : ARPCache(dev_mac, reachable_time, stale_time, probe_retransmit_interval,
                     max_probe_backoff_interval, failed_entry_lifetime, delay_duration,
                     flap_detection_window, max_flaps, max_cache_size,
                     conflict_pol, garp_pol, gratuitous_arp_min_interval) {}

    // Mock for the virtual send_arp_request method (must be virtual in ARPCache)
    MOCK_METHOD(void, send_arp_request, (uint32_t ip), (override));

    // Mock for the virtual log_ip_conflict method (must be virtual in ARPCache)
    MOCK_METHOD(void, log_ip_conflict, (uint32_t ip, const mac_addr_t& existing_mac, const mac_addr_t& new_mac), (override));

    // Mock for the virtual trigger_alert method
    MOCK_METHOD(void, trigger_alert, (uint32_t ip, const mac_addr_t& existing_mac, const mac_addr_t& new_mac), (override));

    // Mock for DHCP validation (can be overridden in specific tests)
    MOCK_METHOD(bool, is_ip_mac_dhcp_validated, (uint32_t ip, const mac_addr_t& mac), (override));

    // Mock for routing validation (can be overridden in specific tests)
    MOCK_METHOD(bool, is_ip_routable, (uint32_t ip_address), (override));

    // Helper for tests to force an entry into a specific state
    void force_set_state_for_test(uint32_t ip, ARPState new_state, std::chrono::steady_clock::time_point timestamp) {
        auto it = cache_.find(ip);
        if (it == cache_.end()) { // If entry doesn't exist, create a dummy one to set state
            mac_addr_t dummy_mac = {0};
            cache_[ip] = {dummy_mac, new_state, timestamp, 0, {}, {}, 0, 0, std::chrono::steady_clock::time_point{}};
        } else {
            it->second.state = new_state;
            it->second.timestamp = timestamp;
            it->second.probe_count = 0;      // Explicitly reset
            it->second.backoff_exponent = 0; // Explicitly reset
        }
    }

    // Allow access to protected members for test verification if needed
    const std::unordered_map<uint32_t, ARPEntry>& get_cache_for_test() const { return cache_; }
    const std::unordered_map<uint32_t, std::chrono::steady_clock::time_point>& get_garp_last_seen_for_test() const { return gratuitous_arp_last_seen_; }
};

// Helper function to create common MAC addresses for tests
mac_addr_t make_mac(uint8_t val) {
    return {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, val};
}

// Helper function to create common IP addresses for tests
uint32_t make_ip(uint8_t val) {
    return 0xC0A80100 + val; // 192.168.1.X
}

// --- Test Fixture for ARPCache Tests ---
class ARPCacheTestFixture : public ::testing::Test {
protected:
    mac_addr_t dev_mac_ = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    uint32_t ip1_ = make_ip(1);
    mac_addr_t mac1_ = make_mac(1);
    uint32_t ip2_ = make_ip(2);
    mac_addr_t mac2_ = make_mac(2);
    mac_addr_t mac3_conflict_ = make_mac(3); // Different MAC for ip1_

    std::unique_ptr<MockARPCache> cache_;

    // Default parameters for cache used in most tests
    std::chrono::seconds reachable_time_ = std::chrono::seconds(300);
    std::chrono::seconds stale_time_ = std::chrono::seconds(30);
    std::chrono::seconds probe_interval_ = std::chrono::seconds(1);
    std::chrono::seconds max_backoff_ = std::chrono::seconds(60);
    std::chrono::seconds failed_lifetime_ = std::chrono::seconds(20);
    std::chrono::seconds delay_duration_ = std::chrono::seconds(5);
    std::chrono::seconds flap_window_ = std::chrono::seconds(10);
    int max_flaps_ = 3;
    size_t max_size_ = 1024;
    ConflictPolicy default_conflict_policy_ = ConflictPolicy::UPDATE_EXISTING;
    GratuitousArpPolicy default_garp_policy_ = GratuitousArpPolicy::PROCESS;
    std::chrono::milliseconds default_garp_interval_ = std::chrono::milliseconds(1000);


    void SetUp() override {
        // Default cache setup for general tests
        cache_ = std::make_unique<MockARPCache>(dev_mac_, reachable_time_, stale_time_, probe_interval_,
                                               max_backoff_, failed_lifetime_, delay_duration_,
                                               flap_window_, max_flaps_, max_size_,
                                               default_conflict_policy_, default_garp_policy_, default_garp_interval_);
        // By default, make DHCP validation pass unless overridden in a specific test
        ON_CALL(*cache_, is_ip_mac_dhcp_validated(testing::_, testing::_))
            .WillByDefault(testing::Return(true));
        ON_CALL(*cache_, is_ip_routable(testing::_))
            .WillByDefault(testing::Return(true));
    }

    void TearDown() override {
        cache_.reset();
    }

    void ReinitializeCache(ConflictPolicy conflict_pol = ConflictPolicy::UPDATE_EXISTING,
                           GratuitousArpPolicy garp_pol = GratuitousArpPolicy::PROCESS,
                           std::chrono::milliseconds garp_interval = std::chrono::milliseconds(1000),
                           std::chrono::seconds flap_window = std::chrono::seconds(10),
                           int max_flaps = 3) {
        cache_ = std::make_unique<MockARPCache>(dev_mac_, reachable_time_, stale_time_, probe_interval_,
                                               max_backoff_, failed_lifetime_, delay_duration_,
                                               flap_window, max_flaps, max_size_,
                                               conflict_pol, garp_pol, garp_interval);
        ON_CALL(*cache_, is_ip_mac_dhcp_validated(testing::_, testing::_))
            .WillByDefault(testing::Return(true));
        ON_CALL(*cache_, is_ip_routable(testing::_))
            .WillByDefault(testing::Return(true));
    }
};

// --- Original/Adapted Basic Tests ---

TEST_F(ARPCacheTestFixture, AddAndLookup) {
    mac_addr_t mac_out;
    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
    ASSERT_FALSE(cache_->lookup(ip1_, mac_out));

    cache_->add_entry(ip1_, mac1_);
    EXPECT_CALL(*cache_, send_arp_request(testing::_)).Times(0); // Should not send request if entry is found
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac1_);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

TEST_F(ARPCacheTestFixture, GratuitousARPConflictDefaultUpdate) {
    cache_->add_entry(ip1_, mac1_, ARPPacketType::GRATUITOUS_ANNOUNCEMENT); // Initial entry

    // No conflict, same MAC
    EXPECT_CALL(*cache_, log_ip_conflict(testing::_, testing::_, testing::_)).Times(0);
    cache_->add_entry(ip1_, mac1_, ARPPacketType::GRATUITOUS_ANNOUNCEMENT);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    // Conflict, different MAC
    EXPECT_CALL(*cache_, log_ip_conflict(ip1_, mac1_, mac2_)).Times(1);
    cache_->add_entry(ip1_, mac2_, ARPPacketType::GRATUITOUS_ANNOUNCEMENT); // mac2_ is different

    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac2_); // Should be updated to mac2_
    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

TEST_F(ARPCacheTestFixture, FastFailoverInLookupIfStale) {
    cache_->add_entry(ip1_, mac1_);
    cache_->add_backup_mac(ip1_, mac2_);

    cache_->force_set_state_for_test(ip1_, ARPCache::ARPState::STALE, std::chrono::steady_clock::now());

    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac2_);

    auto entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::REACHABLE);
}


TEST_F(ARPCacheTestFixture, FailoverInAgeEntriesAfterMaxProbes) {
    ReinitializeCache();
    mac_addr_t mac1_primary_dummy_ = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01}; // Changed from all zeros
    cache_->add_entry(ip1_, mac1_primary_dummy_ );
    cache_->add_backup_mac(ip1_, mac2_);

    cache_->force_set_state_for_test(ip1_, ARPCache::ARPState::INCOMPLETE, std::chrono::steady_clock::now() - probe_interval_ * 10);

    auto current_time = std::chrono::steady_clock::now();
    EXPECT_CALL(*cache_, send_arp_request(ip1_))
        .WillOnce(testing::InvokeWithoutArgs([]() {
            std::cerr << "GMOCK_ACTION_DEBUG: send_arp_request(ip1_) mock action executed (first call)." << '\n';
        }));
    cache_->age_entries(current_time);
    testing::Mock::VerifyAndClearExpectations(&(*cache_)); // Restore this

    for (int k = 0; k < ARPCache::MAX_PROBES; ++k) {
        long long wait_multiplier = (1LL << k);
        std::chrono::seconds wait_duration = probe_interval_ * wait_multiplier;
        if (wait_duration > max_backoff_) wait_duration = max_backoff_;

        current_time += wait_duration + std::chrono::milliseconds(100);
        EXPECT_CALL(*cache_, send_arp_request(ip1_))
            .WillOnce(testing::InvokeWithoutArgs([k]() { // Capture k for unique message
                std::cerr << "GMOCK_ACTION_DEBUG: send_arp_request(ip1_) mock action executed (loop k=" << k << ")." << '\n';
            }));
        cache_->age_entries(current_time);
        testing::Mock::VerifyAndClearExpectations(&(*cache_));
    }

    long long final_wait_multiplier = (1LL << ARPCache::MAX_PROBES);
    std::chrono::seconds final_wait_duration = probe_interval_ * final_wait_multiplier;
    if (final_wait_duration > max_backoff_) final_wait_duration = max_backoff_;
    current_time += final_wait_duration + std::chrono::milliseconds(100);

    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(0);
    cache_->age_entries(current_time);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac2_);
}


TEST_F(ARPCacheTestFixture, ConfigurableTimers_ReachableToStale) {
    ReinitializeCache(default_conflict_policy_, default_garp_policy_, default_garp_interval_, flap_window_, max_flaps_);
    cache_->set_reachable_time(std::chrono::seconds(5));
    cache_->set_stale_time(std::chrono::seconds(3));

    auto time_reference_add = std::chrono::steady_clock::now();
    cache_->add_entry(ip1_, mac1_);

    auto time_for_staling = time_reference_add + std::chrono::seconds(5) + std::chrono::milliseconds(100);
    cache_->age_entries(time_for_staling);

    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out) && mac_out == mac1_);

    auto entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::STALE);

    auto time_for_probing = time_for_staling + std::chrono::seconds(3) + std::chrono::milliseconds(100);
    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
    cache_->age_entries(time_for_probing);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::PROBE);
}

// --- ConflictPolicy Tests ---
struct ConflictPolicyTestParams {
    ConflictPolicy policy;
    bool should_update_mac;
    bool should_alert;
    std::string test_name;
};

class ConflictPolicyTests : public ARPCacheTestFixture,
                            public ::testing::WithParamInterface<ConflictPolicyTestParams> {
};

TEST_P(ConflictPolicyTests, HandlesConflictAccordingToPolicy) {
    auto params = GetParam();
    ReinitializeCache(params.policy);

    cache_->add_entry(ip1_, mac1_, ARPPacketType::REPLY);
    EXPECT_CALL(*cache_, log_ip_conflict(ip1_, mac1_, mac3_conflict_)).Times(1);

    if (params.should_alert) {
        EXPECT_CALL(*cache_, trigger_alert(ip1_, mac1_, mac3_conflict_)).Times(1);
    } else {
        EXPECT_CALL(*cache_, trigger_alert(testing::_, testing::_, testing::_)).Times(0);
    }

    cache_->set_flap_detection_window(std::chrono::seconds(60));
    cache_->set_max_flaps_allowed(5);

    cache_->add_entry(ip1_, mac3_conflict_, ARPPacketType::REPLY);

    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));

    if (params.should_update_mac) {
        ASSERT_EQ(mac_out, mac3_conflict_) << "MAC should have been updated for policy: " << params.test_name;
    } else {
        ASSERT_EQ(mac_out, mac1_) << "MAC should NOT have been updated for policy: " << params.test_name;
    }

    if (params.should_update_mac) {
        const auto& internal_cache = cache_->get_cache_for_test();
        auto it = internal_cache.find(ip1_);
        ASSERT_NE(it, internal_cache.end());
        ASSERT_EQ(it->second.flap_count, 1);
        ASSERT_EQ(it->second.state, ARPCache::ARPState::REACHABLE);
    }

    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

INSTANTIATE_TEST_SUITE_P(
    ARPCacheConflictPolicies,
    ConflictPolicyTests,
    ::testing::Values(
        ConflictPolicyTestParams{ConflictPolicy::LOG_ONLY, false, false, "LogOnly"},
        ConflictPolicyTestParams{ConflictPolicy::DROP_NEW, false, false, "DropNew"},
        ConflictPolicyTestParams{ConflictPolicy::UPDATE_EXISTING, true, false, "UpdateExisting"},
        ConflictPolicyTestParams{ConflictPolicy::ALERT_SYSTEM, true, true, "AlertSystem"}
    ),
    [](const testing::TestParamInfo<ConflictPolicyTestParams>& info) {
        return info.param.test_name;
    }
);

TEST_F(ARPCacheTestFixture, ConflictPolicyFlapDetectionLeadsToStale) {
    ReinitializeCache(ConflictPolicy::UPDATE_EXISTING, default_garp_policy_, default_garp_interval_, std::chrono::seconds(10), 2);

    cache_->add_entry(ip1_, mac1_);

    EXPECT_CALL(*cache_, log_ip_conflict(ip1_, mac1_, mac2_)).Times(1);
    cache_->add_entry(ip1_, mac2_);
    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac2_);
    auto entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.flap_count, 1);
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::REACHABLE);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    EXPECT_CALL(*cache_, log_ip_conflict(ip1_, mac2_, mac3_conflict_)).Times(1);
    cache_->add_entry(ip1_, mac3_conflict_);
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac3_conflict_);
    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.flap_count, 2);
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::STALE);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

// --- GratuitousArpPolicy Tests ---

TEST_F(ARPCacheTestFixture, GratuitousArp_RateLimitAndProcess_DropsWithinInterval) {
    std::chrono::milliseconds garp_interval = std::chrono::milliseconds(500);
    ReinitializeCache(ConflictPolicy::UPDATE_EXISTING, GratuitousArpPolicy::RATE_LIMIT_AND_PROCESS, garp_interval);

    cache_->add_entry(ip1_, mac1_, ARPPacketType::GRATUITOUS_ANNOUNCEMENT);
    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac1_);
    ASSERT_NE(cache_->get_garp_last_seen_for_test().count(ip1_), 0);

    EXPECT_CALL(*cache_, log_ip_conflict(testing::_, testing::_, testing::_)).Times(0);
    cache_->add_entry(ip1_, mac2_, ARPPacketType::GRATUITOUS_ANNOUNCEMENT);

    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac1_) << "MAC should NOT have been updated as GARP should be rate-limited.";

    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

TEST_F(ARPCacheTestFixture, GratuitousArp_RateLimitAndProcess_ProcessesAfterInterval) {
    std::chrono::milliseconds garp_interval = std::chrono::milliseconds(100);
    ReinitializeCache(ConflictPolicy::UPDATE_EXISTING, GratuitousArpPolicy::RATE_LIMIT_AND_PROCESS, garp_interval);

    cache_->add_entry(ip1_, mac1_, ARPPacketType::GRATUITOUS_ANNOUNCEMENT);
    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out) && mac_out == mac1_);

    std::this_thread::sleep_for(garp_interval + std::chrono::milliseconds(50));

    EXPECT_CALL(*cache_, log_ip_conflict(ip1_, mac1_, mac2_)).Times(1);
    cache_->add_entry(ip1_, mac2_, ARPPacketType::GRATUITOUS_ANNOUNCEMENT);
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac2_) << "MAC should have been updated after interval.";
    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

TEST_F(ARPCacheTestFixture, GratuitousArp_RateLimitAndProcess_ReplyNotRateLimited) {
    std::chrono::milliseconds garp_interval = std::chrono::milliseconds(500);
    ReinitializeCache(ConflictPolicy::UPDATE_EXISTING, GratuitousArpPolicy::RATE_LIMIT_AND_PROCESS, garp_interval);

    cache_->add_entry(ip1_, mac1_, ARPPacketType::GRATUITOUS_ANNOUNCEMENT);
    ASSERT_NE(cache_->get_garp_last_seen_for_test().count(ip1_), 0);

    EXPECT_CALL(*cache_, log_ip_conflict(ip1_, mac1_, mac2_)).Times(1);
    cache_->add_entry(ip1_, mac2_, ARPPacketType::REPLY);

    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac2_) << "MAC should be updated by REPLY packet, bypassing GARP rate limit.";
    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

TEST_F(ARPCacheTestFixture, GratuitousArp_LogAndProcess_ProcessesGarp) {
    ReinitializeCache(ConflictPolicy::UPDATE_EXISTING, GratuitousArpPolicy::LOG_AND_PROCESS);

    EXPECT_CALL(*cache_, log_ip_conflict(testing::_, testing::_, testing::_)).Times(0);
    cache_->add_entry(ip1_, mac1_, ARPPacketType::GRATUITOUS_ANNOUNCEMENT);

    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac1_);
    ASSERT_NE(cache_->get_garp_last_seen_for_test().count(ip1_), 0);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

TEST_F(ARPCacheTestFixture, GratuitousArp_DropIfConflict_DropsOnConflict) {
    ReinitializeCache(ConflictPolicy::UPDATE_EXISTING,
                      GratuitousArpPolicy::DROP_IF_CONFLICT);

    cache_->add_entry(ip1_, mac1_, ARPPacketType::REPLY);

    EXPECT_CALL(*cache_, log_ip_conflict(ip1_, mac1_, mac2_)).Times(1);
    EXPECT_CALL(*cache_, trigger_alert(testing::_, testing::_, testing::_)).Times(0);

    cache_->add_entry(ip1_, mac2_, ARPPacketType::GRATUITOUS_ANNOUNCEMENT);

    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac1_) << "MAC should NOT have been updated due to DROP_IF_CONFLICT for GARP.";
    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

TEST_F(ARPCacheTestFixture, GratuitousArp_DropIfConflict_NoConflictNoDrop) {
    ReinitializeCache(ConflictPolicy::UPDATE_EXISTING,
                      GratuitousArpPolicy::DROP_IF_CONFLICT);

    EXPECT_CALL(*cache_, log_ip_conflict(testing::_, testing::_, testing::_)).Times(0);
    cache_->add_entry(ip1_, mac1_, ARPPacketType::GRATUITOUS_ANNOUNCEMENT);
    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac1_);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    EXPECT_CALL(*cache_, log_ip_conflict(testing::_, testing::_, testing::_)).Times(0);
    cache_->add_entry(ip1_, mac1_, ARPPacketType::GRATUITOUS_ANNOUNCEMENT);
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac1_);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

TEST_F(ARPCacheTestFixture, GratuitousArp_DropIfConflict_ReplyCausesNormalConflictHandling) {
    ReinitializeCache(ConflictPolicy::DROP_NEW,
                      GratuitousArpPolicy::DROP_IF_CONFLICT);

    cache_->add_entry(ip1_, mac1_, ARPPacketType::REPLY);

    EXPECT_CALL(*cache_, log_ip_conflict(ip1_, mac1_, mac2_)).Times(1);
    cache_->add_entry(ip1_, mac2_, ARPPacketType::REPLY);

    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac1_) << "MAC should NOT be updated due to ConflictPolicy::DROP_NEW for the REPLY.";
    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

// --- DHCP Snooping Hook Test ---

class DerivedMockARPCache : public MockARPCache {
public:
    using MockARPCache::MockARPCache;
};

TEST_F(ARPCacheTestFixture, DhcpValidation_Fails_LogsAndProcesses) {
    EXPECT_CALL(*cache_, is_ip_mac_dhcp_validated(ip1_, mac1_))
        .WillOnce(testing::Return(false));

    EXPECT_CALL(*cache_, log_ip_conflict(testing::_, testing::_, testing::_)).Times(0);

    cache_->add_entry(ip1_, mac1_, ARPPacketType::REPLY);

    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac1_);

    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

TEST_F(ARPCacheTestFixture, DhcpValidation_Passes_ProcessesNormally) {
    EXPECT_CALL(*cache_, is_ip_mac_dhcp_validated(ip1_, mac1_))
        .WillOnce(testing::Return(true));

    EXPECT_CALL(*cache_, log_ip_conflict(testing::_, testing::_, testing::_)).Times(0);
    cache_->add_entry(ip1_, mac1_, ARPPacketType::REPLY);

    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac1_);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

// --- Setter Methods Tests ---

TEST_F(ARPCacheTestFixture, SetConflictPolicy_ChangesBehavior) {
    cache_->set_conflict_policy(ConflictPolicy::DROP_NEW);

    cache_->add_entry(ip1_, mac1_, ARPPacketType::REPLY);

    EXPECT_CALL(*cache_, log_ip_conflict(ip1_, mac1_, mac2_)).Times(1);
    EXPECT_CALL(*cache_, trigger_alert(testing::_, testing::_, testing::_)).Times(0);

    cache_->add_entry(ip1_, mac2_, ARPPacketType::REPLY);

    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac1_) << "MAC should NOT have been updated due to ConflictPolicy::DROP_NEW set via setter.";
    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

TEST_F(ARPCacheTestFixture, SetGratuitousArpPolicy_ChangesBehavior) {
    cache_->add_entry(ip1_, mac1_, ARPPacketType::REPLY);

    cache_->set_gratuitous_arp_policy(GratuitousArpPolicy::DROP_IF_CONFLICT);

    EXPECT_CALL(*cache_, log_ip_conflict(ip1_, mac1_, mac2_)).Times(1);
    cache_->add_entry(ip1_, mac2_, ARPPacketType::GRATUITOUS_ANNOUNCEMENT);

    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac1_) << "MAC should NOT have been updated due to GratuitousArpPolicy::DROP_IF_CONFLICT set via setter.";
    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

TEST_F(ARPCacheTestFixture, SetGratuitousArpMinInterval_ChangesRateLimiting) {
    cache_->set_gratuitous_arp_policy(GratuitousArpPolicy::RATE_LIMIT_AND_PROCESS);
    cache_->set_gratuitous_arp_min_interval(std::chrono::milliseconds(500));

    cache_->add_entry(ip1_, mac1_, ARPPacketType::GRATUITOUS_ANNOUNCEMENT);
    ASSERT_NE(cache_->get_garp_last_seen_for_test().count(ip1_), 0);

    EXPECT_CALL(*cache_, log_ip_conflict(testing::_, testing::_, testing::_)).Times(0);
    cache_->add_entry(ip1_, mac2_, ARPPacketType::GRATUITOUS_ANNOUNCEMENT);
    mac_addr_t mac_out;
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac1_) << "MAC update should be prevented by rate limiting after interval change.";

    std::this_thread::sleep_for(std::chrono::milliseconds(550));
    EXPECT_CALL(*cache_, log_ip_conflict(ip1_, mac1_, mac3_conflict_)).Times(1);
    cache_->add_entry(ip1_, mac3_conflict_, ARPPacketType::GRATUITOUS_ANNOUNCEMENT);
    ASSERT_TRUE(cache_->lookup(ip1_, mac_out));
    ASSERT_EQ(mac_out, mac3_conflict_) << "MAC should be updated after rate limit interval passed.";
    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

TEST_F(ARPCacheTestFixture, SetDeviceMac_UpdatesProxyArpMac) {
    mac_addr_t new_dev_mac = make_mac(0xDD);
    cache_->set_device_mac(new_dev_mac); // This sets the default device MAC

    uint32_t proxy_ip_prefix = 0xC0A80A00;
    uint32_t proxy_subnet_mask = 0xFFFFFF00;
    uint32_t test_interface_id = 1; // Define an interface ID for this test

    cache_->add_proxy_subnet(proxy_ip_prefix, proxy_subnet_mask, test_interface_id);
    cache_->enable_proxy_arp_on_interface(test_interface_id); // Enable proxy on this interface

    uint32_t ip_in_proxy_subnet = 0xC0A80A05;
    mac_addr_t mac_out;

    ASSERT_TRUE(cache_->resolve_proxy_arp(ip_in_proxy_subnet, test_interface_id, mac_out));
    // If no interface_mac is set for test_interface_id, mac_out should be new_dev_mac (the global one).
    ASSERT_EQ(mac_out, new_dev_mac) << "Proxy ARP should use the updated device MAC via set_device_mac as fallback.";
}

TEST_F(ARPCacheTestFixture, SetMaxCacheSize_EvictsEntries) {
    cache_->add_entry(ip1_, mac1_);
    cache_->add_entry(ip2_, mac2_);
    uint32_t ip3 = make_ip(3); mac_addr_t mac3 = make_mac(3);
    uint32_t ip4 = make_ip(4); mac_addr_t mac4 = make_mac(4);
    cache_->add_entry(ip3, mac3);
    cache_->add_entry(ip4, mac4);

    ASSERT_EQ(cache_->get_cache_for_test().size(), 4);

    cache_->set_max_cache_size(2);
    ASSERT_EQ(cache_->get_cache_for_test().size(), 2) << "Cache size should be reduced by set_max_cache_size.";

    mac_addr_t mac_out;
    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
    ASSERT_FALSE(cache_->lookup(ip1_, mac_out));
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    EXPECT_CALL(*cache_, send_arp_request(ip2_)).Times(1);
    ASSERT_FALSE(cache_->lookup(ip2_, mac_out));
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    ASSERT_TRUE(cache_->lookup(ip3, mac_out));
    ASSERT_TRUE(cache_->lookup(ip4, mac_out));
}

TEST_F(ARPCacheTestFixture, SetTimerValues_AffectsAging) {
    cache_->set_reachable_time(std::chrono::seconds(5));
    cache_->set_stale_time(std::chrono::seconds(3));

    auto time_reference_add = std::chrono::steady_clock::now();
    cache_->add_entry(ip1_, mac1_);

    cache_->age_entries(time_reference_add + std::chrono::seconds(5) + std::chrono::milliseconds(100));

    auto entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::STALE) << "Entry should be STALE after new reachable_time.";

    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
    cache_->age_entries(time_reference_add + std::chrono::seconds(5) + std::chrono::seconds(3) + std::chrono::milliseconds(200));
    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::PROBE) << "Entry should be PROBE after new stale_time.";
    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

// Cleanup of very old tests that are superseded or too complex to adapt quickly
TEST_F(ARPCacheTestFixture, ExponentialBackoff_ProbeIntervalIncrease) {
    // Set specific timer values for predictability
    probe_interval_ = std::chrono::seconds(1);
    max_backoff_ = std::chrono::seconds(10);
    ReinitializeCache(); // Reinitialize with new probe_interval_ and max_backoff_
    cache_->set_probe_retransmit_interval(probe_interval_);
    cache_->set_max_probe_backoff_interval(max_backoff_);


    // Add an entry and force its state to PROBE
    // To get it to PROBE, we can add it, then force to STALE, then age it past stale_time.
    cache_->add_entry(ip1_, mac1_);
    auto current_time = std::chrono::steady_clock::now();
    cache_->force_set_state_for_test(ip1_, ARPCache::ARPState::STALE, current_time - stale_time_ - std::chrono::seconds(1));

    // First call to age_entries to transition from STALE to PROBE
    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
    cache_->age_entries(current_time);
    auto entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::PROBE);
    ASSERT_EQ(entry_ptr->second.backoff_exponent, 0); // Initial backoff exponent
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    // Loop to check for send_arp_request calls and verify backoff_exponent increment
    for (int i = 0; i < 3; ++i) {
        int expected_backoff_exponent = i;
        std::chrono::seconds expected_interval = probe_interval_ * (1 << expected_backoff_exponent);
        if (expected_interval > max_backoff_) {
            expected_interval = max_backoff_;
        }

        current_time += expected_interval; // Advance time by the PREVIOUS interval for the next check

        entry_ptr = cache_->get_cache_for_test().find(ip1_);
        ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
        ASSERT_EQ(entry_ptr->second.backoff_exponent, expected_backoff_exponent);

        // Expect send_arp_request to be called once for the IP
        // The age_entries call will send a probe if the time has come
        EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
        cache_->age_entries(current_time + std::chrono::milliseconds(100)); // Ensure time has passed for the probe
        testing::Mock::VerifyAndClearExpectations(&(*cache_));

        entry_ptr = cache_->get_cache_for_test().find(ip1_);
        ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
        ASSERT_EQ(entry_ptr->second.backoff_exponent, expected_backoff_exponent + 1); // Verify exponent incremented
    }
}

TEST_F(ARPCacheTestFixture, ExponentialBackoff_MaxIntervalCap) {
    // Set probe_interval_ = std::chrono::seconds(1) and max_backoff_ = std::chrono::seconds(4)
    probe_interval_ = std::chrono::seconds(1);
    max_backoff_ = std::chrono::seconds(4);
    ReinitializeCache();
    cache_->set_probe_retransmit_interval(probe_interval_);
    cache_->set_max_probe_backoff_interval(max_backoff_);

    // Add an entry and force its state to PROBE
    cache_->add_entry(ip1_, mac1_);
    auto current_time = std::chrono::steady_clock::now();
    // Force to STALE first, then age to PROBE
    cache_->force_set_state_for_test(ip1_, ARPCache::ARPState::STALE, current_time - stale_time_ - std::chrono::seconds(1));

    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
    cache_->age_entries(current_time); // Transitions to PROBE, sends first probe
    auto entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::PROBE);
    ASSERT_EQ(entry_ptr->second.backoff_exponent, 0);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    // Simulate probe failures until backoff_exponent would cause interval > max_backoff_
    // exponent 0: interval 1s (1 * 2^0)
    // exponent 1: interval 2s (1 * 2^1)
    // exponent 2: interval 4s (1 * 2^2) -> this will hit max_backoff_
    // exponent 3: interval 8s (1 * 2^3) -> should be capped to 4s
    // exponent 4: interval 16s (1 * 2^4) -> should be capped to 4s

    // After first probe (exponent becomes 1)
    current_time += probe_interval_ * (1 << 0); // Advance by 1 second
    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
    cache_->age_entries(current_time + std::chrono::milliseconds(100));
    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_EQ(entry_ptr->second.backoff_exponent, 1);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    // After second probe (exponent becomes 2)
    current_time += probe_interval_ * (1 << 1); // Advance by 2 seconds
    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
    cache_->age_entries(current_time + std::chrono::milliseconds(100));
    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_EQ(entry_ptr->second.backoff_exponent, 2);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    // After third probe (exponent becomes 3). Interval should be 4s (capped)
    current_time += probe_interval_ * (1 << 2); // Advance by 4 seconds (2^2)
    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
    cache_->age_entries(current_time + std::chrono::milliseconds(100));
    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_EQ(entry_ptr->second.backoff_exponent, 3);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    // After fourth probe (exponent becomes 4). Interval should still be capped at 4s (max_backoff_)
    // The current time is exactly when the next probe is due if capped.
    current_time += max_backoff_; // Advance by max_backoff_ (4 seconds)
    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
    cache_->age_entries(current_time + std::chrono::milliseconds(100));
    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_EQ(entry_ptr->second.backoff_exponent, 4);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    // Fifth probe, still capped
    current_time += max_backoff_; // Advance by max_backoff_ (4 seconds)
    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
    cache_->age_entries(current_time + std::chrono::milliseconds(100));
    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_EQ(entry_ptr->second.backoff_exponent, 5); // Exponent keeps increasing
    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

TEST_F(ARPCacheTestFixture, ExponentialBackoff_ResetOnReachable) {
    probe_interval_ = std::chrono::seconds(1); // Not strictly necessary for this test's logic but good for consistency
    ReinitializeCache();
    cache_->set_probe_retransmit_interval(probe_interval_);

    // Add an entry, force to STALE, then age to PROBE
    cache_->add_entry(ip1_, mac1_); // Initial add, doesn't matter much which MAC
    auto current_time = std::chrono::steady_clock::now();
    cache_->force_set_state_for_test(ip1_, ARPCache::ARPState::STALE, current_time - stale_time_ - std::chrono::seconds(1));

    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
    cache_->age_entries(current_time); // Transitions to PROBE, sends first probe
    auto entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::PROBE);
    ASSERT_EQ(entry_ptr->second.backoff_exponent, 0);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    // Simulate a few probe failures to ensure backoff_exponent is > 0
    current_time += probe_interval_ * (1 << 0); // Advance 1s
    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
    cache_->age_entries(current_time + std::chrono::milliseconds(100));
    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_EQ(entry_ptr->second.backoff_exponent, 1); // Now 1
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    current_time += probe_interval_ * (1 << 1); // Advance 2s
    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
    cache_->age_entries(current_time + std::chrono::milliseconds(100));
    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.backoff_exponent, 2); // Now 2, definitely > 0
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::PROBE);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    // Call cache_->add_entry() to simulate a successful ARP reply
    mac_addr_t new_mac = make_mac(0xEF); // A different MAC for clarity
    // No send_arp_request should be called when an entry is updated with a reply
    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(0);
    cache_->add_entry(ip1_, new_mac, ARPPacketType::REPLY); // This should make it REACHABLE

    // Verify backoff_exponent is reset to 0 and state is REACHABLE
    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.mac, new_mac);
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::REACHABLE);
    ASSERT_EQ(entry_ptr->second.backoff_exponent, 0);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));
}

TEST_F(ARPCacheTestFixture, FailedState_TransitionOnProbeFailure) {
    probe_interval_ = std::chrono::seconds(1);
    max_backoff_ = std::chrono::seconds(4); // Keep backoff somewhat predictable for time advancement
    ReinitializeCache();
    cache_->set_probe_retransmit_interval(probe_interval_);
    cache_->set_max_probe_backoff_interval(max_backoff_);

    // Add an entry for ip1_ and make it INCOMPLETE (it will be after first lookup if not found)
    // Ensure it has no backup MACs. Default add_entry doesn't add backups.
    mac_addr_t mac_out;
    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
    ASSERT_FALSE(cache_->lookup(ip1_, mac_out)); // This makes it INCOMPLETE and sends the first probe
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    auto entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::INCOMPLETE);
    ASSERT_TRUE(entry_ptr->second.backup_macs.empty());
    // The first lookup already counts as the first "probe attempt" for INCOMPLETE state logic in age_entries
    // or rather, age_entries will check its timestamp relative to probe_interval.
    // The initial probe_count for a new INCOMPLETE entry is 0. age_entries will increment it.

    auto current_time = entry_ptr->second.timestamp; // Start time from when it became INCOMPLETE

    // Loop ARPCache::MAX_PROBES + 1 times.
    // The first effective probe is sent by lookup.
    // age_entries will handle subsequent probes.
    // We need MAX_PROBES more failures after the initial one to exceed MAX_PROBES total.
    for (int i = 0; i <= ARPCache::MAX_PROBES; ++i) {
        entry_ptr = cache_->get_cache_for_test().find(ip1_);
        ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());

        std::chrono::seconds time_to_advance = probe_interval_;
        if (entry_ptr->second.state == ARPCache::ARPState::PROBE || entry_ptr->second.state == ARPCache::ARPState::INCOMPLETE) {
            int current_exp = entry_ptr->second.backoff_exponent;
            time_to_advance = probe_interval_ * (1LL << current_exp);
            if (time_to_advance > max_backoff_) {
                time_to_advance = max_backoff_;
            }
        }
        current_time += time_to_advance + std::chrono::milliseconds(100); // Ensure time has passed enough

        if (entry_ptr->second.state != ARPCache::ARPState::FAILED) {
             // If it's the very first iteration, lookup already sent a request.
             // For INCOMPLETE, age_entries sends request if time matches.
             // For PROBE, age_entries sends request if time matches.
            if (i > 0 || entry_ptr->second.state == ARPCache::ARPState::PROBE) { // After first auto-probe from lookup
                 EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
            } else if (entry_ptr->second.state == ARPCache::ARPState::INCOMPLETE && i == 0) {
                // For the first call to age_entries on an INCOMPLETE entry from lookup.
                // The initial lookup already sent a request.
                // The first age_entries call will check if probe_interval has passed.
                // If so, it sends a request and increments probe_count.
                EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
            }
        }

        cache_->age_entries(current_time);
        testing::Mock::VerifyAndClearExpectations(&(*cache_));
    }

    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::FAILED);
    ASSERT_EQ(entry_ptr->second.probe_count, 0); // Should be reset
    ASSERT_EQ(entry_ptr->second.backoff_exponent, 0); // Should be reset
}

TEST_F(ARPCacheTestFixture, FailedState_LookupBehavior) {
    ReinitializeCache(); // Use default timers

    // Add an entry for ip1_ with mac1_
    cache_->add_entry(ip1_, mac1_);
    auto entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::REACHABLE);

    // Force the entry's state to ARPCache::ARPState::FAILED
    auto current_time = std::chrono::steady_clock::now();
    cache_->force_set_state_for_test(ip1_, ARPCache::ARPState::FAILED, current_time);

    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::FAILED);

    // Call cache_->lookup(ip1_, mac_out)
    mac_addr_t mac_out;
    mac_addr_t initial_mac_out_val = make_mac(0xFF); // Known initial value
    mac_out = initial_mac_out_val;

    // In FAILED state, lookup should not try to send ARP request
    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(0);
    ASSERT_FALSE(cache_->lookup(ip1_, mac_out));
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    // Optionally, ensure mac_out was not modified
    ASSERT_EQ(mac_out, initial_mac_out_val);
}

TEST_F(ARPCacheTestFixture, FailedState_PurgeAfterLifetime) {
    failed_lifetime_ = std::chrono::seconds(5);
    ReinitializeCache(); // Reinitialize with the new failed_lifetime_
    cache_->set_failed_entry_lifetime(failed_lifetime_);

    // Add an entry for ip1_ with mac1_
    cache_->add_entry(ip1_, mac1_);

    // Force its state to ARPCache::ARPState::FAILED
    auto time_failed = std::chrono::steady_clock::now();
    cache_->force_set_state_for_test(ip1_, ARPCache::ARPState::FAILED, time_failed);

    auto entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::FAILED);
    ASSERT_EQ(entry_ptr->second.timestamp, time_failed);

    // Call cache_->age_entries() just before expiry
    auto time_before_expiry = time_failed + failed_lifetime_ - std::chrono::seconds(1);
    cache_->age_entries(time_before_expiry);
    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end()) << "Entry should still exist before lifetime expires.";

    // Call cache_->age_entries() just after expiry
    auto time_after_expiry = time_failed + failed_lifetime_ + std::chrono::seconds(1);
    cache_->age_entries(time_after_expiry);
    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_EQ(entry_ptr, cache_->get_cache_for_test().end()) << "Entry should be purged after lifetime expires.";
}

TEST_F(ARPCacheTestFixture, BackgroundRefresh_Successful) {
    reachable_time_ = std::chrono::seconds(10);
    // Other timers can be default or set if specific interactions are needed
    ReinitializeCache();
    cache_->set_reachable_time(reachable_time_);

    auto time_start = std::chrono::steady_clock::now();
    cache_->add_entry(ip1_, mac1_); // Becomes REACHABLE

    auto entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::REACHABLE);
    // Update timestamp to exactly time_start for predictable calculation
    cache_->force_set_state_for_test(ip1_, ARPCache::ARPState::REACHABLE, time_start);
    entry_ptr = cache_->get_cache_for_test().find(ip1_); // Re-fetch after force_set
    ASSERT_EQ(entry_ptr->second.timestamp, time_start);


    // Calculate time for proactive refresh trigger
    // Note: REFRESH_THRESHOLD_FACTOR is 0.9 in arp_cache.h
    auto refresh_trigger_time = time_start +
                                std::chrono::seconds(static_cast<long long>(reachable_time_.count() * 0.9)) +
                                std::chrono::milliseconds(100); // Ensure it's slightly past the 0.9 threshold

    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
    cache_->age_entries(refresh_trigger_time);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::PROBE);
    ASSERT_EQ(entry_ptr->second.timestamp, refresh_trigger_time); // Timestamp updated to when it entered PROBE
    ASSERT_EQ(entry_ptr->second.probe_count, 0); // probe_count reset by transition to PROBE from refresh
    ASSERT_EQ(entry_ptr->second.backoff_exponent, 0); // backoff_exponent reset

    // Simulate receiving an ARP reply
    // No send_arp_request should happen when processing a reply that makes an entry reachable
    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(0);
    cache_->add_entry(ip1_, mac1_); // Same MAC, should go back to REACHABLE
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::REACHABLE);
    ASSERT_EQ(entry_ptr->second.probe_count, 0);
    ASSERT_EQ(entry_ptr->second.backoff_exponent, 0);
}

TEST_F(ARPCacheTestFixture, BackgroundRefresh_ToFailedIfNoReply) { // Renamed to reflect actual behavior
    reachable_time_ = std::chrono::seconds(10);
    stale_time_ = std::chrono::seconds(5); // Not directly used by refresh->probe->fail path, but good to set
    probe_interval_ = std::chrono::seconds(1);
    max_backoff_ = std::chrono::seconds(4); // Cap backoff for predictability
    ReinitializeCache();
    cache_->set_reachable_time(reachable_time_);
    cache_->set_stale_time(stale_time_);
    cache_->set_probe_retransmit_interval(probe_interval_);
    cache_->set_max_probe_backoff_interval(max_backoff_);

    auto time_start = std::chrono::steady_clock::now();
    cache_->add_entry(ip1_, mac1_); // Becomes REACHABLE
    cache_->force_set_state_for_test(ip1_, ARPCache::ARPState::REACHABLE, time_start); // Ensure precise timestamp

    auto entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.timestamp, time_start);

    // Trigger background refresh
    auto refresh_trigger_time = time_start +
                                std::chrono::seconds(static_cast<long long>(reachable_time_.count() * 0.9)) +
                                std::chrono::milliseconds(100);

    EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
    cache_->age_entries(refresh_trigger_time);
    testing::Mock::VerifyAndClearExpectations(&(*cache_));

    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::PROBE);
    ASSERT_EQ(entry_ptr->second.timestamp, refresh_trigger_time);
    // probe_count is 0, backoff_exponent is 0 as it just entered PROBE from refresh

    // Simulate probe retries without reply until it should go FAILED
    auto current_time = refresh_trigger_time;
    ASSERT_TRUE(entry_ptr->second.backup_macs.empty()); // Ensure no backup MACs for this test

    for (int i = 0; i <= ARPCache::MAX_PROBES; ++i) {
        entry_ptr = cache_->get_cache_for_test().find(ip1_);
        ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
        if (entry_ptr->second.state == ARPCache::ARPState::FAILED) {
            break; // Already failed, no more probes or time advancement needed
        }

        // Calculate expected interval for the *next* probe
        std::chrono::seconds time_to_advance = probe_interval_;
        // backoff_exponent in the entry is the one that *will be used* for the current probe if due.
        // If probe_count is 0, it's the first probe in this state, use base interval.
        // If probe_count > 0, backoff_exponent would have been incremented from the *previous* probe.
        int exponent_for_current_wait = entry_ptr->second.backoff_exponent;
        time_to_advance = probe_interval_ * (1LL << exponent_for_current_wait);

        if (time_to_advance > max_backoff_) {
            time_to_advance = max_backoff_;
        }

        current_time += time_to_advance;

        // Expect send_arp_request if the entry is still in PROBE and probe_count <= MAX_PROBES
        // The actual send happens in age_entries. The probe_count is incremented in age_entries *before* checking MAX_PROBES.
        if (entry_ptr->second.state == ARPCache::ARPState::PROBE && entry_ptr->second.probe_count < ARPCache::MAX_PROBES) {
             EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
        } else if (entry_ptr->second.state == ARPCache::ARPState::PROBE && entry_ptr->second.probe_count == ARPCache::MAX_PROBES) {
            // This is the call that will make probe_count > MAX_PROBES and transition to FAILED
            // For this specific probe send, it might or might not happen depending on exact sequence.
            // To be safe, allow it to be called or not.
            // More accurately, the send_arp_request for the (MAX_PROBES+1)th effective probe happens, then it's marked FAILED.
             EXPECT_CALL(*cache_, send_arp_request(ip1_)).Times(1);
        }


        cache_->age_entries(current_time + std::chrono::milliseconds(100)); // Ensure enough time passed
        testing::Mock::VerifyAndClearExpectations(&(*cache_));
    }

    entry_ptr = cache_->get_cache_for_test().find(ip1_);
    ASSERT_NE(entry_ptr, cache_->get_cache_for_test().end());
    ASSERT_EQ(entry_ptr->second.state, ARPCache::ARPState::FAILED);
    ASSERT_EQ(entry_ptr->second.probe_count, 0); // Reset upon entering FAILED
    ASSERT_EQ(entry_ptr->second.backoff_exponent, 0); // Reset upon entering FAILED
}


// Note: Some tests above are marked as conceptual or requiring ARPCache modifications
// for full testability (e.g., forcing entry states, mocking non-virtual methods,
// or checking stderr output without a test framework's capture abilities).
// The existing assertions primarily check the cache's state after operations.
// [end of tests/arp_cache_test.cpp]

#include "gtest/gtest.h"
#include "tcam.h" // Assumes tcam.h is in the include path
#include <vector>
#include <string>
#include <chrono>
#include <thread>        // For std::this_thread::sleep_for if needed for timestamp checks
#include <optional>      // For std::optional
#include <iomanip>       // For std::setprecision
#include <numeric>       // For std::iota if needed, though probably not here.
#include <algorithm>     // For std::find

// Helper function to create a packet (vector<uint8_t>)
// Matches the 15-byte structure used in OptimizedTCAM:
// Bytes 0-3: Source IP
// Bytes 4-7: Destination IP
// Bytes 8-9: Source Port
// Bytes 10-11: Destination Port
// Byte 12: Protocol
// Bytes 13-14: Ethernet Type
std::vector<uint8_t> make_packet(uint32_t src_ip, uint32_t dst_ip,
                                 uint16_t src_port, uint16_t dst_port,
                                 uint8_t proto, uint16_t eth_type) {
    std::vector<uint8_t> p(15);
    p[0] = (src_ip >> 24) & 0xFF; p[1] = (src_ip >> 16) & 0xFF; p[2] = (src_ip >> 8) & 0xFF; p[3] = src_ip & 0xFF;
    p[4] = (dst_ip >> 24) & 0xFF; p[5] = (dst_ip >> 16) & 0xFF; p[6] = (dst_ip >> 8) & 0xFF; p[7] = dst_ip & 0xFF;
    p[8] = (src_port >> 8) & 0xFF; p[9] = src_port & 0xFF;
    p[10] = (dst_port >> 8) & 0xFF; p[11] = dst_port & 0xFF;
    p[12] = proto;
    p[13] = (eth_type >> 8) & 0xFF; p[14] = eth_type & 0xFF;
    return p;
}

// Helper to print time_point in a simple way for debugging if needed
// Not strictly for tests, but can be useful.
std::string time_point_to_string_test(std::chrono::steady_clock::time_point tp) {
    if (tp == std::chrono::steady_clock::time_point{}) {
        return "Epoch";
    }
    return std::to_string(tp.time_since_epoch().count()) + " ns since epoch";
}


class TCAMTest : public ::testing::Test {
protected:
    OptimizedTCAM tcam;

    OptimizedTCAM::WildcardFields create_default_fields() {
        OptimizedTCAM::WildcardFields fields{};
        fields.src_ip = 0x0A000001; fields.src_ip_mask = 0xFFFFFFFF;
        fields.dst_ip = 0xC0A80001; fields.dst_ip_mask = 0xFFFFFFFF;
        fields.src_port_min = 1024; fields.src_port_max = 1024;
        fields.dst_port_min = 80; fields.dst_port_max = 80;
        fields.protocol = 6; fields.protocol_mask = 0xFF;
        fields.eth_type = 0x0800; fields.eth_type_mask = 0xFFFF;
        return fields;
    }
};

TEST_F(TCAMTest, EmptyTCAMNoMatch) {
    auto packet = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    // Previous tests used direct lookup_linear etc.
    // The main public API is lookup_single now.
    EXPECT_EQ(tcam.lookup_single(packet), -1);
}

TEST_F(TCAMTest, AddAndLookupExactMatch) { // Renamed and uses lookup_single
    OptimizedTCAM::WildcardFields fields1 = create_default_fields();
    tcam.add_rule_with_ranges(fields1, 100, 1);

    auto matching_packet = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    auto non_matching_packet_ip = make_packet(0x0A000002, 0xC0A80001, 1024, 80, 6, 0x0800);
    auto non_matching_packet_port = make_packet(0x0A000001, 0xC0A80001, 1025, 80, 6, 0x0800);
    auto non_matching_packet_proto = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 17, 0x0800);

    EXPECT_EQ(tcam.lookup_single(matching_packet), 1);
    EXPECT_EQ(tcam.lookup_single(non_matching_packet_ip), -1);
    EXPECT_EQ(tcam.lookup_single(non_matching_packet_port), -1);
    EXPECT_EQ(tcam.lookup_single(non_matching_packet_proto), -1);
}

TEST_F(TCAMTest, PriorityTest) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields(); // More specific
    OptimizedTCAM::WildcardFields fields2 = create_default_fields();
    fields2.src_ip_mask = 0xFFFF0000; // Less specific

    tcam.add_rule_with_ranges(fields1, 100, 1); // Higher priority
    tcam.add_rule_with_ranges(fields2, 90, 2);  // Lower priority but would also match

    auto matching_packet = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    EXPECT_EQ(tcam.lookup_single(matching_packet), 1); // Expect action from higher priority rule
}

TEST_F(TCAMTest, IPWildcardTest) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields();
    fields1.src_ip = 0x0A0A0000;
    fields1.src_ip_mask = 0xFFFF0000; // Match 10.10.x.x
    tcam.add_rule_with_ranges(fields1, 100, 5);

    auto packet_in_subnet = make_packet(0x0A0A0A0A, 0xC0A80001, 1024, 80, 6, 0x0800);
    auto packet_outside_subnet = make_packet(0x0A0B0A0A, 0xC0A80001, 1024, 80, 6, 0x0800);

    EXPECT_EQ(tcam.lookup_single(packet_in_subnet), 5);
    EXPECT_EQ(tcam.lookup_single(packet_outside_subnet), -1);
}

TEST_F(TCAMTest, ProtocolWildcardTest) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields();
    fields1.protocol_mask = 0x00; // Any protocol
    tcam.add_rule_with_ranges(fields1, 100, 8);

    auto packet_tcp = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);   // TCP
    auto packet_udp = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 17, 0x0800); // UDP

    EXPECT_EQ(tcam.lookup_single(packet_tcp), 8);
    EXPECT_EQ(tcam.lookup_single(packet_udp), 8);
}

TEST_F(TCAMTest, SourcePortRangeTest) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields();
    fields1.src_port_min = 2000;
    fields1.src_port_max = 2005;
    tcam.add_rule_with_ranges(fields1, 100, 10);

    auto packet_in_range_low = make_packet(0x0A000001, 0xC0A80001, 2000, 80, 6, 0x0800);
    auto packet_in_range_mid = make_packet(0x0A000001, 0xC0A80001, 2003, 80, 6, 0x0800);
    auto packet_in_range_high = make_packet(0x0A000001, 0xC0A80001, 2005, 80, 6, 0x0800);
    auto packet_below_range = make_packet(0x0A000001, 0xC0A80001, 1999, 80, 6, 0x0800);
    auto packet_above_range = make_packet(0x0A000001, 0xC0A80001, 2006, 80, 6, 0x0800);

    EXPECT_EQ(tcam.lookup_single(packet_in_range_low), 10);
    EXPECT_EQ(tcam.lookup_single(packet_in_range_mid), 10);
    EXPECT_EQ(tcam.lookup_single(packet_in_range_high), 10);
    EXPECT_EQ(tcam.lookup_single(packet_below_range), -1);
    EXPECT_EQ(tcam.lookup_single(packet_above_range), -1);
}

TEST_F(TCAMTest, DestinationPortAnyTest) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields();
    fields1.dst_port_min = 0;
    fields1.dst_port_max = 0xFFFF; // Any destination port
    tcam.add_rule_with_ranges(fields1, 100, 12);

    auto packet1 = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    auto packet2 = make_packet(0x0A000001, 0xC0A80001, 1024, 443, 6, 0x0800);
    auto packet3 = make_packet(0x0A000001, 0xC0A80001, 1024, 30000, 6, 0x0800);

    EXPECT_EQ(tcam.lookup_single(packet1), 12);
    EXPECT_EQ(tcam.lookup_single(packet2), 12);
    EXPECT_EQ(tcam.lookup_single(packet3), 12);
}

TEST_F(TCAMTest, AllLookupsComparison) { // This test might need review as internal lookups are now _idx
    OptimizedTCAM::WildcardFields fields1 = create_default_fields();
    fields1.src_ip = 0x0B000000; fields1.src_ip_mask = 0xFF000000;
    fields1.dst_port_min = 1000; fields1.dst_port_max = 2000;
    tcam.add_rule_with_ranges(fields1, 100, 15);

    OptimizedTCAM::WildcardFields fields2 = create_default_fields();
    fields2.protocol = 17; // UDP
    tcam.add_rule_with_ranges(fields2, 90, 16);

    auto packet1 = make_packet(0x0B010203, 0xC0A80001, 1024, 1500, 6, 0x0800);  // Matches fields1
    auto packet2 = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 17, 0x0800); // Matches fields2
    auto packet3 = make_packet(0x0C000001, 0xC0A80001, 100, 200, 1, 0x8100);  // No match

    // Using lookup_single which encapsulates the logic including optimized structures
    EXPECT_EQ(tcam.lookup_single(packet1), 15);
    EXPECT_EQ(tcam.lookup_single(packet2), 16);
    EXPECT_EQ(tcam.lookup_single(packet3), -1);
}

TEST_F(TCAMTest, BatchLookupTest) {
    OptimizedTCAM::WildcardFields f1{};
    f1.src_ip = 0x11111111; f1.src_ip_mask = 0xFFFFFFFF; f1.dst_ip = 0xAAAAAAAA; f1.dst_ip_mask = 0xFFFFFFFF;
    f1.src_port_min = 1024; f1.src_port_max = 1024; f1.dst_port_min = 80; f1.dst_port_max = 80;
    f1.protocol = 6; f1.protocol_mask = 0xFF; f1.eth_type = 0x0800; f1.eth_type_mask = 0xFFFF;
    tcam.add_rule_with_ranges(f1, 100, 1);

    OptimizedTCAM::WildcardFields f2{};
    f2.src_ip = 0xBBBBBBBB; f2.src_ip_mask = 0xFFFFFFFF; f2.dst_ip = 0x22222222; f2.dst_ip_mask = 0xFFFFFFFF;
    f2.src_port_min = 1024; f2.src_port_max = 1024; f2.dst_port_min = 80; f2.dst_port_max = 80;
    f2.protocol = 6; f2.protocol_mask = 0xFF; f2.eth_type = 0x0800; f2.eth_type_mask = 0xFFFF;
    tcam.add_rule_with_ranges(f2, 90, 2);

    OptimizedTCAM::WildcardFields f3{};
    f3.src_ip = 0xCCCCCCCC; f3.src_ip_mask = 0xFFFFFFFF; f3.dst_ip = 0xDDDDDDDD; f3.dst_ip_mask = 0xFFFFFFFF;
    f3.src_port_min = 5000; f3.src_port_max = 5000; f3.dst_port_min = 80; f3.dst_port_max = 80;
    f3.protocol = 6; f3.protocol_mask = 0xFF; f3.eth_type = 0x0800; f3.eth_type_mask = 0xFFFF;
    tcam.add_rule_with_ranges(f3, 80, 3);

    std::vector<std::vector<uint8_t>> packets_to_test;
    packets_to_test.push_back(make_packet(0x11111111, 0xAAAAAAAA, 1024, 80, 6, 0x0800));
    packets_to_test.push_back(make_packet(0xBBBBBBBB, 0x22222222, 1024, 80, 6, 0x0800));
    packets_to_test.push_back(make_packet(0xCCCCCCCC, 0xDDDDDDDD, 5000, 80, 6, 0x0800));
    packets_to_test.push_back(make_packet(0x0E0E0E0E, 0x0F0F0F0F, 1234, 5678, 17,0x0800));

    std::vector<int> results;
    tcam.lookup_batch(packets_to_test, results);

    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0], 1);
    EXPECT_EQ(results[1], 2);
    EXPECT_EQ(results[2], 3);
    EXPECT_EQ(results[3], -1);
}

TEST_F(TCAMTest, EthTypeMatching) {
    OptimizedTCAM tcam_eth;
    OptimizedTCAM::WildcardFields fields_ipv4 = create_default_fields();
    fields_ipv4.eth_type = 0x0800; fields_ipv4.eth_type_mask = 0xFFFF;
    tcam_eth.add_rule_with_ranges(fields_ipv4, 100, 20);

    OptimizedTCAM::WildcardFields fields_arp = create_default_fields();
    fields_arp.eth_type = 0x0806; fields_arp.eth_type_mask = 0xFFFF;
    tcam_eth.add_rule_with_ranges(fields_arp, 90, 21);

    OptimizedTCAM::WildcardFields fields_any_eth = create_default_fields();
    fields_any_eth.eth_type = 0;
    fields_any_eth.eth_type_mask = 0x0000;
    tcam_eth.add_rule_with_ranges(fields_any_eth, 80, 22);

    auto packet_ipv4 = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    auto packet_arp = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0806);
    auto packet_vlan = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x8100);

    EXPECT_EQ(tcam_eth.lookup_single(packet_ipv4), 20);
    EXPECT_EQ(tcam_eth.lookup_single(packet_arp), 21);
    EXPECT_EQ(tcam_eth.lookup_single(packet_vlan), 22);
}

// --- Tests for new observability features ---

TEST_F(TCAMTest, RuleHitStatsAndTimestamps) {
    OptimizedTCAM::WildcardFields fields = create_default_fields();
    tcam.add_rule_with_ranges(fields, 100, 1); // Rule ID 0
    uint64_t rule_id = 0;

    auto initial_stats_opt = tcam.get_rule_stats(rule_id);
    ASSERT_TRUE(initial_stats_opt.has_value());
    auto initial_stats = initial_stats_opt.value();
    EXPECT_EQ(initial_stats.hit_count, 0);
    EXPECT_EQ(initial_stats.last_hit_timestamp.time_since_epoch().count(), 0); // Default constructed time_point

    auto packet = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    tcam.lookup_single(packet);

    auto stats_after_1st_hit_opt = tcam.get_rule_stats(rule_id);
    ASSERT_TRUE(stats_after_1st_hit_opt.has_value());
    auto stats_after_1st_hit = stats_after_1st_hit_opt.value();
    EXPECT_EQ(stats_after_1st_hit.hit_count, 1);
    EXPECT_NE(stats_after_1st_hit.last_hit_timestamp.time_since_epoch().count(), 0);
    auto time_after_1st_hit = stats_after_1st_hit.last_hit_timestamp;

    // Optional: Introduce a small delay if system clock resolution is an issue
    // std::this_thread::sleep_for(std::chrono::milliseconds(1));

    tcam.lookup_single(packet);
    auto stats_after_2nd_hit_opt = tcam.get_rule_stats(rule_id);
    ASSERT_TRUE(stats_after_2nd_hit_opt.has_value());
    auto stats_after_2nd_hit = stats_after_2nd_hit_opt.value();
    EXPECT_EQ(stats_after_2nd_hit.hit_count, 2);
    EXPECT_GE(stats_after_2nd_hit.last_hit_timestamp.time_since_epoch().count(), time_after_1st_hit.time_since_epoch().count());

    auto non_matching_packet = make_packet(0x0A000002, 0xC0A80001, 1024, 80, 6, 0x0800);
    tcam.lookup_single(non_matching_packet);
    auto stats_after_miss_opt = tcam.get_rule_stats(rule_id);
    ASSERT_TRUE(stats_after_miss_opt.has_value());
    EXPECT_EQ(stats_after_miss_opt.value().hit_count, 2); // Should remain 2
}

TEST_F(TCAMTest, GetSpecificRuleStats) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields();
    tcam.add_rule_with_ranges(fields1, 100, 1); // Rule ID 0
    uint64_t rule1_id = 0;

    OptimizedTCAM::WildcardFields fields2 = create_default_fields();
    fields2.src_ip = 0x0A000002; // Different source IP
    tcam.add_rule_with_ranges(fields2, 90, 2); // Rule ID 1
    uint64_t rule2_id = 1;

    auto stats1_opt = tcam.get_rule_stats(rule1_id);
    ASSERT_TRUE(stats1_opt.has_value());
    EXPECT_EQ(stats1_opt.value().rule_id, rule1_id);
    EXPECT_EQ(stats1_opt.value().action, 1);
    EXPECT_EQ(stats1_opt.value().hit_count, 0);

    auto packet1 = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    tcam.lookup_single(packet1); // Hit rule 1

    stats1_opt = tcam.get_rule_stats(rule1_id);
    ASSERT_TRUE(stats1_opt.has_value());
    EXPECT_EQ(stats1_opt.value().hit_count, 1);

    auto stats2_opt = tcam.get_rule_stats(rule2_id);
    ASSERT_TRUE(stats2_opt.has_value());
    EXPECT_EQ(stats2_opt.value().hit_count, 0); // Rule 2 not hit

    uint64_t non_existent_id = 12345;
    auto no_stats_opt = tcam.get_rule_stats(non_existent_id);
    ASSERT_FALSE(no_stats_opt.has_value());

    // Test get_all_rule_stats
    std::vector<OptimizedTCAM::RuleStats> all_stats = tcam.get_all_rule_stats();
    ASSERT_EQ(all_stats.size(), 2);
    // Could check individual elements if needed, e.g. all_stats[0].rule_id == rule1_id etc.
}


TEST_F(TCAMTest, RuleUtilizationMetrics) {
    // Scenario 1: No rules
    auto metrics_empty = tcam.get_rule_utilization();
    EXPECT_EQ(metrics_empty.total_rules, 0);
    EXPECT_EQ(metrics_empty.active_rules, 0);
    EXPECT_EQ(metrics_empty.inactive_rules, 0);
    EXPECT_EQ(metrics_empty.rules_hit_at_least_once, 0);
    EXPECT_DOUBLE_EQ(metrics_empty.percentage_active_rules_hit, 0.0);
    EXPECT_TRUE(metrics_empty.unused_active_rule_ids.empty());

    // Scenario 2: Active rules, some hit, some not
    OptimizedTCAM::WildcardFields f1 = create_default_fields();
    tcam.add_rule_with_ranges(f1, 100, 1); // ID 0
    OptimizedTCAM::WildcardFields f2 = create_default_fields(); f2.src_ip = 0x0A000002;
    tcam.add_rule_with_ranges(f2, 100, 2); // ID 1
    OptimizedTCAM::WildcardFields f3 = create_default_fields(); f3.src_ip = 0x0A000003;
    tcam.add_rule_with_ranges(f3, 100, 3); // ID 2

    auto packet1 = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800); // Hits rule 0
    auto packet2 = make_packet(0x0A000002, 0xC0A80001, 1024, 80, 6, 0x0800); // Hits rule 1
    tcam.lookup_single(packet1);
    tcam.lookup_single(packet2);

    auto metrics_s2 = tcam.get_rule_utilization();
    EXPECT_EQ(metrics_s2.total_rules, 3);
    EXPECT_EQ(metrics_s2.active_rules, 3);
    EXPECT_EQ(metrics_s2.inactive_rules, 0);
    EXPECT_EQ(metrics_s2.rules_hit_at_least_once, 2);
    EXPECT_DOUBLE_EQ(metrics_s2.percentage_active_rules_hit, (2.0 / 3.0) * 100.0);
    ASSERT_EQ(metrics_s2.unused_active_rule_ids.size(), 1);
    EXPECT_EQ(metrics_s2.unused_active_rule_ids[0], 2); // Rule ID 2 (f3) was not hit

    // Scenario 3: Mix of active and inactive - using RuleOperation for delete
    // This requires using update_rules_atomic or similar if directly setting is_active is not an option
    // For this test, we'll delete rule 0 (f1)
    OptimizedTCAM::RuleOperation delete_op = OptimizedTCAM::RuleOperation::DeleteRule(0); // Delete rule ID 0
    OptimizedTCAM::RuleUpdateBatch batch = {delete_op};
    bool success = tcam.update_rules_atomic(batch); // This also rebuilds structures
    ASSERT_TRUE(success);

    //Hit rule 1 again
    tcam.lookup_single(packet2);


    auto metrics_s3 = tcam.get_rule_utilization();
    EXPECT_EQ(metrics_s3.total_rules, 2); // Rule 0 deleted
    EXPECT_EQ(metrics_s3.active_rules, 2);
    EXPECT_EQ(metrics_s3.inactive_rules, 0); // update_rules_atomic removes inactive rules
    EXPECT_EQ(metrics_s3.rules_hit_at_least_once, 1); // Only rule 1 (original ID 1) is hit among current active rules
    EXPECT_DOUBLE_EQ(metrics_s3.percentage_active_rules_hit, (1.0 / 2.0) * 100.0);

    ASSERT_EQ(metrics_s3.unused_active_rule_ids.size(), 1);
    // Rule with original ID 2 is now the second rule, still ID 2 if next_rule_id is not reset.
    // After delete and rebuild, rule IDs might shift if not careful.
    // Let's get all stats to confirm current IDs
    auto current_stats = tcam.get_all_rule_stats();
    uint64_t unhit_rule_id = -1;
     for(const auto& rs : current_stats){
         if(rs.hit_count == 0) unhit_rule_id = rs.rule_id;
     }
    ASSERT_NE(unhit_rule_id, (uint64_t)-1); // Ensure we found an unhit rule
    EXPECT_EQ(metrics_s3.unused_active_rule_ids[0], unhit_rule_id);
}


TEST_F(TCAMTest, LookupLatencyMetrics) {
    // Scenario 1: No lookups
    auto metrics_none = tcam.get_lookup_latency_metrics();
    EXPECT_EQ(metrics_none.total_lookups_measured, 0);
    EXPECT_EQ(metrics_none.min_latency_ns.count(), 0);
    EXPECT_EQ(metrics_none.max_latency_ns.count(), 0);
    EXPECT_EQ(metrics_none.avg_latency_ns.count(), 0);

    // Scenario 2: Some lookups
    OptimizedTCAM::WildcardFields fields = create_default_fields();
    tcam.add_rule_with_ranges(fields, 100, 1);
    auto packet = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);

    for (int i = 0; i < 5; ++i) {
        tcam.lookup_single(packet);
    }

    auto metrics_some = tcam.get_lookup_latency_metrics();
    EXPECT_EQ(metrics_some.total_lookups_measured, 5);
    EXPECT_GT(metrics_some.min_latency_ns.count(), 0);
    EXPECT_GT(metrics_some.max_latency_ns.count(), 0);
    EXPECT_GT(metrics_some.avg_latency_ns.count(), 0);
    EXPECT_LE(metrics_some.min_latency_ns.count(), metrics_some.avg_latency_ns.count());
    EXPECT_LE(metrics_some.avg_latency_ns.count(), metrics_some.max_latency_ns.count());
}

TEST_F(TCAMTest, DebugTracing) {
    OptimizedTCAM::WildcardFields fields = create_default_fields();
    tcam.add_rule_with_ranges(fields, 100, 1); // Rule ID 0

    std::vector<std::string> trace_log;
    auto matching_packet = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    tcam.lookup_single(matching_packet, &trace_log);

    ASSERT_FALSE(trace_log.empty());
    bool found_starting = false;
    bool found_matches_rule = false;
    bool found_matched_idx = false;
    for (const auto& entry : trace_log) {
        if (entry.find("lookup_single: Starting lookup") != std::string::npos) found_starting = true;
        if (entry.find("matches_rule (RuleID 0)") != std::string::npos) found_matches_rule = true;
        if (entry.find("lookup_single: Matched rule index: 0") != std::string::npos) found_matched_idx = true;
    }
    EXPECT_TRUE(found_starting);
    EXPECT_TRUE(found_matches_rule);
    EXPECT_TRUE(found_matched_idx);

    trace_log.clear();
    auto non_matching_packet = make_packet(0x0A000002, 0xC0A80001, 1024, 80, 6, 0x0800);
    tcam.lookup_single(non_matching_packet, &trace_log);

    ASSERT_FALSE(trace_log.empty());
    bool found_no_match_log = false;
    for (const auto& entry : trace_log) {
        // Example: "lookup_single: No match or invalid index from chosen strategy."
        // Or "matches_rule (RuleID 0): Final -> No Match" for the specific rule check.
        if (entry.find("No Match") != std::string::npos || entry.find("No match") != std::string::npos) {
            found_no_match_log = true;
            break;
        }
    }
    EXPECT_TRUE(found_no_match_log);
}

// Note: Existing tests like AllLookupsComparison might need adjustment
// if the internal lookup_linear, lookup_bitmap, lookup_decision_tree methods
// are changed to also return indices or are removed in favor of _idx versions
// for internal use by lookup_single. For now, they are assumed to still work
// for their original purpose or will be updated if this subtask implied changes to them.
// Based on previous subtasks, they were changed to _idx and lookup_single uses them.
// The current tests that use them directly would fail if they expect actions.
// They have been updated to use lookup_single for simplicity in this pass.

// It's good practice to have a main for gtest if not linking with gtest_main
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

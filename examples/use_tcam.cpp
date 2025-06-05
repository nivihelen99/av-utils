#include "tcam.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip> // For std::setw or other formatting if needed

// Helper function to create a packet (vector<uint8_t>)
std::vector<uint8_t> make_example_packet(uint32_t src_ip, uint32_t dst_ip,
                                         uint16_t src_port, uint16_t dst_port,
                                         uint8_t proto, uint16_t eth_type) {
    std::vector<uint8_t> p(15); // Increased size to ensure all fields can be written if needed
    p[0] = (src_ip >> 24) & 0xFF; p[1] = (src_ip >> 16) & 0xFF; p[2] = (src_ip >> 8) & 0xFF; p[3] = src_ip & 0xFF;
    p[4] = (dst_ip >> 24) & 0xFF; p[5] = (dst_ip >> 16) & 0xFF; p[6] = (dst_ip >> 8) & 0xFF; p[7] = dst_ip & 0xFF;
    p[8] = (src_port >> 8) & 0xFF; p[9] = src_port & 0xFF;
    p[10] = (dst_port >> 8) & 0xFF; p[11] = dst_port & 0xFF;
    p[12] = proto;
    // Ensure eth_type is packed correctly (example used 15 bytes, so up to index 14)
    p[13] = (eth_type >> 8) & 0xFF;
    p[14] = eth_type & 0xFF;
    return p;
}

// Helper to print time_point in a simple way
std::string time_point_to_string(std::chrono::steady_clock::time_point tp) {
    if (tp == std::chrono::steady_clock::time_point{}) { // Or time_since_epoch().count() == 0
        return "Never";
    }
    return std::to_string(tp.time_since_epoch().count()) + " ns since epoch";
}

int main() {
    OptimizedTCAM my_tcam;

    // --- Existing Example: Add a rule ---
    OptimizedTCAM::WildcardFields fields1{};
    fields1.src_ip = 0x0A000001; fields1.src_ip_mask = 0xFFFFFFFF; // 10.0.0.1
    fields1.dst_ip = 0xC0A80001; fields1.dst_ip_mask = 0xFFFFFFFF; // 192.168.0.1
    fields1.src_port_min = 1024; fields1.src_port_max = 1024;
    fields1.dst_port_min = 80; fields1.dst_port_max = 80;
    fields1.protocol = 6; fields1.protocol_mask = 0xFF; // TCP
    fields1.eth_type = 0x0800; fields1.eth_type_mask = 0xFFFF; // IPv4

    my_tcam.add_rule_with_ranges(fields1, 100, 1); // Priority 100, Action 1
    uint64_t rule1_id = 0; // Assuming first rule gets ID 0 with current next_rule_id logic

    OptimizedTCAM::WildcardFields fields2{};
    fields2.src_ip = 0x0A000002; fields2.src_ip_mask = 0xFFFFFFFF; // 10.0.0.2
    fields2.dst_ip = 0x00000000; fields2.dst_ip_mask = 0x00000000; // Any DST IP
    fields2.src_port_min = 0; fields2.src_port_max = 0xFFFF; // Any SRC port
    fields2.dst_port_min = 0; fields2.dst_port_max = 0xFFFF; // Any DST port
    fields2.protocol = 17; fields2.protocol_mask = 0xFF; // UDP
    fields2.eth_type = 0x0800; fields2.eth_type_mask = 0xFFFF; // IPv4
    my_tcam.add_rule_with_ranges(fields2, 90, 2); // Priority 90, Action 2
    uint64_t rule2_id = 1;


    // --- Basic Lookup (as in existing example, now uses lookup_single) ---
    std::cout << "--- Basic Lookup ---" << std::endl;
    auto packet1 = make_example_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800); // Matches rule1
    int action = my_tcam.lookup_single(packet1);

    if (action != -1) {
        std::cout << "Packet 1 matched action: " << action << std::endl;
    } else {
        std::cout << "Packet 1 did not match any rule." << std::endl;
    }

    auto packet2 = make_example_packet(0x0A000002, 0x01020304, 12345, 54321, 17, 0x0800); // Matches rule2
    action = my_tcam.lookup_single(packet2);
     if (action != -1) {
        std::cout << "Packet 2 matched action: " << action << std::endl;
    } else {
        std::cout << "Packet 2 did not match any rule." << std::endl;
    }

    auto packet_nomatch = make_example_packet(0x0B000001, 0xC0A80001, 1024, 80, 6, 0x0800); // No match
    action = my_tcam.lookup_single(packet_nomatch);
     if (action != -1) {
        std::cout << "Packet NoMatch matched action: " << action << std::endl;
    } else {
        std::cout << "Packet NoMatch did not match any rule." << std::endl;
    }


    // --- Demonstrate Rule Statistics ---
    std::cout << "\n--- Rule Statistics ---" << std::endl;
    std::vector<OptimizedTCAM::RuleStats> all_stats = my_tcam.get_all_rule_stats();
    std::cout << "Total rules reported: " << all_stats.size() << std::endl;
    for (const auto& rs : all_stats) {
        std::cout << "Rule ID: " << rs.rule_id
                  << ", Priority: " << rs.priority
                  << ", Action: " << rs.action
                  << ", Active: " << (rs.is_active ? "Yes" : "No")
                  << ", Hit Count: " << rs.hit_count
                  << ", Creation Time: " << time_point_to_string(rs.creation_time)
                  << ", Last Hit: " << time_point_to_string(rs.last_hit_timestamp)
                  << std::endl;
    }

    // Example for get_rule_stats for a specific rule (assuming rule1_id is valid)
    std::optional<OptimizedTCAM::RuleStats> specific_stats = my_tcam.get_rule_stats(rule1_id);
    if (specific_stats) {
        std::cout << "\nStats for specific rule ID " << rule1_id << ":" << std::endl;
        std::cout << "  Hit Count: " << specific_stats->hit_count << std::endl;
        std::cout << "  Last Hit: " << time_point_to_string(specific_stats->last_hit_timestamp) << std::endl;
    } else {
        std::cout << "\nCould not find stats for rule ID " << rule1_id << std::endl;
    }

    // --- Demonstrate Rule Utilization Metrics ---
    std::cout << "\n--- Rule Utilization Metrics ---" << std::endl;
    OptimizedTCAM::RuleUtilizationMetrics util_metrics = my_tcam.get_rule_utilization();
    std::cout << "Total Rules: " << util_metrics.total_rules << std::endl;
    std::cout << "Active Rules: " << util_metrics.active_rules << std::endl;
    std::cout << "Inactive Rules: " << util_metrics.inactive_rules << std::endl;
    std::cout << "Rules Hit At Least Once: " << util_metrics.rules_hit_at_least_once << std::endl;
    std::cout << "Percentage Active Rules Hit: " << std::fixed << std::setprecision(2) << util_metrics.percentage_active_rules_hit << "%" << std::endl;
    std::cout << "Unused Active Rule IDs (" << util_metrics.unused_active_rule_ids.size() << "):";
    for(uint64_t id : util_metrics.unused_active_rule_ids) {
        std::cout << " " << id;
    }
    std::cout << std::endl;


    // --- Demonstrate Lookup Latency Metrics ---
    // Perform a few more lookups to generate more latency data
    for (int i = 0; i < 5; ++i) {
        my_tcam.lookup_single(packet1);
        my_tcam.lookup_single(packet2);
    }

    OptimizedTCAM::AggregatedLatencyMetrics lat_metrics = my_tcam.get_lookup_latency_metrics();
    std::cout << "\n--- Lookup Latency Metrics ---" << std::endl;
    std::cout << "Total Lookups Measured: " << lat_metrics.total_lookups_measured << std::endl;
    if (lat_metrics.total_lookups_measured > 0) {
        std::cout << "Min Latency: " << lat_metrics.min_latency_ns.count() << " ns" << std::endl;
        std::cout << "Max Latency: " << lat_metrics.max_latency_ns.count() << " ns" << std::endl;
        std::cout << "Avg Latency: " << lat_metrics.avg_latency_ns.count() << " ns" << std::endl;
    } else {
        std::cout << "No lookups measured for latency." << std::endl;
    }


    // --- Demonstrate Debug Tracing ---
    std::cout << "\n--- Debug Tracing for a matching packet ---" << std::endl;
    std::vector<std::string> trace_log_match;
    my_tcam.lookup_single(packet1, &trace_log_match);
    for (const auto& log_entry : trace_log_match) {
        std::cout << log_entry << std::endl;
    }

    std::cout << "\n--- Debug Tracing for a non-matching packet ---" << std::endl;
    std::vector<std::string> trace_log_nomatch;
    my_tcam.lookup_single(packet_nomatch, &trace_log_nomatch);
    for (const auto& log_entry : trace_log_nomatch) {
        std::cout << log_entry << std::endl;
    }

    std::cout << "\nOptimizedTCAM example usage complete." << std::endl;
    return 0;
}

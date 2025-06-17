#include "tcam.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip> // For std::setw or other formatting if needed
#include <thread>  // For std::this_thread::sleep_for
#include <sstream> // For std::stringstream (used in backup/restore example)
#include <algorithm> // For std::find_if (if needed, though get_rule_stats is better)


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
    std::cout << "--- Basic Lookup ---" << '\n';
    auto packet1 = make_example_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800); // Matches rule1
    int action = my_tcam.lookup_single(packet1);

    if (action != -1) {
        std::cout << "Packet 1 matched action: " << action << '\n';
    } else {
        std::cout << "Packet 1 did not match any rule." << '\n';
    }

    auto packet2 = make_example_packet(0x0A000002, 0x01020304, 12345, 54321, 17, 0x0800); // Matches rule2
    action = my_tcam.lookup_single(packet2);
     if (action != -1) {
        std::cout << "Packet 2 matched action: " << action << '\n';
    } else {
        std::cout << "Packet 2 did not match any rule." << '\n';
    }

    auto packet_nomatch = make_example_packet(0x0B000001, 0xC0A80001, 1024, 80, 6, 0x0800); // No match
    action = my_tcam.lookup_single(packet_nomatch);
     if (action != -1) {
        std::cout << "Packet NoMatch matched action: " << action << '\n';
    } else {
        std::cout << "Packet NoMatch did not match any rule." << '\n';
    }


    // --- Demonstrate Rule Statistics ---
    std::cout << "\n--- Rule Statistics ---" << '\n';
    std::vector<OptimizedTCAM::RuleStats> all_stats = my_tcam.get_all_rule_stats();
    std::cout << "Total rules reported: " << all_stats.size() << '\n';
    for (const auto& rs : all_stats) {
        std::cout << "Rule ID: " << rs.rule_id
                  << ", Priority: " << rs.priority
                  << ", Action: " << rs.action
                  << ", Active: " << (rs.is_active ? "Yes" : "No")
                  << ", Hit Count: " << rs.hit_count
                  << ", Creation Time: " << time_point_to_string(rs.creation_time)
                  << ", Last Hit: " << time_point_to_string(rs.last_hit_timestamp)
                  << '\n';
    }

    // Example for get_rule_stats for a specific rule (assuming rule1_id is valid)
    std::optional<OptimizedTCAM::RuleStats> specific_stats = my_tcam.get_rule_stats(rule1_id);
    if (specific_stats) {
        std::cout << "\nStats for specific rule ID " << rule1_id << ":" << '\n';
        std::cout << "  Hit Count: " << specific_stats->hit_count << '\n';
        std::cout << "  Last Hit: " << time_point_to_string(specific_stats->last_hit_timestamp) << '\n';
    } else {
        std::cout << "\nCould not find stats for rule ID " << rule1_id << '\n';
    }

    // --- Demonstrate Rule Utilization Metrics ---
    std::cout << "\n--- Rule Utilization Metrics ---" << '\n';
    OptimizedTCAM::RuleUtilizationMetrics util_metrics = my_tcam.get_rule_utilization();
    std::cout << "Total Rules: " << util_metrics.total_rules << '\n';
    std::cout << "Active Rules: " << util_metrics.active_rules << '\n';
    std::cout << "Inactive Rules: " << util_metrics.inactive_rules << '\n';
    std::cout << "Rules Hit At Least Once: " << util_metrics.rules_hit_at_least_once << '\n';
    std::cout << "Percentage Active Rules Hit: " << std::fixed << std::setprecision(2) << util_metrics.percentage_active_rules_hit << "%" << '\n';
    std::cout << "Unused Active Rule IDs (" << util_metrics.unused_active_rule_ids.size() << "):";
    for(uint64_t id : util_metrics.unused_active_rule_ids) {
        std::cout << " " << id;
    }
    std::cout << '\n';


    // --- Demonstrate Lookup Latency Metrics ---
    // Perform a few more lookups to generate more latency data
    for (int i = 0; i < 5; ++i) {
        my_tcam.lookup_single(packet1);
        my_tcam.lookup_single(packet2);
    }

    OptimizedTCAM::AggregatedLatencyMetrics lat_metrics = my_tcam.get_lookup_latency_metrics();
    std::cout << "\n--- Lookup Latency Metrics ---" << '\n';
    std::cout << "Total Lookups Measured: " << lat_metrics.total_lookups_measured << '\n';
    if (lat_metrics.total_lookups_measured > 0) {
        std::cout << "Min Latency: " << lat_metrics.min_latency_ns.count() << " ns" << '\n';
        std::cout << "Max Latency: " << lat_metrics.max_latency_ns.count() << " ns" << '\n';
        std::cout << "Avg Latency: " << lat_metrics.avg_latency_ns.count() << " ns" << '\n';
    } else {
        std::cout << "No lookups measured for latency." << '\n';
    }


    // --- Demonstrate Debug Tracing ---
    std::cout << "\n--- Debug Tracing for a matching packet ---" << '\n';
    std::vector<std::string> trace_log_match;
    my_tcam.lookup_single(packet1, &trace_log_match);
    for (const auto& log_entry : trace_log_match) {
        std::cout << log_entry << '\n';
    }

    std::cout << "\n--- Debug Tracing for a non-matching packet ---" << '\n';
    std::vector<std::string> trace_log_nomatch;
    my_tcam.lookup_single(packet_nomatch, &trace_log_nomatch);
    for (const auto& log_entry : trace_log_nomatch) {
        std::cout << log_entry << '\n';
    }

    std::cout << "\nOptimizedTCAM example usage complete." << '\n';

    // =====================================================================================
    // --- Demonstrate Conflict Detection ---
    // =====================================================================================
    std::cout << "\n\n--- Conflict Detection Example ---" << '\n';
    {
        OptimizedTCAM conflict_tcam;
        OptimizedTCAM::WildcardFields cf1 = fields1; // Base on fields1 (10.0.0.1, TCP 1024->80)

        OptimizedTCAM::WildcardFields cf2 = fields1; // Identical to cf1
        cf2.src_ip_mask = 0xFFFFFF00; // Rule 2: 10.0.0.0/24 (overlaps cf1)
                                      // Different action will make it a conflict

        conflict_tcam.add_rule_with_ranges(cf1, 100, 1001); // Rule ID 0 (example)
        conflict_tcam.add_rule_with_ranges(cf2, 90, 1002);  // Rule ID 1 (example)

        // Add a non-conflicting rule
        OptimizedTCAM::WildcardFields cf3 = fields1;
        cf3.src_ip = 0x0A0000FF; // 10.0.0.255 (distinct)
        conflict_tcam.add_rule_with_ranges(cf3, 100, 1003); // Rule ID 2 (example)


        std::cout << "Rules added for conflict detection:" << '\n';
        for(const auto& stat : conflict_tcam.get_all_rule_stats()){
            std::cout << "  Rule ID: " << stat.rule_id << ", Prio: " << stat.priority << ", Action: " << stat.action
                      << ", SrcIP: " << (stat.rule_id == 0 || stat.rule_id == 2 ? "10.0.0.1 or .255" : "10.0.0.0/24") /* Simplified representation */
                      << '\n';
        }


        std::vector<OptimizedTCAM::Conflict> conflicts = conflict_tcam.detect_conflicts();
        if (conflicts.empty()) {
            std::cout << "No conflicts detected." << '\n';
        } else {
            std::cout << "Detected conflicts (" << conflicts.size() << "):" << '\n';
            for (const auto& conflict : conflicts) {
                // To get original rule IDs, we need to fetch them based on current indices after sorting
                auto all_rules_for_conflict_check = conflict_tcam.get_all_rule_stats(); // Re-fetch to get current sorted order
                uint64_t r1_id = all_rules_for_conflict_check[conflict.rule1_idx].rule_id; // Changed .id to .rule_id
                int r1_action = all_rules_for_conflict_check[conflict.rule1_idx].action;
                uint64_t r2_id = all_rules_for_conflict_check[conflict.rule2_idx].rule_id; // Changed .id to .rule_id
                int r2_action = all_rules_for_conflict_check[conflict.rule2_idx].action;

                std::cout << "  Conflict between rule index " << conflict.rule1_idx << " (ID " << r1_id << ", Action " << r1_action
                          << ") and rule index " << conflict.rule2_idx << " (ID " << r2_id << ", Action " << r2_action << ")"
                          << ": " << conflict.description << '\n';
            }
        }
    }

    // =====================================================================================
    // --- Demonstrate Shadow Rule Detection and Elimination ---
    // =====================================================================================
    std::cout << "\n\n--- Shadow Rule Detection/Elimination Example ---" << '\n';
    {
        OptimizedTCAM shadow_tcam;
        OptimizedTCAM::WildcardFields shadowing_fields = fields1;
        shadowing_fields.src_ip_mask = 0xFFFFFF00;
        shadow_tcam.add_rule_with_ranges(shadowing_fields, 100, 2001);

        OptimizedTCAM::WildcardFields shadowed_fields = fields1;

        shadow_tcam.add_rule_with_ranges(shadowed_fields, 90, 2002);

        std::cout << "Initial rules for shadowing demo (Rule ID 1 should be shadowed by Rule ID 0):" << '\n';
        for(const auto& stat : shadow_tcam.get_all_rule_stats()) {
            std::cout << "  ID: " << stat.rule_id << ", Action: " << stat.action << ", Prio: " << stat.priority << ", Active: " << stat.is_active << '\n';
        }

        std::vector<uint64_t> dry_run_shadowed_ids = shadow_tcam.eliminate_shadowed_rules(true);
        std::cout << "Shadowed rules (dry run): ";
        for (size_t i=0; i < dry_run_shadowed_ids.size(); ++i) { std::cout << dry_run_shadowed_ids[i] << (i < dry_run_shadowed_ids.size()-1 ? ", " : "");}
        std::cout << '\n';
        std::cout << "Rule stats after dry run (should be unchanged):" << '\n';
        for(const auto& stat : shadow_tcam.get_all_rule_stats()) {
             // Assuming rule with action 2002 was assigned ID 1 if added second
             if(stat.action == 2002) std::cout << "  Shadowed Rule (Action 2002, ID " << stat.rule_id <<") still active: " << stat.is_active << '\n';
        }


        std::vector<uint64_t> eliminated_shadowed_ids = shadow_tcam.eliminate_shadowed_rules(false);
        std::cout << "Eliminated shadowed rule IDs: ";
        for (size_t i=0; i < eliminated_shadowed_ids.size(); ++i) { std::cout << eliminated_shadowed_ids[i] << (i < eliminated_shadowed_ids.size()-1 ? ", " : "");}
        std::cout << '\n';

        std::cout << "Rule stats after elimination:" << '\n';
        for(const auto& stat : shadow_tcam.get_all_rule_stats()) {
            std::cout << "  ID: " << stat.rule_id << ", Action: " << stat.action << ", Active: " << stat.is_active << '\n';
        }
        // Packet that would have matched the shadowed rule
        auto packet_for_shadowed = make_example_packet(shadowed_fields.src_ip, shadowed_fields.dst_ip, shadowed_fields.src_port_min, shadowed_fields.dst_port_min, shadowed_fields.protocol, shadowed_fields.eth_type);
        std::cout << "Lookup for packet matching shadowed rule's criteria: Action " << shadow_tcam.lookup_single(packet_for_shadowed) << " (Expected Action 2001 from shadowing rule)" << '\n';
    }

    // =====================================================================================
    // --- Demonstrate Redundant Rule Detection and Compaction ---
    // =====================================================================================
    std::cout << "\n\n--- Redundant Rule Detection/Compaction Example ---" << '\n';
    {
        OptimizedTCAM redundant_tcam;
        OptimizedTCAM::WildcardFields superset_fields = fields1;
        superset_fields.src_ip_mask = 0xFFFFFF00; // Superset rule: 10.0.0.0/24, Prio 100, Action 3001
        redundant_tcam.add_rule_with_ranges(superset_fields, 100, 3001); // Rule ID 0

        OptimizedTCAM::WildcardFields redundant_fields = fields1; // Redundant rule: 10.0.0.1/32 (subset of above)
                                                                // Prio 90, Action 3001 (SAME ACTION)
        redundant_tcam.add_rule_with_ranges(redundant_fields, 90, 3001);  // Rule ID 1

        std::cout << "Initial rules for redundancy demo (Rule ID 1 should be redundant to Rule ID 0):" << '\n';
        for(const auto& stat : redundant_tcam.get_all_rule_stats()) {
            std::cout << "  ID: " << stat.rule_id << ", Action: " << stat.action << ", Prio: " << stat.priority << ", Active: " << stat.is_active << '\n';
        }

        redundant_tcam.compact_redundant_rules(true); // Detect and remove (rebuild)
        std::cout << "Rule stats after compaction (Rule ID 1 should be removed):" << '\n';
        for(const auto& stat : redundant_tcam.get_all_rule_stats()) {
            std::cout << "  ID: " << stat.rule_id << ", Action: " << stat.action << ", Active: " << stat.is_active << '\n';
        }
         auto packet_for_redundant = make_example_packet(redundant_fields.src_ip, redundant_fields.dst_ip, redundant_fields.src_port_min, redundant_fields.dst_port_min, redundant_fields.protocol, redundant_fields.eth_type);
        std::cout << "Lookup for packet matching redundant rule's criteria: Action " << redundant_tcam.lookup_single(packet_for_redundant) << " (Expected Action 3001 from superset rule)" << '\n';
    }

    // =====================================================================================
    // --- Demonstrate Enhanced Rule Aging ---
    // =====================================================================================
    std::cout << "\n\n--- Enhanced Rule Aging Example ---" << '\n';
    {
        OptimizedTCAM aging_tcam;
        // Use fields1 as a base for creating aging rules
        OptimizedTCAM::WildcardFields aging_f1 = fields1;
        aging_f1.src_ip = 0x0A0A0001; // Action 4001

        OptimizedTCAM::WildcardFields aging_f2 = fields1;
        aging_f2.src_ip = 0x0A0A0002; // Action 4002

        aging_tcam.add_rule_with_ranges(aging_f1, 100, 4001);
        std::cout << "Added Rule ID 0 (Action 4001) for CREATION_TIME aging." << '\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(15));

        aging_tcam.add_rule_with_ranges(aging_f2, 100, 4002); // ID 1
        std::cout << "Added Rule ID 1 (Action 4002) for LAST_HIT_TIME aging." << '\n';

        // Hit rule 1 (Action 4002)
        auto packet_for_aging_f2 = make_example_packet(aging_f2.src_ip, aging_f2.dst_ip, aging_f2.src_port_min, aging_f2.dst_port_min, aging_f2.protocol, aging_f2.eth_type);
        aging_tcam.lookup_single(packet_for_aging_f2);
        std::cout << "Performed lookup for Rule ID 1 (Action 4002)." << '\n';

        std::this_thread::sleep_for(std::chrono::milliseconds(15)); // Rule 0 is now ~30ms old, Rule 1 is ~15ms old & hit ~15ms ago

        std::cout << "\nInitial state for aging:" << '\n';
        for(const auto& stat : aging_tcam.get_all_rule_stats()) {
            std::cout << "  ID: " << stat.rule_id << ", Action: " << stat.action << ", Active: " << stat.is_active
                      << ", Created: " << time_point_to_string(stat.creation_time)
                      << ", Last Hit: " << time_point_to_string(stat.last_hit_timestamp) << '\n';
        }

        std::cout << "\nAging CREATION_TIME > 20ms (Rule ID 0 / Action 4001 should age):" << '\n';
        std::vector<uint64_t> aged_creation = aging_tcam.age_rules(std::chrono::milliseconds(20), OptimizedTCAM::AgeCriteria::CREATION_TIME);
        for(uint64_t id : aged_creation) { std::cout << "  Aged out by creation: ID " << id << '\n';}
        for(const auto& stat : aging_tcam.get_all_rule_stats()) {
            if(stat.action == 4001) std::cout << "  Rule ID 0 (Action 4001) Active: " << stat.is_active << '\n';
        }

        std::cout << "\nAging LAST_HIT_TIME > 10ms (Rule ID 1 / Action 4002 should age as it was hit ~15ms ago):" << '\n';
        std::vector<uint64_t> aged_hit = aging_tcam.age_rules(std::chrono::milliseconds(10), OptimizedTCAM::AgeCriteria::LAST_HIT_TIME);
         for(uint64_t id : aged_hit) { std::cout << "  Aged out by last hit: ID " << id << '\n';}
        for(const auto& stat : aging_tcam.get_all_rule_stats()) {
            if(stat.action == 4002) std::cout << "  Rule ID 1 (Action 4002) Active: " << stat.is_active << '\n';
        }

        // Demonstrate never-hit rule not aging by LAST_HIT_TIME
        OptimizedTCAM::WildcardFields aging_f3 = fields1;
        aging_f3.src_ip = 0x0A0A0003;
        aging_tcam.add_rule_with_ranges(aging_f3, 100, 4003); // Rule ID will be 2
        std::cout << "\nAdded Rule ID 2 (Action 4003) (never hit)." << '\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        std::vector<uint64_t> aged_never_hit = aging_tcam.age_rules(std::chrono::milliseconds(10), OptimizedTCAM::AgeCriteria::LAST_HIT_TIME);
        std::cout << "Aging LAST_HIT_TIME > 10ms (Rule ID 2 / Action 4003 should NOT age as it was never hit):" << '\n';

        uint64_t id_action_4003 = 0; // Find ID for action 4003
        auto all_stats_aging = aging_tcam.get_all_rule_stats();
        for(const auto&s : all_stats_aging) if(s.action == 4003) id_action_4003 = s.rule_id;

        bool found_r_4003_in_aged = false;
        for(uint64_t id : aged_never_hit) { if(id == id_action_4003) found_r_4003_in_aged = true; }
        std::cout << "  Rule for Action 4003 (ID " << id_action_4003 << ") aged out: " << (found_r_4003_in_aged ? "Yes" : "No") << '\n';
        auto stats_r_4003 = aging_tcam.get_rule_stats(id_action_4003);
        if(stats_r_4003) std::cout << "  Rule for Action 4003 (ID " << id_action_4003 << ") Active: " << stats_r_4003->is_active << '\n';
    }

    // =====================================================================================
    // --- Demonstrate Backup and Restore (Line-Based Text Format) ---
    // =====================================================================================
    std::cout << "\n\n--- Backup and Restore Example ---" << '\n';
    {
        OptimizedTCAM backup_tcam;
        OptimizedTCAM::WildcardFields rule_cfg1 = fields1;
        int rule_cfg1_action = 5001; int rule_cfg1_priority = 110;
        backup_tcam.add_rule_with_ranges(rule_cfg1, rule_cfg1_priority, rule_cfg1_action);

        OptimizedTCAM::WildcardFields rule_cfg2_ports = fields1;
        rule_cfg2_ports.src_ip = 0x0A0B0C0D;
        rule_cfg2_ports.src_port_min = 7000; rule_cfg2_ports.src_port_max = 7010;
        rule_cfg2_ports.dst_port_min = 0; rule_cfg2_ports.dst_port_max = 0xFFFF;
        int rule_cfg2_action = 5002; int rule_cfg2_priority = 120;
        backup_tcam.add_rule_with_ranges(rule_cfg2_ports, rule_cfg2_priority, rule_cfg2_action);

        std::cout << "Original TCAM state for backup:" << '\n';
        for(const auto& stat : backup_tcam.get_all_rule_stats()) {
            std::cout << "  ID: " << stat.rule_id << ", Action: " << stat.action << ", Prio: " << stat.priority << ", Active: " << stat.is_active << '\n';
        }

        std::stringstream backup_stream;
        backup_tcam.backup_rules(backup_stream);
        std::cout << "\nBacked up rules (text format):\n" << backup_stream.str() << '\n';

        OptimizedTCAM restored_tcam;
        // backup_stream is now at EOF, need to reset for reading
        backup_stream.clear(); // Clear EOF flags
        backup_stream.seekg(0);    // Rewind to beginning

        bool restore_success = restored_tcam.restore_rules(backup_stream);
        std::cout << "Restore operation successful: " << (restore_success ? "Yes" : "No") << '\n';

        std::cout << "\nRestored TCAM state:" << '\n';
        if (restore_success) {
            for(const auto& stat : restored_tcam.get_all_rule_stats()) {
                std::cout << "  ID: " << stat.rule_id << ", Action: " << stat.action << ", Prio: " << stat.priority << ", Active: " << stat.is_active << '\n';
            }
            // Verify with lookups
            auto p_cfg1 = make_example_packet(rule_cfg1.src_ip, rule_cfg1.dst_ip, rule_cfg1.src_port_min, rule_cfg1.dst_port_min, rule_cfg1.protocol, rule_cfg1.eth_type);
            std::cout << "Lookup for rule 1 in restored TCAM: Action " << restored_tcam.lookup_single(p_cfg1) << " (Expected " << rule_cfg1_action << ")" << '\n';

            auto p_cfg2 = make_example_packet(rule_cfg2_ports.src_ip, rule_cfg2_ports.dst_ip, 7005, 12345, rule_cfg2_ports.protocol, rule_cfg2_ports.eth_type);
            std::cout << "Lookup for rule 2 (port range) in restored TCAM: Action " << restored_tcam.lookup_single(p_cfg2) << " (Expected " << rule_cfg2_action << ")" << '\n';

        } else {
            std::cout << "TCAM is empty due to restore failure." << '\n';
        }
    }

    std::cout << "\n\n--- All Examples Complete ---" << '\n';
    return 0;
}

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

// --- Tests for Priority and Specificity ---

// Scenario 1: Higher priority rule matches.
// Rule1: Prio 100, Specific (e.g. /32 src_ip)
// Rule2: Prio 90, Less Specific (e.g. /24 src_ip, but also matches packet)
// Expected: Rule1's action.
TEST_F(TCAMTest, HigherPriorityWinsOverLessSpecificLowerPriority) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields(); // Specific
    fields1.src_ip = 0x0A000001; fields1.src_ip_mask = 0xFFFFFFFF; // /32
    tcam.add_rule_with_ranges(fields1, 100, 101); // Action 101, Prio 100

    OptimizedTCAM::WildcardFields fields2 = create_default_fields();
    fields2.src_ip = 0x0A000000; fields2.src_ip_mask = 0xFFFFFF00; // /24
    tcam.add_rule_with_ranges(fields2, 90, 102);  // Action 102, Prio 90

    auto packet = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    EXPECT_EQ(tcam.lookup_single(packet), 101);
}

// Scenario 2: Same priority, more specific rule matches.
// Rule1: Prio 100, More Specific (e.g., /32 src_ip)
// Rule2: Prio 100, Less Specific (e.g., /24 src_ip)
// Expected: Rule1's action.
TEST_F(TCAMTest, SamePriorityMoreSpecificWins) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields(); // More specific
    fields1.src_ip = 0x0A000001; fields1.src_ip_mask = 0xFFFFFFFF; // /32
    tcam.add_rule_with_ranges(fields1, 100, 201); // Action 201

    OptimizedTCAM::WildcardFields fields2 = create_default_fields(); // Less specific
    fields2.src_ip = 0x0A000000; fields2.src_ip_mask = 0xFFFFFF00; // /24
    tcam.add_rule_with_ranges(fields2, 100, 202); // Action 202

    // Rules are added with the same priority. The add_rule_with_ranges function
    // sorts by priority then specificity. So fields1 (more specific) should end up earlier
    // if priorities are equal.

    auto packet = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    EXPECT_EQ(tcam.lookup_single(packet), 201);
}

// Scenario 3: Same priority, less specific rule also matches, but a more specific one should be chosen.
// This is similar to Scenario 2 but emphasizes that the less specific rule would also match.
// Rule1: Prio 100, Less Specific (e.g. /24 src_ip)
// Rule2: Prio 100, More Specific (e.g. /32 src_ip, matching the packet)
// Expected: Rule2's action. Order of addition might matter if not handled by internal sorting.
// TCAM's `add_rule_with_ranges` sorts by priority then specificity.
TEST_F(TCAMTest, SamePriorityMoreSpecificWinsRegardlessOfOrder) {
    OptimizedTCAM::WildcardFields fields_less_specific = create_default_fields();
    fields_less_specific.src_ip = 0x0A000000; fields_less_specific.src_ip_mask = 0xFFFFFF00; // /24
    fields_less_specific.dst_port_min = 0; fields_less_specific.dst_port_max = 0xFFFF; // Any Dst port

    OptimizedTCAM::WildcardFields fields_more_specific = create_default_fields();
    fields_more_specific.src_ip = 0x0A000001; fields_more_specific.src_ip_mask = 0xFFFFFFFF; // /32
    fields_more_specific.dst_port_min = 80; fields_more_specific.dst_port_max = 80; // Specific Dst port

    // Add less specific rule first
    tcam.add_rule_with_ranges(fields_less_specific, 100, 301); // Action 301
    // Add more specific rule second
    tcam.add_rule_with_ranges(fields_more_specific, 100, 302); // Action 302

    auto packet = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    // Expect the more specific rule's action (302) due to sorting by add_rule_with_ranges
    EXPECT_EQ(tcam.lookup_single(packet), 302);

    // Test with different order of addition
    OptimizedTCAM tcam2;
    tcam2.add_rule_with_ranges(fields_more_specific, 100, 302); // Action 302
    tcam2.add_rule_with_ranges(fields_less_specific, 100, 301); // Action 301
    EXPECT_EQ(tcam2.lookup_single(packet), 302);
}


// Scenario 4: Overlapping rules with different priorities, ensure higher priority is chosen.
// Rule1: Prio 100, Specific Field A (e.g. src_ip /32)
// Rule2: Prio 90, Specific Field B (e.g. dst_ip /32, different field)
// Packet matches both Field A and Field B.
// Expected: Rule1's action.
TEST_F(TCAMTest, HigherPriorityWinsOverDifferentFieldMatchLowerPriority) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields(); // Rule 1
    fields1.src_ip = 0x0A000001; fields1.src_ip_mask = 0xFFFFFFFF; // Specific Src IP
    // Other fields are default (e.g. dst_ip = 0xC0A80001 /32, proto=6 etc)
    tcam.add_rule_with_ranges(fields1, 100, 401); // Action 401, Prio 100

    OptimizedTCAM::WildcardFields fields2 = create_default_fields(); // Rule 2
    fields2.dst_ip = 0xC0A80001; fields2.dst_ip_mask = 0xFFFFFFFF; // Specific Dst IP
    fields2.src_port_min = 1000; fields2.src_port_max = 2000; // Different src port to make it distinct from fields1
                                                            // but the packet will match this too.
    tcam.add_rule_with_ranges(fields2, 90, 402);  // Action 402, Prio 90

    // Packet matches src_ip of Rule1 and dst_ip of Rule2, and other default fields
    auto packet = make_packet(0x0A000001, // Matches fields1 src_ip
                              0xC0A80001, // Matches fields1 dst_ip & fields2 dst_ip
                              1024,       // Matches fields1 src_port, does not match fields2's specific port range
                              80, 6, 0x0800);
    // To ensure fields2 *could* match if it were higher prio, let's make packet match fields2 src_port.
    // The key is that *both* rules match the packet on their respective criteria.
    // The original `create_default_fields()` has src_port=1024, dst_port=80
    // fields1 uses default src_port 1024.
    // fields2 is modified to use src_port 1000-2000.
    // So, a packet with src_port 1024 matches fields1.
    // A packet with src_port 1500 (e.g.) would match fields2 for src_port.

    // Packet that matches both rules completely:
    // src_ip = 0x0A000001 (for R1)
    // dst_ip = 0xC0A80001 (for R1 & R2)
    // src_port = 1024 (for R1)
    // dst_port = 80 (for R1 & R2)
    // protocol = 6 (for R1 & R2)
    // eth_type = 0x0800 (for R1 & R2)
    // This packet is a full match for R1.
    // For R2, it matches on dst_ip, protocol, eth_type, dst_port.
    // R2 has a different src_port range. So if default packet (src_port 1024) is used, it won't match R2.
    // Let's adjust R2 to also match the default packet's src_port to create a true overlap.

    OptimizedTCAM tcam_overlap;
    OptimizedTCAM::WildcardFields f_high_prio = create_default_fields();
    f_high_prio.src_ip_mask = 0xFFFFFF00; // e.g. 10.0.0.x, prio 100
    tcam_overlap.add_rule_with_ranges(f_high_prio, 100, 411);

    OptimizedTCAM::WildcardFields f_low_prio_overlap = create_default_fields();
    f_low_prio_overlap.dst_port_min = 0; // Match any dst port
    f_low_prio_overlap.dst_port_max = 0xFFFF;
    // This rule also matches the packet, but is lower priority
    tcam_overlap.add_rule_with_ranges(f_low_prio_overlap, 90, 412);

    auto packet_overlap = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    // Packet matches f_high_prio (src_ip 10.0.0.1 is in 10.0.0.0/24)
    // Packet also matches f_low_prio_overlap (dst_port 80 is in 0-65535)
    EXPECT_EQ(tcam_overlap.lookup_single(packet_overlap), 411); // Higher priority wins
}

// Scenario 5: Rules that would match in bitmap/decision tree but are lower priority
// than another matching rule (which might be found by linear scan or earlier in optimized structures).
// Rule1: Prio 100, specific (matches packet, should be chosen)
// Rule2: Prio 90, very broad (matches packet, likely to hit in bitmap/tree early if not for priority)
// Rule3: Prio 80, also broad (matches packet, also likely for bitmap/tree)
// Expected: Rule1's action.
TEST_F(TCAMTest, HigherPriorityWinsOverBroadLowerPriorityRules) {
    OptimizedTCAM::WildcardFields fields1_specific_high_prio = create_default_fields();
    fields1_specific_high_prio.src_ip = 0x0A000001; fields1_specific_high_prio.src_ip_mask = 0xFFFFFFFF;
    fields1_specific_high_prio.dst_port_min = 80; fields1_specific_high_prio.dst_port_max = 80;
    tcam.add_rule_with_ranges(fields1_specific_high_prio, 100, 501); // Action 501, Prio 100

    OptimizedTCAM::WildcardFields fields2_broad_low_prio = create_default_fields();
    fields2_broad_low_prio.src_ip_mask = 0x00000000; // Any src_ip
    fields2_broad_low_prio.dst_ip_mask = 0x00000000; // Any dst_ip
    fields2_broad_low_prio.protocol_mask = 0x00;     // Any protocol
    tcam.add_rule_with_ranges(fields2_broad_low_prio, 90, 502);  // Action 502, Prio 90

    OptimizedTCAM::WildcardFields fields3_broader_lower_prio = create_default_fields();
    fields3_broader_lower_prio.src_ip_mask = 0x00000000;
    fields3_broader_lower_prio.dst_ip_mask = 0x00000000;
    fields3_broader_lower_prio.src_port_min = 0; fields3_broader_lower_prio.src_port_max = 0xFFFF; // Any src port
    fields3_broader_lower_prio.dst_port_min = 0; fields3_broader_lower_prio.dst_port_max = 0xFFFF; // Any dst port
    fields3_broader_lower_prio.protocol_mask = 0x00;
    fields3_broader_lower_prio.eth_type_mask = 0x0000; // Any eth type
    tcam.add_rule_with_ranges(fields3_broader_lower_prio, 80, 503); // Action 503, Prio 80

    auto packet = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    // This packet matches fields1_specific_high_prio exactly.
    // It also matches fields2_broad_low_prio (src_ip/dst_ip/proto are within "any").
    // It also matches fields3_broader_lower_prio.

    // The key is that even if bitmap or decision tree structures might quickly identify
    // a match for fields2 or fields3, the lookup_single logic must ensure that
    // fields1 (due to its higher priority and confirmed match) is chosen.
    // The internal sorting of `rules` vector by priority and specificity is crucial here.
    EXPECT_EQ(tcam.lookup_single(packet), 501);
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

// --- Tests for Rule Conflict Detection ---
class TCAMConflictTest : public ::testing::Test {
protected:
    OptimizedTCAM tcam;
    OptimizedTCAM::WildcardFields rule_fields;

    // Helper to create a default rule field set
    OptimizedTCAM::WildcardFields create_default_fields_for_conflict_tests() {
        OptimizedTCAM::WildcardFields fields{};
        fields.src_ip = 0x0A000001; fields.src_ip_mask = 0xFFFFFFFF; // 10.0.0.1/32
        fields.dst_ip = 0xC0A80001; fields.dst_ip_mask = 0xFFFFFFFF; // 192.168.0.1/32
        fields.src_port_min = 1024; fields.src_port_max = 1024;     // TCP 1024
        fields.dst_port_min = 80;   fields.dst_port_max = 80;       // TCP 80
        fields.protocol = 6;        fields.protocol_mask = 0xFF;    // TCP
        fields.eth_type = 0x0800;   fields.eth_type_mask = 0xFFFF;  // IPv4
        return fields;
    }
};

TEST_F(TCAMConflictTest, NoRulesNoConflicts) {
    auto conflicts = tcam.detect_conflicts();
    EXPECT_TRUE(conflicts.empty());
}

TEST_F(TCAMConflictTest, DisjointRulesNoConflicts) {
    rule_fields = create_default_fields_for_conflict_tests();
    tcam.add_rule_with_ranges(rule_fields, 100, 1);

    rule_fields.src_ip = 0x0A000002; // Different Source IP
    tcam.add_rule_with_ranges(rule_fields, 100, 2);

    auto conflicts = tcam.detect_conflicts();
    EXPECT_TRUE(conflicts.empty());
}

TEST_F(TCAMConflictTest, OverlappingRulesSameActionNoConflicts) {
    rule_fields = create_default_fields_for_conflict_tests();
    tcam.add_rule_with_ranges(rule_fields, 100, 1); // Action 1

    rule_fields.src_ip_mask = 0xFFFFFF00; // Overlaps with the first rule (10.0.0.0/24)
    tcam.add_rule_with_ranges(rule_fields, 90, 1);  // Same Action 1

    auto conflicts = tcam.detect_conflicts();
    EXPECT_TRUE(conflicts.empty());
}

TEST_F(TCAMConflictTest, ExactMatchDifferentActionsConflict) {
    rule_fields = create_default_fields_for_conflict_tests();
    // Rule 0: prio 100, action 1
    tcam.add_rule_with_ranges(rule_fields, 100, 1);
    // Rule 1: prio 90, action 2 (same fields)
    tcam.add_rule_with_ranges(rule_fields, 90, 2);

    auto conflicts = tcam.detect_conflicts();
    ASSERT_EQ(conflicts.size(), 1);
    // Higher priority rule (100) should be index 0. Lower (90) index 1.
    EXPECT_EQ(conflicts[0].rule1_idx, 0);
    EXPECT_EQ(conflicts[0].rule2_idx, 1);
}

TEST_F(TCAMConflictTest, SubsetRuleConflict) {
    // Rule A (more specific): 10.0.0.1/32, Action 1, Prio 100 -> Expected index 0
    OptimizedTCAM::WildcardFields fields_A = create_default_fields_for_conflict_tests();
    fields_A.src_ip = 0x0A000001; fields_A.src_ip_mask = 0xFFFFFFFF;
    tcam.add_rule_with_ranges(fields_A, 100, 1);

    // Rule B (less specific, superset of A): 10.0.0.0/24, Action 2, Prio 90 -> Expected index 1
    OptimizedTCAM::WildcardFields fields_B = create_default_fields_for_conflict_tests();
    fields_B.src_ip = 0x0A000000; fields_B.src_ip_mask = 0xFFFFFF00;
    tcam.add_rule_with_ranges(fields_B, 90, 2);

    auto conflicts = tcam.detect_conflicts();
    ASSERT_EQ(conflicts.size(), 1);
    EXPECT_EQ(conflicts[0].rule1_idx, 0); // Rule A (index 0)
    EXPECT_EQ(conflicts[0].rule2_idx, 1); // Rule B (index 1)
}

TEST_F(TCAMConflictTest, SupersetRuleConflict) {
    // Rule A (less specific): 10.0.0.0/24, Action 1, Prio 100 -> Expected index 0
    OptimizedTCAM::WildcardFields fields_A = create_default_fields_for_conflict_tests();
    fields_A.src_ip = 0x0A000000; fields_A.src_ip_mask = 0xFFFFFF00;
    tcam.add_rule_with_ranges(fields_A, 100, 1);

    // Rule B (more specific, subset of A): 10.0.0.1/32, Action 2, Prio 90 -> Expected index 1
    OptimizedTCAM::WildcardFields fields_B = create_default_fields_for_conflict_tests();
    fields_B.src_ip = 0x0A000001; fields_B.src_ip_mask = 0xFFFFFFFF;
    tcam.add_rule_with_ranges(fields_B, 90, 2);

    auto conflicts = tcam.detect_conflicts();
    ASSERT_EQ(conflicts.size(), 1);
    EXPECT_EQ(conflicts[0].rule1_idx, 0); // Rule A (index 0)
    EXPECT_EQ(conflicts[0].rule2_idx, 1); // Rule B (index 1)
}

TEST_F(TCAMConflictTest, PartialOverlapIPConflict) {
    // Rule A: 10.0.0.0/24 (10.0.0.0 - 10.0.0.255), Action 1, Prio 100
    OptimizedTCAM::WildcardFields fields_A = create_default_fields_for_conflict_tests();
    fields_A.src_ip = 0x0A000000; fields_A.src_ip_mask = 0xFFFFFF00;
    tcam.add_rule_with_ranges(fields_A, 100, 1); // Expected index 0

    // Rule B: 10.0.0.128/25 (10.0.0.128 - 10.0.0.255), Action 2, Prio 90
    OptimizedTCAM::WildcardFields fields_B = create_default_fields_for_conflict_tests();
    fields_B.src_ip = 0x0A000080; fields_B.src_ip_mask = 0xFFFFFF80;
    tcam.add_rule_with_ranges(fields_B, 90, 2);  // Expected index 1

    auto conflicts = tcam.detect_conflicts();
    ASSERT_EQ(conflicts.size(), 1);
    EXPECT_EQ(conflicts[0].rule1_idx, 0);
    EXPECT_EQ(conflicts[0].rule2_idx, 1);
}

TEST_F(TCAMConflictTest, PortRangeOverlapConflict) {
    // Rule A: src_port 1000-2000, Action 1, Prio 100
    OptimizedTCAM::WildcardFields fields_A = create_default_fields_for_conflict_tests();
    fields_A.src_port_min = 1000; fields_A.src_port_max = 2000;
    tcam.add_rule_with_ranges(fields_A, 100, 1); // Expected index 0

    // Rule B: src_port 1500-2500 (overlaps with A), Action 2, Prio 90
    OptimizedTCAM::WildcardFields fields_B = create_default_fields_for_conflict_tests();
    fields_B.src_port_min = 1500; fields_B.src_port_max = 2500;
    tcam.add_rule_with_ranges(fields_B, 90, 2);  // Expected index 1

    auto conflicts = tcam.detect_conflicts();
    ASSERT_EQ(conflicts.size(), 1);
    EXPECT_EQ(conflicts[0].rule1_idx, 0);
    EXPECT_EQ(conflicts[0].rule2_idx, 1);
}

TEST_F(TCAMConflictTest, WildcardOverlapConflict) {
    // Rule A: 10.0.0.1/32, Action 1, Prio 100
    OptimizedTCAM::WildcardFields fields_A = create_default_fields_for_conflict_tests();
    tcam.add_rule_with_ranges(fields_A, 100, 1); // Expected index 0

    // Rule B: 0.0.0.0/0 (ANY source IP), Action 2, Prio 90
    OptimizedTCAM::WildcardFields fields_B = create_default_fields_for_conflict_tests();
    fields_B.src_ip = 0x00000000; fields_B.src_ip_mask = 0x00000000;
    tcam.add_rule_with_ranges(fields_B, 90, 2);  // Expected index 1

    auto conflicts = tcam.detect_conflicts();
    ASSERT_EQ(conflicts.size(), 1);
    EXPECT_EQ(conflicts[0].rule1_idx, 0);
    EXPECT_EQ(conflicts[0].rule2_idx, 1);
}

TEST_F(TCAMConflictTest, MultipleConflicts) {
    // Rules are added and sorted by priority, then specificity.
    // Rule 1 (Action 1, Prio 100, Specific: 10.0.0.1, 192.168.0.1) -> Should be idx 0
    OptimizedTCAM::WildcardFields r1_f = create_default_fields_for_conflict_tests();
    tcam.add_rule_with_ranges(r1_f, 100, 1);

    // Rule 3 (Action 3, Prio 100, Specific: 10.0.0.1, 192.168.0.2) -> Should be idx 1
    OptimizedTCAM::WildcardFields r3_f = create_default_fields_for_conflict_tests();
    r3_f.dst_ip = 0xC0A80002;
    tcam.add_rule_with_ranges(r3_f, 100, 3);

    // Rule 2 (Action 2, Prio 90, Specific: 10.0.0.1, 192.168.0.1) -> Should be idx 2 (Conflicts with R1@idx0)
    OptimizedTCAM::WildcardFields r2_f = create_default_fields_for_conflict_tests();
    tcam.add_rule_with_ranges(r2_f, 90, 2);

    // Rule 4 (Action 4, Prio 90, Less Specific: ANY src_ip, 192.168.0.2) -> Should be idx 3 (Conflicts with R3@idx1)
    OptimizedTCAM::WildcardFields r4_f = create_default_fields_for_conflict_tests();
    r4_f.src_ip_mask = 0x00000000;
    r4_f.dst_ip = 0xC0A80002;
    tcam.add_rule_with_ranges(r4_f, 90, 4);

    auto conflicts = tcam.detect_conflicts();
    ASSERT_EQ(conflicts.size(), 2); // Ensure exactly two conflicts are found

    // Original rule definitions:
    // R1_f (Action 1, Prio 100, Specific-IP1, Specific-DP1)
    // R3_f (Action 3, Prio 100, Specific-IP1, Specific-DP2)
    // R2_f (Action 2, Prio 90, Specific-IP1, Specific-DP1) -> Overlaps R1
    // R4_f (Action 4, Prio 90, Wildcard-IP, Specific-DP2) -> Overlaps R3

    // Scenario 1: Assumed initial order R1(0), R3(1), R2(2), R4(3)
    // Conflict R1(idx0) vs R2(idx2)
    // Conflict R3(idx1) vs R4(idx3)
    bool scenario1_met = false;
    if (((conflicts[0].rule1_idx == 0 && conflicts[0].rule2_idx == 2) || (conflicts[0].rule1_idx == 2 && conflicts[0].rule2_idx == 0)) &&
        ((conflicts[1].rule1_idx == 1 && conflicts[1].rule2_idx == 3) || (conflicts[1].rule1_idx == 3 && conflicts[1].rule2_idx == 1))) {
        scenario1_met = true;
    } else if (((conflicts[0].rule1_idx == 1 && conflicts[0].rule2_idx == 3) || (conflicts[0].rule1_idx == 3 && conflicts[0].rule2_idx == 1)) &&
               ((conflicts[1].rule1_idx == 0 && conflicts[1].rule2_idx == 2) || (conflicts[1].rule1_idx == 2 && conflicts[1].rule2_idx == 0))) {
        // This covers the case where the conflicts vector itself is ordered differently: e.g. {(1,3), (0,2)} vs {(0,2), (1,3)}
        scenario1_met = true;
    }

    // Scenario 2: Order is R3(0), R1(1), R2(2), R4(3) due to sort stability of equal elements
    // Conflict R1(idx1) vs R2(idx2)
    // Conflict R3(idx0) vs R4(idx3)
    bool scenario2_met = false;
    if (((conflicts[0].rule1_idx == 1 && conflicts[0].rule2_idx == 2) || (conflicts[0].rule1_idx == 2 && conflicts[0].rule2_idx == 1)) &&
        ((conflicts[1].rule1_idx == 0 && conflicts[1].rule2_idx == 3) || (conflicts[1].rule1_idx == 3 && conflicts[1].rule2_idx == 0))) {
        scenario2_met = true;
    } else if (((conflicts[0].rule1_idx == 0 && conflicts[0].rule2_idx == 3) || (conflicts[0].rule1_idx == 3 && conflicts[0].rule2_idx == 0)) &&
               ((conflicts[1].rule1_idx == 1 && conflicts[1].rule2_idx == 2) || (conflicts[1].rule1_idx == 2 && conflicts[1].rule2_idx == 1))) {
        // This covers the case where the conflicts vector itself is ordered differently for scenario 2
        scenario2_met = true;
    }

    EXPECT_TRUE(scenario1_met || scenario2_met) << "Expected conflict pairs not found. "
        << "Reported Conflict 1: {" << conflicts[0].rule1_idx << "," << conflicts[0].rule2_idx << "} "
        << "Reported Conflict 2: {" << conflicts[1].rule1_idx << "," << conflicts[1].rule2_idx << "}";
}

// Note: Existing tests like AllLookupsComparison might need adjustment
// if the internal lookup_linear, lookup_bitmap, lookup_decision_tree methods
// are changed to also return indices or are removed in favor of _idx versions
// for internal use by lookup_single. For now, they are assumed to still work
// for their original purpose or will be updated if this subtask implied changes to them.
// Based on previous subtasks, they were changed to _idx and lookup_single uses them.
// The current tests that use them directly would fail if they expect actions.
// They have been updated to use lookup_single for simplicity in this pass.


// --- Tests for Shadow Rule Detection and Elimination ---
class TCAMShadowRuleTest : public ::testing::Test {
protected:
    OptimizedTCAM tcam;

    OptimizedTCAM::WildcardFields create_default_fields_for_shadow_tests() {
        OptimizedTCAM::WildcardFields fields{};
        fields.src_ip = 0x0A000001; fields.src_ip_mask = 0xFFFFFFFF; // 10.0.0.1/32
        fields.dst_ip = 0xC0A80001; fields.dst_ip_mask = 0xFFFFFFFF; // 192.168.0.1/32
        fields.src_port_min = 1024; fields.src_port_max = 1024;
        fields.dst_port_min = 80;   fields.dst_port_max = 80;
        fields.protocol = 6;        fields.protocol_mask = 0xFF;    // TCP
        fields.eth_type = 0x0800;   fields.eth_type_mask = 0xFFFF;  // IPv4
        return fields;
    }

    // Helper to count active rules
    size_t count_active_rules() {
        size_t active_count = 0;
        auto all_stats = tcam.get_all_rule_stats();
        for (const auto& stat : all_stats) {
            if (stat.is_active) {
                active_count++;
            }
        }
        return active_count;
    }
};

TEST_F(TCAMShadowRuleTest, DetectNoRules) {
    auto shadowed_indices = tcam.detect_shadowed_rules();
    EXPECT_TRUE(shadowed_indices.empty());
}

TEST_F(TCAMShadowRuleTest, DetectNoShadowedRules) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields_for_shadow_tests();
    tcam.add_rule_with_ranges(fields1, 100, 1); // Rule 0 (ID 0)

    OptimizedTCAM::WildcardFields fields2 = create_default_fields_for_shadow_tests();
    fields2.src_ip = 0x0A000002; // Different IP
    tcam.add_rule_with_ranges(fields2, 90, 2);  // Rule 1 (ID 1)

    auto shadowed_indices = tcam.detect_shadowed_rules();
    EXPECT_TRUE(shadowed_indices.empty());
}

TEST_F(TCAMShadowRuleTest, DetectSimpleShadowing) {
    // Rule B (Shadowing Rule): Prio 100, Action 1
    OptimizedTCAM::WildcardFields fields_B = create_default_fields_for_shadow_tests();
    fields_B.src_ip_mask = 0xFFFFFF00; // 10.0.0.0/24
    tcam.add_rule_with_ranges(fields_B, 100, 1); // Rule 0 (ID 0)

    // Rule A (Shadowed Rule): Prio 90, Action 2, subset of B
    OptimizedTCAM::WildcardFields fields_A = create_default_fields_for_shadow_tests();
    fields_A.src_ip = 0x0A000001; fields_A.src_ip_mask = 0xFFFFFFFF; // 10.0.0.1/32
    // Ensure other fields match fields_B for subset condition where fields_B is specific
    fields_A.dst_ip = fields_B.dst_ip;
    fields_A.dst_ip_mask = fields_B.dst_ip_mask;
    fields_A.src_port_min = fields_B.src_port_min; fields_A.src_port_max = fields_B.src_port_max;
    fields_A.dst_port_min = fields_B.dst_port_min; fields_A.dst_port_max = fields_B.dst_port_max;
    fields_A.protocol = fields_B.protocol; fields_A.protocol_mask = fields_B.protocol_mask;
    fields_A.eth_type = fields_B.eth_type; fields_A.eth_type_mask = fields_B.eth_type_mask;

    tcam.add_rule_with_ranges(fields_A, 90, 2);  // Rule 1 (ID 1)

    // Expected order: Rule B (idx 0, prio 100), Rule A (idx 1, prio 90)
    // Rule A (idx 1) is subset of Rule B (idx 0) and has different action.
    auto shadowed_indices = tcam.detect_shadowed_rules();
    ASSERT_EQ(shadowed_indices.size(), 1);
    EXPECT_EQ(shadowed_indices[0], 1); // Rule A at index 1 should be shadowed
}

TEST_F(TCAMShadowRuleTest, DetectShadowingWithWildcards) {
    // Rule B (Shadowing Rule, more general): Prio 100, Action 1, Any Src IP
    OptimizedTCAM::WildcardFields fields_B = create_default_fields_for_shadow_tests();
    fields_B.src_ip_mask = 0x00000000; // Any source IP
    tcam.add_rule_with_ranges(fields_B, 100, 1); // Rule 0 (ID 0)

    // Rule A (Shadowed Rule): Prio 90, Action 2, Specific Src IP (subset of B)
    OptimizedTCAM::WildcardFields fields_A = create_default_fields_for_shadow_tests();
    fields_A.src_ip = 0x0A000001; fields_A.src_ip_mask = 0xFFFFFFFF; // Specific IP
    tcam.add_rule_with_ranges(fields_A, 90, 2);  // Rule 1 (ID 1)

    auto shadowed_indices = tcam.detect_shadowed_rules();
    ASSERT_EQ(shadowed_indices.size(), 1);
    EXPECT_EQ(shadowed_indices[0], 1); // Rule A at index 1 is shadowed
}

TEST_F(TCAMShadowRuleTest, EliminateDryRun) {
    OptimizedTCAM::WildcardFields fields_B = create_default_fields_for_shadow_tests();
    fields_B.src_ip_mask = 0xFFFFFF00; // 10.0.0.0/24
    tcam.add_rule_with_ranges(fields_B, 100, 1); // Rule ID 0

    OptimizedTCAM::WildcardFields fields_A = create_default_fields_for_shadow_tests();
    fields_A.src_ip = 0x0A000001;
    tcam.add_rule_with_ranges(fields_A, 90, 2);  // Rule ID 1 (shadowed)

    size_t active_rules_before = count_active_rules(); // Should be 2
    EXPECT_EQ(active_rules_before, 2);

    std::vector<uint64_t> shadowed_ids = tcam.eliminate_shadowed_rules(true /* dry_run */);
    ASSERT_EQ(shadowed_ids.size(), 1);
    EXPECT_EQ(shadowed_ids[0], 1); // Rule ID 1 is shadowed

    size_t active_rules_after = count_active_rules();
    EXPECT_EQ(active_rules_after, 2); // No change in active rules

    auto rule1_stats = tcam.get_rule_stats(1);
    ASSERT_TRUE(rule1_stats.has_value());
    EXPECT_TRUE(rule1_stats.value().is_active); // Still active
}

TEST_F(TCAMShadowRuleTest, EliminateAndVerify) {
    // Rule B (Shadowing): 10.0.0.0/24, Prio 100, Action 10 (ID 0)
    OptimizedTCAM::WildcardFields fields_B = create_default_fields_for_shadow_tests();
    fields_B.src_ip_mask = 0xFFFFFF00;
    tcam.add_rule_with_ranges(fields_B, 100, 10);

    // Rule A (Shadowed): 10.0.0.1/32, Prio 90, Action 20 (ID 1)
    OptimizedTCAM::WildcardFields fields_A = create_default_fields_for_shadow_tests();
    fields_A.src_ip = 0x0A000001;
    tcam.add_rule_with_ranges(fields_A, 90, 20);

    auto packet_matches_A_and_B = make_packet(0x0A000001, fields_A.dst_ip, fields_A.src_port_min, fields_A.dst_port_min, fields_A.protocol, fields_A.eth_type);
    EXPECT_EQ(tcam.lookup_single(packet_matches_A_and_B), 10); // Hits Rule B (shadowing rule) due to prio

    std::vector<uint64_t> shadowed_ids = tcam.eliminate_shadowed_rules(false /* dry_run */);
    ASSERT_EQ(shadowed_ids.size(), 1);
    EXPECT_EQ(shadowed_ids[0], 1); // Rule ID 1 (Action 20) was eliminated

    EXPECT_EQ(count_active_rules(), 1); // Only Rule B should be active

    auto rule0_stats = tcam.get_rule_stats(0); // ID 0, Action 10
    ASSERT_TRUE(rule0_stats.has_value());
    EXPECT_TRUE(rule0_stats.value().is_active);

    auto rule1_stats = tcam.get_rule_stats(1); // ID 1, Action 20
    // After eliminate_shadowed_rules(false) and rebuild_optimized_structures(),
    // the rule with ID 1 should be physically removed from the rules vector.
    EXPECT_FALSE(rule1_stats.has_value()); // Expect rule to be gone

    // Lookup should still hit Rule B (Action 10)
    EXPECT_EQ(tcam.lookup_single(packet_matches_A_and_B), 10);
}

TEST_F(TCAMShadowRuleTest, EliminateNoShadowedRules) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields_for_shadow_tests();
    tcam.add_rule_with_ranges(fields1, 100, 1); // ID 0
    OptimizedTCAM::WildcardFields fields2 = create_default_fields_for_shadow_tests();
    fields2.src_ip = 0x0A000002;
    tcam.add_rule_with_ranges(fields2, 90, 2);  // ID 1

    EXPECT_EQ(count_active_rules(), 2);
    std::vector<uint64_t> shadowed_ids_dry = tcam.eliminate_shadowed_rules(true);
    EXPECT_TRUE(shadowed_ids_dry.empty());
    EXPECT_EQ(count_active_rules(), 2);

    std::vector<uint64_t> shadowed_ids_wet = tcam.eliminate_shadowed_rules(false);
    EXPECT_TRUE(shadowed_ids_wet.empty());
    EXPECT_EQ(count_active_rules(), 2);
}

// It's good practice to have a main for gtest if not linking with gtest_main
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

// --- Tests for Advanced Rule Creation with Port Ranges via Atomic Updates ---
class TCAMAdvancedRuleCreationTest : public ::testing::Test {
protected:
    OptimizedTCAM tcam;

    OptimizedTCAM::WildcardFields create_base_fields(uint32_t src_ip_val = 0x0A000001, uint32_t dst_ip_val = 0xC0A80001) {
        OptimizedTCAM::WildcardFields fields{};
        fields.src_ip = src_ip_val; fields.src_ip_mask = 0xFFFFFFFF;
        fields.dst_ip = dst_ip_val; fields.dst_ip_mask = 0xFFFFFFFF;
        // Default to exact ports, can be overridden by tests
        fields.src_port_min = 1024; fields.src_port_max = 1024;
        fields.dst_port_min = 80;   fields.dst_port_max = 80;
        fields.protocol = 6;        fields.protocol_mask = 0xFF;    // TCP
        fields.eth_type = 0x0800;   fields.eth_type_mask = 0xFFFF;  // IPv4
        return fields;
    }

    size_t count_active_rules() {
        size_t active_count = 0;
        auto all_stats = tcam.get_all_rule_stats();
        for (const auto& stat : all_stats) {
            if (stat.is_active) {
                active_count++;
            }
        }
        return active_count;
    }

    std::optional<OptimizedTCAM::RuleStats> get_stats_by_action(int action_val) {
        auto all_stats = tcam.get_all_rule_stats();
        for (const auto& stat : all_stats) {
            if (stat.action == action_val) {
                return stat;
            }
        }
        return std::nullopt;
    }
};

TEST_F(TCAMAdvancedRuleCreationTest, SingleRuleNewSrcPortRange) {
    OptimizedTCAM::WildcardFields fields = create_base_fields();
    fields.src_port_min = 2000;
    fields.src_port_max = 2005;

    OptimizedTCAM::RuleOperation op = OptimizedTCAM::RuleOperation::AddRule(fields, 100, 10);
    OptimizedTCAM::RuleUpdateBatch batch = {op};

    EXPECT_TRUE(tcam.update_rules_atomic(batch));
    ASSERT_EQ(count_active_rules(), 1);

    auto stats = get_stats_by_action(10);
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->priority, 100);

    EXPECT_EQ(tcam.lookup_single(make_packet(fields.src_ip, fields.dst_ip, 1999, fields.dst_port_min, fields.protocol, fields.eth_type)), -1);
    EXPECT_EQ(tcam.lookup_single(make_packet(fields.src_ip, fields.dst_ip, 2000, fields.dst_port_min, fields.protocol, fields.eth_type)), 10);
    EXPECT_EQ(tcam.lookup_single(make_packet(fields.src_ip, fields.dst_ip, 2005, fields.dst_port_min, fields.protocol, fields.eth_type)), 10);
    EXPECT_EQ(tcam.lookup_single(make_packet(fields.src_ip, fields.dst_ip, 2006, fields.dst_port_min, fields.protocol, fields.eth_type)), -1);
}

TEST_F(TCAMAdvancedRuleCreationTest, SingleRuleNewDstPortRange) {
    OptimizedTCAM::WildcardFields fields = create_base_fields();
    fields.dst_port_min = 3000;
    fields.dst_port_max = 3005;

    OptimizedTCAM::RuleOperation op = OptimizedTCAM::RuleOperation::AddRule(fields, 100, 20);
    OptimizedTCAM::RuleUpdateBatch batch = {op};

    EXPECT_TRUE(tcam.update_rules_atomic(batch));
    ASSERT_EQ(count_active_rules(), 1);
    EXPECT_EQ(tcam.lookup_single(make_packet(fields.src_ip, fields.dst_ip, fields.src_port_min, 3000, fields.protocol, fields.eth_type)), 20);
}

TEST_F(TCAMAdvancedRuleCreationTest, SingleRuleBothNewPortRanges) {
    OptimizedTCAM::WildcardFields fields = create_base_fields();
    fields.src_port_min = 2000; fields.src_port_max = 2005;
    fields.dst_port_min = 3000; fields.dst_port_max = 3005;

    OptimizedTCAM::RuleOperation op = OptimizedTCAM::RuleOperation::AddRule(fields, 100, 30);
    OptimizedTCAM::RuleUpdateBatch batch = {op};
    EXPECT_TRUE(tcam.update_rules_atomic(batch));
    ASSERT_EQ(count_active_rules(), 1);
    EXPECT_EQ(tcam.lookup_single(make_packet(fields.src_ip, fields.dst_ip, 2002, 3003, fields.protocol, fields.eth_type)), 30);
}

TEST_F(TCAMAdvancedRuleCreationTest, MultipleRulesDistinctPortRanges) {
    OptimizedTCAM::RuleUpdateBatch batch;
    OptimizedTCAM::WildcardFields f1 = create_base_fields(0x0A000001);
    f1.src_port_min = 2000; f1.src_port_max = 2005;
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(f1, 100, 41));

    OptimizedTCAM::WildcardFields f2 = create_base_fields(0x0A000002);
    f2.dst_port_min = 3000; f2.dst_port_max = 3005;
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(f2, 100, 42));

    EXPECT_TRUE(tcam.update_rules_atomic(batch));
    ASSERT_EQ(count_active_rules(), 2);
    EXPECT_EQ(tcam.lookup_single(make_packet(f1.src_ip, f1.dst_ip, 2002, f1.dst_port_min, f1.protocol, f1.eth_type)), 41);
    EXPECT_EQ(tcam.lookup_single(make_packet(f2.src_ip, f2.dst_ip, f2.src_port_min, 3003, f2.protocol, f2.eth_type)), 42);
}

TEST_F(TCAMAdvancedRuleCreationTest, MultipleRulesReusingPortRangesInBatch) {
    OptimizedTCAM::RuleUpdateBatch batch;
    OptimizedTCAM::WildcardFields f1 = create_base_fields(0x0A000001); // Src: 2000-2005, Dst: Exact 80
    f1.src_port_min = 2000; f1.src_port_max = 2005;
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(f1, 100, 51));

    OptimizedTCAM::WildcardFields f2 = create_base_fields(0x0A000002); // Src: 2000-2005 (same), Dst: Exact 8080
    f2.src_port_min = 2000; f2.src_port_max = 2005;
    f2.dst_port_min = 8080; f2.dst_port_max = 8080;
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(f2, 90, 52));

    EXPECT_TRUE(tcam.update_rules_atomic(batch));
    ASSERT_EQ(count_active_rules(), 2);
    // This implicitly tests that add_port_range_temp_lambda reuses the range {2000,2005}
    // resulting in fewer overall port_range entries than if they were distinct.
    EXPECT_EQ(tcam.lookup_single(make_packet(f1.src_ip, f1.dst_ip, 2002, f1.dst_port_min, f1.protocol, f1.eth_type)), 51);
    EXPECT_EQ(tcam.lookup_single(make_packet(f2.src_ip, f2.dst_ip, 2003, f2.dst_port_min, f2.protocol, f2.eth_type)), 52);
}

TEST_F(TCAMAdvancedRuleCreationTest, MimicFailingBackupRestoreCase) {
    OptimizedTCAM::RuleUpdateBatch batch;

    // Rule 1 (like r1f from BackupAndRestoreMultipleRules): Exact ports
    OptimizedTCAM::WildcardFields wf1 = create_base_fields(0x0A000001, 0xC0A80001);
    wf1.src_port_min = 1024; wf1.src_port_max = 1024;
    wf1.dst_port_min = 80;   wf1.dst_port_max = 80;
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(wf1, 100, 10));

    // Rule 2 (like r2f): Less specific IP, exact ports
    OptimizedTCAM::WildcardFields wf2 = create_base_fields(0x0A000000, 0xC0A80001); // IP 10.0.0.x
    wf2.src_ip_mask = 0xFFFFFF00;
    wf2.src_port_min = 1024; wf2.src_port_max = 1024;
    wf2.dst_port_min = 80;   wf2.dst_port_max = 80;
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(wf2, 90, 20));

    // Rule 3 (like r3f): Src port range, exact Dst port
    OptimizedTCAM::WildcardFields wf3 = create_base_fields(0x0B000001, 0xC0A80001);
    wf3.src_port_min = 2000; wf3.src_port_max = 3000;
    wf3.dst_port_min = 80;   wf3.dst_port_max = 80;
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(wf3, 100, 30));

    EXPECT_TRUE(tcam.update_rules_atomic(batch));
    ASSERT_EQ(count_active_rules(), 3);

    auto p1 = make_packet(wf1.src_ip, wf1.dst_ip, wf1.src_port_min, wf1.dst_port_min, wf1.protocol, wf1.eth_type);
    auto p2 = make_packet(0x0A0000FE, wf2.dst_ip, wf2.src_port_min, wf2.dst_port_min, wf2.protocol, wf2.eth_type);
    auto p3 = make_packet(wf3.src_ip, wf3.dst_ip, 2500, wf3.dst_port_min, wf3.protocol, wf3.eth_type);

    EXPECT_EQ(tcam.lookup_single(p1), 10);
    EXPECT_EQ(tcam.lookup_single(p2), 20);
    EXPECT_EQ(tcam.lookup_single(p3), 30);
}

// --- Tests for Backup and Restore ---
#include <sstream> // Required for std::stringstream

class TCAMBackupRestoreTest : public ::testing::Test {
protected:
    OptimizedTCAM tcam;

    OptimizedTCAM::WildcardFields create_default_fields(uint32_t src_ip_val = 0x0A000001,
                                                        uint32_t dst_ip_val = 0xC0A80001) {
        OptimizedTCAM::WildcardFields fields{};
        fields.src_ip = src_ip_val; fields.src_ip_mask = 0xFFFFFFFF;
        fields.dst_ip = dst_ip_val; fields.dst_ip_mask = 0xFFFFFFFF;
        fields.src_port_min = 1024; fields.src_port_max = 1024;
        fields.dst_port_min = 80;   fields.dst_port_max = 80;
        fields.protocol = 6;        fields.protocol_mask = 0xFF;
        fields.eth_type = 0x0800;   fields.eth_type_mask = 0xFFFF;
        return fields;
    }
     size_t count_active_rules() {
        size_t active_count = 0;
        auto all_stats = tcam.get_all_rule_stats();
        for (const auto& stat : all_stats) {
            if (stat.is_active) {
                active_count++;
            }
        }
        return active_count;
    }
};

TEST_F(TCAMBackupRestoreTest, BackupAndRestoreEmpty) {
    std::stringstream ss;
    tcam.backup_rules(ss);
    // With the new line-based format, an empty TCAM results in an empty string.
    EXPECT_EQ(ss.str(), "");

    OptimizedTCAM tcam_restored;
    std::stringstream input_ss(ss.str());
    EXPECT_TRUE(tcam_restored.restore_rules(input_ss));
    EXPECT_EQ(count_active_rules(), 0); // Original tcam

    // Check restored tcam
    auto restored_stats = tcam_restored.get_all_rule_stats();
    EXPECT_EQ(restored_stats.size(), 0);
}

TEST_F(TCAMBackupRestoreTest, BackupAndRestoreSingleRule) {
    OptimizedTCAM::WildcardFields rule1_fields = create_default_fields();
    tcam.add_rule_with_ranges(rule1_fields, 100, 1);

    std::stringstream ss;
    tcam.backup_rules(ss);

    OptimizedTCAM tcam_restored;
    std::stringstream input_ss(ss.str());
    EXPECT_TRUE(tcam_restored.restore_rules(input_ss));

    auto restored_stats = tcam_restored.get_all_rule_stats();
    ASSERT_EQ(restored_stats.size(), 1);
    // Rule IDs are reassigned on restore, so we verify by action.
    bool found_rule = false;
    for (const auto& stat : restored_stats) {
        if (stat.action == 1 && stat.priority == 100) {
            found_rule = true;
            break;
        }
    }
    ASSERT_TRUE(found_rule);
    EXPECT_EQ(tcam_restored.lookup_single(make_packet(rule1_fields.src_ip, rule1_fields.dst_ip, rule1_fields.src_port_min, rule1_fields.dst_port_min, rule1_fields.protocol, rule1_fields.eth_type)), 1);
}

TEST_F(TCAMBackupRestoreTest, BackupAndRestoreMultipleRules) {
    OptimizedTCAM::WildcardFields r1f = create_default_fields(0x0A000001);
    tcam.add_rule_with_ranges(r1f, 100, 10);
    OptimizedTCAM::WildcardFields r2f = create_default_fields(0x0A000002);
    r2f.src_ip_mask = 0xFFFFFF00;
    tcam.add_rule_with_ranges(r2f, 90, 20);
    OptimizedTCAM::WildcardFields r3f = create_default_fields(0x0B000001);
    r3f.src_port_min = 2000; r3f.src_port_max = 3000;
    tcam.add_rule_with_ranges(r3f, 100, 30);

    std::stringstream ss;
    tcam.backup_rules(ss);

    OptimizedTCAM tcam_restored;
    std::stringstream input_ss(ss.str());
    EXPECT_TRUE(tcam_restored.restore_rules(input_ss));

    auto restored_stats = tcam_restored.get_all_rule_stats();
    ASSERT_EQ(restored_stats.size(), 3);

    auto p1 = make_packet(r1f.src_ip, r1f.dst_ip, r1f.src_port_min, r1f.dst_port_min, r1f.protocol, r1f.eth_type);
    auto p2 = make_packet(0x0A0000FE, r2f.dst_ip, r2f.src_port_min, r2f.dst_port_min, r2f.protocol, r2f.eth_type);
    auto p3 = make_packet(r3f.src_ip, r3f.dst_ip, 2500, r3f.dst_port_min, r3f.protocol, r3f.eth_type);

    EXPECT_EQ(tcam_restored.lookup_single(p1), 10);
    EXPECT_EQ(tcam_restored.lookup_single(p2), 20);
    EXPECT_EQ(tcam_restored.lookup_single(p3), 30);
}

TEST_F(TCAMBackupRestoreTest, RestoreCorruptedDataMissingField) {
    // Line format: src_ip src_ip_mask dst_ip dst_ip_mask src_port_mode p1 p2 dst_port_mode p3 p4 proto proto_mask eth_type eth_type_mask prio action
    std::string corrupted_data = "167772161 4294967295 3232235521 4294967295 E 1024 0 E 80 0 6 255 2048 65535 100"; // Missing action
    std::stringstream input_ss(corrupted_data);
    EXPECT_FALSE(tcam.restore_rules(input_ss));
    EXPECT_EQ(count_active_rules(), 0);
}

TEST_F(TCAMBackupRestoreTest, RestoreCorruptedDataInvalidValue) {
    std::string corrupted_data = "167772161 4294967295 3232235521 4294967295 E 1024 0 E 80 0 not-a-number 255 2048 65535 100 1";
    std::stringstream input_ss(corrupted_data);
    EXPECT_FALSE(tcam.restore_rules(input_ss));
    EXPECT_EQ(count_active_rules(), 0);
}

TEST_F(TCAMBackupRestoreTest, BackupOriginalThenRestoreThenAdd) {
    // Add initial rule
    OptimizedTCAM::WildcardFields rule1_fields = create_default_fields(0x0A000001); // 10.0.0.1
    tcam.add_rule_with_ranges(rule1_fields, 100, 1); // ID 0
    ASSERT_EQ(count_active_rules(), 1);

    std::stringstream ss;
    tcam.backup_rules(ss); // Backup "10.0.0.1" rule

    // Restore to a new TCAM instance
    OptimizedTCAM tcam_restored;
    std::stringstream input_ss(ss.str());
    EXPECT_TRUE(tcam_restored.restore_rules(input_ss));

    auto restored_rules = tcam_restored.get_all_rule_stats();
    ASSERT_EQ(restored_rules.size(), 1);
    EXPECT_EQ(restored_rules[0].action, 1); // Restored rule has action 1
    // The new rule ID in tcam_restored will be 0.

    // Add a new rule to the restored TCAM
    OptimizedTCAM::WildcardFields rule2_fields = create_default_fields(0x0A000002); // 10.0.0.2
    // Using update_rules_atomic for adding to restored TCAM to be consistent with restore_rules
    OptimizedTCAM::RuleUpdateBatch batch;
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(rule2_fields, 90, 2));
    EXPECT_TRUE(tcam_restored.update_rules_atomic(batch));

    restored_rules = tcam_restored.get_all_rule_stats();
    ASSERT_EQ(restored_rules.size(), 2);

    // Check that rule IDs are distinct and assigned starting from 0 for the restored TCAM
    // First rule from restore (action 1) should have ID 0.
    // Second rule added after restore (action 2) should have ID 1.
    bool found_action1_id0 = false;
    bool found_action2_id1 = false;
    for(const auto& r_stat : restored_rules) {
        if (r_stat.action == 1 && r_stat.rule_id == 0) found_action1_id0 = true;
        if (r_stat.action == 2 && r_stat.rule_id == 1) found_action2_id1 = true;
    }
    EXPECT_TRUE(found_action1_id0);
    EXPECT_TRUE(found_action2_id1);

    EXPECT_EQ(tcam_restored.lookup_single(make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800)), 1);
    EXPECT_EQ(tcam_restored.lookup_single(make_packet(0x0A000002, 0xC0A80001, 1024, 80, 6, 0x0800)), 2);
}


// --- Tests for Rule Aging ---
class TCAMRuleAgingTest : public ::testing::Test {
protected:
    OptimizedTCAM tcam;

    OptimizedTCAM::WildcardFields create_default_fields(uint32_t src_ip_val = 0x0A000001) {
        OptimizedTCAM::WildcardFields fields{};
        fields.src_ip = src_ip_val; fields.src_ip_mask = 0xFFFFFFFF;
        fields.dst_ip = 0xC0A80001; fields.dst_ip_mask = 0xFFFFFFFF;
        fields.src_port_min = 1024; fields.src_port_max = 1024;
        fields.dst_port_min = 80;   fields.dst_port_max = 80;
        fields.protocol = 6;        fields.protocol_mask = 0xFF;
        fields.eth_type = 0x0800;   fields.eth_type_mask = 0xFFFF;
        return fields;
    }

    // Helper to get a rule's stats by its action, assuming actions are unique in tests
    std::optional<OptimizedTCAM::RuleStats> get_stats_by_action(int action) {
        auto all_stats = tcam.get_all_rule_stats();
        for (const auto&s : all_stats) {
            if (s.action == action) return s;
        }
        return std::nullopt;
    }
     uint64_t get_id_by_action(int action) {
        auto stats = get_stats_by_action(action);
        if (stats) return stats->rule_id;
        return std::numeric_limits<uint64_t>::max();
    }
};

TEST_F(TCAMRuleAgingTest, AgeByCreationTime_RuleOlder) {
    tcam.add_rule_with_ranges(create_default_fields(), 100, 1); // ID 0, Action 1
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Let rule age a bit

    auto aged_ids = tcam.age_rules(std::chrono::milliseconds(10), OptimizedTCAM::AgeCriteria::CREATION_TIME);
    ASSERT_EQ(aged_ids.size(), 1);
    EXPECT_EQ(aged_ids[0], get_id_by_action(1));

    auto stats = get_stats_by_action(1);
    ASSERT_TRUE(stats.has_value());
    EXPECT_FALSE(stats->is_active);
}

TEST_F(TCAMRuleAgingTest, AgeByCreationTime_RuleYounger) {
    tcam.add_rule_with_ranges(create_default_fields(), 100, 1); // ID 0, Action 1
    // Rule created just now

    auto aged_ids = tcam.age_rules(std::chrono::milliseconds(100), OptimizedTCAM::AgeCriteria::CREATION_TIME);
    EXPECT_TRUE(aged_ids.empty());

    auto stats = get_stats_by_action(1);
    ASSERT_TRUE(stats.has_value());
    EXPECT_TRUE(stats->is_active);
}

TEST_F(TCAMRuleAgingTest, AgeByLastHitTime_HitRecently) {
    tcam.add_rule_with_ranges(create_default_fields(), 100, 1); // ID 0, Action 1
    auto packet = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    tcam.lookup_single(packet); // Hit the rule
    std::this_thread::sleep_for(std::chrono::milliseconds(5)); // Small delay after hit

    // Max age is longer than time since last hit
    auto aged_ids = tcam.age_rules(std::chrono::milliseconds(50), OptimizedTCAM::AgeCriteria::LAST_HIT_TIME);
    EXPECT_TRUE(aged_ids.empty());

    auto stats = get_stats_by_action(1);
    ASSERT_TRUE(stats.has_value());
    EXPECT_TRUE(stats->is_active);
}

TEST_F(TCAMRuleAgingTest, AgeByLastHitTime_HitLongAgo) {
    tcam.add_rule_with_ranges(create_default_fields(), 100, 1); // ID 0, Action 1
    auto packet = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    tcam.lookup_single(packet); // Hit the rule
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Let time pass after hit

    // Max age is shorter than time since last hit
    auto aged_ids = tcam.age_rules(std::chrono::milliseconds(10), OptimizedTCAM::AgeCriteria::LAST_HIT_TIME);
    ASSERT_EQ(aged_ids.size(), 1);
    EXPECT_EQ(aged_ids[0], get_id_by_action(1));

    auto stats = get_stats_by_action(1);
    ASSERT_TRUE(stats.has_value());
    EXPECT_FALSE(stats->is_active);
}

TEST_F(TCAMRuleAgingTest, AgeByLastHitTime_NeverHit) {
    tcam.add_rule_with_ranges(create_default_fields(), 100, 1); // ID 0, Action 1
    // Rule is never hit, last_hit_timestamp is epoch.
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Let time pass

    auto aged_ids = tcam.age_rules(std::chrono::milliseconds(10), OptimizedTCAM::AgeCriteria::LAST_HIT_TIME);
    EXPECT_TRUE(aged_ids.empty()); // Should not be aged by LAST_HIT_TIME if never hit

    auto stats = get_stats_by_action(1);
    ASSERT_TRUE(stats.has_value());
    EXPECT_TRUE(stats->is_active);
}

TEST_F(TCAMRuleAgingTest, AgeMultipleRules) {
    tcam.add_rule_with_ranges(create_default_fields(0x0A000001), 100, 1); // R1 (ID 0)
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    tcam.add_rule_with_ranges(create_default_fields(0x0A000002), 100, 2); // R2 (ID 1)

    // R1 is older than 20ms, R2 is younger.
    auto aged_ids = tcam.age_rules(std::chrono::milliseconds(20), OptimizedTCAM::AgeCriteria::CREATION_TIME);
    ASSERT_EQ(aged_ids.size(), 1);
    EXPECT_EQ(aged_ids[0], get_id_by_action(1)); // R1 (action 1)

    auto stats1 = get_stats_by_action(1);
    ASSERT_TRUE(stats1.has_value());
    EXPECT_FALSE(stats1->is_active);

    auto stats2 = get_stats_by_action(2);
    ASSERT_TRUE(stats2.has_value());
    EXPECT_TRUE(stats2->is_active);
}

TEST_F(TCAMRuleAgingTest, AgeInactiveRule) {
    tcam.add_rule_with_ranges(create_default_fields(), 100, 1); // ID 0, Action 1
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // First, age it out
    tcam.age_rules(std::chrono::milliseconds(10), OptimizedTCAM::AgeCriteria::CREATION_TIME);
    auto stats1 = get_stats_by_action(1);
    ASSERT_TRUE(stats1.has_value());
    ASSERT_FALSE(stats1->is_active);
    uint64_t r1_hits = stats1->hit_count;


    // Try to age it out again
    auto aged_ids_again = tcam.age_rules(std::chrono::milliseconds(10), OptimizedTCAM::AgeCriteria::CREATION_TIME);
    EXPECT_TRUE(aged_ids_again.empty()); // Should not be in the returned list again

    stats1 = get_stats_by_action(1); // Re-fetch stats
    ASSERT_TRUE(stats1.has_value());
    EXPECT_FALSE(stats1->is_active); // Still inactive
    EXPECT_EQ(stats1->hit_count, r1_hits); // Hit count should not change
}

// --- Tests for Atomic Rule Updates ---
class TCAMAtomicUpdateTest : public ::testing::Test {
protected:
    OptimizedTCAM tcam;

    OptimizedTCAM::WildcardFields create_default_fields(uint32_t src_ip_val = 0x0A000001,
                                                        uint32_t dst_ip_val = 0xC0A80001,
                                                        uint16_t src_port_val = 1024,
                                                        uint16_t dst_port_val = 80) {
        OptimizedTCAM::WildcardFields fields{};
        fields.src_ip = src_ip_val; fields.src_ip_mask = 0xFFFFFFFF;
        fields.dst_ip = dst_ip_val; fields.dst_ip_mask = 0xFFFFFFFF;
        fields.src_port_min = src_port_val; fields.src_port_max = src_port_val;
        fields.dst_port_min = dst_port_val; fields.dst_port_max = dst_port_val;
        fields.protocol = 6;        fields.protocol_mask = 0xFF;    // TCP
        fields.eth_type = 0x0800;   fields.eth_type_mask = 0xFFFF;  // IPv4
        return fields;
    }

    size_t count_active_rules() {
        size_t active_count = 0;
        auto all_stats = tcam.get_all_rule_stats();
        for (const auto& stat : all_stats) {
            if (stat.is_active) {
                active_count++;
            }
        }
        return active_count;
    }
     uint64_t get_rule_id_by_action(int action_val) {
        auto all_stats = tcam.get_all_rule_stats();
        for (const auto& stat : all_stats) {
            if (stat.action == action_val) {
                return stat.rule_id;
            }
        }
        return std::numeric_limits<uint64_t>::max(); // Indicates not found
    }
};

TEST_F(TCAMAtomicUpdateTest, BatchOnlyAdd) {
    OptimizedTCAM::RuleUpdateBatch batch;
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(create_default_fields(0x0A000001), 100, 1));
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(create_default_fields(0x0A000002), 100, 2));

    EXPECT_TRUE(tcam.update_rules_atomic(batch));
    EXPECT_EQ(count_active_rules(), 2);
    EXPECT_EQ(tcam.lookup_single(make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800)), 1);
    EXPECT_EQ(tcam.lookup_single(make_packet(0x0A000002, 0xC0A80001, 1024, 80, 6, 0x0800)), 2);
}

TEST_F(TCAMAtomicUpdateTest, BatchOnlyDelete) {
    tcam.add_rule_with_ranges(create_default_fields(0x0A000001), 100, 1); // ID 0
    tcam.add_rule_with_ranges(create_default_fields(0x0A000002), 100, 2); // ID 1
    tcam.add_rule_with_ranges(create_default_fields(0x0A000003), 100, 3); // ID 2
    ASSERT_EQ(count_active_rules(), 3);

    OptimizedTCAM::RuleUpdateBatch batch;
    batch.push_back(OptimizedTCAM::RuleOperation::DeleteRule(0)); // Delete rule with ID 0
    batch.push_back(OptimizedTCAM::RuleOperation::DeleteRule(2)); // Delete rule with ID 2

    EXPECT_TRUE(tcam.update_rules_atomic(batch));
    EXPECT_EQ(count_active_rules(), 1);
    EXPECT_EQ(tcam.lookup_single(make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800)), -1);
    EXPECT_EQ(tcam.lookup_single(make_packet(0x0A000002, 0xC0A80001, 1024, 80, 6, 0x0800)), 2); // Rule ID 1, Action 2
    EXPECT_EQ(tcam.lookup_single(make_packet(0x0A000003, 0xC0A80001, 1024, 80, 6, 0x0800)), -1);
}

TEST_F(TCAMAtomicUpdateTest, BatchMixedAddDelete) {
    tcam.add_rule_with_ranges(create_default_fields(0x0A000001), 100, 1); // ID 0
    tcam.add_rule_with_ranges(create_default_fields(0x0A000002), 90, 2);  // ID 1
    ASSERT_EQ(count_active_rules(), 2);

    OptimizedTCAM::RuleUpdateBatch batch;
    batch.push_back(OptimizedTCAM::RuleOperation::DeleteRule(1)); // Delete rule ID 1 (Action 2)
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(create_default_fields(0x0A000003), 80, 3)); // New rule, Action 3

    EXPECT_TRUE(tcam.update_rules_atomic(batch));
    EXPECT_EQ(count_active_rules(), 2);
    EXPECT_EQ(tcam.lookup_single(make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800)), 1); // Rule ID 0 still exists
    EXPECT_EQ(tcam.lookup_single(make_packet(0x0A000002, 0xC0A80001, 1024, 80, 6, 0x0800)), -1); // Rule ID 1 deleted
    EXPECT_EQ(tcam.lookup_single(make_packet(0x0A000003, 0xC0A80001, 1024, 80, 6, 0x0800)), 3); // New rule ID 2, Action 3
}

TEST_F(TCAMAtomicUpdateTest, AtomicityDeleteNonExistent) {
    tcam.add_rule_with_ranges(create_default_fields(0x0A000001), 100, 1); // ID 0
    ASSERT_EQ(count_active_rules(), 1);

    OptimizedTCAM::RuleUpdateBatch batch;
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(create_default_fields(0x0A000002), 90, 2)); // Try to add
    batch.push_back(OptimizedTCAM::RuleOperation::DeleteRule(5)); // Non-existent ID

    EXPECT_FALSE(tcam.update_rules_atomic(batch)); // Should fail
    EXPECT_EQ(count_active_rules(), 1); // State should be rolled back
    EXPECT_EQ(tcam.lookup_single(make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800)), 1);
    EXPECT_EQ(tcam.lookup_single(make_packet(0x0A000002, 0xC0A80001, 1024, 80, 6, 0x0800)), -1); // Rule for 10.0.0.2 should not exist
}

TEST_F(TCAMAtomicUpdateTest, AtomicityAddThenDeleteInBatch) {
    OptimizedTCAM::RuleUpdateBatch batch;
    // Rule added will get ID 0 internally within the batch processing logic
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(create_default_fields(0x0A0000AA), 100, 10));
    batch.push_back(OptimizedTCAM::RuleOperation::DeleteRule(0)); // Deleting the rule just added in this batch

    EXPECT_TRUE(tcam.update_rules_atomic(batch));
    EXPECT_EQ(count_active_rules(), 0); // Rule added and deleted
    EXPECT_EQ(tcam.lookup_single(make_packet(0x0A0000AA, 0xC0A80001, 1024, 80, 6, 0x0800)), -1);
}

TEST_F(TCAMAtomicUpdateTest, AtomicityDeleteThenAddInBatch) {
    tcam.add_rule_with_ranges(create_default_fields(0x0A0000BB), 100, 11); // ID 0
    ASSERT_EQ(count_active_rules(), 1);
    uint64_t id_of_bb = get_rule_id_by_action(11); // This will be 0

    OptimizedTCAM::RuleUpdateBatch batch;
    batch.push_back(OptimizedTCAM::RuleOperation::DeleteRule(id_of_bb));
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(create_default_fields(0x0A0000BB), 90, 12)); // Re-add with different prio/action

    EXPECT_TRUE(tcam.update_rules_atomic(batch));
    EXPECT_EQ(count_active_rules(), 1);
    EXPECT_EQ(tcam.lookup_single(make_packet(0x0A0000BB, 0xC0A80001, 1024, 80, 6, 0x0800)), 12);
}


TEST_F(TCAMAtomicUpdateTest, BatchRespectsPrioritySpecificity) {
    OptimizedTCAM::RuleUpdateBatch batch;
    // Rule1: More specific, higher priority
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(create_default_fields(0x0A000001, 0xC0A80001, 1000, 80), 100, 1));
    // Rule2: Less specific (source IP mask), lower priority
    OptimizedTCAM::WildcardFields fields2 = create_default_fields(0x0A000000, 0xC0A80001, 1000, 80);
    fields2.src_ip_mask = 0xFFFFFF00; // 10.0.0.0/24
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(fields2, 90, 2));

    EXPECT_TRUE(tcam.update_rules_atomic(batch));
    EXPECT_EQ(count_active_rules(), 2);
    // Packet matches both, should hit Rule1 (Action 1)
    EXPECT_EQ(tcam.lookup_single(make_packet(0x0A000001, 0xC0A80001, 1000, 80, 6, 0x0800)), 1);
}

TEST_F(TCAMAtomicUpdateTest, BatchPortRangeHandling) {
    OptimizedTCAM::RuleUpdateBatch batch;
    OptimizedTCAM::WildcardFields fields1 = create_default_fields();
    fields1.src_port_min = 2000; fields1.src_port_max = 2005; // New range
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(fields1, 100, 1));

    OptimizedTCAM::WildcardFields fields2 = create_default_fields();
    fields2.src_port_min = 2000; fields2.src_port_max = 2005; // Reuse same range
    fields2.dst_ip = 0xC0A80002; // Different dst_ip to make it a distinct rule
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(fields2, 90, 2));

    EXPECT_TRUE(tcam.update_rules_atomic(batch));
    EXPECT_EQ(count_active_rules(), 2);
    // Verify port_ranges size (internal detail, but lambda logic implies it should be 1 for the new range)
    // This is hard to check directly without exposing port_ranges or its size.
    // We trust the lambda reuses existing identical ranges.
    EXPECT_EQ(tcam.lookup_single(make_packet(0x0A000001, 0xC0A80001, 2003, 80, 6, 0x0800)), 1);
    EXPECT_EQ(tcam.lookup_single(make_packet(0x0A000001, 0xC0A80002, 2003, 80, 6, 0x0800)), 2);
}

TEST_F(TCAMAtomicUpdateTest, EmptyBatch) {
    tcam.add_rule_with_ranges(create_default_fields(), 100, 1);
    ASSERT_EQ(count_active_rules(), 1);

    OptimizedTCAM::RuleUpdateBatch batch; // Empty
    EXPECT_TRUE(tcam.update_rules_atomic(batch));
    EXPECT_EQ(count_active_rules(), 1); // No change
}

TEST_F(TCAMAtomicUpdateTest, DeleteAllRules) {
    tcam.add_rule_with_ranges(create_default_fields(0x0A000001), 100, 1); // ID 0
    tcam.add_rule_with_ranges(create_default_fields(0x0A000002), 90, 2);  // ID 1
    ASSERT_EQ(count_active_rules(), 2);

    OptimizedTCAM::RuleUpdateBatch batch;
    batch.push_back(OptimizedTCAM::RuleOperation::DeleteRule(0));
    batch.push_back(OptimizedTCAM::RuleOperation::DeleteRule(1));
    EXPECT_TRUE(tcam.update_rules_atomic(batch));
    EXPECT_EQ(count_active_rules(), 0);
}

TEST_F(TCAMAtomicUpdateTest, AddToEmptyTcam) {
    ASSERT_EQ(count_active_rules(), 0);
    OptimizedTCAM::RuleUpdateBatch batch;
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(create_default_fields(), 100, 1));
    EXPECT_TRUE(tcam.update_rules_atomic(batch));
    EXPECT_EQ(count_active_rules(), 1);
    EXPECT_EQ(tcam.lookup_single(make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800)), 1);
}

// --- Tests for Redundant Rule Detection and Compaction ---
class TCAMRedundantRuleTest : public ::testing::Test {
protected:
    OptimizedTCAM tcam;

    OptimizedTCAM::WildcardFields create_default_fields_for_redundancy_tests() {
        OptimizedTCAM::WildcardFields fields{};
        fields.src_ip = 0x0A000001; fields.src_ip_mask = 0xFFFFFFFF; // 10.0.0.1/32
        fields.dst_ip = 0xC0A80001; fields.dst_ip_mask = 0xFFFFFFFF; // 192.168.0.1/32
        fields.src_port_min = 1024; fields.src_port_max = 1024;
        fields.dst_port_min = 80;   fields.dst_port_max = 80;
        fields.protocol = 6;        fields.protocol_mask = 0xFF;    // TCP
        fields.eth_type = 0x0800;   fields.eth_type_mask = 0xFFFF;  // IPv4
        return fields;
    }

    size_t count_active_rules() {
        size_t active_count = 0;
        auto all_stats = tcam.get_all_rule_stats();
        for (const auto& stat : all_stats) {
            if (stat.is_active) {
                active_count++;
            }
        }
        return active_count;
    }

     size_t get_total_rules_from_utilization() {
        return tcam.get_rule_utilization().total_rules;
    }
};

TEST_F(TCAMRedundantRuleTest, DetectNoRules) {
    auto redundant_indices = tcam.detect_redundant_rules();
    EXPECT_TRUE(redundant_indices.empty());
    tcam.compact_redundant_rules(false); // Should do nothing
    tcam.compact_redundant_rules(true);  // Should do nothing
    EXPECT_EQ(count_active_rules(), 0);
}

TEST_F(TCAMRedundantRuleTest, DetectNoRedundantRules) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields_for_redundancy_tests();
    tcam.add_rule_with_ranges(fields1, 100, 1); // ID 0

    OptimizedTCAM::WildcardFields fields2 = create_default_fields_for_redundancy_tests();
    fields2.src_ip = 0x0A000002; // Different IP
    tcam.add_rule_with_ranges(fields2, 90, 2);  // ID 1

    auto redundant_indices = tcam.detect_redundant_rules();
    EXPECT_TRUE(redundant_indices.empty());
    tcam.compact_redundant_rules(false);
    EXPECT_EQ(count_active_rules(), 2);
}

TEST_F(TCAMRedundantRuleTest, DetectSimpleRedundancySameAction) {
    // Rule B (Superset Rule): Prio 100, Action 1
    OptimizedTCAM::WildcardFields fields_B = create_default_fields_for_redundancy_tests();
    fields_B.src_ip_mask = 0xFFFFFF00; // 10.0.0.0/24
    tcam.add_rule_with_ranges(fields_B, 100, 1); // Rule 0 (ID 0)

    // Rule A (Redundant Rule): Prio 90, Action 1 (Same action), subset of B
    OptimizedTCAM::WildcardFields fields_A = create_default_fields_for_redundancy_tests();
    fields_A.src_ip = 0x0A000001; fields_A.src_ip_mask = 0xFFFFFFFF; // 10.0.0.1/32
    // Ensure other fields match B for subset condition
    fields_A.dst_ip = fields_B.dst_ip; fields_A.dst_ip_mask = fields_B.dst_ip_mask; // etc.
    tcam.add_rule_with_ranges(fields_A, 90, 1);  // Rule 1 (ID 1)

    // Expected order: Rule B (idx 0, prio 100), Rule A (idx 1, prio 90)
    // Rule A (idx 1) is subset of Rule B (idx 0) and has same action.
    auto redundant_indices = tcam.detect_redundant_rules();
    ASSERT_EQ(redundant_indices.size(), 1);
    EXPECT_EQ(redundant_indices[0], 1); // Rule A at index 1 should be redundant
}

TEST_F(TCAMRedundantRuleTest, NotRedundantIfActionsDiffer) {
    OptimizedTCAM::WildcardFields fields_B = create_default_fields_for_redundancy_tests();
    fields_B.src_ip_mask = 0xFFFFFF00;
    tcam.add_rule_with_ranges(fields_B, 100, 1); // Action 1

    OptimizedTCAM::WildcardFields fields_A = create_default_fields_for_redundancy_tests();
    fields_A.src_ip = 0x0A000001;
    tcam.add_rule_with_ranges(fields_A, 90, 2);  // Action 2 (Different)

    auto redundant_indices = tcam.detect_redundant_rules();
    EXPECT_TRUE(redundant_indices.empty()); // This is SHADOWING, not redundancy
}

TEST_F(TCAMRedundantRuleTest, CompactNoRebuild) {
    OptimizedTCAM::WildcardFields fields_B = create_default_fields_for_redundancy_tests();
    fields_B.src_ip_mask = 0xFFFFFF00;
    tcam.add_rule_with_ranges(fields_B, 100, 1); // ID 0

    OptimizedTCAM::WildcardFields fields_A = create_default_fields_for_redundancy_tests();
    fields_A.src_ip = 0x0A000001;
    tcam.add_rule_with_ranges(fields_A, 90, 1);  // ID 1 (Redundant)

    ASSERT_EQ(count_active_rules(), 2);
    ASSERT_EQ(get_total_rules_from_utilization(), 2);


    tcam.compact_redundant_rules(false /* trigger_rebuild = false */);
    EXPECT_EQ(count_active_rules(), 1); // Rule A (ID 1) marked inactive
    EXPECT_EQ(get_total_rules_from_utilization(), 2); // Total rules in vector still 2

    auto rule1_stats = tcam.get_rule_stats(1); // ID 1
    ASSERT_TRUE(rule1_stats.has_value());
    EXPECT_FALSE(rule1_stats.value().is_active);
}

TEST_F(TCAMRedundantRuleTest, CompactWithRebuild) {
    OptimizedTCAM::WildcardFields fields_B = create_default_fields_for_redundancy_tests(); // Superset
    fields_B.src_ip_mask = 0xFFFFFF00; // 10.0.0.0/24
    tcam.add_rule_with_ranges(fields_B, 100, 1); // ID 0

    OptimizedTCAM::WildcardFields fields_A = create_default_fields_for_redundancy_tests(); // Subset, redundant
    fields_A.src_ip = 0x0A000001; // 10.0.0.1/32
    tcam.add_rule_with_ranges(fields_A, 90, 1);  // ID 1

    auto packet_matches_both = make_packet(0x0A000001, fields_A.dst_ip, fields_A.src_port_min, fields_A.dst_port_min, fields_A.protocol, fields_A.eth_type);
    EXPECT_EQ(tcam.lookup_single(packet_matches_both), 1); // Hits Rule B (Action 1)

    ASSERT_EQ(count_active_rules(), 2);
    ASSERT_EQ(get_total_rules_from_utilization(), 2);

    tcam.compact_redundant_rules(true /* trigger_rebuild = true */);

    EXPECT_EQ(count_active_rules(), 1); // Rule A (ID 1) removed
    EXPECT_EQ(get_total_rules_from_utilization(), 1); // Total rules in vector now 1

    auto rule0_stats = tcam.get_rule_stats(0); // ID 0 (Rule B)
    ASSERT_TRUE(rule0_stats.has_value());
    EXPECT_TRUE(rule0_stats.value().is_active);

    auto rule1_stats = tcam.get_rule_stats(1); // ID 1 (Rule A)
    EXPECT_FALSE(rule1_stats.has_value()); // Rule physically removed

    // Lookup should still hit Rule B (Action 1)
    EXPECT_EQ(tcam.lookup_single(packet_matches_both), 1);
}

TEST_F(TCAMRedundantRuleTest, CompactNoRedundantRules) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields_for_redundancy_tests();
    tcam.add_rule_with_ranges(fields1, 100, 1);
    OptimizedTCAM::WildcardFields fields2 = create_default_fields_for_redundancy_tests();
    fields2.src_ip = 0x0A000002; // Different
    tcam.add_rule_with_ranges(fields2, 90, 2);

    ASSERT_EQ(count_active_rules(), 2);
    tcam.compact_redundant_rules(false);
    EXPECT_EQ(count_active_rules(), 2);
    tcam.compact_redundant_rules(true);
    EXPECT_EQ(count_active_rules(), 2);
    EXPECT_EQ(get_total_rules_from_utilization(), 2);
}

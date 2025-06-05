#include "gtest/gtest.h"
#include "tcam.h" // Assumes tcam.h is in the include path

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
    EXPECT_EQ(tcam.lookup_linear(packet), -1);
    EXPECT_EQ(tcam.lookup_bitmap(packet), -1);
    EXPECT_EQ(tcam.lookup_decision_tree(packet), -1);
}

TEST_F(TCAMTest, AddAndLookupLinearExactMatch) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields();
    tcam.add_rule_with_ranges(fields1, 100, 1);

    auto matching_packet = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    auto non_matching_packet_ip = make_packet(0x0A000002, 0xC0A80001, 1024, 80, 6, 0x0800);
    auto non_matching_packet_port = make_packet(0x0A000001, 0xC0A80001, 1025, 80, 6, 0x0800);
    auto non_matching_packet_proto = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 17, 0x0800);

    EXPECT_EQ(tcam.lookup_linear(matching_packet), 1);
    EXPECT_EQ(tcam.lookup_linear(non_matching_packet_ip), -1);
    EXPECT_EQ(tcam.lookup_linear(non_matching_packet_port), -1);
    EXPECT_EQ(tcam.lookup_linear(non_matching_packet_proto), -1);
}

TEST_F(TCAMTest, PriorityTest) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields();
    OptimizedTCAM::WildcardFields fields2 = create_default_fields();
    fields2.src_ip_mask = 0xFFFF0000;

    tcam.add_rule_with_ranges(fields1, 100, 1);
    tcam.add_rule_with_ranges(fields2, 90, 2);

    auto matching_packet = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    EXPECT_EQ(tcam.lookup_linear(matching_packet), 1);
}

TEST_F(TCAMTest, IPWildcardTest) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields();
    fields1.src_ip = 0x0A0A0000;
    fields1.src_ip_mask = 0xFFFF0000;
    tcam.add_rule_with_ranges(fields1, 100, 5);

    auto packet_in_subnet = make_packet(0x0A0A0A0A, 0xC0A80001, 1024, 80, 6, 0x0800);
    auto packet_outside_subnet = make_packet(0x0A0B0A0A, 0xC0A80001, 1024, 80, 6, 0x0800);

    EXPECT_EQ(tcam.lookup_linear(packet_in_subnet), 5);
    EXPECT_EQ(tcam.lookup_linear(packet_outside_subnet), -1);
}

TEST_F(TCAMTest, ProtocolWildcardTest) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields();
    fields1.protocol_mask = 0x00;
    tcam.add_rule_with_ranges(fields1, 100, 8);

    auto packet_tcp = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    auto packet_udp = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 17, 0x0800);

    EXPECT_EQ(tcam.lookup_linear(packet_tcp), 8);
    EXPECT_EQ(tcam.lookup_linear(packet_udp), 8);
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

    EXPECT_EQ(tcam.lookup_linear(packet_in_range_low), 10);
    EXPECT_EQ(tcam.lookup_linear(packet_in_range_mid), 10);
    EXPECT_EQ(tcam.lookup_linear(packet_in_range_high), 10);
    EXPECT_EQ(tcam.lookup_linear(packet_below_range), -1);
    EXPECT_EQ(tcam.lookup_linear(packet_above_range), -1);
}

TEST_F(TCAMTest, DestinationPortAnyTest) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields();
    fields1.dst_port_min = 0;
    fields1.dst_port_max = 0xFFFF;
    tcam.add_rule_with_ranges(fields1, 100, 12);

    auto packet1 = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    auto packet2 = make_packet(0x0A000001, 0xC0A80001, 1024, 443, 6, 0x0800);
    auto packet3 = make_packet(0x0A000001, 0xC0A80001, 1024, 30000, 6, 0x0800);

    EXPECT_EQ(tcam.lookup_linear(packet1), 12);
    EXPECT_EQ(tcam.lookup_linear(packet2), 12);
    EXPECT_EQ(tcam.lookup_linear(packet3), 12);
}

TEST_F(TCAMTest, AllLookupsComparison) {
    OptimizedTCAM::WildcardFields fields1 = create_default_fields();
    fields1.src_ip = 0x0B000000; fields1.src_ip_mask = 0xFF000000;
    fields1.dst_port_min = 1000; fields1.dst_port_max = 2000;
    tcam.add_rule_with_ranges(fields1, 100, 15);

    OptimizedTCAM::WildcardFields fields2 = create_default_fields();
    fields2.protocol = 17;
    tcam.add_rule_with_ranges(fields2, 90, 16);

    auto packet1 = make_packet(0x0B010203, 0xC0A80001, 1024, 1500, 6, 0x0800);
    auto packet2 = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 17, 0x0800);
    auto packet3 = make_packet(0x0C000001, 0xC0A80001, 100, 200, 1, 0x8100);

    EXPECT_EQ(tcam.lookup_linear(packet1), 15);
    EXPECT_EQ(tcam.lookup_bitmap(packet1), 15);
    EXPECT_EQ(tcam.lookup_decision_tree(packet1), 15);

    EXPECT_EQ(tcam.lookup_linear(packet2), 16);
    EXPECT_EQ(tcam.lookup_bitmap(packet2), 16);
    EXPECT_EQ(tcam.lookup_decision_tree(packet2), 16);

    EXPECT_EQ(tcam.lookup_linear(packet3), -1);
    EXPECT_EQ(tcam.lookup_bitmap(packet3), -1);
    EXPECT_EQ(tcam.lookup_decision_tree(packet3), -1);
}

TEST_F(TCAMTest, BatchLookupTest) {
    // Rule 1, matches packet 0
    OptimizedTCAM::WildcardFields f1{}; // Use default initializer
    f1.src_ip = 0x11111111; f1.src_ip_mask = 0xFFFFFFFF;
    f1.dst_ip = 0xAAAAAAAA; f1.dst_ip_mask = 0xFFFFFFFF;
    f1.src_port_min = 1024; f1.src_port_max = 1024;
    f1.dst_port_min = 80; f1.dst_port_max = 80;
    f1.protocol = 6; f1.protocol_mask = 0xFF;
    f1.eth_type = 0x0800; f1.eth_type_mask = 0xFFFF;
    tcam.add_rule_with_ranges(f1, 100, 1);

    // Rule 2, matches packet 1
    OptimizedTCAM::WildcardFields f2{};
    f2.src_ip = 0xBBBBBBBB; f2.src_ip_mask = 0xFFFFFFFF;
    f2.dst_ip = 0x22222222; f2.dst_ip_mask = 0xFFFFFFFF;
    f2.src_port_min = 1024; f2.src_port_max = 1024;
    f2.dst_port_min = 80; f2.dst_port_max = 80;
    f2.protocol = 6; f2.protocol_mask = 0xFF;
    f2.eth_type = 0x0800; f2.eth_type_mask = 0xFFFF;
    tcam.add_rule_with_ranges(f2, 90, 2); // Lower priority to avoid ambiguity if packets were less specific

    // Rule 3, matches packet 2
    OptimizedTCAM::WildcardFields f3{};
    f3.src_ip = 0xCCCCCCCC; f3.src_ip_mask = 0xFFFFFFFF;
    f3.dst_ip = 0xDDDDDDDD; f3.dst_ip_mask = 0xFFFFFFFF;
    f3.src_port_min = 5000; f3.src_port_max = 5000;
    f3.dst_port_min = 80; f3.dst_port_max = 80;
    f3.protocol = 6; f3.protocol_mask = 0xFF;
    f3.eth_type = 0x0800; f3.eth_type_mask = 0xFFFF;
    tcam.add_rule_with_ranges(f3, 80, 3); // Lower priority

    std::vector<std::vector<uint8_t>> packets_to_test;
    packets_to_test.push_back(make_packet(0x11111111, 0xAAAAAAAA, 1024, 80, 6, 0x0800));
    packets_to_test.push_back(make_packet(0xBBBBBBBB, 0x22222222, 1024, 80, 6, 0x0800));
    packets_to_test.push_back(make_packet(0xCCCCCCCC, 0xDDDDDDDD, 5000, 80, 6, 0x0800));
    packets_to_test.push_back(make_packet(0x0E0E0E0E, 0x0F0F0F0F, 1234, 5678, 17,0x0800));

    std::vector<int> results;
    tcam.lookup_batch(packets_to_test, results);

    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0], 1) << "Packet 0 (src 11..., dst AA...) did not match rule 1";
    EXPECT_EQ(results[1], 2) << "Packet 1 (src BB..., dst 22...) did not match rule 2";
    EXPECT_EQ(results[2], 3) << "Packet 2 (src CC..., dst DD..., sport 5000) did not match rule 3";
    EXPECT_EQ(results[3], -1) << "Packet 3 (no match) unexpectedly matched something";
}

TEST_F(TCAMTest, EthTypeMatching) {
    OptimizedTCAM tcam_eth; // Use a fresh TCAM for this specific test
    OptimizedTCAM::WildcardFields fields_ipv4 = create_default_fields();
    fields_ipv4.eth_type = 0x0800; fields_ipv4.eth_type_mask = 0xFFFF;
    tcam_eth.add_rule_with_ranges(fields_ipv4, 100, 20);

    OptimizedTCAM::WildcardFields fields_arp = create_default_fields();
    fields_arp.eth_type = 0x0806; fields_arp.eth_type_mask = 0xFFFF;
    tcam_eth.add_rule_with_ranges(fields_arp, 90, 21);

    OptimizedTCAM::WildcardFields fields_any_eth = create_default_fields();
    fields_any_eth.eth_type = 0; // Value doesn't matter if mask is 0
    fields_any_eth.eth_type_mask = 0x0000;
    tcam_eth.add_rule_with_ranges(fields_any_eth, 80, 22);

    auto packet_ipv4 = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    auto packet_arp = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0806);
    auto packet_vlan = make_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x8100);

    EXPECT_EQ(tcam_eth.lookup_linear(packet_ipv4), 20);
    EXPECT_EQ(tcam_eth.lookup_bitmap(packet_ipv4), 20);
    EXPECT_EQ(tcam_eth.lookup_decision_tree(packet_ipv4), 20);

    EXPECT_EQ(tcam_eth.lookup_linear(packet_arp), 21);
    EXPECT_EQ(tcam_eth.lookup_bitmap(packet_arp), 21);
    EXPECT_EQ(tcam_eth.lookup_decision_tree(packet_arp), 21);

    EXPECT_EQ(tcam_eth.lookup_linear(packet_vlan), 22);
    EXPECT_EQ(tcam_eth.lookup_bitmap(packet_vlan), 22);
    EXPECT_EQ(tcam_eth.lookup_decision_tree(packet_vlan), 22);
}

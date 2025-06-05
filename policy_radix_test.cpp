#include "gtest/gtest.h"
#include "policy_radix.h" // Assuming policy_radix.h is in the root directory
#include <vector>
#include <string>
#include <algorithm> // For std::sort
#include <optional>

// Helper to convert IP string to uint32_t for tests, expecting no errors
uint32_t MustIpStringToInt(const std::string& ip) {
    try {
        return PolicyRoutingTree::ipStringToInt(ip);
    } catch (const std::runtime_error& e) {
        ADD_FAILURE() << "MustIpStringToInt failed for " << ip << ": " << e.what();
        return 0; // Return dummy value
    }
}

TEST(PolicyRadixUtilTest, IpStringConversion) {
    EXPECT_EQ(MustIpStringToInt("0.0.0.0"), 0x00000000);
    EXPECT_EQ(MustIpStringToInt("192.168.1.1"), 0xC0A80101);
    EXPECT_EQ(MustIpStringToInt("255.255.255.255"), 0xFFFFFFFF);
    EXPECT_EQ(MustIpStringToInt("10.0.0.5"), 0x0A000005);

    EXPECT_EQ(PolicyRoutingTree::ipIntToString(0x00000000), "0.0.0.0");
    EXPECT_EQ(PolicyRoutingTree::ipIntToString(0xC0A80101), "192.168.1.1");
    EXPECT_EQ(PolicyRoutingTree::ipIntToString(0xFFFFFFFF), "255.255.255.255");
    EXPECT_EQ(PolicyRoutingTree::ipIntToString(0x0A000005), "10.0.0.5");

    EXPECT_THROW(PolicyRoutingTree::ipStringToInt("invalid.ip"), std::runtime_error);
    EXPECT_THROW(PolicyRoutingTree::ipStringToInt("192.168.1.256"), std::runtime_error);
}

TEST(PolicyRoutingTreeTest, BasicAddAndLookup) {
    PolicyRoutingTree tree;
    RouteAttributes attrs;
    attrs.nextHop = MustIpStringToInt("192.168.1.1");
    attrs.dscp = 0x10; // Example DSCP

    PolicyRule rule_default; // Default policy (match all)
    rule_default.priority = 100;

    tree.addRoute("10.0.0.0", 16, rule_default, attrs);

    PacketInfo packet_in_subnet;
    packet_in_subnet.dstIP = MustIpStringToInt("10.0.1.5");
    // Other PacketInfo fields remain default (0)

    auto best_route_attrs_ptr = tree.findBestRoute(packet_in_subnet);
    ASSERT_NE(best_route_attrs_ptr, nullptr);
    EXPECT_EQ(best_route_attrs_ptr->nextHop, MustIpStringToInt("192.168.1.1"));
    EXPECT_EQ(best_route_attrs_ptr->dscp, 0x10);

    PacketInfo packet_out_subnet;
    packet_out_subnet.dstIP = MustIpStringToInt("172.16.0.1");
    EXPECT_EQ(tree.findBestRoute(packet_out_subnet), nullptr);

    // Test default route
    RouteAttributes attrs_default_route;
    attrs_default_route.nextHop = MustIpStringToInt("8.8.8.8");
    tree.addRoute("0.0.0.0", 0, rule_default, attrs_default_route);

    best_route_attrs_ptr = tree.findBestRoute(packet_out_subnet); // Should now match default
    ASSERT_NE(best_route_attrs_ptr, nullptr);
    EXPECT_EQ(best_route_attrs_ptr->nextHop, MustIpStringToInt("8.8.8.8"));
}

TEST(PolicyRoutingTreeTest, PolicyMatching) {
    PolicyRoutingTree tree;
    RouteAttributes attrs_general;
    attrs_general.nextHop = MustIpStringToInt("1.1.1.1");
    PolicyRule rule_general;
    rule_general.priority = 200;
    tree.addRoute("20.0.0.0", 8, rule_general, attrs_general);

    RouteAttributes attrs_http;
    attrs_http.nextHop = MustIpStringToInt("2.2.2.2");
    attrs_http.dscp = 0x0A;
    PolicyRule rule_http;
    rule_http.dstPort = 80;
    rule_http.protocol = 6; // TCP
    rule_http.priority = 100; // Higher priority
    tree.addRoute("20.0.0.0", 8, rule_http, attrs_http);

    RouteAttributes attrs_dns_udp;
    attrs_dns_udp.nextHop = MustIpStringToInt("3.3.3.3");
    PolicyRule rule_dns_udp;
    rule_dns_udp.dstPort = 53;
    rule_dns_udp.protocol = 17; // UDP
    rule_dns_udp.priority = 50;
    tree.addRoute("20.0.0.0", 8, rule_dns_udp, attrs_dns_udp);


    PacketInfo packet_http;
    packet_http.dstIP = MustIpStringToInt("20.0.0.1");
    packet_http.dstPort = 80;
    packet_http.protocol = 6;
    auto best_route_http = tree.findBestRoute(packet_http);
    ASSERT_NE(best_route_http, nullptr);
    EXPECT_EQ(best_route_http->nextHop, MustIpStringToInt("2.2.2.2"));
    EXPECT_EQ(best_route_http->dscp, 0x0A);

    PacketInfo packet_dns;
    packet_dns.dstIP = MustIpStringToInt("20.0.0.2");
    packet_dns.dstPort = 53;
    packet_dns.protocol = 17;
    auto best_route_dns = tree.findBestRoute(packet_dns);
    ASSERT_NE(best_route_dns, nullptr);
    EXPECT_EQ(best_route_dns->nextHop, MustIpStringToInt("3.3.3.3"));

    PacketInfo packet_general;
    packet_general.dstIP = MustIpStringToInt("20.0.0.3");
    packet_general.dstPort = 1234; // Some other port
    packet_general.protocol = 6;
    auto best_route_general = tree.findBestRoute(packet_general);
    ASSERT_NE(best_route_general, nullptr);
    EXPECT_EQ(best_route_general->nextHop, MustIpStringToInt("1.1.1.1"));

    // Test policy with source prefix
    PolicyRule rule_src_prefix = rule_general; // copy general rule
    rule_src_prefix.srcPrefix = MustIpStringToInt("192.168.5.0");
    rule_src_prefix.srcPrefixLen = 24;
    rule_src_prefix.priority = 20; // Highest priority for this specific source
    RouteAttributes attrs_src_prefix_route;
    attrs_src_prefix_route.nextHop = MustIpStringToInt("4.4.4.4");
    tree.addRoute("20.0.0.0", 8, rule_src_prefix, attrs_src_prefix_route);

    PacketInfo packet_from_specific_src = packet_general; // dst 20.0.0.3
    packet_from_specific_src.srcIP = MustIpStringToInt("192.168.5.10");
    auto best_route_src_spec = tree.findBestRoute(packet_from_specific_src);
    ASSERT_NE(best_route_src_spec, nullptr);
    EXPECT_EQ(best_route_src_spec->nextHop, MustIpStringToInt("4.4.4.4"));

    PacketInfo packet_from_other_src = packet_general; // dst 20.0.0.3
    packet_from_other_src.srcIP = MustIpStringToInt("172.16.0.1"); // Different source
    auto best_route_other_src = tree.findBestRoute(packet_from_other_src);
    ASSERT_NE(best_route_other_src, nullptr);
    EXPECT_EQ(best_route_other_src->nextHop, MustIpStringToInt("1.1.1.1")); // Fallback to general
}

TEST(PolicyRoutingTreeTest, EcmpSelection) {
    PolicyRoutingTree tree;
    std::string dest_prefix = "30.0.0.0";
    uint8_t dest_prefix_len = 8;

    PolicyRule rule_ecmp;
    rule_ecmp.priority = 100;

    RouteAttributes attrs1, attrs2, attrs3;
    attrs1.nextHop = MustIpStringToInt("10.0.0.1"); attrs1.adminDistance = 1; attrs1.localPref = 100; attrs1.med = 0;
    attrs2.nextHop = MustIpStringToInt("10.0.0.2"); attrs2.adminDistance = 1; attrs2.localPref = 100; attrs2.med = 0;
    attrs3.nextHop = MustIpStringToInt("10.0.0.3"); attrs3.adminDistance = 1; attrs3.localPref = 100; attrs3.med = 0;

    tree.addRoute(dest_prefix, dest_prefix_len, rule_ecmp, attrs1);
    tree.addRoute(dest_prefix, dest_prefix_len, rule_ecmp, attrs2);
    tree.addRoute(dest_prefix, dest_prefix_len, rule_ecmp, attrs3);

    PacketInfo packet_base;
    packet_base.dstIP = MustIpStringToInt("30.0.0.100");

    auto ecmp_paths = tree.getEqualCostPaths(packet_base);
    ASSERT_EQ(ecmp_paths.size(), 3);
    std::vector<uint32_t> next_hops;
    for(const auto& attr : ecmp_paths) {
        next_hops.push_back(attr.nextHop);
    }
    std::sort(next_hops.begin(), next_hops.end());
    EXPECT_EQ(next_hops[0], MustIpStringToInt("10.0.0.1"));
    EXPECT_EQ(next_hops[1], MustIpStringToInt("10.0.0.2"));
    EXPECT_EQ(next_hops[2], MustIpStringToInt("10.0.0.3"));

    // Test flow hash selection - we can't predict the exact one, but it should be one of the three
    PacketInfo p1 = packet_base; p1.srcIP = MustIpStringToInt("1.1.1.1"); p1.srcPort = 1000;
    PacketInfo p2 = packet_base; p2.srcIP = MustIpStringToInt("2.2.2.2"); p2.srcPort = 2000;

    auto selected1_opt = tree.selectEcmpPathUsingFlowHash(p1);
    ASSERT_TRUE(selected1_opt.has_value());
    bool found1 = false;
    for(uint32_t nh : next_hops) if(nh == selected1_opt->nextHop) found1 = true;
    EXPECT_TRUE(found1);

    auto selected2_opt = tree.selectEcmpPathUsingFlowHash(p2);
    ASSERT_TRUE(selected2_opt.has_value());
    bool found2 = false;
    for(uint32_t nh : next_hops) if(nh == selected2_opt->nextHop) found2 = true;
    EXPECT_TRUE(found2);

    // It's possible selected1 and selected2 are the same. A more robust test would check distribution
    // over many packets, but that's too complex for a unit test here.
    // Just check that different flow hashes *can* produce different results (though not guaranteed for 2 packets)
    // For now, just ensuring a valid path is selected is sufficient.

    // Add a higher priority specific route
    RouteAttributes attrs_specific;
    attrs_specific.nextHop = MustIpStringToInt("10.0.0.254");
    PolicyRule rule_specific_prio = rule_ecmp;
    rule_specific_prio.priority = 50; // Higher priority
    rule_specific_prio.dstPort = 443; // HTTPS
    tree.addRoute(dest_prefix, dest_prefix_len, rule_specific_prio, attrs_specific);

    PacketInfo https_packet = packet_base;
    https_packet.dstPort = 443;
    auto selected_specific_opt = tree.selectEcmpPathUsingFlowHash(https_packet);
    ASSERT_TRUE(selected_specific_opt.has_value());
    EXPECT_EQ(selected_specific_opt->nextHop, MustIpStringToInt("10.0.0.254")); // Should pick specific, not ECMP
}

TEST(PolicyRoutingTreeTest, RouteSortingCriteria) {
    PolicyRoutingTree tree;
    std::string dest_prefix = "40.0.0.0";
    uint8_t dest_prefix_len = 8;
    PacketInfo packet;
    packet.dstIP = MustIpStringToInt("40.0.0.1");

    PolicyRule rule_base; // Same policy for all

    RouteAttributes r_low_ad; // Low Admin Distance, should be preferred
    r_low_ad.nextHop = MustIpStringToInt("10.0.1.1"); r_low_ad.adminDistance = 5; r_low_ad.localPref = 100; r_low_ad.med = 100;
    tree.addRoute(dest_prefix, dest_prefix_len, rule_base, r_low_ad);

    RouteAttributes r_high_lp; // Higher Local Pref
    r_high_lp.nextHop = MustIpStringToInt("10.0.1.2"); r_high_lp.adminDistance = 10; r_high_lp.localPref = 200; r_high_lp.med = 100;
    tree.addRoute(dest_prefix, dest_prefix_len, rule_base, r_high_lp);

    RouteAttributes r_low_med; // Lower MED
    r_low_med.nextHop = MustIpStringToInt("10.0.1.3"); r_low_med.adminDistance = 10; r_low_med.localPref = 100; r_low_med.med = 50;
    tree.addRoute(dest_prefix, dest_prefix_len, rule_base, r_low_med);

    // Test Admin Distance preference
    auto best_route = tree.findBestRoute(packet);
    ASSERT_NE(best_route, nullptr);
    EXPECT_EQ(best_route->nextHop, MustIpStringToInt("10.0.1.1")); // r_low_ad

    // To test LocalPref, we need same Admin Distance
    PolicyRoutingTree tree_lp;
    RouteAttributes r_lp1, r_lp2;
    r_lp1.nextHop = MustIpStringToInt("10.0.2.1"); r_lp1.adminDistance = 10; r_lp1.localPref = 100;
    r_lp2.nextHop = MustIpStringToInt("10.0.2.2"); r_lp2.adminDistance = 10; r_lp2.localPref = 200; // Higher LP
    tree_lp.addRoute(dest_prefix, dest_prefix_len, rule_base, r_lp1);
    tree_lp.addRoute(dest_prefix, dest_prefix_len, rule_base, r_lp2);
    best_route = tree_lp.findBestRoute(packet);
    ASSERT_NE(best_route, nullptr);
    EXPECT_EQ(best_route->nextHop, MustIpStringToInt("10.0.2.2")); // r_lp2 (higher LP)

    // To test MED, we need same Admin Distance and LocalPref
    PolicyRoutingTree tree_med;
    RouteAttributes r_med1, r_med2;
    r_med1.nextHop = MustIpStringToInt("10.0.3.1"); r_med1.adminDistance = 10; r_med1.localPref = 100; r_med1.med = 100;
    r_med2.nextHop = MustIpStringToInt("10.0.3.2"); r_med2.adminDistance = 10; r_med2.localPref = 100; r_med2.med = 50; // Lower MED
    tree_med.addRoute(dest_prefix, dest_prefix_len, rule_base, r_med1);
    tree_med.addRoute(dest_prefix, dest_prefix_len, rule_base, r_med2);
    best_route = tree_med.findBestRoute(packet);
    ASSERT_NE(best_route, nullptr);
    EXPECT_EQ(best_route->nextHop, MustIpStringToInt("10.0.3.2")); // r_med2 (lower MED)
}

TEST(VrfRoutingTableManagerTest, BasicVrfOperations) {
    VrfRoutingTableManager manager;
    uint32_t vrf_red = 1;
    uint32_t vrf_blue = 2;

    RouteAttributes attrs_red;
    attrs_red.nextHop = MustIpStringToInt("192.168.1.1"); // VRF Red NH
    PolicyRule rule_red_default;
    manager.addRoute(vrf_red, "10.0.0.0", 16, rule_red_default, attrs_red);

    RouteAttributes attrs_blue;
    attrs_blue.nextHop = MustIpStringToInt("172.16.1.1"); // VRF Blue NH
    PolicyRule rule_blue_default;
    manager.addRoute(vrf_blue, "10.0.0.0", 16, rule_blue_default, attrs_blue); // Same prefix, diff VRF

    PacketInfo packet_to_10_0_0_5;
    packet_to_10_0_0_5.dstIP = MustIpStringToInt("10.0.0.5");

    // Check VRF Red
    auto selected_red_opt = manager.selectEcmpPathUsingFlowHash(vrf_red, packet_to_10_0_0_5);
    ASSERT_TRUE(selected_red_opt.has_value());
    EXPECT_EQ(selected_red_opt->nextHop, MustIpStringToInt("192.168.1.1"));

    // Check VRF Blue
    auto selected_blue_opt = manager.selectEcmpPathUsingFlowHash(vrf_blue, packet_to_10_0_0_5);
    ASSERT_TRUE(selected_blue_opt.has_value());
    EXPECT_EQ(selected_blue_opt->nextHop, MustIpStringToInt("172.16.1.1"));

    // Check non-existent VRF
    uint32_t vrf_green = 3;
    auto selected_green_opt = manager.selectEcmpPathUsingFlowHash(vrf_green, packet_to_10_0_0_5);
    EXPECT_FALSE(selected_green_opt.has_value());

    // Test specific policy within a VRF
    PolicyRule rule_red_http = rule_red_default;
    rule_red_http.dstPort = 80; rule_red_http.protocol = 6; rule_red_http.priority = 50;
    RouteAttributes attrs_red_http_route;
    attrs_red_http_route.nextHop = MustIpStringToInt("192.168.1.254"); // Specific NH for HTTP
    manager.addRoute(vrf_red, "10.0.0.0", 16, rule_red_http, attrs_red_http_route);

    PacketInfo http_packet_to_10_0_0_5 = packet_to_10_0_0_5;
    http_packet_to_10_0_0_5.dstPort = 80; http_packet_to_10_0_0_5.protocol = 6;

    selected_red_opt = manager.selectEcmpPathUsingFlowHash(vrf_red, http_packet_to_10_0_0_5);
    ASSERT_TRUE(selected_red_opt.has_value());
    EXPECT_EQ(selected_red_opt->nextHop, MustIpStringToInt("192.168.1.254"));
}

// Test displayRoutes and simulatePacket manually by observing output if necessary.
// Automated testing of console output is possible but more involved.
// For now, these tests focus on the logic and state changes.

TEST(PolicyRoutingTreeTest, RateLimitingAttributes) {
    PolicyRoutingTree tree;
    RouteAttributes attrs;
    attrs.nextHop = MustIpStringToInt("192.168.1.1");
    attrs.rateLimitBps = 1000000; // 1 Mbps
    attrs.burstSizeBytes = 125000; // 125 KB

    PolicyRule rule_default;
    tree.addRoute("50.0.0.0", 8, rule_default, attrs);

    PacketInfo packet;
    packet.dstIP = MustIpStringToInt("50.0.0.1");

    auto best_route_attrs_ptr = tree.findBestRoute(packet);
    ASSERT_NE(best_route_attrs_ptr, nullptr);
    EXPECT_EQ(best_route_attrs_ptr->rateLimitBps, 1000000);
    EXPECT_EQ(best_route_attrs_ptr->burstSizeBytes, 125000);
}

TEST(PolicyRoutingTreeTest, TosAndFlowLabelPolicy) {
    PolicyRoutingTree tree;

    RouteAttributes attrs_tos;
    attrs_tos.nextHop = MustIpStringToInt("10.1.1.1");
    PolicyRule rule_tos;
    rule_tos.tos = 0xB8; // EF (DSCP 46)
    rule_tos.priority = 50;
    tree.addRoute("60.0.0.0", 8, rule_tos, attrs_tos);

    RouteAttributes attrs_flow;
    attrs_flow.nextHop = MustIpStringToInt("10.1.1.2");
    PolicyRule rule_flow;
    rule_flow.flowLabel = 0x12345;
    rule_flow.priority = 60; // Lower priority than ToS rule
    tree.addRoute("60.0.0.0", 8, rule_flow, attrs_flow);

    RouteAttributes attrs_default;
    attrs_default.nextHop = MustIpStringToInt("10.1.1.3");
    PolicyRule rule_default;
    rule_default.priority = 100;
    tree.addRoute("60.0.0.0", 8, rule_default, attrs_default);

    PacketInfo packet_ef;
    packet_ef.dstIP = MustIpStringToInt("60.0.0.1");
    packet_ef.tos = 0xB8;
    auto best_route_ef = tree.findBestRoute(packet_ef);
    ASSERT_NE(best_route_ef, nullptr);
    EXPECT_EQ(best_route_ef->nextHop, MustIpStringToInt("10.1.1.1"));

    PacketInfo packet_flow;
    packet_flow.dstIP = MustIpStringToInt("60.0.0.2");
    packet_flow.flowLabel = 0x12345;
    // packet_flow.tos is 0, so rule_tos won't match
    auto best_route_flow = tree.findBestRoute(packet_flow);
    ASSERT_NE(best_route_flow, nullptr);
    EXPECT_EQ(best_route_flow->nextHop, MustIpStringToInt("10.1.1.2"));

    PacketInfo packet_default;
    packet_default.dstIP = MustIpStringToInt("60.0.0.3");
    // tos = 0, flowLabel = 0
    auto best_route_default = tree.findBestRoute(packet_default);
    ASSERT_NE(best_route_default, nullptr);
    EXPECT_EQ(best_route_default->nextHop, MustIpStringToInt("10.1.1.3"));
}

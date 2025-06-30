#include "policy_radix.h"
#include <iostream> // Explicitly include iostream

// Example usage demonstrating advanced routing scenarios
int main() {
    VrfRoutingTableManager manager; // Changed from PolicyRoutingTree to VrfRoutingTableManager
    
    // Define VRF IDs
    const uint32_t vrfGlobal = 0; // Default/Global VRF
    const uint32_t vrfRed = 1;
    const uint32_t vrfBlue = 2;

    std::cout << "=== Setting up Policy-Based Routing with VRFs ===" << '\n';
    
    // Adding routes to vrfRed
    std::cout << "\n--- Configuring VRF Red (" << vrfRed << ") ---" << '\n';
    // Basic route for 10.0.0.0/16 in vrfRed
    PolicyRule policyRed1_base;
    policyRed1_base.priority = 100;
    RouteAttributes attrsRed1_base;
    attrsRed1_base.nextHop = PolicyRoutingTree::ipStringToInt("192.168.1.1"); // VRF Red specific NH
    attrsRed1_base.adminDistance = 1;
    attrsRed1_base.localPref = 100;
    attrsRed1_base.dscp = 0x00;
    manager.addRoute(vrfRed, "10.0.0.0", 16, policyRed1_base, attrsRed1_base);
    
    // Policy route for HTTP traffic in vrfRed
    PolicyRule policyRed2_http;
    policyRed2_http.srcPrefix = PolicyRoutingTree::ipStringToInt("192.168.100.0");
    policyRed2_http.srcPrefixLen = 24;
    policyRed2_http.priority = 50;
    policyRed2_http.dstPort = 80;
    policyRed2_http.protocol = 6;
    RouteAttributes attrsRed2_http;
    attrsRed2_http.nextHop = PolicyRoutingTree::ipStringToInt("192.168.2.1"); // VRF Red policy NH
    attrsRed2_http.adminDistance = 1;
    attrsRed2_http.localPref = 200;
    attrsRed2_http.dscp = 0x0A;
    attrsRed2_http.rateLimitBps = 1000000; // 1 Mbps
    attrsRed2_http.burstSizeBytes = 125000; // 125 KB
    manager.addRoute(vrfRed, "10.0.0.0", 16, policyRed2_http, attrsRed2_http);

    // Adding routes to vrfBlue
    std::cout << "\n--- Configuring VRF Blue (" << vrfBlue << ") ---" << '\n';
    // Basic route for 10.0.0.0/16 in vrfBlue (same prefix, different NH)
    PolicyRule policyBlue1_base;
    policyBlue1_base.priority = 100;
    RouteAttributes attrsBlue1_base;
    attrsBlue1_base.nextHop = PolicyRoutingTree::ipStringToInt("172.16.1.1"); // VRF Blue specific NH
    attrsBlue1_base.adminDistance = 1;
    attrsBlue1_base.localPref = 100;
    attrsBlue1_base.dscp = 0x00;
    manager.addRoute(vrfBlue, "10.0.0.0", 16, policyBlue1_base, attrsBlue1_base);

    // Different policy route in vrfBlue (e.g., for DNS traffic to 8.8.8.8/32)
    PolicyRule policyBlue2_dns;
    policyBlue2_dns.priority = 60;
    policyBlue2_dns.dstPort = 53; // DNS
    policyBlue2_dns.protocol = 17; // UDP
    RouteAttributes attrsBlue2_dns;
    attrsBlue2_dns.nextHop = PolicyRoutingTree::ipStringToInt("172.16.2.1"); // VRF Blue policy NH
    attrsBlue2_dns.adminDistance = 1;
    attrsBlue2_dns.localPref = 150;
    attrsBlue2_dns.dscp = 0x08; // CS1
    manager.addRoute(vrfBlue, "8.8.8.8", 32, policyBlue2_dns, attrsBlue2_dns);

    // Routes for the Global VRF (vrfGlobal, e.g. for BGP-like and TE routes)
    std::cout << "\n--- Configuring Global VRF (" << vrfGlobal << ") ---" << '\n';
    // BGP-like route for 172.16.0.0/16 in Global VRF
    PolicyRule policyGlobal_bgp;
    policyGlobal_bgp.priority = 100;
    RouteAttributes attrsGlobal_bgp;
    attrsGlobal_bgp.nextHop = PolicyRoutingTree::ipStringToInt("192.168.3.1");
    attrsGlobal_bgp.adminDistance = 20;
    attrsGlobal_bgp.localPref = 150;
    attrsGlobal_bgp.med = 50;
    attrsGlobal_bgp.asPath = {65001, 65002, 12345}; // Changed to vector
    attrsGlobal_bgp.dscp = 0x10;
    manager.addRoute(vrfGlobal, "172.16.0.0", 16, policyGlobal_bgp, attrsGlobal_bgp);
    
    // Traffic engineering example for 203.0.113.0/24 in Global VRF
    // Note: addTrafficEngineering is part of PolicyRoutingTree, not VrfRoutingTableManager directly.
    // For now, we'll manually create these routes in the global VRF.
    // Or, we could add a similar helper to VrfRoutingTableManager if this pattern is common.
    PolicyRule te_primary_policy, te_backup_policy;
    RouteAttributes te_primary_attrs, te_backup_attrs;
    te_primary_policy.priority = 50;
    te_primary_attrs.nextHop = PolicyRoutingTree::ipStringToInt("10.1.1.1");
    te_primary_attrs.localPref = 200; te_primary_attrs.dscp = 0x12;
    manager.addRoute(vrfGlobal, "203.0.113.0", 24, te_primary_policy, te_primary_attrs);
    
    te_backup_policy.priority = 100;
    te_backup_attrs.nextHop = PolicyRoutingTree::ipStringToInt("10.1.1.2");
    te_backup_attrs.localPref = 100; te_backup_attrs.dscp = 0x00;
    manager.addRoute(vrfGlobal, "203.0.113.0", 24, te_backup_policy, te_backup_attrs);


    // Setup for ECMP testing to 77.77.0.0/16 in Global VRF
    std::string ecmpDestPrefix = "77.77.0.0";
    uint8_t ecmpDestPrefixLen = 16;

    PolicyRule p_ecmp_default;
    p_ecmp_default.priority = 90;

    RouteAttributes attr_ecmp1, attr_ecmp2, attr_ecmp3;

    // Path 1 for ECMP in Global VRF
    attr_ecmp1.nextHop = PolicyRoutingTree::ipStringToInt("10.77.1.1");
    attr_ecmp1.adminDistance = 1; attr_ecmp1.localPref = 100; attr_ecmp1.med = 0;
    attr_ecmp1.dscp = 0x08;
    manager.addRoute(vrfGlobal, ecmpDestPrefix, ecmpDestPrefixLen, p_ecmp_default, attr_ecmp1);

    // Path 2 for ECMP in Global VRF
    attr_ecmp2.nextHop = PolicyRoutingTree::ipStringToInt("10.77.1.2");
    attr_ecmp2.adminDistance = 1; attr_ecmp2.localPref = 100; attr_ecmp2.med = 0;
    attr_ecmp2.dscp = 0x08;
    manager.addRoute(vrfGlobal, ecmpDestPrefix, ecmpDestPrefixLen, p_ecmp_default, attr_ecmp2);
    
    // Path 3 for ECMP in Global VRF
    attr_ecmp3.nextHop = PolicyRoutingTree::ipStringToInt("10.77.1.3");
    attr_ecmp3.adminDistance = 1; attr_ecmp3.localPref = 100; attr_ecmp3.med = 0;
    attr_ecmp3.dscp = 0x08;
    manager.addRoute(vrfGlobal, ecmpDestPrefix, ecmpDestPrefixLen, p_ecmp_default, attr_ecmp3);

    // More specific policy route within ECMP range in Global VRF
    PolicyRule p_ecmp_specific_policy;
    p_ecmp_specific_policy.priority = 80;
    p_ecmp_specific_policy.srcPrefix = PolicyRoutingTree::ipStringToInt("55.55.55.0");
    p_ecmp_specific_policy.srcPrefixLen = 24;
    RouteAttributes attr_ecmp_specific;
    attr_ecmp_specific.nextHop = PolicyRoutingTree::ipStringToInt("10.77.2.2");
    attr_ecmp_specific.adminDistance = 1; attr_ecmp_specific.localPref = 150; attr_ecmp_specific.med = 0;
    attr_ecmp_specific.dscp = 0x0C;
    manager.addRoute(vrfGlobal, ecmpDestPrefix, ecmpDestPrefixLen, p_ecmp_specific_policy, attr_ecmp_specific);

    // DSCP marking test route in Global VRF
    PolicyRule policy_dscp_test_global;
    policy_dscp_test_global.priority = 60;
    RouteAttributes attrs_dscp_test_global;
    attrs_dscp_test_global.nextHop = PolicyRoutingTree::ipStringToInt("192.168.70.1");
    attrs_dscp_test_global.dscp = 0x1A;
    manager.addRoute(vrfGlobal, "192.168.70.0", 24, policy_dscp_test_global, attrs_dscp_test_global);

    // Policy route for specific Flow Label in vrfRed
    PolicyRule policy_flow_label_red;
    policy_flow_label_red.priority = 40;
    policy_flow_label_red.flowLabel = 12345;
    RouteAttributes attrs_flow_label_red;
    attrs_flow_label_red.nextHop = PolicyRoutingTree::ipStringToInt("192.168.4.1");
    attrs_flow_label_red.adminDistance = 1;
    attrs_flow_label_red.localPref = 250;
    attrs_flow_label_red.dscp = 0x1C;
    manager.addRoute(vrfRed, "10.0.0.0", 16, policy_flow_label_red, attrs_flow_label_red);

    // ToS/DSCP based routing policy in Global VRF (e.g. for EF traffic)
    PolicyRule policy_match_ef_global;
    policy_match_ef_global.tos = 0xb8;
    policy_match_ef_global.priority = 40;
    RouteAttributes attrs_route_ef_global;
    attrs_route_ef_global.nextHop = PolicyRoutingTree::ipStringToInt("10.200.1.1");
    attrs_route_ef_global.dscp = 0xb8;
    manager.addRoute(vrfGlobal, "0.0.0.0", 0, policy_match_ef_global, attrs_route_ef_global);

    // --- New ToS Specific Policies and Routes for 90.0.0.0/8 in Global VRF ---
    std::cout << "\n--- Configuring ToS-specific routes for 90.0.0.0/8 in Global VRF (" << vrfGlobal << ") ---" << '\n';
    std::string tos_test_prefix = "90.0.0.0";
    uint8_t tos_test_prefix_len = 8;

    // Policy for Critical ToS (CS6 = 0xC0)
    PolicyRule policy_tos_critical;
    policy_tos_critical.priority = 30;
    policy_tos_critical.tos = 0xC0; // DSCP CS6 (Critical)
    RouteAttributes attrs_tos_critical;
    attrs_tos_critical.nextHop = PolicyRoutingTree::ipStringToInt("10.90.1.1");
    attrs_tos_critical.dscp = 0xC0; // Can choose to re-mark or preserve
    manager.addRoute(vrfGlobal, tos_test_prefix, tos_test_prefix_len, policy_tos_critical, attrs_tos_critical);

    // Policy for Low Priority ToS (CS1 = 0x20)
    PolicyRule policy_tos_low_prio;
    policy_tos_low_prio.priority = 70;
    policy_tos_low_prio.tos = 0x20; // DSCP CS1 (Low Priority)
    RouteAttributes attrs_tos_low_prio;
    attrs_tos_low_prio.nextHop = PolicyRoutingTree::ipStringToInt("10.90.1.2");
    attrs_tos_low_prio.dscp = 0x20;
    manager.addRoute(vrfGlobal, tos_test_prefix, tos_test_prefix_len, policy_tos_low_prio, attrs_tos_low_prio);
    
    // Default Policy for the prefix (matches any ToS if others don't, or specifically ToS 0)
    PolicyRule policy_tos_default_for_prefix;
    policy_tos_default_for_prefix.priority = 100;
    policy_tos_default_for_prefix.tos = 0; // Explicitly match ToS 0 / Best Effort, or acts as fallback if packet ToS doesn't match others.
    RouteAttributes attrs_tos_default_for_prefix;
    attrs_tos_default_for_prefix.nextHop = PolicyRoutingTree::ipStringToInt("10.90.1.3");
    attrs_tos_default_for_prefix.dscp = 0x00; // Mark as Best Effort
    manager.addRoute(vrfGlobal, tos_test_prefix, tos_test_prefix_len, policy_tos_default_for_prefix, attrs_tos_default_for_prefix);


    // Display routes for all configured VRFs
    std::cout << "\n=== Displaying All Configured Routing Tables ===" << '\n';
    manager.displayAllRoutes(); // Using displayAllRoutes to show all VRF tables
    
    // Simulate Packets for VRF Red
    std::cout << "\n\n=== Packet Lookups for VRF Red (" << vrfRed << ") ===" << '\n';
    // Regular packet to 10.0.5.5 in vrfRed (should use 192.168.1.1)
    manager.simulatePacket(vrfRed, "10.10.10.10", "10.0.5.5", 12345, 443, 6);
    // HTTP traffic from 192.168.100.50 to 10.0.5.5 in vrfRed (should use 192.168.2.1)
    manager.simulatePacket(vrfRed, "192.168.100.50", "10.0.5.5", 54321, 80, 6);
    // Flow Label specific packet in vrfRed (should use 192.168.4.1)
    manager.simulatePacket(vrfRed, "192.168.200.1", "10.0.5.5", 1000, 2000, 6, 0, 12345);
    // Flow Label mismatch packet in vrfRed (should fall back, e.g. to 192.168.2.1 due to src IP)
    manager.simulatePacket(vrfRed, "192.168.100.50", "10.0.5.5", 1000, 2000, 6, 0, 54321);


    // Simulate Packets for VRF Blue
    std::cout << "\n\n=== Packet Lookups for VRF Blue (" << vrfBlue << ") ===" << '\n';
    // Regular packet to 10.0.5.5 in vrfBlue (should use 172.16.1.1)
    manager.simulatePacket(vrfBlue, "10.10.10.10", "10.0.5.5", 12345, 443, 6);
    // DNS packet to 8.8.8.8 in vrfBlue (should use 172.16.2.1)
    manager.simulatePacket(vrfBlue, "10.10.10.10", "8.8.8.8", 12345, 53, 17);


    // Simulate Packets for Global VRF (vrfGlobal)
    std::cout << "\n\n=== Packet Lookups for Global VRF (" << vrfGlobal << ") ===" << '\n';
    // Traffic to TE-enabled destination 203.0.113.100 (should prefer 10.1.1.1)
    manager.simulatePacket(vrfGlobal, "1.1.1.1", "203.0.113.100", 12345, 443, 17);
    
    std::cout << "\n--- ECMP Test in Global VRF ---" << '\n';
    std::string ecmpTargetIpStr = "77.77.0.100";
    manager.simulatePacket(vrfGlobal, "1.2.3.4", ecmpTargetIpStr, 1001, 80, 6);
    manager.simulatePacket(vrfGlobal, "5.6.7.8", ecmpTargetIpStr, 1001, 80, 6);
    manager.simulatePacket(vrfGlobal, "55.55.55.5", ecmpTargetIpStr, 3000, 80, 6); // Should match specific policy

    std::cout << "\n--- ToS/DSCP based routing in Global VRF ---" << '\n';
    // EF Traffic in Global VRF (should use 10.200.1.1) - Existing test, good for verification
    manager.simulatePacket(vrfGlobal, "192.168.100.10", "10.250.1.1", 1000, 2000, 6, 0xb8);
    // Best Effort traffic to a general IP, not matching specific Global VRF policies (likely no route if no default in Global)
    // This will now be handled by the 0.0.0.0/0 tos 0xb8 if tos is 0xb8, or default 0.0.0.0/0 if no other default exists
    manager.simulatePacket(vrfGlobal, "192.168.100.11", "10.250.1.2", 1000, 2000, 6, 0x00);
    
    std::cout << "\n--- Simulating Specific DSCP Test Route in Global VRF ---" << '\n';
    // This existing test for 192.168.70.0/24 has dscp = 0x1A set on its RouteAttributes, but the policy itself doesn't filter on ToS.
    // The packet simulation here doesn't specify a ToS, so packet.tos will be 0.
    // The policy policy_dscp_test_global has policy.tos = 0. This means it will match.
    manager.simulatePacket(vrfGlobal, "10.10.10.10", "192.168.70.5", 1234, 5678, 6); // Packet ToS = 0

    // --- New ToS Specific Packet Simulations for 90.0.0.0/8 in Global VRF ---
    std::cout << "\n\n=== ToS-Specific Packet Lookups for 90.0.0.0/8 in Global VRF (" << vrfGlobal << ") ===" << '\n';
    std::string tos_target_ip = "90.1.2.3";
    // Packet with ToS 0xC0 (Critical) -> should use 10.90.1.1
    manager.simulatePacket(vrfGlobal, "200.1.1.1", tos_target_ip, 1001, 80, 6, 0xC0);
    // Packet with ToS 0x20 (Low Priority) -> should use 10.90.1.2
    manager.simulatePacket(vrfGlobal, "200.1.1.2", tos_target_ip, 1002, 80, 6, 0x20);
    // Packet with ToS 0x00 (Best Effort) -> should use 10.90.1.3 (matches policy_tos_default_for_prefix)
    manager.simulatePacket(vrfGlobal, "200.1.1.3", tos_target_ip, 1003, 80, 6, 0x00);
    // Packet with ToS 0xA0 (CS5 - no specific policy) -> should use 10.90.1.3 (matches policy_tos_default_for_prefix due to its ToS 0)
    manager.simulatePacket(vrfGlobal, "200.1.1.4", tos_target_ip, 1004, 80, 6, 0xA0);


    std::cout << "\n\nFinal Routing Tables (All VRFs):" << '\n';
    manager.displayAllRoutes(); // Display all VRF tables at the end
    
    return 0;
}

#include "policy_radix.h"
#include <iostream> // Explicitly include iostream

// Example usage demonstrating advanced routing scenarios
int main() {
    PolicyRoutingTree router;
    
    std::cout << "=== Setting up Policy-Based Routing ===" << std::endl;
    
    // Basic route for 10.0.0.0/16
    PolicyRule policy1_base; // Renamed to avoid conflict
    policy1_base.priority = 100; // Default priority
    RouteAttributes attrs1_base; // Renamed
    attrs1_base.nextHop = router.ipStringToInt("192.168.1.1");
    attrs1_base.adminDistance = 1;
    attrs1_base.localPref = 100;
    attrs1_base.dscp = 0x00; // Best Effort (default)
    router.addRoute("10.0.0.0", 16, policy1_base, attrs1_base);
    
    // Policy route for specific HTTP traffic to 10.0.0.0/16 from 192.168.100.0/24
    PolicyRule policy2_http; // Renamed
    policy2_http.srcPrefix = router.ipStringToInt("192.168.100.0");
    policy2_http.srcPrefixLen = 24;
    policy2_http.priority = 50;  // Higher policy priority
    policy2_http.dstPort = 80;  // HTTP traffic
    policy2_http.protocol = 6;  // TCP
    RouteAttributes attrs2_http; // Renamed
    attrs2_http.nextHop = router.ipStringToInt("192.168.2.1"); // Different next hop for this policy
    attrs2_http.adminDistance = 1;
    attrs2_http.localPref = 200;  // Higher preference
    attrs2_http.dscp = 0x0A; // AF11 (DSCP 10)
    router.addRoute("10.0.0.0", 16, policy2_http, attrs2_http); // Applied to the same 10.0.0.0/16
    
    // BGP-like route for 172.16.0.0/16
    PolicyRule policy3_bgp; // Renamed
    policy3_bgp.priority = 100; // Standard priority
    RouteAttributes attrs3_bgp; // Renamed
    attrs3_bgp.nextHop = router.ipStringToInt("192.168.3.1");
    attrs3_bgp.adminDistance = 20;  // BGP admin distance
    attrs3_bgp.localPref = 150;
    attrs3_bgp.med = 50;
    attrs3_bgp.asPath = 12345; // Example AS path
    attrs3_bgp.dscp = 0x10; // CS2 (DSCP 16)
    router.addRoute("172.16.0.0", 16, policy3_bgp, attrs3_bgp);
    
    // Traffic engineering example for 203.0.113.0/24
    router.addTrafficEngineering("203.0.113.0", 24,
                                router.ipStringToInt("10.1.1.1"),  // Primary NH
                                router.ipStringToInt("10.1.1.2"),  // Backup NH
                                1000000,  // Bandwidth indication (e.g., in bps)
                                10);      // Delay indication (e.g., in ms) - currently unused in RouteAttributes logic

    // Setup for ECMP testing to 77.77.0.0/16
    std::string ecmpDestPrefix = "77.77.0.0";
    uint8_t ecmpDestPrefixLen = 16;

    PolicyRule p_ecmp_default; // Policy for ECMP paths
    p_ecmp_default.priority = 90; // A common priority for these ECMP paths

    RouteAttributes attr_ecmp1, attr_ecmp2, attr_ecmp3;

    // Path 1 for ECMP
    attr_ecmp1.nextHop = router.ipStringToInt("10.77.1.1");
    attr_ecmp1.adminDistance = 1; attr_ecmp1.localPref = 100; attr_ecmp1.med = 0;
    attr_ecmp1.dscp = 0x08; // CS1 (DSCP 8)
    router.addRoute(ecmpDestPrefix, ecmpDestPrefixLen, p_ecmp_default, attr_ecmp1);

    // Path 2 for ECMP
    attr_ecmp2.nextHop = router.ipStringToInt("10.77.1.2");
    attr_ecmp2.adminDistance = 1; attr_ecmp2.localPref = 100; attr_ecmp2.med = 0;
    attr_ecmp2.dscp = 0x08; // CS1 (DSCP 8)
    router.addRoute(ecmpDestPrefix, ecmpDestPrefixLen, p_ecmp_default, attr_ecmp2);
    
    // Path 3 for ECMP
    attr_ecmp3.nextHop = router.ipStringToInt("10.77.1.3");
    attr_ecmp3.adminDistance = 1; attr_ecmp3.localPref = 100; attr_ecmp3.med = 0;
    attr_ecmp3.dscp = 0x08; // CS1 (DSCP 8)
    router.addRoute(ecmpDestPrefix, ecmpDestPrefixLen, p_ecmp_default, attr_ecmp3);

    // Add a more specific route within the ECMP range but with a different policy, to test LPM + Policy
    PolicyRule p_ecmp_specific_policy;
    p_ecmp_specific_policy.priority = 80; // Higher priority
    p_ecmp_specific_policy.srcPrefix = router.ipStringToInt("55.55.55.0");
    p_ecmp_specific_policy.srcPrefixLen = 24;
    RouteAttributes attr_ecmp_specific;
    attr_ecmp_specific.nextHop = router.ipStringToInt("10.77.2.2"); // Different NH
    attr_ecmp_specific.adminDistance = 1; attr_ecmp_specific.localPref = 150; attr_ecmp_specific.med = 0;
    attr_ecmp_specific.dscp = 0x0C; // AF12 (DSCP 12)
    router.addRoute(ecmpDestPrefix, ecmpDestPrefixLen, p_ecmp_specific_policy, attr_ecmp_specific);

    // Route for specific DSCP marking test
    PolicyRule policy_dscp_test;
    policy_dscp_test.priority = 60;
    RouteAttributes attrs_dscp_test;
    attrs_dscp_test.nextHop = router.ipStringToInt("192.168.70.1");
    attrs_dscp_test.dscp = 0x1A; // AF31 (DSCP 26)
    router.addRoute("192.168.70.0", 24, policy_dscp_test, attrs_dscp_test);


    router.displayRoutes();
    
    std::cout << "\n=== Standard Packet Lookups (Testing Policy and TE) ===" << std::endl;

    // Regular packet to 10.0.5.5 (should use 192.168.1.1)
    router.simulatePacket("10.10.10.10", "10.0.5.5", 12345, 443, 6); // general UDP
    
    // HTTP traffic from 192.168.100.50 to 10.0.5.5 (should match policy2_http -> 192.168.2.1)
    router.simulatePacket("192.168.100.50", "10.0.5.5", 54321, 80, 6); // TCP, DstPort 80
    
    // Traffic to TE-enabled destination 203.0.113.100 (should prefer 10.1.1.1)
    router.simulatePacket("1.1.1.1", "203.0.113.100", 12345, 443, 17); // general UDP

    std::cout << "\n=== Flow-based Load Balancing Test (ECMP for " << ecmpDestPrefix << "/" << (int)ecmpDestPrefixLen << ") ===" << std::endl;
    std::string ecmpTargetIpStr = "77.77.0.100"; // An IP within the ECMP prefix

    std::cout << "--- Simulating different flows to " << ecmpTargetIpStr << " ---" << std::endl;
    // These flows should be distributed among 10.77.1.1, 10.77.1.2, 10.77.1.3
    router.simulatePacket("1.2.3.4", ecmpTargetIpStr, 1001, 80, 6);    // Flow 1 (TCP)
    router.simulatePacket("5.6.7.8", ecmpTargetIpStr, 1001, 80, 6);    // Flow 2 (TCP, different src IP)
    router.simulatePacket("1.2.3.4", ecmpTargetIpStr, 1002, 80, 6);    // Flow 3 (TCP, different src Port)
    router.simulatePacket("1.2.3.4", ecmpTargetIpStr, 1001, 80, 6);    // Flow 4 (TCP, same as Flow 1 - should take same path)
    router.simulatePacket("1.2.3.4", ecmpTargetIpStr, 1001, 53, 17);   // Flow 5 (UDP, different DstPort and Proto)
    router.simulatePacket("9.10.11.12", ecmpTargetIpStr, 2001, 443, 6); // Flow 6 (TCP)

    // This flow should match the more specific policy p_ecmp_specific_policy (NH 10.77.2.2)
    std::cout << "\n--- Simulating specific policy flow to " << ecmpTargetIpStr << " ---" << std::endl;
    router.simulatePacket("55.55.55.5", ecmpTargetIpStr, 3000, 80, 6);


    // Test with ToS/DSCP based routing
    PolicyRule policy_match_ef; // Renamed for clarity
    policy_match_ef.tos = 0xb8; // Match incoming EF packets (DSCP 46)
    policy_match_ef.priority = 40; // High priority for EF traffic
    RouteAttributes attrs_route_ef; // Renamed for clarity
    attrs_route_ef.nextHop = router.ipStringToInt("10.200.1.1"); // Special path for EF traffic
    attrs_route_ef.dscp = 0xb8; // Re-affirm EF marking, or could be different (e.g., policing action)
    // Apply to a general prefix (e.g. default route) to catch EF traffic to any destination not more specifically covered
    router.addRoute("0.0.0.0", 0, policy_match_ef, attrs_route_ef);

    std::cout << "\n--- Simulating packet with ToS/DSCP value (Matching Policy) ---" << std::endl;
    // This packet has ToS=0xb8, should match policy_match_ef and be routed to 10.200.1.1, applying DSCP 0xb8
    router.simulatePacket("192.168.100.10", "10.250.1.1", 1000, 2000, 6, 0xb8); // EF Traffic
    
    // This packet has default ToS=0, should use policy1_base (192.168.1.1) as 0.0.0.0/0 is less specific than 10.0.0.0/16
    // and policy_match_ef will not match due to ToS.
    router.simulatePacket("192.168.100.11", "10.0.1.2", 1000, 2000, 6, 0x00); // Best Effort
    
    // This HTTP packet from 192.168.100.x with ToS=0xb8.
    // It matches 10.0.0.0/16 (policy2_http, prio 50) and 0.0.0.0/0 (policy_match_ef, prio 40).
    // Since 10.0.0.0/16 is more specific, policy2_http (DSCP 0x0A) should be chosen.
    // The ToS matching policy on 0.0.0.0/0 is not preferred due to less specific prefix.
    router.simulatePacket("192.168.100.12", "10.0.5.5", 54321, 80, 6, 0xb8);
    
    // This HTTP packet from 192.168.100.x with ToS=0x00 should use policy2_http (DSCP 0x0A) due to specific match.
    router.simulatePacket("192.168.100.12", "10.0.5.5", 54321, 80, 6, 0x00);

    std::cout << "\n--- Simulating Specific DSCP Test Route ---" << std::endl;
    // Packet to 192.168.70.5, should match route to 192.168.70.1 and apply DSCP 0x1A
    router.simulatePacket("10.10.10.10", "192.168.70.5", 1234, 5678, 6);


    std::cout << "\nFinal Routing Table:" << std::endl;
    router.displayRoutes();
    
    return 0;
}

#inlcude "policy_radix.h"

// Example usage demonstrating advanced routing scenarios
int main() {
    PolicyRoutingTree router;
    
    std::cout << "=== Setting up Policy-Based Routing ===" << std::endl;
    
    // Basic routes
    PolicyRule policy1;
    policy1.priority = 10;
    RouteAttributes attrs1;
    attrs1.nextHop = router.ipStringToInt("192.168.1.1");
    attrs1.adminDistance = 1;
    attrs1.localPref = 100;
    router.addRoute("10.0.0.0", 16, policy1, attrs1);
    
    // Policy route for specific traffic
    PolicyRule policy2;
    policy2.srcPrefix = router.ipStringToInt("192.168.100.0");
    policy2.srcPrefixLen = 24;
    policy2.priority = 5;  // Higher priority
    policy2.dstPort = 80;  // HTTP traffic
    RouteAttributes attrs2;
    attrs2.nextHop = router.ipStringToInt("192.168.2.1");
    attrs2.adminDistance = 1;
    attrs2.localPref = 200;  // Higher preference
    router.addRoute("10.0.0.0", 16, policy2, attrs2);
    
    // BGP route with different attributes
    PolicyRule policy3;
    policy3.priority = 20;
    RouteAttributes attrs3;
    attrs3.nextHop = router.ipStringToInt("192.168.3.1");
    attrs3.adminDistance = 20;  // BGP admin distance
    attrs3.localPref = 150;
    attrs3.med = 50;
    attrs3.asPath = 12345;
    router.addRoute("172.16.0.0", 16, policy3, attrs3);
    
    // Traffic engineering example
    router.addTrafficEngineering("203.0.113.0", 24,
                                router.ipStringToInt("10.1.1.1"),  // Primary
                                router.ipStringToInt("10.1.1.2"),  // Backup
                                1000000,  // 1Gbps
                                10);      // 10ms delay
    
    router.displayRoutes();
    
    std::cout << "\n=== Testing Packet Lookups ===" << std::endl;
    
    // Regular packet
    router.simulatePacket("10.10.10.10", "10.0.5.5", 12345, 443, 6);
    
    // HTTP traffic from specific subnet (should match policy route)
    router.simulatePacket("192.168.100.50", "10.0.5.5", 54321, 80, 6);
    
    // Traffic to TE-enabled destination
    router.simulatePacket("1.1.1.1", "203.0.113.100", 12345, 443, 6);
    
    std::cout << "\n=== ECMP Analysis ===" << std::endl;
    PacketInfo ecmpPacket;
    ecmpPacket.srcIP = router.ipStringToInt("1.1.1.1");
    ecmpPacket.dstIP = router.ipStringToInt("203.0.113.100");
    ecmpPacket.protocol = 6;
    
    auto ecmpPaths = router.getEqualCostPaths(ecmpPacket);
    std::cout << "Equal-cost paths found: " << ecmpPaths.size() << std::endl;
    for (const auto& path : ecmpPaths) {
        std::cout << "  Next hop: " << router.ipIntToString(path.nextHop) << std::endl;
    }
    
    return 0;
}

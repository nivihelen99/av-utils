#include "tcam.h"

// Usage example with performance comparison
void tcam_optimization_example() {
    OptimizedTCAM tcam;
    
    // Add some rules with ranges
    OptimizedTCAM::WildcardFields rule1;
    rule1.src_ip = 0x0A000000; rule1.src_ip_mask = 0xFF000000; // 10.0.0.0/8
    rule1.dst_ip = 0xC0A80000; rule1.dst_ip_mask = 0xFFFF0000; // 192.168.0.0/16
    rule1.src_port_min = 1024; rule1.src_port_max = 65535;     // High ports
    rule1.dst_port_min = 80; rule1.dst_port_max = 80;         // HTTP
    rule1.protocol = 6; rule1.protocol_mask = 0xFF;           // TCP
    
    tcam.add_rule_with_ranges(rule1, 100, 1);
    
    // Test different lookup methods
    std::vector<uint8_t> test_packet = {
        0x0A, 0x00, 0x00, 0x01,  // src IP: 10.0.0.1
        0xC0, 0xA8, 0x01, 0x01,  // dst IP: 192.168.1.1
        0x04, 0x00,              // src port: 1024
        0x00, 0x50,              // dst port: 80
        0x06,                    // protocol: TCP
        0x08, 0x00               // eth_type: IPv4
    };
    
    int result_linear = tcam.lookup_linear(test_packet);
    int result_tree = tcam.lookup_decision_tree(test_packet);
    int result_bitmap = tcam.lookup_bitmap(test_packet);
    
    // Batch processing
    std::vector<std::vector<uint8_t>> batch_packets(100, test_packet);
    std::vector<int> batch_results;
    tcam.lookup_batch(batch_packets, batch_results);
    
    // Optimize for traffic pattern
    tcam.optimize_for_traffic_pattern(batch_packets);
}
// Usage example
int main() {
    // TCAM for firewall rules
    TCAM firewall;
    std::vector<uint8_t> rule_val = {0x0A, 0x00, 0x00, 0x01}; // 10.0.0.1
    std::vector<uint8_t> rule_mask = {0xFF, 0xFF, 0xFF, 0xFF}; // Exact match
    firewall.add_rule(rule_val, rule_mask, 100, 1); // Priority 100, action 1
    
    // ARP cache
    ARPCache arp;
    std::array<uint8_t, 6> mac;
    bool found = arp.lookup(0x0A000001, mac);
    
    // VLAN processing
    VLANProcessor vlan_proc;
    vlan_proc.configure_port(1, 100, false); // Access port, VLAN 100
    vlan_proc.configure_port(2, 1, true, {100, 200}); // Trunk port
    
    // STP
    std::array<uint8_t, 6> bridge_mac = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    STPProcessor stp(32768, bridge_mac);
    stp.add_port(1, 100);
    
    // Multicast
    MulticastManager mcast;
    mcast.join_group(0xE0000001, 1); // 224.0.0.1 on port 1

    tcam_optimization_example();
}

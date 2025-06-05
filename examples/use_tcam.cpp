#include "tcam.h"

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
}

#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <array>
#include <bitset>
#include <queue>

// 1. TCAM (Ternary Content Addressable Memory) for ACL/Firewall Rules
class TCAM {
private:
    struct TernaryRule {
        std::vector<uint8_t> value;    // 0 or 1
        std::vector<uint8_t> mask;     // 0 = don't care, 1 = must match
        int priority;
        int action;
        
        bool matches(const std::vector<uint8_t>& packet) const {
            for (size_t i = 0; i < value.size() && i < packet.size(); ++i) {
                if (mask[i] && ((value[i] ^ packet[i]) & mask[i])) {
                    return false;
                }
            }
            return true;
        }
    };
    
    std::vector<TernaryRule> rules;
    
public:
    void add_rule(const std::vector<uint8_t>& value,
                  const std::vector<uint8_t>& mask,
                  int priority, int action) {
        rules.push_back({value, mask, priority, action});
        // Sort by priority (higher priority first)
        std::sort(rules.begin(), rules.end(),
                  [](const TernaryRule& a, const TernaryRule& b) {
                      return a.priority > b.priority;
                  });
    }
    
    int lookup(const std::vector<uint8_t>& packet) const {
        for (const auto& rule : rules) {
            if (rule.matches(packet)) {
                return rule.action;
            }
        }
        return -1; // No match
    }
    
    // Optimize with range trees for better performance
    void optimize() {
        // Group rules by common prefixes
        // Could implement decision trees or other optimization
    }
};

// 2. ARP/ND Cache with Aging and State Machine
class ARPCache {
private:
    enum ARPState { INCOMPLETE, REACHABLE, STALE, PROBE, DELAY };
    
    struct ARPEntry {
        std::array<uint8_t, 6> mac;
        ARPState state;
        std::chrono::steady_clock::time_point timestamp;
        int probe_count;
        std::queue<std::vector<uint8_t>> pending_packets; // Packets waiting for resolution
    };
    
    std::unordered_map<uint32_t, ARPEntry> cache;
    static constexpr int MAX_PROBES = 3;
    static constexpr int REACHABLE_TIME = 300; // seconds
    
public:
    bool lookup(uint32_t ip, std::array<uint8_t, 6>& mac) {
        auto it = cache.find(ip);
        if (it != cache.end() && it->second.state == REACHABLE) {
            mac = it->second.mac;
            return true;
        }
        
        // Trigger ARP request if not found or stale
        if (it == cache.end() || it->second.state == STALE) {
            send_arp_request(ip);
            if (it == cache.end()) {
                cache[ip] = {{}, INCOMPLETE, std::chrono::steady_clock::now(), 0, {}};
            }
        }
        return false;
    }
    
    void add_entry(uint32_t ip, const std::array<uint8_t, 6>& mac) {
        cache[ip] = {mac, REACHABLE, std::chrono::steady_clock::now(), 0, {}};
        
        // Send any pending packets
        auto& entry = cache[ip];
        while (!entry.pending_packets.empty()) {
            // Forward pending packet
            entry.pending_packets.pop();
        }
    }
    
    void age_entries() {
        auto now = std::chrono::steady_clock::now();
        for (auto it = cache.begin(); it != cache.end();) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                now - it->second.timestamp).count();
            
            switch (it->second.state) {
                case REACHABLE:
                    if (age > REACHABLE_TIME) {
                        it->second.state = STALE;
                    }
                    ++it;
                    break;
                case INCOMPLETE:
                    if (age > 1) { // 1 second timeout
                        if (++it->second.probe_count > MAX_PROBES) {
                            it = cache.erase(it);
                        } else {
                            send_arp_request(it->first);
                            it->second.timestamp = now;
                            ++it;
                        }
                    } else {
                        ++it;
                    }
                    break;
                default:
                    ++it;
            }
        }
    }
    
private:
    void send_arp_request(uint32_t ip) {
        // Implementation would send actual ARP request
    }
};

// 3. VLAN Tag Processing with 802.1Q/802.1ad Support
class VLANProcessor {
private:
    struct VLANConfig {
        uint16_t vlan_id;
        uint8_t priority;
        bool is_trunk;
        std::vector<uint16_t> allowed_vlans;
    };
    
    std::unordered_map<int, VLANConfig> port_config; // port_id -> config
    
public:
    struct EthernetFrame {
        std::array<uint8_t, 6> dst_mac;
        std::array<uint8_t, 6> src_mac;
        uint16_t ethertype;
        std::vector<uint16_t> vlan_tags; // Support for Q-in-Q
        std::vector<uint8_t> payload;
    };
    
    bool process_ingress(int port_id, EthernetFrame& frame) {
        auto it = port_config.find(port_id);
        if (it == port_config.end()) return false;
        
        const auto& config = it->second;
        
        // Handle untagged frames on access ports
        if (!config.is_trunk && frame.vlan_tags.empty()) {
            frame.vlan_tags.push_back(config.vlan_id);
            return true;
        }
        
        // Validate VLAN tags on trunk ports
        if (config.is_trunk && !frame.vlan_tags.empty()) {
            uint16_t outer_vlan = frame.vlan_tags[0];
            return std::find(config.allowed_vlans.begin(),
                           config.allowed_vlans.end(),
                           outer_vlan) != config.allowed_vlans.end();
        }
        
        return false;
    }
    
    void process_egress(int port_id, EthernetFrame& frame) {
        auto it = port_config.find(port_id);
        if (it == port_config.end()) return;
        
        const auto& config = it->second;
        
        // Remove VLAN tags for access ports
        if (!config.is_trunk) {
            frame.vlan_tags.clear();
        }
    }
    
    void configure_port(int port_id, uint16_t vlan_id, bool is_trunk,
                       const std::vector<uint16_t>& allowed = {}) {
        port_config[port_id] = {vlan_id, 0, is_trunk, allowed};
    }
};

// 4. STP (Spanning Tree Protocol) State Machine
class STPProcessor {
private:
    enum PortState { DISABLED, BLOCKING, LISTENING, LEARNING, FORWARDING };
    enum PortRole { ROOT_PORT, DESIGNATED_PORT, ALTERNATE_PORT, BACKUP_PORT };
    
    struct BridgeID {
        uint16_t priority;
        std::array<uint8_t, 6> mac;
        
        bool operator<(const BridgeID& other) const {
            if (priority != other.priority) return priority < other.priority;
            return mac < other.mac;
        }
    };
    
    struct BPDU {
        BridgeID root_id;
        uint32_t root_path_cost;
        BridgeID bridge_id;
        uint16_t port_id;
        uint16_t message_age;
        uint16_t max_age;
        uint16_t hello_time;
        uint16_t forward_delay;
    };
    
    struct PortInfo {
        PortState state;
        PortRole role;
        BPDU received_bpdu;
        std::chrono::steady_clock::time_point last_bpdu_time;
        uint32_t path_cost;
    };
    
    BridgeID bridge_id;
    BridgeID root_id;
    uint32_t root_path_cost;
    int root_port;
    std::unordered_map<int, PortInfo> ports;
    
public:
    STPProcessor(uint16_t priority, const std::array<uint8_t, 6>& mac)
        : bridge_id{priority, mac}, root_id{priority, mac}, root_path_cost(0), root_port(-1) {}
    
    void add_port(int port_id, uint32_t path_cost) {
        ports[port_id] = {BLOCKING, DESIGNATED_PORT, {}, 
                         std::chrono::steady_clock::now(), path_cost};
    }
    
    void receive_bpdu(int port_id, const BPDU& bpdu) {
        auto& port = ports[port_id];
        port.received_bpdu = bpdu;
        port.last_bpdu_time = std::chrono::steady_clock::now();
        
        // Process BPDU and update topology
        bool topology_changed = false;
        
        // Check if this is a better root
        if (bpdu.root_id < root_id) {
            root_id = bpdu.root_id;
            root_path_cost = bpdu.root_path_cost + port.path_cost;
            root_port = port_id;
            topology_changed = true;
        }
        
        if (topology_changed) {
            recalculate_port_roles();
            send_bpdus();
        }
    }
    
    bool should_forward(int port_id) const {
        auto it = ports.find(port_id);
        return it != ports.end() && 
               (it->second.state == FORWARDING || it->second.state == LEARNING);
    }
    
    void timer_tick() {
        auto now = std::chrono::steady_clock::now();
        bool changes = false;
        
        for (auto& [port_id, port] : ports) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                now - port.last_bpdu_time).count();
            
            // Age out old BPDUs
            if (age > 20) { // Max age
                if (port_id == root_port) {
                    // Lost root port, recalculate
                    recalculate_root();
                    changes = true;
                }
            }
            
            // State machine transitions
            update_port_state(port_id);
        }
        
        if (changes) {
            send_bpdus();
        }
    }
    
private:
    void recalculate_port_roles() {
        // Simplified role calculation
        for (auto& [port_id, port] : ports) {
            if (port_id == root_port) {
                port.role = ROOT_PORT;
            } else {
                port.role = DESIGNATED_PORT; // Simplified
            }
        }
    }
    
    void recalculate_root() {
        // Find new root path
        root_id = bridge_id;
        root_path_cost = 0;
        root_port = -1;
        
        for (const auto& [port_id, port] : ports) {
            if (port.received_bpdu.root_id < root_id) {
                root_id = port.received_bpdu.root_id;
                root_path_cost = port.received_bpdu.root_path_cost + port.path_cost;
                root_port = port_id;
            }
        }
    }
    
    void update_port_state(int port_id) {
        // Simplified state transitions
        auto& port = ports[port_id];
        if (port.role == ROOT_PORT || port.role == DESIGNATED_PORT) {
            port.state = FORWARDING;
        } else {
            port.state = BLOCKING;
        }
    }
    
    void send_bpdus() {
        // Send BPDUs on designated ports
    }
};

// 5. Multicast Group Management (IGMP/MLD)
class MulticastManager {
private:
    struct MulticastGroup {
        uint32_t group_addr;
        std::vector<int> member_ports;
        std::chrono::steady_clock::time_point last_query;
        bool has_querier;
    };
    
    std::unordered_map<uint32_t, MulticastGroup> groups;
    bool is_querier;
    
public:
    void join_group(uint32_t group_addr, int port_id) {
        auto& group = groups[group_addr];
        group.group_addr = group_addr;
        
        if (std::find(group.member_ports.begin(), group.member_ports.end(), port_id) 
            == group.member_ports.end()) {
            group.member_ports.push_back(port_id);
        }
    }
    
    void leave_group(uint32_t group_addr, int port_id) {
        auto it = groups.find(group_addr);
        if (it != groups.end()) {
            auto& members = it->second.member_ports;
            members.erase(std::remove(members.begin(), members.end(), port_id), 
                         members.end());
            
            if (members.empty()) {
                groups.erase(it);
            }
        }
    }
    
    std::vector<int> get_multicast_ports(uint32_t group_addr) const {
        auto it = groups.find(group_addr);
        return it != groups.end() ? it->second.member_ports : std::vector<int>{};
    }
    
    void send_query(uint32_t group_addr = 0) {
        // Send IGMP/MLD query
        // group_addr = 0 means general query
    }
    
    void age_groups() {
        auto now = std::chrono::steady_clock::now();
        for (auto it = groups.begin(); it != groups.end();) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                now - it->second.last_query).count();
            
            if (age > 260) { // Group timeout
                it = groups.erase(it);
            } else {
                ++it;
            }
        }
    }
};


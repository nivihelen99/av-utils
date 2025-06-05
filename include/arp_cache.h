#pragma once

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

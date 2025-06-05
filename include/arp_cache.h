#pragma once

#include <algorithm> // For std::find
#include <array>
#include <chrono>
#include <cstdio> // For fprintf, stderr
#include <queue>
#include <unordered_map>
#include <vector>

// Type alias for MAC addresses
using mac_addr_t = std::array<uint8_t, 6>;

/**
 * @brief Implements an ARP (Address Resolution Protocol) cache for IPv4.
 *
 * Manages mappings from IP addresses to MAC addresses, including features like
 * gratuitous ARP detection, proxy ARP, and fast failover with backup MACs.
 */
class ARPCache {
private:
    /**
     * @brief Defines the state of an ARP cache entry.
     */
    enum class ARPState {
        INCOMPLETE, /**< Address resolution is in progress; an ARP request has been sent. */
        REACHABLE,  /**< The MAC address has been recently confirmed as reachable. */
        STALE,      /**< Reachability is unknown (exceeded REACHABLE_TIME); will verify on next send. */
        PROBE,      /**< Actively sending ARP requests to verify a previously known MAC address. */
        DELAY       /**< A short period after STALE before sending the first probe. */
    };
    
    struct ARPEntry {
        mac_addr_t mac; /**< Primary MAC address. */
        ARPState state; /**< Current state of the ARP entry. */
        std::chrono::steady_clock::time_point timestamp; /**< Last time the entry was updated or confirmed. */
        int probe_count; /**< Number of probes sent for INCOMPLETE or PROBE states. */
        std::queue<std::vector<uint8_t>> pending_packets; /**< Queue of packets waiting for this ARP resolution. */
        std::vector<mac_addr_t> backup_macs; /**< List of backup MAC addresses for failover. */
    };
    
    std::unordered_map<uint32_t, ARPEntry> cache_; /**< The ARP cache, mapping IP to ARPEntry. */
    mac_addr_t device_mac_; /**< MAC address of this device (used for Proxy ARP). */

    struct ProxySubnet {
        uint32_t prefix;
        uint32_t mask;
    };
    std::vector<ProxySubnet> proxy_subnets_; /**< List of subnets for which Proxy ARP is enabled. */

    // Note: MAX_PROBES and REACHABLE_TIME moved to public section for external access (e.g., tests)

protected: // Changed to protected for testability
    /**
     * @brief Sends an ARP request for the given IP address.
     * Made virtual for test mocking. Base implementation is a placeholder.
     * @param ip The IP address to send an ARP request for.
     */
    virtual void send_arp_request(uint32_t ip) {
        // This is a placeholder. A real implementation would construct and send an ARP packet.
        // fprintf(stderr, "MockSend: ARP Request for IP %u\n", ip);
    }

    /**
     * @brief Logs an IP conflict event.
     * Made virtual for test mocking. Base implementation logs to stderr.
     * @param ip The conflicting IP address.
     * @param existing_mac The MAC address currently associated with the IP in the cache.
     * @param new_mac The new MAC address that caused the conflict.
     */
    virtual void log_ip_conflict(uint32_t ip, const mac_addr_t& existing_mac, const mac_addr_t& new_mac) {
        fprintf(stderr, "WARNING: IP conflict detected for IP %u. Existing MAC: %02x:%02x:%02x:%02x:%02x:%02x, New MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                ip,
                existing_mac[0], existing_mac[1], existing_mac[2], existing_mac[3], existing_mac[4], existing_mac[5],
                new_mac[0], new_mac[1], new_mac[2], new_mac[3], new_mac[4], new_mac[5]);
        // The "MAC updated" part of the original message is removed to make this specifically about conflicts.
        // If it's just an update for the same device, that's usually fine. This targets different devices claiming the IP.
    }

public:
    /**
     * @brief Constructs an ARPCache.
     * @param dev_mac The MAC address of the device this cache is running on.
     */
    explicit ARPCache(const mac_addr_t& dev_mac) : device_mac_(dev_mac) {}

    // Publicly accessible constants
    static constexpr int MAX_PROBES = 3; /**< Max number of ARP probes before considering a primary MAC failed. */
    static constexpr std::chrono::seconds REACHABLE_TIME{300}; /**< Time an entry remains REACHABLE before becoming STALE. */
    // Example if a probe timeout constant was needed publicly:
    // static constexpr std::chrono::seconds PROBE_RETRANSMIT_TIMEOUT{1};


    /**
     * @brief Adds a subnet configuration for Proxy ARP.
     * The device will respond with its own MAC for ARP requests targeting IPs in this subnet.
     * @param prefix The network prefix of the subnet.
     * @param mask The subnet mask.
     */
    void add_proxy_subnet(uint32_t prefix, uint32_t mask) {
        proxy_subnets_.push_back({prefix, mask});
    }

    /**
     * @brief Adds a backup MAC address for a given IP address.
     * Used for fast failover if the primary MAC becomes unresponsive.
     * @param ip The IP address for which to add a backup MAC.
     * @param backup_mac The backup MAC address.
     */
    void add_backup_mac(uint32_t ip, const mac_addr_t& backup_mac) {
        auto it = cache_.find(ip);
        if (it != cache_.end()) {
            if (std::find(it->second.backup_macs.begin(), it->second.backup_macs.end(), backup_mac) == it->second.backup_macs.end() && it->second.mac != backup_mac) {
                it->second.backup_macs.push_back(backup_mac);
            }
        }
    }

    /**
     * @brief Looks up the MAC address for a given IP address.
     * Handles proxy ARP and fast failover to backup MACs if the primary is STALE/PROBE/DELAY.
     * @param ip The IP address to look up.
     * @param mac_out Output parameter for the resolved MAC address.
     * @return True if a usable MAC address is found (REACHABLE, or DELAY/failover occurred), false otherwise (e.g. INCOMPLETE).
     */
    bool lookup(uint32_t ip, mac_addr_t& mac_out) {
        auto it = cache_.find(ip);
        if (it != cache_.end()) {
            ARPEntry& entry = it->second;
            if (entry.state == ARPState::REACHABLE) {
                mac_out = entry.mac;
                return true;
            } else if (entry.state == ARPState::STALE || entry.state == ARPState::PROBE || entry.state == ARPState::DELAY) {
                if (!entry.backup_macs.empty()) {
                    mac_addr_t old_primary_mac = entry.mac;
                    entry.mac = entry.backup_macs.front();
                    entry.backup_macs.erase(entry.backup_macs.begin());

                    bool old_mac_is_zero = true;
                    for(uint8_t val : old_primary_mac) if(val != 0) old_mac_is_zero = false;
                    if (!old_mac_is_zero && std::find(entry.backup_macs.begin(), entry.backup_macs.end(), old_primary_mac) == entry.backup_macs.end()) {
                        entry.backup_macs.push_back(old_primary_mac);
                    }

                    entry.state = ARPState::REACHABLE;
                    entry.timestamp = std::chrono::steady_clock::now();
                    entry.probe_count = 0;
                    mac_out = entry.mac;
                    // Log failover event (could be made virtual too if needed for testing)
                    fprintf(stderr, "INFO: Failover for IP %u. New MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            ip, entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);
                    return true;
                }
                mac_out = entry.mac;
                if (entry.state == ARPState::STALE) {
                    entry.state = ARPState::INCOMPLETE;
                    entry.probe_count = 0;
                    this->send_arp_request(ip); // Use this-> for virtual dispatch if ever needed, good practice
                }
                return (entry.state == ARPState::DELAY);
            }
        }
        
        if (it == cache_.end() || it->second.state == ARPState::STALE || it->second.state == ARPState::INCOMPLETE) {
            if (it == cache_.end() || it->second.state == ARPState::STALE) {
                for (const auto& subnet : proxy_subnets_) {
                    if ((ip & subnet.mask) == subnet.prefix) {
                        mac_out = device_mac_;
                        cache_[ip] = {device_mac_, ARPState::REACHABLE, std::chrono::steady_clock::now(), 0, {}, {}};
                        return true;
                    }
                }
            }

            if (it == cache_.end() || it->second.state == ARPState::INCOMPLETE) {
                 this->send_arp_request(ip);
                 if (it == cache_.end()) {
                     cache_[ip] = {{}, ARPState::INCOMPLETE, std::chrono::steady_clock::now(), 0, {}, {}};
                 } else {
                     it->second.timestamp = std::chrono::steady_clock::now();
                 }
            } else if (it->second.state == ARPState::STALE) {
                it->second.state = ARPState::INCOMPLETE;
                it->second.probe_count = 0;
                it->second.timestamp = std::chrono::steady_clock::now();
                this->send_arp_request(ip);
            }
        }
        return false;
    }
    
    /**
     * @brief Adds or updates an ARP entry.
     * If the IP exists with a different MAC, logs an IP conflict warning.
     * Preserves existing backup MACs if the entry is being updated.
     * @param ip The IP address of the entry.
     * @param new_mac The MAC address corresponding to the IP.
     */
    void add_entry(uint32_t ip, const mac_addr_t& new_mac) {
        auto it = cache_.find(ip);
        std::vector<mac_addr_t> existing_backups;

        if (it != cache_.end()) { // Entry exists
            if (it->second.mac != new_mac) {
                 // Call the virtual logging method for IP conflicts
                 this->log_ip_conflict(ip, it->second.mac, new_mac);
            }
            existing_backups = it->second.backup_macs;
        }

        cache_[ip] = {new_mac, ARPState::REACHABLE, std::chrono::steady_clock::now(), 0, {}, existing_backups};
        
        auto& entry = cache_[ip];
        while (!entry.pending_packets.empty()) {
            // TODO: Implement actual packet forwarding logic here
            entry.pending_packets.pop();
        }
    }
    
    /**
     * @brief Ages ARP cache entries, transitioning them through states.
     * Handles re-probing for INCOMPLETE/PROBE states and failover to backup MACs
     * if the primary MAC fails MAX_PROBES.
     */
    void age_entries() {
        auto now = std::chrono::steady_clock::now();
        for (auto it = cache_.begin(); it != cache_.end();) {
            ARPEntry& entry = it->second;
            auto age_duration = std::chrono::duration_cast<std::chrono::seconds>(now - entry.timestamp);
            
            bool entry_erased = false;
            switch (entry.state) {
                case ARPState::REACHABLE:
                    if (age_duration >= REACHABLE_TIME) {
                        entry.state = ARPState::STALE;
                        entry.timestamp = now;
                    }
                    break;
                case ARPState::STALE:
                    break;
                case ARPState::INCOMPLETE:
                case ARPState::PROBE:
                    if (age_duration >= std::chrono::seconds(1)) {
                        if (++entry.probe_count > MAX_PROBES) {
                            if (!entry.backup_macs.empty()) {
                                entry.mac = entry.backup_macs.front();
                                entry.backup_macs.erase(entry.backup_macs.begin());
                                entry.state = ARPState::REACHABLE;
                                entry.timestamp = now;
                                entry.probe_count = 0;
                                fprintf(stderr, "INFO: Primary MAC failed for IP %u. Switched to backup MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                                        it->first, entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);
                            } else {
                                it = cache_.erase(it);
                                entry_erased = true;
                            }
                        } else {
                            this->send_arp_request(it->first);
                            entry.timestamp = now;
                        }
                    }
                    break;
                case ARPState::DELAY:
                    break;
            }

            if (entry_erased) {
                continue;
            }
            ++it;
        }
    }
};

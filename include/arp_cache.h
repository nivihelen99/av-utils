#pragma once

#include <cstdio> // For fprintf, stderr
#include <array> // For std::array
#include <chrono> // For std::chrono
#include <queue> // For std::queue
#include <vector> // For std::vector
#include <unordered_map> // For std::unordered_map
// It's good practice to include iostream if you might use std::cerr or std::cout,
// but for fprintf to stderr, cstdio is sufficient.
// We'll keep the previous #include <iostream> comment from the earlier attempt
// in case it was there for a reason, but comment it out if not strictly needed.
// #include <iostream>

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
    enum ARPState {
        INCOMPLETE, /**< Address resolution is in progress; an ARP request has been sent. */
        REACHABLE,  /**< The MAC address has been recently confirmed as reachable. */
        STALE,      /**< Reachability is unknown (exceeded REACHABLE_TIME); will verify on next send. */
        PROBE,      /**< Actively sending ARP requests to verify a previously known MAC address. */
        DELAY       /**< A short period after STALE before sending the first probe. */
    };
    
    struct ARPEntry {
        std::array<uint8_t, 6> mac; /**< Primary MAC address. */
        ARPState state; /**< Current state of the ARP entry. */
        std::chrono::steady_clock::time_point timestamp; /**< Last time the entry was updated or confirmed. */
        int probe_count; /**< Number of probes sent for INCOMPLETE or PROBE states. */
        std::queue<std::vector<uint8_t>> pending_packets; /**< Queue of packets waiting for this ARP resolution. */
        std::vector<std::array<uint8_t, 6>> backup_macs; /**< List of backup MAC addresses for failover. */
    };
    
    std::unordered_map<uint32_t, ARPEntry> cache; /**< The ARP cache, mapping IP to ARPEntry. */
    std::array<uint8_t, 6> device_mac_; /**< MAC address of this device (used for Proxy ARP). */

    struct ProxySubnet {
        uint32_t prefix;
        uint32_t mask;
    };
    std::vector<ProxySubnet> proxy_subnets_; /**< List of subnets for which Proxy ARP is enabled. */

    static constexpr int MAX_PROBES = 3; /**< Max number of ARP probes before considering a primary MAC failed. */
    static constexpr int REACHABLE_TIME = 300; /**< Time in seconds an entry remains REACHABLE before becoming STALE. */
    
public:
    /**
     * @brief Constructs an ARPCache.
     * @param dev_mac The MAC address of the device this cache is running on.
     */
    ARPCache(const std::array<uint8_t, 6>& dev_mac) : device_mac_(dev_mac) {}

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
    void add_backup_mac(uint32_t ip, const std::array<uint8_t, 6>& backup_mac) {
        auto it = cache.find(ip);
        if (it != cache.end()) {
            // Avoid adding duplicate backup MACs
            if (std::find(it->second.backup_macs.begin(), it->second.backup_macs.end(), backup_mac) == it->second.backup_macs.end() && it->second.mac != backup_mac) {
                it->second.backup_macs.push_back(backup_mac);
            }
        }
        // If IP doesn't exist, backup MAC isn't added. Primary entry should exist first.
    }

    /**
     * @brief Looks up the MAC address for a given IP address.
     * Handles proxy ARP and fast failover to backup MACs if the primary is STALE/PROBE/DELAY.
     * @param ip The IP address to look up.
     * @param mac_out Output parameter for the resolved MAC address.
     * @return True if a usable MAC address is found (REACHABLE, or DELAY/failover occurred), false otherwise (e.g. INCOMPLETE).
     */
    bool lookup(uint32_t ip, std::array<uint8_t, 6>& mac_out) {
        auto it = cache.find(ip);
        if (it != cache.end()) {
            ARPEntry& entry = it->second;
            if (entry.state == REACHABLE) {
                mac_out = entry.mac;
                return true;
            } else if (entry.state == STALE || entry.state == PROBE || entry.state == DELAY) {
                // Primary MAC is not REACHABLE, try a backup if available.
                // This is a simplified fast-failover: if stale/probe, use backup immediately.
                // A more robust approach might probe the backup before using it.
                if (!entry.backup_macs.empty()) {
                    // Promote the first backup MAC
                    std::array<uint8_t, 6> old_primary_mac = entry.mac;
                    entry.mac = entry.backup_macs.front();
                    entry.backup_macs.erase(entry.backup_macs.begin()); // Remove promoted MAC from backups

                    // Demote old primary MAC to backup list if it's not zero and not already there
                    bool old_mac_is_zero = true;
                    for(uint8_t val : old_primary_mac) if(val != 0) old_mac_is_zero = false;

                    if (!old_mac_is_zero && std::find(entry.backup_macs.begin(), entry.backup_macs.end(), old_primary_mac) == entry.backup_macs.end()) {
                        entry.backup_macs.push_back(old_primary_mac);
                    }

                    entry.state = REACHABLE; // New primary MAC is assumed REACHABLE for now
                    entry.timestamp = std::chrono::steady_clock::now();
                    entry.probe_count = 0;
                    mac_out = entry.mac;
                    fprintf(stderr, "INFO: Failover for IP %u. New MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            ip, entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);
                    return true;
                }
                // No backups, fall through to standard STALE/PROBE handling (ARP request)
                mac_out = entry.mac; // Still return current MAC if no backups
                // The age_entries logic will handle sending probes for STALE/PROBE states.
                // If lookup needs to trigger probes directly:
                if (entry.state == STALE) { // If STALE, transition to INCOMPLETE to re-probe
                    entry.state = INCOMPLETE; // Or PROBE, depending on desired NUD behavior
                    entry.probe_count = 0;
                    send_arp_request(ip);
                }
                return (entry.state == DELAY); // Return true if in DELAY (MAC is usable but being probed)
                                               // Or false to indicate it's not reliably REACHABLE
            }
            // If INCOMPLETE, fall through to standard handling
        }
        
        // Standard handling for not found or INCOMPLETE, or STALE/PROBE without backups
        // (or if backup failover didn't make it REACHABLE immediately)
        if (it == cache.end() || it->second.state == STALE || it->second.state == INCOMPLETE) {
            // Check for proxy ARP first if not found or stale
            if (it == cache.end() || it->second.state == STALE) { // Only check proxy if truly not found or stale
                for (const auto& subnet : proxy_subnets_) {
                if ((ip & subnet.mask) == subnet.prefix) {
                    // IP matches a proxy subnet, return device's MAC
                    mac = device_mac_;
                    // It's common to add this to the cache as well, as if we resolved it.
                    // This avoids repeated proxy lookups for the same IP.
                    // However, the state should reflect it's a proxied entry,
                    // or handle it such that it doesn't interfere with actual learned entries
                    // if the real device later appears on the local network.
                    // For now, let's just return the MAC. A more advanced implementation
                    // might cache this differently.
                    // A simple approach is to add it as REACHABLE, similar to a normal ARP reply.
                    // add_entry(ip, device_mac_); // This could cause issues if add_entry triggers conflict detection with itself.
                                                // Let's manage the cache directly here for proxy.
                    cache[ip] = {device_mac_, REACHABLE, std::chrono::steady_clock::now(), 0, {}};
                    return true;
                }
            }

            // Not a proxy ARP target, proceed with normal ARP request
            // If entry exists (it was STALE), its state is already INCOMPLETE or handled by backup logic
            if (it == cache.end() || it->second.state == INCOMPLETE) { // if truly not found or became INCOMPLETE
                 send_arp_request(ip);
                 if (it == cache.end()) {
                     cache[ip] = {{}, INCOMPLETE, std::chrono::steady_clock::now(), 0, {}, {}}; // Init backup_macs
                 } else { // Was INCOMPLETE or became INCOMPLETE
                     it->second.timestamp = std::chrono::steady_clock::now(); // Update timestamp for new probe attempt
                 }
            } else if (it->second.state == STALE) { // If it was STALE and no backups, make it INCOMPLETE
                it->second.state = INCOMPLETE;
                it->second.probe_count = 0;
                it->second.timestamp = std::chrono::steady_clock::now();
                send_arp_request(ip);
            }
        }
        return false;
    }
    
    /**
     * @brief Adds or updates an ARP entry.
     * If the IP exists with a different MAC, logs an IP conflict warning.
     * Preserves existing backup MACs if the entry is being updated.
     * @param ip The IP address of the entry.
     * @param mac The MAC address corresponding to the IP.
     */
    void add_entry(uint32_t ip, const std::array<uint8_t, 6>& mac) {
        auto it = cache.find(ip);
        bool is_new_entry = (it == cache.end());
        std::vector<std::array<uint8_t, 6>> existing_backups;

        if (!is_new_entry) {
            if (it->second.mac != mac) {
                // This indicates an IP conflict if a different host sends a gratuitous ARP,
                // or it could be a legitimate MAC update for the same host.
                // The original requirement was to log a warning for different MACs.
                 fprintf(stderr, "INFO: MAC change for IP %u. Old MAC: %02x:%02x:%02x:%02x:%02x:%02x, New MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                        ip, it->second.mac[0],it->second.mac[1],it->second.mac[2],it->second.mac[3],it->second.mac[4],it->second.mac[5],
                        mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
                 fprintf(stderr, "WARNING: IP conflict detected for IP %u, or MAC updated.\n", ip);
            }
            existing_backups = it->second.backup_macs; // Preserve existing backups
        }

        // Add or update the entry, preserving backup MACs if entry existed.
        cache[ip] = {mac, REACHABLE, std::chrono::steady_clock::now(), 0, {}, existing_backups};
        
        // Send any pending packets for this IP
        auto& entry = cache[ip];
        while (!entry.pending_packets.empty()) {
            // TODO: Implement actual packet forwarding logic here
            // For example: forward_packet(entry.pending_packets.front());
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
        for (auto it = cache.begin(); it != cache.end();) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                now - it->second.timestamp).count();
            
            ARPEntry& entry = it->second; // Use reference for direct modification
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                now - entry.timestamp).count();

            bool entry_erased = false;
            switch (entry.state) {
                case REACHABLE:
                    if (age > REACHABLE_TIME) {
                        entry.state = STALE;
                        entry.timestamp = now; // Mark time it became STALE
                    }
                    break;
                case STALE:
                    // In lookup, if STALE, it's changed to INCOMPLETE to trigger probes.
                    // Or, if we want age_entries to manage this:
                    // entry.state = INCOMPLETE; // Or PROBE
                    // entry.probe_count = 0;
                    // send_arp_request(it->first);
                    // entry.timestamp = now;
                    break;
                case INCOMPLETE:
                case PROBE: // Treat PROBE similar to INCOMPLETE for re-probing
                    if (age > 1) { // Retransmission timeout (simplified)
                        if (++entry.probe_count > MAX_PROBES) {
                            // Primary MAC failed. Try to failover to a backup.
                            if (!entry.backup_macs.empty()) {
                                std::array<uint8_t, 6> failed_mac = entry.mac;
                                entry.mac = entry.backup_macs.front();
                                entry.backup_macs.erase(entry.backup_macs.begin());

                                // Optionally, add failed_mac to end of backup list or discard
                                // For now, discard failed_mac to prevent immediate re-selection if it's still bad.

                                entry.state = REACHABLE; // Assume backup is reachable (or send a probe first)
                                entry.timestamp = now;
                                entry.probe_count = 0;
                                fprintf(stderr, "INFO: Primary MAC failed for IP %u. Switched to backup MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                                        it->first, entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);
                            } else {
                                // No backups left, remove the entry
                                it = cache.erase(it);
                                entry_erased = true;
                            }
                        } else {
                            // Send another ARP request for the current primary MAC
                            send_arp_request(it->first);
                            entry.timestamp = now;
                        }
                    }
                    break;
                case DELAY: // Not explicitly handled here, PROBE is the active check state
                    // If DELAY implies waiting for a probe result, this logic is okay.
                    // If DELAY has its own timeout to transition to PROBE:
                    // if (age > DELAY_TIMEOUT) { entry.state = PROBE; entry.timestamp = now; entry.probe_count = 0; send_arp_request(it->first); }
                    break;
                default: // Should not happen
                    break;
            }

            if (entry_erased) {
                // it already advanced by cache.erase(it)
                continue;
            }
            ++it;
        }
    }
    
private:
    /**
     * @brief Sends an ARP request for the given IP address.
     * Placeholder for actual network packet sending logic.
     * @param ip The IP address to send an ARP request for.
     */
    void send_arp_request(uint32_t ip) {
        // This is a placeholder. A real implementation would construct and send an ARP packet.
        // It typically probes for the IP address, and the MAC in the cache entry (it->second.mac)
        // is the one being verified or resolved.
        // For testing or simulation, this might log or interact with a mock network layer.
        // fprintf(stderr, "MockSend: ARP Request for IP %u targeting MAC %02x:...\n", ip, cache[ip].mac[0]);
    }
};

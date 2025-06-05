#pragma once

#include <algorithm> // For std::find
#include <array>
#include <chrono>
#include <cstdio> // For fprintf, stderr
#include <list> // Required for std::list
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
public: // ARPState made public for test access
    /**
     * @brief Defines the state of an ARP cache entry.
     */
    enum class ARPState {
        INCOMPLETE, /**< Address resolution is in progress; an ARP request has been sent. */
        REACHABLE,  /**< The MAC address has been recently confirmed as reachable. */
        STALE,      /**< Reachability is unknown (exceeded REACHABLE_TIME); will verify on next send. */
        PROBE,      /**< Actively sending ARP requests to verify a previously known MAC address. */
        DELAY,       /**< A short period after STALE before sending the first probe. */
        FAILED      // New state
    };
    
    struct ARPEntry {
        mac_addr_t mac; /**< Primary MAC address. */
        ARPState state; /**< Current state of the ARP entry. */
        std::chrono::steady_clock::time_point timestamp; /**< Last time the entry was updated or confirmed. */
        int probe_count; /**< Number of probes sent for INCOMPLETE or PROBE states. */
        std::queue<std::vector<uint8_t>> pending_packets; /**< Queue of packets waiting for this ARP resolution. */
        std::vector<mac_addr_t> backup_macs; /**< List of backup MAC addresses for failover. */
        int backoff_exponent;
        uint8_t flap_count;
        std::chrono::steady_clock::time_point last_mac_update_time;
    };
    
    mac_addr_t device_mac_; /**< MAC address of this device (used for Proxy ARP). */

protected: // cache_ made protected for test access via MockARPCache
    std::unordered_map<uint32_t, ARPEntry> cache_; /**< The ARP cache, mapping IP to ARPEntry. */
    struct ProxySubnet {
        uint32_t prefix;
        uint32_t mask;
    };
    std::vector<ProxySubnet> proxy_subnets_; /**< List of subnets for which Proxy ARP is enabled. */
    std::chrono::seconds reachable_time_sec_;
    std::chrono::seconds stale_time_sec_;
    std::chrono::seconds probe_retransmit_interval_sec_;
    std::chrono::seconds max_probe_backoff_interval_sec_;
    std::chrono::seconds failed_entry_lifetime_sec_;
    std::chrono::seconds delay_duration_sec_;
    std::chrono::seconds flap_detection_window_sec_;
    int max_flaps_allowed_;
    size_t max_cache_size_;
    std::list<uint32_t> lru_tracker_;
    std::unordered_map<uint32_t, std::list<uint32_t>::iterator> ip_to_lru_iterator_;

protected: // promoteToMRU made protected for test access via MockARPCache
    void promoteToMRU(uint32_t ip) {
        if (ip_to_lru_iterator_.count(ip)) {
            lru_tracker_.erase(ip_to_lru_iterator_[ip]);
        }
        lru_tracker_.push_front(ip);
        ip_to_lru_iterator_[ip] = lru_tracker_.begin();
    }

    void removeFromLRUTracking(uint32_t ip) {
        if (ip_to_lru_iterator_.count(ip)) {
            lru_tracker_.erase(ip_to_lru_iterator_[ip]);
            ip_to_lru_iterator_.erase(ip);
        }
    }

    void evictLRUEntries() {
        if (max_cache_size_ == 0) return;
        while (cache_.size() > max_cache_size_) {
            bool evicted_one_entry = false;
            for (auto it_lru = lru_tracker_.rbegin(); it_lru != lru_tracker_.rend(); ++it_lru) {
                uint32_t candidate_ip = *it_lru;
                auto cache_it = cache_.find(candidate_ip);
                if (cache_it != cache_.end()) {
                    if (cache_it->second.state != ARPState::INCOMPLETE &&
                        cache_it->second.state != ARPState::PROBE) {
                        fprintf(stderr, "INFO: ARP Cache full. Evicting IP %u.\n", candidate_ip);
                        cache_.erase(cache_it);
                        removeFromLRUTracking(candidate_ip);
                        evicted_one_entry = true;
                        break;
                    }
                } else {
                    // IP in LRU but not cache: inconsistency. Clean LRU.
                    removeFromLRUTracking(candidate_ip);
                    evicted_one_entry = true;
                    break;
                }
            }
            if (!evicted_one_entry) {
                fprintf(stderr, "WARNING: ARP Cache over size, but no evictable entries.\n");
                break;
            }
        }
    }

protected:
    /**
     * @brief Sends an ARP request for the given IP address.
     * Made virtual for test mocking. Base implementation is a placeholder.
     * @param ip The IP address to send an ARP request for.
     */
    virtual void send_arp_request(uint32_t ip) {
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
    }

public:
    // Publicly accessible constants
    static constexpr int MAX_PROBES = 3; /**< Max number of ARP probes before considering a primary MAC failed. */

    /**
     * @brief Constructs an ARPCache.
     * @param dev_mac The MAC address of the device this cache is running on.
     * @param reachable_time Time an entry remains REACHABLE before becoming STALE.
     * @param stale_time Time an entry remains STALE before transitioning to PROBE.
     * @param probe_retransmit_interval Interval for re-probing INCOMPLETE/PROBE entries.
     * @param max_probe_backoff_interval Maximum interval for exponential backoff probing.
     * @param failed_entry_lifetime Lifetime for entries in the FAILED state.
     * @param delay_duration Duration for the DELAY state.
     * @param flap_detection_window Time window for detecting flaps.
     * @param max_flaps Maximum allowed flaps within the detection window before penalizing.
     * @param max_cache_size Maximum number of entries in the cache.
     */
    explicit ARPCache(const mac_addr_t& dev_mac,
                      std::chrono::seconds reachable_time = std::chrono::seconds(300),
                      std::chrono::seconds stale_time = std::chrono::seconds(30),
                      std::chrono::seconds probe_retransmit_interval = std::chrono::seconds(1),
                      std::chrono::seconds max_probe_backoff_interval = std::chrono::seconds(60),
                      std::chrono::seconds failed_entry_lifetime = std::chrono::seconds(20),
                      std::chrono::seconds delay_duration = std::chrono::seconds(5),
                      std::chrono::seconds flap_detection_window = std::chrono::seconds(10), // New
                      int max_flaps = 3,                                                    // New
                      size_t max_cache_size = 1024)
        : device_mac_(dev_mac),
          reachable_time_sec_(reachable_time),
          stale_time_sec_(stale_time),
          probe_retransmit_interval_sec_(probe_retransmit_interval),
          max_probe_backoff_interval_sec_(max_probe_backoff_interval),
          failed_entry_lifetime_sec_(failed_entry_lifetime),
          delay_duration_sec_(delay_duration),
          flap_detection_window_sec_(flap_detection_window), // Initialize
          max_flaps_allowed_(max_flaps),                     // Initialize
          max_cache_size_(max_cache_size) {}

    /**
     * @brief Adds a subnet configuration for Proxy ARP.
     * @param prefix The network prefix of the subnet.
     * @param mask The subnet mask.
     */
    void add_proxy_subnet(uint32_t prefix, uint32_t mask) {
        proxy_subnets_.push_back({prefix, mask});
    }

    /**
     * @brief Adds a backup MAC address for a given IP address.
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
     * @param ip The IP address to look up.
     * @param mac_out Output parameter for the resolved MAC address.
     * @return True if a usable MAC address is found, false otherwise.
     */
    bool lookup(uint32_t ip, mac_addr_t& mac_out) {
        auto current_time = std::chrono::steady_clock::now();
        auto it = cache_.find(ip);

        if (it != cache_.end()) {
            ARPEntry& entry = it->second;

            if (entry.state == ARPState::FAILED) { // Add this check first
                return false;
            }
            if (entry.state == ARPState::REACHABLE) {
                promoteToMRU(ip);
                mac_out = entry.mac;
                return true;
            }

            // Handle STALE, PROBE, DELAY states, prioritizing backup MACs
            if (entry.state == ARPState::STALE || entry.state == ARPState::PROBE || entry.state == ARPState::DELAY) {
                if (!entry.backup_macs.empty()) {
                    // Perform failover to backup MAC
                    mac_addr_t old_primary_mac = entry.mac;
                    entry.mac = entry.backup_macs.front();
                    entry.backup_macs.erase(entry.backup_macs.begin());
                    bool old_mac_is_zero = true;
                    for(uint8_t val : old_primary_mac) if(val != 0) old_mac_is_zero = false;
                    if (!old_mac_is_zero && std::find(entry.backup_macs.begin(), entry.backup_macs.end(), old_primary_mac) == entry.backup_macs.end()) {
                        entry.backup_macs.push_back(old_primary_mac);
                    }
                    entry.state = ARPState::REACHABLE;
                    entry.timestamp = current_time;
                    entry.probe_count = 0;
                    entry.backoff_exponent = 0;
                    promoteToMRU(ip);
                    mac_out = entry.mac;
                    fprintf(stderr, "INFO: Failover for IP %u. New MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            ip, entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);
                    return true;
                } else {
                    // No backup MACs available
                    if (entry.state == ARPState::STALE) {
                        // For STALE, we return the MAC but don't necessarily promote it as aggressively
                        // as a confirmed REACHABLE entry. Promotion happens on active confirmation.
                        mac_out = entry.mac; // Return the stale MAC address
                        // Probing for this stale entry will be handled by age_entries if stale_time_sec_ expires
                        return true;
                    } else { // entry.state is PROBE or DELAY
                        // In PROBE or DELAY without backup MACs, resolution is ongoing.
                        // No stable MAC to return.
                        return false;
                    }
                }
            }
            // If state is INCOMPLETE, it will fall through to logic below
        }

        // Entry not found OR entry is INCOMPLETE
        if (it == cache_.end() || (it != cache_.end() && it->second.state == ARPState::INCOMPLETE) ) {
            // Proxy ARP check
            for (const auto& subnet : proxy_subnets_) {
                if ((ip & subnet.mask) == subnet.prefix) {
                    mac_out = device_mac_;
                    if (it == cache_.end()) { // New entry for proxy ARP
                        cache_[ip] = {device_mac_, ARPState::REACHABLE, current_time, 0, {}, {}, 0, 0, current_time};
                    } else { // Entry was INCOMPLETE, now resolved by proxy ARP
                        it->second.mac = device_mac_;
                        it->second.state = ARPState::REACHABLE;
                        it->second.timestamp = current_time;
                        it->second.probe_count = 0;
                        it->second.backoff_exponent = 0;
                        it->second.flap_count = 0;
                        it->second.last_mac_update_time = current_time;
                    }
                    promoteToMRU(ip);
                    return true; // Proxy ARP success
                }
            }

            // Not a proxy ARP case. Send ARP request for INCOMPLETE or new entry.
            this->send_arp_request(ip);
            if (it == cache_.end()) { // Create new INCOMPLETE entry
                cache_[ip] = {{}, ARPState::INCOMPLETE, current_time, 0, {}, {}, 0, 0, {}};
                promoteToMRU(ip);
            } else { // Entry was already INCOMPLETE (it->second.state == ARPState::INCOMPLETE)
                it->second.timestamp = current_time; // Update timestamp for this new probe attempt initiated by lookup
                // Promote INCOMPLETE as well, as it's actively being worked on
                promoteToMRU(ip);
            }
            return false; // Resolution started or ongoing for INCOMPLETE
        }

        // Fallback, should ideally not be reached if all states are handled above
        return false;
    }
    
    /**
     * @brief Adds or updates an ARP entry.
     * @param ip The IP address of the entry.
     * @param new_mac The MAC address corresponding to the IP.
     */
    void add_entry(uint32_t ip, const mac_addr_t& new_mac) {
        auto it = cache_.find(ip);
        auto current_time = std::chrono::steady_clock::now();
        std::vector<mac_addr_t> existing_backups; // Keep existing backups

        if (it != cache_.end()) { // Entry exists
            ARPEntry& entry = it->second;
            existing_backups = entry.backup_macs; // Preserve backups

            if (entry.mac != new_mac) { // MAC address has changed
                this->log_ip_conflict(ip, entry.mac, new_mac); // Log the conflict

                if (current_time - entry.last_mac_update_time < flap_detection_window_sec_) {
                    if (entry.flap_count < 255) { // Prevent overflow
                        entry.flap_count++;
                    }
                } else {
                    entry.flap_count = 1; // Reset flap count if change is outside the window
                }
                entry.last_mac_update_time = current_time;

                entry.mac = new_mac; // Update to the new MAC
                entry.timestamp = current_time;
                entry.probe_count = 0;
                entry.backoff_exponent = 0;
                // entry.pending_packets queue should be cleared, which happens if we re-assign 'entry' or clear it.
                // If just modifying members, ensure pending_packets are handled if necessary.
                // The original code re-assigned cache_[ip], effectively clearing queue. Let's ensure that.
                while (!entry.pending_packets.empty()) entry.pending_packets.pop();


                if (entry.flap_count >= max_flaps_allowed_) {
                    entry.state = ARPState::STALE; // Penalize by setting to STALE
                    fprintf(stderr, "INFO: Flapping detected for IP %u (count %u). Setting to STALE with new MAC to force re-verify.\n", ip, entry.flap_count);
                } else {
                    entry.state = ARPState::REACHABLE;
                }
            } else { // MAC is the same, just refresh to REACHABLE
                entry.state = ARPState::REACHABLE;
                entry.timestamp = current_time;
                entry.probe_count = 0; // Reset probe activity trackers
                entry.backoff_exponent = 0;
                // Optionally reset flap_count if MAC is same and confirmed:
                // entry.flap_count = 0;
                // Resetting flap_count here means any prior flaps are forgiven on a successful re-confirmation of the same MAC.
                // If we want flaps to only decay over time (i.e., only reset if last_mac_update_time is old), then don't reset flap_count here.
                // Current requirement implies reset only if outside window or penalty.
                // For a non-changing MAC, it makes sense to reset flap_count to 0 as it's stable.
                entry.flap_count = 0; // Explicitly reset on same MAC confirmation
                entry.last_mac_update_time = current_time; // Also update time to solidify this state
            }
        } else { // New entry
            // Initialize ARPEntry including new flap fields
            // ARPEntry { mac, state, timestamp, probe_count, pending_packets_q, backup_macs_vec, backoff_exp, flap_cnt, last_mac_update_t }
            std::queue<std::vector<uint8_t>> no_packets; // Define empty queue
            // existing_backups is already empty if new entry
            cache_[ip] = {new_mac, ARPState::REACHABLE, current_time, 0, no_packets, existing_backups, 0, 0, current_time};
            // For a truly new entry, last_mac_update_time could be 'epoch' or current_time. Current_time is fine.
        }

        promoteToMRU(ip);
        evictLRUEntries();
    }
    
    /**
     * @brief Ages ARP cache entries using the current system time.
     */
    void age_entries() {
        age_entries(std::chrono::steady_clock::now());
    }

    /**
     * @brief Ages ARP cache entries using a specific time point.
     * @param current_time The time point to consider as "now" for aging calculations.
     */
    void age_entries(std::chrono::steady_clock::time_point current_time) {
        for (auto it = cache_.begin(); it != cache_.end();) {
            ARPEntry& entry = it->second;
            auto age_duration = std::chrono::duration_cast<std::chrono::seconds>(current_time - entry.timestamp);
            
            bool entry_erased = false;
            switch (entry.state) {
                case ARPState::REACHABLE: { // Add braces for scope
                    // auto age_duration = ... (already calculated before switch)
                    constexpr double REFRESH_THRESHOLD_FACTOR = 0.9; // Define inside the case block
                    auto refresh_trigger_duration = std::chrono::seconds(static_cast<std::chrono::seconds::rep>(reachable_time_sec_.count() * REFRESH_THRESHOLD_FACTOR));

                    if (age_duration >= refresh_trigger_duration && age_duration < reachable_time_sec_) {
                        // Proactive refresh condition met
                        entry.state = ARPState::PROBE;
                        entry.timestamp = current_time;
                        entry.probe_count = 0;
                        entry.backoff_exponent = 0;
                        promoteToMRU(it->first); // Promote as it becomes active for probing
                        this->send_arp_request(it->first);
                        fprintf(stderr, "INFO: Proactive ARP refresh for IP %u.\n", it->first);
                    } else if (age_duration >= reachable_time_sec_) {
                        // Standard transition to STALE if refresh window was missed or this is later
                        entry.state = ARPState::STALE;
                        entry.timestamp = current_time;
                        // No MRU promotion for STALE here, lookup would handle it if accessed.
                        fprintf(stderr, "INFO: ARP entry for IP %u became STALE.\n", it->first);
                    }
                    break;
                } // Close scope for case REACHABLE
                case ARPState::STALE:
                    if (age_duration >= stale_time_sec_) {
                        entry.state = ARPState::PROBE;
                        entry.timestamp = current_time;
                        entry.probe_count = 0;
                        entry.backoff_exponent = 0;
                        entry.flap_count = 0; // Reset flap count
                        entry.last_mac_update_time = std::chrono::steady_clock::time_point{}; // Reset time
                        promoteToMRU(it->first); // Promote as it becomes active for probing
                        this->send_arp_request(it->first);
                    }
                    break;
                case ARPState::INCOMPLETE:
                    // [[fallthrough]]; // Optional C++17 attribute to denote intentional fallthrough
                case ARPState::PROBE: { // Add braces for scope
                    long long interval_val_s = probe_retransmit_interval_sec_.count();
                    if (entry.backoff_exponent > 0) {
                        int current_exp = (entry.backoff_exponent > 30) ? 30 : entry.backoff_exponent;
                        interval_val_s *= (static_cast<long long>(1) << current_exp);
                    }
                    // Corrected std::min usage
                    interval_val_s = std::min(interval_val_s, static_cast<long long>(max_probe_backoff_interval_sec_.count()));
                    std::chrono::seconds current_required_wait(interval_val_s);

                    if (age_duration >= current_required_wait) {
                        entry.probe_count++;
                        if (entry.probe_count > MAX_PROBES) {
                            if (!entry.backup_macs.empty()) {
                                // ... (failover to backup logic as before, including promoteToMRU) ...
                                entry.mac = entry.backup_macs.front();
                                entry.backup_macs.erase(entry.backup_macs.begin());
                                entry.state = ARPState::REACHABLE;
                                entry.timestamp = current_time;
                                entry.probe_count = 0;
                                entry.backoff_exponent = 0;
                                entry.flap_count = 0; // Reset flap count on successful failover
                                entry.last_mac_update_time = current_time; // Update time for new MAC
                                promoteToMRU(it->first); // Promote on becoming REACHABLE
                                fprintf(stderr, "INFO: Primary MAC failed for IP %u. Switched to backup MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                                        it->first, entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);
                            } else {
                                // ... (transition to FAILED logic as before) ...
                                entry.state = ARPState::FAILED;
                                entry.timestamp = current_time;
                                entry.probe_count = 0;
                                entry.backoff_exponent = 0;
                                entry.flap_count = 0; // Reset flap count when resolution fails
                                entry.last_mac_update_time = std::chrono::steady_clock::time_point{}; // Reset time
                                // No MRU promotion for FAILED
                                fprintf(stderr, "INFO: IP %u resolution failed, entry marked FAILED.\n", it->first);
                            }
                        } else {
                            // ... (send another probe logic as before) ...
                            this->send_arp_request(it->first);
                            entry.timestamp = current_time;
                            if (entry.backoff_exponent < 30) {
                                 entry.backoff_exponent++;
                            }
                            // No MRU promotion just for re-probing, only when state definitively changes to active/reachable.
                        }
                    }
                    break;
                } // Close scope for case PROBE (and INCOMPLETE due to fallthrough)
                case ARPState::DELAY:
                    // age_duration is calculated from entry.timestamp, which should be set when entry enters DELAY state.
                    if (age_duration >= delay_duration_sec_) {
                        entry.state = ARPState::PROBE;
                        entry.timestamp = current_time;     // Mark start of probing
                        entry.probe_count = 0;              // Reset for this new probe cycle
                        entry.backoff_exponent = 0;         // Reset backoff
                        entry.flap_count = 0;               // Reset flap count
                        entry.last_mac_update_time = std::chrono::steady_clock::time_point{}; // Reset time
                        // Also promote to MRU as it's becoming active for probing
                        promoteToMRU(it->first);
                        this->send_arp_request(it->first);
                        fprintf(stderr, "INFO: ARP Entry for IP %u transitioning DELAY -> PROBE.\n", it->first);
                    }
                    break;
                case ARPState::FAILED:
                    if (age_duration >= failed_entry_lifetime_sec_) {
                        uint32_t purged_ip = it->first;
                        it = cache_.erase(it);
                        removeFromLRUTracking(purged_ip);
                        entry_erased = true;
                        fprintf(stderr, "INFO: Purged FAILED entry for IP %u after lifetime.\n", purged_ip);
                    }
                    break;
            }

            if (entry_erased) {
                continue;
            }
            ++it;
        }
    }

    /**
     * @brief Handles a link-down event by purging all entries from the cache.
     */
    void handle_link_down() {
        cache_.clear(); // Removes all entries from the unordered_map
        lru_tracker_.clear();
        ip_to_lru_iterator_.clear();
        fprintf(stderr, "INFO: ARP cache purged due to link-down event, including LRU tracking.\n");
    }
};

#pragma once

#include <algorithm> // For std::find
#include <array>
#include <chrono>
#include <cstdio> // For fprintf, stderr
#include <list> // Required for std::list
#include <queue>
#include <unordered_map>
#include <unordered_set> // Required for std::unordered_set
#include <vector>
#include <string> // For std::to_string in potential logging helpers
#include <functional> // Required for std::hash

// Type alias for MAC addresses
using mac_addr_t = std::array<uint8_t, 6>;

// Custom hash function for mac_addr_t
struct MacAddrHash {
    std::size_t operator()(const mac_addr_t& mac) const {
        std::size_t hash_value = 0;
        std::hash<uint8_t> hasher;
        // Simple hash combination. Could be replaced with a more robust one if needed.
        for (uint8_t byte : mac) {
            hash_value = hash_value ^ (hasher(byte) << 1);
        }
        return hash_value;
    }
};

/**
 * @brief Defines modes for general ARP rate limiting.
 */
enum class ArpRateLimitMode {
    DISABLED,         /**< No rate limiting is applied. */
    PER_SOURCE_MAC,   /**< Rate limit based on the source MAC address of ARP packets. */
    PER_SOURCE_IP     /**< Rate limit based on the source IP address in ARP packets. */
    // PER_SOURCE_MAC_AND_IP // Future: Could combine
};

/**
 * @brief Defines the policy for handling ARP conflicts.
 */
enum class ConflictPolicy {
    LOG_ONLY,        /**< Log the conflict but take no other action. */
    DROP_NEW,        /**< Ignore the new ARP information, keeping the existing entry. */
    UPDATE_EXISTING, /**< Update the existing entry with the new information (Default). */
    ALERT_SYSTEM     /**< Log the conflict and send an alert to a system management entity. */
};

/**
 * @brief Defines the policy for handling gratuitous ARP packets.
 */
enum class GratuitousArpPolicy {
    PROCESS,                /**< Process the gratuitous ARP as a normal ARP packet (Default). */
    LOG_AND_PROCESS,        /**< Log the gratuitous ARP and then process it normally. */
    RATE_LIMIT_AND_PROCESS, /**< Process gratuitous ARPs, but apply rate limiting. */
    DROP_IF_CONFLICT        /**< Drop the gratuitous ARP if it conflicts with an existing entry. */
};

/**
 * @brief Distinguishes the type of ARP packet being processed.
 */
enum class ARPPacketType {
    UNKNOWN,                /**< Type not determined or not specified by the caller. */
    REPLY,                  /**< ARP Reply, typically in response to a request. */
    GRATUITOUS_ANNOUNCEMENT /**< Unsolicited ARP (e.g., on IP change, startup, or failover). */
};

/**
 * @brief Implements an ARP (Address Resolution Protocol) cache for IPv4.
 *
 * Manages mappings from IP addresses to MAC addresses.
 * Includes features like gratuitous ARP detection, proxy ARP, fast failover with backup MACs,
 * and various ARP security mechanisms such as interface trust, static ARP entries,
 * known MAC validation, DHCP validation enforcement, and general ARP rate limiting.
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
        uint32_t interface_id; // New field for interface awareness
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

    // Policies
    ConflictPolicy conflict_policy_;
    GratuitousArpPolicy gratuitous_arp_policy_;

    // Gratuitous ARP rate limiting
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> gratuitous_arp_last_seen_;
    std::chrono::milliseconds gratuitous_arp_min_interval_ms_;

    // Per-interface Proxy ARP control
    std::unordered_map<uint32_t, bool> interface_proxy_arp_enabled_;
    std::unordered_map<uint32_t, mac_addr_t> interface_macs_;

    // New data structures for trusted ports, static ARP, and rate limiting
    std::unordered_map<uint32_t, bool> interface_trust_status_; // For trusted/untrusted ports
    std::unordered_map<uint32_t, mac_addr_t> static_arp_entries_; /**< Static ARP entries (IP to MAC). */
    std::unordered_map<uint64_t, std::pair<int, std::chrono::steady_clock::time_point>> arp_rate_limit_counters_; /**< Counters for general ARP rate limiting. Key can be MAC or IP. */

    bool default_interface_trust_status_; /**< Default trust status for interfaces not explicitly set. */

    // Data structures for known MACs enforcement
    std::unordered_map<uint32_t, std::unordered_set<mac_addr_t, MacAddrHash>> known_macs_per_interface_; /**< List of known MACs allowed per interface, using custom hash. */
    std::unordered_map<uint32_t, bool> enforce_known_macs_status_; /**< Per-interface enforcement status for known MACs. */
    bool default_enforce_known_macs_; /**< Default policy for enforcing known MACs on interfaces. */

    // DHCP Validation Enforcement
    bool enforce_dhcp_validation_; /**< Global policy for enforcing DHCP validation. */

    // General ARP Rate Limiting
    ArpRateLimitMode general_arp_rate_limit_mode_; /**< Current mode for general ARP rate limiting. */
    int general_arp_rate_limit_count_;             /**< Max packets allowed by general rate limiter in an interval. */
    std::chrono::seconds general_arp_rate_limit_interval_; /**< Time interval for the general rate limiter. */


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
                        if (gratuitous_arp_last_seen_.count(candidate_ip)) {
                            gratuitous_arp_last_seen_.erase(candidate_ip);
                        }
                        removeFromLRUTracking(candidate_ip);
                        evicted_one_entry = true;
                        break;
                    }
                } else {
                    // IP in LRU but not cache: inconsistency. Clean LRU.
                    if (gratuitous_arp_last_seen_.count(candidate_ip)) { // Also try to clean from GARP tracking if present
                        gratuitous_arp_last_seen_.erase(candidate_ip);
                    }
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

    /**
     * @brief Triggers a system alert for an IP conflict.
     * Default implementation logs to stderr. Override for custom alert mechanisms.
     * @param ip The conflicting IP address.
     * @param existing_mac The MAC address currently associated with the IP in the cache.
     * @param new_mac The new MAC address that caused the conflict.
     */
    virtual void trigger_alert(uint32_t ip, const mac_addr_t& existing_mac, const mac_addr_t& new_mac) {
        fprintf(stderr, "ALERT: IP Conflict for IP %u. Existing MAC: %02x:%02x:%02x:%02x:%02x:%02x, New MAC: %02x:%02x:%02x:%02x:%02x:%02x. System alert action should be taken.\n",
                ip,
                existing_mac[0], existing_mac[1], existing_mac[2], existing_mac[3], existing_mac[4], existing_mac[5],
                new_mac[0], new_mac[1], new_mac[2], new_mac[3], new_mac[4], new_mac[5]);
    }

    /**
     * @brief (Placeholder) Checks if an IP-MAC mapping is considered valid by an external
     *        DHCP snooping mechanism or IP source guard.
     * @param ip The IP address.
     * @param mac The MAC address.
     * @return True if the mapping is considered valid or if no validation is performed,
     *         false if the mapping is explicitly considered invalid.
     */
    virtual bool is_ip_mac_dhcp_validated(uint32_t ip, const mac_addr_t& mac) {
        // Default implementation: assumes valid or no validation.
        // Users can override this to integrate with a DHCP snooping database.
        (void)ip; // Suppress unused parameter warning
        (void)mac; // Suppress unused parameter warning
        return true;
    }

    /**
     * @brief (Placeholder) Checks if the given IP address is considered routable
     *        by the device's routing table.
     * @param ip_address The IP address to check.
     * @return True if the IP is routable or if no check is performed (default).
     *         False if the IP is explicitly considered unroutable.
     */
    virtual bool is_ip_routable(uint32_t ip_address) {
        (void)ip_address; // Suppress unused parameter warning in default implementation
        return true;      // Default: assume routable or no validation performed
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
     * @param conflict_pol Policy for handling ARP conflicts.
     * @param garp_pol Policy for handling gratuitous ARP packets.
     * @param gratuitous_arp_min_interval Minimum interval between gratuitous ARPs for the same IP.
     * @param default_interface_trust_status Default trust status for interfaces.
     * @param default_enforce_known_macs Default policy for enforcing known MACs on interfaces. If true, interfaces by default will enforce known MAC list.
     * @param enforce_dhcp_validation Global policy for enforcing DHCP validation. If true, ARP entries failing DHCP snooping validation will be dropped.
     * @param general_arp_rate_limit_mode Mode for general ARP rate limiting (e.g., disabled, per source MAC, per source IP).
     * @param general_arp_rate_limit_count Maximum number of packets allowed by the general rate limiter within its interval.
     * @param general_arp_rate_limit_interval The time interval for the general rate limiter.
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
                      size_t max_cache_size = 1024,
                      ConflictPolicy conflict_pol = ConflictPolicy::UPDATE_EXISTING,
                      GratuitousArpPolicy garp_pol = GratuitousArpPolicy::PROCESS,
                      std::chrono::milliseconds gratuitous_arp_min_interval = std::chrono::milliseconds(1000),
                      bool default_interface_trust_status = false,
                      bool default_enforce_known_macs = false,
                      bool enforce_dhcp_validation = false,
                      ArpRateLimitMode general_arp_rate_limit_mode = ArpRateLimitMode::DISABLED,
                      int general_arp_rate_limit_count = 5,
                      std::chrono::seconds general_arp_rate_limit_interval = std::chrono::seconds(1))
        : device_mac_(dev_mac),
          reachable_time_sec_(reachable_time),
          stale_time_sec_(stale_time),
          probe_retransmit_interval_sec_(probe_retransmit_interval),
          max_probe_backoff_interval_sec_(max_probe_backoff_interval),
          failed_entry_lifetime_sec_(failed_entry_lifetime),
          delay_duration_sec_(delay_duration),
          flap_detection_window_sec_(flap_detection_window), // Initialize
          max_flaps_allowed_(max_flaps),                     // Initialize
          max_cache_size_(max_cache_size),
          conflict_policy_(conflict_pol),
          gratuitous_arp_policy_(garp_pol),
          gratuitous_arp_min_interval_ms_(gratuitous_arp_min_interval),
          default_interface_trust_status_(default_interface_trust_status),
          default_enforce_known_macs_(default_enforce_known_macs),
          enforce_dhcp_validation_(enforce_dhcp_validation),
          general_arp_rate_limit_mode_(general_arp_rate_limit_mode),
          general_arp_rate_limit_count_(general_arp_rate_limit_count),
          general_arp_rate_limit_interval_(general_arp_rate_limit_interval) {}

    /**
     * @brief Adds a subnet configuration for Proxy ARP.
     * @param prefix The network prefix of the subnet.
     * @param mask The subnet mask.
     * @param interface_id The identifier of the interface associated with this proxy ARP subnet.
     */
    void add_proxy_subnet(uint32_t prefix, uint32_t mask, uint32_t interface_id) {
        // Optional: Check for duplicate configurations if necessary before adding
        proxy_subnets_.push_back({prefix, mask, interface_id});
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
        // Check static ARP entries first
        if (get_static_arp_entry(ip, mac_out)) {
            // Static entries are always considered valid and reachable.
            // If it's also in the dynamic cache, ensure it's promoted to reflect its use.
            if (cache_.count(ip)) {
                promoteToMRU(ip);
            }
            return true;
        }

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

    // --- Static ARP Management ---
    /**
     * @brief Adds or updates a static ARP entry.
     * These entries are permanent, take precedence over dynamic entries in lookups,
     * and ARP packets conflicting with static entries may be dropped.
     * @param ip The IP address for the static entry.
     * @param mac The MAC address for the static entry.
     */
    void add_static_arp_entry(uint32_t ip, const mac_addr_t& mac);

    /**
     * @brief Removes a static ARP entry.
     * @param ip The IP address of the static entry to remove.
     */
    void remove_static_arp_entry(uint32_t ip);

    /**
     * @brief Retrieves a static ARP entry.
     * @param ip The IP address to look up in static entries.
     * @param mac_out Output parameter for the MAC address if found.
     * @return True if a static entry exists for the IP, false otherwise.
     */
    bool get_static_arp_entry(uint32_t ip, mac_addr_t& mac_out) const;

    // --- Configuration Setters ---

    /**
     * @brief Sets the general ARP rate limiting configuration.
     * When enabled, incoming ARP packets are rate-limited based on the specified mode,
     * count, and interval to prevent ARP storms or excessive processing.
     * Changing the configuration resets any existing rate limit counters.
     * @param mode The rate limiting mode (e.g., DISABLED, PER_SOURCE_MAC, PER_SOURCE_IP).
     * @param count The number of packets allowed from a source (MAC or IP) within the interval.
     * @param interval The time interval over which the packet count is measured.
     */
    void set_general_arp_rate_limit_config(ArpRateLimitMode mode, int count, std::chrono::seconds interval);

    /**
     * @brief Sets whether to enforce DHCP validation for ARP entries.
     * If enabled, ARP packets mapping an IP to a MAC will be checked against a
     * (typically external, via virtual method `is_ip_mac_dhcp_validated`) DHCP snooping database.
     * If validation fails and enforcement is on, the packet is dropped.
     * @param enforce True to enable enforcement, false to disable.
     */
    void set_enforce_dhcp_validation(bool enforce);

    /**
     * @brief Adds a MAC address to the list of known (allowed) MACs for a specific interface.
     * Used in conjunction with `set_enforce_known_macs_on_interface`.
     * @param interface_id The identifier of the interface.
     * @param mac The MAC address to add to the known list.
     */
    void add_known_mac(uint32_t interface_id, const mac_addr_t& mac);

    /**
     * @brief Removes a MAC address from the list of known MACs for a specific interface.
     * @param interface_id The identifier of the interface.
     * @param mac The MAC address to remove from the known list.
     */
    void remove_known_mac(uint32_t interface_id, const mac_addr_t& mac);

    /**
     * @brief Checks if a MAC address is in the known list for a specific interface.
     * @param interface_id The identifier of the interface.
     * @param mac The MAC address to check.
     * @return True if the MAC is known on the interface, false otherwise.
     */
    bool is_mac_known_on_interface(uint32_t interface_id, const mac_addr_t& mac) const;

    /**
     * @brief Enables or disables source MAC validation (known MAC list) for a given interface.
     * If enabled, only ARP packets from MAC addresses registered via `add_known_mac`
     * for that interface will be processed. Others will be dropped.
     * @param interface_id The identifier of the interface.
     * @param enforce True to enable enforcement, false to disable.
     */
    void set_enforce_known_macs_on_interface(uint32_t interface_id, bool enforce);

    /**
     * @brief Gets the source MAC validation enforcement status for a given interface.
     * @param interface_id The identifier of the interface.
     * @return True if enforcement is enabled for the interface, false otherwise.
     *         Returns the default enforcement policy if not explicitly set for the interface.
     */
    bool get_enforce_known_macs_on_interface(uint32_t interface_id) const;

    /**
     * @brief Sets the policy for handling IP address conflicts.
     * @param policy The new conflict policy.
     */
    void set_conflict_policy(ConflictPolicy policy) {
        conflict_policy_ = policy;
    }

    /**
     * @brief Sets the policy for handling gratuitous ARP packets.
     * @param policy The new gratuitous ARP policy.
     */
    void set_gratuitous_arp_policy(GratuitousArpPolicy policy) {
        gratuitous_arp_policy_ = policy;
    }

    /**
     * @brief Sets the minimum interval between processing gratuitous ARP packets for the same IP.
     * @param interval The minimum interval. A value of zero disables rate limiting for new GARPs.
     */
    void set_gratuitous_arp_min_interval(std::chrono::milliseconds interval) {
        gratuitous_arp_min_interval_ms_ = interval;
    }

    /**
     * @brief Updates the device MAC address used for Proxy ARP.
     * @param dev_mac The new device MAC address.
     */
    void set_device_mac(const mac_addr_t& dev_mac) {
        device_mac_ = dev_mac;
    }

    /** @brief Sets the time an entry remains REACHABLE before becoming STALE. */
    void set_reachable_time(std::chrono::seconds time) { reachable_time_sec_ = time; }

    /** @brief Sets the time an entry remains STALE before transitioning to PROBE (or DELAY if configured). */
    void set_stale_time(std::chrono::seconds time) { stale_time_sec_ = time; }

    /** @brief Sets the base interval for re-probing INCOMPLETE/PROBE entries. */
    void set_probe_retransmit_interval(std::chrono::seconds interval) { probe_retransmit_interval_sec_ = interval; }

    /** @brief Sets the maximum interval for exponential backoff probing. */
    void set_max_probe_backoff_interval(std::chrono::seconds interval) { max_probe_backoff_interval_sec_ = interval; }

    /** @brief Sets the lifetime for entries in the FAILED state before being purged. */
    void set_failed_entry_lifetime(std::chrono::seconds lifetime) { failed_entry_lifetime_sec_ = lifetime; }

    /** @brief Sets the duration for the DELAY state before transitioning to PROBE. */
    void set_delay_duration(std::chrono::seconds duration) { delay_duration_sec_ = duration; }

    /** @brief Sets the time window for detecting MAC flaps. */
    void set_flap_detection_window(std::chrono::seconds window) { flap_detection_window_sec_ = window; }

    /** @brief Sets the maximum allowed MAC flaps within the detection window before penalizing. */
    void set_max_flaps_allowed(int max_flaps) { max_flaps_allowed_ = max_flaps; }

    /**
     * @brief Sets the maximum number of entries in the cache.
     * If the new size is smaller than the current number of entries, LRU entries will be evicted.
     */
    void set_max_cache_size(size_t size) {
        max_cache_size_ = size;
        if (size > 0 && cache_.size() > max_cache_size_) {
            evictLRUEntries();
        }
    }

    // --- Main ARP Cache Operations ---

    /**
     * @brief Sets the trust status for a specific network interface.
     * ARP packets received on untrusted interfaces might be subject to stricter validation rules
     * or different processing logic, though current direct enforcement is via other specific features
     * like known MAC lists or DHCP validation. This status serves as a classification.
     * @param interface_id The identifier of the interface.
     * @param is_trusted True if the interface is considered trusted, false otherwise.
     */
    void set_interface_trust_status(uint32_t interface_id, bool is_trusted);

    /**
     * @brief Gets the trust status for a specific interface.
     * @param interface_id The identifier of the interface.
     * @return True if the interface is trusted, false if untrusted.
     *         Returns the default trust status if not explicitly set for the interface.
     */
    bool get_interface_trust_status(uint32_t interface_id) const;

    /**
     * @brief Sets the MAC address for a specific interface.
     * This MAC will be used for Proxy ARP replies on this interface.
     * @param interface_id The identifier of the interface.
     * @param mac The MAC address for this interface.
     */
    void set_interface_mac(uint32_t interface_id, const mac_addr_t& mac) {
        interface_macs_[interface_id] = mac;
    }

    /**
     * @brief Enables Proxy ARP functionality on a specific interface.
     * @param interface_id The identifier of the interface to enable Proxy ARP on.
     */
    void enable_proxy_arp_on_interface(uint32_t interface_id) {
        interface_proxy_arp_enabled_[interface_id] = true;
    }

    /**
     * @brief Disables Proxy ARP functionality on a specific interface.
     * @param interface_id The identifier of the interface to disable Proxy ARP on.
     */
    void disable_proxy_arp_on_interface(uint32_t interface_id) {
        interface_proxy_arp_enabled_[interface_id] = false;
    }

    /**
     * @brief Checks if Proxy ARP is currently enabled on a specific interface.
     * @param interface_id The identifier of the interface.
     * @return True if Proxy ARP is enabled, false otherwise (including if never explicitly set).
     */
    bool is_proxy_arp_enabled_on_interface(uint32_t interface_id) const {
        auto it = interface_proxy_arp_enabled_.find(interface_id);
        if (it != interface_proxy_arp_enabled_.end()) {
            return it->second; // Return the explicitly set state
        }
        return false; // Default to disabled if not found in map
    }

    /**
     * @brief Attempts to resolve an IP address using Proxy ARP configurations.
     * @param target_ip The IP address to resolve.
     * @param request_interface_id The identifier of the interface on which the ARP request was received.
     * @param mac_out Output parameter for the MAC address if proxy ARP is successful.
     * @return True if a proxy ARP response should be sent, false otherwise.
     */
    bool resolve_proxy_arp(uint32_t target_ip, uint32_t request_interface_id, mac_addr_t& mac_out) {
        // 1. Check if proxy ARP is enabled on the requesting interface
        if (!this->is_proxy_arp_enabled_on_interface(request_interface_id)) {
            // Optional: Log if needed, but often desirable to be silent if it's just disabled.
            // fprintf(stderr, "INFO: Proxy ARP lookup for IP %u on interface %u skipped: Proxy ARP disabled on interface.\n", target_ip, request_interface_id);
            return false; // Proxy ARP is disabled on this interface
        }

        // 2. Iterate through proxy_subnets_
        for (const auto& config : proxy_subnets_) {
            if (config.interface_id == request_interface_id) {
                if ((target_ip & config.mask) == config.prefix) { // Check if IP matches the subnet
                    // --- Routing Table Integration Hook ---
                    if (!this->is_ip_routable(target_ip)) {
                        // Optional: Log this event
                        fprintf(stderr, "INFO: Proxy ARP for IP %u on interface %u denied: IP not routable by system policy.\n", target_ip, request_interface_id);
                        return false; // Do not proxy if not routable
                    }
                    // --- End Routing Table Integration Hook ---

                    // Determine MAC for proxy reply
                    auto it_mac = interface_macs_.find(request_interface_id);
                    if (it_mac != interface_macs_.end()) {
                        mac_out = it_mac->second; // Use specific interface MAC
                    } else {
                        mac_out = device_mac_; // Fallback to default device MAC
                    }
                    return true; // Proxy ARP match and routable
                }
            }
        }
        return false; // No suitable proxy ARP configuration found or not routable
    }
    
    /**
     * @brief Adds or updates an ARP entry.
     * @param ip The IP address of the entry.
     * @param new_mac The MAC address corresponding to the IP.
     * @param received_interface_id The ID of the interface on which the ARP packet was received.
     * @param packet_type The type of ARP packet that triggered this entry update.
     */
    void add_entry(uint32_t ip, const mac_addr_t& new_mac, uint32_t received_interface_id, ARPPacketType packet_type = ARPPacketType::UNKNOWN) {
        // Log interface trust status
        bool is_trusted = get_interface_trust_status(received_interface_id);
        fprintf(stderr, "INFO: ARP packet on interface %u (Trusted: %s) for IP %u MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
                received_interface_id, is_trusted ? "yes" : "no", ip,
                new_mac[0], new_mac[1], new_mac[2], new_mac[3], new_mac[4], new_mac[5]);

        // General ARP Rate Limiting Check
        if (general_arp_rate_limit_mode_ != ArpRateLimitMode::DISABLED) {
            auto current_time = std::chrono::steady_clock::now();
            uint64_t key = 0;
            char id_str[64] = {0}; // For logging

            if (general_arp_rate_limit_mode_ == ArpRateLimitMode::PER_SOURCE_MAC) {
                for (int i = 0; i < 6; ++i) key = (key << 8) | new_mac[i];
                snprintf(id_str, sizeof(id_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                         new_mac[0], new_mac[1], new_mac[2], new_mac[3], new_mac[4], new_mac[5]);
            } else { // PER_SOURCE_IP
                key = ip;
                snprintf(id_str, sizeof(id_str), "%u.%u.%u.%u", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
            }

            auto it_limit = arp_rate_limit_counters_.find(key);
            if (it_limit == arp_rate_limit_counters_.end() ||
                (current_time - it_limit->second.second) > general_arp_rate_limit_interval_) {
                arp_rate_limit_counters_[key] = {1, current_time};
            } else {
                it_limit->second.first++;
                if (it_limit->second.first > general_arp_rate_limit_count_) {
                    fprintf(stderr, "WARNING: General ARP rate limit exceeded for %s %s. Packet dropped.\n",
                            (general_arp_rate_limit_mode_ == ArpRateLimitMode::PER_SOURCE_MAC ? "MAC" : "IP"),
                            id_str);
                    return; // Drop the packet
                }
            }
        }

        // Static ARP check
        mac_addr_t static_mac;
        if (get_static_arp_entry(ip, static_mac)) {
            if (static_mac != new_mac) {
                fprintf(stderr, "WARNING: ARP packet for IP %u (MAC %02x:%02x:%02x:%02x:%02x:%02x) conflicts with static entry (MAC %02x:%02x:%02x:%02x:%02x:%02x). Packet dropped.\n",
                        ip,
                        new_mac[0], new_mac[1], new_mac[2], new_mac[3], new_mac[4], new_mac[5],
                        static_mac[0], static_mac[1], static_mac[2], static_mac[3], static_mac[4], static_mac[5]);
                return; // Drop the packet by returning early.
            }
            // If MACs match, normal processing will ensure the entry becomes REACHABLE.
        }

        // Known MAC enforcement check (Source MAC validation)
        if (get_enforce_known_macs_on_interface(received_interface_id)) {
            if (!is_mac_known_on_interface(received_interface_id, new_mac)) {
                fprintf(stderr, "WARNING: ARP packet on interface %u from unknown MAC %02x:%02x:%02x:%02x:%02x:%02x. Packet dropped.\n",
                        received_interface_id,
                        new_mac[0], new_mac[1], new_mac[2], new_mac[3], new_mac[4], new_mac[5]);
                return; // Drop the packet
            }
        }

        auto current_time_for_processing = std::chrono::steady_clock::now(); // Time for GARP checks

        // 1. Handle GratuitousArpPolicy::DROP_IF_CONFLICT
        if (packet_type == ARPPacketType::GRATUITOUS_ANNOUNCEMENT &&
            this->gratuitous_arp_policy_ == GratuitousArpPolicy::DROP_IF_CONFLICT) {
            auto it_check_conflict = cache_.find(ip);
            if (it_check_conflict != cache_.end() && it_check_conflict->second.mac != new_mac) {
                bool existing_mac_is_valid = false;
                for(uint8_t val : it_check_conflict->second.mac) if(val != 0) existing_mac_is_valid = true;

                if (existing_mac_is_valid) {
                     fprintf(stderr, "INFO: Gratuitous ARP Announcement for IP %u (MAC %02x:%02x:%02x:%02x:%02x:%02x) conflicts with existing MAC (%02x:%02x:%02x:%02x:%02x:%02x). Dropping due to DROP_IF_CONFLICT policy.\n",
                             ip, new_mac[0], new_mac[1], new_mac[2], new_mac[3], new_mac[4], new_mac[5],
                             it_check_conflict->second.mac[0], it_check_conflict->second.mac[1], it_check_conflict->second.mac[2], it_check_conflict->second.mac[3], it_check_conflict->second.mac[4], it_check_conflict->second.mac[5]);
                     this->log_ip_conflict(ip, it_check_conflict->second.mac, new_mac); // Log it formally as a conflict
                     return; // Drop
                }
            }
        }

        // 2. Handle Rate Limiting for Gratuitous ARP
        if (packet_type == ARPPacketType::GRATUITOUS_ANNOUNCEMENT &&
            this->gratuitous_arp_policy_ == GratuitousArpPolicy::RATE_LIMIT_AND_PROCESS &&
            gratuitous_arp_min_interval_ms_.count() > 0) {
            auto last_seen_it = gratuitous_arp_last_seen_.find(ip);
            if (last_seen_it != gratuitous_arp_last_seen_.end()) {
                if (current_time_for_processing - last_seen_it->second < gratuitous_arp_min_interval_ms_) {
                    fprintf(stderr, "INFO: Gratuitous ARP Announcement for IP %u (MAC %02x:%02x:%02x:%02x:%02x:%02x) dropped due to rate limiting.\n",
                            ip, new_mac[0], new_mac[1], new_mac[2], new_mac[3], new_mac[4], new_mac[5]);
                    return; // Drop the packet/update
                }
            }
            // If not dropped by rate limit, the timestamp will be updated later
        }

        // 3. Handle Logging for Gratuitous ARP (LOG_AND_PROCESS)
        if (packet_type == ARPPacketType::GRATUITOUS_ANNOUNCEMENT &&
            this->gratuitous_arp_policy_ == GratuitousArpPolicy::LOG_AND_PROCESS) {
            fprintf(stderr, "INFO: Processing Gratuitous ARP Announcement for IP %u with MAC %02x:%02x:%02x:%02x:%02x:%02x (LOG_AND_PROCESS policy).\n",
                   ip, new_mac[0], new_mac[1], new_mac[2], new_mac[3], new_mac[4], new_mac[5]);
        }

        // Update last seen time for any processed Gratuitous ARP (not dropped by specific GARP policies above)
        if (packet_type == ARPPacketType::GRATUITOUS_ANNOUNCEMENT) {
            // This ensures that if it passed DROP_IF_CONFLICT and rate limit "too soon" check,
            // its time is recorded for future rate limiting.
            // current_time_for_processing is used here for consistency with checks above.
            gratuitous_arp_last_seen_[ip] = current_time_for_processing;
        }

        // --- DHCP Snooping Validation Hook ---
        if (!this->is_ip_mac_dhcp_validated(ip, new_mac)) {
            if (enforce_dhcp_validation_) {
                fprintf(stderr, "WARNING: IP-MAC mapping for IP %u, MAC %02x:%02x:%02x:%02x:%02x:%02x failed DHCP snooping validation. Packet dropped due to enforcement policy.\n",
                        ip, new_mac[0], new_mac[1], new_mac[2], new_mac[3], new_mac[4], new_mac[5]);
                return; // Drop the packet
            } else {
                fprintf(stderr, "WARNING: IP-MAC mapping for IP %u, MAC %02x:%02x:%02x:%02x:%02x:%02x failed DHCP snooping validation. Processing continues (enforcement policy is off).\n",
                        ip, new_mac[0], new_mac[1], new_mac[2], new_mac[3], new_mac[4], new_mac[5]);
            }
            // Future: Could consult a policy here, e.g., if (dhcp_validation_policy_ == DROP_INVALID) return;
        }
        // --- End DHCP Snooping Validation Hook ---

        // 4. Main add_entry logic (conflict detection, flap detection, etc.)
        auto it = cache_.find(ip);
        auto current_time = std::chrono::steady_clock::now(); // This is the main current_time for entry updates
        std::vector<mac_addr_t> existing_backups; // Keep existing backups

        if (it != cache_.end()) { // Entry exists
            ARPEntry& entry = it->second;
            existing_backups = entry.backup_macs; // Preserve backups

            if (entry.mac != new_mac) { // MAC address has changed
                this->log_ip_conflict(ip, entry.mac, new_mac); // Always log

                mac_addr_t mac_to_potentially_update_to = new_mac;
                bool should_update_mac = false;

                switch (this->conflict_policy_) {
                    case ConflictPolicy::DROP_NEW:
                        // Logged already. Do nothing further. Existing MAC is kept.
                        should_update_mac = false;
                        break;
                    case ConflictPolicy::ALERT_SYSTEM:
                        this->trigger_alert(ip, entry.mac, mac_to_potentially_update_to);
                        // Defaulting to alert AND update.
                        should_update_mac = true;
                        break;
                    case ConflictPolicy::LOG_ONLY:
                        // Logged already. Policy is to only log, not change.
                        should_update_mac = false;
                        break;
                    case ConflictPolicy::UPDATE_EXISTING:
                    default: // Default to update
                        should_update_mac = true;
                        break;
                }

                if (should_update_mac) {
                    // Flap detection logic
                    if (current_time - entry.last_mac_update_time < flap_detection_window_sec_) {
                        if (entry.flap_count < 255) {
                            entry.flap_count++;
                        }
                    } else {
                        entry.flap_count = 1;
                    }
                    entry.last_mac_update_time = current_time;

                    entry.mac = mac_to_potentially_update_to; // Update to the new MAC
                    entry.timestamp = current_time;
                    entry.probe_count = 0;
                    entry.backoff_exponent = 0;
                    while (!entry.pending_packets.empty()) entry.pending_packets.pop();

                    if (entry.flap_count >= max_flaps_allowed_) {
                        entry.state = ARPState::STALE;
                        fprintf(stderr, "INFO: Flapping detected for IP %u (count %u). Setting to STALE with new MAC to force re-verify under conflict policy UPDATE_EXISTING/ALERT_SYSTEM.\n", ip, entry.flap_count);
                    } else {
                        entry.state = ARPState::REACHABLE;
                    }
                }
                // If should_update_mac is false, the old MAC and its state are preserved.
                // (e.g. for LOG_ONLY or DROP_NEW policies)
                // The timestamp of the entry is not updated in this case, and flap count for the *existing* MAC is not affected by this specific event.
            } else { // MAC is the same, just refresh to REACHABLE
                entry.state = ARPState::REACHABLE;
                entry.timestamp = current_time;
                entry.probe_count = 0; // Reset probe activity trackers
                entry.backoff_exponent = 0;
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

        // If a static entry exists for this IP and MAC, ensure the state is REACHABLE.
        // The general logic above (setting state to REACHABLE on new/matching MAC) handles this.
        // If a static entry forced this update, it will be marked REACHABLE.
        mac_addr_t temp_static_mac_check;
        if (get_static_arp_entry(ip, temp_static_mac_check) && temp_static_mac_check == new_mac) {
            // Ensure the state is set to REACHABLE if it's a static entry.
            // The main logic block for adding/updating entries already does this if new_mac matches entry.mac or if it's a new entry.
            // If an existing entry.mac was different but conflict policy allowed update, it would also become REACHABLE.
            // So, this explicit check might be redundant but ensures intent for static entries.
            if (cache_.count(ip)) {
                 cache_[ip].state = ARPState::REACHABLE;
                 cache_[ip].timestamp = current_time; // Also update timestamp to prevent immediate aging
            }
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
                        if (gratuitous_arp_last_seen_.count(purged_ip)) {
                            gratuitous_arp_last_seen_.erase(purged_ip);
                        }
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
        gratuitous_arp_last_seen_.clear(); // Clear GARP tracking on link down
        fprintf(stderr, "INFO: ARP cache purged due to link-down event, including LRU tracking and GARP history.\n");
    }
};

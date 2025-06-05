#pragma once

#include <vector>
#include <unordered_map>
#include <chrono>
#include <array>
#include <cstdint> // For uintX_t types
#include <queue>   // For pending packets, similar to ARPCache
#include <algorithm> // For std::remove_if

// Forward declaration for classes/structs if needed later
// class NetworkInterface; // Example if we need to interact with network interface specifics

// Define IPv6 address type for convenience
using ipv6_addr_t = std::array<uint8_t, 16>; /**< Type alias for IPv6 addresses. */
using mac_addr_t = std::array<uint8_t, 6>;  /**< Type alias for MAC addresses. */

// ND Message Types (RFC 4861)
/** @brief Defines the types of Neighbor Discovery Protocol messages. */
enum class NDMessageType : uint8_t {
    ROUTER_SOLICITATION = 133,
    ROUTER_ADVERTISEMENT = 134,
    NEIGHBOR_SOLICITATION = 135,
    NEIGHBOR_ADVERTISEMENT = 136,
    REDIRECT = 137
};

// ND Cache Entry States (similar to ARP)
/** @brief Defines the state of a Neighbor Cache entry (RFC 4861). */
enum class NDCacheState {
    INCOMPLETE, /**< Address resolution is in progress; NS has been sent. */
    REACHABLE,  /**< Neighbor is confirmed reachable recently. */
    STALE,      /**< Reachability unknown; confirm on next send. */
    DELAY,      /**< Waiting period after STALE before sending first probe. */
    PROBE,      /**< Actively probing to verify reachability. */
    PERMANENT   /**< Manually configured entry; does not expire or change state due to NUD. */
};

/** @brief Represents an entry in the Neighbor Cache. */
struct NDEntry {
    mac_addr_t mac; /**< Primary MAC address of the neighbor. */
    NDCacheState state; /**< Current state of this cache entry. */
    std::chrono::steady_clock::time_point timestamp; /**< Time of last state change or reachability confirmation. */
    std::chrono::seconds reachable_time; /**< Duration this entry remains REACHABLE. */
    int probe_count; /**< Number of probes sent in INCOMPLETE or PROBE state. */
    bool is_router; /**< True if this neighbor is known to be a router. */
    std::queue<std::vector<uint8_t>> pending_packets; /**< Packets queued awaiting address resolution. */

    // SLAAC related fields - these are more descriptive if an NDEntry represents a self-configured address.
    // For typical neighbor entries, these might not be directly used or relevant.
    // Consider if these are better suited for a separate structure for self-addresses.
    ipv6_addr_t prefix;         /**< Prefix used if this entry relates to a SLAAC configured address on self. */
    uint8_t prefix_length;      /**< Prefix length for SLAAC. */
    std::chrono::seconds valid_lifetime;   /**< Valid lifetime for the SLAAC address. */
    std::chrono::seconds preferred_lifetime; /**< Preferred lifetime for the SLAAC address. */
    bool on_link;               /**< L-bit from RA's Prefix Information Option (PIO). */
    bool autonomous;            /**< A-bit from RA's PIO, indicates if prefix can be used for SLAAC. */

    std::vector<mac_addr_t> backup_macs; /**< List of backup MAC addresses for failover. */
};

/** @brief Represents a default router learned from Router Advertisements. */
struct RouterEntry {
    ipv6_addr_t address;        /**< IPv6 address of the router. */
    mac_addr_t mac;             /**< MAC address of the router (if resolved). */
    std::chrono::seconds lifetime; /**< Router lifetime remaining, from RA. */
    std::chrono::steady_clock::time_point last_seen; /**< When this router was last confirmed via RA. */
};

/** @brief Represents a prefix learned from Router Advertisements for SLAAC. */
struct PrefixEntry {
    ipv6_addr_t prefix;         /**< The IPv6 prefix. */
    uint8_t prefix_length;      /**< Length of the prefix. */
    std::chrono::seconds valid_lifetime; /**< Valid lifetime for addresses generated from this prefix. */
    std::chrono::seconds preferred_lifetime; /**< Preferred lifetime for addresses from this prefix. */
    std::chrono::steady_clock::time_point received_time; /**< When this prefix information was received/updated. */
    bool on_link;               /**< L-bit: Is this prefix considered on-link. */
    bool autonomous;            /**< A-bit: Can this prefix be used for autonomous address configuration (SLAAC). */
    ipv6_addr_t generated_address; /**< The IPv6 address generated using this prefix via SLAAC, if any. */
    bool dad_completed;         /**< True if DAD has successfully completed for the generated_address. */
};

/**
 * @brief Implements an IPv6 Neighbor Discovery Protocol (NDP) cache.
 *
 * Handles neighbor resolution, router discovery, SLAAC, DAD, and fast failover mechanisms
 * as described in RFC 4861 and related RFCs.
 */
class NDCache {
private:
    std::unordered_map<ipv6_addr_t, NDEntry> cache_; /**< Main neighbor cache: maps IPv6 addresses to NDEntry. */
    std::vector<RouterEntry> default_routers_;      /**< List of known default routers. */
    std::vector<PrefixEntry> prefix_list_;          /**< List of prefixes learned from RAs for SLAAC. */
    mac_addr_t device_mac_;                         /**< MAC address of the device this cache is on. */
    ipv6_addr_t link_local_address_;                /**< Link-local IPv6 address of the device. */
    bool link_local_dad_completed_ = false;         /**< Tracks DAD completion for the link-local address. */

    // Constants for ND protocol (values typically from RFC 4861)
    static constexpr int MAX_RTR_SOLICITATION_DELAY = 1; // second (used for DAD random delay)
    static constexpr int RTR_SOLICITATION_INTERVAL = 4; // seconds
    static constexpr int MAX_RTR_SOLICITATIONS = 3;
    static constexpr int MAX_MULTICAST_SOLICIT = 3; // For DAD and address resolution
    static constexpr int MAX_UNICAST_SOLICIT = 3; // For NUD
    static constexpr int RETRANS_TIMER = 1000; // milliseconds (RFC 4861 default)
    static constexpr int DELAY_FIRST_PROBE_TIME = 5; // seconds (RFC 4861)
    static constexpr std::chrono::seconds DEFAULT_REACHABLE_TIME = std::chrono::seconds(30); // RFC 4861

    // Helper to generate EUI-64 based interface ID (lower 64 bits of an IPv6 address)
    std::array<uint8_t, 8> generate_eui64_interface_id_bytes(const mac_addr_t& mac); // Forms the interface identifier part of an IPv6 address.
    // Helper to construct solicited-node multicast address
    ipv6_addr_t solicited_node_multicast_address(const ipv6_addr_t& target_ip); // Used as destination for NS messages.

public:
    /**
     * @brief Constructs an NDCache.
     * @param own_mac The MAC address of the device this cache is on; used for EUI-64 generation.
     */
    NDCache(const mac_addr_t& own_mac);
    ~NDCache();

    // --- Virtual methods for sending ND packets (to be implemented by network layer/driver) ---
    /** @brief Sends a Router Solicitation message. (Placeholder - override in derived class for actual sending) */
    virtual void send_router_solicitation(const ipv6_addr_t& source_ip);
    /** @brief Sends a Neighbor Solicitation message. (Placeholder - override in derived class for actual sending) */
    virtual void send_neighbor_solicitation(const ipv6_addr_t& target_ip, const ipv6_addr_t& source_ip, const mac_addr_t* sllao, bool for_dad = false);
    /** @brief Sends a Neighbor Advertisement message. (Placeholder - override in derived class for actual sending) */
    virtual void send_neighbor_advertisement(const ipv6_addr_t& target_ip, const ipv6_addr_t& adv_source_ip, const mac_addr_t& tllao, bool is_router, bool solicited, bool override_flag);

    // --- Structures for simplified message info processing ---
    /** @brief Simplified structure to pass Router Advertisement data into the cache. */
    struct RAInfo {
        ipv6_addr_t source_ip;      /**< Source IPv6 address of the RA (router's link-local). */
        mac_addr_t router_mac;      /**< MAC address from Source Link-Layer Address Option (SLLAO) in RA, if present. */
        std::chrono::seconds router_lifetime; /**< Router lifetime; 0 means not a default router. */
        std::vector<PrefixEntry> prefixes; /**< List of Prefix Information Options from the RA. */
    };
    /** @brief Simplified structure for Neighbor Solicitation data. */
    struct NSInfo {
        ipv6_addr_t source_ip;      /**< Source IPv6 of NS. Unspecified (::) if for DAD. */
        ipv6_addr_t target_ip;      /**< Target IPv6 address being solicited. */
        mac_addr_t sllao;           /**< Source Link-Layer Address Option, if present. Zeroed if not. */
        bool is_dad_ns;             /**< True if this NS is for DAD (source_ip is ::, no SLLAO). */
    };
    /** @brief Simplified structure for Neighbor Advertisement data. */
    struct NAInfo {
        ipv6_addr_t source_ip;      /**< Source IPv6 of the NA packet (sender of NA). */
        ipv6_addr_t target_ip;      /**< Target IPv6 address in the NA message body (address whose MAC is being advertised). */
        mac_addr_t tllao;           /**< Target Link-Layer Address Option. */
        bool is_router;             /**< R-flag: sender is a router. */
        bool solicited;             /**< S-flag: NA sent in response to a unicast NS. */
        bool override_flag;         /**< O-flag: override existing cache entry. */
    };

    // --- Processing incoming ND messages ---
    /** @brief Processes an incoming Router Advertisement. Updates router list and prefix list, triggers SLAAC. */
    void process_router_advertisement(const RAInfo& ra_info);
    /** @brief Processes an incoming Neighbor Solicitation. Handles DAD conflicts and responds if NS is for own address. */
    void process_neighbor_solicitation(const NSInfo& ns_info);
    /** @brief Processes an incoming Neighbor Advertisement. Handles DAD conflicts and updates neighbor cache. */
    void process_neighbor_advertisement(const NAInfo& na_info);

    // --- Core cache operations ---
    /**
     * @brief Looks up the MAC address for a given IPv6 address.
     * Implements fast failover to backup MACs if primary is STALE/PROBE/DELAY.
     * @param ip IPv6 address to resolve.
     * @param mac_out Output parameter for the resolved MAC address.
     * @return True if a usable MAC is found, false otherwise (e.g., resolution in progress).
     */
    bool lookup(const ipv6_addr_t& ip, mac_addr_t& mac_out);
    /**
     * @brief Adds or updates a Neighbor Cache entry.
     * @param ip IPv6 address of the neighbor.
     * @param mac MAC address of the neighbor.
     * @param state Initial state of the entry.
     * @param reachable_time Duration for which the entry is considered REACHABLE.
     * @param is_router Whether the neighbor is a router.
     * @param backups Optional list of backup MACs for this entry.
     */
    void add_entry(const ipv6_addr_t& ip, const mac_addr_t& mac, NDCacheState state, std::chrono::seconds reachable_time = DEFAULT_REACHABLE_TIME, bool is_router = false, const std::vector<mac_addr_t>& backups = {});
    /** @brief Removes an entry from the cache. */
    void remove_entry(const ipv6_addr_t& ip);
    /**
     * @brief Adds a backup MAC address for an existing IPv6 neighbor entry.
     * @param ipv6 The IPv6 address of the neighbor.
     * @param backup_mac The backup MAC address to add.
     */
    void add_backup_mac(const ipv6_addr_t& ipv6, const mac_addr_t& backup_mac);
    /**
     * @brief Ages cache entries, router list, prefix list, and DAD states.
     * Manages state transitions, NUD probing, and failover to backups on primary MAC failure.
     */
    void age_entries();

    // --- SLAAC related functions ---
    /**
     * @brief Configures an IPv6 address using SLAAC based on a received prefix.
     * Generates an address and initiates DAD for it.
     * @param prefix_entry The prefix information from an RA.
     */
    void configure_address_slaac(const PrefixEntry& prefix_entry);

    // --- DAD related functions ---
    /**
     * @brief Starts Duplicate Address Detection for a given IPv6 address.
     * @param address_to_check The IPv6 address to perform DAD on.
     * @return True if DAD was initiated, false if already in progress or address is already assigned.
     */
    bool start_dad(const ipv6_addr_t& address_to_check);
    /** @brief Processes a successful DAD outcome for an address. */
    void process_dad_success(const ipv6_addr_t& address);
    /** @brief Processes a failed DAD outcome (conflict detected) for an address. */
    void process_dad_failure(const ipv6_addr_t& address);

    // --- Utility functions ---
    /** @brief Gets the device's current link-local IPv6 address. */
    ipv6_addr_t get_link_local_address() const { return link_local_address_; }
    /** @brief Checks if DAD has successfully completed for the device's link-local address. */
    bool is_link_local_dad_completed() const { return link_local_dad_completed_; }

private:
    /** @brief Internal state for an ongoing DAD process. */
    struct DadState {
        ipv6_addr_t address;        /**< The address being checked. */
        int probes_sent;            /**< Number of DAD NS probes sent. */
        std::chrono::steady_clock::time_point next_probe_time; /**< When to send the next DAD probe. */
        std::chrono::steady_clock::time_point start_time;      /**< When DAD process started (for overall timeout, if any). */
    };
    std::vector<DadState> dad_in_progress_; /**< List of addresses currently undergoing DAD. */

    void generate_link_local_address(); // Generates link-local address using EUI-64.
    void initiate_link_local_dad();     // Starts DAD for the generated link-local address.
    ipv6_addr_t get_unspecified_address() const { ipv6_addr_t addr; addr.fill(0); return addr; } // Returns '::'.
};

// --- Method Implementations ---

NDCache::NDCache(const mac_addr_t& own_mac) : device_mac_(own_mac) {
    generate_link_local_address();
    initiate_link_local_dad();
}

NDCache::~NDCache() {}

void NDCache::generate_link_local_address() {
    link_local_address_.fill(0); link_local_address_[0] = 0xfe; link_local_address_[1] = 0x80;
    std::array<uint8_t, 8> iid_bytes = generate_eui64_interface_id_bytes(device_mac_);
    for(size_t i = 0; i < 8; ++i) { link_local_address_[i+8] = iid_bytes[i]; }
}

void NDCache::initiate_link_local_dad(){
    if (!start_dad(link_local_address_)) { /* Log error */ }
}

std::array<uint8_t, 8> NDCache::generate_eui64_interface_id_bytes(const mac_addr_t& mac) {
    std::array<uint8_t, 8> iid_bytes{};
    iid_bytes[0] = mac[0] ^ 0x02; iid_bytes[1] = mac[1]; iid_bytes[2] = mac[2]; iid_bytes[3] = 0xFF;
    iid_bytes[4] = 0xFE; iid_bytes[5] = mac[3]; iid_bytes[6] = mac[4]; iid_bytes[7] = mac[5];
    return iid_bytes;
}

ipv6_addr_t NDCache::solicited_node_multicast_address(const ipv6_addr_t& target_ip) {
    ipv6_addr_t snmc_addr = {{0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0x00, 0x00, 0x00}};
    snmc_addr[13] = target_ip[13]; snmc_addr[14] = target_ip[14]; snmc_addr[15] = target_ip[15];
    return snmc_addr;
}

// Note: Mock send implementations are for example purposes.
// Real implementations would interact with the network stack.
void NDCache::send_router_solicitation(const ipv6_addr_t& source_ip) {
    // fprintf(stderr, "MockSend: Router Solicitation from IP: ...\n");
}
void NDCache::send_neighbor_solicitation(const ipv6_addr_t& target_ip, const ipv6_addr_t& source_ip, const mac_addr_t* sllao, bool for_dad) {
    // fprintf(stderr, "MockSend: Neighbor Solicitation for IP: ..., from IP: ..., DAD: %d\n", for_dad);
}
void NDCache::send_neighbor_advertisement(const ipv6_addr_t& target_ip, const ipv6_addr_t& adv_source_ip, const mac_addr_t& tllao, bool is_router, bool solicited, bool override_flag) {
    // fprintf(stderr, "MockSend: Neighbor Advertisement for Target IP: ..., TLLAO: ...\n");
}

bool NDCache::lookup(const ipv6_addr_t& ip, mac_addr_t& mac_out) {
    auto it = cache_.find(ip);
    if (it != cache_.end()) {
        NDEntry& entry = it->second;
        switch (entry.state) {
            case NDCacheState::REACHABLE:
            case NDCacheState::PERMANENT:
                mac_out = entry.mac;
                entry.timestamp = std::chrono::steady_clock::now(); // Refresh timestamp on use for REACHABLE
                return true;
            case NDCacheState::STALE:
            case NDCacheState::DELAY:
            case NDCacheState::PROBE:
                // Primary MAC is not REACHABLE, try a backup if available.
                if (!entry.backup_macs.empty()) {
                    mac_addr_t old_primary_mac = entry.mac;
                    entry.mac = entry.backup_macs.front();
                    entry.backup_macs.erase(entry.backup_macs.begin());

                    bool old_mac_is_zero = true;
                    for(uint8_t val : old_primary_mac) if(val != 0) old_mac_is_zero = false;
                    if (!old_mac_is_zero && std::find(entry.backup_macs.begin(), entry.backup_macs.end(), old_primary_mac) == entry.backup_macs.end()) {
                        entry.backup_macs.push_back(old_primary_mac);
                    }

                    entry.state = NDCacheState::REACHABLE; // Assume backup is good for now
                    entry.timestamp = std::chrono::steady_clock::now();
                    entry.probe_count = 0;
                    mac_out = entry.mac;
                    // fprintf(stderr, "INFO: ND Failover for IP. New MAC used.\n"); // Add IP formatting
                    return true;
                }
                // No backups, proceed with NUD for the current MAC
                mac_out = entry.mac; // Return current MAC
                if (entry.state == NDCacheState::STALE) {
                    entry.state = NDCacheState::DELAY; // Transition to DELAY as per RFC 4861
                    entry.timestamp = std::chrono::steady_clock::now();
                }
                // For DELAY/PROBE, NUD is already in progress.
                return (entry.state == NDCacheState::DELAY); // True if in DELAY (usable but probe pending)
            case NDCacheState::INCOMPLETE:
                return false; // Resolution in progress
        }
    }

    // Not in cache (it == cache_.end())
    if (it == cache_.end()) {
         add_entry(ip, {}, NDCacheState::INCOMPLETE);
         // Send initial NS for address resolution (if link_local_address_ is DAD'ed and ready)
         if (is_link_local_dad_completed() || link_local_address_ == get_unspecified_address()) { // Allow sending if LL is unspecified (bootstrap)
            send_neighbor_solicitation(ip, link_local_address_, &device_mac_, false);
         }
    }
    return false;
}

void NDCache::add_entry(const ipv6_addr_t& ip, const mac_addr_t& mac, NDCacheState state, std::chrono::seconds reachable_time, bool is_router, const std::vector<mac_addr_t>& backups) {
    auto it = cache_.find(ip);
    if (it != cache_.end()) {
        // Update existing entry
        it->second.mac = mac;
        if (it->second.state != NDCacheState::PERMANENT) { it->second.state = state; }
        it->second.timestamp = std::chrono::steady_clock::now();
        if (reachable_time != std::chrono::seconds(0)) { it->second.reachable_time = reachable_time; }
        it->second.is_router = is_router;
        it->second.probe_count = 0;
        it->second.backup_macs = backups; // Update backup MACs

        if (state == NDCacheState::REACHABLE) { while(!it->second.pending_packets.empty()) { it->second.pending_packets.pop(); } }
    } else {
        // Add new entry
        NDEntry new_entry = {
            mac, state, std::chrono::steady_clock::now(),
            (reachable_time == std::chrono::seconds(0) ? DEFAULT_REACHABLE_TIME : reachable_time),
            0, is_router, {}, // pending_packets
            {}, 0, {}, {}, false, false, // slaac info
            backups // backup_macs
        };
        cache_[ip] = new_entry;
    }
}

void NDCache::remove_entry(const ipv6_addr_t& ip) { cache_.erase(ip); }

void NDCache::add_backup_mac(const ipv6_addr_t& ipv6, const mac_addr_t& backup_mac) {
    auto it = cache_.find(ipv6);
    if (it != cache_.end()) {
        if (std::find(it->second.backup_macs.begin(), it->second.backup_macs.end(), backup_mac) == it->second.backup_macs.end() && it->second.mac != backup_mac) {
            it->second.backup_macs.push_back(backup_mac);
        }
    }
}

void NDCache::age_entries() {
    auto now = std::chrono::steady_clock::now();
    for (auto it = cache_.begin(); it != cache_.end(); ) {
        bool erased = false; NDEntry& entry = it->second; // Use reference
        auto time_since_last_update = std::chrono::duration_cast<std::chrono::seconds>(now - entry.timestamp);

        switch (entry.state) {
            case NDCacheState::INCOMPLETE:
                if (time_since_last_update.count() >= (RETRANS_TIMER / 1000)) { // Assuming RETRANS_TIMER is ms
                    if (entry.probe_count >= MAX_MULTICAST_SOLICIT) { // Max probes for initial resolution
                        // Failed to resolve primary MAC. Try backups.
                        if (!entry.backup_macs.empty()) {
                            entry.mac = entry.backup_macs.front();
                            entry.backup_macs.erase(entry.backup_macs.begin());
                            entry.state = NDCacheState::REACHABLE; // Or PROBE this backup
                            entry.timestamp = now;
                            entry.probe_count = 0;
                            // fprintf(stderr, "INFO: ND Primary MAC resolution failed. Switched to backup.\n");
                        } else {
                            while(!entry.pending_packets.empty()) entry.pending_packets.pop();
                            it = cache_.erase(it);
                            erased = true;
                        }
                    } else {
                        send_neighbor_solicitation(it->first, link_local_address_, &device_mac_, false);
                        entry.probe_count++;
                        entry.timestamp = now;
                    }
                }
                break;
            case NDCacheState::REACHABLE:
                if (time_since_last_update >= entry.reachable_time) {
                    entry.state = NDCacheState::STALE;
                    entry.timestamp = now;
                }
                break;
            case NDCacheState::STALE:
                // In our lookup, STALE transitions to DELAY. If lookup is not called, it stays STALE.
                // Or, age_entries can transition it to PROBE after some time if desired.
                // For now, DELAY transition is in lookup.
                break;
            case NDCacheState::DELAY:
                if (time_since_last_update.count() >= DELAY_FIRST_PROBE_TIME) {
                    entry.state = NDCacheState::PROBE;
                    entry.timestamp = now;
                    entry.probe_count = 0;
                    send_neighbor_solicitation(it->first, link_local_address_, &device_mac_, false); // Send first unicast probe
                }
                break;
            case NDCacheState::PROBE:
                if (time_since_last_update.count() >= (RETRANS_TIMER / 1000)) { // Unicast probe retransmission
                    if (entry.probe_count >= MAX_UNICAST_SOLICIT) { // Max probes for NUD
                        // Primary MAC failed NUD. Try backups.
                        if (!entry.backup_macs.empty()) {
                            mac_addr_t old_primary_mac = entry.mac; // Keep for demotion
                            entry.mac = entry.backup_macs.front();
                            entry.backup_macs.erase(entry.backup_macs.begin());

                            bool old_mac_is_zero = true; for(uint8_t val : old_primary_mac) if(val != 0) old_mac_is_zero = false;
                            if(!old_mac_is_zero && std::find(entry.backup_macs.begin(), entry.backup_macs.end(), old_primary_mac) == entry.backup_macs.end()){
                                entry.backup_macs.push_back(old_primary_mac);
                            }

                            entry.state = NDCacheState::REACHABLE; // Or PROBE this backup
                            entry.timestamp = now;
                            entry.probe_count = 0;
                            // fprintf(stderr, "INFO: ND NUD failed for primary MAC. Switched to backup.\n");
                        } else {
                            it = cache_.erase(it);
                            erased = true;
                        }
                    } else {
                        send_neighbor_solicitation(it->first, link_local_address_, &device_mac_, false);
                        entry.probe_count++;
                        entry.timestamp = now;
                    }
                }
                break;
            case NDCacheState::PERMANENT:
                // No aging for permanent entries
                break;
        }
        if (!erased) { ++it; }
    }
    default_routers_.erase(std::remove_if(default_routers_.begin(), default_routers_.end(), [&](const RouterEntry& r){ return std::chrono::duration_cast<std::chrono::seconds>(now - r.last_seen) > r.lifetime; }), default_routers_.end());
    prefix_list_.erase(std::remove_if(prefix_list_.begin(), prefix_list_.end(), [&](const PrefixEntry& p){ return std::chrono::duration_cast<std::chrono::seconds>(now - p.received_time) > p.valid_lifetime; }), prefix_list_.end());

    for (auto dad_it = dad_in_progress_.begin(); dad_it != dad_in_progress_.end(); ) {
        bool dad_entry_removed = false;
        if (now >= dad_it->next_probe_time) {
            if (dad_it->probes_sent >= MAX_MULTICAST_SOLICIT) {
                process_dad_success(dad_it->address);
                dad_it = dad_in_progress_.erase(dad_it); // process_dad_success also erases, ensure this is safe or let only one erase.
                dad_entry_removed = true;
            } else {
                send_neighbor_solicitation(dad_it->address, get_unspecified_address(), nullptr, true);
                dad_it->probes_sent++;
                dad_it->next_probe_time = now + std::chrono::milliseconds(RETRANS_TIMER);
            }
        }
        if (!dad_entry_removed) { ++dad_it; } // Only increment if not erased
        else if (dad_it != dad_in_progress_.end() && dad_in_progress_.empty()) { break; } // Break if erased last element
    }
}

bool NDCache::start_dad(const ipv6_addr_t& address_to_check) {
    if (std::any_of(dad_in_progress_.begin(), dad_in_progress_.end(), [&](const DadState& ds){ return ds.address == address_to_check;})) { return false; }
    if ((link_local_dad_completed_ && link_local_address_ == address_to_check) ||
        std::any_of(prefix_list_.begin(), prefix_list_.end(), [&](const PrefixEntry& pe){ return pe.generated_address == address_to_check && pe.dad_completed;})) {
        return false; // Already DAD'ed and using this address
    }
    DadState new_dad_state = {address_to_check, 0, std::chrono::steady_clock::now(), std::chrono::steady_clock::now()};
    // First probe sent by age_entries processing loop.
    // RFC 4862 section 5.4: delay of rand(0, MAX_RTR_SOLICITATION_DELAY) before first probe. For simplicity, immediate here.
    dad_in_progress_.push_back(new_dad_state);
    return true;
}

void NDCache::process_dad_success(const ipv6_addr_t& address) {
    if (address == link_local_address_) {
        link_local_dad_completed_ = true;
        // send_router_solicitation(link_local_address_); // Trigger RS
    } else {
        for (auto& prefix_entry : prefix_list_) {
            if (prefix_entry.generated_address == address) {
                prefix_entry.dad_completed = true;
                break;
            }
        }
    }
    // Remove from dad_in_progress_ list. Be careful if called from age_entries loop.
    // It's safer if the loop that calls this handles removal.
    // For now, let this function also try to remove.
    dad_in_progress_.erase(std::remove_if(dad_in_progress_.begin(), dad_in_progress_.end(),
                                         [&](const DadState& ds){ return ds.address == address; }),
                           dad_in_progress_.end());
}

void NDCache::process_dad_failure(const ipv6_addr_t& address) {
    if (address == link_local_address_) {
        link_local_dad_completed_ = false; /* Critical error */
    } else {
        for (auto& prefix_entry : prefix_list_) {
            if (prefix_entry.generated_address == address) {
                prefix_entry.generated_address.fill(0);
                prefix_entry.dad_completed = false;
                break;
            }
        }
    }
    dad_in_progress_.erase(std::remove_if(dad_in_progress_.begin(), dad_in_progress_.end(),
                                         [&](const DadState& ds){ return ds.address == address; }),
                           dad_in_progress_.end());
}

void NDCache::process_router_advertisement(const RAInfo& ra_info) {
    auto now = std::chrono::steady_clock::now();
    if (ra_info.router_lifetime > std::chrono::seconds(0)) { /* Add/update router */
        auto it = std::find_if(default_routers_.begin(), default_routers_.end(), [&](const RouterEntry& r) { return r.address == ra_info.source_ip; });
        if (it != default_routers_.end()) {
            it->lifetime = ra_info.router_lifetime; it->last_seen = now;
            if (ra_info.router_mac != mac_addr_t{} ) { it->mac = ra_info.router_mac; add_entry(ra_info.source_ip, ra_info.router_mac, NDCacheState::REACHABLE, DEFAULT_REACHABLE_TIME, true); }
        } else {
            default_routers_.push_back({ra_info.source_ip, ra_info.router_mac, ra_info.router_lifetime, now});
            if (ra_info.router_mac != mac_addr_t{} ) { add_entry(ra_info.source_ip, ra_info.router_mac, NDCacheState::REACHABLE, DEFAULT_REACHABLE_TIME, true); }
            else { add_entry(ra_info.source_ip, {}, NDCacheState::INCOMPLETE, DEFAULT_REACHABLE_TIME, true); }
        }
    } else { /* Remove router */
        default_routers_.erase(std::remove_if(default_routers_.begin(), default_routers_.end(), [&](const RouterEntry& r) { return r.address == ra_info.source_ip; }), default_routers_.end());
        auto cache_it = cache_.find(ra_info.source_ip); if (cache_it != cache_.end()) { cache_it->second.is_router = false; }
    }

    for (const auto& new_prefix_info : ra_info.prefixes) {
        if (!new_prefix_info.on_link && !new_prefix_info.autonomous) continue;
        auto it = std::find_if(prefix_list_.begin(), prefix_list_.end(), [&](const PrefixEntry& p) { return p.prefix == new_prefix_info.prefix && p.prefix_length == new_prefix_info.prefix_length; });
        if (new_prefix_info.valid_lifetime == std::chrono::seconds(0)) {
            if (it != prefix_list_.end()) { if (it->generated_address != ipv6_addr_t{} ) { cache_.erase(it->generated_address); } prefix_list_.erase(it); } continue;
        }
        if (it != prefix_list_.end()) {
            it->valid_lifetime = new_prefix_info.valid_lifetime; it->preferred_lifetime = new_prefix_info.preferred_lifetime;
            it->on_link = new_prefix_info.on_link; it->autonomous = new_prefix_info.autonomous; it->received_time = now;
            if (new_prefix_info.autonomous && it->generated_address == ipv6_addr_t{} ) { configure_address_slaac(*it); }
        } else {
            PrefixEntry to_add = new_prefix_info; to_add.received_time = now; to_add.generated_address.fill(0); to_add.dad_completed = false;
            prefix_list_.push_back(to_add);
            if (to_add.autonomous) { configure_address_slaac(prefix_list_.back()); }
        }
    }
}

// Made configure_address_slaac take non-const PrefixEntry by changing its call site
// or by finding the element again inside. For now, let's assume it finds it again if it needs to modify.
// The provided code for configure_address_slaac finds it again, so const is fine.
void NDCache::configure_address_slaac(const PrefixEntry& prefix_entry_const) {
    if (!prefix_entry_const.autonomous || prefix_entry_const.prefix_length != 64) { return; }
    ipv6_addr_t new_address = prefix_entry_const.prefix;
    std::array<uint8_t, 8> iid_bytes = generate_eui64_interface_id_bytes(device_mac_);
    for (size_t i = 0; i < 8; ++i) { new_address[i + 8] = iid_bytes[i]; }

    auto it = std::find_if(prefix_list_.begin(), prefix_list_.end(),
                           [&](const PrefixEntry& p) { return p.prefix == prefix_entry_const.prefix && p.prefix_length == prefix_entry_const.prefix_length; });
    if (it != prefix_list_.end()) { // Should always find it as we pass an existing entry
        it->generated_address = new_address;
        it->dad_completed = false;
        if (!start_dad(new_address)) { it->generated_address.fill(0); /* DAD start failed */ }
    }
}

void NDCache::process_neighbor_solicitation(const NSInfo& ns_info) {
    // Check if NS is for an address we are DAD'ing
    for (auto& dad_entry : dad_in_progress_) {
        if (dad_entry.address == ns_info.target_ip) {
            // DAD conflict detected if the source IP is not unspecified, or if it is unspecified and it's not our own DAD NS.
            // Simplified: any NS for an address under DAD (not sent by us) is a conflict.
            // A more precise check: if ns_info.source_ip is unspecified, it's another DAD. Conflict.
            // If ns_info.source_ip is specified, it's a lookup. If DAD is still pending, this is also a conflict.
            process_dad_failure(dad_entry.address);
            return; // DAD failed, stop processing this NS for this address.
        }
    }

    // Check if NS is for one of our configured addresses
    bool target_is_own_address = (link_local_dad_completed_ && ns_info.target_ip == link_local_address_);
    if (!target_is_own_address) {
        for(const auto& p_entry : prefix_list_){
            if(p_entry.dad_completed && p_entry.generated_address == ns_info.target_ip){
                target_is_own_address = true;
                break;
            }
        }
    }

    if (target_is_own_address) {
        // We are the target. Send NA.
        // Target of NA is source_ip of NS. Target Link-Layer Opt is our MAC.
        // R-flag is 0 unless we are a router. S-flag is 1 (solicited). O-flag is 1 (override).
        send_neighbor_advertisement(ns_info.source_ip, ns_info.target_ip, device_mac_, false /*is_router?*/, true, true);
    } else {
        // Not for us, and not a DAD conflict for an address we are actively checking.
        // Could be a lookup for another host. If we have it in cache and it's REACHABLE,
        // we could act as a proxy (not in scope for typical host ND).
        // Generally, hosts only respond to NS for their own addresses.
    }
}

void NDCache::process_neighbor_advertisement(const NAInfo& na_info) {
    // Check if NA is a DAD conflict
    for (const auto& dad_entry : dad_in_progress_) {
        if (dad_entry.address == na_info.target_ip) { // Target IP in NA body, source IP of NA packet is who sent it
            // This NA means someone else is already using/claiming the address.
            process_dad_failure(dad_entry.address);
            return;
        }
    }

    // Regular NA processing (update cache)
    // RFC 4861 Section 7.2.5
    auto it = cache_.find(na_info.target_ip); // Target IP in NA body is the key for cache
    if (it != cache_.end()) {
        NDEntry& entry = it->second;
        bool mac_changed = (entry.mac != na_info.tllao);

        if (entry.state == NDCacheState::INCOMPLETE) {
            entry.mac = na_info.tllao;
            entry.state = na_info.solicited ? NDCacheState::REACHABLE : NDCacheState::STALE;
            entry.timestamp = std::chrono::steady_clock::now();
            entry.is_router = na_info.is_router;
            // Process pending packets
        } else { // STALE, REACHABLE, DELAY, PROBE
            if (!na_info.override_flag && mac_changed) {
                // If O-flag is clear and MAC is different, set to STALE
                if (entry.state == NDCacheState::REACHABLE) entry.state = NDCacheState::STALE;
                // Don't update MAC if O=0 and MAC differs
            } else {
                // O-flag is set, or O-flag is clear and MAC is same. Update MAC if different.
                if (mac_changed) entry.mac = na_info.tllao; // Update MAC only if O=1 or MAC was same

                if (na_info.solicited) { // If S-flag is set, update to REACHABLE
                    entry.state = NDCacheState::REACHABLE;
                    entry.timestamp = std::chrono::steady_clock::now(); // S=1 means forward reachability confirmed
                }
                // If S=0, state remains unchanged unless O=0 and MAC changed (handled above)
            }
        }
        if (entry.is_router != na_info.is_router) { // Router flag changed
            entry.is_router = na_info.is_router;
            // This might affect default router list or path selection, complex logic.
        }
    } else {
        // NA for an unknown entry, if TLLAO is present, can create a STALE entry.
        if (na_info.tllao != mac_addr_t{} ) { // Check for non-zero MAC
             add_entry(na_info.target_ip, na_info.tllao, NDCacheState::STALE, DEFAULT_REACHABLE_TIME, na_info.is_router);
        }
    }
}

// Example of how one might call these (conceptual)
// ... (rest of comments from before)

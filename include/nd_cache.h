#pragma once

#include <vector>
#include <unordered_map>
#include <chrono>
#include <array>
#include <cstdint> // For uintX_t types
#include <queue>   // For pending packets, similar to ARPCache
#include <algorithm> // For std::remove_if
#include <functional> // For std::hash specialization

using ipv6_addr_t = std::array<uint8_t, 16>;
using mac_addr_t = std::array<uint8_t, 6>;

namespace std {
    template <>
    struct hash<ipv6_addr_t> {
        std::size_t operator()(const ipv6_addr_t& addr) const {
            std::size_t h = 0;
            for (uint8_t byte : addr) {
                h ^= std::hash<uint8_t>{}(byte) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
            return h;
        }
    };
}

enum class NDMessageType : uint8_t {
    ROUTER_SOLICITATION = 133, ROUTER_ADVERTISEMENT = 134, NEIGHBOR_SOLICITATION = 135,
    NEIGHBOR_ADVERTISEMENT = 136, REDIRECT = 137
};
enum class NDCacheState { INCOMPLETE, REACHABLE, STALE, DELAY, PROBE, PERMANENT };

struct NDEntry {
    mac_addr_t mac; NDCacheState state; std::chrono::steady_clock::time_point timestamp;
    std::chrono::seconds reachable_time; int probe_count; bool is_router;
    std::queue<std::vector<uint8_t>> pending_packets;
    ipv6_addr_t prefix; uint8_t prefix_length; std::chrono::seconds valid_lifetime;
    std::chrono::seconds preferred_lifetime; bool on_link; bool autonomous;
    std::vector<mac_addr_t> backup_macs;
};
struct RouterEntry {
    ipv6_addr_t address; mac_addr_t mac; std::chrono::seconds lifetime;
    std::chrono::steady_clock::time_point last_seen;
};
struct PrefixEntry {
    ipv6_addr_t prefix; uint8_t prefix_length; std::chrono::seconds valid_lifetime;
    std::chrono::seconds preferred_lifetime; std::chrono::steady_clock::time_point received_time;
    bool on_link; bool autonomous; ipv6_addr_t generated_address; bool dad_completed;
};

class NDCache {
private:
    struct DadState {
        ipv6_addr_t address; int probes_sent;
        std::chrono::steady_clock::time_point next_probe_time;
        std::chrono::steady_clock::time_point start_time; // When DAD for this address was initiated
    };

    std::unordered_map<ipv6_addr_t, NDEntry> cache_;
    std::vector<RouterEntry> default_routers_;
    std::vector<PrefixEntry> prefix_list_;
    mac_addr_t device_mac_;
    ipv6_addr_t link_local_address_;
    ipv6_addr_t link_local_solicited_node_address_;
    bool link_local_dad_completed_ = false;
    std::vector<DadState> dad_in_progress_;

    std::array<uint8_t, 8> generate_eui64_interface_id_bytes(const mac_addr_t& mac);
    void generate_link_local_address();
    void initiate_link_local_dad();
    ipv6_addr_t get_unspecified_address() const { ipv6_addr_t addr; addr.fill(0); return addr; }

public:
    // Debug members
    mutable int debug_dad_probes_sent_for_link_local = 0;
    mutable bool debug_dad_success_called_for_link_local = false;
    mutable bool debug_dad_entry_exists_for_link_local = false; // True if LL DAD entry is in dad_in_progress_
    mutable std::chrono::steady_clock::time_point debug_link_local_next_probe_time{};

    // Public helper, also used internally for DAD NS target
    ipv6_addr_t solicited_node_multicast_address(const ipv6_addr_t& target_ip);

    static constexpr int MAX_RTR_SOLICITATION_DELAY = 1;
    static constexpr int RTR_SOLICITATION_INTERVAL = 4;
    static constexpr int MAX_RTR_SOLICITATIONS = 3;
    static constexpr int MAX_MULTICAST_SOLICIT = 3;
    static constexpr int MAX_UNICAST_SOLICIT = 3;
    static constexpr int RETRANS_TIMER = 1000;
    static constexpr int DELAY_FIRST_PROBE_TIME = 5;
    static constexpr std::chrono::seconds DEFAULT_REACHABLE_TIME = std::chrono::seconds(30);

    NDCache(const mac_addr_t& own_mac);
    ~NDCache();

    virtual void send_router_solicitation(const ipv6_addr_t& source_ip);
    virtual void send_neighbor_solicitation(const ipv6_addr_t& target_ip, const ipv6_addr_t& source_ip, const mac_addr_t* sllao, bool for_dad = false);
    virtual void send_neighbor_advertisement(const ipv6_addr_t& target_ip, const ipv6_addr_t& adv_source_ip, const mac_addr_t& tllao, bool is_router, bool solicited, bool override_flag);

    struct RAInfo { ipv6_addr_t source_ip; mac_addr_t router_mac; std::chrono::seconds router_lifetime; std::vector<PrefixEntry> prefixes; };
    struct NSInfo { ipv6_addr_t source_ip; ipv6_addr_t target_ip; mac_addr_t sllao; bool is_dad_ns; };
    struct NAInfo { ipv6_addr_t source_ip; ipv6_addr_t target_ip; mac_addr_t tllao; bool is_router; bool solicited; bool override_flag; };

    void process_router_advertisement(const RAInfo& ra_info);
    void process_neighbor_solicitation(const NSInfo& ns_info);
    void process_neighbor_advertisement(const NAInfo& na_info);

    bool lookup(const ipv6_addr_t& ip, mac_addr_t& mac_out);
    void add_entry(const ipv6_addr_t& ip, const mac_addr_t& mac, NDCacheState state, std::chrono::seconds reachable_time = DEFAULT_REACHABLE_TIME, bool is_router = false, const std::vector<mac_addr_t>& backups = {});
    void remove_entry(const ipv6_addr_t& ip);
    void add_backup_mac(const ipv6_addr_t& ipv6, const mac_addr_t& backup_mac);

    void age_entries() { age_entries(std::chrono::steady_clock::now()); }
    void age_entries(std::chrono::steady_clock::time_point current_time);

    void configure_address_slaac(const PrefixEntry& prefix_entry);

    bool start_dad(const ipv6_addr_t& address_to_check);
    void process_dad_success(const ipv6_addr_t& address);
    void process_dad_failure(const ipv6_addr_t& address);

    ipv6_addr_t get_link_local_address() const { return link_local_address_; }
    ipv6_addr_t get_link_local_solicited_node_address() const { return link_local_solicited_node_address_; }
    bool is_link_local_dad_completed() const { return link_local_dad_completed_; }
};

// Method Implementations
NDCache::NDCache(const mac_addr_t& own_mac) : device_mac_(own_mac) {
    // Initialize debug members
    debug_dad_probes_sent_for_link_local = 0;
    debug_dad_success_called_for_link_local = false;
    debug_dad_entry_exists_for_link_local = false; // Will be set true by start_dad if for link-local

    generate_link_local_address();
    initiate_link_local_dad();
}
NDCache::~NDCache() {}

void NDCache::generate_link_local_address() {
    link_local_address_.fill(0); link_local_address_[0] = 0xfe; link_local_address_[1] = 0x80;
    std::array<uint8_t, 8> iid_bytes = generate_eui64_interface_id_bytes(device_mac_);
    for(size_t i = 0; i < 8; ++i) { link_local_address_[i+8] = iid_bytes[i]; }
    link_local_solicited_node_address_ = solicited_node_multicast_address(link_local_address_);
}
void NDCache::initiate_link_local_dad(){ if (!start_dad(link_local_address_)) { /* Log error */ } }
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
void NDCache::send_router_solicitation(const ipv6_addr_t& source_ip) { /* Placeholder */ }
void NDCache::send_neighbor_solicitation(const ipv6_addr_t& target_ip, const ipv6_addr_t& source_ip, const mac_addr_t* sllao, bool for_dad) { /* Placeholder */ }
void NDCache::send_neighbor_advertisement(const ipv6_addr_t& target_ip, const ipv6_addr_t& adv_source_ip, const mac_addr_t& tllao, bool is_router, bool solicited, bool override_flag) { /* Placeholder */ }

bool NDCache::lookup(const ipv6_addr_t& ip, mac_addr_t& mac_out) {
    // ... (Implementation from previous successful state) ...
    auto it = cache_.find(ip);
    if (it != cache_.end()) {
        NDEntry& entry = it->second;
        switch (entry.state) {
            case NDCacheState::REACHABLE: case NDCacheState::PERMANENT:
                mac_out = entry.mac; entry.timestamp = std::chrono::steady_clock::now(); return true;
            case NDCacheState::STALE: case NDCacheState::DELAY: case NDCacheState::PROBE:
                if (!entry.backup_macs.empty()) {
                    mac_addr_t old_primary_mac = entry.mac; entry.mac = entry.backup_macs.front();
                    entry.backup_macs.erase(entry.backup_macs.begin());
                    bool old_mac_is_zero = true; for(uint8_t val : old_primary_mac) if(val != 0) old_mac_is_zero = false;
                    if (!old_mac_is_zero && std::find(entry.backup_macs.begin(), entry.backup_macs.end(), old_primary_mac) == entry.backup_macs.end()) {
                        entry.backup_macs.push_back(old_primary_mac);
                    }
                    entry.state = NDCacheState::REACHABLE; entry.timestamp = std::chrono::steady_clock::now();
                    entry.probe_count = 0; mac_out = entry.mac; return true;
                }
                mac_out = entry.mac;
                if (entry.state == NDCacheState::STALE) {
                    entry.state = NDCacheState::DELAY; entry.timestamp = std::chrono::steady_clock::now();
                }
                return (entry.state == NDCacheState::DELAY);
            case NDCacheState::INCOMPLETE: return false;
        }
    }
    if (it == cache_.end()) {
         add_entry(ip, {}, NDCacheState::INCOMPLETE);
         if (is_link_local_dad_completed() || link_local_address_ == get_unspecified_address()) {
            send_neighbor_solicitation(ip, link_local_address_, &device_mac_, false);
         }
    }
    return false;
}
void NDCache::add_entry(const ipv6_addr_t& ip, const mac_addr_t& mac, NDCacheState state, std::chrono::seconds reachable_time, bool is_router, const std::vector<mac_addr_t>& backups) {
    // ... (Implementation from previous successful state) ...
    auto it = cache_.find(ip);
    if (it != cache_.end()) {
        it->second.mac = mac; if (it->second.state != NDCacheState::PERMANENT) { it->second.state = state; }
        it->second.timestamp = std::chrono::steady_clock::now();
        if (reachable_time != std::chrono::seconds(0)) { it->second.reachable_time = reachable_time; }
        it->second.is_router = is_router; it->second.probe_count = 0; it->second.backup_macs = backups;
        if (state == NDCacheState::REACHABLE) { while(!it->second.pending_packets.empty()) { it->second.pending_packets.pop(); } }
    } else {
        NDEntry new_entry = { mac, state, std::chrono::steady_clock::now(), (reachable_time == std::chrono::seconds(0) ? DEFAULT_REACHABLE_TIME : reachable_time), 0, is_router, {}, {}, 0, {}, {}, false, false, backups };
        cache_[ip] = new_entry;
    }
}
void NDCache::remove_entry(const ipv6_addr_t& ip) { cache_.erase(ip); }
void NDCache::add_backup_mac(const ipv6_addr_t& ipv6, const mac_addr_t& backup_mac) {
    // ... (Implementation from previous successful state) ...
    auto it = cache_.find(ipv6);
    if (it != cache_.end()) {
        if (std::find(it->second.backup_macs.begin(), it->second.backup_macs.end(), backup_mac) == it->second.backup_macs.end() && it->second.mac != backup_mac) {
            it->second.backup_macs.push_back(backup_mac);
        }
    }
}

void NDCache::age_entries(std::chrono::steady_clock::time_point current_time) {
    // Age NDEntry (cache_)
    for (auto it = cache_.begin(); it != cache_.end(); ) {
        // ... (Existing NDEntry aging logic as per previous file state, using current_time) ...
        bool erased = false; NDEntry& entry = it->second;
        auto time_since_last_update = std::chrono::duration_cast<std::chrono::seconds>(current_time - entry.timestamp);
        switch (entry.state) {
            case NDCacheState::INCOMPLETE:
                if (time_since_last_update.count() >= (RETRANS_TIMER / 1000)) {
                    if (entry.probe_count >= MAX_MULTICAST_SOLICIT) {
                        if (!entry.backup_macs.empty()) { entry.mac = entry.backup_macs.front(); entry.backup_macs.erase(entry.backup_macs.begin()); entry.state = NDCacheState::REACHABLE; entry.timestamp = current_time; entry.probe_count = 0;}
                        else { while(!entry.pending_packets.empty()) entry.pending_packets.pop(); it = cache_.erase(it); erased = true; }
                    } else { send_neighbor_solicitation(it->first, link_local_address_, &device_mac_, false); entry.probe_count++; entry.timestamp = current_time; }
                } break;
            case NDCacheState::REACHABLE:
                if (time_since_last_update >= entry.reachable_time) { entry.state = NDCacheState::STALE; entry.timestamp = current_time; } break;
            case NDCacheState::STALE: break;
            case NDCacheState::DELAY:
                if (time_since_last_update.count() >= DELAY_FIRST_PROBE_TIME) { entry.state = NDCacheState::PROBE; entry.timestamp = current_time; entry.probe_count = 0; send_neighbor_solicitation(it->first, link_local_address_, &device_mac_, false); } break;
            case NDCacheState::PROBE:
                if (time_since_last_update.count() >= (RETRANS_TIMER / 1000)) {
                    if (entry.probe_count >= MAX_UNICAST_SOLICIT) {
                        if (!entry.backup_macs.empty()) { mac_addr_t old_primary_mac = entry.mac; entry.mac = entry.backup_macs.front(); entry.backup_macs.erase(entry.backup_macs.begin()); bool old_mac_is_zero = true; for(uint8_t val : old_primary_mac) if(val != 0) old_mac_is_zero = false; if(!old_mac_is_zero && std::find(entry.backup_macs.begin(), entry.backup_macs.end(), old_primary_mac) == entry.backup_macs.end()){ entry.backup_macs.push_back(old_primary_mac); } entry.state = NDCacheState::REACHABLE; entry.timestamp = current_time; entry.probe_count = 0; }
                        else { it = cache_.erase(it); erased = true; }
                    } else { send_neighbor_solicitation(it->first, link_local_address_, &device_mac_, false); entry.probe_count++; entry.timestamp = current_time; }
                } break;
            case NDCacheState::PERMANENT: break;
        }
        if (!erased) { ++it; }
    }
    // Age RouterList and PrefixList
    default_routers_.erase(std::remove_if(default_routers_.begin(), default_routers_.end(), [&](const RouterEntry& r){ return std::chrono::duration_cast<std::chrono::seconds>(current_time - r.last_seen) > r.lifetime; }), default_routers_.end());
    prefix_list_.erase(std::remove_if(prefix_list_.begin(), prefix_list_.end(), [&](const PrefixEntry& p){ return std::chrono::duration_cast<std::chrono::seconds>(current_time - p.received_time) > p.valid_lifetime; }), prefix_list_.end());

    // DAD processing loop (Corrected logic)
    debug_dad_entry_exists_for_link_local = false; // Reset for this run
    auto dad_it = dad_in_progress_.begin();
    while (dad_it != dad_in_progress_.end()) {
        // Update debug info if processing link-local
        if (dad_it->address == link_local_address_) {
            debug_dad_entry_exists_for_link_local = true;
            debug_dad_probes_sent_for_link_local = dad_it->probes_sent;
            debug_link_local_next_probe_time = dad_it->next_probe_time;
        }

        if (current_time < dad_it->next_probe_time) {
            ++dad_it; // Not yet time for any action on this DAD entry
            continue;
        }

        // Time for action (probe or declare success)
        if (dad_it->probes_sent < MAX_MULTICAST_SOLICIT) {
            // Action: Send a probe
            send_neighbor_solicitation(solicited_node_multicast_address(dad_it->address),
                                       get_unspecified_address(), nullptr, true);
            dad_it->probes_sent++;
            dad_it->start_time = current_time; // Using start_time as last_event_time for DAD entry
            dad_it->next_probe_time = current_time + std::chrono::milliseconds(RETRANS_TIMER);

            // Update debug info again after modification for link-local
            if (dad_it->address == link_local_address_) {
                debug_dad_probes_sent_for_link_local = dad_it->probes_sent;
                debug_link_local_next_probe_time = dad_it->next_probe_time;
            }
            ++dad_it;
        } else { // dad_it->probes_sent == MAX_MULTICAST_SOLICIT
            // Action: DAD Success (all probes sent, and DAD resolution period has passed)
            process_dad_success(dad_it->address);
            dad_it = dad_in_progress_.erase(dad_it);
        }
    }
}

bool NDCache::start_dad(const ipv6_addr_t& address_to_check) {
    if (std::any_of(dad_in_progress_.begin(), dad_in_progress_.end(), [&](const DadState& ds){ return ds.address == address_to_check;})) { return false; }
    if ((link_local_dad_completed_ && link_local_address_ == address_to_check) ||
        std::any_of(prefix_list_.begin(), prefix_list_.end(), [&](const PrefixEntry& pe){ return pe.generated_address == address_to_check && pe.dad_completed;})) {
        return false;
    }
    auto now = std::chrono::steady_clock::now();
    DadState new_dad_state = {address_to_check, 0, now, now}; // probes_sent=0, next_probe_time=now, start_time=now
    dad_in_progress_.push_back(new_dad_state);

    if (address_to_check == link_local_address_) {
        debug_dad_probes_sent_for_link_local = 0; // Reset on new DAD start for LL
        debug_dad_success_called_for_link_local = false;
        debug_dad_entry_exists_for_link_local = true;
        debug_link_local_next_probe_time = new_dad_state.next_probe_time;
    }
    return true;
}

void NDCache::process_dad_success(const ipv6_addr_t& address) {
    if (address == link_local_address_) {
        link_local_dad_completed_ = true;
        debug_dad_success_called_for_link_local = true;
    } else {
        for (auto& prefix_entry : prefix_list_) {
            if (prefix_entry.generated_address == address) {
                prefix_entry.dad_completed = true;
                break;
            }
        }
    }
    // Element is removed by the DAD loop in age_entries.
}

void NDCache::process_dad_failure(const ipv6_addr_t& address) {
    if (address == link_local_address_) {
        link_local_dad_completed_ = false;
        debug_dad_success_called_for_link_local = false; // Ensure success flag is false
    } else {
        for (auto& prefix_entry : prefix_list_) {
            if (prefix_entry.generated_address == address) {
                prefix_entry.generated_address.fill(0);
                prefix_entry.dad_completed = false;
                break;
            }
        }
    }
    // Element is removed by the caller (process_neighbor_solicitation or process_neighbor_advertisement).
}

void NDCache::process_router_advertisement(const RAInfo& ra_info) {
    // ... (Implementation from previous successful state, ensure current_time usage if any) ...
    auto current_processing_time = std::chrono::steady_clock::now();
    if (ra_info.router_lifetime > std::chrono::seconds(0)) {
        auto it = std::find_if(default_routers_.begin(), default_routers_.end(), [&](const RouterEntry& r) { return r.address == ra_info.source_ip; });
        if (it != default_routers_.end()) {
            it->lifetime = ra_info.router_lifetime; it->last_seen = current_processing_time;
            if (ra_info.router_mac != mac_addr_t{} ) { it->mac = ra_info.router_mac; add_entry(ra_info.source_ip, ra_info.router_mac, NDCacheState::REACHABLE, DEFAULT_REACHABLE_TIME, true); }
        } else {
            default_routers_.push_back({ra_info.source_ip, ra_info.router_mac, ra_info.router_lifetime, current_processing_time});
            if (ra_info.router_mac != mac_addr_t{} ) { add_entry(ra_info.source_ip, ra_info.router_mac, NDCacheState::REACHABLE, DEFAULT_REACHABLE_TIME, true); }
            else { add_entry(ra_info.source_ip, {}, NDCacheState::INCOMPLETE, DEFAULT_REACHABLE_TIME, true); }
        }
    } else {
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
            it->on_link = new_prefix_info.on_link; it->autonomous = new_prefix_info.autonomous; it->received_time = current_processing_time;
            if (new_prefix_info.autonomous && it->generated_address == ipv6_addr_t{} ) { configure_address_slaac(*it); }
        } else {
            PrefixEntry to_add = new_prefix_info; to_add.received_time = current_processing_time; to_add.generated_address.fill(0); to_add.dad_completed = false;
            prefix_list_.push_back(to_add);
            if (to_add.autonomous) { configure_address_slaac(prefix_list_.back()); }
        }
    }
}
void NDCache::configure_address_slaac(const PrefixEntry& prefix_entry_const) {
    // ... (Implementation from previous successful state) ...
    if (!prefix_entry_const.autonomous || prefix_entry_const.prefix_length != 64) { return; }
    ipv6_addr_t new_address = prefix_entry_const.prefix;
    std::array<uint8_t, 8> iid_bytes = generate_eui64_interface_id_bytes(device_mac_);
    for (size_t i = 0; i < 8; ++i) { new_address[i + 8] = iid_bytes[i]; }
    auto it = std::find_if(prefix_list_.begin(), prefix_list_.end(),
                           [&](const PrefixEntry& p) { return p.prefix == prefix_entry_const.prefix && p.prefix_length == prefix_entry_const.prefix_length; });
    if (it != prefix_list_.end()) {
        it->generated_address = new_address; it->dad_completed = false;
        if (!start_dad(new_address)) { it->generated_address.fill(0); }
    }
}

void NDCache::process_neighbor_solicitation(const NSInfo& ns_info) {
    // ... (Implementation from previous successful state, ensuring dad_in_progress_ is handled carefully) ...
    for (auto dad_it = dad_in_progress_.begin(); dad_it != dad_in_progress_.end(); ) {
        if (dad_it->address == ns_info.target_ip) {
            process_dad_failure(dad_it->address); // Sets flags
            // process_dad_failure now also removes from dad_in_progress_ list as per its implementation
            return;
        } else {
            ++dad_it;
        }
    }
    bool target_is_own_address = (link_local_dad_completed_ && ns_info.target_ip == link_local_address_);
    if (!target_is_own_address) {
        for(const auto& p_entry : prefix_list_){
            if(p_entry.dad_completed && p_entry.generated_address == ns_info.target_ip){
                target_is_own_address = true; break;
            }
        }
    }
    if (target_is_own_address) {
        send_neighbor_advertisement(ns_info.source_ip, ns_info.target_ip, device_mac_, false , true, true);
    }
}

void NDCache::process_neighbor_advertisement(const NAInfo& na_info) {
    // ... (Implementation from previous successful state, ensuring dad_in_progress_ is handled carefully) ...
    for (auto dad_it = dad_in_progress_.begin(); dad_it != dad_in_progress_.end(); ) {
        if (dad_it->address == na_info.target_ip) {
            process_dad_failure(dad_it->address); // Sets flags and removes from dad_in_progress_
            return;
        } else {
            ++dad_it;
        }
    }
    auto it = cache_.find(na_info.target_ip);
    if (it != cache_.end()) {
        NDEntry& entry = it->second; bool mac_changed = (entry.mac != na_info.tllao);
        if (entry.state == NDCacheState::INCOMPLETE) {
            entry.mac = na_info.tllao; entry.state = na_info.solicited ? NDCacheState::REACHABLE : NDCacheState::STALE;
            entry.timestamp = std::chrono::steady_clock::now(); entry.is_router = na_info.is_router;
        } else {
            if (!na_info.override_flag && mac_changed) {
                if (entry.state == NDCacheState::REACHABLE) entry.state = NDCacheState::STALE;
            } else {
                if (mac_changed) entry.mac = na_info.tllao;
                if (na_info.solicited) {
                    entry.state = NDCacheState::REACHABLE; entry.timestamp = std::chrono::steady_clock::now();
                }
            }
        }
        if (entry.is_router != na_info.is_router) { entry.is_router = na_info.is_router; }
    } else {
        if (na_info.tllao != mac_addr_t{} ) {
             add_entry(na_info.target_ip, na_info.tllao, NDCacheState::STALE, DEFAULT_REACHABLE_TIME, na_info.is_router);
        }
    }
}

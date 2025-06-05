#include <iostream>
#include <vector>
#include <array>
#include <thread> // For std::this_thread::sleep_for
#include <chrono> // For std::chrono::seconds
#include <cstdio> // For printf in helper functions

#include "../include/nd_cache.h" // Adjust path as necessary

// Helper to print MAC addresses
void print_mac_nd(const mac_addr_t& mac) {
    for (size_t i = 0; i < mac.size(); ++i) {
        printf("%02x", mac[i]);
        if (i < mac.size() - 1) printf(":");
    }
}

// Helper to print IPv6 addresses (simplified)
void print_ipv6(const ipv6_addr_t& ipv6) {
    for (size_t i = 0; i < ipv6.size(); i += 2) {
        printf("%02x%02x", ipv6[i], ipv6[i+1]);
        if (i < ipv6.size() - 2) printf(":");
    }
}

// Mocked send functions for the example (NDCache expects these to be implemented)
// In a real application, these would interact with a network driver.
// We make them global or static within a derived class for simplicity here.
// To make this example self-contained without modifying NDCache to have non-pure virtuals,
// we'd typically derive a class. But for a simple example, we'll just note they are placeholders.

// If NDCache's send methods were not pure virtual, we could use this.
// For this example, we assume they are callable (e.g. have dummy bodies in NDCache or are not pure)
// Or, we create a simple derived class for the example.

class ExampleNDCache : public NDCache {
public:
    ExampleNDCache(const mac_addr_t& own_mac) : NDCache(own_mac) {}

    void send_router_solicitation(const ipv6_addr_t& source_ip) override {
        std::cout << "  [Network Send] Router Solicitation from IP: "; print_ipv6(source_ip); std::cout << std::endl;
    }

    void send_neighbor_solicitation(const ipv6_addr_t& target_ip, const ipv6_addr_t& source_ip, const mac_addr_t* sllao, bool for_dad = false) override {
        std::cout << "  [Network Send] Neighbor Solicitation for IP: "; print_ipv6(target_ip);
        std::cout << " from IP: "; print_ipv6(source_ip);
        if (sllao) { std::cout << " with SLLAO: "; print_mac_nd(*sllao); }
        if (for_dad) { std::cout << " (for DAD)"; }
        std::cout << std::endl;
    }

    void send_neighbor_advertisement(const ipv6_addr_t& target_ip, const ipv6_addr_t& adv_source_ip, const mac_addr_t& tllao, bool is_router, bool solicited, bool override_flag) override {
        std::cout << "  [Network Send] Neighbor Advertisement for Target IP: "; print_ipv6(adv_source_ip); // adv_source_ip is the address whose MAC is being advertised
        std::cout << " to IP: "; print_ipv6(target_ip); // target_ip is the destination of the NA packet
        std::cout << " with TLLAO: "; print_mac_nd(tllao);
        std::cout << " (Router: " << is_router << ", Solicited: " << solicited << ", Override: " << override_flag << ")";
        std::cout << std::endl;
    }
};


int main() {
    std::cout << "ND Cache Example\n" << std::endl;

    mac_addr_t device_mac = {0x00, 0x0E, 0x0C, 0x01, 0x02, 0x03};
    ExampleNDCache nd_cache(device_mac); // Using example class with mock sends

    std::cout << "NDCache created. Device MAC: "; print_mac_nd(device_mac); std::cout << std::endl;
    std::cout << "Device Link-Local Address (initially, DAD pending): "; print_ipv6(nd_cache.get_link_local_address()); std::cout << std::endl;

    // 1. DAD for Link-Local Address
    std::cout << "\n--- DAD for Link-Local Address ---" << std::endl;
    // The constructor calls start_dad. age_entries drives the process.
    for (int i = 0; i <= NDCache::MAX_MULTICAST_SOLICIT; ++i) {
        std::cout << "Calling age_entries (DAD attempt " << i+1 << ")" << std::endl;
        nd_cache.age_entries(); // Simulates time passing and DAD probes being sent/checked
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Small delay for output readability
        if (nd_cache.is_link_local_dad_completed()) {
            std::cout << "  DAD for Link-Local Address completed successfully." << std::endl;
            break;
        }
    }
    if (!nd_cache.is_link_local_dad_completed()) {
        std::cerr << "  ERROR: DAD for Link-Local Address did not complete as expected in this example." << std::endl;
    } else {
        // After DAD, a Router Solicitation might be sent
        std::cout << "Sending Router Solicitation after LL DAD completion:" << std::endl;
        nd_cache.send_router_solicitation(nd_cache.get_link_local_address());
    }
    std::cout << std::endl;

    // 2. Processing Router Advertisement and SLAAC
    std::cout << "--- Processing Router Advertisement & SLAAC ---" << std::endl;
    NDCache::RAInfo ra_data; // Mocked RA data
    ra_data.source_ip = {{0xfe,0x80,0,0,0,0,0,0, 0xaa,0xbb,0xcc,0xff,0xfe,0xdd,0xee,0xff}}; // Router's LL addr
    ra_data.router_mac = {{0xaa,0xbb,0xcc,0xdd,0xee,0xff}};
    ra_data.router_lifetime = std::chrono::seconds(1800);

    PrefixEntry prefix_info;
    prefix_info.prefix = {{0x20,0x01,0x0d,0xb8,0x00,0x10,0,0, 0,0,0,0,0,0,0,0}}; // 2001:db8:10::/64
    prefix_info.prefix_length = 64;
    prefix_info.on_link = true;
    prefix_info.autonomous = true; // Key for SLAAC
    prefix_info.valid_lifetime = std::chrono::seconds(7200);
    prefix_info.preferred_lifetime = std::chrono::seconds(3600);
    ra_data.prefixes.push_back(prefix_info);

    std::cout << "Processing incoming Router Advertisement..." << std::endl;
    nd_cache.process_router_advertisement(ra_data);
    std::cout << "  RA processed. If SLAAC prefix was present, DAD for new address should start via age_entries." << std::endl;

    // SLAAC should have generated an address and started DAD.
    // We need to find this address to see its DAD progress.
    // This is hard without inspecting NDCache's prefix_list_ or dad_in_progress_.
    // For this example, we'll just run age_entries and observe NS for DAD.
    std::cout << "Running age_entries to process DAD for SLAAC address:" << std::endl;
    for (int i = 0; i <= NDCache::MAX_MULTICAST_SOLICIT; ++i) {
        std::cout << "Calling age_entries (SLAAC DAD attempt " << i+1 << ")" << std::endl;
        nd_cache.age_entries();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // Conceptual: check if SLAAC address DAD completed.
        // bool slaac_dad_done = ... (would need a getter in NDCache)
        // if (slaac_dad_done) break;
    }
    // ipv6_addr_t generated_slaac_addr = ... (how to get this for printing?)
    // std::cout << "  SLAAC generated address (e.g., "; print_ipv6(generated_slaac_addr); std::cout << ") DAD process shown." << std::endl;
    std::cout << "  (Observing [Network Send] Neighbor Solicitation for DAD of the new SLAAC address is key)." << std::endl;
    std::cout << std::endl;

    // 3. DAD Conflict Example (Illustrative)
    std::cout << "--- DAD Conflict Illustration ---" << std::endl;
    ipv6_addr_t conflicting_address = {{0x20,0x01,0x0d,0xb8,0x00,0xAA,0,0, 0,0,0,0,0,0,0,0x01}};
    std::cout << "Starting DAD for a test address: "; print_ipv6(conflicting_address); std::cout << std::endl;
    nd_cache.start_dad(conflicting_address);
    nd_cache.age_entries(); // Should send first DAD NS.

    std::cout << "Simulating receiving a Neighbor Advertisement for this address (conflict):" << std::endl;
    NDCache::NAInfo na_conflict;
    na_conflict.target_ip = conflicting_address; // NA is for the address we are DAD'ing
    na_conflict.source_ip = {{0xfe,0x80,0,0,0,0,0,0, 0x11,0x22,0x33,0xff,0xfe,0x44,0x55,0x66}}; // Some other node
    na_conflict.tllao = {{0x11,0x22,0x33,0x44,0x55,0x66}};
    na_conflict.is_router = false;
    na_conflict.solicited = false;
    na_conflict.override_flag = true;
    nd_cache.process_neighbor_advertisement(na_conflict);
    // DAD for conflicting_address should have failed.
    // Conceptual check: assert(!nd_cache.is_address_dad_completed(conflicting_address));
    std::cout << "  DAD for "; print_ipv6(conflicting_address); std::cout << " should have failed due to NA." << std::endl;
    std::cout << std::endl;

    // 4. Basic Add/Lookup & Failover
    std::cout << "--- Basic Add/Lookup & Failover ---" << std::endl;
    ipv6_addr_t neighbor_ip = {{0x20,0x01,0x0d,0xb8,0x00,0xBB,0,0, 0,0,0,0,0,0,0,0x01}};
    mac_addr_t primary_nd_mac = {{0xBB,0xBB,0xBB,0xBB,0xBB,0x01}};
    mac_addr_t backup_nd_mac = {{0xBB,0xBB,0xBB,0xBB,0xBB,0x02}};
    mac_addr_t mac_nd_out;

    std::cout << "Adding entry for neighbor: "; print_ipv6(neighbor_ip); std::cout << " -> "; print_mac_nd(primary_nd_mac); std::cout << std::endl;
    nd_cache.add_entry(neighbor_ip, primary_nd_mac, NDCacheState::REACHABLE);
    std::cout << "Adding backup MAC: "; print_mac_nd(backup_nd_mac); std::cout << " for "; print_ipv6(neighbor_ip); std::cout << std::endl;
    nd_cache.add_backup_mac(neighbor_ip, backup_nd_mac);

    if (nd_cache.lookup(neighbor_ip, mac_nd_out)) {
        std::cout << "  Lookup for "; print_ipv6(neighbor_ip); std::cout << " successful. MAC: "; print_mac_nd(mac_nd_out); std::cout << std::endl;
    } else {
        std::cerr << "  ERROR: Lookup failed for " << std::endl;
    }
    // Conceptual failover description similar to ARP example
    std::cout << "Conceptual: If primary ND MAC becomes STALE/PROBE, lookup or age_entries should trigger failover." << std::endl;
    std::cout << std::endl;

    std::cout << "ND Cache example finished." << std::endl;
    return 0;
}

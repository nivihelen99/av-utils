# `NDCache` - IPv6 Neighbor Discovery Protocol Cache

## Overview

The `NDCache` class (`nd_cache.h`) implements core functionalities of the IPv6 Neighbor Discovery Protocol (NDP), as defined in RFC 4861 and related RFCs. NDP is responsible for various link-local operations in IPv6, including:

-   **Address Resolution:** Mapping IPv6 addresses to link-layer (MAC) addresses (analogous to ARP in IPv4).
-   **Neighbor Reachability Tracking (NUD):** Determining if a neighbor is still reachable.
-   **Duplicate Address Detection (DAD):** Ensuring that an IPv6 address configured on an interface is unique on the link.
-   **Router Discovery:** Finding default routers on the link.
-   **Prefix Discovery:** Learning on-link prefixes and parameters for address autoconfiguration.
-   **Stateless Address Autoconfiguration (SLAAC):** Automatically configuring IPv6 addresses based on advertised prefixes.

The `NDCache` class manages a neighbor cache (similar to an ARP cache), a default router list, and a prefix list. It handles the state transitions for neighbor entries and the logic for sending and processing NDP messages (Router Solicitations/Advertisements, Neighbor Solicitations/Advertisements).

## Key Data Structures and Concepts

-   **`ipv6_addr_t`**: `std::array<uint8_t, 16>` for IPv6 addresses.
-   **`mac_addr_t`**: `std::array<uint8_t, 6>` for MAC addresses.
-   **`NDCacheState`**: Enum representing the state of a neighbor cache entry (e.g., `INCOMPLETE`, `REACHABLE`, `STALE`, `PROBE`, `DELAY`, `PERMANENT`).
-   **`NDEntry`**: Struct for neighbor cache entries, storing MAC address, state, timestamps, router flag, pending packets, prefix information, SLAAC flags, and backup MACs.
-   **`RouterEntry`**: Struct for storing information about default routers.
-   **`PrefixEntry`**: Struct for storing prefix information from Router Advertisements, used for on-link determination and SLAAC.
-   **`DadState`**: Internal struct to manage the state of Duplicate Address Detection for an IPv6 address.

## Features

-   **Link-Local Address Management:** Automatically generates an EUI-64 based link-local IPv6 address and performs DAD on it.
-   **Duplicate Address Detection (DAD):** Implements DAD by sending Neighbor Solicitations and processing replies to ensure address uniqueness before use.
-   **Neighbor Cache Management:**
    -   Maintains a cache of IPv6-to-MAC address mappings with states (`INCOMPLETE`, `REACHABLE`, `STALE`, etc.).
    -   Handles neighbor reachability probing and state transitions based on NDP rules.
    -   Supports backup MAC addresses for failover.
-   **Router and Prefix Discovery:**
    -   Processes Router Advertisement (RA) messages to populate a default router list and a prefix list.
-   **Stateless Address Autoconfiguration (SLAAC):**
    -   Can generate global IPv6 addresses based on autonomous prefixes received in RAs and perform DAD on them.
-   **NDP Message Processing:** Includes logic to process incoming Neighbor Solicitations (NS) and Neighbor Advertisements (NA), updating the cache and DAD states accordingly.
-   **Extensible Packet Sending:** NDP message sending functions (`send_router_solicitation`, `send_neighbor_solicitation`, `send_neighbor_advertisement`) are `virtual`, allowing a derived class to implement actual network transmission.
-   **Periodic Aging:** An `age_entries()` method is provided to handle timeouts, state transitions, and DAD probe retransmissions. This method is intended to be called periodically by the system.

## Public Interface Highlights

### Constructor
-   **`NDCache(const mac_addr_t& own_mac)`**: Initializes the cache with the device's own MAC address. Automatically starts DAD for the link-local address.

### Core Operations
-   **`void process_router_advertisement(const RAInfo& ra_info)`**: Processes an incoming RA.
-   **`void process_neighbor_solicitation(const NSInfo& ns_info)`**: Processes an incoming NS.
-   **`void process_neighbor_advertisement(const NAInfo& na_info)`**: Processes an incoming NA.
-   **`bool lookup(const ipv6_addr_t& ip, mac_addr_t& mac_out)`**: Attempts to resolve an IPv6 address to a MAC address. May trigger NS if the entry is `INCOMPLETE`.
-   **`void add_entry(...)` / `void remove_entry(...)` / `void add_backup_mac(...)`**: Manual cache manipulation.
-   **`void age_entries()` / `void age_entries(std::chrono::steady_clock::time_point current_time)`**: Performs periodic processing (state updates, DAD, timeouts).
-   **`bool start_dad(const ipv6_addr_t& address_to_check)`**: Manually initiates DAD for an address.
-   **`void configure_address_slaac(const PrefixEntry& prefix_entry)`**: Generates a SLAAC address and starts DAD.

### Virtual Send Methods (to be overridden by user)
-   **`virtual void send_router_solicitation(...)`**
-   **`virtual void send_neighbor_solicitation(...)`**
-   **`virtual void send_neighbor_advertisement(...)`**

### Getters
-   **`ipv6_addr_t get_link_local_address() const`**
-   **`bool is_link_local_dad_completed() const`**

## Usage Example (Conceptual)

To use `NDCache`, you would typically:
1.  Derive a class from `NDCache`.
2.  Override the virtual `send_...` methods to implement actual packet transmission using your system's network interface.
3.  Instantiate your derived class with the device's MAC address.
4.  Feed incoming NDP packets (parsed into `RAInfo`, `NSInfo`, `NAInfo` structs) to the respective `process_...` methods.
5.  Call `age_entries()` periodically (e.g., every second) to drive internal state machines, DAD, and NUD.
6.  Use `lookup()` for address resolution when sending IPv6 packets.

```cpp
#include "nd_cache.h" // Adjust path
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

// Mock network interface for sending packets
class MyNetworkInterface {
public:
    void send_ndp_packet(const std::string& type, const ipv6_addr_t& target, const ipv6_addr_t& source) {
        // Simplified print_ipv6
        auto ipv6_to_str = [](const ipv6_addr_t& ip) {
            char buf[40]; sprintf(buf, "%02x%02x:%02x%02x:...", ip[0],ip[1],ip[2],ip[3]); return std::string(buf);
        };
        std::cout << "  [MyNetIF Send] " << type
                  << " Target: " << ipv6_to_str(target)
                  << " Source: " << ipv6_to_str(source) << std::endl;
    }
};

class MyNDCache : public NDCache {
public:
    MyNetworkInterface net_if;

    MyNDCache(const mac_addr_t& own_mac) : NDCache(own_mac) {}

    void send_router_solicitation(const ipv6_addr_t& source_ip) override {
        net_if.send_ndp_packet("Router Solicitation", {}, source_ip);
    }

    void send_neighbor_solicitation(const ipv6_addr_t& target_ip,
                                    const ipv6_addr_t& source_ip,
                                    const mac_addr_t* sllao, bool for_dad = false) override {
        net_if.send_ndp_packet(for_dad ? "DAD NS" : "NS", target_ip, source_ip);
    }
    // ... implement other send methods ...
    void send_neighbor_advertisement(const ipv6_addr_t& target_ip,
                                     const ipv6_addr_t& adv_source_ip,
                                     const mac_addr_t& tllao, bool is_router,
                                     bool solicited, bool override_flag) override {
        net_if.send_ndp_packet("NA", target_ip, adv_source_ip);
    }
};

int main() {
    mac_addr_t my_mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    MyNDCache nd_manager(my_mac);

    std::cout << "NDCache initialized. Link-local DAD will proceed via age_entries." << std::endl;
    std::cout << "Initial Link-Local: ";
    // Simplified print_ipv6
    for(size_t i=0; i<nd_manager.get_link_local_address().size(); ++i) printf("%02x", nd_manager.get_link_local_address()[i]);
    std::cout << std::endl;


    // In a real system, a timer would call age_entries() periodically
    for (int i = 0; i < 5; ++i) { // Simulate a few seconds
        std::cout << "Calling age_entries() cycle " << i+1 << std::endl;
        nd_manager.age_entries();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (nd_manager.is_link_local_dad_completed()) {
            std::cout << "Link-Local DAD completed successfully!" << std::endl;
            break;
        }
    }

    // Example: Try to resolve an address
    ipv6_addr_t some_neighbor_ipv6 = {{0x20,0x01,0x0d,0xb8,0,0,0,0,0,0,0,0,0,0,0x12,0x34}};
    mac_addr_t resolved_mac;
    if (nd_manager.lookup(some_neighbor_ipv6, resolved_mac)) {
        // ... use resolved_mac ...
    } else {
        std::cout << "Lookup for neighbor initiated NS. Will resolve after NA." << std::endl;
    }
    // ... system would receive NA and call process_neighbor_advertisement ...
}
```

## Dependencies
- `<vector>`, `<unordered_map>`, `<chrono>`, `<array>`, `<cstdint>`, `<queue>`, `<algorithm>`, `<functional>`
- A `std::hash` specialization for `ipv6_addr_t` (i.e., `std::array<uint8_t, 16>`) is provided in the header.

The `NDCache` class provides a substantial foundation for implementing IPv6 host stack functionalities related to neighbor discovery.

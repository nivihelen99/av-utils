# `ARPCache`

## Overview

The `ARPCache` class (`arp_cache.h`) implements a sophisticated Address Resolution Protocol (ARP) cache for IPv4 environments. It is responsible for mapping IP addresses to MAC addresses, a fundamental requirement for Layer 2 network communication. Beyond basic IP-to-MAC mapping, this implementation includes advanced features such as:

-   Stateful ARP entries with defined lifetimes and transitions (Incomplete, Reachable, Stale, Probe, Delay, Failed).
-   Proxy ARP functionality, allowing the device to answer ARP requests on behalf of other hosts.
-   Conflict detection and configurable resolution policies for IP-MAC mapping disputes.
-   Handling of Gratuitous ARP (GARP) packets with configurable policies, including rate limiting.
-   MAC address flap detection and mitigation.
-   Support for backup MAC addresses and fast failover mechanisms.
-   LRU (Least Recently Used) eviction strategy when cache limits are reached.
-   Interface-specific Proxy ARP configurations and MAC addresses.
-   Extensibility through virtual methods for integration with logging, alerting, DHCP snooping, and routing policy systems.

## Key Concepts

### `ARPEntry` and `ARPState`

Each entry in the cache is represented by an `ARPEntry` struct, which includes:
-   `mac_addr_t mac`: The primary MAC address.
-   `ARPState state`: The current state of the entry (e.g., `INCOMPLETE`, `REACHABLE`, `STALE`, `PROBE`, `DELAY`, `FAILED`).
-   `std::chrono::steady_clock::time_point timestamp`: Last update/confirmation time.
-   `probe_count`: Number of ARP probes sent.
-   `pending_packets`: Queue for packets awaiting resolution for this IP.
-   `backup_macs`: A list of backup MAC addresses for failover.
-   `flap_count`, `last_mac_update_time`: For MAC flap detection.

The state transitions are managed by the `age_entries()` method based on configurable timeouts.

### Policies

-   **`ConflictPolicy`**: Defines behavior when a new ARP packet suggests a different MAC for an existing IP.
    -   `LOG_ONLY`: Log the conflict.
    -   `DROP_NEW`: Keep the existing entry, ignore the new information.
    -   `UPDATE_EXISTING` (Default): Update the entry with the new MAC.
    -   `ALERT_SYSTEM`: Log and trigger a system alert (defaults to also updating).
-   **`GratuitousArpPolicy`**: Defines behavior for GARP packets.
    -   `PROCESS` (Default): Treat as a normal ARP.
    -   `LOG_AND_PROCESS`: Log then process.
    -   `RATE_LIMIT_AND_PROCESS`: Apply rate limiting before processing.
    -   `DROP_IF_CONFLICT`: Drop GARP if it conflicts with an existing, valid entry.

### Timeouts and Configuration

The cache's behavior is heavily influenced by configurable time parameters:
-   `reachable_time_sec_`: How long an entry stays `REACHABLE`.
-   `stale_time_sec_`: How long an entry stays `STALE` before probing.
-   `probe_retransmit_interval_sec_`: Base interval for ARP probes.
-   `max_probe_backoff_interval_sec_`: Max backoff for probes.
-   `failed_entry_lifetime_sec_`: How long a `FAILED` entry persists.
-   `delay_duration_sec_`: Duration of the `DELAY` state.
-   `flap_detection_window_sec_`, `max_flaps_allowed_`: Parameters for MAC flap detection.
-   `max_cache_size_`: Maximum number of entries.
-   `gratuitous_arp_min_interval_ms_`: Rate limit interval for GARPs.

## Public Interface Highlights

### Constructor
```cpp
explicit ARPCache(
    const mac_addr_t& dev_mac, // MAC of this device
    std::chrono::seconds reachable_time = std::chrono::seconds(300),
    // ... other timing and policy parameters ...
    size_t max_cache_size = 1024,
    ConflictPolicy conflict_pol = ConflictPolicy::UPDATE_EXISTING,
    GratuitousArpPolicy garp_pol = GratuitousArpPolicy::PROCESS,
    std::chrono::milliseconds gratuitous_arp_min_interval = std::chrono::milliseconds(1000)
);
```

### Core Operations
-   **`bool lookup(uint32_t ip, mac_addr_t& mac_out)`**:
    Attempts to find the MAC for `ip`. May return a cached MAC, use proxy ARP, or initiate ARP resolution (marking entry `INCOMPLETE` and returning `false`).
-   **`void add_entry(uint32_t ip, const mac_addr_t& new_mac, ARPPacketType packet_type = ARPPacketType::UNKNOWN)`**:
    Adds or updates an ARP entry. Handles conflict resolution, GARP policies, and flap detection.
-   **`void age_entries()` / `void age_entries(std::chrono::steady_clock::time_point current_time)`**:
    Periodically called to update entry states, send probes, handle timeouts, and perform failovers.
-   **`void handle_link_down()`**:
    Clears the entire cache.

### Configuration
-   **`void add_proxy_subnet(uint32_t prefix, uint32_t mask, uint32_t interface_id)`**:
    Configures a subnet for proxy ARP on a specific interface.
-   **`void add_backup_mac(uint32_t ip, const mac_addr_t& backup_mac)`**:
    Adds a backup MAC for potential failover.
-   **`void set_conflict_policy(ConflictPolicy policy)`**, **`void set_gratuitous_arp_policy(GratuitousArpPolicy policy)`**:
    Setters for various policies.
-   **`void set_reachable_time(std::chrono::seconds time)`**, etc.:
    Setters for timeout and interval parameters.
-   **`void set_interface_mac(uint32_t interface_id, const mac_addr_t& mac)`**:
    Assigns a specific MAC to an interface for proxy ARP replies from that interface.
-   **`void enable_proxy_arp_on_interface(uint32_t interface_id)` / `void disable_proxy_arp_on_interface(uint32_t interface_id)`**:
    Controls proxy ARP on a per-interface basis.

### Extensibility (Virtual Methods)
-   `virtual void send_arp_request(uint32_t ip)`
-   `virtual void log_ip_conflict(uint32_t ip, const mac_addr_t& existing_mac, const mac_addr_t& new_mac)`
-   `virtual void trigger_alert(uint32_t ip, const mac_addr_t& existing_mac, const mac_addr_t& new_mac)`
-   `virtual bool is_ip_mac_dhcp_validated(uint32_t ip, const mac_addr_t& mac)`
-   `virtual bool is_ip_routable(uint32_t ip_address)`
    These can be overridden in derived classes to integrate with system-specific packet sending, logging, alerting, and policy enforcement mechanisms.

## Usage Examples

(Based on `examples/arp_cache_example.cpp`)

### Initializing and Basic Lookup/Add

```cpp
#include "arp_cache.h"
#include <iostream>

// Helper to print MAC
void print_mac_addr(const mac_addr_t& mac) { /* ... */ }

int main() {
    mac_addr_t device_mac = {0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    ARPCache cache(device_mac, std::chrono::seconds(60)); // Short reachable time for demo

    uint32_t target_ip = 0xC0A80001; // 192.168.0.1
    mac_addr_t target_mac_actual = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    mac_addr_t resolved_mac;

    // Initial lookup (cache miss)
    if (!cache.lookup(target_ip, resolved_mac)) {
        std::cout << "IP 192.168.0.1 not in cache. ARP request would be sent." << std::endl;
        // In a real system, an ARP reply would eventually call add_entry
    }

    // Simulate receiving an ARP reply
    cache.add_entry(target_ip, target_mac_actual);
    std::cout << "Added entry for 192.168.0.1." << std::endl;

    if (cache.lookup(target_ip, resolved_mac)) {
        std::cout << "Lookup for 192.168.0.1 successful. MAC: ";
        print_mac_addr(resolved_mac);
    }
}
```

### Proxy ARP Configuration

```cpp
// ... (ARPCache initialized as above) ...
mac_addr_t interface1_mac = {0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0x01};
uint32_t interface1_id = 1;
cache.set_interface_mac(interface1_id, interface1_mac);
cache.enable_proxy_arp_on_interface(interface1_id);

uint32_t proxy_subnet_prefix = 0x0A000000; // 10.0.0.0
uint32_t proxy_subnet_mask = 0xFF000000;   // /8
cache.add_proxy_subnet(proxy_subnet_prefix, proxy_subnet_mask, interface1_id);
std::cout << "Configured Proxy ARP for 10.0.0.0/8 on interface " << interface1_id << std::endl;

uint32_t ip_in_proxied_subnet = 0x0A010203; // 10.1.2.3
mac_addr_t proxy_resolved_mac;

// Assume an ARP request for ip_in_proxied_subnet is received on interface1_id
// The system would call resolve_proxy_arp:
if (cache.resolve_proxy_arp(ip_in_proxied_subnet, interface1_id, proxy_resolved_mac)) {
    std::cout << "Proxy ARP resolution for 10.1.2.3 successful. Respond with MAC: ";
    print_mac_addr(proxy_resolved_mac); // Should be interface1_mac
    // Then, the system would send an ARP reply using proxy_resolved_mac
} else {
    // If the system also uses lookup for proxy, it might look like:
    // if (cache.lookup(ip_in_proxied_subnet, proxy_resolved_mac)) { ... }
    // The current lookup implementation in the header shows it can directly resolve proxy ARP.
    std::cout << "Proxy ARP resolution for 10.1.2.3 failed or direct lookup handled it." << std::endl;
}
```

### Handling Conflicts

```cpp
// ... (ARPCache initialized, target_ip and target_mac_actual added) ...
cache.set_conflict_policy(ConflictPolicy::UPDATE_EXISTING); // Default, but explicit for demo

mac_addr_t conflicting_mac = {0x11, 0x22, 0x33, 0xAA, 0xBB, 0xCC};
std::cout << "Introducing a conflicting MAC for 192.168.0.1." << std::endl;
// This simulates receiving an ARP packet (e.g., GARP) for target_ip with conflicting_mac
cache.add_entry(target_ip, conflicting_mac, ARPPacketType::GRATUITOUS_ANNOUNCEMENT);
// ARPCache's log_ip_conflict (stderr by default) should show a warning.

mac_addr_t current_mac;
if (cache.lookup(target_ip, current_mac)) {
    std::cout << "After conflict, MAC for 192.168.0.1 is: ";
    print_mac_addr(current_mac); // Should be conflicting_mac due to UPDATE_EXISTING
}
```

### Aging and State Transitions
Proper demonstration of aging requires controlling time. In a real system, `age_entries()` would be called periodically (e.g., every second).

```cpp
// Conceptual:
// cache.add_entry(ip, mac); // Entry is REACHABLE
// wait(reachable_time_sec_ + 1 second);
// cache.age_entries(); // Entry should transition to STALE
// wait(stale_time_sec_ + 1 second);
// cache.age_entries(); // Entry should transition to PROBE, send_arp_request called
```

## Dependencies
- `<chrono>`, `<unordered_map>`, `<vector>`, `<array>`, `<list>`, `<queue>`, `<algorithm>`, `<cstdio>` (for default logging).

The `ARPCache` provides a feature-rich solution for managing ARP information in networked devices, balancing correctness, performance, and robustness against common network events.

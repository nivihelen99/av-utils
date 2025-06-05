#include "../include/arp_cache.h" // Adjust path as necessary
#include <iostream>
#include <vector>
#include <array>
#include <thread> // For std::this_thread::sleep_for
#include <chrono> // For std::chrono::seconds

// For examples, we don't have a mock framework.
// We'll assume the send_arp_request in ARPCache prints to stdout or is a no-op.
// If ARPCache was designed with a pluggable sender, we could set a verbose one.

void print_mac(const std::array<uint8_t, 6>& mac) {
    for (size_t i = 0; i < mac.size(); ++i) {
        printf("%02x", mac[i]);
        if (i < mac.size() - 1) printf(":");
    }
    printf("\n");
}

int main() {
    std::cout << "ARP Cache Example\n" << std::endl;

    std::array<uint8_t, 6> device_mac = {0x00, 0x5A, 0x1C, 0x01, 0x02, 0x03};
    ARPCache arp_cache(device_mac);
    std::cout << "ARPCache created with device MAC: ";
    print_mac(device_mac);
    std::cout << std::endl;

    // 1. Basic Add, Lookup
    std::cout << "--- Basic Add & Lookup ---" << std::endl;
    uint32_t ip1 = 0xC0A8010A; // 192.168.1.10
    std::array<uint8_t, 6> mac1 = {0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F};
    std::array<uint8_t, 6> mac_out;

    std::cout << "Looking up IP 192.168.1.10 (should be a cache miss, trigger ARP request):" << std::endl;
    if (!arp_cache.lookup(ip1, mac_out)) {
        std::cout << "  IP 192.168.1.10 not found in cache. ARP request would be sent." << std::endl;
    }

    std::cout << "Adding entry for 192.168.1.10 -> "; print_mac(mac1);
    arp_cache.add_entry(ip1, mac1);

    if (arp_cache.lookup(ip1, mac_out)) {
        std::cout << "  Lookup hit for 192.168.1.10. MAC: "; print_mac(mac_out);
        if (mac_out != mac1) std::cerr << "  ERROR: MAC mismatch!" << std::endl;
    } else {
        std::cerr << "  ERROR: Lookup failed after adding entry!" << std::endl;
    }
    std::cout << std::endl;

    // 2. Gratuitous ARP / IP Conflict Detection
    std::cout << "--- Gratuitous ARP / IP Conflict ---" << std::endl;
    uint32_t ip_conflict = 0xC0A8010B; // 192.168.1.11
    std::array<uint8_t, 6> mac_orig = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01};
    std::array<uint8_t, 6> mac_new_conflict = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x02};

    std::cout << "Adding initial entry for 192.168.1.11 -> "; print_mac(mac_orig);
    arp_cache.add_entry(ip_conflict, mac_orig);

    std::cout << "Attempting to add same IP (192.168.1.11) with a DIFFERENT MAC ("; print_mac(mac_new_conflict);
    std::cout << "  (ARPCache should print a warning to stderr if conflict detected)" << std::endl;
    arp_cache.add_entry(ip_conflict, mac_new_conflict); // This should trigger the warning in ARPCache

    if (arp_cache.lookup(ip_conflict, mac_out) && mac_out == mac_new_conflict) {
        std::cout << "  IP 192.168.1.11 now resolved to new MAC: "; print_mac(mac_out);
    } else {
        std::cerr << "  ERROR: IP conflict update failed or MAC is incorrect." << std::endl;
    }
    std::cout << std::endl;

    // 3. Proxy ARP
    std::cout << "--- Proxy ARP ---" << std::endl;
    uint32_t proxy_prefix = 0x0A000000; // 10.0.0.0
    uint32_t proxy_mask = 0xFF000000;   // /8
    arp_cache.add_proxy_subnet(proxy_prefix, proxy_mask);
    std::cout << "Added proxy ARP for subnet 10.0.0.0/8." << std::endl;

    uint32_t ip_in_proxy = 0x0A010203; // 10.1.2.3
    std::cout << "Looking up IP 10.1.2.3 (in proxy subnet, not in cache):" << std::endl;
    if (arp_cache.lookup(ip_in_proxy, mac_out)) {
        std::cout << "  Proxy ARP lookup successful. MAC returned: "; print_mac(mac_out);
        if (mac_out != device_mac) std::cerr << "  ERROR: Proxy ARP returned incorrect MAC!" << std::endl;
    } else {
        std::cerr << "  ERROR: Proxy ARP lookup failed!" << std::endl;
    }
    std::cout << std::endl;

    // 4. Fast Failover
    std::cout << "--- Fast Failover ---" << std::endl;
    uint32_t ip_failover = 0xC0A8010C; // 192.168.1.12
    std::array<uint8_t, 6> primary_mac = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
    std::array<uint8_t, 6> backup_mac1 = {0x22, 0x22, 0x22, 0x22, 0x22, 0x22};
    std::array<uint8_t, 6> backup_mac2 = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33};

    std::cout << "Adding entry for 192.168.1.12 with primary MAC: "; print_mac(primary_mac);
    arp_cache.add_entry(ip_failover, primary_mac);
    std::cout << "Adding backup MAC for 192.168.1.12: "; print_mac(backup_mac1);
    arp_cache.add_backup_mac(ip_failover, backup_mac1);
    std::cout << "Adding another backup MAC for 192.168.1.12: "; print_mac(backup_mac2);
    arp_cache.add_backup_mac(ip_failover, backup_mac2);

    // Simulate primary MAC becoming STALE/PROBE (conceptually)
    // In a real scenario, this happens due to lack of traffic or failed probes.
    // The `lookup` in ARPCache has logic to use a backup if primary is STALE/PROBE/DELAY.
    // To demonstrate this, we'd need to manually set the state or control time.
    // For this example, we'll just describe the scenario.
    std::cout << "Conceptual: If primary MAC ("; print_mac(primary_mac);
    std::cout << "  for 192.168.1.12 becomes STALE/PROBE, the next lookup *should* failover." << std::endl;
    std::cout << "  (ARPCache may print an INFO message to stderr if failover occurs in lookup)" << std::endl;
    // Example: if (arp_cache.lookup(ip_failover, mac_out) && mac_out == backup_mac1) {
    //    std::cout << "  Successfully failed over to backup MAC: "; print_mac(mac_out);
    // }

    // To demonstrate age_entries failover, we'd need to simulate probe failures.
    // This is hard to show in a simple example without internal state manipulation.
    std::cout << "Conceptual: If primary MAC fails MAX_PROBES in age_entries, it *should* failover." << std::endl;
    std::cout << "  (ARPCache may print an INFO message to stderr if failover occurs in age_entries)" << std::endl;
    std::cout << std::endl;

    std::cout << "ARP Cache example finished." << std::endl;
    return 0;
}

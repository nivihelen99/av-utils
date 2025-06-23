# Optimized TCAM and VRF Routing Manager (`tcam.h`)

## Overview

The `tcam.h` header provides a sophisticated C++ system for simulating Ternary Content-Addressable Memory (TCAM) functionality, primarily geared towards policy-based routing and packet classification. It includes the `OptimizedTCAM` class, which implements the core TCAM logic with multiple lookup strategies, and the `VrfRoutingTableManager` class, which manages multiple TCAM instances for Virtual Routing and Forwarding (VRF) support.

This system allows defining complex matching rules based on various packet header fields (IPs, ports, protocol, EtherType), associating actions and priorities with these rules, and performing efficient lookups.

## Core Components

### 1. `OptimizedTCAM`
This class simulates a TCAM and includes several advanced features:

-   **Rule Definition (`WildcardFields`, internal `Rule` struct):**
    -   Rules match on Source/Destination IP (with masks), Source/Destination Port (exact or ranges), Protocol (with mask), and EtherType (with mask).
    -   Each rule has a `priority` (higher numeric value is higher priority) and an `action` (integer).
    -   Internal `Rule` struct also stores hit counts, timestamps for aging, unique ID, and an active flag.
-   **Lookup Strategies:**
    -   **Longest Prefix Match (LPM):** While the name suggests TCAM (exact match with wildcards), the `PolicyRoutingTree` name (which `OptimizedTCAM` seems to be or evolved from) and IP prefix handling imply LPM capabilities for IP fields, which are then combined with exact/wildcard matching for other fields. The `lookup` in `PolicyRoutingTree` performs LPM.
    -   **Optimized Lookups:** `OptimizedTCAM` can employ different strategies for matching:
        -   Linear scan of sorted rules.
        -   Decision Tree: A pre-built tree to guide packet classification.
        -   Bitmap TCAM: Uses bitsets for fast pre-filtering of rules based on individual packet bytes.
    -   The `lookup_single` method adaptively chooses a strategy.
-   **Port Range Handling:** Efficiently handles rules matching ranges of source or destination ports.
-   **Rule Management:**
    -   Adding rules with priorities and actions (`add_rule_with_ranges`).
    -   Atomic batch updates (`update_rules_atomic`) for adding/deleting multiple rules.
    -   Soft deletion of rules (`delete_rule`, `is_active` flag).
    -   Rule aging based on creation or last-hit time (`age_rules`).
    -   Detection and compaction/elimination of conflicting, shadowed, or redundant rules.
-   **Observability & Statistics:**
    -   Per-rule hit counts and timestamps (`RuleStats`).
    -   Overall rule utilization metrics (`RuleUtilizationMetrics`).
    -   Lookup latency metrics (`AggregatedLatencyMetrics`).
-   **Backup and Restore:** Methods to serialize active rules to/from a stream.
-   **ECMP (Equal Cost Multi-Path) Support:** `getEqualCostPaths` and `selectEcmpPathUsingFlowHash` allow for load balancing over multiple best paths.

### 2. `VrfRoutingTableManager`
-   Manages multiple independent `OptimizedTCAM` instances, each associated with a VRF ID (`uint32_t`).
-   Provides a unified interface to add routes, perform lookups, and display routing tables for specific VRFs or all VRFs.

### Helper Structs
-   **`WildcardFields`**: A user-friendly struct to define rule matching criteria before they are packed into the internal TCAM format.
-   **`RouteAttributes`**: (Defined in `policy_radix.h` but used conceptually here if `OptimizedTCAM` is part of a larger routing system) Defines properties of a route like next hop, AS path, MED, Local Preference, DSCP marking, rate limiting. *Self-correction: `RouteAttributes` and `PolicyRule` are indeed defined within `tcam.h` itself, not just from `policy_radix.h`.*
-   **`PacketInfo`**: (Also defined in `policy_radix.h` and used here) Represents packet fields for lookup. *Self-correction: `PacketInfo` is defined within `tcam.h`.*

## Key Public Interfaces

### `OptimizedTCAM`
-   **`OptimizedTCAM()`**: Constructor.
-   **`void add_rule_with_ranges(const WildcardFields& fields, int priority, int action)`**: Adds a new rule.
-   **`bool update_rules_atomic(const RuleUpdateBatch& batch)`**: Atomically applies a batch of add/delete operations.
-   **`int lookup_single(const std::vector<uint8_t>& packet, std::vector<std::string>* debug_trace_log = nullptr) const`**: Main lookup function, returns action of best matching rule or -1.
-   **`void displayRoutes() const`**: Prints the TCAM rules.
-   **`std::vector<Conflict> detect_conflicts() const`**: Detects conflicting rules.
-   **`std::vector<uint64_t> age_rules(...)`, `eliminate_shadowed_rules(...)`, `compact_redundant_rules(...)`**.
-   **`get_rule_stats()`, `get_all_rule_stats()`, `get_rule_utilization()`, `get_lookup_latency_metrics()`**.
-   **`void backup_rules(std::ostream& stream) const`, `bool restore_rules(std::istream& stream)`**.

*(For policy routing features like in `PolicyRoutingTree` involving `RouteAttributes` for next-hop selection, ECMP, etc., refer to `policy_radix.h` documentation if `OptimizedTCAM` is used as a component within that system. The `tcam.h` provided focuses more on the TCAM matching mechanism itself, with `action` being an integer. The `VrfRoutingTableManager` in the example, however, uses `PolicyRoutingTree::ipStringToInt` and the example's `RouteAttributes` has `nextHop`, indicating a blend or evolution of these concepts. The `OptimizedTCAM` in `tcam.h` does not directly use `RouteAttributes` or `PolicyRule` from `policy_radix.h` in its core TCAM rule structure, but the example test file `tcam_test.cpp` re-introduces these structs, suggesting `OptimizedTCAM` is intended to be used in conjunction with such policy elements.)*

*Self-correction based on full header content:* `tcam.h` *does* define its own `PolicyRule` and `RouteAttributes` (though `RouteAttributes` is minimal compared to the one in `policy_radix.h`, and `PolicyRule` in `tcam.h` is not used by `OptimizedTCAM::add_rule_with_ranges`). The primary rule addition method `add_rule_with_ranges` takes `WildcardFields`, `priority`, and `action`. The more complex policy routing seems to be in `policy_radix.h`. The `OptimizedTCAM` is a general TCAM. The `VrfRoutingTableManager` in `tcam.h` seems to wrap `PolicyRoutingTree` (which is defined in `policy_radix.h`). This is confusing.

**Revised Understanding:**
`tcam.h` defines `OptimizedTCAM` which is a general TCAM matching byte patterns.
`policy_radix.h` defines `PolicyRoutingTree` which uses a Radix Tree and seems to be the one using `PolicyRule` and richer `RouteAttributes` for routing decisions.
The example `use_policy.cpp` (associated with `policy_radix.h`) uses `VrfRoutingTableManager`.
The example `tcam_test.cpp` (associated with `tcam.h`) tests `OptimizedTCAM`.

The `VrfRoutingTableManager` inside `tcam.h` uses `PolicyRoutingTree`. This suggests that `tcam.h` might be intended to provide general TCAM capabilities, and `PolicyRoutingTree` (from `policy_radix.h`) might *use* an `OptimizedTCAM` internally, or they are related components in a larger system.

Given the plan step is to document `tcam.h`, I will focus on `OptimizedTCAM`. The `VrfRoutingTableManager` seems to be a higher-level construct that *uses* the concepts from `policy_radix.h` rather than being solely dependent on `OptimizedTCAM` as defined in `tcam.h`. This is somewhat tangled.

For clarity, this README will primarily describe `OptimizedTCAM`.

## `OptimizedTCAM` Usage Examples

(Based on `tests/tcam_test.cpp`)

### Defining Rules and Basic Lookup

```cpp
#include "tcam.h" // Adjust path as needed
#include <iostream>
#include <vector>

// Helper to create a packet byte vector (example structure)
std::vector<uint8_t> create_packet(uint32_t src_ip, uint32_t dst_ip,
                                   uint16_t src_port, uint16_t dst_port,
                                   uint8_t proto, uint16_t eth_type) {
    std::vector<uint8_t> p(15);
    // Fill packet bytes (order matches OptimizedTCAM internal packing)
    p[0] = (src_ip >> 24) & 0xFF; p[1] = (src_ip >> 16) & 0xFF;
    p[2] = (src_ip >> 8) & 0xFF; p[3] = src_ip & 0xFF;
    p[4] = (dst_ip >> 24) & 0xFF; p[5] = (dst_ip >> 16) & 0xFF;
    p[6] = (dst_ip >> 8) & 0xFF; p[7] = dst_ip & 0xFF;
    p[8] = (src_port >> 8) & 0xFF; p[9] = src_port & 0xFF;
    p[10] = (dst_port >> 8) & 0xFF; p[11] = dst_port & 0xFF;
    p[12] = proto;
    p[13] = (eth_type >> 8) & 0xFF; p[14] = eth_type & 0xFF;
    return p;
}

int main() {
    OptimizedTCAM tcam;

    // Define rule fields using WildcardFields struct
    OptimizedTCAM::WildcardFields rule1_fields{};
    rule1_fields.src_ip = 0x0A000001; // 10.0.0.1
    rule1_fields.src_ip_mask = 0xFFFFFFFF; // /32
    rule1_fields.dst_ip = 0xC0A80101; // 192.168.1.1
    rule1_fields.dst_ip_mask = 0xFFFFFFFF; // /32
    rule1_fields.src_port_min = 1024; rule1_fields.src_port_max = 1024; // Exact port
    rule1_fields.dst_port_min = 80;   rule1_fields.dst_port_max = 80;   // Exact port
    rule1_fields.protocol = 6;        rule1_fields.protocol_mask = 0xFF; // TCP
    rule1_fields.eth_type = 0x0800;   rule1_fields.eth_type_mask = 0xFFFF; // IPv4

    tcam.add_rule_with_ranges(rule1_fields, 100 /*priority*/, 1 /*action*/);

    // Create a packet that matches this rule
    std::vector<uint8_t> packet1 = create_packet(0x0A000001, 0xC0A80101, 1024, 80, 6, 0x0800);
    int action_result = tcam.lookup_single(packet1);
    std::cout << "Packet 1 lookup action: " << action_result << std::endl; // Expected: 1

    // Create a packet that does not match
    std::vector<uint8_t> packet2 = create_packet(0x0A000002, 0xC0A80101, 1024, 80, 6, 0x0800);
    action_result = tcam.lookup_single(packet2);
    std::cout << "Packet 2 lookup action: " << action_result << std::endl; // Expected: -1 (no match)
}
```

### Using Port Ranges

```cpp
#include "tcam.h"
#include <iostream>
#include <vector>

// create_packet helper as above

int main() {
    OptimizedTCAM tcam;
    OptimizedTCAM::WildcardFields rule_fields{};
    // Match any source IP, any destination IP
    rule_fields.src_ip_mask = 0x00000000;
    rule_fields.dst_ip_mask = 0x00000000;
    // Match source port in range 1000-2000
    rule_fields.src_port_min = 1000; rule_fields.src_port_max = 2000;
    // Match any destination port
    rule_fields.dst_port_min = 0;    rule_fields.dst_port_max = 0xFFFF;
    rule_fields.protocol = 17;       rule_fields.protocol_mask = 0xFF; // UDP
    rule_fields.eth_type = 0x0800;   rule_fields.eth_type_mask = 0xFFFF;

    tcam.add_rule_with_ranges(rule_fields, 50, 5);

    // Packet with source port in range
    std::vector<uint8_t> packet_in_range = create_packet(0x01020304, 0x05060708, 1500, 1234, 17, 0x0800);
    std::cout << "Packet (src_port 1500) action: " << tcam.lookup_single(packet_in_range) << std::endl; // Expected: 5

    // Packet with source port out of range
    std::vector<uint8_t> packet_out_range = create_packet(0x01020304, 0x05060708, 500, 1234, 17, 0x0800);
    std::cout << "Packet (src_port 500) action: " << tcam.lookup_single(packet_out_range) << std::endl; // Expected: -1
}
```

### Atomic Batch Updates

```cpp
#include "tcam.h"
#include <iostream>
#include <vector>

// create_packet and default WildcardFields helpers as above

int main() {
    OptimizedTCAM tcam;
    OptimizedTCAM::WildcardFields fields1 = {}; // Assume create_default_fields_for_conflict_tests or similar
    fields1.src_ip = 0x0A000001; fields1.src_ip_mask = 0xFFFFFFFF;
    fields1.dst_ip = 0xC0A80101; fields1.dst_ip_mask = 0xFFFFFFFF;
    fields1.protocol = 6; fields1.protocol_mask = 0xFF;
    fields1.eth_type = 0x0800; fields1.eth_type_mask = 0xFFFF;
    fields1.src_port_min=1024; fields1.src_port_max=1024;
    fields1.dst_port_min=80; fields1.dst_port_max=80;


    // Initial rule
    tcam.add_rule_with_ranges(fields1, 100, 1); // Rule ID 0
    uint64_t original_rule_id = 0; // Assuming next_rule_id starts at 0 and increments

    OptimizedTCAM::RuleUpdateBatch batch;
    // Add a new rule
    OptimizedTCAM::WildcardFields fields2 = fields1;
    fields2.src_ip = 0x0A000002; // Different source IP
    batch.push_back(OptimizedTCAM::RuleOperation::AddRule(fields2, 90, 2));

    // Delete the original rule
    batch.push_back(OptimizedTCAM::RuleOperation::DeleteRule(original_rule_id));

    bool success = tcam.update_rules_atomic(batch);
    std::cout << "Atomic batch update successful: " << std::boolalpha << success << std::endl;

    if (success) {
        std::cout << "Lookup for original rule's packet: "
                  << tcam.lookup_single(create_packet(0x0A000001, 0xC0A80101, 1024, 80, 6, 0x0800))
                  << std::endl; // Expected: -1 (deleted)
        std::cout << "Lookup for new rule's packet: "
                  << tcam.lookup_single(create_packet(0x0A000002, 0xC0A80101, 1024, 80, 6, 0x0800))
                  << std::endl; // Expected: 2 (added)
    }
}
```

## Dependencies
- Standard C++ libraries: `<vector>`, `<ostream>`, `<istream>`, `<cctype>`, `<sstream>`, `<unordered_map>`, `<bitset>`, `<memory>`, `<algorithm>`, `<limits>`, `<chrono>`, `<numeric>`, `<map>`, `<array>`, `<string>`, `<utility>`, `<cmath>`, `<optional>`.
- Network headers: `<arpa/inet.h>` (for IP string conversion).
- SIMD headers: `<immintrin.h>` (for AVX2, though currently a placeholder).

The `OptimizedTCAM` provides a powerful engine for rule-based packet matching with various performance optimizations and management features.
The `VrfRoutingTableManager` (also in `tcam.h`) builds upon this by managing multiple TCAM instances for VRF contexts, likely integrating with `PolicyRoutingTree` from `policy_radix.h` for full policy routing decisions.

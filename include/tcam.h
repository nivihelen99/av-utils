#pragma once

#include <vector>
#include <unordered_map>
#include <bitset>
#include <memory>
#include <algorithm>
#include <immintrin.h> // For SIMD operations

// Advanced TCAM with multiple optimization techniques
class OptimizedTCAM {
private:
    struct Rule {
        std::vector<uint8_t> value;
        std::vector<uint8_t> mask;
        int priority;
        int action;
        uint32_t range_id;  // For range compression
    };
    
    // 1. Range Compression for port ranges
    struct RangeEntry {
        uint16_t min_port, max_port;
        uint32_t range_id;
    };
    
    // 2. Decision Tree Node for hierarchical lookup
    struct DecisionNode {
        int field_offset;           // Which byte to examine
        uint8_t test_value;
        uint8_t mask;
        std::unique_ptr<DecisionNode> left, right;
        std::vector<int> rule_indices; // Rules that match at this node
    };
    
    // 3. Bitmap for parallel rule matching
    struct BitmapTCAM {
        static constexpr size_t MAX_RULES = 1024;
        std::array<std::bitset<MAX_RULES>, 256> value_bitmaps;  // Per byte value
        std::array<std::bitset<MAX_RULES>, 256> mask_bitmaps;   // Per byte mask
        size_t num_rules = 0;
        
        void add_rule(size_t rule_idx, uint8_t value, uint8_t mask) {
            if (rule_idx < MAX_RULES) {
                value_bitmaps[value].set(rule_idx);
                mask_bitmaps[mask].set(rule_idx);
                num_rules = std::max(num_rules, rule_idx + 1);
            }
        }
        
        std::bitset<MAX_RULES> lookup(uint8_t packet_byte) const {
            std::bitset<MAX_RULES> matches;
            matches.set(); // Start with all rules matching
            
            // For each possible mask value
            for (int mask_val = 0; mask_val < 256; ++mask_val) {
                if (mask_bitmaps[mask_val].any()) {
                    // Check if packet matches this mask pattern
                    for (int val = 0; val < 256; ++val) {
                        if (value_bitmaps[val].any()) {
                            if ((packet_byte & mask_val) == (val & mask_val)) {
                                matches &= (value_bitmaps[val] & mask_bitmaps[mask_val]);
                            } else {
                                matches &= ~(value_bitmaps[val] & mask_bitmaps[mask_val]);
                            }
                        }
                    }
                }
            }
            return matches;
        }
    };
    
    std::vector<Rule> rules;
    std::vector<RangeEntry> port_ranges;
    std::unique_ptr<DecisionNode> decision_tree;
    std::vector<BitmapTCAM> field_bitmaps; // One per field
    
    // OpenFlow-style wildcards support
    struct WildcardFields {
        uint32_t src_ip, src_ip_mask;
        uint32_t dst_ip, dst_ip_mask;
        uint16_t src_port_min, src_port_max;
        uint16_t dst_port_min, dst_port_max;
        uint8_t protocol, protocol_mask;
        uint16_t eth_type, eth_type_mask;
    };
    
public:
    // Add rule with range compression
    void add_rule_with_ranges(const WildcardFields& fields, int priority, int action) {
        Rule rule;
        rule.priority = priority;
        rule.action = action;
        
        // Compress port ranges
        if (fields.src_port_min != fields.src_port_max) {
            uint32_t range_id = add_port_range(fields.src_port_min, fields.src_port_max);
            rule.range_id = range_id;
        }
        
        // Convert to byte representation for TCAM
        rule.value.resize(14); // Simplified: src_ip(4) + dst_ip(4) + ports(4) + proto(1) + eth_type(2)
        rule.mask.resize(14);
        
        // Pack fields into bytes
        pack_ip(rule.value.data(), rule.mask.data(), 0, fields.src_ip, fields.src_ip_mask);
        pack_ip(rule.value.data(), rule.mask.data(), 4, fields.dst_ip, fields.dst_ip_mask);
        pack_ports(rule.value.data(), rule.mask.data(), 8, 
                  fields.src_port_min, fields.dst_port_min);
        rule.value[12] = fields.protocol;
        rule.mask[12] = fields.protocol_mask;
        
        rules.push_back(rule);
        rebuild_optimized_structures();
    }
    
    // SIMD-optimized lookup for multiple packets
    void lookup_batch(const std::vector<std::vector<uint8_t>>& packets,
                     std::vector<int>& results) {
        results.resize(packets.size());
        
        // Use SIMD for parallel processing
        #ifdef __AVX2__
        for (size_t i = 0; i + 8 <= packets.size(); i += 8) {
            lookup_simd_avx2(packets.data() + i, results.data() + i);
        }
        
        // Handle remaining packets
        for (size_t i = (packets.size() / 8) * 8; i < packets.size(); ++i) {
            results[i] = lookup_single(packets[i]);
        }
        #else
        for (size_t i = 0; i < packets.size(); ++i) {
            results[i] = lookup_single(packets[i]);
        }
        #endif
    }
    
    // Decision tree lookup (for sparse rule sets)
    int lookup_decision_tree(const std::vector<uint8_t>& packet) const {
        return traverse_decision_tree(decision_tree.get(), packet);
    }
    
    // Bitmap-based parallel lookup
    int lookup_bitmap(const std::vector<uint8_t>& packet) const {
        if (field_bitmaps.empty() || packet.empty()) return -1;
        
        std::bitset<1024> matches;
        matches.set(); // Start with all rules matching
        
        // Check each field
        for (size_t field = 0; field < std::min(packet.size(), field_bitmaps.size()); ++field) {
            auto field_matches = field_bitmaps[field].lookup(packet[field]);
            matches &= field_matches;
        }
        
        // Find highest priority match
        for (size_t i = 0; i < matches.size() && i < rules.size(); ++i) {
            if (matches[i]) {
                return rules[i].action;
            }
        }
        return -1;
    }
    
    // Traditional linear search (baseline)
    int lookup_linear(const std::vector<uint8_t>& packet) const {
        for (const auto& rule : rules) {
            if (matches_rule(packet, rule)) {
                return rule.action;
            }
        }
        return -1;
    }
    
    // Statistics and optimization
    struct LookupStats {
        size_t linear_lookups = 0;
        size_t decision_tree_lookups = 0;
        size_t bitmap_lookups = 0;
        size_t simd_lookups = 0;
        double avg_linear_time = 0.0;
        double avg_tree_time = 0.0;
        double avg_bitmap_time = 0.0;
    };
    
    LookupStats stats;
    
    void optimize_for_traffic_pattern(const std::vector<std::vector<uint8_t>>& sample_traffic) {
        // Analyze traffic to choose best lookup method
        auto start = std::chrono::high_resolution_clock::now();
        
        // Test linear
        for (const auto& packet : sample_traffic) {
            lookup_linear(packet);
        }
        auto linear_time = std::chrono::high_resolution_clock::now() - start;
        
        start = std::chrono::high_resolution_clock::now();
        // Test decision tree
        for (const auto& packet : sample_traffic) {
            lookup_decision_tree(packet);
        }
        auto tree_time = std::chrono::high_resolution_clock::now() - start;
        
        start = std::chrono::high_resolution_clock::now();
        // Test bitmap
        for (const auto& packet : sample_traffic) {
            lookup_bitmap(packet);
        }
        auto bitmap_time = std::chrono::high_resolution_clock::now() - start;
        
        // Choose optimal method based on performance
        stats.avg_linear_time = linear_time.count();
        stats.avg_tree_time = tree_time.count();
        stats.avg_bitmap_time = bitmap_time.count();
    }
    
private:
    uint32_t add_port_range(uint16_t min_port, uint16_t max_port) {
        uint32_t range_id = port_ranges.size();
        port_ranges.push_back({min_port, max_port, range_id});
        return range_id;
    }
    
    void pack_ip(uint8_t* value, uint8_t* mask, int offset, uint32_t ip, uint32_t ip_mask) {
        for (int i = 0; i < 4; ++i) {
            value[offset + i] = (ip >> (24 - i * 8)) & 0xFF;
            mask[offset + i] = (ip_mask >> (24 - i * 8)) & 0xFF;
        }
    }
    
    void pack_ports(uint8_t* value, uint8_t* mask, int offset, uint16_t src_port, uint16_t dst_port) {
        value[offset] = (src_port >> 8) & 0xFF;
        value[offset + 1] = src_port & 0xFF;
        value[offset + 2] = (dst_port >> 8) & 0xFF;
        value[offset + 3] = dst_port & 0xFF;
        
        // Full mask for exact port match
        mask[offset] = mask[offset + 1] = mask[offset + 2] = mask[offset + 3] = 0xFF;
    }
    
    bool matches_rule(const std::vector<uint8_t>& packet, const Rule& rule) const {
        for (size_t i = 0; i < rule.value.size() && i < packet.size(); ++i) {
            if (rule.mask[i] && ((rule.value[i] ^ packet[i]) & rule.mask[i])) {
                return false;
            }
        }
        return true;
    }
    
    int lookup_single(const std::vector<uint8_t>& packet) const {
        // Choose best method based on rule characteristics
        if (rules.size() < 16) {
            return lookup_linear(packet);
        } else if (decision_tree) {
            return lookup_decision_tree(packet);
        } else {
            return lookup_bitmap(packet);
        }
    }
    
    #ifdef __AVX2__
    void lookup_simd_avx2(const std::vector<uint8_t>* packets, int* results) {
        // Simplified SIMD implementation for 8 packets in parallel
        __m256i packet_data[8];
        
        // Load 8 packets into SIMD registers
        for (int i = 0; i < 8; ++i) {
            if (packets[i].size() >= 32) {
                packet_data[i] = _mm256_loadu_si256(
                    reinterpret_cast<const __m256i*>(packets[i].data()));
            }
        }
        
        // Check against rules in parallel
        for (const auto& rule : rules) {
            if (rule.value.size() >= 32) {
                __m256i rule_value = _mm256_loadu_si256(
                    reinterpret_cast<const __m256i*>(rule.value.data()));
                __m256i rule_mask = _mm256_loadu_si256(
                    reinterpret_cast<const __m256i*>(rule.mask.data()));
                
                for (int i = 0; i < 8; ++i) {
                    __m256i xor_result = _mm256_xor_si256(packet_data[i], rule_value);
                    __m256i masked = _mm256_and_si256(xor_result, rule_mask);
                    
                    if (_mm256_testz_si256(masked, masked)) {
                        results[i] = rule.action;
                        break; // First match wins (highest priority)
                    }
                }
            }
        }
    }
    #endif
    
    int traverse_decision_tree(const DecisionNode* node, 
                              const std::vector<uint8_t>& packet) const {
        if (!node) return -1;
        
        // Check rules at this node first
        for (int rule_idx : node->rule_indices) {
            if (matches_rule(packet, rules[rule_idx])) {
                return rules[rule_idx].action;
            }
        }
        
        // Traverse tree
        if (node->field_offset < packet.size()) {
            uint8_t field_value = packet[node->field_offset];
            if ((field_value & node->mask) == (node->test_value & node->mask)) {
                int result = traverse_decision_tree(node->left.get(), packet);
                if (result != -1) return result;
            }
            return traverse_decision_tree(node->right.get(), packet);
        }
        
        return -1;
    }
    
    void rebuild_optimized_structures() {
        // Sort rules by priority
        std::sort(rules.begin(), rules.end(),
                  [](const Rule& a, const Rule& b) {
                      return a.priority > b.priority;
                  });
        
        // Rebuild bitmap structures
        field_bitmaps.clear();
        if (!rules.empty() && !rules[0].value.empty()) {
            field_bitmaps.resize(rules[0].value.size());
            
            for (size_t rule_idx = 0; rule_idx < rules.size(); ++rule_idx) {
                for (size_t field = 0; field < rules[rule_idx].value.size(); ++field) {
                    field_bitmaps[field].add_rule(rule_idx, 
                                                 rules[rule_idx].value[field],
                                                 rules[rule_idx].mask[field]);
                }
            }
        }
        
        // Could also rebuild decision tree here
        build_decision_tree();
    }
    
    void build_decision_tree() {
        // Simplified decision tree construction
        // In practice, would use more sophisticated algorithms like C4.5
        decision_tree = std::make_unique<DecisionNode>();
        decision_tree->field_offset = 0; // Start with first field
        
        // This is a placeholder - real implementation would analyze
        // rule distribution to create optimal tree structure
    }
};


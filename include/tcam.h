#pragma once

#include <vector>
#include <unordered_map>
#include <bitset>
#include <memory>
#include <algorithm>
#include <immintrin.h> // For SIMD operations
#include <limits>      // Required for std::numeric_limits
#include <chrono>      // For std::chrono in optimize_for_traffic_pattern
#include <numeric>     // For std::iota
#include <map>         // For std::map (used in find_best_split)
#include <array>       // For std::array
#include <string>      // For std::string
#include <utility>     // For std::pair
#include <cmath>       // For std::log2, std::round
#include <numeric>     // For std::iota, already present but good to note for specificity parts
// <chrono> is already included higher up, but ensure it's there for this change.
// #include <chrono> // Not strictly needed here if already present globally

class OptimizedTCAM {
public: // Changed private to public for WildcardFields
    struct WildcardFields {
        uint32_t src_ip, src_ip_mask;
        uint32_t dst_ip, dst_ip_mask;
        uint16_t src_port_min, src_port_max;
        uint16_t dst_port_min, dst_port_max;
        uint8_t protocol, protocol_mask;
        uint16_t eth_type, eth_type_mask;
    };

    struct Conflict {
        size_t rule1_idx;
        size_t rule2_idx;
        std::string description;
    };

public: // For RuleOperation and RuleUpdateBatch
    struct RuleOperation {
        enum class Type {
            ADD,
            DELETE
        };

        Type type;

        // Fields for ADD operation
        WildcardFields fields;
        int priority = 0;
        int action = 0;

        // Field for DELETE operation
        uint64_t rule_id_to_delete = 0;

        // Constructors for convenience
        RuleOperation() : type(Type::ADD) {}

        static RuleOperation AddRule(const WildcardFields& wf, int prio, int act) {
            RuleOperation op;
            op.type = Type::ADD;
            op.fields = wf;
            op.priority = prio;
            op.action = act;
            return op;
        }

        static RuleOperation DeleteRule(uint64_t id) {
            RuleOperation op;
            op.type = Type::DELETE;
            op.rule_id_to_delete = id;
            return op;
        }
    };

    using RuleUpdateBatch = std::vector<RuleOperation>;

private:
    struct Rule {
        std::vector<uint8_t> value;
        std::vector<uint8_t> mask;
        int priority;
        int action;
        uint32_t src_port_range_id;
        uint32_t dst_port_range_id;
        uint64_t id;
        bool is_active;
        std::chrono::steady_clock::time_point creation_time;
    };
    
    struct RangeEntry {
        uint16_t min_port, max_port;
        uint32_t range_id;
    };
    
    struct DecisionNode {
        int field_offset; // -1 for leaf, or field to test
        uint8_t test_value; // Value to test against (if field_offset != -1)
        uint8_t mask;       // Mask to use for test (if field_offset != -1)
        std::unique_ptr<DecisionNode> left, right;
        std::vector<int> rule_indices; // Rules stored at this node (leaf or wildcarded rules)
    };
    
    struct BitmapTCAM {
        static constexpr size_t MAX_RULES = 1024;
        std::array<std::bitset<MAX_RULES>, 256> value_bitmaps;
        std::array<std::bitset<MAX_RULES>, 256> mask_bitmaps;
        size_t num_rules = 0;
        
        void add_rule(size_t rule_idx, uint8_t value_byte, uint8_t mask_byte) {
            if (rule_idx < MAX_RULES) {
                value_bitmaps[value_byte].set(rule_idx);
                mask_bitmaps[mask_byte].set(rule_idx);
                num_rules = std::max(num_rules, rule_idx + 1);
            }
        }
        
        std::bitset<MAX_RULES> lookup(uint8_t packet_byte) const {
            std::bitset<MAX_RULES> result_matches;
            for (int val_int = 0; val_int < 256; ++val_int) {
                for (int mask_int = 0; mask_int < 256; ++mask_int) {
                    uint8_t rule_field_value = static_cast<uint8_t>(val_int);
                    uint8_t rule_field_mask = static_cast<uint8_t>(mask_int);
                    std::bitset<MAX_RULES> rules_with_this_val_mask = value_bitmaps[rule_field_value] & mask_bitmaps[rule_field_mask];
                    if (rules_with_this_val_mask.any()) {
                        if ((packet_byte & rule_field_mask) == (rule_field_value & rule_field_mask)) {
                            result_matches |= rules_with_this_val_mask;
                        }
                    }
                }
            }
            std::bitset<MAX_RULES> final_valid_matches;
            if (num_rules > 0) {
                for (size_t i = 0; i < num_rules; ++i) {
                    if (i < MAX_RULES && result_matches[i]) {
                        final_valid_matches.set(i);
                    }
                }
            }
            return final_valid_matches;
        }
    };
    
    std::vector<Rule> rules;
    std::vector<RangeEntry> port_ranges;
    std::unique_ptr<DecisionNode> decision_tree;
    std::vector<BitmapTCAM> field_bitmaps;
    uint64_t next_rule_id = 0;
    
    // WildcardFields moved to public section

    std::unique_ptr<DecisionNode> build_tree_recursive(const std::vector<int>& rule_indices_for_node, int current_depth,
                                                       const int leaf_threshold, const int max_depth) {
        if (rule_indices_for_node.empty()) {
            return nullptr;
        }

        if (current_depth >= max_depth || rule_indices_for_node.size() <= static_cast<size_t>(leaf_threshold)) {
            auto leaf_node = std::make_unique<DecisionNode>();
            leaf_node->field_offset = -1;
            leaf_node->mask = 0;
            leaf_node->test_value = 0;
            leaf_node->rule_indices = rule_indices_for_node;
            return leaf_node;
        }

        int best_field_offset = -1;
        uint8_t best_test_value = 0;
        uint8_t best_mask_for_split = 0xFF;
        size_t min_child_rules_sum_sq = std::numeric_limits<size_t>::max();

        std::vector<int> candidate_field_offsets = {0, 1, 2, 3, 4, 5, 6, 7, 12};

        for (int field_off : candidate_field_offsets) {
            if (rules.empty() || rule_indices_for_node.empty() ||
                (rules[rule_indices_for_node[0]].value.size() <= static_cast<size_t>(field_off))) {
                continue;
            }

            std::map<uint8_t, int> value_counts;
            int rules_considered_for_split_on_this_field = 0;
            for (int rule_idx : rule_indices_for_node) {
                if (rules[rule_idx].mask.size() > static_cast<size_t>(field_off) &&
                    rules[rule_idx].value.size() > static_cast<size_t>(field_off) &&
                    rules[rule_idx].mask[field_off] == 0xFF) {
                    value_counts[rules[rule_idx].value[field_off]]++;
                    rules_considered_for_split_on_this_field++;
                }
            }

            if (value_counts.empty() || rules_considered_for_split_on_this_field < 2 || value_counts.size() < 2) {
                continue;
            }

            uint8_t current_test_val = 0;
            int max_freq = 0;
            for (const auto& pair_val : value_counts) { // Renamed pair to pair_val
                if (pair_val.second > max_freq) {
                    max_freq = pair_val.second;
                    current_test_val = pair_val.first;
                }
            }

            std::vector<int> left_children_rules, right_children_rules, current_node_rules_test;
            for (int rule_idx : rule_indices_for_node) {
                const auto& r = rules[rule_idx];
                if (r.mask.size() <= static_cast<size_t>(field_off) || r.value.size() <= static_cast<size_t>(field_off)) continue;

                if (r.mask[field_off] == 0x00) {
                    current_node_rules_test.push_back(rule_idx);
                } else if (r.mask[field_off] == 0xFF && r.value[field_off] == current_test_val) {
                    left_children_rules.push_back(rule_idx);
                } else {
                    right_children_rules.push_back(rule_idx);
                }
            }

            bool is_effective_split = false;
            size_t non_current_node_rules = rule_indices_for_node.size() - current_node_rules_test.size();
            if (non_current_node_rules > 0) {
                if ((!left_children_rules.empty() && left_children_rules.size() < non_current_node_rules) ||
                    (!right_children_rules.empty() && right_children_rules.size() < non_current_node_rules) ){
                     is_effective_split = true;
                }
                if ( (left_children_rules.size() == non_current_node_rules && right_children_rules.empty()) ||
                     (right_children_rules.size() == non_current_node_rules && left_children_rules.empty()) ) {
                     is_effective_split = true;
                }
            }

            if(is_effective_split) {
                size_t current_sum_sq = left_children_rules.size() * left_children_rules.size() +
                                        right_children_rules.size() * right_children_rules.size();
                if (best_field_offset == -1 || current_sum_sq < min_child_rules_sum_sq) {
                    min_child_rules_sum_sq = current_sum_sq;
                    best_field_offset = field_off;
                    best_test_value = current_test_val;
                }
            }
        }

        if (best_field_offset == -1) {
            auto leaf_node = std::make_unique<DecisionNode>();
            leaf_node->field_offset = -1;
            leaf_node->mask = 0;
            leaf_node->test_value = 0;
            leaf_node->rule_indices = rule_indices_for_node;
            return leaf_node;
        }

        auto node = std::make_unique<DecisionNode>();
        node->field_offset = best_field_offset;
        node->test_value = best_test_value;
        node->mask = best_mask_for_split;

        std::vector<int> final_left_indices, final_right_indices;
        for (int rule_idx : rule_indices_for_node) {
            const auto& r = rules[rule_idx];
            if (r.mask.size() <= static_cast<size_t>(best_field_offset) || r.value.size() <= static_cast<size_t>(best_field_offset)) continue;

            if (r.mask[best_field_offset] == 0x00) {
                node->rule_indices.push_back(rule_idx);
            } else if (r.mask[best_field_offset] == best_mask_for_split && r.value[best_field_offset] == best_test_value) {
                final_left_indices.push_back(rule_idx);
            } else {
                final_right_indices.push_back(rule_idx);
            }
        }

        if (final_left_indices.empty() && final_right_indices.empty() && !node->rule_indices.empty()) {
            node->field_offset = -1;
            node->mask = 0;
            return node;
        }

        node->left = build_tree_recursive(final_left_indices, current_depth + 1, leaf_threshold, max_depth);
        node->right = build_tree_recursive(final_right_indices, current_depth + 1, leaf_threshold, max_depth);

        if (node->rule_indices.empty() && !node->left && !node->right) {
            return nullptr;
        }
        return node;
    }

public:
    void add_rule_with_ranges(const WildcardFields& fields, int priority, int action) {
        Rule rule_obj;
        rule_obj.priority = priority;
        rule_obj.action = action;
        rule_obj.id = next_rule_id++;
        rule_obj.is_active = true;
        rule_obj.creation_time = std::chrono::steady_clock::now(); // Set creation timestamp
        rule_obj.src_port_range_id = std::numeric_limits<uint32_t>::max();
        rule_obj.dst_port_range_id = std::numeric_limits<uint32_t>::max();
        
        const size_t rule_byte_size = 15;
        rule_obj.value.resize(rule_byte_size);
        rule_obj.mask.resize(rule_byte_size);

        pack_ip(rule_obj.value.data(), rule_obj.mask.data(), 0, fields.src_ip, fields.src_ip_mask);
        pack_ip(rule_obj.value.data(), rule_obj.mask.data(), 4, fields.dst_ip, fields.dst_ip_mask);

        pack_ports(rule_obj.value.data(), rule_obj.mask.data(), 8, fields.src_port_min, fields.dst_port_min);

        if (fields.src_port_min == 0 && fields.src_port_max == 0xFFFF) {
            rule_obj.mask[8] = 0x00; rule_obj.mask[9] = 0x00;
        } else if (fields.src_port_min != fields.src_port_max) {
            if (fields.src_port_min < fields.src_port_max) {
                 rule_obj.src_port_range_id = add_port_range(fields.src_port_min, fields.src_port_max);
                 rule_obj.mask[8] = 0x00; rule_obj.mask[9] = 0x00;
            }
        }

        if (fields.dst_port_min == 0 && fields.dst_port_max == 0xFFFF) {
            rule_obj.mask[10] = 0x00; rule_obj.mask[11] = 0x00;
        } else if (fields.dst_port_min != fields.dst_port_max) {
             if (fields.dst_port_min < fields.dst_port_max) {
                rule_obj.dst_port_range_id = add_port_range(fields.dst_port_min, fields.dst_port_max);
                rule_obj.mask[10] = 0x00; rule_obj.mask[11] = 0x00;
            }
        }
        
        rule_obj.value[12] = fields.protocol;
        rule_obj.mask[12] = fields.protocol_mask;
        
        rule_obj.value[13] = static_cast<uint8_t>((fields.eth_type >> 8) & 0xFF);
        rule_obj.mask[13] = static_cast<uint8_t>((fields.eth_type_mask >> 8) & 0xFF);
        rule_obj.value[14] = static_cast<uint8_t>(fields.eth_type & 0xFF);
        rule_obj.mask[14] = static_cast<uint8_t>(fields.eth_type_mask & 0xFF);

        // 1. Remove inactive rules first
        rules.erase(std::remove_if(rules.begin(), rules.end(), [](const Rule& r){ return !r.is_active; }), rules.end());

        // 2. Define comparator
        auto rule_comparator = [this](const Rule& a, const Rule& b) {
            if (a.priority != b.priority) {
                return a.priority > b.priority; // Higher priority comes first
            }
            return this->calculate_specificity(a) > this->calculate_specificity(b); // Higher specificity comes first
        };

        // 3. Find insertion point in the now active and sorted (or soon to be fully sorted) list
        auto it = std::lower_bound(rules.begin(), rules.end(), rule_obj, rule_comparator);
        
        // 4. Insert the new rule
        rules.insert(it, rule_obj);

        // 5. Rebuild structures from (now active and sorted) rules
        // No need to call the full rebuild_optimized_structures, as rules are already active and sorted.
        // However, the full rebuild_optimized_structures first filters and sorts, which is what we've manually done.
        // So we call a new method that skips filtering and sorting.
        rebuild_optimized_structures_from_sorted_rules();
    }
    
    void lookup_batch(const std::vector<std::vector<uint8_t>>& packets, std::vector<int>& results) {
        results.resize(packets.size());
        #ifdef __AVX2__
        size_t num_batches = packets.size() / 8;
        for (size_t batch_idx = 0; batch_idx < num_batches; ++batch_idx) {
            lookup_simd_avx2(&packets[batch_idx * 8], &results[batch_idx * 8]);
        }
        for (size_t i = num_batches * 8; i < packets.size(); ++i) {
            results[i] = lookup_single(packets[i]);
        }
        #else
        for (size_t i = 0; i < packets.size(); ++i) {
            results[i] = lookup_single(packets[i]);
        }
        #endif
    }
    
    int lookup_decision_tree(const std::vector<uint8_t>& packet) const {
        if (!decision_tree) return -1;
        return traverse_decision_tree(decision_tree.get(), packet);
    }
    
    int lookup_bitmap(const std::vector<uint8_t>& packet) const {
        if (field_bitmaps.empty() || packet.empty() || rules.empty()) return -1;
        
        std::bitset<BitmapTCAM::MAX_RULES> matches_bs;
        matches_bs.set();
        
        for (size_t field_idx = 0; field_idx < field_bitmaps.size(); ++field_idx) {
            if (field_idx < packet.size()) {
                auto field_matches = field_bitmaps[field_idx].lookup(packet[field_idx]);
                matches_bs &= field_matches;
            } else {
                std::bitset<BitmapTCAM::MAX_RULES> rules_wildcarded_for_this_field;
                for(size_t rule_idx = 0; rule_idx < rules.size() && rule_idx < BitmapTCAM::MAX_RULES; ++rule_idx) {
                    if (rules[rule_idx].mask.size() > field_idx && rules[rule_idx].mask[field_idx] == 0x00) {
                        rules_wildcarded_for_this_field.set(rule_idx);
                    }
                }
                matches_bs &= rules_wildcarded_for_this_field;
            }
             if (!matches_bs.any()) break;
        }

        for (size_t i = 0; i < rules.size(); ++i) {
            if (i < BitmapTCAM::MAX_RULES && matches_bs[i]) {
                const auto& r = rules[i];
                bool port_ranges_match = true;
                if (r.src_port_range_id != std::numeric_limits<uint32_t>::max()) {
                    if (packet.size() < 10) { port_ranges_match = false; }
                    else {
                        uint16_t packet_src_port = (static_cast<uint16_t>(packet[8]) << 8) | packet[9];
                        if (r.src_port_range_id >= port_ranges.size()) { port_ranges_match = false; }
                        else {
                            const auto& range_entry = port_ranges[r.src_port_range_id];
                            if (packet_src_port < range_entry.min_port || packet_src_port > range_entry.max_port) {
                                port_ranges_match = false;
                            }
                        }
                    }
                }
                if (port_ranges_match && r.dst_port_range_id != std::numeric_limits<uint32_t>::max()) {
                    if (packet.size() < 12) { port_ranges_match = false; }
                    else {
                        uint16_t packet_dst_port = (static_cast<uint16_t>(packet[10]) << 8) | packet[11];
                        if (r.dst_port_range_id >= port_ranges.size()) { port_ranges_match = false; }
                        else {
                            const auto& range_entry = port_ranges[r.dst_port_range_id];
                            if (packet_dst_port < range_entry.min_port || packet_dst_port > range_entry.max_port) {
                                port_ranges_match = false;
                            }
                        }
                    }
                }

                if (port_ranges_match) {
                    return r.action;
                }
            }
        }
        return -1;
    }
    
    int lookup_linear(const std::vector<uint8_t>& packet) const {
        for (const auto& r : rules) {
            if (matches_rule(packet, r)) {
                return r.action;
            }
        }
        return -1;
    }
    
    struct LookupStats {
        size_t linear_lookups = 0;
        size_t decision_tree_lookups = 0;
        size_t bitmap_lookups = 0;
        size_t simd_lookups = 0;
        double avg_linear_time = 0.0;
        double avg_tree_time = 0.0;
        double avg_bitmap_time = 0.0;
    };
    mutable LookupStats stats;
    
    void optimize_for_traffic_pattern(const std::vector<std::vector<uint8_t>>& sample_traffic) {
        auto t_start = std::chrono::high_resolution_clock::now();
        for (const auto& packet : sample_traffic) lookup_linear(packet);
        auto t_linear_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - t_start);
        
        t_start = std::chrono::high_resolution_clock::now();
        for (const auto& packet : sample_traffic) lookup_decision_tree(packet);
        auto t_tree_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - t_start);
        
        t_start = std::chrono::high_resolution_clock::now();
        for (const auto& packet : sample_traffic) lookup_bitmap(packet);
        auto t_bitmap_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - t_start);
        
        if (!sample_traffic.empty()) {
            stats.avg_linear_time = static_cast<double>(t_linear_time.count()) / sample_traffic.size();
            stats.avg_tree_time = static_cast<double>(t_tree_time.count()) / sample_traffic.size();
            stats.avg_bitmap_time = static_cast<double>(t_bitmap_time.count()) / sample_traffic.size();
        }
    }
    
private:
    uint32_t add_port_range(uint16_t min_port, uint16_t max_port) {
        uint32_t new_range_id = static_cast<uint32_t>(port_ranges.size());
        port_ranges.push_back({min_port, max_port, new_range_id});
        return new_range_id;
    }
    
    void pack_ip(uint8_t* value_arr, uint8_t* mask_arr, int offset, uint32_t ip, uint32_t ip_mask) {
        for (int i = 0; i < 4; ++i) {
            value_arr[offset + i] = static_cast<uint8_t>((ip >> (24 - i * 8)) & 0xFF);
            mask_arr[offset + i] = static_cast<uint8_t>((ip_mask >> (24 - i * 8)) & 0xFF);
        }
    }
    
    void pack_ports(uint8_t* value_arr, uint8_t* mask_arr, int offset,
                    uint16_t src_port_val, uint16_t dst_port_val) {
        value_arr[offset + 0] = static_cast<uint8_t>((src_port_val >> 8) & 0xFF);
        value_arr[offset + 1] = static_cast<uint8_t>(src_port_val & 0xFF);
        mask_arr[offset + 0] = 0xFF; mask_arr[offset + 1] = 0xFF;
        value_arr[offset + 2] = static_cast<uint8_t>((dst_port_val >> 8) & 0xFF);
        value_arr[offset + 3] = static_cast<uint8_t>(dst_port_val & 0xFF);
        mask_arr[offset + 2] = 0xFF; mask_arr[offset + 3] = 0xFF;
    }
    
    bool matches_rule(const std::vector<uint8_t>& packet, const Rule& r) const {
        if (!r.is_active) return false;
        for (size_t i = 0; i < r.value.size(); ++i) {
            if (i >= packet.size()) {
                if (r.mask[i] != 0x00) return false;
                continue;
            }
            if (((packet[i] ^ r.value[i]) & r.mask[i]) != 0) return false;
        }
        if (r.src_port_range_id != std::numeric_limits<uint32_t>::max()) {
            if (packet.size() < 10) return false;
            if (r.src_port_range_id >= port_ranges.size()) return false;
            uint16_t packet_src_port = (static_cast<uint16_t>(packet[8]) << 8) | packet[9];
            const auto& range_entry = port_ranges[r.src_port_range_id];
            if (packet_src_port < range_entry.min_port || packet_src_port > range_entry.max_port) return false;
        }
        if (r.dst_port_range_id != std::numeric_limits<uint32_t>::max()) {
            if (packet.size() < 12) return false;
            if (r.dst_port_range_id >= port_ranges.size()) return false;
            uint16_t packet_dst_port = (static_cast<uint16_t>(packet[10]) << 8) | packet[11];
            const auto& range_entry = port_ranges[r.dst_port_range_id];
            if (packet_dst_port < range_entry.min_port || packet_dst_port > range_entry.max_port) return false;
        }
        return true;
    }
    
    int lookup_single(const std::vector<uint8_t>& packet) const {
        if (rules.empty()) return -1;
        if (stats.avg_linear_time > 0 && stats.avg_bitmap_time > 0 && !field_bitmaps.empty()) {
            if (stats.avg_bitmap_time < stats.avg_linear_time && (stats.avg_tree_time == 0 || stats.avg_bitmap_time < stats.avg_tree_time) ) {
                stats.bitmap_lookups++; return lookup_bitmap(packet);
            }
            if (stats.avg_tree_time > 0 && decision_tree && (stats.avg_tree_time < stats.avg_linear_time) ){
                stats.decision_tree_lookups++; return lookup_decision_tree(packet);
            }
        }
        if (rules.size() < 16 || (!decision_tree && field_bitmaps.empty())) {
             stats.linear_lookups++; return lookup_linear(packet);
        } else if (!field_bitmaps.empty()) {
             stats.bitmap_lookups++; return lookup_bitmap(packet);
        } else if (decision_tree) {
             stats.decision_tree_lookups++; return lookup_decision_tree(packet);
        }
        stats.linear_lookups++; return lookup_linear(packet);
    }
    
    #ifdef __AVX2__
    void lookup_simd_avx2(const std::vector<uint8_t>* packets_ptr, int* results_ptr) {
        for (int i = 0; i < 8; ++i) {
            results_ptr[i] = lookup_single(packets_ptr[i]);
        }
    }
    #endif
    
    int traverse_decision_tree(const DecisionNode* current_node, const std::vector<uint8_t>& packet) const {
        if (!current_node) return -1;
        for (int rule_idx_val : current_node->rule_indices) {
            if (static_cast<size_t>(rule_idx_val) < rules.size() && matches_rule(packet, rules[rule_idx_val])) {
                return rules[rule_idx_val].action;
            }
        }
        if (current_node->field_offset == -1 || (!current_node->left && !current_node->right)) {
             return -1;
        }

        if (current_node->field_offset >=0 && static_cast<size_t>(current_node->field_offset) < packet.size()) {
            uint8_t packet_field_byte = packet[current_node->field_offset];
            bool condition_met = ((packet_field_byte & current_node->mask) == (current_node->test_value & current_node->mask));
            if (condition_met) {
                if (current_node->left) {
                    int result = traverse_decision_tree(current_node->left.get(), packet);
                    if (result != -1) return result;
                }
            } else {
                if (current_node->right) {
                    int result = traverse_decision_tree(current_node->right.get(), packet);
                    if (result != -1) return result;
                }
            }
        }
        return -1;
    }

private:
    int popcount_manual(uint8_t n) const {
        int count = 0;
        while (n > 0) {
            n &= (n - 1);
            count++;
        }
        return count;
    }

    int calculate_specificity(const Rule& r) const {
        int specificity_score = 0;

        // Non-port fields specificity
        for (size_t k = 0; k < r.mask.size(); ++k) {
            bool is_src_port_field = (k == 8 || k == 9);
            bool is_dst_port_field = (k == 10 || k == 11);

            if (is_src_port_field && r.src_port_range_id != std::numeric_limits<uint32_t>::max()) {
                // Skip byte-wise specificity for source port if range ID is used
                if (k == 8) { // Only process once for the range
                    const auto& range = port_ranges[r.src_port_range_id];
                    uint32_t size = static_cast<uint32_t>(range.max_port) - range.min_port + 1;
                    specificity_score += (16 - static_cast<int>(std::round(std::log2(size > 0 ? size : 1))));
                }
                continue;
            }
            if (is_dst_port_field && r.dst_port_range_id != std::numeric_limits<uint32_t>::max()) {
                // Skip byte-wise specificity for destination port if range ID is used
                if (k == 10) { // Only process once for the range
                    const auto& range = port_ranges[r.dst_port_range_id];
                    uint32_t size = static_cast<uint32_t>(range.max_port) - range.min_port + 1;
                    specificity_score += (16 - static_cast<int>(std::round(std::log2(size > 0 ? size : 1))));
                }
                continue;
            }
            specificity_score += popcount_manual(r.mask[k]);
        }

        // Handle cases where port range ID is NOT used (i.e. exact match or wildcard in mask bits)
        // These would have been skipped above, or their popcount was already added if mask wasn't 0 for all bits.
        // This ensures that if range ID is not used, the byte-wise popcount of mask bits is used.
        // If a range ID *is* used, we've already added its specificity and skipped the byte-wise count.
        // If no range ID is used, the general loop already added popcount for mask[8], mask[9], mask[10], mask[11].
        // So, no explicit separate handling for "Else" for port specificity is needed here if general loop covers it.

        return specificity_score;
    }

    std::pair<uint16_t, uint16_t> get_effective_port_range(const Rule& r, bool is_source) const {
        uint32_t range_id = is_source ? r.src_port_range_id : r.dst_port_range_id;
        size_t port_idx_start = is_source ? 8 : 10;

        if (range_id != std::numeric_limits<uint32_t>::max()) {
            if (range_id < port_ranges.size()) {
                return {port_ranges[range_id].min_port, port_ranges[range_id].max_port};
            } else {
                // This case should ideally not happen if data is consistent
                // Return a range that won't overlap if ID is invalid.
                return {0, 0};
            }
        } else {
            // Check mask bits for wildcard
            if (r.mask.size() > port_idx_start + 1 &&
                r.mask[port_idx_start] == 0x00 && r.mask[port_idx_start + 1] == 0x00) {
                return {0, 0xFFFF}; // Wildcard
            } else if (r.value.size() > port_idx_start + 1) {
                uint16_t port = (static_cast<uint16_t>(r.value[port_idx_start]) << 8) | r.value[port_idx_start + 1];
                return {port, port}; // Exact port
            } else {
                // Should not happen with valid rule structure
                return {0,0}; // Non-overlapping range
            }
        }
    }

    bool are_rules_overlapping(const Rule& r1, const Rule& r2, size_t r1_idx, size_t r2_idx) const {
        // Field-wise overlap check
        size_t common_size = std::min({r1.value.size(), r2.value.size(), r1.mask.size(), r2.mask.size()});
        for (size_t k = 0; k < common_size; ++k) {
            // Skip port fields, as they are handled by port range checks
            if (k >= 8 && k <= 11) continue;
            if (((r1.value[k] ^ r2.value[k]) & r1.mask[k] & r2.mask[k]) != 0) {
                return false; // No overlap based on exact match fields
            }
        }

        // Port range overlap check
        auto r1_src_ports = get_effective_port_range(r1, true);
        auto r2_src_ports = get_effective_port_range(r2, true);
        if (std::max(r1_src_ports.first, r2_src_ports.first) > std::min(r1_src_ports.second, r2_src_ports.second)) {
            return false; // Source ports do not overlap
        }

        auto r1_dst_ports = get_effective_port_range(r1, false);
        auto r2_dst_ports = get_effective_port_range(r2, false);
        if (std::max(r1_dst_ports.first, r2_dst_ports.first) > std::min(r1_dst_ports.second, r2_dst_ports.second)) {
            return false; // Destination ports do not overlap
        }

        return true; // All checks passed, rules overlap
    }

public:
    std::vector<Conflict> detect_conflicts() const {
        std::vector<Conflict> conflicts_list;
        for (size_t i = 0; i < rules.size(); ++i) {
            for (size_t j = i + 1; j < rules.size(); ++j) {
                const auto& r1 = rules[i];
                const auto& r2 = rules[j];

                if (r1.action == r2.action) {
                    continue; // No conflict in outcome
                }

                if (are_rules_overlapping(r1, r2, i, j)) {
                    conflicts_list.push_back({i, j, "Conflicting actions for overlapping rules"});
                }
            }
        }
        return conflicts_list;
    }

    std::vector<uint64_t> age_rules(std::chrono::steady_clock::duration max_age) {
        std::vector<uint64_t> aged_rule_ids;
        const auto current_time = std::chrono::steady_clock::now();

        for (Rule& rule : rules) { // Iterate by reference to modify is_active
            if (rule.is_active) {
                if ((current_time - rule.creation_time) > max_age) {
                    rule.is_active = false;
                    aged_rule_ids.push_back(rule.id);
                }
            }
        }
        // Note: This method intentionally does not call rebuild_optimized_structures.
        // A separate call to rebuild_optimized_structures or an add_rule call
        // would be needed to compact the rule list and update optimized structures.
        return aged_rule_ids;
    }

public:
    bool delete_rule(uint64_t rule_id) {
        for (auto& rule : rules) {
            if (rule.id == rule_id) {
                rule.is_active = false;
                // Note: rebuild_optimized_structures() is NOT called here for "soft" delete
                return true;
            }
        }
        return false; // Rule not found
    }

private:
    bool is_subset(const Rule& r_sub, const Rule& r_super) const {
        // Field-wise check (non-port fields)
        size_t common_value_size = std::min(r_sub.value.size(), r_super.value.size());
        size_t common_mask_size = std::min(r_sub.mask.size(), r_super.mask.size());
        size_t common_size = std::min(common_value_size, common_mask_size);

        for (size_t k = 0; k < common_size; ++k) {
            // Skip port fields for this check, they are handled by port range checks below
            if (k >= 8 && k <= 11) continue;

            // If r_sub's value differs from r_super's value in a bit position where r_super's mask is set,
            // then r_sub is not a subset.
            if (((r_sub.value[k] ^ r_super.value[k]) & r_super.mask[k]) != 0) {
                return false;
            }
            // If r_super's mask has a bit set that r_sub's mask does not (i.e., r_sub is more general),
            // then r_sub cannot be a subset of r_super for this field.
            if ((r_super.mask[k] & ~r_sub.mask[k]) != 0) {
                return false;
            }
        }
         // If r_super has more specific fields defined than r_sub, r_sub cannot be a subset.
        if (r_super.mask.size() > r_sub.mask.size()) { // Assuming masks are padded with 0 for non-existent fields
            for(size_t k = r_sub.mask.size(); k < r_super.mask.size(); ++k) {
                if (r_super.mask[k] != 0) return false; // r_super has a specific bit where r_sub is wildcard by omission
            }
        }


        // Port range check
        auto sub_src_ports = get_effective_port_range(r_sub, true);
        auto super_src_ports = get_effective_port_range(r_super, true);
        if (!(sub_src_ports.first >= super_src_ports.first && sub_src_ports.second <= super_src_ports.second)) {
            return false; // Source port range of r_sub is not within r_super
        }

        auto sub_dst_ports = get_effective_port_range(r_sub, false);
        auto super_dst_ports = get_effective_port_range(r_super, false);
        if (!(sub_dst_ports.first >= super_dst_ports.first && sub_dst_ports.second <= super_dst_ports.second)) {
            return false; // Destination port range of r_sub is not within r_super
        }

        return true; // All checks passed
    }

public:
    std::vector<size_t> detect_shadowed_rules() const {
        std::vector<size_t> shadowed_rule_indices;
        // Assumes rules are already sorted by priority then specificity by rebuild_optimized_structures()

        for (size_t i = 0; i < rules.size(); ++i) { // rules[i] is the candidate shadowed rule
            for (size_t j = 0; j < i; ++j) { // rules[j] is the potential shadowing rule (higher priority/specificity)
                // If rules[i] and rules[j] have different actions and rules[i] is a subset of rules[j],
                // then rules[i] is shadowed by rules[j].
                // If actions are the same, it's not "shadowed" in a problematic way, but redundant.
                // The problem definition of "shadowed" usually implies a different outcome is hidden.
                // However, if any higher priority rule (rules[j]) that is a superset of rules[i] exists,
                // rules[i] will never be the highest priority match.
                if (rules[i].action != rules[j].action && is_subset(rules[i], rules[j])) {
                    shadowed_rule_indices.push_back(i);
                    break; // Found a rule that shadows rules[i], no need to check further for rules[i]
                }
            }
        }
        return shadowed_rule_indices;
    }

public:
    void build_decision_tree() {
        decision_tree.reset();
        if (rules.empty()) {
            return;
        }
        
        std::vector<int> all_rule_indices(rules.size());
        std::iota(all_rule_indices.begin(), all_rule_indices.end(), 0);

        const int LEAF_NODE_RULE_THRESHOLD = 4;
        const int MAX_TREE_DEPTH = 8;

        decision_tree = build_tree_recursive(all_rule_indices, 0, LEAF_NODE_RULE_THRESHOLD, MAX_TREE_DEPTH);
    }

private:
    void rebuild_optimized_structures_from_sorted_rules() {
        // This method assumes rules are already active and sorted.
        field_bitmaps.clear();
        if (!rules.empty()) {
            size_t num_fields_for_bitmap = rules[0].value.size(); // Assuming all rules have same size
            if (num_fields_for_bitmap > 0) {
                field_bitmaps.resize(num_fields_for_bitmap);
                for (size_t field_idx = 0; field_idx < num_fields_for_bitmap; ++field_idx) {
                    field_bitmaps[field_idx].num_rules = 0; // Reset rule count for this bitmap
                    for (size_t rule_idx = 0; rule_idx < rules.size(); ++rule_idx) {
                         if (rule_idx < BitmapTCAM::MAX_RULES) {
                            // Ensure rule has this field, though with fixed size rules this should be true
                            if (rules[rule_idx].value.size() > field_idx && rules[rule_idx].mask.size() > field_idx) {
                                field_bitmaps[field_idx].add_rule(rule_idx,
                                                                 rules[rule_idx].value[field_idx],
                                                                 rules[rule_idx].mask[field_idx]);
                            } else {
                                // This case implies inconsistent rule structure, handle defensively
                                field_bitmaps[field_idx].add_rule(rule_idx, 0, 0);
                            }
                        } else {
                            break;
                        }
                    }
                    field_bitmaps[field_idx].num_rules = std::min(rules.size(), BitmapTCAM::MAX_RULES);
                }
            }
        }
        build_decision_tree(); // build_decision_tree uses this->rules
    }

    void rebuild_optimized_structures() {
        // 1. Filter to keep only active rules
        rules.erase(std::remove_if(rules.begin(), rules.end(), [](const Rule& r){ return !r.is_active; }), rules.end());

        // 2. Sort the active rules
        std::sort(rules.begin(), rules.end(), [this](const Rule& a, const Rule& b) {
            if (a.priority != b.priority) {
                return a.priority > b.priority;
            }
            return this->calculate_specificity(a) > this->calculate_specificity(b);
        });

        // 3. Call the new method that rebuilds from already sorted & active rules
        rebuild_optimized_structures_from_sorted_rules();
    }
};

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
#include <optional>    // For std::optional
// <chrono> is already included higher up, but ensure it's there for this change.
// #include <chrono> // Not strictly needed here if already present globally

class OptimizedTCAM {
public: // For RuleStats and RuleUtilizationMetrics
    struct RuleStats {
        uint64_t rule_id;
        int priority;
        int action;
        uint64_t hit_count;
        std::chrono::steady_clock::time_point last_hit_timestamp;
        bool is_active;
        std::chrono::steady_clock::time_point creation_time;
    };

    struct RuleUtilizationMetrics {
        size_t total_rules = 0;
        size_t active_rules = 0;
        size_t inactive_rules = 0;
        size_t rules_hit_at_least_once = 0;
        double percentage_active_rules_hit = 0.0;
        std::vector<uint64_t> unused_active_rule_ids;
    };

    struct AggregatedLatencyMetrics {
        uint64_t total_lookups_measured = 0;
        std::chrono::nanoseconds min_latency_ns{0};
        std::chrono::nanoseconds max_latency_ns{0};
        std::chrono::nanoseconds avg_latency_ns{0};
    };

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

public:
    // Note: This method is not thread-safe if other operations modify rules or port_ranges concurrently.
    bool update_rules_atomic(const RuleUpdateBatch& batch) {
        std::vector<Rule> temp_rules = this->rules;
        std::vector<RangeEntry> temp_port_ranges = this->port_ranges;
        uint64_t current_next_rule_id = this->next_rule_id;
        uint32_t temp_next_port_range_id_counter = static_cast<uint32_t>(temp_port_ranges.size());

        // Filter temp_rules to keep only active rules initially
        temp_rules.erase(std::remove_if(temp_rules.begin(), temp_rules.end(),
                                        [](const Rule& r){ return !r.is_active; }),
                         temp_rules.end());

        auto add_port_range_temp_lambda =
            [](std::vector<RangeEntry>& local_port_ranges, uint16_t min_p, uint16_t max_p, uint32_t& next_id_counter_ref) -> uint32_t {
            // Check if an identical range already exists
            for (uint32_t i = 0; i < local_port_ranges.size(); ++i) {
                if (local_port_ranges[i].min_port == min_p && local_port_ranges[i].max_port == max_p) {
                    return i; // Return existing ID
                }
            }
            // If not, add new one
            uint32_t new_id = next_id_counter_ref;
            local_port_ranges.push_back({min_p, max_p, new_id});
            next_id_counter_ref++;
            return new_id;
        };

        for (const auto& op : batch) {
            if (op.type == RuleOperation::Type::ADD) {
                Rule rule_obj;
                const size_t rule_byte_size = 15; // As defined in add_rule_with_ranges
                rule_obj.value.resize(rule_byte_size);
                rule_obj.mask.resize(rule_byte_size);

                pack_ip(rule_obj.value.data(), rule_obj.mask.data(), 0, op.fields.src_ip, op.fields.src_ip_mask);
                pack_ip(rule_obj.value.data(), rule_obj.mask.data(), 4, op.fields.dst_ip, op.fields.dst_ip_mask);

                // Simplified port packing - direct values or new ranges
                // Source Port
                if (op.fields.src_port_min == 0 && op.fields.src_port_max == 0xFFFF) { // Wildcard
                    rule_obj.mask[8] = 0x00; rule_obj.mask[9] = 0x00;
                    rule_obj.src_port_range_id = std::numeric_limits<uint32_t>::max();
                } else if (op.fields.src_port_min == op.fields.src_port_max) { // Exact port
                    rule_obj.value[8] = static_cast<uint8_t>((op.fields.src_port_min >> 8) & 0xFF);
                    rule_obj.value[9] = static_cast<uint8_t>(op.fields.src_port_min & 0xFF);
                    rule_obj.mask[8] = 0xFF; rule_obj.mask[9] = 0xFF;
                    rule_obj.src_port_range_id = std::numeric_limits<uint32_t>::max();
                } else { // Range
                    rule_obj.src_port_range_id = add_port_range_temp_lambda(temp_port_ranges, op.fields.src_port_min, op.fields.src_port_max, temp_next_port_range_id_counter);
                    rule_obj.mask[8] = 0x00; rule_obj.mask[9] = 0x00; // Mask implies range lookup
                }

                // Destination Port
                if (op.fields.dst_port_min == 0 && op.fields.dst_port_max == 0xFFFF) { // Wildcard
                    rule_obj.mask[10] = 0x00; rule_obj.mask[11] = 0x00;
                    rule_obj.dst_port_range_id = std::numeric_limits<uint32_t>::max();
                } else if (op.fields.dst_port_min == op.fields.dst_port_max) { // Exact port
                    rule_obj.value[10] = static_cast<uint8_t>((op.fields.dst_port_min >> 8) & 0xFF);
                    rule_obj.value[11] = static_cast<uint8_t>(op.fields.dst_port_min & 0xFF);
                    rule_obj.mask[10] = 0xFF; rule_obj.mask[11] = 0xFF;
                    rule_obj.dst_port_range_id = std::numeric_limits<uint32_t>::max();
                } else { // Range
                    rule_obj.dst_port_range_id = add_port_range_temp_lambda(temp_port_ranges, op.fields.dst_port_min, op.fields.dst_port_max, temp_next_port_range_id_counter);
                    rule_obj.mask[10] = 0x00; rule_obj.mask[11] = 0x00; // Mask implies range lookup
                }

                rule_obj.value[12] = op.fields.protocol;
                rule_obj.mask[12] = op.fields.protocol_mask;
                rule_obj.value[13] = static_cast<uint8_t>((op.fields.eth_type >> 8) & 0xFF);
                rule_obj.mask[13] = static_cast<uint8_t>((op.fields.eth_type_mask >> 8) & 0xFF);
                rule_obj.value[14] = static_cast<uint8_t>(op.fields.eth_type & 0xFF);
                rule_obj.mask[14] = static_cast<uint8_t>(op.fields.eth_type_mask & 0xFF);

                rule_obj.priority = op.priority;
                rule_obj.action = op.action;
                rule_obj.id = current_next_rule_id++;
                rule_obj.is_active = true;
                rule_obj.creation_time = std::chrono::steady_clock::now();

                normalize_rule_fields(rule_obj); // Normalize before adding to temp_rules
                temp_rules.push_back(rule_obj);

            } else if (op.type == RuleOperation::Type::DELETE) {
                auto it = std::find_if(temp_rules.begin(), temp_rules.end(),
                                       [&](const Rule& r){ return r.id == op.rule_id_to_delete && r.is_active; });
                if (it != temp_rules.end()) {
                    it->is_active = false; // Mark for removal from temp_rules later
                } else {
                    // Rule to delete not found among active rules in the current transaction state
                    // Or rule already marked inactive by a previous delete op in the same batch
                    bool already_marked_inactive = false;
                    for(const auto& r_check : temp_rules) {
                        if (r_check.id == op.rule_id_to_delete && !r_check.is_active) {
                            already_marked_inactive = true;
                            break;
                        }
                    }
                    if(!already_marked_inactive) {
                        // If it wasn't just marked inactive in this batch, then it's an error.
                         // Consider logging: std::cerr << "Rule ID " << op.rule_id_to_delete << " not found for deletion in batch." << std::endl;
                        return false; // Abort batch
                    }
                }
            }
        }

        // Finalize: remove rules marked inactive by DELETE operations
        temp_rules.erase(std::remove_if(temp_rules.begin(), temp_rules.end(),
                                        [](const Rule& r){ return !r.is_active; }),
                         temp_rules.end());

        // Sort temp_rules.
        // Note: calculate_specificity uses this->port_ranges. For new rules in temp_rules
        // that use temp_port_ranges, specificity might be approximate if their range_id
        // is not yet in this->port_ranges. This is a known limitation for atomicity here
        // without refactoring calculate_specificity to accept a port_ranges argument.
        std::sort(temp_rules.begin(), temp_rules.end(), [this](const Rule& a, const Rule& b) {
            if (a.priority != b.priority) { return a.priority > b.priority; }
            // Pass this->port_ranges explicitly if calculate_specificity is refactored
            return this->calculate_specificity(a) > this->calculate_specificity(b);
        });

        // Commit changes
        this->rules = temp_rules;
        this->port_ranges = temp_port_ranges;
        this->next_rule_id = current_next_rule_id;
        this->rebuild_optimized_structures_from_sorted_rules(); // Assumes rules are active and sorted

        return true;
    }

public:
    std::vector<size_t> detect_redundant_rules() const {
        std::vector<size_t> redundant_rule_indices;
        // Assumes rules are sorted by priority/specificity.
        // And also that is_active flags are current.

        for (size_t i = 0; i < rules.size(); ++i) {
            if (!rules[i].is_active) {
                continue;
            }
            for (size_t j = 0; j < i; ++j) { // Check against higher-priority/specificity rules
                if (!rules[j].is_active) {
                    continue;
                }

                // If rule[i] is a subset of rule[j] and they have the same action,
                // then rule[i] is redundant because rule[j] would have already matched.
                if (rules[i].action == rules[j].action && is_subset(rules[i], rules[j])) {
                    redundant_rule_indices.push_back(i);
                    break; // Found a rule that makes rules[i] redundant
                }
            }
        }
        return redundant_rule_indices;
    }

    void compact_redundant_rules(bool trigger_rebuild = false) {
        std::vector<size_t> redundant_indices = detect_redundant_rules();

        if (redundant_indices.empty()) {
            return;
        }

        for (size_t idx : redundant_indices) {
            if (idx < rules.size()) { // Boundary check, though indices should be valid
                rules[idx].is_active = false;
            }
        }

        if (trigger_rebuild) {
            rebuild_optimized_structures();
        }
    }

public:
    struct MemoryUsageStats {
        size_t total_rules_in_vector = 0;
        size_t active_rules_count = 0;
        size_t inactive_rules_count = 0;

        size_t rules_vector_capacity_bytes = 0;
        size_t rules_vector_size_bytes = 0; // Memory for the Rule objects themselves in the vector

        size_t port_ranges_capacity_bytes = 0;
        size_t port_ranges_size_bytes = 0;

        size_t field_bitmaps_count = 0;
        size_t field_bitmaps_approx_bytes = 0;

        size_t decision_tree_nodes_count = 0;
        size_t decision_tree_approx_bytes = 0;

        size_t total_approx_bytes = 0;
    };

    MemoryUsageStats get_memory_usage_stats() const {
        MemoryUsageStats stats;
        stats.total_rules_in_vector = rules.size();
        for(const auto& rule : rules) {
            if (rule.is_active) {
                stats.active_rules_count++;
            }
        }
        stats.inactive_rules_count = stats.total_rules_in_vector - stats.active_rules_count;

        stats.rules_vector_capacity_bytes = rules.capacity() * sizeof(Rule);
        stats.rules_vector_size_bytes = rules.size() * sizeof(Rule); // Does not include heap data per rule (like vectors in Rule)

        stats.port_ranges_capacity_bytes = port_ranges.capacity() * sizeof(RangeEntry);
        stats.port_ranges_size_bytes = port_ranges.size() * sizeof(RangeEntry);

        stats.field_bitmaps_count = field_bitmaps.size();
        // field_bitmaps stores BitmapTCAM objects directly.
        stats.field_bitmaps_approx_bytes = field_bitmaps.size() * sizeof(BitmapTCAM);

        stats.decision_tree_nodes_count = count_decision_tree_nodes_recursive(decision_tree.get());
        // decision_tree_nodes are heap allocated via unique_ptr.
        stats.decision_tree_approx_bytes = stats.decision_tree_nodes_count * sizeof(DecisionNode);

        // Note: sizeof(Rule) doesn't account for std::vector<uint8_t> value and mask data if stored on heap.
        // This calculation is an approximation of the main structures.
        stats.total_approx_bytes = stats.rules_vector_size_bytes +
                                   stats.port_ranges_size_bytes +
                                   stats.field_bitmaps_approx_bytes +
                                   stats.decision_tree_approx_bytes;
        return stats;
    }

private:
    // --- Struct Definitions ---
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
        mutable uint64_t hit_count = 0;
        mutable std::chrono::steady_clock::time_point last_hit_timestamp;
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
    
    // --- Member Variable Declarations ---
    std::vector<Rule> rules;
    std::vector<RangeEntry> port_ranges;
    std::unique_ptr<DecisionNode> decision_tree;
    std::vector<BitmapTCAM> field_bitmaps;
    uint64_t next_rule_id = 0;

    // --- Helper Methods that depend on struct definitions and member variables ---
    static void normalize_rule_fields(Rule& rule) {
        // Ensures that if mask[i] is 0x00 (wildcard), value[i] is also 0x00.
        size_t common_size = std::min(rule.value.size(), rule.mask.size());
        for (size_t i = 0; i < common_size; ++i) {
            if (rule.mask[i] == 0x00) {
                rule.value[i] = 0x00;
            }
        }
        // If value or mask is longer than common_size, those bytes are not touched here.
        // This assumes that rule construction ensures value and mask have meaningful (and typically equal) lengths.
    }

    size_t count_decision_tree_nodes_recursive(const DecisionNode* node) const {
        if (!node) {
            return 0;
        }
        // Accesses node->left and node->right, requiring DecisionNode to be fully defined.
        return 1 + count_decision_tree_nodes_recursive(node->left.get()) +
                   count_decision_tree_nodes_recursive(node->right.get());
    }

    // --- Other Private Methods ---
    // (The comment "WildcardFields moved to public section" below was likely a general note from earlier refactoring, not specific to this line)
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

    // --- Public methods start here in the original file ---
    // The SEARCH block should end before this, and the REPLACE block will re-insert the private methods
    // like add_port_range, pack_ip, etc., after the reordered helper methods.
    // For the purpose of this diff, I will assume the SEARCH block ends before the next public method,
    // and the REPLACE block will correctly place ALL private methods after the moved ones.
    // The provided SEARCH block in the prompt ends before `build_tree_recursive`, which is fine.
    // The REPLACE block will then put the struct definitions, then members, then the two moved methods,
    // then `build_tree_recursive` and the rest of the private methods.

public: // This public keyword is just to mark where the next section starts in the original file
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

        normalize_rule_fields(rule_obj); // Normalize before insertion logic

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

    // Returns rule index or -1 if no match
    int lookup_decision_tree_idx(const std::vector<uint8_t>& packet, std::vector<std::string>* debug_trace_log = nullptr) const {
        if (debug_trace_log) debug_trace_log->push_back("lookup_decision_tree_idx: Starting tree traversal.");
        if (!decision_tree) {
            if (debug_trace_log) debug_trace_log->push_back("lookup_decision_tree_idx: Decision tree is null. Returning -1.");
            return -1;
        }
        return traverse_decision_tree(decision_tree.get(), packet, debug_trace_log);
    }
    
    // Returns rule index or -1 if no match
    int lookup_bitmap_idx(const std::vector<uint8_t>& packet, std::vector<std::string>* debug_trace_log = nullptr) const {
        if (debug_trace_log) debug_trace_log->push_back("lookup_bitmap_idx: Starting bitmap lookup.");
        if (field_bitmaps.empty() || packet.empty() || rules.empty()) { // Ensure rules is not empty
            if (debug_trace_log) {
                if (field_bitmaps.empty()) debug_trace_log->push_back("lookup_bitmap_idx: Field bitmaps empty.");
                if (packet.empty()) debug_trace_log->push_back("lookup_bitmap_idx: Packet empty.");
                if (rules.empty()) debug_trace_log->push_back("lookup_bitmap_idx: Rules empty."); // Added this check
                debug_trace_log->push_back("lookup_bitmap_idx: Pre-check failed, returning -1.");
            }
            return -1;
        }
        
        std::bitset<BitmapTCAM::MAX_RULES> matches_bs; // Initialize to all ones
        matches_bs.set();
        
        for (size_t field_idx = 0; field_idx < field_bitmaps.size(); ++field_idx) {
            if (field_idx < packet.size()) {
                auto field_matches = field_bitmaps[field_idx].lookup(packet[field_idx]);
                if (debug_trace_log) debug_trace_log->push_back("lookup_bitmap_idx: Field " + std::to_string(field_idx) + " PktByte=" + std::to_string(packet[field_idx]) + " -> Initial field_matches_count=" + std::to_string(field_matches.count()));
                matches_bs &= field_matches;
            } else { // Packet is shorter than the number of fields in field_bitmaps
                std::bitset<BitmapTCAM::MAX_RULES> rules_wildcarded_for_this_field;
                // This logic assumes rules might be shorter than num_fields_for_bitmap,
                // or that field_idx check is for fields beyond packet length.
                // A rule matches if its mask for this field_idx is 0x00 (wildcard).
                for(size_t rule_idx = 0; rule_idx < rules.size() && rule_idx < BitmapTCAM::MAX_RULES; ++rule_idx) {
                    // This check is crucial: only consider rules active in the bitmap for this field
                    if (rules[rule_idx].mask.size() > field_idx && rules[rule_idx].mask[field_idx] == 0x00) {
                        rules_wildcarded_for_this_field.set(rule_idx);
                    }
                }
                if (debug_trace_log) debug_trace_log->push_back("lookup_bitmap_idx: Field " + std::to_string(field_idx) + " (packet short) -> Wildcarded_rules_count=" + std::to_string(rules_wildcarded_for_this_field.count()));
                matches_bs &= rules_wildcarded_for_this_field;
            }
            if (debug_trace_log) debug_trace_log->push_back("lookup_bitmap_idx: After field " + std::to_string(field_idx) + ", combined matches_bs_count=" + std::to_string(matches_bs.count()));
            if (!matches_bs.any()) {
                if (debug_trace_log) debug_trace_log->push_back("lookup_bitmap_idx: No matches after field " + std::to_string(field_idx) + ". Breaking.");
                break;
            }
        }

        if (debug_trace_log) debug_trace_log->push_back("lookup_bitmap_idx: Iterating over " + std::to_string(matches_bs.count()) + " potential matches from bitmap.");
        for (size_t i = 0; i < rules.size(); ++i) { // Iterate up to actual rules.size()
            if (i < BitmapTCAM::MAX_RULES && matches_bs[i]) { // Check if bit is set AND within MAX_RULES
                if (debug_trace_log) debug_trace_log->push_back("lookup_bitmap_idx: Checking rule index " + std::to_string(i) + " (ID: " + std::to_string(rules[i].id) + ") from bitmap result.");
                const auto& r = rules[i];
                if (!r.is_active) {
                     if (debug_trace_log) debug_trace_log->push_back("lookup_bitmap_idx: Rule " + std::to_string(r.id) + " is inactive, skipping.");
                    continue;
                }

                bool port_ranges_match = true;
                // Source Port Check
                if (r.src_port_range_id != std::numeric_limits<uint32_t>::max()) {
                    std::string src_port_log = "SrcPort Check (RuleID " + std::to_string(r.id) + "): ";
                    if (packet.size() < 10) {
                        port_ranges_match = false;
                        if (debug_trace_log) src_port_log += "Packet too short.";
                    } else if (r.src_port_range_id >= port_ranges.size()) {
                        port_ranges_match = false;
                        if (debug_trace_log) src_port_log += "Invalid range_id " + std::to_string(r.src_port_range_id) + ". Max is " + std::to_string(port_ranges.size()-1);
                    } else {
                        uint16_t packet_src_port = (static_cast<uint16_t>(packet[8]) << 8) | packet[9];
                        const auto& range_entry = port_ranges[r.src_port_range_id];
                        if (packet_src_port < range_entry.min_port || packet_src_port > range_entry.max_port) {
                            port_ranges_match = false;
                        }
                        if (debug_trace_log) src_port_log += "PktPort=" + std::to_string(packet_src_port) + " Range=" + std::to_string(range_entry.min_port) + "-" + std::to_string(range_entry.max_port);
                    }
                    if (debug_trace_log) debug_trace_log->push_back(src_port_log + " -> " + (port_ranges_match ? "Match" : "Mismatch"));
                }

                // Destination Port Check
                if (port_ranges_match && r.dst_port_range_id != std::numeric_limits<uint32_t>::max()) {
                    std::string dst_port_log = "DstPort Check (RuleID " + std::to_string(r.id) + "): ";
                    if (packet.size() < 12) {
                        port_ranges_match = false;
                        if (debug_trace_log) dst_port_log += "Packet too short.";
                    } else if (r.dst_port_range_id >= port_ranges.size()) {
                        port_ranges_match = false;
                        if (debug_trace_log) dst_port_log += "Invalid range_id " + std::to_string(r.dst_port_range_id) + ". Max is " + std::to_string(port_ranges.size()-1);
                    } else {
                        uint16_t packet_dst_port = (static_cast<uint16_t>(packet[10]) << 8) | packet[11];
                        const auto& range_entry = port_ranges[r.dst_port_range_id];
                        if (packet_dst_port < range_entry.min_port || packet_dst_port > range_entry.max_port) {
                            port_ranges_match = false;
                        }
                        if (debug_trace_log) dst_port_log += "PktPort=" + std::to_string(packet_dst_port) + " Range=" + std::to_string(range_entry.min_port) + "-" + std::to_string(range_entry.max_port);
                    }
                     if (debug_trace_log) debug_trace_log->push_back(dst_port_log + " -> " + (port_ranges_match ? "Match" : "Mismatch"));
                }

                if (port_ranges_match) {
                    if (debug_trace_log) debug_trace_log->push_back("lookup_bitmap_idx: Rule " + std::to_string(r.id) + " (index " + std::to_string(i) + ") fully matched. Returning index.");
                    return static_cast<int>(i);
                } else {
                    if (debug_trace_log) debug_trace_log->push_back("lookup_bitmap_idx: Rule " + std::to_string(r.id) + " (index " + std::to_string(i) + ") failed port match.");
                }
            }
        }
        if (debug_trace_log) debug_trace_log->push_back("lookup_bitmap_idx: No rule from bitmap fully matched. Returning -1.");
        return -1;
    }
    
    // Returns rule index or -1 if no match
    int lookup_linear_idx(const std::vector<uint8_t>& packet, std::vector<std::string>* debug_trace_log = nullptr) const {
        if (debug_trace_log) debug_trace_log->push_back("lookup_linear_idx: Starting linear search.");
        for (size_t i = 0; i < rules.size(); ++i) {
            if (debug_trace_log) debug_trace_log->push_back("lookup_linear_idx: Iterating rule index " + std::to_string(i) + " (ID: " + std::to_string(rules[i].id) + ")");
            if (matches_rule(packet, rules[i], debug_trace_log)) {
                if (debug_trace_log) debug_trace_log->push_back("lookup_linear_idx: Matched rule index " + std::to_string(i) + ". Returning index.");
                return static_cast<int>(i); // Return rule index
            }
        }
        if (debug_trace_log) debug_trace_log->push_back("lookup_linear_idx: No match found after iterating all rules. Returning -1.");
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
        // Latency tracking fields
        mutable std::chrono::nanoseconds current_min_latency_ns{std::chrono::nanoseconds::max()};
        mutable std::chrono::nanoseconds current_max_latency_ns{std::chrono::nanoseconds::zero()};
        mutable std::chrono::nanoseconds accumulated_latency_ns{std::chrono::nanoseconds::zero()};
        mutable uint64_t num_lookups_for_latency = 0;
    };
    mutable LookupStats stats;
    
    void optimize_for_traffic_pattern(const std::vector<std::vector<uint8_t>>& sample_traffic) {
        auto t_start = std::chrono::high_resolution_clock::now();
        for (const auto& packet : sample_traffic) lookup_linear_idx(packet); // Call _idx version
        auto t_linear_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - t_start);
        
        t_start = std::chrono::high_resolution_clock::now();
        for (const auto& packet : sample_traffic) lookup_decision_tree_idx(packet); // Call _idx version
        auto t_tree_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - t_start);
        
        t_start = std::chrono::high_resolution_clock::now();
        for (const auto& packet : sample_traffic) lookup_bitmap_idx(packet); // Call _idx version
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
    
    bool matches_rule(const std::vector<uint8_t>& packet, const Rule& r, std::vector<std::string>* debug_trace_log = nullptr) const {
        if (!r.is_active) {
            if (debug_trace_log) debug_trace_log->push_back("matches_rule (RuleID " + std::to_string(r.id) + "): Rule not active.");
            return false;
        }

        for (size_t i = 0; i < r.value.size(); ++i) {
            bool field_match_condition = true;
            std::string field_log_details;
            if (i >= packet.size()) { // Packet is shorter than rule field being checked
                if (r.mask[i] != 0x00) { // If mask requires bits here, it's a mismatch
                    field_match_condition = false;
                }
                if (debug_trace_log) field_log_details = "Packet too short for this field (idx " + std::to_string(i) + "). RuleMask=" + std::to_string(r.mask[i]);
            } else {
                field_match_condition = ((packet[i] ^ r.value[i]) & r.mask[i]) == 0;
                if (debug_trace_log) field_log_details = "Field " + std::to_string(i) + ": PktByte=" + std::to_string(packet[i]) + " RuleVal=" + std::to_string(r.value[i]) + " RuleMask=" + std::to_string(r.mask[i]);
            }

            if (debug_trace_log) debug_trace_log->push_back("matches_rule (RuleID " + std::to_string(r.id) + "): " + field_log_details + " -> " + (field_match_condition ? "Match" : "Mismatch"));
            if (!field_match_condition) {
                 if (debug_trace_log) debug_trace_log->push_back("matches_rule (RuleID " + std::to_string(r.id) + "): Final -> No Match (due to field " + std::to_string(i) + ")");
                return false;
            }
        }

        if (r.src_port_range_id != std::numeric_limits<uint32_t>::max()) {
            bool port_match = true;
            std::string port_log_details = "SrcPort Check: ";
            if (packet.size() < 10) { // Packet too short for src port
                port_match = false;
                if (debug_trace_log) port_log_details += "Packet too short for src port.";
            } else if (r.src_port_range_id >= port_ranges.size()) { // Invalid range ID
                port_match = false;
                if (debug_trace_log) port_log_details += "Invalid src_port_range_id " + std::to_string(r.src_port_range_id);
            } else {
                uint16_t packet_src_port = (static_cast<uint16_t>(packet[8]) << 8) | packet[9];
                const auto& range_entry = port_ranges[r.src_port_range_id];
                port_match = (packet_src_port >= range_entry.min_port && packet_src_port <= range_entry.max_port);
                if (debug_trace_log) port_log_details += "PktPort=" + std::to_string(packet_src_port) + " Range=" + std::to_string(range_entry.min_port) + "-" + std::to_string(range_entry.max_port);
            }
            if (debug_trace_log) debug_trace_log->push_back("matches_rule (RuleID " + std::to_string(r.id) + "): " + port_log_details + " -> " + (port_match ? "Match" : "Mismatch"));
            if (!port_match) {
                if (debug_trace_log) debug_trace_log->push_back("matches_rule (RuleID " + std::to_string(r.id) + "): Final -> No Match (due to src port)");
                return false;
            }
        }

        if (r.dst_port_range_id != std::numeric_limits<uint32_t>::max()) {
            bool port_match = true;
            std::string port_log_details = "DstPort Check: ";
             if (packet.size() < 12) { // Packet too short for dst port
                port_match = false;
                if (debug_trace_log) port_log_details += "Packet too short for dst port.";
            } else if (r.dst_port_range_id >= port_ranges.size()) { // Invalid range ID
                port_match = false;
                if (debug_trace_log) port_log_details += "Invalid dst_port_range_id " + std::to_string(r.dst_port_range_id);
            } else {
                uint16_t packet_dst_port = (static_cast<uint16_t>(packet[10]) << 8) | packet[11];
                const auto& range_entry = port_ranges[r.dst_port_range_id];
                port_match = (packet_dst_port >= range_entry.min_port && packet_dst_port <= range_entry.max_port);
                if (debug_trace_log) port_log_details += "PktPort=" + std::to_string(packet_dst_port) + " Range=" + std::to_string(range_entry.min_port) + "-" + std::to_string(range_entry.max_port);
            }
            if (debug_trace_log) debug_trace_log->push_back("matches_rule (RuleID " + std::to_string(r.id) + "): " + port_log_details + " -> " + (port_match ? "Match" : "Mismatch"));
            if (!port_match) {
                if (debug_trace_log) debug_trace_log->push_back("matches_rule (RuleID " + std::to_string(r.id) + "): Final -> No Match (due to dst port)");
                return false;
            }
        }
        if (debug_trace_log) debug_trace_log->push_back("matches_rule (RuleID " + std::to_string(r.id) + "): Final -> Matched");
        return true;
    }
    
    int lookup_single(const std::vector<uint8_t>& packet, std::vector<std::string>* debug_trace_log = nullptr) const {
        auto tcam_lookup_start_time = std::chrono::high_resolution_clock::now();
        int action_to_return = -1;
        std::string chosen_strategy_log;

        if (debug_trace_log) debug_trace_log->push_back("lookup_single: Starting lookup.");

        if (rules.empty()) {
            if (debug_trace_log) debug_trace_log->push_back("lookup_single: Rules empty, returning -1.");
            // Latency update block
            auto tcam_lookup_end_time = std::chrono::high_resolution_clock::now();
            auto tcam_lookup_duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(tcam_lookup_end_time - tcam_lookup_start_time);
            if (debug_trace_log) debug_trace_log->push_back("lookup_single: Calculated duration " + std::to_string(tcam_lookup_duration_ns.count()) + "ns for empty rules case.");
            stats.num_lookups_for_latency++;
            stats.accumulated_latency_ns += tcam_lookup_duration_ns;
            if (tcam_lookup_duration_ns < stats.current_min_latency_ns) stats.current_min_latency_ns = tcam_lookup_duration_ns;
            if (tcam_lookup_duration_ns > stats.current_max_latency_ns) stats.current_max_latency_ns = tcam_lookup_duration_ns;
            return -1;
        }

        int matched_rule_idx = -1;

        if (stats.avg_linear_time > 0 && stats.avg_bitmap_time > 0 && !field_bitmaps.empty()) {
            if (stats.avg_bitmap_time < stats.avg_linear_time && (stats.avg_tree_time == 0 || stats.avg_bitmap_time < stats.avg_tree_time) ) {
                stats.bitmap_lookups++;
                chosen_strategy_log = "Bitmap";
                matched_rule_idx = lookup_bitmap_idx(packet, debug_trace_log);
            } else if (stats.avg_tree_time > 0 && decision_tree && (stats.avg_tree_time < stats.avg_linear_time) ){
                stats.decision_tree_lookups++;
                chosen_strategy_log = "Tree";
                matched_rule_idx = lookup_decision_tree_idx(packet, debug_trace_log);
            } else {
                stats.linear_lookups++;
                chosen_strategy_log = "Linear (fallback from preferred)";
                matched_rule_idx = lookup_linear_idx(packet, debug_trace_log);
            }
        } else if (rules.size() < 16 || (!decision_tree && field_bitmaps.empty())) {
             stats.linear_lookups++;
             chosen_strategy_log = "Linear (small rule set or no optimized structures)";
             matched_rule_idx = lookup_linear_idx(packet, debug_trace_log);
        } else if (!field_bitmaps.empty()) {
             stats.bitmap_lookups++;
             chosen_strategy_log = "Bitmap (default)";
             matched_rule_idx = lookup_bitmap_idx(packet, debug_trace_log);
        } else if (decision_tree) {
             stats.decision_tree_lookups++;
             chosen_strategy_log = "Tree (default, no bitmap)";
             matched_rule_idx = lookup_decision_tree_idx(packet, debug_trace_log);
        } else {
            stats.linear_lookups++;
            chosen_strategy_log = "Linear (final fallback)";
            matched_rule_idx = lookup_linear_idx(packet, debug_trace_log);
        }
        if (debug_trace_log) debug_trace_log->push_back("lookup_single: Chosen strategy: " + chosen_strategy_log);

        if (matched_rule_idx != -1 && static_cast<size_t>(matched_rule_idx) < rules.size()) {
            rules[matched_rule_idx].hit_count++;
            rules[matched_rule_idx].last_hit_timestamp = std::chrono::steady_clock::now();
            action_to_return = rules[matched_rule_idx].action;
            if (debug_trace_log) debug_trace_log->push_back("lookup_single: Matched rule index: " + std::to_string(matched_rule_idx) + ", Action: " + std::to_string(rules[matched_rule_idx].action));
        } else {
            action_to_return = -1;
            if (debug_trace_log) debug_trace_log->push_back("lookup_single: No match or invalid index from chosen strategy.");
        }

        // Latency update block
        auto tcam_lookup_end_time = std::chrono::high_resolution_clock::now();
        auto tcam_lookup_duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(tcam_lookup_end_time - tcam_lookup_start_time);
        if (debug_trace_log) debug_trace_log->push_back("lookup_single: Calculated duration " + std::to_string(tcam_lookup_duration_ns.count()) + "ns.");
        stats.num_lookups_for_latency++;
        stats.accumulated_latency_ns += tcam_lookup_duration_ns;
        if (tcam_lookup_duration_ns < stats.current_min_latency_ns) stats.current_min_latency_ns = tcam_lookup_duration_ns;
        if (tcam_lookup_duration_ns > stats.current_max_latency_ns) stats.current_max_latency_ns = tcam_lookup_duration_ns;

        return action_to_return;
    }
    
    #ifdef __AVX2__
    void lookup_simd_avx2(const std::vector<uint8_t>* packets_ptr, int* results_ptr) {
        for (int i = 0; i < 8; ++i) {
            results_ptr[i] = lookup_single(packets_ptr[i]);
        }
    }
    #endif
    
    // Returns rule index or -1 if no match
    int traverse_decision_tree(const DecisionNode* current_node, const std::vector<uint8_t>& packet, std::vector<std::string>* debug_trace_log = nullptr) const {
        if (!current_node) {
            if (debug_trace_log) debug_trace_log->push_back("traverse_decision_tree: Current node is null. Returning -1.");
            return -1;
        }
        if (debug_trace_log) {
             debug_trace_log->push_back("traverse_decision_tree: Node FieldOffset=" + std::to_string(current_node->field_offset) +
                                       " TestValue=" + std::to_string(current_node->test_value) + " Mask=" + std::to_string(current_node->mask) +
                                       " NumRulesAtNode=" + std::to_string(current_node->rule_indices.size()));
        }

        // Check rules directly attached to this node (wildcarded for this field or at max depth)
        for (int rule_idx_val : current_node->rule_indices) {
            if (debug_trace_log) debug_trace_log->push_back("traverse_decision_tree: Checking rule index " + std::to_string(rule_idx_val) + " (ID: " + (static_cast<size_t>(rule_idx_val) < rules.size() ? std::to_string(rules[rule_idx_val].id) : "OOB") + ") at current node.");
            if (static_cast<size_t>(rule_idx_val) < rules.size() && matches_rule(packet, rules[rule_idx_val], debug_trace_log)) {
                if (debug_trace_log) debug_trace_log->push_back("traverse_decision_tree: Matched rule index " + std::to_string(rule_idx_val) + " at current node. Returning index.");
                return rule_idx_val; // Return rule index
            }
        }

        // If it's a leaf node (field_offset == -1) or no children to explore further.
        if (current_node->field_offset == -1 || (!current_node->left && !current_node->right)) {
            if (debug_trace_log) debug_trace_log->push_back("traverse_decision_tree: Leaf node or no children to explore further. No match from direct rules. Returning -1.");
            return -1;
        }

        // Check if packet is long enough for the field test
        if (current_node->field_offset >= 0 && static_cast<size_t>(current_node->field_offset) < packet.size()) {
            uint8_t packet_field_byte = packet[current_node->field_offset];
            bool condition_met = ((packet_field_byte & current_node->mask) == (current_node->test_value & current_node->mask));

            if (debug_trace_log) debug_trace_log->push_back("traverse_decision_tree: Testing PktByte[" + std::to_string(current_node->field_offset) + "]=" + std::to_string(packet_field_byte) + " against NodeTestVal=" + std::to_string(current_node->test_value) + " with Mask=" + std::to_string(current_node->mask));

            if (condition_met) {
                if (debug_trace_log) debug_trace_log->push_back("traverse_decision_tree: Condition met. Going Left.");
                if (current_node->left) {
                    int result = traverse_decision_tree(current_node->left.get(), packet, debug_trace_log);
                    if (result != -1) return result;
                } else {
                    if (debug_trace_log) debug_trace_log->push_back("traverse_decision_tree: Left child is null, no path.");
                }
            } else {
                if (debug_trace_log) debug_trace_log->push_back("traverse_decision_tree: Condition NOT met. Going Right.");
                if (current_node->right) {
                    int result = traverse_decision_tree(current_node->right.get(), packet, debug_trace_log);
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
    // It rebuilds data structures like bitmaps and decision trees from the current `rules` vector.
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
        // This method serves as a full rebuild and defragmentation step.
        // 1. Filter to keep only active rules, effectively compacting the rules vector.
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

public: // Statistics methods
    std::optional<RuleStats> get_rule_stats(uint64_t rule_id) const {
        for (const auto& r : rules) {
            if (r.id == rule_id) {
                RuleStats stats;
                stats.rule_id = r.id;
                stats.priority = r.priority;
                stats.action = r.action;
                stats.hit_count = r.hit_count;
                stats.last_hit_timestamp = r.last_hit_timestamp;
                stats.is_active = r.is_active;
                stats.creation_time = r.creation_time;
                return stats;
            }
        }
        return std::nullopt;
    }

    std::vector<RuleStats> get_all_rule_stats() const {
        std::vector<RuleStats> all_stats;
        all_stats.reserve(rules.size());
        for (const auto& r : rules) {
            RuleStats stats;
            stats.rule_id = r.id;
            stats.priority = r.priority;
            stats.action = r.action;
            stats.hit_count = r.hit_count;
            stats.last_hit_timestamp = r.last_hit_timestamp;
            stats.is_active = r.is_active;
            stats.creation_time = r.creation_time;
            all_stats.push_back(stats);
        }
        return all_stats;
    }

    RuleUtilizationMetrics get_rule_utilization() const {
        RuleUtilizationMetrics metrics;
        metrics.total_rules = rules.size();

        for (const auto& r : rules) {
            if (r.is_active) {
                metrics.active_rules++;
                if (r.hit_count > 0) {
                    metrics.rules_hit_at_least_once++;
                } else {
                    metrics.unused_active_rule_ids.push_back(r.id);
                }
            } else {
                metrics.inactive_rules++;
            }
        }

        if (metrics.active_rules > 0) {
            metrics.percentage_active_rules_hit = (static_cast<double>(metrics.rules_hit_at_least_once) / metrics.active_rules) * 100.0;
        } else {
            metrics.percentage_active_rules_hit = 0.0;
        }

        return metrics;
    }

    AggregatedLatencyMetrics get_lookup_latency_metrics() const {
        AggregatedLatencyMetrics metrics;
        metrics.total_lookups_measured = stats.num_lookups_for_latency;
        if (stats.num_lookups_for_latency > 0) {
            metrics.min_latency_ns = stats.current_min_latency_ns;
            metrics.max_latency_ns = stats.current_max_latency_ns;
            metrics.avg_latency_ns = stats.accumulated_latency_ns / stats.num_lookups_for_latency;
        } else {
            // Already initialized to zeros by default constructor of AggregatedLatencyMetrics
            // Or explicitly:
            // metrics.min_latency_ns = std::chrono::nanoseconds::zero();
            // metrics.max_latency_ns = std::chrono::nanoseconds::zero();
            // metrics.avg_latency_ns = std::chrono::nanoseconds::zero();
        }
        return metrics;
    }
};

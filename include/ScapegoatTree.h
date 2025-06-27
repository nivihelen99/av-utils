#pragma once

#include <memory>
#include <functional> // For std::less
#include <vector>     // For rebuilding
#include <algorithm>  // For std::sort, std::max
#include <cmath>      // For std::log, std::floor

namespace cpp_collections {

// Forward declaration of ScapegoatTree
template <typename Key, typename Value, typename Compare = std::less<Key>>
class ScapegoatTree {
private:
    struct Node {
        Key key;
        Value value;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
        // Parent pointer is not strictly necessary for scapegoat tree's core logic
        // but can be useful for some traversal or utility functions.
        // For simplicity and to reduce overhead, we'll omit it initially.
        // Node* parent;

        size_t subtree_size; // Total number of physical nodes in the subtree rooted here.
        size_t active_nodes; // Number of active (not logically deleted) nodes in the subtree.
        bool is_deleted;     // Flag for lazy deletion.

        Node(const Key& k, const Value& v)
            : key(k), value(v), subtree_size(1), active_nodes(1), is_deleted(false) {}

        Node(Key&& k, Value&& v)
            : key(std::move(k)), value(std::move(v)), subtree_size(1), active_nodes(1), is_deleted(false) {}
    };

    std::unique_ptr<Node> root_;
    Compare compare_;
    double alpha_ = 0.75; // Typical alpha value for scapegoat trees (e.g., between 0.5 and 1.0)
                         // Determines how imbalanced a tree can get before a rebuild.
                         // For alpha-weight-balanced: size(left) <= alpha * size(parent) and size(right) <= alpha * size(parent)
                         // For alpha-height-balanced: height(child) <= alpha * height(parent) - not used here.
                         // We'll use a size-based balancing criterion.

    size_t total_nodes_ = 0;     // Total number of nodes in the tree (including logically deleted)
    size_t active_elements_ = 0; // Number of active elements (user-perceived size)
    size_t max_total_nodes_since_rebuild_ = 0; // Tracks max nodes to trigger global rebuild if too many deleted nodes.

    // --- Private Helper Functions ---

    // Helper to get subtree_size of a node (0 if null)
    size_t get_subtree_size(const std::unique_ptr<Node>& node) const {
        return node ? node->subtree_size : 0;
    }

    // Helper to get active_nodes of a node (0 if null)
    size_t get_active_nodes(const std::unique_ptr<Node>& node) const {
        return node ? node->active_nodes : 0;
    }

    // Updates subtree_size and active_nodes for a node based on its children
    void update_node_counts(Node* node) {
        if (!node) return;
        node->subtree_size = 1 + get_subtree_size(node->left) + get_subtree_size(node->right);
        node->active_nodes = (node->is_deleted ? 0 : 1) + get_active_nodes(node->left) + get_active_nodes(node->right);
    }

    // Recursive helper for find
    Node* find_node(Node* node, const Key& key) const {
        if (!node) {
            return nullptr;
        }
        if (compare_(key, node->key)) {
            return find_node(node->left.get(), key);
        } else if (compare_(node->key, key)) {
            return find_node(node->right.get(), key);
        } else { // Keys match
            return node->is_deleted ? nullptr : node; // Return node only if active
        }
    }

    // Recursive helper for insert
    // Returns the (potentially new) root of the modified subtree.
    // Sets `inserted_depth` to the depth of the newly inserted node, or -1 if key already existed.
    // `current_depth` is the depth of `node`.
    // `scapegoat_ancestor_ptr` will point to the unique_ptr owning the scapegoat node if one is found.
    std::unique_ptr<Node> insert_recursive(std::unique_ptr<Node> current_node, const Key& key, const Value& value, int current_depth, int& inserted_node_depth, std::unique_ptr<Node>** scapegoat_ancestor_ptr) {
        if (!current_node) {
            inserted_node_depth = current_depth;
            total_nodes_++;
            active_elements_++;
            // max_total_nodes_since_rebuild_ is updated by caller if necessary
            return std::make_unique<Node>(key, value);
        }

        bool key_less = compare_(key, current_node->key);
        bool key_greater = compare_(current_node->key, key);

        if (key_less) {
            current_node->left = insert_recursive(std::move(current_node->left), key, value, current_depth + 1, inserted_node_depth, scapegoat_ancestor_ptr);
        } else if (key_greater) {
            current_node->right = insert_recursive(std::move(current_node->right), key, value, current_depth + 1, inserted_node_depth, scapegoat_ancestor_ptr);
        } else { // Key already exists
            if (current_node->is_deleted) {
                current_node->is_deleted = false;
                current_node->value = value;
                active_elements_++;
                inserted_node_depth = current_depth;
            } else {
                current_node->value = value;
                inserted_node_depth = -1;
            }
        }

        update_node_counts(current_node.get());

        if (inserted_node_depth != -1) { // Check for imbalance if a node was inserted/reactivated
            // An ancestor of the inserted/reactivated node is `current_node`.
            // The actual inserted/reactivated node is at depth `inserted_node_depth`.
            // We only care about ancestors, so `current_depth` must be less than `inserted_node_depth`.
            if (current_depth < inserted_node_depth) {
                // Check if current_node is alpha-weight-unbalanced
                // Using active_nodes for balancing condition as per some Scapegoat Tree variants.
                // size_t child_active_nodes = (key_less) ? get_active_nodes(current_node->left) : get_active_nodes(current_node->right);
                // if (child_active_nodes > alpha_ * current_node->active_nodes) {
                // Using subtree_size (total physical nodes) for balance condition:
                size_t child_subtree_size = (key_less) ? get_subtree_size(current_node->left) : get_subtree_size(current_node->right);
                if (child_subtree_size > alpha_ * current_node->subtree_size) {
                    if (!(*scapegoat_ancestor_ptr)) { // Found the highest imbalanced node
                        // This is tricky. We need to return a pointer to the unique_ptr that OWNS current_node.
                        // This recursive structure doesn't easily give that.
                        // Instead, insert_recursive will find the scapegoat and the main insert will call rebuild.
                        // Let's stick to the original plan: scapegoat_candidate raw pointer.
                        // The caller (main insert) will then find which unique_ptr owns this raw pointer.
                        // This is still complex.
                        //
                        // Simpler: if current_node is the scapegoat, the caller (parent in recursion)
                        // will receive the rebuilt tree.
                        // So, if current_node itself becomes the scapegoat, it will be rebuilt.
                        // The `scapegoat_ancestor_ptr` should point to the `std::unique_ptr` that holds the scapegoat.
                        // This means if `current_node` is the scapegoat, `scapegoat_ancestor_ptr` should effectively be
                        // a pointer to the unique_ptr that currently holds `current_node`.
                        // This is best handled by the caller of insert_recursive.
                        // For now, let's assume `scapegoat_ancestor_ptr` is correctly set by the main `insert` function
                        // to point to `root_` initially, and then to `node->left` or `node->right` before recursive call.
                        // This is still not quite right.
                        //
                        // Let insert_recursive return a pair: {new_node_root, scapegoat_found_in_this_subtree_bool}
                        // Or, the main `insert` will iterate down to find scapegoat.
                        //
                        // Let's stick to the raw pointer `scapegoat_candidate` as initially planned.
                        // The main `insert` function will handle finding the unique_ptr that owns it.
                        // This means `scapegoat_candidate` is an out-parameter.
                        // No, `scapegoat_ancestor_ptr` was the correct idea for the recursive version
                        // to allow the recursive call to modify the parent's unique_ptr.

                        // Let's simplify. The `insert` method will call `insert_recursive`.
                        // If `insert_recursive` detects an imbalance that makes `current_node` a scapegoat,
                        // it should rebuild `current_node` and return the new root of this rebuilt subtree.
                        // This means `rebuild_subtree_at_node` should be callable here.

                        // Let's refine `scapegoat_ancestor_ptr` to `Node*& scapegoat_node_found`.
                        // If after a recursive call, a scapegoat is identified *within the subtree handled by that call*,
                        // the rebuild should happen there.
                        // The scapegoat is the highest node on the path from root to inserted node that is unbalanced.
                        // So, as we return from recursion, if `current_node` is unbalanced due to insertion in its child,
                        // and `current_node` is deeper than or at the same level as any previously identified scapegoat
                        // (or if no scapegoat yet), then `current_node` is the new candidate.
                        // This is what `Node*& scapegoat_candidate` in the original plan did.
                         *scapegoat_ancestor_ptr = &current_node; // Mark this node's unique_ptr as the one to rebuild
                    }
                }
            }
        }
        return current_node;
    }

    // Flattens the subtree rooted at `node_ptr` into `nodes_vector`, collecting only active nodes.
    // Takes Node* because it doesn't modify ownership, just reads.
    void flatten_subtree_active_nodes(Node* node_ptr, std::vector<Node*>& nodes_vector) {
        if (!node_ptr) {
            return;
        }
        flatten_subtree_active_nodes(node_ptr->left.get(), nodes_vector);
        if (!node_ptr->is_deleted) {
            nodes_vector.push_back(node_ptr); // Store pointers to original nodes
        }
        flatten_subtree_active_nodes(node_ptr->right.get(), nodes_vector);
    }

    // Builds a perfectly balanced BST from a sorted vector of Node pointers.
    // The original nodes' memory is reused by moving their content.
    // The unique_ptrs in the original tree structure from which these nodes came
    // will be reset or re-pointed.
    std::unique_ptr<Node> build_balanced_from_active_nodes(std::vector<Node*>& node_pointers, int start, int end) {
        if (start > end) {
            return nullptr;
        }
        int mid = start + (end - start) / 2;
        Node* current_node_data_source = node_pointers[mid];

        std::unique_ptr<Node> new_node = std::make_unique<Node>(std::move(current_node_data_source->key), std::move(current_node_data_source->value));
        // Original node `current_node_data_source` is now effectively hollow.
        // Its is_deleted, subtree_size, active_nodes will be set by update_node_counts.
        new_node->is_deleted = false; // It's an active node in the new tree.

        new_node->left = build_balanced_from_active_nodes(node_pointers, start, mid - 1);
        new_node->right = build_balanced_from_active_nodes(node_pointers, mid + 1, end);

        update_node_counts(new_node.get());
        return new_node;
    }

    // Rebuilds the subtree pointed to by the unique_ptr `node_uptr_ref`.
    // `node_uptr_ref` is a reference to the unique_ptr that owns the scapegoat node.
    void rebuild_subtree_at_node(std::unique_ptr<Node>& node_uptr_ref) {
        if (!node_uptr_ref) return;

        std::vector<Node*> active_node_sources;
        // We need to pass the raw pointer to flatten, but ensure unique_ptrs in it are nulled out
        // to prevent double frees if these nodes are part of the vector for rebuild.
        // The `flatten_subtree_active_nodes` collects raw pointers. The actual nodes
        // are still owned by the tree structure.

        // Temporarily release children to avoid them being deleted if the current node is moved from.
        // This is complex. Let's make flatten_subtree_active_nodes create new Nodes or copy data.
        // No, the goal is to reuse nodes if possible, or at least their data.

        // Revised flatten: it will extract data and we make new nodes.
        // This simplifies ownership but might be less efficient if nodes are large.
        std::vector<std::pair<Key, Value>> active_node_data;
        std::function<void(std::unique_ptr<Node>&, std::vector<std::pair<Key, Value>>&)> collect_data =
            [&](std::unique_ptr<Node>& n, std::vector<std::pair<Key, Value>>& data_list) {
            if (!n) return;
            collect_data(n->left, data_list);
            if (!n->is_deleted) {
                data_list.emplace_back(std::move(n->key), std::move(n->value));
            }
            collect_data(n->right, data_list);
            // After moving data, the node `n` is effectively empty.
            // The unique_ptr `n` itself will be reset when its parent is rebuilt or `node_uptr_ref` is reassigned.
        };

        collect_data(node_uptr_ref, active_node_data); // This moves data out of the subtree at node_uptr_ref

        // The old subtree at node_uptr_ref is now "hollow" data-wise.
        // Reassigning node_uptr_ref will delete the old structure.

        // Re-calculate total_nodes_ and active_elements_ based on the rebuilt subtree.
        // This is tricky because rebuild_balanced_from_sorted_list_data only returns the new root.
        // The counts (total_nodes_, active_elements_) for the whole tree need adjustment.
        // When a subtree is rebuilt, total_nodes_ might decrease if logically deleted nodes are physically removed.
        // active_elements_ within that subtree remains the same.

        // Let's count nodes before clearing, for adjustment:
        size_t old_subtree_total_nodes = get_subtree_size(node_uptr_ref);
        // size_t old_subtree_active_nodes = get_active_nodes(node_uptr_ref); // Should be active_node_data.size()

        node_uptr_ref = build_balanced_from_sorted_list_data(active_node_data, 0, active_node_data.empty() ? -1 : static_cast<int>(active_node_data.size() - 1));

        // Adjust global counts
        size_t new_subtree_total_nodes = get_subtree_size(node_uptr_ref); // Should be active_node_data.size()
        total_nodes_ = total_nodes_ - old_subtree_total_nodes + new_subtree_total_nodes;
        // active_elements_ count should remain consistent globally if only active nodes are rebuilt.
        // The active_elements_ count of the rebuilt subtree is active_node_data.size().
        // This logic needs to be very careful. The main `active_elements_` tracks the whole tree.
    }

    // Helper for building from data pairs
    std::unique_ptr<Node> build_balanced_from_sorted_list_data(const std::vector<std::pair<Key, Value>>& data_list, int start, int end) {
        if (start > end) {
            return nullptr;
        }
        int mid = start + (end - start) / 2;
        // Create new node by copying or moving from data_list[mid]
        // Here, we copy from data_list as it's const&
        std::unique_ptr<Node> new_node = std::make_unique<Node>(data_list[mid].first, data_list[mid].second);
        new_node->is_deleted = false;

        new_node->left = build_balanced_from_sorted_list_data(data_list, start, mid - 1);
        new_node->right = build_balanced_from_sorted_list_data(data_list, mid + 1, end);

        update_node_counts(new_node.get());
        return new_node;
    }


    // Global rebuild
    void rebuild_entire_tree() {
        std::vector<std::pair<Key, Value>> active_node_data;
        std::function<void(std::unique_ptr<Node>&, std::vector<std::pair<Key, Value>>&)> collect_data =
            [&](std::unique_ptr<Node>& n, std::vector<std::pair<Key, Value>>& data_list) {
            if (!n) return;
            collect_data(n->left, data_list);
            if (!n->is_deleted) {
                // Assuming Key and Value are movable for efficiency
                data_list.emplace_back(std::move(n->key), std::move(n->value));
            }
            collect_data(n->right, data_list);
        };

        collect_data(root_, active_node_data);

        root_ = build_balanced_from_sorted_list_data(active_node_data, 0, active_node_data.empty() ? -1 : static_cast<int>(active_node_data.size() - 1));

        // Reset global counts based on the new tree
        total_nodes_ = active_elements_; // After global rebuild, all nodes are active
        max_total_nodes_since_rebuild_ = total_nodes_;
        // active_elements_ remains the same.
    }

    void check_and_rebuild_globally_if_needed() {
        if (total_nodes_ > 0 && active_elements_ * 2 < total_nodes_ && total_nodes_ > 10) { // If more than half are deleted (and tree is non-trivial)
            rebuild_entire_tree();
        }
    }

    // Finds the unique_ptr that owns the target_raw_ptr and calls rebuild_subtree_at_node on it.
    // This is complex due to unique_ptr ownership. It's easier if rebuild is called on the unique_ptr directly
    // by the function that has access to it (e.g., the recursive parent or the main insert/erase).
    // Let's simplify: insert_recursive will set a flag or return the scapegoat, and `insert` will manage the rebuild on the correct unique_ptr.
    // The `scapegoat_candidate` raw pointer approach in original plan for insert_recursive:
    // `insert` calls `insert_recursive(std::move(root_), ..., &scapegoat_raw_ptr)`.
    // If `scapegoat_raw_ptr` is set, `insert` needs to find which of its unique_ptrs (`root_`, `root_->left`, etc.)
    // owns this raw pointer. This is still cumbersome.
    //
    // Best approach: `insert_recursive` returns the `unique_ptr` to the modified subtree.
    // If it identifies ITS OWN ROOT (`current_node` passed to it) as the scapegoat, it rebuilds itself
    // and returns the `unique_ptr` to the new, rebuilt root of that subtree.

    // Modified insert_recursive to handle scapegoat detection and trigger rebuild.
    // It will now return the unique_ptr to the root of the (potentially rebuilt) subtree.
    // `scapegoat_found_on_path` is an out-parameter to signal to the caller if a rebuild happened deeper.
    std::unique_ptr<Node> insert_recursive_with_rebuild(std::unique_ptr<Node> current_node_uptr, const Key& key, const Value& value, int current_depth, int& inserted_node_depth, bool& scapegoat_found_on_path) {
        if (!current_node_uptr) {
            inserted_node_depth = current_depth;
            total_nodes_++;
            active_elements_++;
            max_total_nodes_since_rebuild_ = std::max(max_total_nodes_since_rebuild_, total_nodes_);
            scapegoat_found_on_path = false; // No scapegoat deeper than a new leaf
            return std::make_unique<Node>(key, value);
        }

        Node* current_node_raw = current_node_uptr.get(); // Get raw pointer for comparisons, etc.
        bool recursive_scapegoat_found = false;

        bool key_less = compare_(key, current_node_raw->key);
        bool key_greater = compare_(current_node_raw->key, key);

        if (key_less) {
            current_node_uptr->left = insert_recursive_with_rebuild(std::move(current_node_uptr->left), key, value, current_depth + 1, inserted_node_depth, recursive_scapegoat_found);
        } else if (key_greater) {
            current_node_uptr->right = insert_recursive_with_rebuild(std::move(current_node_uptr->right), key, value, current_depth + 1, inserted_node_depth, recursive_scapegoat_found);
        } else { // Key already exists
            if (current_node_raw->is_deleted) {
                current_node_raw->is_deleted = false;
                current_node_raw->value = value;
                active_elements_++;
                inserted_node_depth = current_depth;
            } else {
                current_node_raw->value = value;
                inserted_node_depth = -1;
            }
            scapegoat_found_on_path = false; // No structural change, no deeper scapegoat
        }

        if (recursive_scapegoat_found) { // If a child call already found and handled a scapegoat
            update_node_counts(current_node_raw);
            scapegoat_found_on_path = true; // Propagate upwards
            return current_node_uptr;
        }

        update_node_counts(current_node_raw);

        if (inserted_node_depth != -1) {
             // Check if current_node_uptr.get() is the scapegoat
            bool current_is_scapegoat = false;
            if (current_depth < inserted_node_depth) { // Only ancestors of the new/reactivated node
                 // Using subtree_size for balance:
                size_t child_size_for_check = (key_less) ? get_subtree_size(current_node_uptr->left) : get_subtree_size(current_node_uptr->right);
                if (child_size_for_check > alpha_ * current_node_uptr->subtree_size) {
                    current_is_scapegoat = true;
                }
            }

            if (current_is_scapegoat) {
                // current_node_uptr is the scapegoat. Rebuild the subtree it roots.
                // The rebuild_subtree_at_node takes a reference to a unique_ptr.
                // Here, current_node_uptr *is* that unique_ptr.
                rebuild_subtree_at_node(current_node_uptr); // This modifies current_node_uptr in place
                scapegoat_found_on_path = true; // Signal that a rebuild happened
                // Counts are updated within rebuild_subtree_at_node and its helpers.
                // The root of the rebuilt subtree is now in current_node_uptr.
            } else {
                scapegoat_found_on_path = false;
            }
        } else {
            scapegoat_found_on_path = false; // No insertion, so no new imbalance from this path
        }
        return current_node_uptr;
    }


public:
    // Constructor
    explicit ScapegoatTree(double alpha = 0.75) : compare_(Compare()), alpha_(alpha) {
        if (alpha_ <= 0.5 || alpha_ >= 1.0) { // Alpha should be (0.5, 1.0)
            throw std::invalid_argument("Alpha must be strictly between 0.5 and 1.0");
        }
    }

    // Destructor - unique_ptr will handle recursive deletion
    ~ScapegoatTree() = default;

    bool insert(const Key& key, const Value& value) {
        int inserted_depth = -1;
        bool scapegoat_handled = false; // Out-param to track if a rebuild occurred
        root_ = insert_recursive_with_rebuild(std::move(root_), key, value, 0, inserted_depth, scapegoat_handled);

        if (inserted_depth != -1) { // If actual insertion or reactivation happened
             max_total_nodes_since_rebuild_ = std::max(max_total_nodes_since_rebuild_, total_nodes_);
             // Check for global rebuild if many nodes are deleted relative to max size or current active size
             // This condition might need tuning. Original paper suggests total_nodes > max_total_nodes_since_rebuild_ * alpha is problematic
             // Or if active_elements_ < alpha_ * total_nodes_
             if (active_elements_ < alpha_ * total_nodes_ && total_nodes_ > 10) { // Heuristic
                 rebuild_entire_tree();
             }
        }
        return inserted_depth != -1;
    }

    // Overload for rvalue Value (Key is usually copied for BSTs)
    bool insert(const Key& key, Value&& value) {
        // This requires insert_recursive_with_rebuild to be templated for Value type or take Value&&
        // For now, convert to const ref call for simplicity.
        // Proper implementation would involve perfect forwarding.
        const Value& val_ref = value;
        return insert(key, val_ref); // This will copy, then move internally. Not ideal.
        // A better approach would be to template insert_recursive_with_rebuild on value type.
    }

    // Erase (lazy deletion)
    // TODO: After erase, check if a rebuild is needed due to too many deleted nodes.
    // Also, erase might trigger an imbalance if we were using active_nodes for balance checks,
    // but with subtree_size, it doesn't directly cause alpha-imbalance for scapegoat finding.
    // However, the global condition (active_elements vs total_nodes) is important.
    bool erase(const Key& key) {
        Node* node_to_delete = find_node(root_.get(), key); // find_node already checks is_deleted
        if (node_to_delete && !node_to_delete->is_deleted) { // Double check is_deleted before marking
            node_to_delete->is_deleted = true;
            active_elements_--;

            // Update active_nodes counts upwards. This is tricky without parent pointers.
            // The counts will be fixed during the next rebuild. For now, this is a known limitation.
            // A full traversal or specific path update would be needed.
            // Alternatively, accept that active_nodes in subtrees might be stale until rebuild.

            // Check for global rebuild condition
             if (active_elements_ < alpha_ * total_nodes_ && total_nodes_ > 10) {
                 rebuild_entire_tree();
             }
            return true;
        }
        return false;
    }

    const Value* find(const Key& key) const {
        Node* node = find_node(root_.get(), key);
        if (node && !node->is_deleted) {
            return &(node->value);
        }
        return nullptr;
    }

    bool contains(const Key& key) const {
        Node* node = find_node(root_.get(), key);
        return node && !node->is_deleted;
    }

    size_t size() const noexcept { // Number of active elements
        return active_elements_;
    }

    bool empty() const noexcept {
        return active_elements_ == 0;
    }

    void clear() {
        root_.reset(); // Destroys all nodes
        total_nodes_ = 0;
        active_elements_ = 0;
        max_total_nodes_since_rebuild_ = 0;
    }

    // Iterators (to be added)

    // --- Iterator Implementation ---
private:
    template <bool IsConst>
    class ScapegoatTreeIterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = std::pair<const Key, Value>; // What operator* returns (conceptually)
        using difference_type = std::ptrdiff_t;

        // The actual reference type returned by operator*
        // It's a proxy because the Key is const but Value might not be.
        struct PairProxy {
            const Key& first;
            typename std::conditional_t<IsConst, const Value&, Value&> second;
        };
        using reference = PairProxy;

        // Pointer type for operator->
        // Needs to be a pointer to something that has 'first' and 'second' members.
        // Can return a pointer to a temporary PairProxy, or PairProxy itself if it has operator->
        // Let's make operator-> return PairProxy by value, and PairProxy will have its own operator->
        // if Value is a class/struct. For simplicity, if Value is not a class, this might be tricky.
        // A common approach for map-like iterators is that operator->() returns a pointer to the internal std::pair.
        // Since we construct PairProxy on the fly, we can return a pointer to a member that holds it,
        // or return PairProxy itself if it defines operator->
        // For now, let operator-> return a pointer to the PairProxy created by operator*
        // This requires operator* to return by value or have its result stored.
        // Let's make operator* return PairProxy by value, and operator-> return a pointer to a temporary.
        // This is not ideal.
        //
        // Alternative: operator-> returns a pointer to the node's value_type if Key is const.
        // But our value_type is std::pair<const Key, Value>.
        // The node stores Key and Value separately.
        //
        // Let's make operator* return PairProxy by value.
        // Let operator-> return a pointer to a PairProxy, this implies PairProxy needs to be stored.
        //
        // Simplest approach that mimics std::map iterators:
        // operator* returns a reference to a pair-like object.
        // operator-> returns a pointer to that pair-like object.
        // Our PairProxy is the pair-like object.
        using pointer = PairProxy*; // This would mean operator* needs to return a reference to a stable PairProxy.

        // Let's try this: operator* returns PairProxy by value.
        // operator-> is often `&(operator*())` if operator* returns a reference.
        // If operator* returns by value, then operator-> often returns the value itself
        // if that value has an operator-> (like smart pointers).
        // Or, it returns a pointer to a member that holds the result of operator*().
        //
        // For now, make operator-> return the PairProxy itself and let the user deal with it.
        // This means pointer type is PairProxy.
        using pointer_proxy = PairProxy;


    private:
        Node* current_node_;
        std::vector<Node*> path_stack_; // Stack to keep track of the path for inorder traversal
        const ScapegoatTree* tree_ptr_; // Pointer to the tree, needed for end iterator comparison

        // Helper to find the leftmost non-deleted node from a given node
        void find_leftmost_leaf(Node* node) {
            while (node) {
                path_stack_.push_back(node);
                if (!node->is_deleted && node->left && tree_ptr_->get_active_nodes(node->left) > 0) {
                     node = node->left.get();
                } else if (!node->is_deleted) { // Current node is active and no more left active children
                    current_node_ = node;
                    return;
                } else if (node->right && tree_ptr_->get_active_nodes(node->right) > 0) { // Current node deleted, try right
                    node = node->right.get();
                } else { // Current node deleted, no active right child, pop and try parent's right
                    break;
                }
            }
            // If loop finishes, current_node_ might be null or stack is used to find next.
            // This initial setup might be complex. A simpler way:
            // Push all left children until null. Then current is top of stack if not deleted.
            // If deleted, advance.

            path_stack_.clear(); // Reset stack for simpler find_leftmost
            Node* p = node;
            while(p) {
                path_stack_.push_back(p);
                p = p->left.get();
            }
            advance_to_valid_node(); // Find the first valid node
        }

        // Advance to the next valid (non-deleted) node in-order
        void advance_to_valid_node() {
            while (!path_stack_.empty()) {
                current_node_ = path_stack_.back();
                path_stack_.pop_back();

                if (!current_node_->is_deleted) {
                    // Found an active node. Now, prepare for the next call to ++
                    // by adding its right child and all left children of the right child.
                    Node* p = current_node_->right.get();
                    while (p) {
                        path_stack_.push_back(p);
                        p = p->left.get();
                    }
                    return; // current_node_ is now the valid node
                }
                // If current_node_ is deleted, try its right subtree
                Node* p = current_node_->right.get();
                while (p) {
                    path_stack_.push_back(p);
                    p = p->left.get();
                }
                // Continue loop to find next valid node from stack
            }
            current_node_ = nullptr; // Reached end
        }


    public:
        // Default constructor (end iterator)
        ScapegoatTreeIterator(const ScapegoatTree* tree = nullptr) : current_node_(nullptr), tree_ptr_(tree) {}

        // Begin iterator constructor
        ScapegoatTreeIterator(Node* start_node, const ScapegoatTree* tree) : tree_ptr_(tree) {
            find_leftmost_leaf(start_node); // Sets current_node_ and path_stack_
        }

        reference operator*() const {
            // Should only be called on valid, non-end iterators
            return {current_node_->key, current_node_->value};
        }

        pointer_proxy operator->() const {
            return {current_node_->key, current_node_->value};
        }

        ScapegoatTreeIterator& operator++() { // Prefix increment
            advance_to_valid_node();
            return *this;
        }

        ScapegoatTreeIterator operator++(int) { // Postfix increment
            ScapegoatTreeIterator temp = *this;
            ++(*this);
            return temp;
        }

        // Bidirectional: operator-- (more complex, requires parent pointers or different stack logic)
        // For now, let's keep it as forward iterator and add bidirectional later if needed.
        // To make it bidirectional, path_stack_ needs to store parent pointers as we go down.

        bool operator==(const ScapegoatTreeIterator& other) const {
            return current_node_ == other.current_node_ && tree_ptr_ == other.tree_ptr_;
        }

        bool operator!=(const ScapegoatTreeIterator& other) const {
            return !(*this == other);
        }

        // Conversion from non-const to const iterator
        operator ScapegoatTreeIterator<true>() const {
            return ScapegoatTreeIterator<true>(current_node_, tree_ptr_);
        }
    };

public:
    using iterator = ScapegoatTreeIterator<false>;
    using const_iterator = ScapegoatTreeIterator<true>;

    iterator begin() {
        if (!root_) return iterator(this); // End iterator if tree is empty
        return iterator(root_.get(), this);
    }

    const_iterator begin() const {
        if (!root_) return const_iterator(this);
        return const_iterator(root_.get(), this);
    }

    const_iterator cbegin() const {
        return begin();
    }

    iterator end() {
        return iterator(this); // Default constructor is end iterator
    }

    const_iterator end() const {
        return const_iterator(this);
    }

    const_iterator cend() const {
        return end();
    }

};

} // namespace cpp_collections

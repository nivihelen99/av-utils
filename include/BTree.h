/**
 * @file BTree.h
 * @brief Defines the BTree class, an in-memory B-Tree implementation.
 */
#ifndef BTREE_H
#define BTREE_H

#include <vector>
#include <memory>
#include <functional> // For std::less
#include <algorithm> // For std::upper_bound, std::copy, std::move
#include <stdexcept> // For std::out_of_range

namespace cpp_collections {

/**
 * @brief A B-Tree data structure.
 *
 * This class implements an in-memory B-Tree that stores key-value pairs.
 * B-Trees are balanced search trees optimized for systems that read and write
 * large blocks of data. They are commonly used in databases and filesystems.
 *
 * @tparam Key The type of the keys.
 * @tparam Value The type of the values associated with the keys.
 * @tparam Compare A comparison functor for keys. Defaults to std::less<Key>.
 *                 It should define a strict weak ordering.
 * @tparam MinDegree The minimum degree (often denoted as 't') of the B-Tree.
 *                   Each node (except possibly the root) must have at least MinDegree-1 keys
 *                   and at most 2*MinDegree-1 keys. Each internal node (except root)
 *                   must have at least MinDegree children and at most 2*MinDegree children.
 *                   MinDegree must be >= 2.
 */
template <typename Key, typename Value, typename Compare = std::less<Key>, int MinDegree = 2>
class BTree {
public:
    static_assert(MinDegree >= 2, "MinDegree must be at least 2");

    /**
     * @struct BTreeNode
     * @brief Represents a node in the B-Tree.
     *
     * Each node contains keys, associated values (paired with keys),
     * child pointers (for internal nodes), a leaf flag, and the current number of keys.
     */
    struct BTreeNode {
        using KeyValueType = std::pair<Key, Value>; ///< Type for key-value pairs stored in nodes.

        std::vector<KeyValueType> keys;             ///< Vector to store key-value pairs.
        std::vector<std::unique_ptr<BTreeNode>> children; ///< Vector of child node pointers.
        bool leaf;                                  ///< True if this node is a leaf, false otherwise.
        size_t num_keys;                            ///< Current number of keys in this node.

        /**
         * @brief Constructs a BTreeNode.
         * @param is_leaf True if the node is a leaf, false otherwise.
         */
        BTreeNode(bool is_leaf) : leaf(is_leaf), num_keys(0) {
            keys.reserve(2 * MinDegree - 1);    // Max keys: 2*t - 1
            children.reserve(2 * MinDegree);    // Max children: 2*t
        }

        /**
         * @brief Finds the index of the first key in this node that is greater than or equal to k.
         * @param k The key to search for.
         * @param comp The comparison functor.
         * @return The index of the key, or num_keys if all keys are smaller than k.
         * @note Currently uses linear scan; can be optimized to binary search for large MinDegree.
         */
        size_t find_key_idx(const Key& k, const Compare& comp) const {
            size_t idx = 0;
            while (idx < num_keys && comp(keys[idx].first, k)) {
                idx++;
            }
            return idx;
        }

        // Helper methods for inserting keys/children, used internally by BTree.
        // These are simplified and primarily for illustration within BTreeNode if needed directly,
        // but BTree methods manage these operations more comprehensively.
        void insert_key_value(const KeyValueType& kv_pair, const Compare& comp) {
            size_t i = num_keys;
            keys.resize(num_keys + 1);
            while (i > 0 && comp(kv_pair.first, keys[i - 1].first)) {
                keys[i] = std::move(keys[i - 1]);
                i--;
            }
            keys[i] = kv_pair;
            num_keys++;
        }
         void insert_child(std::unique_ptr<BTreeNode> child, size_t child_idx) {
            children.insert(children.begin() + child_idx, std::move(child));
        }
    };

private:
    std::unique_ptr<BTreeNode> root_;   ///< Pointer to the root node of the B-Tree.
    Compare compare_;                   ///< Comparison functor instance.
    const int min_degree_ = MinDegree;  ///< The minimum degree 't' of the B-Tree.

    // --- Private Helper Methods for B-Tree Operations ---

    /**
     * @brief Inserts a key-value pair into a non-full node.
     * @param node The node to insert into (must not be full).
     * @param k The key to insert.
     * @param v The value to insert.
     */
    void insert_non_full(BTreeNode* node, const Key& k, const Value& v) {
        int i = node->num_keys - 1;

        if (node->leaf) {
            auto insert_pos_it = node->keys.begin();
            while(insert_pos_it != node->keys.begin() + node->num_keys && compare_(insert_pos_it->first, k)) {
                insert_pos_it++;
            }
            node->keys.insert(insert_pos_it, {k,v});
            node->num_keys++;
        } else {
            while (i >= 0 && compare_(k, node->keys[i].first)) {
                i--;
            }
            int child_to_descend_idx = i + 1;

            if (node->children[child_to_descend_idx]->num_keys == static_cast<size_t>(2 * min_degree_ - 1)) {
                split_child(node, child_to_descend_idx);
                if (compare_(node->keys[child_to_descend_idx].first, k)) {
                    child_to_descend_idx++;
                }
            }
            insert_non_full(node->children[child_to_descend_idx].get(), k, v);
        }
    }

    /**
     * @brief Splits a full child of a given node.
     * @param parent_node The parent of the child to be split.
     * @param child_idx_in_parent The index of the full child in parent_node's children list.
     */
    void split_child(BTreeNode* parent_node, int child_idx_in_parent) {
        BTreeNode* child_to_split = parent_node->children[child_idx_in_parent].get();
        auto new_sibling = std::make_unique<BTreeNode>(child_to_split->leaf);
        new_sibling->num_keys = min_degree_ - 1;

        new_sibling->keys.resize(min_degree_ - 1);
        for (int j = 0; j < min_degree_ - 1; ++j) {
            new_sibling->keys[j] = std::move(child_to_split->keys[j + min_degree_]);
        }

        if (!child_to_split->leaf) {
            new_sibling->children.resize(min_degree_);
            for (int j = 0; j < min_degree_; ++j) {
                new_sibling->children[j] = std::move(child_to_split->children[j + min_degree_]);
            }
            child_to_split->children.resize(min_degree_);
        }

        typename BTreeNode::KeyValueType middle_key_from_child = std::move(child_to_split->keys[min_degree_ - 1]);

        child_to_split->keys.resize(min_degree_ - 1);
        child_to_split->num_keys = min_degree_ - 1;

        parent_node->children.insert(parent_node->children.begin() + child_idx_in_parent + 1, std::move(new_sibling));
        parent_node->keys.insert(parent_node->keys.begin() + child_idx_in_parent, std::move(middle_key_from_child));
        parent_node->num_keys++;
    }

    /**
     * @brief Recursively searches for a key in the B-Tree starting from a given node.
     * @param node The current node to search in.
     * @param k The key to search for.
     * @return Pointer to the KeyValueType pair if found, otherwise nullptr.
     */
    typename BTreeNode::KeyValueType* search_recursive(BTreeNode* node, const Key& k) {
        if (!node) {
            return nullptr;
        }
        size_t i = 0;
        while (i < node->num_keys && compare_(node->keys[i].first, k)) {
            i++;
        }
        if (i < node->num_keys && !compare_(k, node->keys[i].first) && !compare_(node->keys[i].first, k)) {
            return &(node->keys[i]);
        }
        if (node->leaf) {
            return nullptr;
        }
        if (i < node->children.size()) {
             return search_recursive(node->children[i].get(), k);
        } else {
            return nullptr;
        }
    }

    // (Placeholder for private deletion helper methods)

public:
    /**
     * @brief Default constructor. Creates an empty B-Tree.
     */
    BTree() : root_(nullptr), compare_(Compare()) {}

    /**
     * @brief Searches for a key in the B-Tree.
     * @param k The key to search for.
     * @return A pointer to the value if the key is found, otherwise nullptr.
     * @complexity O(t log_t N) where t is MinDegree and N is the number of keys.
     */
    Value* search(const Key& k) {
        if (!root_) return nullptr;
        typename BTreeNode::KeyValueType* found_pair = search_recursive(root_.get(), k);
        return found_pair ? &(found_pair->second) : nullptr;
    }

    /**
     * @brief Searches for a key in the B-Tree (const version).
     * @param k The key to search for.
     * @return A const pointer to the value if the key is found, otherwise nullptr.
     * @complexity O(t log_t N)
     */
    const Value* search(const Key& k) const {
        if (!root_) return nullptr;
        typename BTreeNode::KeyValueType* found_pair = const_cast<BTree*>(this)->search_recursive(root_.get(), k);
        return found_pair ? &(found_pair->second) : nullptr;
    }

    /**
     * @brief Inserts a key-value pair into the B-Tree.
     * If the key already exists, its value is typically not updated (standard B-Tree behavior).
     * For map-like update-or-insert, a separate method or check would be needed.
     * @param k The key to insert.
     * @param v The value to associate with the key.
     * @complexity O(t log_t N)
     */
    void insert(const Key& k, const Value& v) {
        if (!root_) {
            root_ = std::make_unique<BTreeNode>(true /*is_leaf*/);
            root_->keys.emplace_back(k, v);
            root_->num_keys = 1;
            return;
        }

        if (root_->num_keys == static_cast<size_t>(2 * min_degree_ - 1)) {
            auto new_root = std::make_unique<BTreeNode>(false /*is_leaf*/);
            new_root->children.push_back(std::move(root_));
            root_ = std::move(new_root);
            split_child(root_.get(), 0);
            insert_non_full(root_.get(), k, v);
        } else {
            insert_non_full(root_.get(), k, v);
        }
    }

    // void remove(const Key& k); // (Placeholder for public remove method)

    /**
     * @brief Checks if the B-Tree is empty.
     * @return True if the tree contains no keys, false otherwise.
     */
    bool is_empty() const {
        return root_ == nullptr;
    }

    /**
     * @brief Gets the minimum degree 't' of the B-Tree.
     * @return The minimum degree.
     */
    int get_min_degree() const {
        return min_degree_;
    }

    /**
     * @brief Destructor. Cleans up nodes using std::unique_ptr.
     */
    ~BTree() = default;
};

} // namespace cpp_collections

#endif // BTREE_H

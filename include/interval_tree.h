#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <functional>
#include <cassert>
#include <cstdint>

namespace interval_tree {

template <typename T>
struct Interval {
    int64_t start;
    int64_t end;  // Exclusive: [start, end)
    T value;

    Interval(int64_t s, int64_t e, T val) 
        : start(s), end(e), value(std::move(val)) {
        assert(start < end);
    }

    bool overlaps(int64_t point) const {
        return start <= point && point < end;
    }

    bool overlaps(int64_t range_start, int64_t range_end) const {
        return start < range_end && range_start < end;
    }

    bool operator==(const Interval& other) const {
        return start == other.start && end == other.end && value == other.value;
    }
};

template <typename T>
class IntervalTree {
private:
    struct Node {
        Interval<T> iv;
        int64_t max_end;
        int height;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;

        Node(Interval<T> interval) 
            : iv(std::move(interval)), max_end(iv.end), height(1) {}
    };

    std::unique_ptr<Node> root;
    size_t tree_size = 0;

    // AVL Tree operations
    int get_height(const std::unique_ptr<Node>& node) {
        return node ? node->height : 0;
    }

    int get_balance(const std::unique_ptr<Node>& node) {
        return node ? get_height(node->left) - get_height(node->right) : 0;
    }

    void update_node(std::unique_ptr<Node>& node) {
        if (!node) return;
        
        node->height = 1 + std::max(get_height(node->left), get_height(node->right));
        
        node->max_end = node->iv.end;
        if (node->left) {
            node->max_end = std::max(node->max_end, node->left->max_end);
        }
        if (node->right) {
            node->max_end = std::max(node->max_end, node->right->max_end);
        }
    }

    std::unique_ptr<Node> rotate_right(std::unique_ptr<Node> y) {
        auto x = std::move(y->left);
        y->left = std::move(x->right);
        x->right = std::move(y);
        
        update_node(x->right);
        update_node(x);
        
        return x;
    }

    std::unique_ptr<Node> rotate_left(std::unique_ptr<Node> x) {
        auto y = std::move(x->right);
        x->right = std::move(y->left);
        y->left = std::move(x);
        
        update_node(y->left);
        update_node(y);
        
        return y;
    }

    std::unique_ptr<Node> insert_impl(std::unique_ptr<Node> node, Interval<T> iv) {
        // BST insertion
        if (!node) {
            ++tree_size;
            return std::make_unique<Node>(std::move(iv));
        }

        if (iv.start < node->iv.start || 
            (iv.start == node->iv.start && iv.end < node->iv.end)) {
            node->left = insert_impl(std::move(node->left), std::move(iv));
        } else {
            node->right = insert_impl(std::move(node->right), std::move(iv));
        }

        update_node(node);

        // AVL balancing
        int balance = get_balance(node);

        // Left Left Case
        if (balance > 1 && get_balance(node->left) >= 0) {
            return rotate_right(std::move(node));
        }

        // Right Right Case
        if (balance < -1 && get_balance(node->right) <= 0) {
            return rotate_left(std::move(node));
        }

        // Left Right Case
        if (balance > 1 && get_balance(node->left) < 0) {
            node->left = rotate_left(std::move(node->left));
            return rotate_right(std::move(node));
        }

        // Right Left Case
        if (balance < -1 && get_balance(node->right) > 0) {
            node->right = rotate_right(std::move(node->right));
            return rotate_left(std::move(node));
        }

        return node;
    }

    std::unique_ptr<Node> find_min(std::unique_ptr<Node>& node) {
        while (node->left) {
            node = std::move(node->left);
        }
        return std::move(node);
    }

    std::unique_ptr<Node> remove_impl(std::unique_ptr<Node> node, const Interval<T>& iv) {
        if (!node) return nullptr;

        if (iv.start < node->iv.start || 
            (iv.start == node->iv.start && iv.end < node->iv.end)) {
            node->left = remove_impl(std::move(node->left), iv);
        } else if (node->iv.start < iv.start || 
                   (node->iv.start == iv.start && node->iv.end < iv.end)) {
            node->right = remove_impl(std::move(node->right), iv);
        } else if (node->iv == iv) {
            // Found the node to delete
            --tree_size;
            
            if (!node->left || !node->right) {
                node = node->left ? std::move(node->left) : std::move(node->right);
            } else {
                // Node with two children: get inorder successor
                auto temp = std::move(node->right);
                auto min_node = find_min(temp);
                
                node->iv = std::move(min_node->iv);
                node->right = remove_impl(std::move(temp), node->iv);
            }
        } else {
            // Same start and end but different value, continue searching
            node->right = remove_impl(std::move(node->right), iv);
        }

        if (!node) return node;

        update_node(node);

        // AVL balancing
        int balance = get_balance(node);

        if (balance > 1 && get_balance(node->left) >= 0) {
            return rotate_right(std::move(node));
        }

        if (balance > 1 && get_balance(node->left) < 0) {
            node->left = rotate_left(std::move(node->left));
            return rotate_right(std::move(node));
        }

        if (balance < -1 && get_balance(node->right) <= 0) {
            return rotate_left(std::move(node));
        }

        if (balance < -1 && get_balance(node->right) > 0) {
            node->right = rotate_right(std::move(node->right));
            return rotate_left(std::move(node));
        }

        return node;
    }

    void query_point_impl(const std::unique_ptr<Node>& node, int64_t point, 
                         std::vector<Interval<T>>& result) const {
        if (!node) return;

        // If point is beyond max_end of this subtree, no overlaps possible
        if (point >= node->max_end) return;

        // Check left subtree
        query_point_impl(node->left, point, result);

        // Check current node
        if (node->iv.overlaps(point)) {
            result.push_back(node->iv);
        }

        // Check right subtree only if point >= node start
        if (point >= node->iv.start) {
            query_point_impl(node->right, point, result);
        }
    }

    void query_range_impl(const std::unique_ptr<Node>& node, int64_t start, int64_t end,
                         std::vector<Interval<T>>& result) const {
        if (!node) return;

        // If range end is beyond max_end of this subtree, no overlaps possible
        if (start >= node->max_end) return;

        // Check left subtree
        query_range_impl(node->left, start, end, result);

        // Check current node
        if (node->iv.overlaps(start, end)) {
            result.push_back(node->iv);
        }

        // Check right subtree only if range overlaps with node's start
        if (end > node->iv.start) {
            query_range_impl(node->right, start, end, result);
        }
    }

    void collect_all_impl(const std::unique_ptr<Node>& node, 
                         std::vector<Interval<T>>& result) const {
        if (!node) return;
        
        collect_all_impl(node->left, result);
        result.push_back(node->iv);
        collect_all_impl(node->right, result);
    }

public:
    IntervalTree() = default;

    // Non-copyable for simplicity (could be implemented)
    IntervalTree(const IntervalTree&) = delete;
    IntervalTree& operator=(const IntervalTree&) = delete;

    // Moveable
    IntervalTree(IntervalTree&& other) noexcept
        : root(std::move(other.root)), tree_size(other.tree_size) {
        other.root = nullptr;
        other.tree_size = 0;
    }
    IntervalTree& operator=(IntervalTree&& other) noexcept {
        if (this != &other) {
            root = std::move(other.root);
            tree_size = other.tree_size;
            other.root = nullptr;
            other.tree_size = 0;
        }
        return *this;
    }

    void insert(int64_t start, int64_t end, const T& value) {
        root = insert_impl(std::move(root), Interval<T>(start, end, value));
    }

    void insert(int64_t start, int64_t end, T&& value) {
        root = insert_impl(std::move(root), Interval<T>(start, end, std::move(value)));
    }

    void remove(int64_t start, int64_t end, const T& value) {
        root = remove_impl(std::move(root), Interval<T>(start, end, value));
    }

    std::vector<Interval<T>> query(int64_t point) const {
        std::vector<Interval<T>> result;
        query_point_impl(root, point, result);
        return result;
    }

    std::vector<Interval<T>> query(int64_t start, int64_t end) const {
        std::vector<Interval<T>> result;
        query_range_impl(root, start, end, result);
        return result;
    }

    std::vector<Interval<T>> all() const {
        std::vector<Interval<T>> result;
        result.reserve(tree_size);
        collect_all_impl(root, result);
        return result;
    }

    size_t size() const noexcept {
        return tree_size;
    }

    bool empty() const noexcept {
        return tree_size == 0;
    }

    void clear() {
        root.reset();
        tree_size = 0;
    }
};

} // namespace interval_tree

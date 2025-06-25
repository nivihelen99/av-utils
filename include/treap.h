#ifndef TREAP_H
#define TREAP_H

#include <functional>
#include <memory>
#include <random>
#include <stdexcept>
#include <utility> // For std::pair
#include <vector> // For iterator implementation

// Forward declaration for iterator
template <typename Key, typename Value, typename Compare> // Removed default for Compare here
class Treap;

template <typename Key, typename Value, typename Compare = std::less<Key>> // Kept default for Compare here
class TreapIterator {
public:
    using NodeType = typename Treap<Key, Value, Compare>::Node; // This needs to match the forward declaration
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<const Key, Value>;
    using pointer = value_type*;
    using reference = value_type&;
    using difference_type = std::ptrdiff_t;

private:
    NodeType* current_node_ = nullptr;
    std::vector<NodeType*> path_; // Stack to keep track of the path for inorder traversal
    const Treap<Key, Value, Compare>* treap_ = nullptr;

    void push_left_path(NodeType* node) {
        while (node) {
            path_.push_back(node);
            node = node->left.get();
        }
    }

public:
    TreapIterator() = default;
    TreapIterator(NodeType* start_node, const Treap<Key, Value, Compare>* treap_instance)
        : treap_(treap_instance) {
        push_left_path(start_node);
        if (!path_.empty()) {
            current_node_ = path_.back();
        } else {
            current_node_ = nullptr;
        }
    }

    reference operator*() const {
        return current_node_->data;
    }

    pointer operator->() const {
        return &operator*();
    }

    TreapIterator& operator++() {
        if (path_.empty()) {
            current_node_ = nullptr;
            return *this;
        }

        NodeType* node = path_.back();
        path_.pop_back();

        push_left_path(node->right.get());

        if (!path_.empty()) {
            current_node_ = path_.back();
        } else {
            current_node_ = nullptr;
        }
        return *this;
    }

    TreapIterator operator++(int) {
        TreapIterator temp = *this;
        ++(*this);
        return temp;
    }

    bool operator==(const TreapIterator& other) const {
        return current_node_ == other.current_node_ && path_ == other.path_ && treap_ == other.treap_;
    }

    bool operator!=(const TreapIterator& other) const {
        return !(*this == other);
    }

    friend class Treap<Key, Value, Compare>;
};


template <typename Key, typename Value, typename Compare = std::less<Key>>
class Treap {
public:
    struct Node {
        std::pair<const Key, Value> data;
        int priority;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;

        Node(Key k, Value v, int p)
            : data(std::move(k), std::move(v)), priority(p), left(nullptr), right(nullptr) {}
    };

private:
    std::unique_ptr<Node> root_;
    Compare compare_;
    std::mt19937 random_engine_;
    size_t size_ = 0;

    int getRandomPriority() {
        std::uniform_int_distribution<int> dist(0, std::numeric_limits<int>::max());
        return dist(random_engine_);
    }

    std::unique_ptr<Node> rotateRight(std::unique_ptr<Node> y) {
        std::unique_ptr<Node> x = std::move(y->left);
        y->left = std::move(x->right);
        x->right = std::move(y);
        return x;
    }

    std::unique_ptr<Node> rotateLeft(std::unique_ptr<Node> x) {
        std::unique_ptr<Node> y = std::move(x->right);
        x->right = std::move(y->left);
        y->left = std::move(x);
        return y;
    }

    std::unique_ptr<Node> insertRecursive(std::unique_ptr<Node> current_node, Key key, Value value) {
        if (!current_node) {
            size_++;
            return std::make_unique<Node>(std::move(key), std::move(value), getRandomPriority());
        }

        if (compare_(key, current_node->data.first)) {
            current_node->left = insertRecursive(std::move(current_node->left), std::move(key), std::move(value));
            if (current_node->left && current_node->left->priority > current_node->priority) {
                current_node = rotateRight(std::move(current_node));
            }
        } else if (compare_(current_node->data.first, key)) {
            current_node->right = insertRecursive(std::move(current_node->right), std::move(key), std::move(value));
            if (current_node->right && current_node->right->priority > current_node->priority) {
                current_node = rotateLeft(std::move(current_node));
            }
        } else {
            current_node->data.second = std::move(value);
        }
        return current_node;
    }

    std::pair<std::unique_ptr<Node>, bool> insertRecursiveWithResult(std::unique_ptr<Node> current_node, Key key, Value value, bool& inserted_new) {
        if (!current_node) {
            inserted_new = true;
            size_++;
            return {std::make_unique<Node>(std::move(key), std::move(value), getRandomPriority()), true};
        }

        bool result_from_recursion = false;
        if (compare_(key, current_node->data.first)) {
            auto result = insertRecursiveWithResult(std::move(current_node->left), std::move(key), std::move(value), inserted_new);
            current_node->left = std::move(result.first);
            result_from_recursion = result.second;
            if (current_node->left && current_node->left->priority > current_node->priority) {
                current_node = rotateRight(std::move(current_node));
            }
        } else if (compare_(current_node->data.first, key)) {
            auto result = insertRecursiveWithResult(std::move(current_node->right), std::move(key), std::move(value), inserted_new);
            current_node->right = std::move(result.first);
            result_from_recursion = result.second;
            if (current_node->right && current_node->right->priority > current_node->priority) {
                current_node = rotateLeft(std::move(current_node));
            }
        } else {
            current_node->data.second = std::move(value);
            inserted_new = false;
            result_from_recursion = true;
        }
        return {std::move(current_node), result_from_recursion};
    }


    std::unique_ptr<Node> eraseRecursive(std::unique_ptr<Node> current_node, const Key& key, bool& erased) {
        if (!current_node) {
            erased = false;
            return nullptr;
        }

        if (compare_(key, current_node->data.first)) {
            current_node->left = eraseRecursive(std::move(current_node->left), key, erased);
        } else if (compare_(current_node->data.first, key)) {
            current_node->right = eraseRecursive(std::move(current_node->right), key, erased);
        } else {
            erased = true;
            if (!current_node->left && !current_node->right) {
                size_--;
                return nullptr;
            } else if (!current_node->left) {
                size_--;
                return std::move(current_node->right);
            } else if (!current_node->right) {
                size_--;
                return std::move(current_node->left);
            } else {
                if (current_node->left->priority > current_node->right->priority) {
                    current_node = rotateRight(std::move(current_node));
                    current_node->right = eraseRecursive(std::move(current_node->right), key, erased);
                } else {
                    current_node = rotateLeft(std::move(current_node));
                    current_node->left = eraseRecursive(std::move(current_node->left), key, erased);
                }
            }
        }
        return current_node;
    }

    Node* findRecursive(Node* current_node, const Key& key) const {
        if (!current_node) {
            return nullptr;
        }
        if (compare_(key, current_node->data.first)) {
            return findRecursive(current_node->left.get(), key);
        } else if (compare_(current_node->data.first, key)) {
            return findRecursive(current_node->right.get(), key);
        } else {
            return current_node;
        }
    }

    Node* getMinNode(Node* current_node) const {
        if (!current_node) return nullptr;
        while(current_node->left) {
            current_node = current_node->left.get();
        }
        return current_node;
    }


public:
    using iterator = TreapIterator<Key, Value, Compare>;
    using const_iterator = TreapIterator<Key, Value, Compare>;

    Treap() : compare_(Compare()) {
        std::random_device rd;
        random_engine_.seed(rd());
    }

    Treap(const Treap&) = delete;
    Treap& operator=(const Treap&) = delete;

    Treap(Treap&& other) noexcept
        : root_(std::move(other.root_)),
          compare_(std::move(other.compare_)),
          random_engine_(std::move(other.random_engine_)),
          size_(other.size_) {
        other.size_ = 0;
    }

    Treap& operator=(Treap&& other) noexcept {
        if (this != &other) {
            root_ = std::move(other.root_);
            compare_ = std::move(other.compare_);
            random_engine_ = std::move(other.random_engine_);
            size_ = other.size_;
            other.size_ = 0;
        }
        return *this;
    }

    std::pair<iterator, bool> insert(const Key& key, const Value& value) {
        bool inserted_new = false;
        Key k_copy = key;
        Value v_copy = value;
        auto result_pair = insertRecursiveWithResult(std::move(root_), std::move(k_copy), std::move(v_copy), inserted_new);
        root_ = std::move(result_pair.first);
        Node* result_node = findRecursive(root_.get(), key);
        return {iterator(result_node, this), inserted_new};
    }

    std::pair<iterator, bool> insert(Key&& key, Value&& value) {
        bool inserted_new = false;
        Key key_for_lookup = key; // Make a copy for lookup BEFORE `key` is moved from.

        auto result_node_pair = insertRecursiveWithResult(std::move(root_), std::forward<Key>(key), std::forward<Value>(value), inserted_new);
        root_ = std::move(result_node_pair.first);

        Node* found_node = findRecursive(root_.get(), key_for_lookup); // Use the pre-move copy for lookup
        return {iterator(found_node, this), inserted_new};
    }

    Value& operator[](const Key& key) {
        Node* found_node = findRecursive(root_.get(), key);
        if (found_node) {
            return found_node->data.second;
        } else {
            Value default_value{};
            Key k_copy = key;
            root_ = insertRecursive(std::move(root_), std::move(k_copy), std::move(default_value));
            found_node = findRecursive(root_.get(), key);
            return found_node->data.second;
        }
    }

    Value& operator[](Key&& key) {
        Key key_for_lookup = key; // Create a copy for lookup BEFORE `key` is moved from.
        Node* found_node = findRecursive(root_.get(), key_for_lookup);
        if (found_node) {
            return found_node->data.second;
        } else {
            Value default_value{};
            // `key` (the rvalue ref parameter) is forwarded and moved into insertRecursive.
            // `key_for_lookup` holds the original value for the subsequent find.
            root_ = insertRecursive(std::move(root_), std::forward<Key>(key), std::move(default_value));

            Node* newly_inserted_node = findRecursive(root_.get(), key_for_lookup);
            if (!newly_inserted_node) {
                 throw std::logic_error("Treap::operator[] (rvalue): Node not found after insertion.");
            }
            return newly_inserted_node->data.second;
        }
    }

    bool erase(const Key& key) {
        bool erased = false;
        root_ = eraseRecursive(std::move(root_), key, erased);
        return erased;
    }

    Value* find(const Key& key) {
        Node* result_node = findRecursive(root_.get(), key);
        return result_node ? &result_node->data.second : nullptr;
    }

    const Value* find(const Key& key) const {
        Node* result_node = findRecursive(root_.get(), key);
        return result_node ? &result_node->data.second : nullptr;
    }

    bool contains(const Key& key) const {
        return findRecursive(root_.get(), key) != nullptr;
    }

    size_t size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    void clear() {
        root_.reset();
        size_ = 0;
    }

    iterator begin() {
        Node* min_node = getMinNode(root_.get());
        return iterator(min_node, this);
    }

    const_iterator begin() const {
        Node* min_node = getMinNode(root_.get());
        return const_iterator(min_node, this);
    }

    const_iterator cbegin() const {
        Node* min_node = getMinNode(root_.get());
        return const_iterator(min_node, this);
    }

    iterator end() {
        return iterator(nullptr, this);
    }

    const_iterator end() const {
        return const_iterator(nullptr, this);
    }

    const_iterator cend() const {
         return const_iterator(nullptr, this);
    }

    friend class TreapIterator<Key, Value, Compare>;
};

#endif // TREAP_H

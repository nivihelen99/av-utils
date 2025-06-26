#ifndef CPP_UTILS_SKIPLIST_H
#define CPP_UTILS_SKIPLIST_H

#include <vector>
#include <memory> // For std::shared_ptr or std::unique_ptr if we go that route for nodes
#include <random> // For std::mt19937, std::uniform_real_distribution, std::random_device
#include <functional> // For std::less (default comparator)

namespace cpp_utils {

// Forward declaration of SkipList is not strictly necessary here anymore as SkipListNode
// doesn't need to know about SkipList's internals for its basic definition.

template <typename T>
struct SkipListNode {
    T value;
    std::vector<SkipListNode<T>*> forward_; // Pointers to next nodes at various levels

    SkipListNode(const T& val, int level) : value(val), forward_(level + 1, nullptr) {}
    SkipListNode(T&& val, int level) : value(std::move(val)), forward_(level + 1, nullptr) {}

    // Note: In a production SkipList, you might use smart pointers or careful manual memory management.
    // For this initial implementation, raw pointers are used, and the SkipList class will manage memory.
};


// Default template parameters:
// MaxLevel: Determines the max height. Max items roughly (1/P)^MaxLevel.
//           e.g., for P=0.5, MaxLevel=16 -> ~65536 items. MaxLevel=32 -> ~4 billion.
// P: Probability factor for level generation. Typically 0.5 or 0.25.
template <typename T,
          typename Compare = std::less<T>,
          int MaxLevelParam = 16, // A common default, can be tuned.
          double PParam = 0.5>      // Probability for increasing level.
class SkipList {
public:
    static constexpr int MaxLevel = MaxLevelParam;
    static constexpr double P = PParam;

private:
    using Node = SkipListNode<T>;

    Node* head_;          // Sentinel head node
    int current_level_;   // Current highest level in the skip list (0 to MaxLevel-1)
    size_t count_;        // Number of elements in the skip list
    Compare compare_;     // Comparator instance

    // For random level generation. std::mt19937 is a good general-purpose generator.
    // Needs to be mutable if randomLevel is const, or randomLevel cannot be const.
    // Or, pass generator by reference if it's a global/shared one.
    // For simplicity, each SkipList instance has its own.
    std::mt19937 random_generator_;
    std::uniform_real_distribution<double> distribution_;

    // Helper to create a new node
    Node* makeNode(const T& value, int level) {
        // Node constructor expects level index (0 to level), so forward_.size() will be level+1
        return new Node(value, level);
    }

    Node* makeNode(T&& value, int level) {
        return new Node(std::move(value), level);
    }

    // Helper to determine the level for a new node (0 to MaxLevel-1)
    int randomLevel() {
        int lvl = 0;
        // P is the probability that a node has a pointer at level i+1 given it has one at i
        // MaxLevel-1 because levels are 0-indexed. A node of level `lvl` has pointers from index 0 to `lvl`.
        while (distribution_(random_generator_) < P && lvl < MaxLevel - 1) {
            lvl++;
        }
        return lvl;
    }

public:
    // --- Constructors and Destructor ---
    SkipList() : current_level_(0), count_(0), compare_(Compare()) {
        std::random_device rd;
        random_generator_ = std::mt19937(rd());
        distribution_ = std::uniform_real_distribution<double>(0.0, 1.0);

        // Head node's value is not used for comparisons.
        // Its level is MaxLevel-1, as it's the entry point for all potential levels.
        // The forward_ vector in head_ will have size MaxLevel.
        head_ = new Node(T(), MaxLevel - 1);
    }

    ~SkipList() {
        clear();
        delete head_;
        head_ = nullptr;
    }

    // --- Basic Properties ---
    size_t size() const noexcept {
        return count_;
    }

    bool empty() const noexcept {
        return count_ == 0;
    }

    int currentListLevel() const noexcept {
        return current_level_;
    }

    // --- Modifiers ---
    void clear() noexcept {
        Node* current = head_->forward_[0];
        while (current != nullptr) {
            Node* next = current->forward_[0];
            delete current;
            current = next;
        }
        // Reset head's forward pointers to null and reset skiplist state
        for (int i = 0; i < MaxLevel; ++i) { // head_ has MaxLevel forward pointers
            head_->forward_[i] = nullptr;
        }
        current_level_ = 0;
        count_ = 0;
    }

    // --- Operations ---

    // Returns a pointer to the node if found, otherwise nullptr.
    // This is a helper that can be used by `contains` and potentially iterators.
    Node* findNode(const T& value) const {
        Node* current = head_;
        // Start from the highest level of the skip list
        for (int i = current_level_; i >= 0; --i) {
            // Traverse along the current level as long as the next node's value is less than the target value
            while (current->forward_[i] != nullptr && compare_(current->forward_[i]->value, value)) {
                current = current->forward_[i];
            }
        }
        // After the loops, current is the predecessor of the potential node at level 0.
        // Move to the next node at level 0, which is the candidate.
        current = current->forward_[0];

        // Check if the candidate node exists and its value is equal to the target value.
        // Using `!compare_(a, b) && !compare_(b, a)` for equivalence.
        if (current != nullptr && !compare_(value, current->value) && !compare_(current->value, value)) {
            return current; // Value found
        }
        return nullptr; // Value not found
    }

    bool contains(const T& value) const {
        return findNode(value) != nullptr;
    }

    // Returns true if inserted, false if value already existed.
    // We'll simplify the return type from std::pair<iterator, bool> to just bool for now.
    bool insert(const T& value) {
        std::vector<Node*> update(MaxLevel); // update[i] stores the predecessor of insertion point at level i
        Node* current = head_;

        for (int i = current_level_; i >= 0; --i) {
            while (current->forward_[i] != nullptr && compare_(current->forward_[i]->value, value)) {
                current = current->forward_[i];
            }
            update[i] = current;
        }

        Node* candidate = current->forward_[0]; // Potential existing node

        // Check for duplicates: !(a < b) && !(b < a)
        if (candidate != nullptr && !compare_(value, candidate->value) && !compare_(candidate->value, value)) {
            return false; // Value already exists
        }

        int new_node_level = randomLevel(); // Determine level for the new node (0 to MaxLevel-1)

        // If new node's level is higher than current max level of the list,
        // update the list's current_level_ and link head directly to new node at these new levels.
        if (new_node_level > current_level_) {
            for (int i = current_level_ + 1; i <= new_node_level; ++i) {
                update[i] = head_; // Predecessor at these new levels is head
            }
            current_level_ = new_node_level;
        }

        Node* new_node = makeNode(value, new_node_level);

        // Splice the new node into the skip list at all its levels
        // new_node_level is the highest index in its forward_ vector.
        for (int i = 0; i <= new_node_level; ++i) {
            // new_node points to whatever update[i] was pointing to
            new_node->forward_[i] = update[i]->forward_[i];
            // update[i] now points to new_node
            update[i]->forward_[i] = new_node;
        }

        count_++;
        return true;
    }

    bool insert(T&& value) { // R-value version
        std::vector<Node*> update(MaxLevel);
        Node* current = head_;

        for (int i = current_level_; i >= 0; --i) {
            // `value` is an rvalue reference, but its value is stable for comparisons here
            while (current->forward_[i] != nullptr && compare_(current->forward_[i]->value, value)) {
                current = current->forward_[i];
            }
            update[i] = current;
        }

        Node* candidate = current->forward_[0];

        if (candidate != nullptr && !compare_(value, candidate->value) && !compare_(candidate->value, value)) {
            return false;
        }

        int new_node_level = randomLevel();
        if (new_node_level > current_level_) {
            for (int i = current_level_ + 1; i <= new_node_level; ++i) {
                update[i] = head_;
            }
            current_level_ = new_node_level;
        }

        // Use makeNode for rvalue to move the value into the new node
        Node* new_node = makeNode(std::move(value), new_node_level);

        for (int i = 0; i <= new_node_level; ++i) {
            new_node->forward_[i] = update[i]->forward_[i];
            update[i]->forward_[i] = new_node;
        }
        count_++;
        return true;
    }

    bool erase(const T& value) {
        std::vector<Node*> update(MaxLevel, nullptr); // Stores predecessors
        Node* current = head_;

        for (int i = current_level_; i >= 0; --i) {
            while (current->forward_[i] != nullptr && compare_(current->forward_[i]->value, value)) {
                current = current->forward_[i];
            }
            update[i] = current;
        }

        Node* node_to_delete = current->forward_[0]; // Candidate for deletion

        // Check if node exists and its value is equal
        if (node_to_delete != nullptr && !compare_(value, node_to_delete->value) && !compare_(node_to_delete->value, value)) {
            // Node found, proceed with deletion
            // Unlink the node from all levels it's part of
            // node_to_delete->forward_.size() gives number of levels for this node (its level + 1)
            for (int i = 0; i < node_to_delete->forward_.size(); ++i) {
                // Only update if update[i] indeed points to node_to_delete
                // This check is crucial: update[i] must be the direct predecessor at level i
                if (update[i]->forward_[i] == node_to_delete) {
                    update[i]->forward_[i] = node_to_delete->forward_[i];
                }
            }

            delete node_to_delete;
            count_--;

            // Adjust current_level_ if the deleted node was the only one at the highest level(s)
            while (current_level_ > 0 && head_->forward_[current_level_] == nullptr) {
                current_level_--;
            }
            return true; // Successfully erased
        }
        return false; // Value not found
    }

    // TODO: Implement Copy/Move semantics
    // SkipList(const SkipList& other);
    // SkipList& operator=(const SkipList& other);
    // SkipList(SkipList&& other) noexcept;
    // SkipList& operator=(SkipList& other) noexcept;

    // TODO: Iterators
    // iterator begin();
    // iterator end();
    // const_iterator cbegin() const;
    // const_iterator cend() const;


};


} // namespace cpp_utils

#endif // CPP_UTILS_SKIPLIST_H

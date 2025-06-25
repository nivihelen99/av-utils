#pragma once

#include <vector>
#include <memory>
#include <random>
#include <limits> // For std::numeric_limits
#include <functional> // For std::less

namespace cpp_utils {

template <typename Key, typename Value, typename Compare = std::less<Key>, int MAX_LEVEL = 16>
class SkipList {
private:
    struct Node {
        Key key;
        Value value;
        std::vector<std::shared_ptr<Node>> forward; // Pointers to next nodes at each level

        Node(const Key& k, const Value& v, int level)
            : key(k), value(v), forward(level + 1, nullptr) {}

        Node(Key&& k, Value&& v, int level)
            : key(std::move(k)), value(std::move(v)), forward(level + 1, nullptr) {}
    };

    std::shared_ptr<Node> head_;
    int current_max_level_;
    size_t size_;
    Compare compare_;
    std::mt19937 random_engine_; // For determining node levels

    // Probability for a node to be promoted to the next level
    // A common value is 0.5, meaning on average, half the nodes at level i
    // also appear at level i+1.
    // Or 0.25 for sparser higher levels. Let's use 0.5 for now.
    const double PROBABILITY = 0.5;


    int randomLevel() {
        int lvl = 0;
        // std::uniform_real_distribution<double> dist(0.0, 1.0);
        // Using integer arithmetic for potentially better performance/simplicity
        // equivalent to dist(random_engine_) < PROBABILITY
        while ( (random_engine_() % 2 == 0) && lvl < MAX_LEVEL) {
            lvl++;
        }
        return lvl;
    }

public:
    SkipList() : current_max_level_(0), size_(0) {
        // Initialize random engine with a seed
        std::random_device rd;
        random_engine_.seed(rd());

        // Create a dummy head node with a sentinel key
        // Assuming Key can be default-constructed or use a specific sentinel.
        // For simplicity with templates, we might need to adjust how sentinel keys are handled.
        // Using default constructed Key and Value for head.
        // The level of the head node is MAX_LEVEL.
        head_ = std::make_shared<Node>(Key{}, Value{}, MAX_LEVEL);
    }

    ~SkipList() = default;

    SkipList(const SkipList&) = delete; // For simplicity, disable copy
    SkipList& operator=(const SkipList&) = delete; // For simplicity, disable copy assignment

    SkipList(SkipList&&) noexcept = default;
    SkipList& operator=(SkipList&&) noexcept = default;


    // --- Public API to be implemented ---

    bool insert(const Key& key, const Value& value) {
        return insertOrUpdate(key, value);
    }

    bool insert(Key&& key, Value&& value) {
        return insertOrUpdate(std::move(key), std::move(value));
    }

private:
    template<typename K, typename V>
    bool insertOrUpdate(K&& key_param, V&& value_param) {
        std::vector<std::shared_ptr<Node>> update(MAX_LEVEL + 1, nullptr);
        std::shared_ptr<Node> current = head_;

        for (int i = current_max_level_; i >= 0; --i) {
            while (current->forward[i] && compare_(current->forward[i]->key, key_param)) {
                current = current->forward[i];
            }
            update[i] = current;
        }

        // current is now the node just before the potential position of key_param at level 0
        // Move to the node that might contain key_param
        current = current->forward[0];

        // Check if key already exists
        if (current && !compare_(key_param, current->key) && !compare_(current->key, key_param)) { // equivalent to current->key == key_param
            current->value = std::forward<V>(value_param); // Update existing value
            return true; // Indicates update
        } else {
            // Key does not exist, proceed with insertion
            int new_level = randomLevel();

            if (new_level > current_max_level_) {
                for (int i = current_max_level_ + 1; i <= new_level; ++i) {
                    update[i] = head_; // New levels point back to head
                }
                current_max_level_ = new_level;
            }

            auto new_node = std::make_shared<Node>(std::forward<K>(key_param), std::forward<V>(value_param), new_level);

            for (int i = 0; i <= new_level; ++i) {
                new_node->forward[i] = update[i]->forward[i];
                update[i]->forward[i] = new_node;
            }
            size_++;
            return true; // Indicates insertion
        }
    }

public:
    // std::optional<Value> find(const Key& key) const;
    // bool contains(const Key& key) const;

    // bool erase(const Key& key);

    // size_t size() const { return size_; }
    // bool empty() const { return size_ == 0; }
    // int currentMaxLevel() const { return current_max_level_; } // For debugging/testing

    // --- Iterator support (optional, for later) ---
    // class Iterator { ... };
    // Iterator begin();
    // Iterator end();
};

} // namespace cpp_utils

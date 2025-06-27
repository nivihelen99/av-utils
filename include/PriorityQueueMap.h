#pragma once

#include <vector>
#include <unordered_map>
#include <functional> // For std::less, std::greater
#include <stdexcept>  // For std::out_of_range
#include <utility>    // For std::pair, std::move

namespace cpp_utils {

// Forward declaration for friend class in potential future extensions or specific tests
// template <typename KeyType, typename ValueType, typename PriorityType, typename Compare>
// class PriorityQueueMapTester;

template <
    typename KeyType,
    typename ValueType,
    typename PriorityType,
    typename Compare = std::greater<PriorityType> // Min-heap: smallest priority value is "top"
>
class PriorityQueueMap {
public:
    using KeyPriorityPair = std::pair<PriorityType, KeyType>;

    PriorityQueueMap() = default;

    // Modifiers
    void push(const KeyType& key, const ValueType& value, const PriorityType& priority);
    void push(KeyType&& key, ValueType&& value, PriorityType&& priority);

    PriorityType pop(); // Returns priority of the popped element. User can get value via key if needed.
                        // Consider returning std::pair<KeyType, ValueType> or just KeyType

    void update_priority(const KeyType& key, const PriorityType& new_priority);
    void remove(const KeyType& key);

    // Accessors
    bool empty() const {
        return heap_.empty();
    }

    size_t size() const {
        return heap_.size();
    }

    bool contains(const KeyType& key) const {
        return key_to_heap_index_.count(key);
    }

    // Returns the key of the top element
    const KeyType& top_key() const {
        if (empty()) {
            throw std::out_of_range("PriorityQueueMap is empty");
        }
        return heap_[0].second;
    }

    // Returns the priority of the top element
    const PriorityType& top_priority() const {
        if (empty()) {
            throw std::out_of_range("PriorityQueueMap is empty");
        }
        return heap_[0].first;
    }

    // Returns the value associated with a key
    const ValueType& get_value(const KeyType& key) const {
        auto it = value_map_.find(key);
        if (it == value_map_.end()) {
            throw std::out_of_range("Key not found in PriorityQueueMap");
        }
        return it->second;
    }

    ValueType& get_value(const KeyType& key) {
        auto it = value_map_.find(key);
        if (it == value_map_.end()) {
            throw std::out_of_range("Key not found in PriorityQueueMap");
        }
        return it->second;
    }


private:
    std::vector<KeyPriorityPair> heap_; // Stores {priority, key}
    std::unordered_map<KeyType, ValueType> value_map_;
    std::unordered_map<KeyType, size_t> key_to_heap_index_;
    Compare compare_priority_{}; // Defaults to std::greater for min-heap behavior with sift_up/down

    // Internal helper methods
    void sift_up(size_t index);
    void sift_down(size_t index);
    void swap_elements(size_t index1, size_t index2);

    size_t parent(size_t index) const { return (index - 1) / 2; }
    size_t left_child(size_t index) const { return 2 * index + 1; }
    size_t right_child(size_t index) const { return 2 * index + 2; }

    // Friend class for testing private members, if needed for complex scenarios
    // friend class PriorityQueueMapTester<KeyType, ValueType, PriorityType, Compare>;
};

// --- Implementation of Member Functions ---

template <typename K, typename V, typename P, typename C>
void PriorityQueueMap<K, V, P, C>::swap_elements(size_t index1, size_t index2) {
    if (index1 == index2) return;
    std::swap(heap_[index1], heap_[index2]);
    key_to_heap_index_[heap_[index1].second] = index1;
    key_to_heap_index_[heap_[index2].second] = index2;
}

template <typename K, typename V, typename P, typename C>
void PriorityQueueMap<K, V, P, C>::sift_up(size_t index) {
    if (index == 0) return;
    size_t p_idx = parent(index);
    // For min-heap (compare_priority_ is std::greater):
    // if heap_[index].first < heap_[p_idx].first, then compare_priority_(heap_[p_idx].first, heap_[index].first) is true
    while (index > 0 && compare_priority_(heap_[p_idx].first, heap_[index].first)) {
        swap_elements(index, p_idx);
        index = p_idx;
        if (index == 0) break;
        p_idx = parent(index);
    }
}

template <typename K, typename V, typename P, typename C>
void PriorityQueueMap<K, V, P, C>::sift_down(size_t index) {
    size_t min_index = index;
    size_t l_idx = left_child(index);
    size_t r_idx = right_child(index);

    if (l_idx < heap_.size() && compare_priority_(heap_[min_index].first, heap_[l_idx].first)) {
        min_index = l_idx;
    }
    if (r_idx < heap_.size() && compare_priority_(heap_[min_index].first, heap_[r_idx].first)) {
        min_index = r_idx;
    }

    if (index != min_index) {
        swap_elements(index, min_index);
        sift_down(min_index);
    }
}

template <typename K, typename V, typename P, typename C>
void PriorityQueueMap<K, V, P, C>::push(const K& key, const V& value, const P& priority) {
    if (contains(key)) {
        // If key exists, update its priority and value
        value_map_[key] = value; // Update value
        update_priority(key, priority);
    } else {
        // New key
        heap_.emplace_back(priority, key);
        key_to_heap_index_[key] = heap_.size() - 1;
        value_map_[key] = value;
        sift_up(heap_.size() - 1);
    }
}

template <typename K, typename V, typename P, typename C>
void PriorityQueueMap<K, V, P, C>::push(K&& key, V&& value, P&& priority) {
    // Use a temporary to check contains, as key might be moved.
    // Create a copy for contains check if key is rvalue that will be moved.
    K key_copy_for_lookup = key;
    if (contains(key_copy_for_lookup)) {
        // If key exists, update its priority and value
        value_map_[key_copy_for_lookup] = std::move(value); // Update value
        update_priority(key_copy_for_lookup, std::move(priority));
    } else {
        // New key
        // Move key and priority into the pair, then move the pair into the vector
        heap_.emplace_back(std::move(priority), std::move(key));
        // After key is moved from, its state is valid but unspecified if it was an rvalue.
        // We need to use the key from heap_.back().second for map insertion,
        // as the original 'key' parameter might have been moved from.
        key_to_heap_index_[heap_.back().second] = heap_.size() - 1;
        value_map_[heap_.back().second] = std::move(value);
        sift_up(heap_.size() - 1);
    }
}


template <typename K, typename V, typename P, typename C>
P PriorityQueueMap<K, V, P, C>::pop() {
    if (empty()) {
        throw std::out_of_range("PriorityQueueMap is empty (pop)");
    }
    auto top_element = heap_[0]; // Type is std::pair<P, K>

    // Remove from value_map and key_to_heap_index_
    value_map_.erase(top_element.second);
    key_to_heap_index_.erase(top_element.second);

    if (heap_.size() == 1) {
        heap_.pop_back();
    } else {
        heap_[0] = heap_.back(); // Move last element to root
        heap_.pop_back();
        key_to_heap_index_[heap_[0].second] = 0; // Update index of moved element
        sift_down(0);
    }
    return top_element.first; // Return priority
}

template <typename K, typename V, typename P, typename C>
void PriorityQueueMap<K, V, P, C>::update_priority(const K& key, const P& new_priority) {
    auto it = key_to_heap_index_.find(key);
    if (it == key_to_heap_index_.end()) {
        throw std::out_of_range("Key not found for priority update");
    }
    size_t index = it->second;
    P old_priority = heap_[index].first; // P is PriorityType here
    heap_[index].first = new_priority;

    // If new priority is "better" (e.g. smaller for min-heap) than old, try sifting up.
    // compare_priority_(old_priority, new_priority) will be true if new_priority < old_priority for min-heap.
    if (compare_priority_(old_priority, new_priority)) {
        sift_up(index);
    } else {
        // Otherwise (new priority is "worse" or same), try sifting down.
        // Sifting down is safe even if priority is same, just won't move.
        sift_down(index);
    }
}

template <typename K, typename V, typename P, typename C>
void PriorityQueueMap<K, V, P, C>::remove(const K& key) {
    auto it = key_to_heap_index_.find(key);
    if (it == key_to_heap_index_.end()) {
        throw std::out_of_range("Key not found for removal");
    }
    size_t index_to_remove = it->second;
    P priority_of_removed_element = heap_[index_to_remove].first; // Priority of the element being removed

    if (heap_.empty()) { // Should be caught by key_to_heap_index_.find already
        return;
    }

    // If the element to remove is the last element in the heap
    if (index_to_remove == heap_.size() - 1) {
        heap_.pop_back();
    } else {
        // Not the last element. Swap it with the last element.
        // The key of the element being moved from the end is heap_.back().second.
        // This element will replace the one at index_to_remove.
        // Our swap_elements helper correctly updates key_to_heap_index_ for both swapped parties.
        swap_elements(index_to_remove, heap_.size() - 1);

        // Now pop the (original) element (which is now at the end)
        heap_.pop_back();

        // The element that was moved from the end is now at index_to_remove.
        // We need to restore heap property for this moved element.
        // Its current priority is heap_[index_to_remove].first.
        // We compare this with the priority of the element it replaced (priority_of_removed_element)
        // to decide whether to sift up or down. This is similar to update_priority's logic.
        if (index_to_remove < heap_.size()) { // Ensure index is still valid (heap didn't become empty or too small)
            P current_priority_at_index = heap_[index_to_remove].first;
            if (compare_priority_(priority_of_removed_element, current_priority_at_index)) {
                // If the moved element's priority is "better" (e.g., smaller for min-heap)
                // than the priority of the element it replaced, it might need to go up.
                sift_up(index_to_remove);
            } else {
                // Otherwise (priority is "worse" or same), it might need to go down.
                sift_down(index_to_remove);
            }
        }
    }

    // Remove the key from tracking maps after heap operations are complete
    key_to_heap_index_.erase(key);
    value_map_.erase(key);
}

} // namespace cpp_utils

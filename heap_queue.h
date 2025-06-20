#ifndef HEAP_QUEUE_H_
#define HEAP_QUEUE_H_

#include <vector>
#include <functional>
#include <algorithm> // For std::make_heap, std::push_heap, std::pop_heap
#include <utility> // For std::move
#include <stdexcept> // For std::out_of_range
#include <type_traits> // For std::invoke_result_t

// Forward declaration if KeyFn uses T by value and T is complex
// Or ensure T is lightweight or passed by const& in KeyFn type

template <typename T,
          typename KeyFn = std::function<T(const T&)>,
          typename Compare = std::less<std::invoke_result_t<KeyFn, const T&>>>
class HeapQueue {
public:
    // Default constructor
    HeapQueue()
        : key_fn_([](const T& val) { return val; }),
          compare_instance_(),
          internal_comparator_fn_([this](const T& a, const T& b) {
              return compare_instance_(key_fn_(b), key_fn_(a));
          }) {}

    // Constructor with custom KeyFn and Compare
    HeapQueue(KeyFn key_fn, Compare compare_fn)
        : key_fn_(std::move(key_fn)),
          compare_instance_(std::move(compare_fn)),
          internal_comparator_fn_([this](const T& a, const T& b) {
              // Min-heap behavior: compare_instance_(key_for_b, key_for_a)
              // If compare_instance_ is std::less, this means key_for_b < key_for_a
              // which means 'b' has higher priority if its key is smaller.
              // For std::pop_heap, the largest element (by this comparator) comes to the front.
              // So if key_fn_(b) < key_fn_(a) is true, b is "larger" according to internal_comparator_fn_
              // and will be closer to the "largest" end of the heap (which pop_heap moves).
              // To get min-heap behavior (smallest key at the top/front after pop_heap),
              // we want the element with the smallest key to be considered the "largest" by the comparator.
              // So, if key_fn_(a) is smaller than key_fn_(b), 'a' should have higher priority.
              // The internal std::*_heap functions build a max-heap by default.
              // To simulate a min-heap, the comparator should return true if the first element
              // is "greater" than the second. So, if we want 'a' to be "smaller" than 'b'
              // (i.e. key_fn_(a) < key_fn_(b)), then compare_instance_(key_fn_(a), key_fn_(b)) should be true.
              // For pop_heap to bring the minimum element to the front, the comparator
              // needs to consider 'b' as having higher priority (being "larger") if key_fn_(b) < key_fn_(a).
              // Thus, the arguments to compare_instance_ should be key_fn_(b) and key_fn_(a).
              return compare_instance_(this->key_fn_(b), this->key_fn_(a));
          }) {}

    // Constructor with custom KeyFn only
    HeapQueue(KeyFn key_fn)
        : key_fn_(std::move(key_fn)),
          compare_instance_(),
          internal_comparator_fn_([this](const T& a, const T& b) {
              return compare_instance_(this->key_fn_(b), this->key_fn_(a));
          }) {}

    // Constructor with custom Compare only
    HeapQueue(Compare compare_fn)
        : key_fn_([](const T& val) { return val; }),
          compare_instance_(std::move(compare_fn)),
          internal_comparator_fn_([this](const T& a, const T& b) {
              return compare_instance_(this->key_fn_(b), this->key_fn_(a));
          }) {}


    void push(const T& value) {
        data_.push_back(value);
        std::push_heap(data_.begin(), data_.end(), internal_comparator_fn_);
    }

    void push(T&& value) {
        data_.push_back(std::move(value));
        std::push_heap(data_.begin(), data_.end(), internal_comparator_fn_);
    }

    T pop() {
        if (empty()) {
            throw std::out_of_range("HeapQueue is empty");
        }
        std::pop_heap(data_.begin(), data_.end(), internal_comparator_fn_);
        T val = std::move(data_.back());
        data_.pop_back();
        return val;
    }

    const T& top() const {
        if (empty()) {
            throw std::out_of_range("HeapQueue is empty");
        }
        return data_.front();
    }

    T update_top(const T& item) {
        if (empty()) {
            throw std::out_of_range("HeapQueue is empty");
        }
        T old_top = data_[0];
        data_[0] = item;
        _sift_down(0);
        return old_top;
    }

    bool empty() const {
        return data_.empty();
    }

    size_t size() const {
        return data_.size();
    }

    const std::vector<T>& as_vector() const {
        return data_;
    }

    void clear() {
        data_.clear();
    }

    void heapify(const std::vector<T>& items) {
        data_ = items; // Copy items to internal storage
        std::make_heap(data_.begin(), data_.end(), internal_comparator_fn_);
    }

    void heapify(std::vector<T>&& items) {
        data_ = std::move(items); // Move items to internal storage
        std::make_heap(data_.begin(), data_.end(), internal_comparator_fn_);
    }

private:
    std::vector<T> data_;
    KeyFn key_fn_;
    Compare compare_instance_; // Added as per instruction
    std::function<bool(const T&, const T&)> internal_comparator_fn_;

    void _sift_down(size_t index) {
        size_t parent_idx = index;
        size_t num_elements = data_.size();

        while (true) {
            size_t left_child_idx = 2 * parent_idx + 1;
            if (left_child_idx >= num_elements) { // No children, so parent_idx is a leaf
                break;
            }

            size_t child_to_swap_idx = left_child_idx; // Assume left child is the one to potentially swap with
            size_t right_child_idx = left_child_idx + 1;

            // If right child exists and is "preferred" (considered "larger" by comparator) over left child
            // internal_comparator_fn_(A, B) is true if B's key < A's key (for std::less as Compare)
            // This means B has higher priority for a min-heap.
            // So, if internal_comparator_fn_(data_[left_child_idx], data_[right_child_idx]) is true,
            // it means key(data_[right_child_idx]) < key(data_[left_child_idx]).
            // Thus, the right child has a smaller key (higher priority) and should be chosen.
            if (right_child_idx < num_elements && internal_comparator_fn_(data_[left_child_idx], data_[right_child_idx])) {
                child_to_swap_idx = right_child_idx;
            }

            // Now child_to_swap_idx is the index of the child with the higher priority (smaller key).
            // We need to check if parent_idx should be swapped with child_to_swap_idx.
            // Swap if parent is "less preferred" (considered "smaller" by comparator) than child_to_swap_idx.
            // This means swap if internal_comparator_fn_(data_[parent_idx], data_[child_to_swap_idx]) is true.
            // internal_comparator_fn_(parent, child) means key(child) < key(parent).
            // If key(child) < key(parent), then parent is not in the right place, so swap.
            if (internal_comparator_fn_(data_[parent_idx], data_[child_to_swap_idx])) {
                std::swap(data_[parent_idx], data_[child_to_swap_idx]);
                parent_idx = child_to_swap_idx; // Move down to the child's position
            } else {
                // Parent has higher or equal priority (key <= child's key), so heap property is satisfied locally.
                break;
            }
        }
    }
};

#endif // HEAP_QUEUE_H_

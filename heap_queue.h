#ifndef HEAP_QUEUE_H_
#define HEAP_QUEUE_H_

#include <vector>
#include <functional>
#include <algorithm> // For std::make_heap, std::push_heap, std::pop_heap
#include <utility> // For std::move

// Forward declaration if KeyFn uses T by value and T is complex
// Or ensure T is lightweight or passed by const& in KeyFn type

template <typename T,
          typename Compare = std::less<typename std::result_of<std::function<T(const T&)>(const T&)>::type>,
          typename KeyFn = std::function<T(const T&)>>
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
    HeapQueue(KeyFn key_fn, Compare compare_fn = Compare())
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

    bool empty() const {
        return data_.empty();
    }

    size_t size() const {
        return data_.size();
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
};

#endif // HEAP_QUEUE_H_

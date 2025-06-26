#ifndef DEQUE_H
#define DEQUE_H

#include <vector>
#include <stdexcept>
#include <algorithm>
#include <initializer_list>

namespace ankerl { // Assuming the namespace used in other files

template <typename T>
class Deque {
public:
    // Typedefs for iterators (to be defined later)
    class iterator;
    class const_iterator;

    // Constructors and Destructor
    Deque();
    explicit Deque(size_t count, const T& value);
    Deque(std::initializer_list<T> init);
    Deque(const Deque& other);
    Deque(Deque&& other) noexcept;
    ~Deque() = default;

    // Assignment operators
    Deque& operator=(const Deque& other);
    Deque& operator=(Deque&& other) noexcept;

    // Element access
    T& front();
    const T& front() const;
    T& back();
    const T& back() const;
    T& operator[](size_t index);
    const T& operator[](size_t index) const;
    T& at(size_t index);
    const T& at(size_t index) const;

    // Modifiers
    void push_front(const T& value);
    void push_front(T&& value);
    void push_back(const T& value);
    void push_back(T&& value);
    void pop_front();
    void pop_back();

    // Capacity
    bool empty() const noexcept;
    size_t size() const noexcept;
    void clear() noexcept;

    // Iterators (to be implemented)
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;

private:
    std::vector<T> data_;
    size_t head_; // Index of the first element
    size_t tail_; // Index of the position AFTER the last element
    size_t size_; // Number of elements currently in the deque

    // Helper to convert logical index to physical index in data_
    size_t physical_index(size_t logical_idx) const;
    void resize_if_needed_for_push();
    // No resize_if_needed_for_pop, as we might want to keep capacity
    // Or implement a shrink_to_fit later if desired.

    static constexpr size_t DEFAULT_CAPACITY = 8; // Default initial capacity

    // Helper to reallocate and copy elements to a new underlying vector
    void reallocate(size_t new_capacity);
};


// Implementation of element access methods

template<typename T>
T& Deque<T>::front() {
    if (empty()) {
        throw std::out_of_range("Deque::front(): deque is empty");
    }
    return data_[head_];
}

template<typename T>
const T& Deque<T>::front() const {
    if (empty()) {
        throw std::out_of_range("Deque::front(): deque is empty");
    }
    return data_[head_];
}

template<typename T>
T& Deque<T>::back() {
    if (empty()) {
        throw std::out_of_range("Deque::back(): deque is empty");
    }
    // tail_ points one past the last element.
    // If head_ is 0 and tail_ is 0 (empty), this is bad.
    // If head_ is 0 and tail_ is 1 (size 1), data_[0] is correct (tail_ - 1).
    // If head_ is N-1, tail_ is 0 (wrapped around, size 1), data_[N-1] is correct. (tail_ - 1 + capacity) % capacity
    // Physical index of last element is (tail_ + capacity_ - 1) % capacity_
    return data_[(tail_ == 0 ? data_.capacity() : tail_) - 1];
}

template<typename T>
const T& Deque<T>::back() const {
    if (empty()) {
        throw std::out_of_range("Deque::back(): deque is empty");
    }
    return data_[(tail_ == 0 ? data_.capacity() : tail_) - 1];
}

template<typename T>
T& Deque<T>::operator[](size_t index) {
    // No bounds checking for operator[] as per std::vector behavior
    return data_[physical_index(index)];
}

template<typename T>
const T& Deque<T>::operator[](size_t index) const {
    // No bounds checking for operator[]
    return data_[physical_index(index)];
}

template<typename T>
T& Deque<T>::at(size_t index) {
    if (index >= size_) {
        throw std::out_of_range("Deque::at(): index out of range");
    }
    return data_[physical_index(index)];
}

template<typename T>
const T& Deque<T>::at(size_t index) const {
    if (index >= size_) {
        throw std::out_of_range("Deque::at(): index out of range");
    }
    return data_[physical_index(index)];
}


// Implementation of modification methods

template<typename T>
void Deque<T>::push_front(const T& value) {
    resize_if_needed_for_push();
    if (data_.capacity() == 0) { // Should be handled by resize_if_needed
        reallocate(DEFAULT_CAPACITY);
    }
    head_ = (head_ == 0) ? data_.capacity() - 1 : head_ - 1;
    data_[head_] = value;
    size_++;
}

template<typename T>
void Deque<T>::push_front(T&& value) {
    resize_if_needed_for_push();
    if (data_.capacity() == 0) {
        reallocate(DEFAULT_CAPACITY);
    }
    head_ = (head_ == 0) ? data_.capacity() - 1 : head_ - 1;
    data_[head_] = std::move(value);
    size_++;
}

template<typename T>
void Deque<T>::push_back(const T& value) {
    resize_if_needed_for_push();
    if (data_.capacity() == 0) {
        reallocate(DEFAULT_CAPACITY);
    }
    data_[tail_] = value;
    tail_ = (tail_ + 1) % data_.capacity();
    size_++;
}

template<typename T>
void Deque<T>::push_back(T&& value) {
    resize_if_needed_for_push();
    if (data_.capacity() == 0) {
        reallocate(DEFAULT_CAPACITY);
    }
    data_[tail_] = std::move(value);
    tail_ = (tail_ + 1) % data_.capacity();
    size_++;
}

template<typename T>
void Deque<T>::pop_front() {
    if (empty()) {
        throw std::out_of_range("Deque::pop_front(): deque is empty");
    }
    // Optional: Call destructor for T if it's non-trivial
    // data_[head_].~T(); // If manual destruction is needed
    head_ = (head_ + 1) % data_.capacity();
    size_--;
    // Optional: Add shrink logic if size becomes much smaller than capacity
}

template<typename T>
void Deque<T>::pop_back() {
    if (empty()) {
        throw std::out_of_range("Deque::pop_back(): deque is empty");
    }
    tail_ = (tail_ == 0) ? data_.capacity() - 1 : tail_ - 1;
    // Optional: Call destructor for T if it's non-trivial
    // data_[tail_].~T(); // If manual destruction is needed
    size_--;
    // Optional: Add shrink logic
}


// Implementation of constructors and basic operations

template <typename T>
Deque<T>::Deque() : head_(0), tail_(0), size_(0) {
    data_.resize(DEFAULT_CAPACITY);
}

template <typename T>
Deque<T>::Deque(size_t count, const T& value) : head_(0), size_(count) {
    if (count == 0) {
        data_.resize(DEFAULT_CAPACITY);
        tail_ = 0;
    } else {
        data_.resize(std::max(DEFAULT_CAPACITY, count));
        std::fill_n(data_.begin(), count, value);
        tail_ = count % data_.capacity();
    }
}

template <typename T>
Deque<T>::Deque(std::initializer_list<T> init) : head_(0), size_(init.size()) {
    if (size_ == 0) {
        data_.resize(DEFAULT_CAPACITY);
        tail_ = 0;
    } else {
        data_.resize(std::max(DEFAULT_CAPACITY, size_));
        size_t current = 0;
        for (const T& item : init) {
            data_[current++] = item;
        }
        tail_ = size_ % data_.capacity();
    }
}

template <typename T>
Deque<T>::Deque(const Deque& other)
    : data_(other.data_), head_(other.head_), tail_(other.tail_), size_(other.size_) {}

template <typename T>
Deque<T>::Deque(Deque&& other) noexcept
    : data_(std::move(other.data_)),
      head_(other.head_),
      tail_(other.tail_),
      size_(other.size_) {
    other.head_ = 0;
    other.tail_ = 0;
    other.size_ = 0;
}

template <typename T>
Deque<T>& Deque<T>::operator=(const Deque& other) {
    if (this != &other) {
        data_ = other.data_;
        head_ = other.head_;
        tail_ = other.tail_;
        size_ = other.size_;
    }
    return *this;
}

template <typename T>
Deque<T>& Deque<T>::operator=(Deque&& other) noexcept {
    if (this != &other) {
        data_ = std::move(other.data_);
        head_ = other.head_;
        tail_ = other.tail_;
        size_ = other.size_;

        other.head_ = 0;
        other.tail_ = 0;
        other.size_ = 0;
    }
    return *this;
}

template <typename T>
bool Deque<T>::empty() const noexcept {
    return size_ == 0;
}

template <typename T>
size_t Deque<T>::size() const noexcept {
    return size_;
}

template <typename T>
void Deque<T>::clear() noexcept {
    // For non-trivial types, we might want to call destructors
    // but vector assignment or resize(0) followed by resize(capacity) handles this.
    // For simplicity and to keep capacity, just reset indices and size.
    // If elements are PoD, this is fine. If they have complex destructors,
    // a loop to destroy them might be needed if not using vector's clearing mechanisms.
    // However, since push/pop will overwrite or they will be out of logical range,
    // this should be okay.
    head_ = 0;
    tail_ = 0;
    size_ = 0;
    // data_ retains its capacity. If shrink-to-fit is desired:
    // std::vector<T>().swap(data_); data_.resize(DEFAULT_CAPACITY); head_ = tail_ = size_ = 0;
}


// Helper implementations
template<typename T>
size_t Deque<T>::physical_index(size_t logical_idx) const {
    if (data_.capacity() == 0) return 0; // Should not happen if constructed properly
    return (head_ + logical_idx) % data_.capacity();
}

template<typename T>
void Deque<T>::reallocate(size_t new_capacity) {
    std::vector<T> new_data(new_capacity);
    for (size_t i = 0; i < size_; ++i) {
        new_data[i] = data_[physical_index(i)];
    }
    data_ = std::move(new_data);
    head_ = 0;
    tail_ = size_; // tail_ is size_ when head_ is 0 and capacity >= size_
}


template<typename T>
void Deque<T>::resize_if_needed_for_push() {
    if (data_.capacity() == 0) { // Initial allocation if default constructed and then pushed
         reallocate(DEFAULT_CAPACITY);
    } else if (size_ == data_.capacity()) {
        reallocate(data_.capacity() * 2);
    }
}


// Iterator class definitions

template <typename T>
class Deque<T>::iterator {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

private:
    Deque<T>* deque_ptr_;
    size_t current_logical_idx_; // Logical index from the start of the deque

public:
    iterator(Deque<T>* deq_ptr, size_t logical_idx)
        : deque_ptr_(deq_ptr), current_logical_idx_(logical_idx) {}

    reference operator*() const {
        return (*deque_ptr_)[current_logical_idx_];
    }

    pointer operator->() const {
        return &((*deque_ptr_)[current_logical_idx_]);
    }

    iterator& operator++() { // Pre-increment
        current_logical_idx_++;
        return *this;
    }

    iterator operator++(int) { // Post-increment
        iterator temp = *this;
        ++(*this);
        return temp;
    }

    iterator& operator--() { // Pre-decrement
        current_logical_idx_--;
        return *this;
    }

    iterator operator--(int) { // Post-decrement
        iterator temp = *this;
        --(*this);
        return temp;
    }

    iterator& operator+=(difference_type n) {
        current_logical_idx_ += n;
        return *this;
    }

    iterator operator+(difference_type n) const {
        iterator temp = *this;
        return temp += n;
    }

    friend iterator operator+(difference_type n, const iterator& it) {
        return it + n;
    }

    iterator& operator-=(difference_type n) {
        current_logical_idx_ -= n;
        return *this;
    }

    iterator operator-(difference_type n) const {
        iterator temp = *this;
        return temp -= n;
    }

    difference_type operator-(const iterator& other) const {
        return static_cast<difference_type>(current_logical_idx_) - static_cast<difference_type>(other.current_logical_idx_);
    }

    reference operator[](difference_type n) const {
        return (*deque_ptr_)[current_logical_idx_ + n];
    }

    bool operator==(const iterator& other) const {
        return deque_ptr_ == other.deque_ptr_ && current_logical_idx_ == other.current_logical_idx_;
    }

    bool operator!=(const iterator& other) const {
        return !(*this == other);
    }

    bool operator<(const iterator& other) const {
        return current_logical_idx_ < other.current_logical_idx_;
    }
    bool operator>(const iterator& other) const {
        return current_logical_idx_ > other.current_logical_idx_;
    }
    bool operator<=(const iterator& other) const {
        return current_logical_idx_ <= other.current_logical_idx_;
    }
    bool operator>=(const iterator& other) const {
        return current_logical_idx_ >= other.current_logical_idx_;
    }

    // Allow conversion to const_iterator
    operator const_iterator() const {
        return const_iterator(deque_ptr_, current_logical_idx_);
    }
};

template <typename T>
class Deque<T>::const_iterator {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;

private:
    const Deque<T>* deque_ptr_;
    size_t current_logical_idx_;

public:
    const_iterator(const Deque<T>* deq_ptr, size_t logical_idx)
        : deque_ptr_(deq_ptr), current_logical_idx_(logical_idx) {}

    reference operator*() const {
        return (*deque_ptr_)[current_logical_idx_];
    }

    pointer operator->() const {
        return &((*deque_ptr_)[current_logical_idx_]);
    }

    const_iterator& operator++() {
        current_logical_idx_++;
        return *this;
    }

    const_iterator operator++(int) {
        const_iterator temp = *this;
        ++(*this);
        return temp;
    }

    const_iterator& operator--() {
        current_logical_idx_--;
        return *this;
    }

    const_iterator operator--(int) {
        const_iterator temp = *this;
        --(*this);
        return temp;
    }

    const_iterator& operator+=(difference_type n) {
        current_logical_idx_ += n;
        return *this;
    }

    const_iterator operator+(difference_type n) const {
        const_iterator temp = *this;
        return temp += n;
    }

    friend const_iterator operator+(difference_type n, const const_iterator& it) {
        return it + n;
    }

    const_iterator& operator-=(difference_type n) {
        current_logical_idx_ -= n;
        return *this;
    }

    const_iterator operator-(difference_type n) const {
        const_iterator temp = *this;
        return temp -= n;
    }

    difference_type operator-(const const_iterator& other) const {
         return static_cast<difference_type>(current_logical_idx_) - static_cast<difference_type>(other.current_logical_idx_);
    }

    reference operator[](difference_type n) const {
        return (*deque_ptr_)[current_logical_idx_ + n];
    }

    bool operator==(const const_iterator& other) const {
        return deque_ptr_ == other.deque_ptr_ && current_logical_idx_ == other.current_logical_idx_;
    }

    bool operator!=(const const_iterator& other) const {
        return !(*this == other);
    }

    bool operator<(const const_iterator& other) const {
        return current_logical_idx_ < other.current_logical_idx_;
    }
    bool operator>(const const_iterator& other) const {
        return current_logical_idx_ > other.current_logical_idx_;
    }
    bool operator<=(const const_iterator& other) const {
        return current_logical_idx_ <= other.current_logical_idx_;
    }
    bool operator>=(const const_iterator& other) const {
        return current_logical_idx_ >= other.current_logical_idx_;
    }
};

// Deque methods for iterators
template<typename T>
typename Deque<T>::iterator Deque<T>::begin() {
    return iterator(this, 0);
}

template<typename T>
typename Deque<T>::iterator Deque<T>::end() {
    return iterator(this, size_); // One past the last element
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::begin() const {
    return const_iterator(this, 0);
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::end() const {
    return const_iterator(this, size_);
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::cbegin() const {
    return const_iterator(this, 0);
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::cend() const {
    return const_iterator(this, size_);
}


} // namespace ankerl

#endif // DEQUE_H

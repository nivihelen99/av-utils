#pragma once

#include <vector>
#include <stdexcept>
#include <iterator>
#include <algorithm>
#include <type_traits>

template<typename T, typename Allocator = std::allocator<T>>
class CircularBuffer {
public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

private:
    using vector_type = std::vector<T, Allocator>;
    vector_type data_;
    size_type head_ = 0;
    size_type tail_ = 0;
    size_type count_ = 0;
    size_type capacity_ = 0;

public:
    explicit CircularBuffer(size_type capacity, const Allocator& alloc = Allocator())
        : data_(alloc), capacity_(capacity) {
        if (capacity == 0) {
            throw std::invalid_argument("CircularBuffer capacity must be positive.");
        }
        data_.resize(capacity_);
    }

    // Capacity
    size_type size() const noexcept { return count_; }
    size_type capacity() const noexcept { return capacity_; }
    bool empty() const noexcept { return count_ == 0; }
    bool full() const noexcept { return count_ == capacity_; }

    void push_back(const_reference item) {
        if (full()) {
            // Overwrite the oldest element
            head_ = (head_ + 1) % capacity_;
        } else {
            count_++;
        }
        data_[tail_] = item;
        tail_ = (tail_ + 1) % capacity_;
    }

    void push_front(const_reference item) {
        head_ = (head_ + capacity_ - 1) % capacity_;
        data_[head_] = item;
        if (full()) {
            tail_ = (tail_ + capacity_ - 1) % capacity_;
        } else {
            count_++;
        }
    }

    void pop_front() {
        if (empty()) {
            throw std::runtime_error("pop_front() on empty CircularBuffer");
        }
        head_ = (head_ + 1) % capacity_;
        count_--;
    }

    void pop_back() {
        if (empty()) {
            throw std::runtime_error("pop_back() on empty CircularBuffer");
        }
        tail_ = (tail_ + capacity_ - 1) % capacity_;
        count_--;
    }

    reference front() {
        if (empty()) {
            throw std::runtime_error("front() on empty CircularBuffer");
        }
        return data_[head_];
    }

    const_reference front() const {
        if (empty()) {
            throw std::runtime_error("front() on empty CircularBuffer");
        }
        return data_[head_];
    }

    reference back() {
        if (empty()) {
            throw std::runtime_error("back() on empty CircularBuffer");
        }
        return data_[(tail_ + capacity_ - 1) % capacity_];
    }

    const_reference back() const {
        if (empty()) {
            throw std::runtime_error("back() on empty CircularBuffer");
        }
        return data_[(tail_ + capacity_ - 1) % capacity_];
    }

    reference operator[](size_type index) {
        if (index >= count_) {
            throw std::out_of_range("Index out of range");
        }
        return data_[(head_ + index) % capacity_];
    }

    const_reference operator[](size_type index) const {
        if (index >= count_) {
            throw std::out_of_range("Index out of range");
        }
        return data_[(head_ + index) % capacity_];
    }

    void rotate(difference_type n) {
        if (empty()) {
            return;
        }
        if (n > 0) {
            head_ = (head_ + n) % capacity_;
            tail_ = (tail_ + n) % capacity_;
        } else {
            head_ = (head_ + capacity_ - ((-n) % capacity_)) % capacity_;
            tail_ = (tail_ + capacity_ - ((-n) % capacity_)) % capacity_;
        }
    }

    void clear() noexcept {
        head_ = 0;
        tail_ = 0;
        count_ = 0;
    }

    // Iterator implementation
    template <bool IsConst>
    class base_iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = CircularBuffer::value_type;
        using difference_type = CircularBuffer::difference_type;
        using pointer = std::conditional_t<IsConst, CircularBuffer::const_pointer, CircularBuffer::pointer>;
        using reference = std::conditional_t<IsConst, CircularBuffer::const_reference, CircularBuffer::reference>;

    private:
        using parent_type = std::conditional_t<IsConst, const CircularBuffer*, CircularBuffer*>;
        parent_type buf_;
        size_type index_;

    public:
        base_iterator(parent_type buf, size_type index) : buf_(buf), index_(index) {}
        base_iterator() : buf_(nullptr), index_(0) {}

        // Allow conversion from non-const to const iterator
        operator base_iterator<true>() const {
            return base_iterator<true>(buf_, index_);
        }

        reference operator*() const {
            return (*buf_)[index_];
        }

        pointer operator->() const {
            return &(*buf_)[index_];
        }

        base_iterator& operator++() {
            ++index_;
            return *this;
        }

        base_iterator operator++(int) {
            base_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        base_iterator& operator--() {
            --index_;
            return *this;
        }

        base_iterator operator--(int) {
            base_iterator tmp = *this;
            --(*this);
            return tmp;
        }

        base_iterator& operator+=(difference_type n) {
            index_ += n;
            return *this;
        }

        base_iterator operator+(difference_type n) const {
            base_iterator tmp = *this;
            return tmp += n;
        }

        base_iterator& operator-=(difference_type n) {
            index_ -= n;
            return *this;
        }

        base_iterator operator-(difference_type n) const {
            base_iterator tmp = *this;
            return tmp -= n;
        }

        difference_type operator-(const base_iterator& other) const {
            return index_ - other.index_;
        }

        reference operator[](difference_type n) const {
            return *(*this + n);
        }

        bool operator==(const base_iterator& other) const {
            return buf_ == other.buf_ && index_ == other.index_;
        }

        bool operator!=(const base_iterator& other) const {
            return !(*this == other);
        }

        bool operator<(const base_iterator& other) const {
            return index_ < other.index_;
        }

        bool operator>(const base_iterator& other) const {
            return index_ > other.index_;
        }

        bool operator<=(const base_iterator& other) const {
            return index_ <= other.index_;
        }

        bool operator>=(const base_iterator& other) const {
            return index_ >= other.index_;
        }
    };

    using iterator = base_iterator<false>;
    using const_iterator = base_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, count_); }
    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end() const { return const_iterator(this, count_); }
    const_iterator cbegin() const { return const_iterator(this, 0); }
    const_iterator cend() const { return const_iterator(this, count_); }

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
    const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }
};

template <typename T, typename Alloc>
void swap(CircularBuffer<T, Alloc>& lhs, CircularBuffer<T, Alloc>& rhs) noexcept {
    lhs.swap(rhs);
}

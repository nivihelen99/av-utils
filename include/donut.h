#pragma once

#include <vector>
#include <iterator>

namespace cpp_utils {

template <typename T>
class Donut {
public:
    explicit Donut(size_t capacity) : capacity_(capacity), data_(capacity) {}

    void push(T item) {
        data_[head_] = std::move(item);
        head_ = (head_ + 1) % capacity_;
        if (size_ < capacity_) {
            size_++;
        }
    }

    const T& operator[](size_t index) const {
        return data_[(head_ + index) % capacity_];
    }

    T& operator[](size_t index) {
        return data_[(head_ + index) % capacity_];
    }

    size_t size() const {
        return size_;
    }

    size_t capacity() const {
        return capacity_;
    }

    class Iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        Iterator(Donut& donut, size_t index) : donut_(donut), index_(index) {}

        reference operator*() const {
            return donut_[index_];
        }

        pointer operator->() const {
            return &donut_[index_];
        }

        Iterator& operator++() {
            index_++;
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        Iterator& operator--() {
            index_--;
            return *this;
        }

        Iterator operator--(int) {
            Iterator tmp = *this;
            --(*this);
            return tmp;
        }

        Iterator& operator+=(difference_type n) {
            index_ += n;
            return *this;
        }

        Iterator& operator-=(difference_type n) {
            index_ -= n;
            return *this;
        }

        friend bool operator==(const Iterator& a, const Iterator& b) {
            return a.index_ == b.index_;
        }

        friend bool operator!=(const Iterator& a, const Iterator& b) {
            return a.index_ != b.index_;
        }

        friend bool operator<(const Iterator& a, const Iterator& b) {
            return a.index_ < b.index_;
        }

        friend bool operator>(const Iterator& a, const Iterator& b) {
            return a.index_ > b.index_;
        }

        friend bool operator<=(const Iterator& a, const Iterator& b) {
            return a.index_ <= b.index_;
        }

        friend bool operator>=(const Iterator& a, const Iterator& b) {
            return a.index_ >= b.index_;
        }

        friend difference_type operator-(const Iterator& a, const Iterator& b) {
            return a.index_ - b.index_;
        }

        friend Iterator operator+(Iterator a, difference_type n) {
            a += n;
            return a;
        }

        friend Iterator operator+(difference_type n, Iterator a) {
            a += n;
            return a;
        }

        friend Iterator operator-(Iterator a, difference_type n) {
            a -= n;
            return a;
        }

    private:
        Donut& donut_;
        size_t index_;
    };

    Iterator begin() {
        return Iterator(*this, 0);
    }

    Iterator end() {
        return Iterator(*this, size_);
    }

private:
    size_t capacity_;
    std::vector<T> data_;
    size_t head_ = 0;
    size_t size_ = 0;
};

}

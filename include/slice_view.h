#pragma once

#include <iterator>
#include <type_traits>

namespace slice_util {

template<typename T>
class SliceView {
private:
    T* data_;
    size_t size_;
    int step_;
    
public:
    using value_type = std::remove_cv_t<T>;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    
    class iterator {
    private:
        T* ptr_;
        int step_;
        
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = std::remove_cv_t<T>;
        using difference_type = ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        
        iterator(T* ptr, int step) : ptr_(ptr), step_(step) {}
        
        reference operator*() const { return *ptr_; }
        pointer operator->() const { return ptr_; }
        
        iterator& operator++() { ptr_ += step_; return *this; }
        iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }
        iterator& operator--() { ptr_ -= step_; return *this; }
        iterator operator--(int) { iterator tmp = *this; --(*this); return tmp; }
        
        iterator& operator+=(difference_type n) { ptr_ += n * step_; return *this; }
        iterator& operator-=(difference_type n) { ptr_ -= n * step_; return *this; }
        
        iterator operator+(difference_type n) const { return iterator(ptr_ + n * step_, step_); }
        iterator operator-(difference_type n) const { return iterator(ptr_ - n * step_, step_); }
        
        difference_type operator-(const iterator& other) const { 
            return (ptr_ - other.ptr_) / step_; 
        }
        
        reference operator[](difference_type n) const { return *(ptr_ + n * step_); }
        
        bool operator==(const iterator& other) const { return ptr_ == other.ptr_; }
        bool operator!=(const iterator& other) const { return ptr_ != other.ptr_; }
        bool operator<(const iterator& other) const { return ptr_ < other.ptr_; }
        bool operator<=(const iterator& other) const { return ptr_ <= other.ptr_; }
        bool operator>(const iterator& other) const { return ptr_ > other.ptr_; }
        bool operator>=(const iterator& other) const { return ptr_ >= other.ptr_; }
    };
    
    using const_iterator = iterator; // For simplicity, assuming const correctness is handled by T type
    
    SliceView() : data_(nullptr), size_(0), step_(1) {}
    
    SliceView(T* data, size_t size, int step = 1) 
        : data_(data), size_(size), step_(step) {}
    
    iterator begin() const { return iterator(data_, step_); }
    iterator end() const { 
        if (step_ > 0) {
            return iterator(data_ + size_ * step_, step_);
        } else {
            return iterator(data_ + size_ * step_, step_);
        }
    }
    
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }
    
    size_type size() const { return size_; }
    bool empty() const { return size_ == 0; }
    
    reference operator[](size_type idx) const {
        return data_[idx * step_];
    }
    
    reference front() const { return *data_; }
    reference back() const { return data_[(size_ - 1) * step_]; }
    
    T* data() const { return data_; }
};

// Helper function to normalize negative indices
inline int normalize_index(int idx, size_t size) {
    if (idx < 0) {
        idx += static_cast<int>(size);
    }
    return std::max(0, std::min(idx, static_cast<int>(size)));
}

// Helper function to get container data pointer
template<typename Container>
auto get_data_ptr(Container& c) -> decltype(c.data()) {
    return c.data();
}

template<typename Container>
auto get_data_ptr(const Container& c) -> decltype(c.data()) {
    return c.data();
}

// Main slice function with start, stop, step
template<typename Container>
auto slice(Container& c, int start, int stop, int step = 1) {
    using value_type = typename Container::value_type;
    
    const size_t container_size = c.size();
    
    // Normalize indices
    start = normalize_index(start, container_size);
    stop = normalize_index(stop, container_size);
    
    auto* data_ptr = get_data_ptr(c);
    
    if (step > 0) {
        if (start >= stop) {
            return SliceView<value_type>(data_ptr, 0, step);
        }
        size_t slice_size = (stop - start + step - 1) / step; // ceiling division
        return SliceView<value_type>(data_ptr + start, slice_size, step);
    } else if (step < 0) {
        if (start <= stop) {
            return SliceView<value_type>(data_ptr, 0, step);
        }
        size_t slice_size = (start - stop - step - 1) / (-step); // ceiling division
        return SliceView<value_type>(data_ptr + start, slice_size, step);
    } else {
        // step == 0 is invalid, return empty slice
        return SliceView<value_type>(data_ptr, 0, 1);
    }
}

// Const version
template<typename Container>
auto slice(const Container& c, int start, int stop, int step = 1) {
    using value_type = const typename Container::value_type;
    
    const size_t container_size = c.size();
    
    // Normalize indices
    start = normalize_index(start, container_size);
    stop = normalize_index(stop, container_size);
    
    auto* data_ptr = get_data_ptr(c);
    
    if (step > 0) {
        if (start >= stop) {
            return SliceView<value_type>(data_ptr, 0, step);
        }
        size_t slice_size = (stop - start + step - 1) / step;
        return SliceView<value_type>(data_ptr + start, slice_size, step);
    } else if (step < 0) {
        if (start <= stop) {
            return SliceView<value_type>(data_ptr, 0, step);
        }
        size_t slice_size = (start - stop - step - 1) / (-step);
        return SliceView<value_type>(data_ptr + start, slice_size, step);
    } else {
        return SliceView<value_type>(data_ptr, 0, 1);
    }
}

// Slice from start to end
template<typename Container>
auto slice(Container& c, int start) {
    return slice(c, start, static_cast<int>(c.size()));
}

template<typename Container>
auto slice(const Container& c, int start) {
    return slice(c, start, static_cast<int>(c.size()));
}

// Full slice (returns full view)
template<typename Container>
auto slice(Container& c) {
    return slice(c, 0, static_cast<int>(c.size()));
}

template<typename Container>
auto slice(const Container& c) {
    return slice(c, 0, static_cast<int>(c.size()));
}

} // namespace slice_util

// Bring into global namespace for convenience
using slice_util::slice;

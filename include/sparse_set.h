#pragma once

#include <vector>
#include <memory> // For std::allocator
#include <stdexcept> // For std::out_of_range, std::invalid_argument
#include <limits>   // For std::numeric_limits
#include <iterator> // For std::iterator tags
#include <type_traits> // For std::is_signed_v

namespace cpp_collections {

// Forward declaration for iterator
template <typename T, typename IndexT, typename AllocatorT, bool IsConst>
class SparseSetIterator;

template <
    typename T_cls = uint32_t,
    typename IndexT_cls = uint32_t,
    typename AllocatorT_cls = std::allocator<T_cls>,
    typename AllocatorIndexT_cls = typename std::allocator_traits<AllocatorT_cls>::template rebind_alloc<IndexT_cls>
>
class SparseSet {
public:
    using value_type = T_cls;
    using allocator_type_T = AllocatorT_cls;
    using allocator_type_IndexT = AllocatorIndexT_cls;
    using size_type = IndexT_cls;
    using difference_type = std::ptrdiff_t;
    using reference = T_cls&;
    using const_reference = const T_cls&;
    using pointer = T_cls*;
    using const_pointer = const T_cls*;

    using iterator = SparseSetIterator<T_cls, IndexT_cls, AllocatorT_cls, false>;
    using const_iterator = SparseSetIterator<T_cls, IndexT_cls, AllocatorT_cls, true>;

    // --- Construction and Destruction ---

    explicit SparseSet(
        size_type max_value,
        size_type initial_dense_capacity = 0,
        const AllocatorT_cls& alloc_t = AllocatorT_cls(),
        const AllocatorIndexT_cls& alloc_idx_t = AllocatorIndexT_cls()
    );

    SparseSet(const SparseSet& other) = default;
    SparseSet(SparseSet&& other) noexcept = default;
    SparseSet& operator=(const SparseSet& other) = default;
    SparseSet& operator=(SparseSet&& other) noexcept = default;
    ~SparseSet() = default;

    std::pair<iterator, bool> insert(const_reference value);
    bool erase(const_reference value);
    void clear() noexcept;
    void swap(SparseSet& other) noexcept;

    bool contains(const_reference value) const noexcept;
    iterator find(const_reference value);
    const_iterator find(const_reference value) const;

    bool empty() const noexcept;
    size_type size() const noexcept;
    size_type max_value_capacity() const noexcept;
    size_type dense_capacity() const noexcept;
    void reserve_dense(size_type new_cap);

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

    bool operator==(const SparseSet& other) const;
    bool operator!=(const SparseSet& other) const;

private:
    friend class SparseSetIterator<T_cls, IndexT_cls, AllocatorT_cls, false>;
    friend class SparseSetIterator<T_cls, IndexT_cls, AllocatorT_cls, true>;

    std::vector<T_cls, AllocatorT_cls> dense_;
    std::vector<IndexT_cls, AllocatorIndexT_cls> sparse_;
    size_type count_;
};

template <typename T, typename IndexT, typename AllocatorT_iter, bool IsConst> // Iterator uses AllocatorT_iter for consistency if needed
class SparseSetIterator {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T; // T from SparseSet's T_cls
    using difference_type = std::ptrdiff_t;
    using pointer_type = std::conditional_t<IsConst, const T*, T*>;
    using reference_type = std::conditional_t<IsConst, const T&, T&>;

public:
    pointer_type ptr_ = nullptr;

public:
    SparseSetIterator() = default;
    explicit SparseSetIterator(pointer_type p) : ptr_(p) {}

    template <bool OtherIsConst = IsConst, typename = std::enable_if_t<IsConst && !OtherIsConst>>
    SparseSetIterator(const SparseSetIterator<T, IndexT, AllocatorT_iter, false>& other)
        : ptr_(other.ptr_) {}

    reference_type operator*() const { return *ptr_; }
    pointer_type operator->() const { return ptr_; }
    reference_type operator[](difference_type n) const { return ptr_[n]; }

    SparseSetIterator& operator++() { ++ptr_; return *this; }
    SparseSetIterator operator++(int) { SparseSetIterator tmp = *this; ++ptr_; return tmp; }
    SparseSetIterator& operator--() { --ptr_; return *this; }
    SparseSetIterator operator--(int) { SparseSetIterator tmp = *this; --ptr_; return tmp; }

    SparseSetIterator& operator+=(difference_type n) { ptr_ += n; return *this; }
    SparseSetIterator& operator-=(difference_type n) { ptr_ -= n; return *this; }

    friend SparseSetIterator operator+(SparseSetIterator lhs, difference_type n) { lhs += n; return lhs; }
    friend SparseSetIterator operator+(difference_type n, SparseSetIterator rhs) { rhs += n; return rhs; }
    friend SparseSetIterator operator-(SparseSetIterator lhs, difference_type n) { lhs -= n; return lhs; }
    friend difference_type operator-(const SparseSetIterator& lhs, const SparseSetIterator& rhs) { return lhs.ptr_ - rhs.ptr_; }

    friend bool operator==(const SparseSetIterator& a, const SparseSetIterator& b) { return a.ptr_ == b.ptr_; }
    friend bool operator!=(const SparseSetIterator& a, const SparseSetIterator& b) { return a.ptr_ != b.ptr_; }
    friend bool operator<(const SparseSetIterator& a, const SparseSetIterator& b) { return a.ptr_ < b.ptr_; }
    friend bool operator>(const SparseSetIterator& a, const SparseSetIterator& b) { return a.ptr_ > b.ptr_; }
    friend bool operator<=(const SparseSetIterator& a, const SparseSetIterator& b) { return a.ptr_ <= b.ptr_; }
    friend bool operator>=(const SparseSetIterator& a, const SparseSetIterator& b) { return a.ptr_ >= b.ptr_; }
};

// Method Definitions
template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::SparseSet(
    typename SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::size_type max_val_cap,
    typename SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::size_type initial_dense_cap,
    const AllocatorT_cls& alloc_t,
    const AllocatorIndexT_cls& alloc_idx_t)
    : dense_(alloc_t), sparse_(alloc_idx_t), count_(0) {
    sparse_.resize(max_val_cap);
    if (initial_dense_cap > 0) {
        dense_.reserve(initial_dense_cap);
    }
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
std::pair<typename SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::iterator, bool>
SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::insert(const_reference value) {
    size_type val_as_idx = static_cast<size_type>(value);

    if (val_as_idx >= sparse_.size() || (std::is_signed_v<T_cls> && value < 0) ) {
        return {end(), false};
    }
    IndexT_cls& sparse_slot_for_value = sparse_[val_as_idx];
    if (sparse_slot_for_value < count_ && dense_[sparse_slot_for_value] == value) {
        return {iterator(&dense_[sparse_slot_for_value]), false};
    }
    dense_.push_back(value);
    sparse_slot_for_value = count_;
    iterator it_to_inserted = iterator(&dense_[count_]);
    count_++;
    return {it_to_inserted, true};
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
bool SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::erase(const_reference value) {
    size_type val_as_idx = static_cast<size_type>(value);
    if (val_as_idx >= sparse_.size() || (std::is_signed_v<T_cls> && value < 0)) {
        return false;
    }
    IndexT_cls& potential_idx_in_dense = sparse_[val_as_idx];
    if (!(potential_idx_in_dense < count_ && dense_[potential_idx_in_dense] == value)) {
        return false;
    }
    T_cls last_element_in_dense = dense_.back();
    dense_[potential_idx_in_dense] = last_element_in_dense;
    sparse_[static_cast<size_type>(last_element_in_dense)] = potential_idx_in_dense;
    dense_.pop_back();
    count_--;
    return true;
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
void SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::clear() noexcept {
    dense_.clear();
    count_ = 0;
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
void SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::swap(SparseSet& other) noexcept {
    using std::swap;
    swap(dense_, other.dense_);
    swap(sparse_, other.sparse_);
    swap(count_, other.count_);
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
bool SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::contains(const_reference value) const noexcept {
    size_type val_as_idx = static_cast<size_type>(value);
    if (val_as_idx >= sparse_.size() || (std::is_signed_v<T_cls> && value < 0)) {
        return false;
    }
    IndexT_cls index_in_dense = sparse_[val_as_idx];
    return index_in_dense < count_ && dense_[index_in_dense] == value;
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
typename SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::iterator
SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::find(const_reference value) {
    size_type val_as_idx = static_cast<size_type>(value);
    if (val_as_idx < sparse_.size() && !(std::is_signed_v<T_cls> && value < 0)) {
        IndexT_cls index_in_dense = sparse_[val_as_idx];
        if (index_in_dense < count_ && dense_[index_in_dense] == value) {
            return iterator(&dense_[index_in_dense]);
        }
    }
    return end();
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
typename SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::const_iterator
SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::find(const_reference value) const {
    size_type val_as_idx = static_cast<size_type>(value);
     if (val_as_idx < sparse_.size() && !(std::is_signed_v<T_cls> && value < 0)) {
        IndexT_cls index_in_dense = sparse_[val_as_idx];
        if (index_in_dense < count_ && dense_[index_in_dense] == value) {
            return const_iterator(&dense_[index_in_dense]);
        }
    }
    return cend();
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
bool SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::empty() const noexcept {
    return count_ == 0;
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
typename SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::size_type
SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::size() const noexcept {
    return count_;
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
typename SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::size_type
SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::max_value_capacity() const noexcept {
    return static_cast<size_type>(sparse_.size());
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
typename SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::size_type
SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::dense_capacity() const noexcept {
    return static_cast<size_type>(dense_.capacity());
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
void SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::reserve_dense(size_type new_cap) {
    dense_.reserve(new_cap);
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
typename SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::iterator
SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::begin() noexcept {
    return iterator(dense_.data());
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
typename SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::iterator
SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::end() noexcept {
    return iterator(dense_.data() + count_);
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
typename SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::const_iterator
SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::begin() const noexcept {
    return const_iterator(dense_.data());
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
typename SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::const_iterator
SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::end() const noexcept {
    return const_iterator(dense_.data() + count_);
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
typename SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::const_iterator
SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::cbegin() const noexcept {
    return begin();
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
typename SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::const_iterator
SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::cend() const noexcept {
    return end();
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
bool SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::operator==(const SparseSet& other) const {
    if (count_ != other.count_) {
        return false;
    }
    for (const_iterator it = cbegin(); it != cend(); ++it) {
        if (!other.contains(*it)) {
            return false;
        }
    }
    return true;
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
bool SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>::operator!=(const SparseSet& other) const {
    return !(*this == other);
}

template <typename T_cls, typename IndexT_cls, typename AllocatorT_cls, typename AllocatorIndexT_cls>
void swap(SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>& lhs,
          SparseSet<T_cls, IndexT_cls, AllocatorT_cls, AllocatorIndexT_cls>& rhs) noexcept {
    lhs.swap(rhs);
}

} // namespace cpp_collections

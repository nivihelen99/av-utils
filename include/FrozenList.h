#ifndef FROZEN_LIST_H
#define FROZEN_LIST_H

#include <vector>
#include <utility>      // For std::move
#include <algorithm>    // For std::equal, std::lexicographical_compare
#include <stdexcept>    // For std::out_of_range
#include <functional>   // For std::hash
#include <memory>       // For std::allocator, std::allocator_traits
#include <initializer_list> // For std::initializer_list
#include <iterator>     // For std::reverse_iterator

namespace cpp_collections {

// Forward declaration for std::hash specialization
template <typename T, typename Allocator>
class FrozenList;

} // namespace cpp_collections

namespace std {
// Forward declaration of std::hash specialization for FrozenList
template <typename T, typename Allocator>
struct hash<cpp_collections::FrozenList<T, Allocator>>;
} // namespace std


namespace cpp_collections {

template <
    typename T,
    typename Allocator = std::allocator<T>
>
class FrozenList {
public:
    // Types
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using reference = const value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::const_pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

private:
    // Internal storage
    using internal_storage_type = std::vector<T, Allocator>;
    internal_storage_type data_;

    template<typename InputIt>
    void build_from_range(InputIt first, InputIt last) {
        data_.assign(first, last);
        data_.shrink_to_fit(); // Minimize capacity as it's frozen
    }

public:
    // Iterators (const only, as FrozenList is immutable)
    using iterator = typename internal_storage_type::const_iterator;
    using const_iterator = typename internal_storage_type::const_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // Constructors
    FrozenList(const Allocator& alloc = Allocator())
        : data_(alloc) {}

    FrozenList(size_type count, const T& value, const Allocator& alloc = Allocator())
        : data_(count, value, alloc) {
        data_.shrink_to_fit();
    }

    template <typename InputIt, typename = typename std::enable_if<!std::is_integral<InputIt>::value>::type>
    FrozenList(InputIt first, InputIt last, const Allocator& alloc = Allocator())
        : data_(alloc) {
        build_from_range(first, last);
    }

    FrozenList(std::initializer_list<T> ilist, const Allocator& alloc = Allocator())
        : data_(alloc) {
        build_from_range(ilist.begin(), ilist.end());
    }

    // Copy constructor
    FrozenList(const FrozenList& other) = default;
    FrozenList(const FrozenList& other, const Allocator& alloc)
        : data_(other.data_, alloc) {
        // shrink_to_fit is implicitly handled by vector's constructor from another vector
        // if allocators are different. If same, it copies capacity.
        // Since it's frozen, ensure it's shrunk.
        data_.shrink_to_fit();
    }


    // Move constructor
    FrozenList(FrozenList&& other) noexcept = default;
    FrozenList(FrozenList&& other, const Allocator& alloc) noexcept
        : data_(std::move(other.data_), alloc) {
        // shrink_to_fit might not be strictly necessary if other was already shrunk,
        // but doesn't hurt to ensure.
        // However, vector move constructor with allocator might reallocate.
        // If it reallocates, it will likely be exact size.
        // If it doesn't reallocate (e.g. allocator propagates), then it takes over other's buffer.
        // Assuming 'other' was properly shrunk.
    }

    // Destructor
    ~FrozenList() = default;

    // Assignment operators (deleted, as the list is frozen)
    // FrozenList& operator=(const FrozenList& other) = delete;
    // FrozenList& operator=(FrozenList&& other) noexcept = delete;
    // FrozenList& operator=(std::initializer_list<T>) = delete;
    // Note: Standard practice for immutable objects is to make them copyable/movable
    // but not assignable after construction. Or, make assignment return a new object.
    // For this, we'll keep them assignable to allow patterns like:
    // FrozenList<int> list; list = FrozenList<int>{1,2,3};
    // This uses move assignment.

    FrozenList& operator=(const FrozenList& other) {
        if (this == &other) {
            return *this;
        }
        FrozenList temp(other); // Copy
        swap(temp);             // Swap
        return *this;
    }

    FrozenList& operator=(FrozenList&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        // Use vector's move assignment, which is allocator-aware.
        data_ = std::move(other.data_);
        // key_comparator_ and key_hasher_ are not part of FrozenList
        return *this;
    }
     FrozenList& operator=(std::initializer_list<value_type> ilist) {
        FrozenList temp(ilist, get_allocator());
        swap(temp);
        return *this;
    }


    // Iterators
    iterator begin() const noexcept { return data_.begin(); }
    const_iterator cbegin() const noexcept { return data_.cbegin(); }
    iterator end() const noexcept { return data_.end(); }
    const_iterator cend() const noexcept { return data_.cend(); }

    reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

    // Capacity
    bool empty() const noexcept { return data_.empty(); }
    size_type size() const noexcept { return data_.size(); }
    size_type max_size() const noexcept { return data_.max_size(); }
    // capacity() is not provided as it should ideally be same as size() after shrink_to_fit

    // Element access
    const_reference at(size_type pos) const {
        if (pos >= size()) {
            throw std::out_of_range("FrozenList::at: position out of range");
        }
        return data_[pos];
    }

    const_reference operator[](size_type pos) const {
        return data_[pos];
    }

    const_reference front() const {
        // UB if empty, like std::vector. Consider adding check if desired.
        return data_.front();
    }

    const_reference back() const {
        // UB if empty, like std::vector. Consider adding check if desired.
        return data_.back();
    }

    const T* data() const noexcept {
        return data_.data();
    }

    // Allocator
    allocator_type get_allocator() const noexcept { return data_.get_allocator(); }

    // Comparison operators
    friend bool operator==(const FrozenList& lhs, const FrozenList& rhs) {
        return lhs.data_ == rhs.data_; // std::vector::operator== handles this
    }

    friend bool operator!=(const FrozenList& lhs, const FrozenList& rhs) {
        return !(lhs == rhs);
    }

    friend bool operator<(const FrozenList& lhs, const FrozenList& rhs) {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    friend bool operator<=(const FrozenList& lhs, const FrozenList& rhs) {
        return !(rhs < lhs);
    }

    friend bool operator>(const FrozenList& lhs, const FrozenList& rhs) {
        return rhs < lhs;
    }

    friend bool operator>=(const FrozenList& lhs, const FrozenList& rhs) {
        return !(lhs < rhs);
    }

    // Swap (member swap)
    void swap(FrozenList& other) noexcept(
        std::allocator_traits<Allocator>::propagate_on_container_swap::value ||
        std::allocator_traits<Allocator>::is_always_equal::value) {
        using std::swap;
        swap(data_, other.data_);
    }

    // Friend declaration for std::hash specialization
    friend struct std::hash<FrozenList<T, Allocator>>;
};

// Non-member swap
template <typename T, typename A>
void swap(FrozenList<T, A>& lhs, FrozenList<T, A>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace cpp_collections

// Specialization of std::hash for FrozenList
namespace std {
template <typename T, typename Allocator>
struct hash<cpp_collections::FrozenList<T, Allocator>> {
    std::size_t operator()(const cpp_collections::FrozenList<T, Allocator>& fl) const {
        std::hash<T> value_hasher;
        std::size_t seed = fl.size(); // Start seed with size

        // Access internal data_ for hashing. This requires FrozenList to be a friend
        // or provide a const accessor to its data or begin/end iterators.
        // Since std::hash is already friended with the forward declaration,
        // it can access private members if defined within the same TU or if FrozenList allows it.
        // For now, using public iterators is fine.
        for (const auto& elem : fl) { // Uses FrozenList's public iterators
            seed ^= value_hasher(elem) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};
} // namespace std

// Deduction guides (C++17)
namespace cpp_collections {
template<typename InputIt, typename Alloc = std::allocator<typename std::iterator_traits<InputIt>::value_type>>
FrozenList(InputIt, InputIt, Alloc = Alloc())
    -> FrozenList<typename std::iterator_traits<InputIt>::value_type, Alloc>;

template<typename T, typename Alloc = std::allocator<T>>
FrozenList(std::initializer_list<T>, Alloc = Alloc())
    -> FrozenList<T, Alloc>;

// Deduction guide for (size_type, const T&, Allocator)
// Changed to be more explicit about the first argument's type to avoid ambiguity with iterator constructor.
template<typename T_elem, typename Alloc = std::allocator<T_elem>>
FrozenList(std::size_t /*count*/,
           const T_elem& /*value*/,
           Alloc /*alloc*/ = Alloc())
    -> FrozenList<T_elem, Alloc>;

} // namespace cpp_collections


#endif // FROZEN_LIST_H

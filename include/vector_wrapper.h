#ifndef VECTOR_WRAPPER_H
#define VECTOR_WRAPPER_H

#include <vector>
#include <utility>      // For std::move, std::forward
#include <initializer_list>
#include <stdexcept>    // For std::out_of_range

template <typename T, typename InnerContainer = std::vector<T>>
class VectorWrapper {
public:
    using value_type = typename InnerContainer::value_type;
    using allocator_type = typename InnerContainer::allocator_type;
    using size_type = typename InnerContainer::size_type;
    using difference_type = typename InnerContainer::difference_type;
    using reference = typename InnerContainer::reference;
    using const_reference = typename InnerContainer::const_reference;
    using pointer = typename InnerContainer::pointer;
    using const_pointer = typename InnerContainer::const_pointer;
    using iterator = typename InnerContainer::iterator;
    using const_iterator = typename InnerContainer::const_iterator;
    using reverse_iterator = typename InnerContainer::reverse_iterator;
    using const_reverse_iterator = typename InnerContainer::const_reverse_iterator;

protected:
    InnerContainer data;

public:
    // Constructors
    VectorWrapper() = default;
    explicit VectorWrapper(const InnerContainer& d) : data(d) {}
    explicit VectorWrapper(InnerContainer&& d) : data(std::move(d)) {}
    VectorWrapper(size_type count, const T& value) : data(count, value) {}
    explicit VectorWrapper(size_type count) : data(count) {} // C++11 std::vector constructor
    template <typename InputIt>
    VectorWrapper(InputIt first, InputIt last) : data(first, last) {}
    VectorWrapper(std::initializer_list<T> ilist) : data(ilist) {}

    // Copy constructor
    VectorWrapper(const VectorWrapper& other) : data(other.data) {}

    // Move constructor
    VectorWrapper(VectorWrapper&& other) noexcept : data(std::move(other.data)) {}

    // Assignment operators
    VectorWrapper& operator=(const VectorWrapper& other) {
        if (this != &other) {
            data = other.data;
        }
        return *this;
    }

    VectorWrapper& operator=(VectorWrapper&& other) noexcept {
        if (this != &other) {
            data = std::move(other.data);
        }
        return *this;
    }

    VectorWrapper& operator=(std::initializer_list<T> ilist) {
        data = ilist;
        return *this;
    }

    virtual void assign(size_type count, const T& value) {
        data.assign(count, value);
    }

    template <typename InputIt>
    void assign(InputIt first, InputIt last) { // Removed virtual
        data.assign(first, last);
    }

    virtual void assign(std::initializer_list<T> ilist) {
        data.assign(ilist);
    }

    virtual allocator_type get_allocator() const noexcept {
        return data.get_allocator();
    }

    // Element access
    virtual reference at(size_type pos) {
        return data.at(pos);
    }

    virtual const_reference at(size_type pos) const {
        return data.at(pos);
    }

    virtual reference operator[](size_type pos) {
        return data[pos];
    }

    virtual const_reference operator[](size_type pos) const {
        return data[pos];
    }

    virtual reference front() {
        return data.front();
    }

    virtual const_reference front() const {
        return data.front();
    }

    virtual reference back() {
        return data.back();
    }

    virtual const_reference back() const {
        return data.back();
    }

    virtual T* data_ptr() noexcept { // Renamed from 'data' to avoid conflict with member name
        return data.data();
    }

    virtual const T* data_ptr() const noexcept { // Renamed from 'data'
        return data.data();
    }

    // Iterators
    virtual iterator begin() noexcept {
        return data.begin();
    }

    virtual const_iterator begin() const noexcept {
        return data.begin();
    }

    virtual const_iterator cbegin() const noexcept {
        return data.cbegin();
    }

    virtual iterator end() noexcept {
        return data.end();
    }

    virtual const_iterator end() const noexcept {
        return data.end();
    }

    virtual const_iterator cend() const noexcept {
        return data.cend();
    }

    virtual reverse_iterator rbegin() noexcept {
        return data.rbegin();
    }

    virtual const_reverse_iterator rbegin() const noexcept {
        return data.rbegin();
    }

    virtual const_reverse_iterator crbegin() const noexcept {
        return data.crbegin();
    }

    virtual reverse_iterator rend() noexcept {
        return data.rend();
    }

    virtual const_reverse_iterator rend() const noexcept {
        return data.rend();
    }

    virtual const_reverse_iterator crend() const noexcept {
        return data.crend();
    }

    // Capacity
    virtual bool empty() const noexcept {
        return data.empty();
    }

    virtual size_type size() const noexcept {
        return data.size();
    }

    virtual size_type max_size() const noexcept {
        return data.max_size();
    }

    virtual void reserve(size_type new_cap) {
        data.reserve(new_cap);
    }

    virtual size_type capacity() const noexcept {
        return data.capacity();
    }

    virtual void shrink_to_fit() {
        data.shrink_to_fit();
    }

    // Modifiers
    virtual void clear() noexcept {
        data.clear();
    }

    virtual iterator insert(const_iterator pos, const T& value) {
        return data.insert(pos, value);
    }

    virtual iterator insert(const_iterator pos, T&& value) {
        return data.insert(pos, std::move(value));
    }

    virtual iterator insert(const_iterator pos, size_type count, const T& value) {
        return data.insert(pos, count, value);
    }

    template <typename InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last) { // Removed virtual
        return data.insert(pos, first, last);
    }

    virtual iterator insert(const_iterator pos, std::initializer_list<T> ilist) {
        return data.insert(pos, ilist);
    }

    template <typename... Args>
    iterator emplace(const_iterator pos, Args&&... args) { // Removed virtual
        return data.emplace(pos, std::forward<Args>(args)...);
    }

    template <typename... Args>
    reference emplace_back(Args&&... args) { // Removed virtual
        return data.emplace_back(std::forward<Args>(args)...);
    }

    virtual iterator erase(const_iterator pos) {
        return data.erase(pos);
    }

    virtual iterator erase(const_iterator first, const_iterator last) {
        return data.erase(first, last);
    }

    virtual void push_back(const T& value) {
        data.push_back(value);
    }

    virtual void push_back(T&& value) {
        data.push_back(std::move(value));
    }

    virtual void pop_back() {
        data.pop_back();
    }

    virtual void resize(size_type count) {
        data.resize(count);
    }

    virtual void resize(size_type count, const value_type& value) {
        data.resize(count, value);
    }

    virtual void swap(VectorWrapper& other) noexcept {
        data.swap(other.data);
    }

protected:
    InnerContainer& get_data() { return data; }
    const InnerContainer& get_data() const { return data; }
};

// Non-member functions
template <typename T, typename InnerContainer>
void swap(VectorWrapper<T, InnerContainer>& lhs, VectorWrapper<T, InnerContainer>& rhs) noexcept {
    lhs.swap(rhs);
}

template <typename T, typename InnerContainer>
bool operator==(const VectorWrapper<T, InnerContainer>& lhs, const VectorWrapper<T, InnerContainer>& rhs) {
    // Access protected data for comparison.
    // This relies on the InnerContainer supporting operator==. std::vector does.
    // Similar to DictWrapper, this would require friendship or a protected getter if 'data' were private.
    // Since 'data' is protected, direct access is not possible here.
    // A simple way is to compare element by element if direct access to 'data' for comparison is not feasible.
    // However, std::vector comparison is efficient.
    // Let's assume the InnerContainer itself has operator==.
    // The current get_data() is protected, so we can't call it from a free function without friendship.
    // For simplicity, iterate and compare or make VectorWrapper a friend.
    // A common approach:
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (typename VectorWrapper<T, InnerContainer>::size_type i = 0; i < lhs.size(); ++i) {
        if (!(lhs[i] == rhs[i])) { // Relies on T having operator==
            return false;
        }
    }
    return true;
}

template <typename T, typename InnerContainer>
bool operator!=(const VectorWrapper<T, InnerContainer>& lhs, const VectorWrapper<T, InnerContainer>& rhs) {
    return !(lhs == rhs);
}

// C++20 spaceship operator (optional, could be added if repo uses C++20)
/*
#if __cplusplus >= 202002L
template <typename T, typename InnerContainer>
auto operator<=>(const VectorWrapper<T, InnerContainer>& lhs, const VectorWrapper<T, InnerContainer>& rhs) {
    // Similar to operator==, this would ideally delegate to InnerContainer's <=>
    // For now, a lexicographical comparison:
    return std::lexicographical_compare_three_way(
        lhs.begin(), lhs.end(),
        rhs.begin(), rhs.end()
    );
}
#endif
*/

#endif // VECTOR_WRAPPER_H

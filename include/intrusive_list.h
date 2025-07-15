#pragma once

#include <cstddef>
#include <iterator>

namespace cpp_utils {

class intrusive_list_hook {
public:
    intrusive_list_hook() : prev_(this), next_(this) {}

    bool is_linked() const { return next_ != this; }

private:
    template <typename T, intrusive_list_hook T::*Hook>
    friend class intrusive_list;

    intrusive_list_hook* prev_;
    intrusive_list_hook* next_;
};

template <typename T, intrusive_list_hook T::*Hook>
class intrusive_list {
    using hook_ptr = intrusive_list_hook*;
    using const_hook_ptr = const intrusive_list_hook*;

    static T* to_value(hook_ptr hook) {
        return (T*)((char*)hook - ((char*)&(((T*)0)->*Hook) - (char*)0));
    }

    static const T* to_value(const_hook_ptr hook) {
        return (const T*)((const char*)hook - ((const char*)&(((const T*)0)->*Hook) - (const char*)0));
    }

public:
    class iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        iterator(hook_ptr hook) : hook_(hook) {}

        reference operator*() const { return *to_value(hook_); }
        pointer operator->() const { return to_value(hook_); }

        iterator& operator++() {
            hook_ = hook_->next_;
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        iterator& operator--() {
            hook_ = hook_->prev_;
            return *this;
        }

        iterator operator--(int) {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }

        bool operator==(const iterator& other) const { return hook_ == other.hook_; }
        bool operator!=(const iterator& other) const { return hook_ != other.hook_; }

    private:
        friend class intrusive_list;
        hook_ptr hook_;
    };

    class const_iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = const T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        const_iterator(const_hook_ptr hook) : hook_(hook) {}

        reference operator*() const { return *to_value(hook_); }
        pointer operator->() const { return to_value(hook_); }

        const_iterator& operator++() {
            hook_ = hook_->next_;
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        const_iterator& operator--() {
            hook_ = hook_->prev_;
            return *this;
        }

        const_iterator operator--(int) {
            const_iterator tmp = *this;
            --(*this);
            return tmp;
        }

        bool operator==(const const_iterator& other) const { return hook_ == other.hook_; }
        bool operator!=(const const_iterator& other) const { return hook_ != other.hook_; }

    private:
        friend class intrusive_list;
        const_hook_ptr hook_;
    };

    intrusive_list() : head_() {}

    ~intrusive_list() { clear(); }

    intrusive_list(const intrusive_list&) = delete;
    intrusive_list& operator=(const intrusive_list&) = delete;

    intrusive_list(intrusive_list&& other) noexcept : head_() {
        if (!other.empty()) {
            head_.next_ = other.head_.next_;
            head_.prev_ = other.head_.prev_;
            other.head_.next_->prev_ = &head_;
            other.head_.prev_->next_ = &head_;
            other.head_.next_ = &other.head_;
            other.head_.prev_ = &other.head_;
        }
    }

    intrusive_list& operator=(intrusive_list&& other) noexcept {
        if (this != &other) {
            clear();
            if (!other.empty()) {
                head_.next_ = other.head_.next_;
                head_.prev_ = other.head_.prev_;
                other.head_.next_->prev_ = &head_;
                other.head_.prev_->next_ = &head_;
                other.head_.next_ = &other.head_;
                other.head_.prev_ = &other.head_;
            }
        }
        return *this;
    }

    iterator begin() { return iterator(head_.next_); }
    const_iterator begin() const { return const_iterator(head_.next_); }
    const_iterator cbegin() const { return const_iterator(head_.next_); }

    iterator end() { return iterator(&head_); }
    const_iterator end() const { return const_iterator(&head_); }
    const_iterator cend() const { return const_iterator(&head_); }

    T& front() { return *to_value(head_.next_); }
    const T& front() const { return *to_value(head_.next_); }

    T& back() { return *to_value(head_.prev_); }
    const T& back() const { return *to_value(head_.prev_); }

    bool empty() const { return head_.next_ == &head_; }

    size_t size() const {
        size_t count = 0;
        for (auto it = cbegin(); it != cend(); ++it) {
            ++count;
        }
        return count;
    }

    void push_front(T& value) {
        insert(begin(), value);
    }

    void push_back(T& value) {
        insert(end(), value);
    }

    void pop_front() {
        erase(begin());
    }

    void pop_back() {
        erase(--end());
    }

    iterator insert(iterator pos, T& value) {
        hook_ptr hook = &(value.*Hook);
        hook_ptr next = pos.hook_;
        hook_ptr prev = next->prev_;

        hook->prev_ = prev;
        hook->next_ = next;
        prev->next_ = hook;
        next->prev_ = hook;

        return iterator(hook);
    }

    iterator erase(iterator pos) {
        hook_ptr hook = pos.hook_;
        hook_ptr next = hook->next_;
        hook_ptr prev = hook->prev_;

        prev->next_ = next;
        next->prev_ = prev;

        hook->prev_ = hook;
        hook->next_ = hook;

        return iterator(next);
    }

    void clear() {
        while (!empty()) {
            pop_front();
        }
    }

private:
    intrusive_list_hook head_;
};

} // namespace cpp_utils

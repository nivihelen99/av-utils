#pragma once

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new> // For std::launder
#include <stdexcept>
#include <type_traits>
#include <utility> // For std::move, std::forward

// Forward declaration for a helper
namespace detail {
template <typename T_detail, typename Alloc_detail>
void destroy_range_helper(T_detail* first, T_detail* last, Alloc_detail& alloc);
}

template <typename T, size_t N_this_param, typename Allocator = std::allocator<T>>
class small_vector {
public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

    using iterator = T*;
    using const_iterator = const T*;

    static constexpr size_type N_this = N_this_param; // Make N_this available as a member

private:
    static_assert(N_this > 0, "small_vector inline capacity N_this must be greater than 0.");

    using AllocTraits = std::allocator_traits<Allocator>;

    union Storage {
        std::aligned_storage_t<sizeof(T) * N_this, alignof(T)> inline_buffer;
        pointer heap_ptr;

        Storage() {}
        ~Storage() {}
    } storage_;

    size_type size_ = 0;
    size_type capacity_ = N_this;
    [[no_unique_address]] Allocator alloc_;

    bool is_inline_runtime() const noexcept {
        return capacity_ == N_this;
    }

    pointer get_inline_ptr_raw() noexcept {
        return std::launder(reinterpret_cast<pointer>(&storage_.inline_buffer));
    }

    const_pointer get_inline_ptr_raw() const noexcept {
        return std::launder(reinterpret_cast<const_pointer>(&storage_.inline_buffer));
    }

    pointer current_data() noexcept {
        return (capacity_ == N_this) ? get_inline_ptr_raw() : storage_.heap_ptr;
    }

    const_pointer current_data() const noexcept {
        return (capacity_ == N_this) ? get_inline_ptr_raw() : storage_.heap_ptr;
    }

    void set_heap_data(pointer ptr, size_type new_capacity) noexcept {
        storage_.heap_ptr = ptr;
        capacity_ = new_capacity;
    }

    void set_inline_active() noexcept {
        capacity_ = N_this;
    }

    // For move operations from a different N_other small_vector
    pointer steal_heap_data_for_move() {
        return storage_.heap_ptr; // Simply return, ownership transferred by logic
    }

    void clear_after_move() {
        size_ = 0;
        set_inline_active();
    }

public:
    // Constructors
    small_vector() noexcept(std::is_nothrow_default_constructible_v<Allocator>)
        : size_(0), capacity_(N_this), alloc_(Allocator()) {}

    explicit small_vector(const Allocator& alloc) noexcept
        : size_(0), capacity_(N_this), alloc_(alloc) {}

    small_vector(size_type count, const_reference value, const Allocator& alloc = Allocator())
        : alloc_(alloc) {
        if (count > N_this) {
            allocate_and_set_heap(count);
        } else {
            set_inline_active();
        }
        construct_fill(current_data(), count, value);
        size_ = count;
    }

    explicit small_vector(size_type count, const Allocator& alloc = Allocator())
        : alloc_(alloc) {
        static_assert(std::is_default_constructible_v<T>, "T must be default constructible for this constructor.");
        if (count > N_this) {
            allocate_and_set_heap(count);
        } else {
            set_inline_active();
        }
        construct_default(current_data(), count);
        size_ = count;
    }

    template <typename InputIt, typename = std::enable_if_t<std::is_base_of_v<std::input_iterator_tag, typename std::iterator_traits<InputIt>::iterator_category>>>
    small_vector(InputIt first, InputIt last, const Allocator& alloc = Allocator())
        : alloc_(alloc) {
        size_type count = static_cast<size_type>(std::distance(first, last));
        if (count > N_this) {
            allocate_and_set_heap(count);
        } else {
            set_inline_active();
        }
        construct_range(current_data(), first, last);
        size_ = count;
    }

    template<size_t N_other>
    small_vector(const small_vector<T, N_other, Allocator>& other)
        : alloc_(AllocTraits::select_on_container_copy_construction(other.get_allocator())) {
        if (other.size() > N_this) {
            allocate_and_set_heap(other.size());
        } else {
            set_inline_active();
        }
        construct_range(current_data(), other.begin(), other.end());
        size_ = other.size();
    }

    small_vector(const small_vector<T, N_this, Allocator>& other) // Non-template for same N
        : alloc_(AllocTraits::select_on_container_copy_construction(other.alloc_)) {
        if (other.size_ > N_this) {
            allocate_and_set_heap(other.size_);
        } else {
            set_inline_active();
        }
        construct_range(current_data(), other.begin(), other.end());
        size_ = other.size_;
    }

    template<size_t N_other>
    small_vector(const small_vector<T, N_other, Allocator>& other, const Allocator& alloc)
        : alloc_(alloc) {
        if (other.size() > N_this) {
            allocate_and_set_heap(other.size());
        } else {
            set_inline_active();
        }
        construct_range(current_data(), other.begin(), other.end());
        size_ = other.size();
    }
     small_vector(const small_vector<T, N_this, Allocator>& other, const Allocator& alloc) // Non-template for same N
        : alloc_(alloc) {
        if (other.size_ > N_this) {
            allocate_and_set_heap(other.size_);
        } else {
            set_inline_active();
        }
        construct_range(current_data(), other.begin(), other.end());
        size_ = other.size_;
    }

    template<size_t N_other>
    small_vector(small_vector<T, N_other, Allocator>&& other)
        noexcept(std::is_nothrow_move_constructible_v<Allocator> && std::is_nothrow_move_constructible_v<T>)
        : alloc_(std::move(other.get_allocator())) {
        if (other.capacity() == N_other) { // other is using its inline buffer
            if (other.size() > N_this) { // Will it fit our inline?
                allocate_and_set_heap(other.size());
            } else {
                set_inline_active();
            }
            if (other.size() > 0) {
                std::uninitialized_move_n(other.data(), other.size(), current_data());
            }
            size_ = other.size();
        } else { // Other is on heap.
            set_heap_data(other.steal_heap_data_for_move(), other.capacity());
            size_ = other.size();
        }
        other.clear_after_move();
    }

    small_vector(small_vector<T, N_this, Allocator>&& other) // Non-template for same N
        noexcept(std::is_nothrow_move_constructible_v<Allocator> && std::is_nothrow_move_constructible_v<T>)
        : alloc_(std::move(other.alloc_)) {
        if (other.is_inline_runtime()) {
            set_inline_active();
            if (other.size_ > 0) {
                std::uninitialized_move_n(other.current_data(), other.size_, current_data());
            }
            size_ = other.size_;
        } else {
            set_heap_data(other.current_data(), other.capacity_); // Steal its heap_ptr & capacity
            size_ = other.size_;
        }
        other.size_ = 0;
        other.set_inline_active();
    }


    small_vector(std::initializer_list<T> ilist, const Allocator& alloc = Allocator())
        : alloc_(alloc) {
        size_type count = ilist.size();
        if (count > N_this) {
            allocate_and_set_heap(count);
        } else {
            set_inline_active();
        }
        construct_range(current_data(), ilist.begin(), ilist.end());
        size_ = count;
    }

    ~small_vector() {
        destroy_elements();
        if (!is_inline_runtime()) {
            AllocTraits::deallocate(alloc_, current_data(), capacity_); // Use current_data for heap ptr
        }
    }

    template<size_t N_other>
    small_vector& operator=(const small_vector<T, N_other, Allocator>& other) {
        if (reinterpret_cast<const void*>(this) == reinterpret_cast<const void*>(&other)) return *this;

        bool pocca = AllocTraits::propagate_on_container_copy_assignment::value;
        if (pocca && alloc_ != other.get_allocator()) {
            clear_and_deallocate_all();
            alloc_ = other.get_allocator();
        }
        assign_common(other.begin(), other.end(), other.size(), pocca);
        return *this;
    }

    small_vector& operator=(const small_vector<T, N_this, Allocator>& other) { // Non-template
        if (this == &other) return *this;
        bool pocca = AllocTraits::propagate_on_container_copy_assignment::value;
        if (pocca && alloc_ != other.alloc_) {
            clear_and_deallocate_all();
            alloc_ = other.alloc_;
        }
        assign_common(other.begin(), other.end(), other.size(), pocca);
        return *this;
    }

    template<size_t N_other>
    small_vector& operator=(small_vector<T, N_other, Allocator>&& other)
        noexcept (AllocTraits::propagate_on_container_move_assignment::value || AllocTraits::is_always_equal::value) {
        if (reinterpret_cast<void*>(this) == reinterpret_cast<void*>(&other)) return *this;

        bool pocma = AllocTraits::propagate_on_container_move_assignment::value;

        if (pocma) {
            clear_and_deallocate_all();
            alloc_ = std::move(other.get_allocator());
        } else if (alloc_ != other.get_allocator()) {
            assign_common(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()), other.size(), pocma);
            other.clear();
            return *this;
        }

        destroy_elements();
        if (!is_inline_runtime() && !pocma) {
             AllocTraits::deallocate(alloc_, current_data(), capacity_);
        }

        if (other.capacity() == N_other) { // other is inline
            if (other.size() > N_this) {
                allocate_and_set_heap(other.size());
            } else {
                set_inline_active();
            }
            if (other.size() > 0) {
                std::uninitialized_move_n(other.data(), other.size(), current_data());
            }
            size_ = other.size();
        } else {
            set_heap_data(other.steal_heap_data_for_move(), other.capacity());
            size_ = other.size();
        }

        other.clear_after_move();
        return *this;
    }

    small_vector& operator=(small_vector<T, N_this, Allocator>&& other) // Non-template
        noexcept (AllocTraits::propagate_on_container_move_assignment::value || AllocTraits::is_always_equal::value) {
        if (this == &other) return *this;
        bool pocma = AllocTraits::propagate_on_container_move_assignment::value;

        if (pocma) {
            clear_and_deallocate_all();
            alloc_ = std::move(other.alloc_);
        } else if (alloc_ != other.alloc_) {
             assign_common(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()), other.size(), pocma);
            other.clear();
            return *this;
        }

        destroy_elements();
        if (!is_inline_runtime() && !pocma) {
             AllocTraits::deallocate(alloc_, current_data(), capacity_);
        }

        if (other.is_inline_runtime()) {
            set_inline_active();
            if (other.size() > 0) {
                std::uninitialized_move_n(other.current_data(), other.size_, current_data());
            }
            size_ = other.size_;
        } else {
            set_heap_data(other.current_data(), other.capacity_);
            size_ = other.size_;
        }

        other.size_ = 0;
        other.set_inline_active();
        return *this;
    }


    small_vector& operator=(std::initializer_list<T> ilist) {
        assign_common(ilist.begin(), ilist.end(), ilist.size(), false);
        return *this;
    }

    void assign(size_type count, const_reference value) {
        destroy_elements();
        if (count > capacity_) {
            if (!is_inline_runtime()) AllocTraits::deallocate(alloc_, current_data(), capacity_);
            if (count > N_this) allocate_and_set_heap(count);
            else set_inline_active();
        }
        construct_fill(current_data(), count, value);
        size_ = count;
    }

    template <typename InputIt, typename = std::enable_if_t<std::is_base_of_v<std::input_iterator_tag, typename std::iterator_traits<InputIt>::iterator_category>>>
    void assign(InputIt first, InputIt last) {
        size_type count = static_cast<size_type>(std::distance(first, last));
        assign_common(first, last, count, false);
    }

    Allocator get_allocator() const noexcept { return alloc_; }

    reference at(size_type pos) {
        if (pos >= size_) throw std::out_of_range("small_vector::at");
        return current_data()[pos];
    }
    const_reference at(size_type pos) const {
        if (pos >= size_) throw std::out_of_range("small_vector::at");
        return current_data()[pos];
    }

    reference operator[](size_type pos) noexcept { return current_data()[pos]; }
    const_reference operator[](size_type pos) const noexcept { return current_data()[pos]; }

    reference front() noexcept { return current_data()[0]; }
    const_reference front() const noexcept { return current_data()[0]; }

    reference back() noexcept { return current_data()[size_ - 1]; }
    const_reference back() const noexcept { return current_data()[size_ - 1]; }

    T* data() noexcept { return current_data(); }
    const T* data() const noexcept { return current_data(); }

    iterator begin() noexcept { return current_data(); }
    const_iterator begin() const noexcept { return current_data(); }
    const_iterator cbegin() const noexcept { return current_data(); }

    iterator end() noexcept { return current_data() + size_; }
    const_iterator end() const noexcept { return current_data() + size_; }
    const_iterator cend() const noexcept { return current_data() + size_; }

    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }

    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }


    bool empty() const noexcept { return size_ == 0; }
    size_type size() const noexcept { return size_; }
    size_type max_size() const noexcept { return AllocTraits::max_size(alloc_); }

    void reserve(size_type new_cap_request) {
        if (new_cap_request <= capacity_) return;
        if (new_cap_request > max_size()) throw std::length_error("small_vector::reserve exceeds max_size");

        pointer old_buffer_ptr = current_data();
        bool was_inline = is_inline_runtime();
        size_type old_capacity = capacity_;

        pointer new_buffer_ptr = AllocTraits::allocate(alloc_, new_cap_request);

        try {
            if (size_ > 0) {
                std::uninitialized_move_n(old_buffer_ptr, size_, new_buffer_ptr);
            }
        } catch (...) {
            AllocTraits::deallocate(alloc_, new_buffer_ptr, new_cap_request);
            throw;
        }

        if (size_ > 0) {
             destroy_elements_in_buffer(old_buffer_ptr, size_);
        }

        if (!was_inline) {
            AllocTraits::deallocate(alloc_, old_buffer_ptr, old_capacity);
        }

        set_heap_data(new_buffer_ptr, new_cap_request);
    }

    size_type capacity() const noexcept { return capacity_; }

    void clear() noexcept {
        destroy_elements();
        size_ = 0;
    }

    void push_back(const_reference value) {
        if (size_ == capacity_) {
            grow_to_at_least(size_ + 1);
        }
        AllocTraits::construct(alloc_, current_data() + size_, value);
        ++size_;
    }

    void push_back(T&& value) {
        if (size_ == capacity_) {
            grow_to_at_least(size_ + 1);
        }
        AllocTraits::construct(alloc_, current_data() + size_, std::move(value));
        ++size_;
    }

    template <typename... Args>
    reference emplace_back(Args&&... args) {
        if (size_ == capacity_) {
            grow_to_at_least(size_ + 1);
        }
        pointer element_ptr = current_data() + size_;
        AllocTraits::construct(alloc_, element_ptr, std::forward<Args>(args)...);
        ++size_;
        return *element_ptr;
    }

    void pop_back() {
        if (empty()) return;
        --size_;
        AllocTraits::destroy(alloc_, current_data() + size_);
    }

    void resize(size_type new_size) {
        static_assert(std::is_default_constructible_v<T>, "T must be default constructible for resize without value.");
        if (new_size < size_) {
            destroy_elements_in_buffer(current_data() + new_size, size_ - new_size);
        } else if (new_size > size_) {
            if (new_size > capacity_) {
                grow_to_at_least(new_size);
            }
            construct_default(current_data() + size_, new_size - size_);
        }
        size_ = new_size;
    }

    void resize(size_type new_size, const_reference value) {
        if (new_size < size_) {
            destroy_elements_in_buffer(current_data() + new_size, size_ - new_size);
        } else if (new_size > size_) {
            if (new_size > capacity_) {
                grow_to_at_least(new_size);
            }
            construct_fill(current_data() + size_, new_size - size_, value);
        }
        size_ = new_size;
    }

    void swap(small_vector& other) noexcept (
        AllocTraits::propagate_on_container_swap::value || AllocTraits::is_always_equal::value
    ) {
        if (this == &other) return; // Must be same type (N_this == other.N_this) for this non-templated swap

        bool pocs = AllocTraits::propagate_on_container_swap::value;
        if (pocs || alloc_ == other.alloc_) {
            bool this_was_inline = this->is_inline_runtime();
            bool other_was_inline = other.is_inline_runtime();

            if (this_was_inline && other_was_inline) {
                // Both inline: element-wise swap/move
                // This part of the logic remains the same as it was deemed plausible
                size_type min_s = std::min(size_, other.size_);
                for (size_type i = 0; i < min_s; ++i) {
                    std::swap(current_data()[i], other.current_data()[i]);
                }
                if (size_ > other.size_) {
                    std::uninitialized_move_n(current_data() + min_s, size_ - min_s, other.current_data() + min_s);
                    destroy_elements_in_buffer(current_data() + min_s, size_ - min_s);
                } else if (other.size_ > size_) {
                    std::uninitialized_move_n(other.current_data() + min_s, other.size_ - min_s, current_data() + min_s);
                    destroy_elements_in_buffer(other.current_data() + min_s, other.size_ - min_s);
                }
            } else if (!this_was_inline && !other_was_inline) {
                // Both heap: swap heap pointers
                std::swap(storage_.heap_ptr, other.storage_.heap_ptr);
                // capacity_ will be swapped later by the conditional global swap
            } else {
                // Mixed case: one inline, one heap
                small_vector* p_inline = this_was_inline ? this : &other;
                small_vector* p_heap = this_was_inline ? &other : this;

                pointer heap_buffer_ptr = p_heap->current_data(); // Get heap data
                size_type heap_buffer_capacity = p_heap->capacity_;

                // Temp storage for inline contents
                std::aligned_storage_t<sizeof(T) * N_this, alignof(T)> temp_inline_storage_bytes;
                pointer temp_inline_elements = std::launder(reinterpret_cast<pointer>(&temp_inline_storage_bytes));

                if (p_inline->size_ > 0) { // Move from p_inline's inline buf to temp
                   std::uninitialized_move_n(p_inline->current_data(), p_inline->size_, temp_inline_elements);
                }
                size_type inline_content_size = p_inline->size_;
                // p_inline's inline elements are now moved-from.

                // p_inline (was inline) becomes heap, takes p_heap's buffer
                p_inline->set_heap_data(heap_buffer_ptr, heap_buffer_capacity);

                // p_heap (was heap) becomes inline, takes p_inline's original content from temp
                p_heap->set_inline_active();
                if (inline_content_size > 0) {
                    std::uninitialized_move_n(temp_inline_elements, inline_content_size, p_heap->current_data());
                }

                // Destroy moved-from elements in temp buffer (using p_inline's allocator at time of move)
                if (!std::is_trivially_destructible_v<T> && inline_content_size > 0) {
                     detail::destroy_range_helper(temp_inline_elements, temp_inline_elements + inline_content_size, p_inline->alloc_);
                }
                // After this block, p_inline has p_heap's old capacity, and p_heap has N_this capacity.
                // These should NOT be swapped by the global capacity swap.
            }

            // Common state swaps
            std::swap(size_, other.size_);
            if (this_was_inline == other_was_inline) { // Only swap capacity if states were originally the same
                 std::swap(capacity_, other.capacity_);
            }
            // For mixed case, capacities were set by set_heap_data/set_inline_active and must not be swapped again.

            if (pocs) {
                std::swap(alloc_, other.alloc_);
            }
        } else { // Fallback for incompatible allocators where POCZS is false
            small_vector temp = std::move(*this);
            *this = std::move(other);
            other = std::move(temp);
        }
    }

private:
    void destroy_elements() noexcept {
        if (size_ > 0) {
            destroy_elements_in_buffer(current_data(), size_);
        }
    }

    void destroy_elements_in_buffer(pointer buffer_start, size_type num_elements) noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            if (num_elements > 0) {
                for (size_type i = 0; i < num_elements; ++i) {
                    AllocTraits::destroy(alloc_, buffer_start + i);
                }
            }
        }
    }

    template<typename InputIt>
    void construct_range(pointer dest, InputIt first, InputIt last) {
        pointer current_ptr = dest;
        try {
            for (; first != last; ++first, ++current_ptr) {
                AllocTraits::construct(alloc_, current_ptr, *first);
            }
        } catch (...) {
            destroy_elements_in_buffer(dest, static_cast<size_type>(current_ptr - dest) );
            throw;
        }
    }

    void construct_fill(pointer dest, size_type count, const_reference value) {
        pointer current_ptr = dest;
        try {
            for (size_type i = 0; i < count; ++i, ++current_ptr) {
                AllocTraits::construct(alloc_, current_ptr, value);
            }
        } catch (...) {
            destroy_elements_in_buffer(dest, static_cast<size_type>(current_ptr - dest));
            throw;
        }
    }

    void construct_default(pointer dest, size_type count) {
        pointer current_ptr = dest;
        try {
            for (size_type i = 0; i < count; ++i, ++current_ptr) {
                AllocTraits::construct(alloc_, current_ptr);
            }
        } catch (...) {
            destroy_elements_in_buffer(dest, static_cast<size_type>(current_ptr - dest));
            throw;
        }
    }

    void allocate_and_set_heap(size_type required_cap) {
        if (required_cap > max_size()) throw std::length_error("small_vector allocation exceeds max_size");
        pointer new_ptr = AllocTraits::allocate(alloc_, required_cap);
        set_heap_data(new_ptr, required_cap);
    }

    void grow_to_at_least(size_type required_min_capacity) {
        size_type new_capacity = calculate_new_capacity(required_min_capacity);
        if (new_capacity > max_size()) new_capacity = max_size();
        if (new_capacity < required_min_capacity && required_min_capacity <= max_size()) {
             new_capacity = required_min_capacity; // Ensure it meets requirement if possible
        } else if (new_capacity < required_min_capacity) {
            throw std::length_error("small_vector growth cannot meet requirement due to max_size");
        }


        pointer new_buffer = AllocTraits::allocate(alloc_, new_capacity);
        pointer old_data_ptr = current_data();
        bool was_inline = is_inline_runtime();
        size_type old_cap = capacity_;

        try {
            if (size_ > 0) {
                std::uninitialized_move_n(old_data_ptr, size_, new_buffer);
            }
        } catch (...) {
            AllocTraits::deallocate(alloc_, new_buffer, new_capacity);
            throw;
        }

        if (size_ > 0) {
             destroy_elements_in_buffer(old_data_ptr, size_);
        }

        if (!was_inline) {
            AllocTraits::deallocate(alloc_, old_data_ptr, old_cap);
        }

        set_heap_data(new_buffer, new_capacity);
    }

    size_type calculate_new_capacity(size_type required_min_capacity) const {
        size_type current_cap = capacity_;
        size_type new_cap = (current_cap == 0 && N_this > 0) ? N_this : current_cap + current_cap / 2;
        if (new_cap < N_this && N_this > 0) new_cap = N_this;
        if (new_cap < required_min_capacity) new_cap = required_min_capacity;
        return new_cap;
    }

    void clear_and_deallocate_all() {
        destroy_elements();
        if (!is_inline_runtime()) {
            AllocTraits::deallocate(alloc_, current_data(), capacity_);
        }
        size_ = 0;
        set_inline_active();
    }

    template<typename Iter>
    void assign_common(Iter first, Iter last, size_type count, bool pocca_alloc_changed_in_caller) {
        if (count > capacity_) { // Path 1: Reallocation needed
            // 1. Destroy existing elements IN THE CURRENT BUFFER.
            //    This must happen before the buffer is potentially deallocated or repurposed.
            //    size_ still holds the old size here.
            destroy_elements_in_buffer(current_data(), size_);

            // 2. Deallocate old buffer if it was heap and its deallocation wasn't handled by the caller (e.g. via POCCA)
            if (!is_inline_runtime() && !pocca_alloc_changed_in_caller) {
                AllocTraits::deallocate(alloc_, current_data(), capacity_);
                // After this, capacity_ is conceptually N_this, or rather, the state is "no heap buffer".
            }
            // Note: if pocca_alloc_changed_in_caller was true, clear_and_deallocate_all() was called by operator=,
            // which already destroyed elements and deallocated heap. So, the above is effectively skipped or harmless.
            // In all cases, *this is now considered to have no valid elements or specific heap buffer from before.

            // 3. Allocate new storage based on 'count'
            if (count > N_this) {
                allocate_and_set_heap(count); // Allocates and sets storage_.heap_ptr, capacity_
            } else {
                set_inline_active(); // Uses inline buffer, sets capacity_ = N_this
            }

            // 4. Construct new elements. size_ should be 0 before this.
            size_ = 0; // Set size to 0 before construction
            construct_range(current_data(), first, last);
            size_ = count; // Set new size

        } else { // Path 2: Enough capacity, no reallocation.
            // Strategy: Destroy ALL old elements currently in the vector, then construct ALL new ones.
            // This matches std::vector::assign behavior and the test's expectation.
            pointer d = current_data();
            size_type old_size = size_;

            destroy_elements_in_buffer(d, old_size); // Destroy all 'old_size' elements in place

            // The memory itself (pointed to by d) is still valid and has enough capacity.
            // Construct the new 'count' elements into the start of the buffer.
            // construct_range needs size to be conceptually 0 for its internal logic if it relies on it,
            // but it just writes to `dest`. So, direct construction is fine.
            // Size is effectively 0 after destruction for the purpose of new construction.
            construct_range(d, first, last);
            size_ = count; // Update size to the new count
        }
    }

    template <typename T2, size_t N2, typename Alloc2>
    friend class small_vector;

    friend void detail::destroy_range_helper<T, Allocator>(T* first, T* last, Allocator& alloc);
};


template <typename T, size_t N_lhs, typename AllocatorLhs, size_t N_rhs, typename AllocatorRhs>
bool operator==(const small_vector<T, N_lhs, AllocatorLhs>& lhs, const small_vector<T, N_rhs, AllocatorRhs>& rhs) {
    if (lhs.size() != rhs.size()) return false;
    // TODO: Consider allocator equality if required by standard for cross-type comparison
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename T, size_t N_lhs, typename AllocatorLhs, size_t N_rhs, typename AllocatorRhs>
bool operator!=(const small_vector<T, N_lhs, AllocatorLhs>& lhs, const small_vector<T, N_rhs, AllocatorRhs>& rhs) {
    return !(lhs == rhs);
}

template <typename T, size_t N_lhs, typename AllocatorLhs, size_t N_rhs, typename AllocatorRhs>
bool operator<(const small_vector<T, N_lhs, AllocatorLhs>& lhs, const small_vector<T, N_rhs, AllocatorRhs>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename T, size_t N_lhs, typename AllocatorLhs, size_t N_rhs, typename AllocatorRhs>
bool operator<=(const small_vector<T, N_lhs, AllocatorLhs>& lhs, const small_vector<T, N_rhs, AllocatorRhs>& rhs) {
    return !(rhs < lhs);
}

template <typename T, size_t N_lhs, typename AllocatorLhs, size_t N_rhs, typename AllocatorRhs>
bool operator>(const small_vector<T, N_lhs, AllocatorLhs>& lhs, const small_vector<T, N_rhs, AllocatorRhs>& rhs) {
    return rhs < lhs;
}

template <typename T, size_t N_lhs, typename AllocatorLhs, size_t N_rhs, typename AllocatorRhs>
bool operator>=(const small_vector<T, N_lhs, AllocatorLhs>& lhs, const small_vector<T, N_rhs, AllocatorRhs>& rhs) {
    return !(lhs < rhs);
}


namespace std {
// Swap only defined for small_vectors of the exact same type (including N)
template <typename T, size_t N_val, typename Allocator> // N_val used to avoid conflict with N_this
void swap(small_vector<T, N_val, Allocator>& lhs, small_vector<T, N_val, Allocator>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}
}

namespace detail {
    template <typename T_detail, typename Alloc_detail>
    void destroy_range_helper(T_detail* first, T_detail* last, Alloc_detail& alloc) {
        using Traits = std::allocator_traits<Alloc_detail>;
        if constexpr (!std::is_trivially_destructible_v<T_detail>) {
            for (; first != last; ++first) {
                Traits::destroy(alloc, first);
            }
        }
    }
}

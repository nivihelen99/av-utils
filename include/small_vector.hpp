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
template <typename T_detail, typename Alloc_detail> // Changed T to T_detail, Allocator to Alloc_detail
void destroy_range_helper(T_detail* first, T_detail* last, Alloc_detail& alloc); // Renamed to avoid conflict
}

template <typename T, size_t N, typename Allocator = std::allocator<T>>
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

    // Basic iterator implementation (can be improved)
    using iterator = T*;
    using const_iterator = const T*;


private:
    static_assert(N > 0, "small_vector inline capacity N must be greater than 0.");

    using AllocTraits = std::allocator_traits<Allocator>;

    union Storage {
        std::aligned_storage_t<sizeof(T) * N, alignof(T)> inline_buffer;
        pointer heap_ptr;

        Storage() {}
        ~Storage() {}
    } storage_;

    size_type size_ = 0;
    size_type capacity_ = N;
    [[no_unique_address]] Allocator alloc_;

    bool is_inline() const noexcept {
        // A robust check: if current_data() points within &storage_.inline_buffer
        // For this implementation, capacity_ > N implies heap.
        // If capacity_ == N, it could be inline or a heap allocation of exactly N (unlikely with growth strategy).
        // Let's refine: if heap_ptr is active and points outside inline_buffer.
        // The simplest is: if capacity_ == N, we assume it's inline unless heap_ptr was explicitly set.
        // The current logic: capacity_ is N for inline, and >N for heap.
        // If we allocate heap, capacity_ is updated. If we shrink back to inline, capacity_ becomes N.
        return current_data() == get_inline_ptr_raw();
    }

    pointer get_inline_ptr_raw() noexcept {
        return std::launder(reinterpret_cast<pointer>(&storage_.inline_buffer));
    }

    const_pointer get_inline_ptr_raw() const noexcept {
        return std::launder(reinterpret_cast<const_pointer>(&storage_.inline_buffer));
    }


    pointer get_heap_ptr() noexcept {
        return storage_.heap_ptr;
    }

    const_pointer get_heap_ptr() const noexcept {
        return storage_.heap_ptr;
    }

    pointer current_data() noexcept {
        // If capacity_ == N, it implies inline data buffer.
        // If capacity_ > N, it implies heap_ptr is active.
        return (capacity_ == N) ? get_inline_ptr_raw() : storage_.heap_ptr;
    }

    const_pointer current_data() const noexcept {
        return (capacity_ == N) ? get_inline_ptr_raw() : storage_.heap_ptr;
    }

    void set_heap_data(pointer ptr, size_type new_capacity) noexcept {
        storage_.heap_ptr = ptr;
        capacity_ = new_capacity;
    }

    void set_inline_active() noexcept {
        // Called when data is moved back to inline buffer or vector is reset to inline state
        capacity_ = N;
        // Union management: current_data() will now return inline_ptr_raw()
    }


public:
    // Constructors
    small_vector() noexcept(std::is_nothrow_default_constructible_v<Allocator>)
        : size_(0), capacity_(N), alloc_(Allocator()) {}

    explicit small_vector(const Allocator& alloc) noexcept
        : size_(0), capacity_(N), alloc_(alloc) {}

    small_vector(size_type count, const_reference value, const Allocator& alloc = Allocator())
        : alloc_(alloc) {
        if (count > N) {
            allocate_and_set_heap(count); // Sets heap_ptr and capacity_
        } else {
            set_inline_active(); // capacity_ = N
        }
        construct_fill(current_data(), count, value);
        size_ = count;
    }

    explicit small_vector(size_type count, const Allocator& alloc = Allocator())
        : alloc_(alloc) {
        static_assert(std::is_default_constructible_v<T>, "T must be default constructible for this constructor.");
        if (count > N) {
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
        // Need to determine distance first to decide on allocation strategy
        // This is inefficient for non-random access iterators.
        // A common approach is to construct iteratively, growing as needed.
        // For simplicity here, let's get distance first.
        size_type count = static_cast<size_type>(std::distance(first, last));
        if (count > N) {
            allocate_and_set_heap(count);
        } else {
            set_inline_active();
        }
        construct_range(current_data(), first, last);
        size_ = count;
    }

    small_vector(const small_vector& other)
        : alloc_(AllocTraits::select_on_container_copy_construction(other.alloc_)) {
        if (other.size_ > N) {
            allocate_and_set_heap(other.size_);
        } else {
            set_inline_active();
        }
        construct_range(current_data(), other.begin(), other.end());
        size_ = other.size_;
    }

    small_vector(const small_vector& other, const Allocator& alloc)
        : alloc_(alloc) {
        if (other.size_ > N) {
            allocate_and_set_heap(other.size_);
        } else {
            set_inline_active();
        }
        construct_range(current_data(), other.begin(), other.end());
        size_ = other.size_;
    }

    small_vector(small_vector&& other) noexcept(std::is_nothrow_move_constructible_v<Allocator>)
        : alloc_(std::move(other.alloc_)) { // Move allocator first
        if (other.is_inline()) {
            set_inline_active(); // We become inline
            if (other.size_ > 0) {
                std::uninitialized_move_n(other.current_data(), other.size_, current_data());
            }
            size_ = other.size_;
        } else { // Other is on heap, steal its buffer
            set_heap_data(other.get_heap_ptr(), other.capacity_);
            size_ = other.size_;
        }
        // Invalidate other: make it empty and inline
        other.size_ = 0;
        other.set_inline_active(); // other.capacity_ becomes N
                                   // other.storage_.heap_ptr is no longer valid if it was heap
    }


    small_vector(small_vector&& other, const Allocator& alloc)
    : alloc_(alloc) { // Use provided allocator
        if (alloc_ == other.alloc_) { // Allocators compatible for stealing
            if (other.is_inline()) {
                set_inline_active();
                if (other.size_ > 0) {
                    std::uninitialized_move_n(other.current_data(), other.size_, current_data());
                }
                size_ = other.size_;
            } else { // Other is heap, steal
                set_heap_data(other.get_heap_ptr(), other.capacity_);
                size_ = other.size_;
            }
            other.size_ = 0;
            other.set_inline_active();
        } else { // Allocators differ, must allocate and move/copy elements
            if (other.size_ > N) {
                allocate_and_set_heap(other.size_);
            } else {
                set_inline_active();
            }
            // This simplified version uses uninitialized_move. Real std::vector uses allocator-aware move.
            if (other.size_ > 0) {
                 std::uninitialized_move_n(other.begin(), other.size_, current_data());
            }
            size_ = other.size_;
            // Elements in 'other' are moved-from. Destroy them if necessary for 'other's cleanup.
            // Since 'other' will be destructed or cleared later, its own destructor/clear will handle its elements.
            // For safety, we could clear 'other' here.
            other.clear(); // Ensures its elements are properly destroyed if non-trivial and not fully "empty" after move.
        }
    }


    small_vector(std::initializer_list<T> ilist, const Allocator& alloc = Allocator())
        : alloc_(alloc) {
        size_type count = ilist.size();
        if (count > N) {
            allocate_and_set_heap(count);
        } else {
            set_inline_active();
        }
        construct_range(current_data(), ilist.begin(), ilist.end());
        size_ = count;
    }

    ~small_vector() {
        destroy_elements();
        if (!is_inline()) {
            AllocTraits::deallocate(alloc_, get_heap_ptr(), capacity_);
        }
    }

    small_vector& operator=(const small_vector& other) {
        if (this == &other) return *this;

        bool pocca = AllocTraits::propagate_on_container_copy_assignment::value;
        if (pocca && alloc_ != other.alloc_) {
            clear_and_deallocate_all(); // Deallocate with old alloc_
            alloc_ = other.alloc_;      // Assign new allocator
        }

        assign_common(other.begin(), other.end(), other.size(), pocca);
        return *this;
    }

    small_vector& operator=(small_vector&& other) noexcept (
        AllocTraits::propagate_on_container_move_assignment::value ||
        AllocTraits::is_always_equal::value
    ) {
        if (this == &other) return *this;

        bool pocma = AllocTraits::propagate_on_container_move_assignment::value;

        if (pocma) { // If POCMA is true, allocator is moved.
            clear_and_deallocate_all(); // Deallocate existing memory with current allocator.
            alloc_ = std::move(other.alloc_); // Move the allocator
        } else if (alloc_ != other.alloc_) {
            // POCMA false and allocators differ: must element-wise move.
            // Content of 'other' is effectively copied/moved, 'other' is left in a valid state.
            assign_common(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()), other.size(), pocma);
            other.clear(); // Clear other as its resources were not stolen but moved from.
            return *this;
        }
        // If POCMA is true OR allocators are equal (and POCMA is false): we can steal resources.
        // Destroy our current elements and deallocate heap if necessary.
        // (This is slightly different from clear_and_deallocate_all if alloc_ was already moved for POCMA)
        // If alloc_ was just moved, use the new alloc_ for deallocation if we were heap. This is complex.
        // Let's simplify: destroy elements, then deallocate heap *before* alloc_ move for POCMA.
        // If not POCMA but allocs are equal, just destroy/dealloc our stuff.

        destroy_elements();
        if (!is_inline() && !pocma) { // If not POCMA, deallocate with current alloc
             AllocTraits::deallocate(alloc_, get_heap_ptr(), capacity_);
        } else if (!is_inline() && pocma) {
            // If POCMA, alloc_ is already other.alloc_. Our old buffer needs deallocation with our *old* allocator.
            // This is tricky. The standard says if POCMA is true, old resources are released via old allocator.
            // This implies `clear_and_deallocate_all()` should have happened before `alloc_ = std::move(other.alloc_)`.
            // My `clear_and_deallocate_all` is called before `alloc_` move, so it's correct.
        }


        // Now, take ownership of other's resources
        if (other.is_inline()) {
            set_inline_active(); // We become inline (or stay if already)
            if (other.size_ > 0) {
                std::uninitialized_move_n(other.current_data(), other.size_, current_data());
            }
            size_ = other.size_;
        } else { // Other is on heap, steal it
            set_heap_data(other.get_heap_ptr(), other.capacity_);
            size_ = other.size_;
        }

        // Invalidate other
        other.size_ = 0;
        other.set_inline_active(); // Reset other to valid empty inline state
        return *this;
    }


    small_vector& operator=(std::initializer_list<T> ilist) {
        assign_common(ilist.begin(), ilist.end(), ilist.size(), false); // POCMA doesn't apply here
        return *this;
    }

    void assign(size_type count, const_reference value) {
        destroy_elements();
        if (count > capacity_) {
            if (!is_inline()) AllocTraits::deallocate(alloc_, get_heap_ptr(), capacity_);
            if (count > N) allocate_and_set_heap(count);
            else set_inline_active();
        }
        construct_fill(current_data(), count, value);
        size_ = count;
    }

    template <typename InputIt, typename = std::enable_if_t<std::is_base_of_v<std::input_iterator_tag, typename std::iterator_traits<InputIt>::iterator_category>>>
    void assign(InputIt first, InputIt last) {
        // This is simplified. A proper assign would handle iterator categories
        // to optimize for random access (get distance) vs input (grow iteratively).
        size_type count = static_cast<size_type>(std::distance(first, last));
        assign_common(first, last, count, false);
    }


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

        // If currently inline, and new_cap_request > N, then must go to heap.
        // If currently on heap, and new_cap_request > current capacity, must reallocate.
        // If currently inline, and new_cap_request <= N, do nothing (already covered by first check).

        pointer old_buffer_ptr = current_data();
        bool was_inline = is_inline();
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

        if (!std::is_trivially_destructible_v<T> && size_ > 0) {
             destroy_elements_in_buffer(old_buffer_ptr, size_);
        }

        if (!was_inline) {
            AllocTraits::deallocate(alloc_, old_buffer_ptr, old_capacity);
        }

        set_heap_data(new_buffer_ptr, new_cap_request); // capacity_ updated here
    }

    size_type capacity() const noexcept { return capacity_; }

    void clear() noexcept {
        destroy_elements();
        size_ = 0;
        // Capacity remains, buffer (inline or heap) remains allocated.
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
        // No automatic shrink-to-fit to inline implemented here.
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
        if (this == &other) return;

        bool pocs = AllocTraits::propagate_on_container_swap::value;
        if (pocs || alloc_ == other.alloc_) { // Can do true swap of resources
            if (is_inline() && other.is_inline()) {
                // Both inline: element-wise swap up to min_size, move for tails.
                size_type min_s = std::min(size_, other.size_);
                for (size_type i = 0; i < min_s; ++i) {
                    std::swap((*this)[i], other[i]);
                }
                if (size_ > other.size_) { // this has more elements
                    std::uninitialized_move_n(current_data() + min_s, size_ - min_s, other.current_data() + min_s);
                    destroy_elements_in_buffer(current_data() + min_s, size_ - min_s);
                } else if (other.size_ > size_) { // other has more elements
                    std::uninitialized_move_n(other.current_data() + min_s, other.size_ - min_s, current_data() + min_s);
                    destroy_elements_in_buffer(other.current_data() + min_s, other.size_ - min_s);
                }
            } else if (!is_inline() && !other.is_inline()) { // Both heap
                std::swap(storage_.heap_ptr, other.storage_.heap_ptr);
                // std::swap(capacity_, other.capacity_); // Swapped below
            } else { // Mixed: one inline, one heap. This is the complex one.
                small_vector* p_inline = is_inline() ? this : &other;
                small_vector* p_heap = is_inline() ? &other : this;

                pointer heap_buf_content = p_heap->get_heap_ptr();
                size_type heap_buf_capacity = p_heap->capacity();
                size_type heap_content_size = p_heap->size();

                // Temp store inline contents (on stack if small enough, or temp heap alloc)
                // Using std::aligned_storage for stack buffer:
                std::aligned_storage_t<sizeof(T) * N, alignof(T)> temp_inline_storage_bytes;
                pointer temp_inline_elements = std::launder(reinterpret_cast<pointer>(&temp_inline_storage_bytes));

                if (p_inline->size_ > 0) {
                   std::uninitialized_move_n(p_inline->current_data(), p_inline->size_, temp_inline_elements);
                }
                size_type inline_content_size = p_inline->size_;
                // Original inline elements in p_inline are now moved-from, effectively destroyed for p_inline.

                // p_inline takes ownership of p_heap's buffer and state
                p_inline->set_heap_data(heap_buf_content, heap_buf_capacity);
                // p_inline->size_ will be updated by std::swap(size_, other.size_) below.

                // p_heap becomes inline, takes ownership of p_inline's original content (from temp_inline_elements)
                p_heap->set_inline_active(); // capacity becomes N
                if (inline_content_size > 0) {
                    std::uninitialized_move_n(temp_inline_elements, inline_content_size, p_heap->current_data());
                }
                // Destroy elements in temp_inline_storage (they were moved from)
                if (!std::is_trivially_destructible_v<T> && inline_content_size > 0) {
                     detail::destroy_range_helper(temp_inline_elements, temp_inline_elements + inline_content_size, p_inline->alloc_);
                }
            }
            // Swap size, capacity, and allocator (if POCZS)
            std::swap(size_, other.size_);
            std::swap(capacity_, other.capacity_); // This correctly swaps N with heap_capacity etc.
            if (pocs) {
                std::swap(alloc_, other.alloc_);
            }
        } else { // Allocators differ and POCZS is false. Element-wise move/swap.
            // This is not O(1) and has complex exception safety. Standard says UB for std::vector.
            // Fallback to move assignments (less efficient, not necessarily noexcept)
            small_vector temp = std::move(*this); // Requires move ctor
            *this = std::move(other);      // Requires move assignment
            other = std::move(temp);       // Requires move assignment
        }
    }


private:
    void destroy_elements() noexcept { // Destroys all elements [0, size_)
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
        if (new_capacity > max_size()) new_capacity = max_size(); // Cap at max_size
        if (new_capacity < required_min_capacity) throw std::length_error("small_vector growth cannot meet requirement");

        pointer new_buffer = AllocTraits::allocate(alloc_, new_capacity);
        pointer old_data_ptr = current_data();
        bool was_inline = is_inline();
        size_type old_cap = capacity_;

        try {
            if (size_ > 0) {
                std::uninitialized_move_n(old_data_ptr, size_, new_buffer);
            }
        } catch (...) {
            AllocTraits::deallocate(alloc_, new_buffer, new_capacity);
            throw;
        }

        if (size_ > 0) { // Destroy original elements (they were moved from)
             destroy_elements_in_buffer(old_data_ptr, size_);
        }

        if (!was_inline) { // Deallocate old heap buffer if we were on heap
            AllocTraits::deallocate(alloc_, old_data_ptr, old_cap);
        }

        set_heap_data(new_buffer, new_capacity); // Updates storage_ and capacity_
    }

    size_type calculate_new_capacity(size_type required_min_capacity) const {
        size_type current_cap = capacity();
        size_type new_cap = (current_cap == 0) ? N : current_cap + current_cap / 2; // 1.5x growth, ensure at least N
        if (new_cap < N) new_cap = N;
        if (new_cap < required_min_capacity) new_cap = required_min_capacity;
        return new_cap;
    }

    void clear_and_deallocate_all() { // Used before changing allocator
        destroy_elements();
        if (!is_inline()) {
            AllocTraits::deallocate(alloc_, get_heap_ptr(), capacity_);
        }
        size_ = 0;
        set_inline_active(); // Conceptually back to inline state for next operations
    }

    template<typename Iter>
    void assign_common(Iter first, Iter last, size_type count, bool pocca_alloc_changed) {
        if (count > capacity_) {
            if (!pocca_alloc_changed && !is_inline()) { // If not POCCA and we were heap, dealloc with current alloc
                 AllocTraits::deallocate(alloc_, get_heap_ptr(), capacity_);
            }
            // If POCCA, old buffer was deallocated by clear_and_deallocate_all() with old alloc.
            // New alloc is already set.

            if (count > N) allocate_and_set_heap(count); // Uses current (possibly new) alloc
            else set_inline_active(); // Uses current (possibly new) alloc for inline
            // size_ is 0 here
            construct_range(current_data(), first, last);
            size_ = count;
        } else { // Enough capacity
            // Destroy elements reusable in place, copy/move assign, then construct/destroy tails.
            // Simpler: destroy all, then construct all. (matches current clear() + construct)
            destroy_elements(); // size_ becomes effectively 0 for construction
            construct_range(current_data(), first, last);
            size_ = count;
        }
    }

    friend void detail::destroy_range_helper<T, Allocator>(T* first, T* last, Allocator& alloc);
};


template <typename T, size_t N, typename Allocator>
bool operator==(const small_vector<T, N, Allocator>& lhs, const small_vector<T, N, Allocator>& rhs) {
    if (lhs.size() != rhs.size()) return false;
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename T, size_t N, typename Allocator>
bool operator!=(const small_vector<T, N, Allocator>& lhs, const small_vector<T, N, Allocator>& rhs) {
    return !(lhs == rhs);
}

template <typename T, size_t N, typename Allocator>
bool operator<(const small_vector<T, N, Allocator>& lhs, const small_vector<T, N, Allocator>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename T, size_t N, typename Allocator>
bool operator<=(const small_vector<T, N, Allocator>& lhs, const small_vector<T, N, Allocator>& rhs) {
    return !(rhs < lhs);
}

template <typename T, size_t N, typename Allocator>
bool operator>(const small_vector<T, N, Allocator>& lhs, const small_vector<T, N, Allocator>& rhs) {
    return rhs < lhs;
}

template <typename T, size_t N, typename Allocator>
bool operator>=(const small_vector<T, N, Allocator>& lhs, const small_vector<T, N, Allocator>& rhs) {
    return !(lhs < rhs);
}


namespace std {
template <typename T, size_t N, typename Allocator>
void swap(small_vector<T, N, Allocator>& lhs, small_vector<T, N, Allocator>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
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

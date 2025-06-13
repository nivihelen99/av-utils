#pragma once

#include <atomic>
#include <memory>
#include <type_traits>
#include <utility>
#include <functional> // Added for std::function
#include <cstddef>
#include <new>
#include <bit>
#include <concepts>
#include <optional>
#include <chrono>
#include <thread>

namespace concurrent {

// Memory ordering policies for different performance/safety trade-offs
enum class memory_ordering {
    relaxed,        // Maximum performance, minimal ordering guarantees
    acquire_release, // Balanced performance and safety (default)
    sequential      // Strongest guarantees, may impact performance
};

namespace detail {
    // Compile-time power of 2 check
    constexpr bool is_power_of_two(std::size_t n) {
        return n > 0 && (n & (n - 1)) == 0;
    }

    // Get memory ordering for load operations
    constexpr std::memory_order get_load_order(memory_ordering policy) {
        switch (policy) {
            case memory_ordering::relaxed: return std::memory_order_relaxed;
            case memory_ordering::acquire_release: return std::memory_order_acquire;
            case memory_ordering::sequential: return std::memory_order_seq_cst;
        }
        return std::memory_order_acquire;
    }

    // Get memory ordering for store operations
    constexpr std::memory_order get_store_order(memory_ordering policy) {
        switch (policy) {
            case memory_ordering::relaxed: return std::memory_order_relaxed;
            case memory_ordering::acquire_release: return std::memory_order_release;
            case memory_ordering::sequential: return std::memory_order_seq_cst;
        }
        return std::memory_order_release;
    }

    // Cache line size for padding
    constexpr std::size_t cache_line_size = 64;

    // Cache line aligned storage
    template<typename T>
    struct alignas(cache_line_size) cache_aligned {
        T value;
        
        template<typename... Args>
        explicit cache_aligned(Args&&... args) : value(std::forward<Args>(args)...) {}
        
        operator T&() noexcept { return value; }
        operator const T&() const noexcept { return value; }
        
        T& get() noexcept { return value; }
        const T& get() const noexcept { return value; }
    };
}

// Statistics for monitoring ring buffer performance
struct ring_buffer_stats {
    std::atomic<std::uint64_t> total_pushes{0};
    std::atomic<std::uint64_t> total_pops{0};
    std::atomic<std::uint64_t> failed_pushes{0};  // When buffer was full
    std::atomic<std::uint64_t> failed_pops{0};    // When buffer was empty
    std::atomic<std::uint64_t> contention_events{0}; // High-water mark hits
    
    void reset() noexcept {
        total_pushes.store(0, std::memory_order_relaxed);
        total_pops.store(0, std::memory_order_relaxed);
        failed_pushes.store(0, std::memory_order_relaxed);
        failed_pops.store(0, std::memory_order_relaxed);
        contention_events.store(0, std::memory_order_relaxed);
    }
    
    double utilization() const noexcept {
        auto pushes = total_pushes.load(std::memory_order_relaxed);
        auto failed = failed_pushes.load(std::memory_order_relaxed);
        if (pushes + failed == 0) return 0.0;
        return static_cast<double>(pushes) / (pushes + failed);
    }
};

template<
    typename T,
    memory_ordering Ordering = memory_ordering::acquire_release,
    typename Allocator = std::allocator<T>
>
class ring_buffer {
public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

    static constexpr memory_ordering ordering_policy = Ordering;
    static constexpr std::memory_order load_order = detail::get_load_order(Ordering);
    static constexpr std::memory_order store_order = detail::get_store_order(Ordering);

private:
    // Storage for ring buffer elements
    struct slot {
        alignas(T) std::byte storage[sizeof(T)];
        std::atomic<bool> ready{false};
        
        T* data() noexcept {
            return reinterpret_cast<T*>(storage);
        }
        
        const T* data() const noexcept {
            return reinterpret_cast<const T*>(storage);
        }
    };

    using slot_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<slot>;
    using value_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;

    // Core data members with cache line alignment to prevent false sharing
    detail::cache_aligned<std::atomic<size_type>> head_{0};
    detail::cache_aligned<std::atomic<size_type>> tail_{0};
    
    std::unique_ptr<slot[], std::function<void(slot*)>> slots_;
    size_type capacity_;
    size_type mask_;  // For fast modulo with power-of-2 sizes
    
    slot_allocator slot_alloc_;
    value_allocator value_alloc_;
    
    // Optional statistics
    mutable std::unique_ptr<ring_buffer_stats> stats_;

    // Custom deleter for slots array
    struct slot_deleter {
        slot_allocator alloc;
        size_type count;
        
        void operator()(slot* ptr) const {
            if (ptr) {
                // Destroy any remaining constructed objects
                value_allocator val_alloc; // Create an lvalue instance
                for (size_type i = 0; i < count; ++i) {
                    if (ptr[i].ready.load(std::memory_order_relaxed)) {
                        std::allocator_traits<value_allocator>::destroy(
                            val_alloc, ptr[i].data()); // Pass the lvalue
                    }
                }
                
                // Deallocate slots
                std::allocator_traits<slot_allocator>::deallocate(
                    const_cast<slot_allocator&>(alloc), ptr, count);
            }
        }
    };

public:
    // Constructor - capacity must be power of 2
    explicit ring_buffer(size_type capacity, 
                        const Allocator& alloc = Allocator{})
        : capacity_(capacity)
        , mask_(capacity - 1)
        , slot_alloc_(alloc)
        , value_alloc_(alloc)
    {
        static_assert(std::is_nothrow_destructible_v<T>, 
                     "T must be nothrow destructible for lock-free operation");
        
        if (!detail::is_power_of_two(capacity) || capacity == 0) {
            throw std::invalid_argument("Capacity must be a power of 2 and greater than 0");
        }
        
        if (capacity > (std::numeric_limits<size_type>::max() / 2)) {
            throw std::invalid_argument("Capacity too large");
        }
        
        // Allocate slots
        auto* raw_slots = std::allocator_traits<slot_allocator>::allocate(slot_alloc_, capacity);
        
        try {
            // Initialize slots
            for (size_type i = 0; i < capacity; ++i) {
                new (raw_slots + i) slot{};
            }
            
            slots_ = std::unique_ptr<slot[], std::function<void(slot*)>>(
                raw_slots, slot_deleter{slot_alloc_, capacity});
            
        } catch (...) {
            std::allocator_traits<slot_allocator>::deallocate(slot_alloc_, raw_slots, capacity);
            throw;
        }
    }

    // Move constructor
    ring_buffer(ring_buffer&& other) noexcept
        : head_(other.head_.get().load(std::memory_order_relaxed))
        , tail_(other.tail_.get().load(std::memory_order_relaxed))
        , slots_(std::move(other.slots_))
        , capacity_(other.capacity_)
        , mask_(other.mask_)
        , slot_alloc_(std::move(other.slot_alloc_))
        , value_alloc_(std::move(other.value_alloc_))
        , stats_(std::move(other.stats_))
    {
        other.capacity_ = 0;
        other.mask_ = 0;
    }

    // No copy constructor - lock-free structures are typically move-only
    ring_buffer(const ring_buffer&) = delete;
    ring_buffer& operator=(const ring_buffer&) = delete;
    ring_buffer& operator=(ring_buffer&&) = delete;

    ~ring_buffer() = default;

    // Push operations (producer side)
    
    // Try to push an element (non-blocking)
    template<typename U>
    bool try_push(U&& item) noexcept(std::is_nothrow_constructible_v<T, U&&>) {
        const auto current_tail = tail_.get().load(load_order);
        const auto next_tail = (current_tail + 1) & mask_;
        
        // Check if buffer is full
        if (next_tail == head_.get().load(load_order)) {
            if (stats_) {
                stats_->failed_pushes.fetch_add(1, std::memory_order_relaxed);
            }
            return false;
        }
        
        auto& slot = slots_[current_tail];
        
        // Construct the object in place
        try {
            std::allocator_traits<value_allocator>::construct(
                value_alloc_, slot.data(), std::forward<U>(item));
        } catch (...) {
            if (stats_) {
                stats_->failed_pushes.fetch_add(1, std::memory_order_relaxed);
            }
            return false;
        }
        
        // Mark slot as ready and advance tail
        slot.ready.store(true, store_order);
        tail_.get().store(next_tail, store_order);
        
        if (stats_) {
            stats_->total_pushes.fetch_add(1, std::memory_order_relaxed);
        }
        
        return true;
    }
    
    // Emplace push
    template<typename... Args>
    bool try_emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
        const auto current_tail = tail_.get().load(load_order);
        const auto next_tail = (current_tail + 1) & mask_;
        
        if (next_tail == head_.get().load(load_order)) {
            if (stats_) {
                stats_->failed_pushes.fetch_add(1, std::memory_order_relaxed);
            }
            return false;
        }
        
        auto& slot = slots_[current_tail];
        
        try {
            std::allocator_traits<value_allocator>::construct(
                value_alloc_, slot.data(), std::forward<Args>(args)...);
        } catch (...) {
            if (stats_) {
                stats_->failed_pushes.fetch_add(1, std::memory_order_relaxed);
            }
            return false;
        }
        
        slot.ready.store(true, store_order);
        tail_.get().store(next_tail, store_order);
        
        if (stats_) {
            stats_->total_pushes.fetch_add(1, std::memory_order_relaxed);
        }
        
        return true;
    }
    
    // Blocking push with timeout
    template<typename U, typename Rep, typename Period>
    bool push_for(U&& item, const std::chrono::duration<Rep, Period>& timeout) {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        
        while (std::chrono::steady_clock::now() < deadline) {
            if (try_push(std::forward<U>(item))) {
                return true;
            }
            
            // Brief yield to prevent excessive spinning
            std::this_thread::yield();
        }
        
        return false;
    }
    
    // Blocking push (busy-wait)
    template<typename U>
    void push(U&& item) {
        while (!try_push(std::forward<U>(item))) {
            std::this_thread::yield();
        }
    }

    // Pop operations (consumer side)
    
    // Try to pop an element (non-blocking)
    std::optional<T> try_pop() {
        const auto current_head = head_.get().load(load_order);
        
        if (current_head == tail_.get().load(load_order)) {
            if (stats_) {
                stats_->failed_pops.fetch_add(1, std::memory_order_relaxed);
            }
            return std::nullopt;
        }
        
        auto& slot = slots_[current_head];
        
        // Wait for slot to be ready (should be very brief)
        while (!slot.ready.load(load_order)) {
            std::this_thread::yield();
        }
        
        // Move/copy the value out
        std::optional<T> result;
        if constexpr (std::is_nothrow_move_constructible_v<T>) {
            result = std::move(*slot.data());
        } else {
            result = *slot.data();
        }
        
        // Destroy the object and mark slot as not ready
        std::allocator_traits<value_allocator>::destroy(value_alloc_, slot.data());
        slot.ready.store(false, store_order);
        
        // Advance head
        head_.get().store((current_head + 1) & mask_, store_order);
        
        if (stats_) {
            stats_->total_pops.fetch_add(1, std::memory_order_relaxed);
        }
        
        return result;
    }
    
    // Try to pop into existing object (avoids optional overhead)
    bool try_pop_into(T& item) {
        const auto current_head = head_.get().load(load_order);
        
        if (current_head == tail_.get().load(load_order)) {
            if (stats_) {
                stats_->failed_pops.fetch_add(1, std::memory_order_relaxed);
            }
            return false;
        }
        
        auto& slot = slots_[current_head];
        
        while (!slot.ready.load(load_order)) {
            std::this_thread::yield();
        }
        
        // Assign to existing object
        if constexpr (std::is_nothrow_move_assignable_v<T>) {
            item = std::move(*slot.data());
        } else {
            item = *slot.data();
        }
        
        std::allocator_traits<value_allocator>::destroy(value_alloc_, slot.data());
        slot.ready.store(false, store_order);
        head_.get().store((current_head + 1) & mask_, store_order);
        
        if (stats_) {
            stats_->total_pops.fetch_add(1, std::memory_order_relaxed);
        }
        
        return true;
    }
    
    // Blocking pop with timeout
    template<typename Rep, typename Period>
    std::optional<T> pop_for(const std::chrono::duration<Rep, Period>& timeout) {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        
        while (std::chrono::steady_clock::now() < deadline) {
            if (auto result = try_pop()) {
                return result;
            }
            std::this_thread::yield();
        }
        
        return std::nullopt;
    }
    
    // Blocking pop (busy-wait)
    T pop() {
        while (true) {
            if (auto result = try_pop()) {
                return std::move(*result);
            }
            std::this_thread::yield();
        }
    }

    // Capacity and size information
    size_type capacity() const noexcept { return capacity_; }
    
    size_type size() const noexcept {
        const auto tail = tail_.get().load(load_order);
        const auto head = head_.get().load(load_order);
        return (tail - head) & mask_;
    }
    
    bool empty() const noexcept {
        return head_.get().load(load_order) == tail_.get().load(load_order);
    }
    
    bool full() const noexcept {
        const auto tail = tail_.get().load(load_order);
        const auto head = head_.get().load(load_order);
        return ((tail + 1) & mask_) == head;
    }
    
    // Approximate utilization (0.0 to 1.0)
    double utilization() const noexcept {
        return static_cast<double>(size()) / capacity();
    }

    // Statistics management
    void enable_stats() {
        if (!stats_) {
            stats_ = std::make_unique<ring_buffer_stats>();
        }
    }
    
    void disable_stats() {
        stats_.reset();
    }
    
    const ring_buffer_stats* get_stats() const noexcept {
        return stats_.get();
    }
    
    void reset_stats() {
        if (stats_) {
            stats_->reset();
        }
    }

    // Advanced operations
    
    // Peek at front element without popping
    template<typename F>
    bool peek(F&& func) const {
        const auto current_head = head_.get().load(load_order);
        
        if (current_head == tail_.get().load(load_order)) {
            return false;
        }
        
        const auto& slot = slots_[current_head];
        
        if (!slot.ready.load(load_order)) {
            return false;
        }
        
        func(*slot.data());
        return true;
    }
    
    // Clear all elements (not thread-safe, use only when no concurrent access)
    void clear() {
        while (!empty()) {
            try_pop();
        }
    }
    
    // Get allocator
    allocator_type get_allocator() const {
        return allocator_type(value_alloc_);
    }
};

// Factory functions for common use cases

template<typename T, memory_ordering Ordering = memory_ordering::acquire_release>
auto make_ring_buffer(std::size_t capacity) {
    return ring_buffer<T, Ordering>(capacity);
}

template<typename T>
auto make_relaxed_ring_buffer(std::size_t capacity) {
    return ring_buffer<T, memory_ordering::relaxed>(capacity);
}

template<typename T>
auto make_sequential_ring_buffer(std::size_t capacity) {
    return ring_buffer<T, memory_ordering::sequential>(capacity);
}

// Utility function to find next power of 2
constexpr std::size_t next_power_of_two(std::size_t n) {
    if (n <= 1) return 1;
    return std::size_t{1} << (std::bit_width(n - 1));
}

} // namespace concurrent


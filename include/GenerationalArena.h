#ifndef GENERATIONAL_ARENA_H
#define GENERATIONAL_ARENA_H

#include <cstdint>
#include <vector>
#include <memory> // For std::allocator, std::allocator_traits, std::destroy_at
#include <utility>  // For std::forward, std::move
#include <stdexcept> // For std::out_of_range (potentially)
#include <limits>   // For std::numeric_limits
#include <type_traits> // For std::aligned_storage_t, std::is_trivially_destructible
#include <functional> // For std::hash

namespace cpp_collections {

// Forward declaration of GenerationalArena for Handle's friend declaration if needed,
// though Handle is typically public or part of GenerationalArena's public interface.
template <typename T>
class GenerationalArena;

struct ArenaHandle {
    uint32_t index;
    uint32_t generation;

    // Default constructor for invalid/null handle
    ArenaHandle() : index(std::numeric_limits<uint32_t>::max()), generation(0) {}

    ArenaHandle(uint32_t idx, uint32_t gen) : index(idx), generation(gen) {}

    bool operator==(const ArenaHandle& other) const {
        return index == other.index && generation == other.generation;
    }

    bool operator!=(const ArenaHandle& other) const {
        return !(*this == other);
    }

    // A convenience method to check if the handle is likely null/invalid
    // based on the default constructed values.
    // Note: A generation of 0 might be valid for the first allocation in a slot.
    // A truly robust "is_null" would depend on GenerationalArena's specific invalid pattern.
    bool is_null() const {
        return index == std::numeric_limits<uint32_t>::max();
    }

    // Static factory for a null/invalid handle
    static ArenaHandle null() {
        return ArenaHandle();
    }
};

} // namespace cpp_collections

namespace std {
template <>
struct hash<cpp_collections::ArenaHandle> {
    std::size_t operator()(const cpp_collections::ArenaHandle& handle) const noexcept {
        // Combine hashes of index and generation.
        // A common way to combine two hashes:
        std::size_t h1 = std::hash<uint32_t>{}(handle.index);
        std::size_t h2 = std::hash<uint32_t>{}(handle.generation);
        return h1 ^ (h2 << 1); // Or use Boost.hash_combine logic if available/desired
    }
};
} // namespace std

namespace cpp_collections {

template <typename T>
class GenerationalArena {
public:
    using value_type = T;
    using handle_type = ArenaHandle;

private:
    struct Slot {
        // Storage for the object. Using aligned_storage to avoid constructing T
        // until an object is actually allocated in this slot.
        std::aligned_storage_t<sizeof(T), alignof(T)> storage;
        uint32_t generation;
        bool is_active; // True if this slot currently holds a live object

        Slot() : generation(0), is_active(false) {} // Initial generation is 0

        // Delete copy operations
        Slot(const Slot&) = delete;
        Slot& operator=(const Slot&) = delete;

        // Move constructor
        Slot(Slot&& other) noexcept
            : generation(other.generation), is_active(other.is_active) {
            if (is_active) { // If other was active (and now this is marked active)
                new (std::addressof(storage)) T(std::move(*reinterpret_cast<T*>(std::addressof(other.storage))));
                // Do NOT call destructor on T in other.storage after move.
                // Mark other as inactive so its ~Slot() doesn't try to destruct the moved-from T.
                other.is_active = false;
            }
        }

        // Move assignment operator
        Slot& operator=(Slot&& other) noexcept {
            if (this != &other) {
                // Destroy current object if active
                if (is_active) {
                    if constexpr (!std::is_trivially_destructible_v<T>) {
                        reinterpret_cast<T*>(std::addressof(storage))->~T();
                    }
                    is_active = false; // Explicitly mark inactive before reassignment
                }

                // Move data from other
                generation = other.generation;
                is_active = other.is_active; // is_active of *this* slot is now what other's was

                if (is_active) { // If other was active (and now this is marked active)
                    new (std::addressof(storage)) T(std::move(*reinterpret_cast<T*>(std::addressof(other.storage))));
                    // Do NOT call destructor on T in other.storage after move.
                    // Mark other as inactive so its ~Slot() doesn't try to destruct the moved-from T.
                    other.is_active = false;
                }
            }
            return *this;
        }

        // Destructor
        ~Slot() {
            if (is_active) {
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    reinterpret_cast<T*>(std::addressof(storage))->~T();
                }
                // is_active = false; // Not strictly necessary as the Slot object is being destroyed
            }
        }
    };

    std::vector<Slot> slots_;
    std::vector<uint32_t> free_indices_; // Stores indices of slots that are currently free
    size_t active_count_; // Number of active objects

public:
    // Default constructor
    GenerationalArena() : active_count_(0) {}

    // Pre-allocating constructor
    explicit GenerationalArena(size_t initial_capacity) : active_count_(0) {
        slots_.reserve(initial_capacity);
        // free_indices_ could also be reserved if a pattern of growth is known
    }

    // Destructor: must manually call destructors for active objects
    ~GenerationalArena() {
        clear(); // clear() will handle destroying objects
    }

    // Copy constructor (deleted for now, can be complex due to raw storage and handles)
    GenerationalArena(const GenerationalArena&) = delete;
    // Copy assignment (deleted for now)
    GenerationalArena& operator=(const GenerationalArena&) = delete;

    // Move constructor
    GenerationalArena(GenerationalArena&& other) noexcept
        : slots_(std::move(other.slots_)),
          free_indices_(std::move(other.free_indices_)),
          active_count_(other.active_count_) {
        other.active_count_ = 0; // Source is now empty
    }

    // Move assignment operator
    GenerationalArena& operator=(GenerationalArena&& other) noexcept {
        if (this != &other) {
            // Destroy existing objects in this arena before overwriting
            clear();

            slots_ = std::move(other.slots_);
            free_indices_ = std::move(other.free_indices_);
            active_count_ = other.active_count_;

            other.active_count_ = 0;
        }
        return *this;
    }

    // Core Methods

    template <typename... Args>
    handle_type allocate(Args&&... args) {
        uint32_t slot_idx;
        if (!free_indices_.empty()) {
            slot_idx = free_indices_.back();
            free_indices_.pop_back();
            // Slot's generation is already set from its last deallocation (or initial 0)
        } else {
            slot_idx = static_cast<uint32_t>(slots_.size());
            // Ensure generation is 0 for brand new slots if not default constructed that way.
            // Our Slot constructor already sets generation to 0.
            slots_.emplace_back(); // Creates a new Slot
        }

        Slot& slot = slots_[slot_idx];
        // Use placement new to construct T in the slot's storage
        new (&slot.storage) T(std::forward<Args>(args)...);
        slot.is_active = true;
        // The slot's current generation value is used for the handle.
        // If it's a brand new slot, generation is 0.
        // If it's a reused slot, generation was incremented upon its last deallocation.

        active_count_++;
        return handle_type(slot_idx, slot.generation);
    }

    void deallocate(handle_type handle) {
        if (!is_valid(handle)) {
            // Optional: throw an exception or handle error, e.g. std::invalid_argument("Invalid handle for deallocation");
            // For now, silently return if handle is invalid, common in some arena designs.
            return;
        }

        uint32_t slot_idx = handle.index;
        Slot& slot = slots_[slot_idx];

        // Call destructor for T
        if constexpr (!std::is_trivially_destructible_v<T>) {
            reinterpret_cast<T*>(&slot.storage)->~T();
        }
        slot.is_active = false;

        // Increment generation. Handle potential overflow, though unlikely with uint32_t.
        // If generation reaches max, it could wrap around. A common strategy is to
        // reserve a specific generation value (e.g., 0 or max) as strictly invalid
        // or handle wrap-around explicitly if it's a concern for very long-lived arenas
        // with extreme churn. For typical use, simple increment is fine.
        if (slot.generation == std::numeric_limits<uint32_t>::max()) {
            slot.generation = 0; // Wrap around, or choose another strategy
        } else {
            slot.generation++;
        }

        free_indices_.push_back(slot_idx);
        active_count_--;
    }

    T* get(handle_type handle) {
        if (!is_valid(handle)) {
            return nullptr;
        }
        // If valid, slot must be active and generations match.
        return reinterpret_cast<T*>(&slots_[handle.index].storage);
    }

    const T* get(handle_type handle) const {
        if (!is_valid(handle)) {
            return nullptr;
        }
        return reinterpret_cast<const T*>(&slots_[handle.index].storage);
    }

    bool is_valid(handle_type handle) const {
        // Check if index is within bounds
        if (handle.index >= slots_.size() || handle.is_null()) { // Also check for default invalid handle
            return false;
        }
        const Slot& slot = slots_[handle.index];
        // Check if slot is active and generation matches
        return slot.is_active && slot.generation == handle.generation;
    }

    // Utility Methods

    size_t size() const noexcept {
        return active_count_;
    }

    bool empty() const noexcept {
        return active_count_ == 0;
    }

    size_t capacity() const noexcept {
        return slots_.size(); // Total number of slots (active or free)
    }

    void reserve(size_t new_capacity) {
        slots_.reserve(new_capacity);
        // Optionally, reserve for free_indices_ if a growth pattern is known,
        // e.g., free_indices_.reserve(new_capacity); but this might be an overestimate.
    }

    void clear() {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (uint32_t i = 0; i < slots_.size(); ++i) {
                if (slots_[i].is_active) {
                    reinterpret_cast<T*>(&slots_[i].storage)->~T();
                    slots_[i].is_active = false; // Mark as inactive after destruction
                }
            }
        }
        // Reset all slots to a default state if needed, or just clear vectors.
        // Clearing vectors is simpler and effectively invalidates all handles.
        slots_.clear();
        free_indices_.clear();
        active_count_ = 0;
        // After clear, all previous handles are invalid.
        // Generations are effectively reset as slots will be new or default constructed.
    }

    // Iterators
private:
    template <bool IsConst>
    class ArenaIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T; // Iterate over T objects
        using pointer           = std::conditional_t<IsConst, const T*, T*>;
        using reference         = std::conditional_t<IsConst, const T&, T&>;

        using ArenaPtrType      = std::conditional_t<IsConst, const GenerationalArena*, GenerationalArena*>;
        using SlotPtrType       = std::conditional_t<IsConst, const Slot*, Slot*>;

    private:
        ArenaPtrType arena_ptr_ = nullptr;
        uint32_t current_index_ = 0;

        void advance_to_valid() {
            if (!arena_ptr_) return;
            while (current_index_ < arena_ptr_->slots_.size() &&
                   !arena_ptr_->slots_[current_index_].is_active) {
                current_index_++;
            }
            if (current_index_ >= arena_ptr_->slots_.size()) {
                // Reached end or arena became empty while advancing
                arena_ptr_ = nullptr; // Mark as end iterator
                current_index_ = 0; // Reset index for consistency with default end iterator
            }
        }

    public:
        ArenaIterator() = default; // End iterator

        ArenaIterator(ArenaPtrType arena, uint32_t start_index)
            : arena_ptr_(arena), current_index_(start_index) {
            advance_to_valid();
        }

        // Allow conversion from non-const to const iterator
        template <bool WasConst = IsConst, typename = std::enable_if_t<IsConst && !WasConst>>
        ArenaIterator(const ArenaIterator<false>& other)
            : arena_ptr_(other.arena_ptr_), current_index_(other.current_index_) {}


        reference operator*() const {
            // Assuming advance_to_valid() ensures current_index_ points to an active slot
            return *reinterpret_cast<pointer>(&arena_ptr_->slots_[current_index_].storage);
        }

        pointer operator->() const {
            return reinterpret_cast<pointer>(&arena_ptr_->slots_[current_index_].storage);
        }

        ArenaIterator& operator++() {
            if (arena_ptr_) { // Only advance if not already an end iterator
                current_index_++;
                advance_to_valid();
            }
            return *this;
        }

        ArenaIterator operator++(int) {
            ArenaIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const ArenaIterator& a, const ArenaIterator& b) {
            // Both are end iterators if arena_ptr_ is null
            if (!a.arena_ptr_ && !b.arena_ptr_) return true;
            // If one is end and other is not, they are not equal
            if (!a.arena_ptr_ || !b.arena_ptr_) return false;
            // Both are valid, compare arena instance and index
            return a.arena_ptr_ == b.arena_ptr_ && a.current_index_ == b.current_index_;
        }

        friend bool operator!=(const ArenaIterator& a, const ArenaIterator& b) {
            return !(a == b);
        }

        // Friend declaration to allow non-const to const conversion (used in constructor)
        template<bool OtherIsConst>
        friend class ArenaIterator;
    };

public:
    using iterator = ArenaIterator<false>;
    using const_iterator = ArenaIterator<true>;

    iterator begin() {
        return iterator(this, 0);
    }

    iterator end() {
        return iterator(); // Default constructed is end iterator
    }

    const_iterator begin() const {
        return const_iterator(this, 0);
    }

    const_iterator end() const {
        return const_iterator(); // Default constructed is end iterator
    }

    const_iterator cbegin() const {
        return begin();
    }

    const_iterator cend() const {
        return end();
    }

};

} // namespace cpp_collections

#endif // GENERATIONAL_ARENA_H

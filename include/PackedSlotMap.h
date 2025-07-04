#ifndef CPP_COLLECTIONS_PACKED_SLOT_MAP_H
#define CPP_COLLECTIONS_PACKED_SLOT_MAP_H

#include <vector>
#include <cstdint>
#include <limits>
#include <stdexcept> // For std::out_of_range (potentially)
#include <utility>   // For std::move, std::swap

// Forward declaration for the iterator
template <typename T, bool IsConst>
class PackedSlotMapIterator;

template <typename T>
class PackedSlotMap {
public:
    struct Key {
        uint32_t slot_idx;
        uint32_t generation;

        bool operator==(const Key& other) const = default;
        bool operator!=(const Key& other) const = default;
        // Provide a hash for Key to allow its use in unordered_map etc.
        struct Hasher {
            std::size_t operator()(const Key& k) const {
                std::size_t h1 = std::hash<uint32_t>{}(k.slot_idx);
                std::size_t h2 = std::hash<uint32_t>{}(k.generation);
                return h1 ^ (h2 << 1); // Combine hashes
            }
        };
    };

private:
    friend class PackedSlotMapIterator<T, false>;
    friend class PackedSlotMapIterator<T, true>;

    struct SlotEntry {
        uint32_t generation{0};
        uint32_t data_idx{0}; // Index into the data_ vector
        bool is_active{false}; // Indicates if this slot is currently active (points to valid data)
    };

    struct DataEntry {
        T value;
        uint32_t slot_idx; // Back-pointer to the slot in slots_

        // Constructor to create DataEntry by moving or copying value
        template <typename U>
        DataEntry(U&& val, uint32_t s_idx) : value(std::forward<U>(val)), slot_idx(s_idx) {}
    };

    std::vector<SlotEntry> slots_;
    std::vector<DataEntry> data_; // Stores active elements contiguously
    std::vector<uint32_t> free_slot_indices_; // Stores indices of free slots in slots_

    static constexpr uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();

public:
    // Types
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = std::size_t;
    using key_type = Key;

    // Iterators
    using iterator = PackedSlotMapIterator<T, false>;
    using const_iterator = PackedSlotMapIterator<T, true>;

    PackedSlotMap() = default;

    Key insert(const T& value) {
        return emplace(value);
    }

    Key insert(T&& value) {
        return emplace(std::move(value));
    }

    template <typename... Args>
    Key emplace(Args&&... args) {
        uint32_t current_slot_idx;
        if (!free_slot_indices_.empty()) {
            current_slot_idx = free_slot_indices_.back();
            free_slot_indices_.pop_back();
            // Generation was incremented when slot was freed.
        } else {
            current_slot_idx = static_cast<uint32_t>(slots_.size());
            slots_.emplace_back(); // Default SlotEntry: gen 0, data_idx 0, is_active false
                                   // Generation will be set/confirmed below.
        }

        uint32_t current_data_idx = static_cast<uint32_t>(data_.size());
        data_.emplace_back(T(std::forward<Args>(args)...), current_slot_idx);

        slots_[current_slot_idx].data_idx = current_data_idx;
        slots_[current_slot_idx].is_active = true;
        // If this slot was reused, its generation is already set.
        // If it's a new slot, generation is 0 by default.

        return Key{current_slot_idx, slots_[current_slot_idx].generation};
    }

    bool erase(Key key) {
        if (key.slot_idx >= slots_.size() ||
            !slots_[key.slot_idx].is_active ||
            slots_[key.slot_idx].generation != key.generation) {
            return false; // Invalid key or slot not active / generation mismatch
        }

        uint32_t slot_idx_to_erase = key.slot_idx;
        uint32_t data_idx_to_remove = slots_[slot_idx_to_erase].data_idx;

        // Invalidate the slot
        slots_[slot_idx_to_erase].is_active = false;
        if (slots_[slot_idx_to_erase].generation == std::numeric_limits<uint32_t>::max()) {
            slots_[slot_idx_to_erase].generation = 0; // Wrap around
        } else {
            slots_[slot_idx_to_erase].generation++;
        }
        free_slot_indices_.push_back(slot_idx_to_erase);

        // If the element to remove is not the last one in data_
        if (data_idx_to_remove < data_.size() - 1) {
            // Move the last element into the position of the removed element
            DataEntry& last_data_entry = data_.back();
            data_[data_idx_to_remove] = std::move(last_data_entry);

            // Update the slot entry of the moved element to point to its new data_idx
            slots_[last_data_entry.slot_idx].data_idx = data_idx_to_remove;
        }

        data_.pop_back(); // Remove the last element (either the one to erase or the one that was moved)
        return true;
    }

    T* get(Key key) {
        if (key.slot_idx >= slots_.size() ||
            !slots_[key.slot_idx].is_active ||
            slots_[key.slot_idx].generation != key.generation) {
            return nullptr;
        }
        return &data_[slots_[key.slot_idx].data_idx].value;
    }

    const T* get(Key key) const {
        if (key.slot_idx >= slots_.size() ||
            !slots_[key.slot_idx].is_active ||
            slots_[key.slot_idx].generation != key.generation) {
            return nullptr;
        }
        return &data_[slots_[key.slot_idx].data_idx].value;
    }

    bool contains(Key key) const {
        return key.slot_idx < slots_.size() &&
               slots_[key.slot_idx].is_active &&
               slots_[key.slot_idx].generation == key.generation;
    }

    size_type size() const noexcept {
        return data_.size();
    }

    bool empty() const noexcept {
        return data_.empty();
    }

    size_type capacity() const noexcept {
        return data_.capacity();
    }

    size_type slot_capacity() const noexcept {
        return slots_.capacity();
    }

    void clear() {
        data_.clear();
        // For slots, mark all as inactive and increment generation, then add to free list
        // Or simply clear and let them be re-added. Simpler:
        slots_.clear();
        free_slot_indices_.clear();
        // Note: This invalidates all existing keys completely.
        // A more nuanced clear might keep slot generations and add all to free_list.
        // For now, this is fine.
    }

    void reserve(size_type new_cap) {
        data_.reserve(new_cap);
        // slots_ also needs reservation if we expect a certain number of total slots (active + free)
        // For now, only reserve data_ as it's the primary storage.
    }

    iterator begin() {
        return iterator(data_.data());
    }

    iterator end() {
        return iterator(data_.data() + data_.size());
    }

    const_iterator begin() const {
        return const_iterator(data_.data());
    }

    const_iterator end() const {
        return const_iterator(data_.data() + data_.size());
    }

    const_iterator cbegin() const {
        return begin();
    }

    const_iterator cend() const {
        return end();
    }

    // Direct access to raw data vector (e.g., for serialization or external iteration)
    // Be cautious with this as modifications can break PackedSlotMap invariants.
    const std::vector<DataEntry>& get_data_entries() const {
        return data_;
    }
};


// Iterator Implementation
template <typename T, bool IsConst>
class PackedSlotMapIterator {
public:
    using DataEntryType = typename PackedSlotMap<T>::DataEntry;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T; // Iterate over T, not DataEntry
    using difference_type = std::ptrdiff_t;
    using pointer = std::conditional_t<IsConst, const T*, T*>;
    using reference = std::conditional_t<IsConst, const T&, T&>;
    using internal_pointer = std::conditional_t<IsConst, const DataEntryType*, DataEntryType*>;

private:
    internal_pointer ptr_ = nullptr;

public:
    PackedSlotMapIterator() = default;
    explicit PackedSlotMapIterator(internal_pointer p) : ptr_(p) {}

    // Allow conversion from non-const to const iterator
    template <bool WasConst = IsConst, typename = std::enable_if_t<IsConst && !WasConst>>
    PackedSlotMapIterator(const PackedSlotMapIterator<T, false>& other) : ptr_(other.ptr_) {}


    reference operator*() const { return ptr_->value; }
    pointer operator->() const { return &(ptr_->value); }

    PackedSlotMapIterator& operator++() { ++ptr_; return *this; }
    PackedSlotMapIterator operator++(int) { PackedSlotMapIterator tmp = *this; ++ptr_; return tmp; }
    PackedSlotMapIterator& operator--() { --ptr_; return *this; }
    PackedSlotMapIterator operator--(int) { PackedSlotMapIterator tmp = *this; --ptr_; return tmp; }

    PackedSlotMapIterator& operator+=(difference_type n) { ptr_ += n; return *this; }
    PackedSlotMapIterator operator+(difference_type n) const { PackedSlotMapIterator tmp = *this; tmp += n; return tmp; }
    friend PackedSlotMapIterator operator+(difference_type n, const PackedSlotMapIterator& iter) { return iter + n; }

    PackedSlotMapIterator& operator-=(difference_type n) { ptr_ -= n; return *this; }
    PackedSlotMapIterator operator-(difference_type n) const { PackedSlotMapIterator tmp = *this; tmp -= n; return tmp; }
    difference_type operator-(const PackedSlotMapIterator& other) const { return ptr_ - other.ptr_; }

    reference operator[](difference_type n) const { return (ptr_ + n)->value; }

    bool operator==(const PackedSlotMapIterator& other) const { return ptr_ == other.ptr_; }
    bool operator!=(const PackedSlotMapIterator& other) const { return ptr_ != other.ptr_; }
    bool operator<(const PackedSlotMapIterator& other) const { return ptr_ < other.ptr_; }
    bool operator>(const PackedSlotMapIterator& other) const { return ptr_ > other.ptr_; }
    bool operator<=(const PackedSlotMapIterator& other) const { return ptr_ <= other.ptr_; }
    bool operator>=(const PackedSlotMapIterator& other) const { return ptr_ >= other.ptr_; }

    // Expose ptr_ for conversion from non-const to const iterator
    // template <bool OtherIsConst> // This friend declaration was incorrect here.
    // friend class PackedSlotMapIterator; // An iterator class does not friend itself.
};


#endif // CPP_COLLECTIONS_PACKED_SLOT_MAP_H

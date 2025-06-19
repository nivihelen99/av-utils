#ifndef UTILS_SLOTMAP_OLD_H
#define UTILS_SLOTMAP_OLD_H

#include <vector>
#include <cstdint>
#include <limits> // Required for std::numeric_limits

namespace utils { // Assuming a namespace based on other files in the repo, adjust if necessary

template <typename T>
class SlotMap {
public:
    struct Key {
        uint32_t index;
        uint32_t generation;

        bool operator==(const Key& other) const = default;
        // It's good practice to also provide operator!= if operator== is provided.
        bool operator!=(const Key& other) const = default;
    };

    // Forward declare methods to be implemented later
    Key insert(T value);
    bool erase(Key key);
    T* get(Key key);
    const T* get(Key key) const;
    bool contains(Key key) const;
    size_t size() const;
    bool empty() const;

private:
    std::vector<T> data_; // Changed to data_ to avoid conflict with potential member functions named data
    std::vector<uint32_t> generations_;
    std::vector<size_t> free_list_;

    // Helper to check if a key is potentially valid (index in bounds)
    bool is_valid_index(uint32_t index) const {
        return index < data_.size();
    }
};

// Implementation of SlotMap methods

template <typename T>
typename SlotMap<T>::Key SlotMap<T>::insert(T value) {
    uint32_t index;
    if (!free_list_.empty()) {
        index = free_list_.back();
        free_list_.pop_back();
        data_[index] = std::move(value);
        // The generation for this slot was already incremented in erase().
        // We just use it as is.
        // generations_[index] already holds the correct new generation.
    } else {
        index = data_.size();
        data_.push_back(std::move(value));
        generations_.push_back(0); // Initial generation is 0 for a brand new slot
    }
    return Key{index, generations_[index]};
}

template <typename T>
bool SlotMap<T>::erase(Key key) {
    if (!is_valid_index(key.index) || generations_[key.index] != key.generation) {
        return false; // Key is invalid or generation mismatch
    }

    // Add index to free list
    free_list_.push_back(key.index);

    // Increment generation to invalidate old keys
    // Check for wrapping around, though highly unlikely with uint32_t.
    if (generations_[key.index] == std::numeric_limits<uint32_t>::max()) {
        generations_[key.index] = 0; // Or handle error/warning
    } else {
        generations_[key.index]++;
    }

    // Optional: Destruct or reset the object at data_[key.index]
    // if T is a complex type. For simple types, this might not be necessary.
    // For example: data_[key.index] = T{}; // Reset to default value
    // Or if T is a pointer, you might need to delete it: delete data_[key.index]; data_[key.index] = nullptr;

    return true;
}

template <typename T>
T* SlotMap<T>::get(Key key) {
    if (!is_valid_index(key.index) || generations_[key.index] != key.generation) {
        return nullptr; // Invalid key or generation mismatch
    }
    // Check if the slot is active (not in the free_list conceptually, though erase handles generation)
    // This check is implicitly handled by the generation check.
    // If an element was erased, its generation would have been incremented,
    // so a key with an old generation is invalid.
    return &data_[key.index];
}

template <typename T>
const T* SlotMap<T>::get(Key key) const {
    if (!is_valid_index(key.index) || generations_[key.index] != key.generation) {
        return nullptr; // Invalid key or generation mismatch
    }
    return &data_[key.index];
}

template <typename T>
bool SlotMap<T>::contains(Key key) const {
    return is_valid_index(key.index) && generations_[key.index] == key.generation;
}

template <typename T>
size_t SlotMap<T>::size() const {
    return data_.size() - free_list_.size();
}

template <typename T>
bool SlotMap<T>::empty() const {
    return size() == 0;
}

} // namespace utils

#endif // UTILS_SLOTMAP_OLD_H

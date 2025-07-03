#ifndef VALUE_INDEX_MAP_H
#define VALUE_INDEX_MAP_H

#include <vector>
#include <unordered_map>
#include <optional>
#include <stdexcept> // For std::out_of_range (later for sealed exceptions)

// Forward declaration for custom hash/equality later
template<typename T, typename Hash, typename KeyEqual>
class ValueIndexMap;

// Default template arguments will be specified in the actual class definition
template<
    typename T,
    typename Hash = std::hash<T>,
    typename KeyEqual = std::equal_to<T>
>
class ValueIndexMap {
public:
    // Types
    using value_type = T;
    using size_type = size_t;
    using mapped_type = size_type;

    // Constructors
    ValueIndexMap() = default;

    // Copy constructor
    ValueIndexMap(const ValueIndexMap& other)
        : to_index_(other.to_index_), from_index_(other.from_index_), sealed_(other.sealed_) {}

    // Move constructor
    ValueIndexMap(ValueIndexMap&& other) noexcept
        : to_index_(std::move(other.to_index_)),
          from_index_(std::move(other.from_index_)),
          sealed_(other.sealed_) {
        other.sealed_ = false; // Reset moved-from object state
    }

    // Copy assignment
    ValueIndexMap& operator=(const ValueIndexMap& other) {
        if (this != &other) {
            to_index_ = other.to_index_;
            from_index_ = other.from_index_;
            sealed_ = other.sealed_;
        }
        return *this;
    }

    // Move assignment
    ValueIndexMap& operator=(ValueIndexMap&& other) noexcept {
        if (this != &other) {
            // If 'this' is sealed, this operation effectively replaces its state.
            // The sealed status of 'this' will become that of 'other'.
            to_index_ = std::move(other.to_index_);
            from_index_ = std::move(other.from_index_);
            sealed_ = other.sealed_; // Adopt sealed state from source

            // Clear other to a valid, default-constructed state
            // No need to check other.sealed_ before clearing, as it's being gutted.
            other.to_index_.clear();
            other.from_index_.clear();
            other.sealed_ = false; // Moved-from object is no longer sealed and is empty.
        }
        return *this;
    }

    // Serialization: Returns a const reference to the internal vector of values.
    // This vector alone is sufficient to reconstruct the map.
    const std::vector<T>& get_values_for_serialization() const {
        return from_index_;
    }

    // Deserialization: Constructor from a vector of values (copying version)
    // Assumes values are unique and their order defines their index.
    explicit ValueIndexMap(const std::vector<T>& values)
        : sealed_(false) // A deserialized map is not sealed by default
    {
        from_index_.reserve(values.size());
        to_index_.reserve(values.size());
        for (const auto& value : values) {
            size_type new_index = from_index_.size();
            from_index_.push_back(value);
            // Check for duplicates during deserialization
            if (!to_index_.emplace(from_index_.back(), new_index).second) {
                // Rollback partial construction and throw
                from_index_.clear();
                to_index_.clear();
                throw std::invalid_argument("Duplicate value found in input vector during deserialization. Values must be unique.");
            }
        }
    }

    // Deserialization: Constructor from a vector of values (moving version)
    // Assumes values are unique and their order defines their index.
    explicit ValueIndexMap(std::vector<T>&& values)
        : from_index_(std::move(values)), // Move the input vector
          sealed_(false) // A deserialized map is not sealed by default
    {
        to_index_.reserve(from_index_.size());
        for (size_type i = 0; i < from_index_.size(); ++i) {
            // Use the value already in from_index_ (which was moved)
            // Check for duplicates during deserialization
            if (!to_index_.emplace(from_index_[i], i).second) {
                // Rollback partial construction and throw
                // Note: from_index_ still holds the moved data, but map is inconsistent.
                // Clearing them ensures a clean state for the exception.
                from_index_.clear();
                to_index_.clear();
                throw std::invalid_argument("Duplicate value found in input vector during deserialization. Values must be unique.");
            }
        }
    }

    // Insert or retrieve index
    size_type insert(const T& value) {
        if (sealed_) {
            // Behavior for sealed map will be defined in step 2
            // For now, let's throw or handle error appropriately
            throw std::logic_error("Map is sealed, cannot insert new values.");
        }
        auto it = to_index_.find(value);
        if (it != to_index_.end()) {
            return it->second; // Value already exists, return its index
        }

        size_type new_index = from_index_.size();
        from_index_.push_back(value);
        to_index_[value] = new_index; // Or to_index_.emplace(value, new_index);
        return new_index;
    }

    // Overload for rvalue references to allow moving values into the map
    size_type insert(T&& value) {
        if (sealed_) {
            throw std::logic_error("Map is sealed, cannot insert new values.");
        }
        auto it = to_index_.find(value); // Find requires const T&, so value is fine
        if (it != to_index_.end()) {
            return it->second;
        }

        size_type new_index = from_index_.size();
        // Use emplace_back for potential move efficiency if T is movable
        from_index_.emplace_back(std::move(value));
        // After moving `value` into `from_index_`, we need to insert into `to_index_`
        // using the instance now stored in `from_index_` to ensure consistency
        // if `value` itself was a proxy or if its state changed upon move.
        // However, `unordered_map` keys are typically copied/moved on insertion.
        // The key for `to_index_` should be equivalent to what's in `from_index_`.
        // `from_index_.back()` gives access to the moved-in value.
        to_index_[from_index_.back()] = new_index;
        return new_index;
    }


    // Query if present
    std::optional<size_type> index_of(const T& value) const {
        auto it = to_index_.find(value);
        if (it != to_index_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    // Reverse lookup
    const T* value_at(size_type index) const {
        if (index < from_index_.size()) {
            return &from_index_[index];
        }
        return nullptr; // Out of range
    }

    // State queries
    bool contains(const T& value) const {
        return to_index_.count(value) > 0;
    }

    size_type size() const {
        return from_index_.size();
    }

    bool empty() const {
        return from_index_.empty();
    }

    void clear() {
        if (sealed_) {
            throw std::logic_error("Map is sealed, cannot clear.");
        }
        to_index_.clear();
        from_index_.clear();
        // Note: clearing does not unseal the map.
        // If unsealing is desired, `sealed_` should be set to false here.
        // Current behavior: a sealed map, if clearable (e.g. by relaxing the check), would remain sealed but empty.
        // As implemented, a sealed map cannot be cleared.
    }

    // Seal functionality
    void seal() {
        sealed_ = true;
    }

    bool is_sealed() const {
        return sealed_;
    }

    // Erase functionality
    // Returns true if an element was erased, false otherwise.
    // Note on index stability: Erasing an element causes the element previously
    // at the end of the map to move into the erased element's slot. The index of
    // this moved element changes. Indices of other elements remain stable.
    bool erase(const T& value) {
        if (sealed_) {
            throw std::logic_error("Map is sealed, cannot erase values.");
        }
        auto it = to_index_.find(value);
        if (it == to_index_.end()) {
            return false; // Value not found
        }
        size_type index_to_erase = it->second;
        return erase_at_index(index_to_erase); // Call helper
    }

    bool erase_at_index(size_type index) {
        if (sealed_) {
            throw std::logic_error("Map is sealed, cannot erase values.");
        }
        if (index >= from_index_.size()) {
            // Or throw std::out_of_range("Index out of bounds for erase_at_index");
            return false; // Index out of bounds
        }

        // Value to be removed from to_index_
        // Important: get a copy or reference BEFORE it's potentially overwritten in from_index_
        T value_to_remove = from_index_[index];

        if (index < from_index_.size() - 1) {
            // Not the last element, so swap with the last element
            T& last_value = from_index_.back(); // Reference to the last element
            from_index_[index] = std::move(last_value); // Move last element to the erased slot
            to_index_[from_index_[index]] = index; // Update index for the moved element
                                                 // (use from_index_[index] as key in case T is complex)
        }
        // Whether it was the last element or not, now pop the last one
        from_index_.pop_back();

        // Remove the original value (that was targeted for erasure) from to_index_
        size_t num_erased_from_map = to_index_.erase(value_to_remove);

        // num_erased_from_map should be 1 if logic is correct and value_to_remove was indeed in to_index_
        // If value_to_remove was somehow not in to_index_ but its index was valid,
        // that would indicate an inconsistent state. For robustness, one might assert(num_erased_from_map > 0).
        return num_erased_from_map > 0; // Or simply return true if we reached here successfully.
    }


    // Iterator Support
    // All iterators are const_iterators to prevent modification of values
    // in from_index_ which are used as keys in to_index_.
    using const_iterator = typename std::vector<T>::const_iterator;

    const_iterator begin() const {
        return from_index_.cbegin();
    }

    const_iterator end() const {
        return from_index_.cend();
    }

    const_iterator cbegin() const {
        return from_index_.cbegin();
    }

    const_iterator cend() const {
        return from_index_.cend();
    }

private:
    std::unordered_map<T, size_type, Hash, KeyEqual> to_index_;
    std::vector<T> from_index_;
    bool sealed_ = false;
};

#endif // VALUE_INDEX_MAP_H

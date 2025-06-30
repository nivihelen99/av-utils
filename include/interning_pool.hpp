#ifndef INTERNING_POOL_HPP
#define INTERNING_POOL_HPP

#include <unordered_set>
#include <string> // Default type for T, and for std::hash/equal_to specializations
#include <functional> // For std::hash, std::equal_to
#include <memory>     // For std::allocator
#include <utility>    // For std::move

namespace cpp_collections {

template <
    typename T,
    typename Hash = std::hash<T>,
    typename KeyEqual = std::equal_to<T>,
    typename Allocator = std::allocator<T>
>
class InterningPool {
public:
    using value_type = T;
    using handle_type = const T*;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using allocator_type = Allocator;
    using internal_set_type = std::unordered_set<T, Hash, KeyEqual, Allocator>;
    using size_type = typename internal_set_type::size_type;

private:
    internal_set_type S_set_;

public:
    // Constructors
    InterningPool() : S_set_() {}

    explicit InterningPool(const Allocator& alloc) : S_set_(alloc) {}

    InterningPool(size_type bucket_count,
                  const Hash& hash = Hash(),
                  const KeyEqual& equal = KeyEqual(),
                  const Allocator& alloc = Allocator())
        : S_set_(bucket_count, hash, equal, alloc) {}

    // No copy constructor/assignment, as copying an interning pool is tricky
    // regarding handle validity from the original. Typically, interning pools
    // are unique instances. If copying is needed, it implies a new set of handles.
    // For simplicity, make it non-copyable. Users can create a new pool and re-intern.
    InterningPool(const InterningPool&) = delete;
    InterningPool& operator=(const InterningPool&) = delete;

    // Move constructor/assignment
    InterningPool(InterningPool&& other) noexcept = default;
    InterningPool& operator=(InterningPool&& other) noexcept = default;

    /**
     * @brief Interns a value.
     * If the value is already in the pool, returns a handle to the existing object.
     * Otherwise, inserts a copy of the value into the pool and returns a handle to it.
     * @param value The value to intern.
     * @return A handle (const T*) to the interned object.
     */
    handle_type intern(const T& value) {
        auto it = S_set_.find(value);
        if (it != S_set_.end()) {
            return &(*it);
        }
        // Value not found, insert it. std::unordered_set::insert returns std::pair<iterator, bool>.
        auto insert_result = S_set_.insert(value);
        return &(*(insert_result.first)); // Pointer to the newly inserted element
    }

    /**
     * @brief Interns a value using move semantics.
     * If an equivalent value is already in the pool, returns a handle to the existing object.
     * The passed rvalue `value` might be discarded if an existing entry is found.
     * Otherwise, moves the value into the pool and returns a handle to it.
     * @param value The value to intern (rvalue reference).
     * @return A handle (const T*) to the interned object.
     */
    handle_type intern(T&& value) {
        // Need to check if the value (or an equivalent one) already exists.
        // `find` requires a const T& or equivalent.
        auto it = S_set_.find(value);
        if (it != S_set_.end()) {
            return &(*it);
        }
        // Value not found, insert it by moving.
        auto insert_result = S_set_.insert(std::move(value));
        return &(*(insert_result.first)); // Pointer to the newly inserted element
    }

    /**
     * @brief Checks if a value has already been interned.
     * @param value The value to check.
     * @return True if the value is in the pool, false otherwise.
     */
    bool contains(const T& value) const {
        return S_set_.count(value) > 0;
    }

    /**
     * @brief Returns the number of unique items stored in the pool.
     * @return The number of unique items.
     */
    size_type size() const noexcept {
        return S_set_.size();
    }

    /**
     * @brief Checks if the pool is empty.
     * @return True if the pool contains no items, false otherwise.
     */
    bool empty() const noexcept {
        return S_set_.empty();
    }

    /**
     * @brief Removes all items from the pool.
     * This invalidates all previously returned handles.
     */
    void clear() noexcept {
        S_set_.clear();
    }

    // Expose allocator
    allocator_type get_allocator() const {
        return S_set_.get_allocator();
    }

    // Expose hash function and key equal
    hasher hash_function() const {
        return S_set_.hash_function();
    }

    key_equal key_eq() const {
        return S_set_.key_eq();
    }
};

// Deduction guide for common case (e.g. from initializer list of strings)
// This is tricky for InterningPool as its constructors don't directly take ranges of values to intern.
// Users typically construct an empty pool and then call intern().
// So, standard deduction guides for containers might not apply directly or be as useful.

} // namespace cpp_collections

#endif // INTERNING_POOL_HPP

#pragma once

#include <unordered_map>
#include <any>
#include <typeindex>
#include <stdexcept> // For std::out_of_range
#include <memory>    // For std::shared_ptr or std::unique_ptr if we choose to store pointers

namespace cpp_utils {

/**
 * @brief A TypeMap is a heterogeneous container that maps types to instances of those types.
 *
 * It allows storing and retrieving objects of different types in a type-safe manner.
 * Each type can be stored at most once.
 *
 * This implementation uses std::type_index as the key and std::any to store the objects.
 */
class TypeMap {
private:
    std::unordered_map<std::type_index, std::any> map_;

public:
    TypeMap() = default;

    // Rule of 5/3: Defaulted is fine as std::unordered_map and std::any handle themselves.
    TypeMap(const TypeMap& other) = default;
    TypeMap(TypeMap&& other) noexcept = default;
    TypeMap& operator=(const TypeMap& other) = default;
    TypeMap& operator=(TypeMap&& other) noexcept = default;
    ~TypeMap() = default;

    /**
     * @brief Inserts or replaces an object of type T.
     *
     * @tparam T The type of the object to store.
     * @param value The object to store. It will be copied or moved into the map.
     * @return A reference to the inserted or updated object within the map.
     */
    template<typename T_param>
    std::decay_t<T_param>& put(T_param&& value) {
        using T = std::decay_t<T_param>;
        std::type_index type_idx = std::type_index(typeid(T));

        auto it = map_.find(type_idx);
        if (it != map_.end()) {
            // Key exists, update it by assigning a new std::any constructed with in_place_type
            it->second = std::any(std::in_place_type_t<T>{}, std::forward<T_param>(value));
            return std::any_cast<T&>(it->second);
        } else {
            // Key doesn't exist, emplace it
            // Emplace a new std::any constructed with in_place_type
            auto [new_it, success] = map_.emplace(type_idx, std::any(std::in_place_type_t<T>{}, std::forward<T_param>(value)));
            // Ensure emplace succeeded, which it should if find failed.
            // This assertion is mostly for sanity checking during development.
            if (!success) {
                 // This case should ideally not be reached if find() failed.
                 // If it does, it might indicate a concurrent modification or an unexpected map state.
                 // For robustness, one might re-query or handle, but for typical use, assert is okay.
                 throw std::runtime_error("TypeMap: emplace failed unexpectedly after find indicated absence.");
            }
            return std::any_cast<T&>(new_it->second);
        }
    }

    /**
     * @brief Retrieves a pointer to the stored object of type T.
     *
     * @tparam T The type of the object to retrieve.
     * @return A pointer to the object if found, otherwise nullptr.
     */
    template<typename T>
    T* get() {
        std::type_index type_idx = std::type_index(typeid(T));
        auto it = map_.find(type_idx);
        if (it != map_.end()) {
            // std::any_cast<T> returns a copy. We want a pointer to the stored object.
            // std::any_cast<T*> on an std::any storing T* would work if we stored T*.
            // If std::any stores T, then std::any_cast<T>(&it->second) gets a pointer to T.
            return std::any_cast<T>(&it->second);
        }
        return nullptr;
    }

    /**
     * @brief Retrieves a const pointer to the stored object of type T.
     *
     * @tparam T The type of the object to retrieve.
     * @return A const pointer to the object if found, otherwise nullptr.
     */
    template<typename T>
    const T* get() const {
        std::type_index type_idx = std::type_index(typeid(T));
        auto it = map_.find(type_idx);
        if (it != map_.end()) {
            return std::any_cast<T>(&it->second);
        }
        return nullptr;
    }

    /**
     * @brief Retrieves a reference to the stored object of type T.
     *
     * @tparam T The type of the object to retrieve.
     * @return A reference to the object.
     * @throw std::out_of_range if the type T is not found in the map.
     * @throw std::bad_any_cast if the stored type is somehow not T (should not happen with current put logic).
     */
    template<typename T>
    T& get_ref() {
        std::type_index type_idx = std::type_index(typeid(T));
        auto it = map_.find(type_idx);
        if (it != map_.end()) {
            // This will throw std::bad_any_cast if the type is incorrect,
            // but our put logic ensures it's correct.
            return std::any_cast<T&>(it->second);
        }
        throw std::out_of_range("Type not found in TypeMap: " + std::string(typeid(T).name()));
    }

    /**
     * @brief Retrieves a const reference to the stored object of type T.
     *
     * @tparam T The type of the object to retrieve.
     * @return A const reference to the object.
     * @throw std::out_of_range if the type T is not found in the map.
     * @throw std::bad_any_cast if the stored type is somehow not T.
     */
    template<typename T>
    const T& get_ref() const {
        std::type_index type_idx = std::type_index(typeid(T));
        auto it = map_.find(type_idx);
        if (it != map_.end()) {
            return std::any_cast<const T&>(it->second);
        }
        throw std::out_of_range("Type not found in TypeMap: " + std::string(typeid(T).name()));
    }

    /**
     * @brief Checks if an object of type T is stored in the map.
     *
     * @tparam T The type to check for.
     * @return True if an object of type T is stored, false otherwise.
     */
    template<typename T>
    bool contains() const {
        std::type_index type_idx = std::type_index(typeid(T));
        return map_.count(type_idx) > 0;
    }

    /**
     * @brief Removes the object of type T from the map.
     *
     * @tparam T The type of the object to remove.
     * @return True if an object of type T was found and removed, false otherwise.
     */
    template<typename T>
    bool remove() {
        std::type_index type_idx = std::type_index(typeid(T));
        return map_.erase(type_idx) > 0;
    }

    /**
     * @brief Returns the number of distinct types stored in the map.
     * @return The number of elements in the map.
     */
    size_t size() const noexcept {
        return map_.size();
    }

    /**
     * @brief Checks if the map is empty (contains no elements).
     * @return True if the map is empty, false otherwise.
     */
    bool empty() const noexcept {
        return map_.empty();
    }

    /**
     * @brief Removes all elements from the map.
     */
    void clear() noexcept {
        map_.clear();
    }
};

} // namespace cpp_utils

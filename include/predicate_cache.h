#ifndef PREDICATE_CACHE_H
#define PREDICATE_CACHE_H

#include <vector>
#include <functional>
#include <unordered_map>
#include <optional> // For std::optional
#include <stdexcept> // For std::out_of_range

// Define PredicateId as a type alias for clarity
using PredicateId = size_t;

template<typename T>
class PredicateCache {
public:
    // Type alias for the predicate function
    using Predicate = std::function<bool(const T&)>;

    // Registers a new predicate and returns its unique ID.
    // Predicate IDs are stable and can be used for subsequent calls.
    PredicateId register_predicate(Predicate p) {
        predicates_.push_back(std::move(p));
        // The ID is simply the index in the vector
        return predicates_.size() - 1;
    }

    // Evaluates the predicate for the given object.
    // If the result is already cached, it returns the cached value.
    // Otherwise, it evaluates the predicate, caches the result, and returns it.
    // Throws std::out_of_range if the predicate ID is invalid.
    bool evaluate(const T& obj, PredicateId id) {
        if (id >= predicates_.size()) {
            throw std::out_of_range("PredicateId out of range.");
        }

        // Ensure the object exists in the cache map
        auto& obj_cache_entry = cache_[obj];

        // Ensure the cache entry for this object is large enough for the given predicate ID
        if (obj_cache_entry.size() <= id) {
            // Resize with std::nullopt for any new predicate slots
            obj_cache_entry.resize(id + 1, std::nullopt);
        }

        // Check if the result is already cached
        if (obj_cache_entry[id].has_value()) {
            return obj_cache_entry[id].value();
        } else {
            // Evaluate the predicate
            bool result = predicates_[id](obj);
            // Cache the result
            obj_cache_entry[id] = result;
            return result;
        }
    }

    // Returns the cached result for the given object and predicate ID if it exists.
    // Otherwise, returns std::nullopt.
    // Throws std::out_of_range if the predicate ID is invalid.
    std::optional<bool> get_if(const T& obj, PredicateId id) const {
        if (id >= predicates_.size()) {
            throw std::out_of_range("PredicateId out of range.");
        }

        auto it = cache_.find(obj);
        if (it != cache_.end()) {
            const auto& obj_cache_entry = it->second;
            if (id < obj_cache_entry.size()) {
                return obj_cache_entry[id]; // This will be std::nullopt if not cached
            }
        }
        return std::nullopt; // Object not in cache or predicate not cached for this object
    }

    // Manually sets (primes) the cache with a result for the given object and predicate ID.
    // Throws std::out_of_range if the predicate ID is invalid.
    void prime(const T& obj, PredicateId id, bool result) {
        if (id >= predicates_.size()) {
            throw std::out_of_range("PredicateId out of range.");
        }

        auto& obj_cache_entry = cache_[obj];
        if (obj_cache_entry.size() <= id) {
            obj_cache_entry.resize(id + 1, std::nullopt);
        }
        obj_cache_entry[id] = result;
    }

    // Invalidates all cached predicate results for a specific object.
    // The object itself remains in the cache_ map (potentially with an empty vector),
    // but all its predicate results are cleared.
    void invalidate(const T& obj) {
        auto it = cache_.find(obj);
        if (it != cache_.end()) {
            // Clear all std::optional<bool> values for this object
            for(auto& cached_val : it->second) {
                cached_val.reset();
            }
        }
    }

    // Clears all cached results for all objects and all predicates.
    // Registered predicates remain.
    void invalidate_all() {
        for (auto& pair : cache_) {
            for (auto& cached_val : pair.second) {
                cached_val.reset(); // Set to std::nullopt
            }
        }
    }

    // Removes an object and all its cached results entirely from the cache.
    void remove(const T& obj) {
        cache_.erase(obj);
    }

    // Returns the number of objects currently held in the cache.
    // This corresponds to the number of entries in the outer unordered_map.
    size_t size() const {
        return cache_.size();
    }

private:
    // Storage for the registered predicate functions
    std::vector<Predicate> predicates_;

    // The main cache: maps an object T to a vector of optional booleans.
    // Each element in the vector corresponds to a predicate ID.
    // std::optional<bool> is used to distinguish between a cached 'false'
    // and a non-existent cache entry.
    std::unordered_map<T, std::vector<std::optional<bool>>> cache_;

    // Design constraint: T must be hashable for std::unordered_map
    // and comparable if used in other contexts (though not strictly by unordered_map itself
    // unless for specific collision resolution strategies not typically exposed).
};

#endif // PREDICATE_CACHE_H

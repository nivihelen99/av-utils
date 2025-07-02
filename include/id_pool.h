#ifndef ID_POOL_H
#define ID_POOL_H

#include <vector>
#include <cstdint> // For uint32_t
#include <limits>  // For std::numeric_limits
#include <stdexcept> // For std::runtime_error (optional, for allocation failure)

// Forward declaration for potential friend class or other uses if needed
class IdPool;

struct Id {
    uint32_t index;
    uint32_t generation;

    // Default equality operator
    bool operator==(const Id& other) const = default;
    // Add a less-than operator for potential use in sorted containers (e.g. std::map keys)
    bool operator<(const Id& other) const {
        if (index != other.index) {
            return index < other.index;
        }
        return generation < other.generation;
    }
};

// Hash functor for Id, enabling its use as a key in unordered containers
struct IdHash {
    std::size_t operator()(const Id& id) const {
        // A common way to combine hashes:
        // https://stackoverflow.com/questions/17016175/c-unordered-map-using-a-custom-class-type-as-the-key
        std::size_t h1 = std::hash<uint32_t>{}(id.index);
        std::size_t h2 = std::hash<uint32_t>{}(id.generation);
        return h1 ^ (h2 << 1); // Or use boost::hash_combine if Boost was allowed
    }
};


class IdPool {
public:
    IdPool() : active_id_count_(0) {}

    Id allocate() {
        uint32_t index;
        uint32_t current_generation;

        if (!free_indices_.empty()) {
            index = free_indices_.back();
            free_indices_.pop_back();
            // The generation for this index was already incremented upon release.
            // So, the value currently in generations_[index] is the correct new generation.
            current_generation = generations_[index];
        } else {
            // No free indices, allocate a new one
            // Check if we'd exceed uint32_t for index if generations_ is already full.
            // generations_.size() is size_t, index is uint32_t.
            if (generations_.size() >= std::numeric_limits<uint32_t>::max()) {
                // This is an extreme edge case, but good to consider.
                // Depending on requirements, could throw, or return an invalid Id.
                // For now, let's throw, as it indicates pool exhaustion for new indices.
                throw std::runtime_error("IdPool has exhausted all possible indices.");
            }
            index = static_cast<uint32_t>(generations_.size());
            generations_.push_back(0); // New IDs start with generation 0
            current_generation = 0;
        }

        active_id_count_++;
        return {index, current_generation};
    }

    void release(Id id) {
        // Check if index is valid
        if (id.index >= generations_.size()) {
            // Index out of bounds, invalid ID or logic error somewhere else
            return;
        }

        // Check if the generation matches (prevents releasing a stale or already processed ID)
        // And also ensure it's not already in the free list (though generation check should mostly cover this)
        if (generations_[id.index] == id.generation) {
            // Check if it's already in free_indices_ (double free attempt of the *same* valid ID instance)
            // This check is more complex if free_indices_ is not sorted.
            // A robust way for a small number of free_indices is to iterate.
            // However, if an ID is valid (index and generation match), and it's active,
            // it shouldn't be in free_indices_. The generation check is key.
            // If generations_[id.index] == id.generation, it means it's currently considered "active"
            // from the pool's perspective.

            // Increment generation for this slot. The next time this index is allocated,
            // it will have this new generation.
            generations_[id.index]++;
            // Handle generation overflow, though highly unlikely for uint32_t.
            // If generations_[id.index] becomes 0 again, it might collide with a newly allocated
            // ID at the same index if that new ID also starts at 0.
            // A common strategy is to ensure generation 0 is special or to reserve it.
            // Or, if generation wraps around, it's a known limitation.
            // For now, we'll assume uint32_t is large enough that wrap-around is not a typical concern.
            // If it wraps to a generation that is somehow live, that's a problem.
            // But it would require 2^32 releases and reallocations of the same index.

            free_indices_.push_back(id.index);
            active_id_count_--;
        }
        // If generation doesn't match, it's a stale ID or already released and re-allocated.
        // Silently ignore in that case, as per "do nothing or throw an error (document behavior, doing nothing is safer for release)".
    }

    bool is_valid(Id id) const {
        // Check if index is within the current bounds of allocated slots
        if (id.index >= generations_.size()) {
            return false; // Index never allocated or out of bounds
        }

        // The ID is valid if its generation matches the current generation stored for that index.
        // A released ID would have its stored generation incremented, thus not matching.
        // A never-allocated ID (if somehow an Id struct was formed for it) would fail the index check or generation match.
        // An ID whose index was recycled will only be valid if the generation also matches the NEW generation.
        return generations_[id.index] == id.generation;
    }

    size_t size() const {
        return active_id_count_;
    }

    // Consider adding a capacity() method or a way to reserve space if needed
    // size_t capacity() const { return generations_.size(); }

private:
    std::vector<uint32_t> generations_;    // Stores the current generation for each ID index.
                                         // The value 0 could mean it's never been used or generation 0.
                                         // Let's assume generation starts at 0 for allocated IDs.
    std::vector<uint32_t> free_indices_;   // Stores indices of released IDs.
    size_t active_id_count_;             // Number of currently active IDs.

    // Maximum number of IDs. Chosen to be reasonably large but not exhaust memory quickly.
    // Can be made configurable in constructor if needed.
    // For now, let's assume the pool can grow dynamically.
    // static constexpr uint32_t MAX_IDS = std::numeric_limits<uint32_t>::max();
    // This constant isn't strictly needed if generations_ can grow indefinitely up to system limits.
};

#endif // ID_POOL_H

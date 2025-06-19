#ifndef ID_ALLOCATOR_H
#define ID_ALLOCATOR_H

#include <optional>
#include <vector>
#include <queue>
#include <set>
#include <stdexcept>
#include <numeric>
#include <algorithm>
// #include <iostream> // Logging removed

template <typename IDType>
class IDAllocator {
public:
    IDAllocator(IDType min_id, IDType max_id) : min_id_(min_id), max_id_(max_id), next_available_id_(min_id) {
        if (min_id > max_id) {
            throw std::invalid_argument("max_id cannot be less than min_id");
        }
    }

    std::optional<IDType> allocate() {
        while (!freed_ids_.empty()) {
            IDType id_from_freed_pool = freed_ids_.top();
            // Temporarily pop to check. If it's unusable, it stays popped.
            // If it's usable, we'll use it. If not, the loop continues or exits.
            freed_ids_.pop();

            if (used_ids_.count(id_from_freed_pool)) {
                // This ID was in freed_ids_ but also in used_ids_ (e.g. reserved after being freed).
                // It's not actually available from the free pool. Continue to check next from freed_ids_.
                continue;
            }
            // This ID from freed_ids_ is genuinely available.
            used_ids_.insert(id_from_freed_pool);
            return id_from_freed_pool;
        }

        // If freed_ids_ is exhausted or was empty, try sequential allocation
        IDType candidate = next_available_id_;
        while (candidate <= max_id_) {
            if (used_ids_.count(candidate)) {
                candidate++;
                continue;
            }
            used_ids_.insert(candidate);
            next_available_id_ = candidate + 1;
            return candidate;
        }
        return std::nullopt;
    }

    bool free(IDType id) {
        if (id < min_id_ || id > max_id_) {
            return false; // ID out of range
        }

        if (used_ids_.erase(id)) { // erase returns number of elements removed
            freed_ids_.push(id);
            return true;
        }
        return false; // Not allocated or already freed
    }

    bool reserve(IDType id) {
        if (id < min_id_ || id > max_id_) {
            return false; // ID out of range
        }
        if (used_ids_.count(id)) {
            return false; // Already used (allocated or reserved)
        }

        used_ids_.insert(id);
        return true;
    }

    bool is_allocated(IDType id) const {
        return used_ids_.count(id);
    }

    size_t used() const {
        return used_ids_.size();
    }

    size_t capacity() const {
        // Ensure IDType can be subtracted and cast to size_t.
        // This might require more constraints or a different approach for non-numeric IDTypes,
        // but for integral types this should be fine.
        if (max_id_ < min_id_) return 0; // Should not happen due to constructor check, but good for safety
        return static_cast<size_t>(max_id_ - min_id_) + 1;
    }

    size_t available() const {
        return capacity() - used();
    }

    void reset() {
        used_ids_.clear();
        while (!freed_ids_.empty()) {
            freed_ids_.pop();
        }
        next_available_id_ = min_id_;
    }

private:
    IDType min_id_;
    IDType max_id_;
    IDType next_available_id_;
    std::priority_queue<IDType, std::vector<IDType>, std::greater<IDType>> freed_ids_;
    std::set<IDType> used_ids_;
};

#endif // ID_ALLOCATOR_H

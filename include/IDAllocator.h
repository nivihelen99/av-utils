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
        if (max_id_ < min_id_) return 0; // Should not happen due to constructor check, but good for safety
        return static_cast<size_t>(max_id_ - min_id_) + 1;
    }

    std::optional<IDType> allocate_range(size_t n) {
        if (n == 0) {
            return std::nullopt;
        }
        if (n == 1) {
            return allocate();
        }

        // We only try to allocate from the block starting at next_available_id_ for this version.
        IDType range_start = next_available_id_;

        // Check if next_available_id_ is already past max_id_
        if (range_start > max_id_) {
            return std::nullopt;
        }

        // Calculate potential_end and check for overflow and if it exceeds max_id_
        IDType range_end;
        // Check if there are theoretically enough IDs from range_start up to max_id_
        // (max_id_ - range_start + 1) must be >= n
        // To avoid underflow with unsigned types if range_start > max_id_ (already checked),
        // and to correctly calculate remaining count:
        IDType available_count_in_sequence;
        if (max_id_ >= range_start) {
            available_count_in_sequence = max_id_ - range_start + 1;
        } else { // range_start > max_id_
            available_count_in_sequence = 0;
        }

        if (available_count_in_sequence < static_cast<IDType>(n)) {
            return std::nullopt; // Not enough IDs from range_start to max_id_
        }

        // Now it's safe to calculate range_end
        range_end = range_start + static_cast<IDType>(n - 1);

        // At this point, range_end is guaranteed not to have overflowed IDType
        // and range_end <= max_id_ is also guaranteed by the available_count_in_sequence check.

        // Check if all IDs in the range [range_start, range_end] are available
        for (IDType current_id = range_start; ; ++current_id) {
            if (used_ids_.count(current_id)) {
                // This range is blocked.
                return std::nullopt;
            }
            if (current_id == range_end) { // Successfully checked all IDs up to range_end
                break;
            }
            // Defensive check against infinite loop if IDType doesn't increment as expected past range_end
            // This should ideally not be hit if current_id == range_end break works.
            if (current_id > range_end && n > 0) { // n > 0 ensures range_end >= range_start
                return std::nullopt; // Should not happen
            }
        }

        // If we reach here, the range is available. Allocate it.
        for (IDType current_id = range_start; ; ++current_id) {
            used_ids_.insert(current_id);
            if (current_id == range_end) {
                break;
            }
            if (current_id > range_end && n > 0) { // Defensive
                 return std::nullopt; // Should not happen, means prior logic error
            }
        }

        // Update next_available_id_ only if the allocated range started at the old next_available_id_
        // For this V1, current_search_start is always next_available_id_
        if (range_end == max_id_) { // If we allocated up to the very end
             next_available_id_ = max_id_ + static_cast<IDType>(1); // Go beyond max_id_
        } else {
             next_available_id_ = range_end + static_cast<IDType>(1);
        }

        return range_start;
    }

    bool release_range(IDType start_id, size_t n) {
        if (n == 0) {
            return true;
        }

        // Calculate range_end
        IDType range_end;
        // Check for potential overflow before addition for n-1
        if (n > 0 && start_id > max_id_ - static_cast<IDType>(n - 1) ) { // potential_end would be > max_id_ or wrap
             return false; // Not enough space or would overflow
        }
        range_end = start_id + static_cast<IDType>(n - 1);

        // Validate basic bounds
        if (start_id < min_id_ || range_end > max_id_ || range_end < start_id) { // range_end < start_id also catches overflow for n>0
            return false;
        }

        // Validation Pass: Check if all IDs in the range are currently allocated
        for (IDType current_id = start_id; ; ++current_id) {
            if (!used_ids_.count(current_id)) {
                return false; // An ID in the range is not allocated
            }
            if (current_id == range_end) {
                break; // Successfully checked all IDs up to range_end
            }
             // Defensive check against an issue with IDType increment or loop logic
            if (current_id > range_end && n > 0) { return false; }
        }

        // Release Pass: All IDs are valid and allocated, now free them
        for (IDType current_id = start_id; ; ++current_id) {
            used_ids_.erase(current_id);
            freed_ids_.push(current_id);
            if (current_id == range_end) {
                break;
            }
            // Defensive check
            if (current_id > range_end && n > 0) { return false; } // Should not be reached
        }

        return true;
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

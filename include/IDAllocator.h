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

class IDAllocator {
public:
    IDAllocator(int min_id, int max_id) : min_id_(min_id), max_id_(max_id), next_available_id_(min_id) {
        // std::cerr << "[CONSTRUCTOR] min_id: " << min_id << ", max_id: " << max_id << std::endl; // Logging removed
        if (min_id > max_id) {
            throw std::invalid_argument("max_id cannot be less than min_id");
        }
    }

    std::optional<int> allocate() {
        // std::cerr << "[ALLOCATE START] next_available_id_: " << next_available_id_ // Logging removed
        //           << ", max_id_: " << max_id_
        //           << ", freed_ids_.size(): " << freed_ids_.size()
        //           << ", used_ids_.size(): " << used_ids_.size() << std::endl;

        if (!freed_ids_.empty()) {
            int id = freed_ids_.top();
            freed_ids_.pop();
            // std::cerr << "  Allocating from freed_ids_: " << id << ". Adding to used_ids_." << std::endl; // Logging removed
            used_ids_.insert(id);
            return id;
        }

        // std::cerr << "  Attempting sequential allocation. next_available_id_: " << next_available_id_ // Logging removed
        //           << ", max_id_: " << max_id_ << std::endl;

        int candidate = next_available_id_;
        while (candidate <= max_id_) {
            // std::cerr << "    Looping: candidate: " << candidate << std::endl; // Logging removed
            if (used_ids_.count(candidate)) {
                // std::cerr << "      Candidate " << candidate << " is in used_ids_. Incrementing." << std::endl; // Logging removed
                candidate++;
                continue;
            }
            // std::cerr << "      Candidate " << candidate << " is available. Inserting into used_ids_." << std::endl; // Logging removed
            used_ids_.insert(candidate);
            next_available_id_ = candidate + 1;
            // std::cerr << "      Allocated: " << candidate << ". Updated next_available_id_ to: " << next_available_id_ << std::endl; // Logging removed
            return candidate;
        }

        // std::cerr << "  Sequential allocation failed or range exhausted. Candidate: " << candidate << ", next_available_id_: " << next_available_id_ << std::endl; // Logging removed
        return std::nullopt;
    }

    bool free(int id) {
        // std::cerr << "[FREE START] id: " << id << ", min_id_: " << min_id_ << ", max_id_: " << max_id_ << std::endl; // Logging removed
        if (id < min_id_ || id > max_id_) {
            // std::cerr << "  ID out of range." << std::endl; // Logging removed
            return false; // ID out of range
        }

        if (used_ids_.erase(id)) { // erase returns number of elements removed
            // std::cerr << "  ID " << id << " removed from used_ids_. Added to freed_ids_." << std::endl; // Logging removed
            freed_ids_.push(id);
            return true;
        }
        // std::cerr << "  ID " << id << " was not in used_ids_ (not allocated or already freed)." << std::endl; // Logging removed
        return false; // Not allocated or already freed
    }

    bool reserve(int id) {
        // std::cerr << "[RESERVE START] id: " << id << ", min_id_: " << min_id_ << ", max_id_: " << max_id_  // Logging removed
        //           << ", used_ids_.size(): " << used_ids_.size() << std::endl;
        if (id < min_id_ || id > max_id_) {
            return false; // ID out of range
        }
        if (used_ids_.count(id)) {
            return false; // Already used (allocated or reserved)
        }

        used_ids_.insert(id);
        return true;
    }

    bool is_allocated(int id) const {
        return used_ids_.count(id);
    }

    size_t used() const {
        return used_ids_.size();
    }

    size_t capacity() const {
        return static_cast<size_t>(max_id_ - min_id_ + 1);
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
    int min_id_;
    int max_id_;
    int next_available_id_;
    std::priority_queue<int, std::vector<int>, std::greater<int>> freed_ids_;
    std::set<int> used_ids_;
};

#endif // ID_ALLOCATOR_H

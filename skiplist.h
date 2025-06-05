#include <iostream>
#include <vector>
#include <atomic> // Added for std::atomic
#include <mutex>  // Added for std::mutex
#include <random>
#include <climits>
#include <iomanip>
#include <string> // Added for std::string
#include <list>   // For MemoryPool's free list
#include <memory> // For std::unique_ptr

// Forward declarations for specializations
// template<> class SkipListNode<int>; // Will be specialized later if needed, not strictly for finger
// template<> class SkipList<int>;     // Forward declarations for SkipList specializations
// template<> class SkipListNode<std::string>;
// template<> class SkipList<std::string>;
// No need to forward declare the specializations themselves before the primary template.
// We only need to forward declare if one specialization refers to another in its definition.

// Definition for thread_local_finger (forward declare SkipList to define its static member)
template<typename T> class SkipList;
template<typename T>
thread_local SkipListNode<T>* SkipList<T>::thread_local_finger = nullptr;


template<> class SkipListNode<int>;
template<> class SkipList<int>;
template<> class SkipListNode<std::string>;
template<> class SkipList<std::string>;


template<typename T>
class SkipListNode {
public:
    T value;
    // std::vector<SkipListNode*> forward; // Replaced
    std::atomic<SkipListNode<T>*>* forward; // Changed to array of atomic pointers
    int node_level; // Store the actual level of this node for easier deletion of forward array

    SkipListNode(T val, int level) : value(val), node_level(level) {
        // forward now an array of atomic pointers. Size is level + 1.
        forward = new std::atomic<SkipListNode<T>*>[level + 1];
        for (int i = 0; i <= level; ++i) {
            forward[i].store(nullptr, std::memory_order_relaxed); // Initialize to nullptr
        }
    }

    ~SkipListNode() {
        delete[] forward; // Clean up the dynamically allocated array of atomics
    }
};

template<typename T>
class SkipList {
public: // Making thread_local_finger public for easier definition outside if needed by some compilers
        // or due to template instantiation rules. Can be private if definition is robustly handled.
    thread_local static SkipListNode<T>* thread_local_finger;
private:
    class MemoryPool {
    public:
        static const size_t BLOCK_SIZE = 64; // Number of nodes per block
        std::list<SkipListNode<T>*> free_list_; // List of available nodes
        std::vector<std::unique_ptr<SkipListNode<T>[]>> blocks_; // Store allocated blocks
        std::mutex pool_mutex_;

        MemoryPool() = default;

        ~MemoryPool() {
            // Destructor for SkipListNode objects in blocks is implicitly handled
            // by unique_ptr if they were constructed.
            // However, we explicitly call destructors in deallocate.
            // The primary role here is to free the raw memory of blocks.
            // free_list_ nodes point into memory owned by blocks_, so no separate delete on them.
            blocks_.clear();
        }

        SkipListNode<T>* allocate(T value, int level) {
            std::lock_guard<std::mutex> lock(pool_mutex_);
            if (free_list_.empty()) {
                // Allocate a new block
                SkipListNode<T>* new_block_raw = new SkipListNode<T>[BLOCK_SIZE](); // Value-initialize
                blocks_.emplace_back(new_block_raw); // Store with unique_ptr for automatic deallocation of the block itself

                // Add nodes from the new block to the free list
                // These nodes are just raw memory at this point.
                for (size_t i = 0; i < BLOCK_SIZE; ++i) {
                    free_list_.push_back(&new_block_raw[i]);
                }
            }

            SkipListNode<T>* node_ptr = free_list_.front();
            free_list_.pop_front();

            // Use placement new to construct the object in the allocated memory
            // This calls the SkipListNode constructor
            new (node_ptr) SkipListNode<T>(value, level);
            return node_ptr;
        }

        void deallocate(SkipListNode<T>* node) {
            if (!node) return;
            std::lock_guard<std::mutex> lock(pool_mutex_);
            node->~SkipListNode<T>(); // Explicitly call destructor
            free_list_.push_back(node);
        }
    };

    static const int MAX_LEVEL = 16; // Max level for skip list
    SkipListNode<T>* header;
    std::atomic<int> currentLevel; // Made currentLevel atomic
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;
    std::mutex random_mutex; // Mutex for protecting random number generation
    MemoryPool memory_pool_; // Added memory pool instance

public:
    SkipList() : currentLevel(0), rng(std::random_device{}()), dist(0.0, 1.0) {
        // Header node is allocated directly, not from the pool,
        // as its lifetime is tied to SkipList and it's special.
        header = new SkipListNode<T>(T{}, MAX_LEVEL);
    }

    ~SkipList() {
        SkipListNode<T>* current = header->forward[0].load(std::memory_order_acquire);
        while (current != nullptr) {
            SkipListNode<T>* next = current->forward[0].load(std::memory_order_relaxed);
            // current->~SkipListNode<T>(); // Destructor called by deallocate
            memory_pool_.deallocate(current); // Return node to pool
            current = next;
        }
        delete header; // Delete the header node itself (wasn't from pool)
        // memory_pool_ destructor will clean up all blocks.
    }

    // Generate random level for new node (geometric distribution)
    int randomLevel() {
        std::lock_guard<std::mutex> lock(random_mutex); // Protect RNG usage
        int level = 0;
        while (dist(rng) < 0.5 && level < MAX_LEVEL) {
            level++;
        }
        return level;
    }

    // Insert a value into the skip list
    void insert(T value) {
        if (thread_local_finger == nullptr) {
            thread_local_finger = header;
        }

        std::vector<SkipListNode<T>*> update(MAX_LEVEL + 1, nullptr);
        SkipListNode<T>* current_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int search_start_level = localCurrentLevel;

        // Heuristic for starting search
        SkipListNode<T>* finger_candidate = thread_local_finger;
        if (finger_candidate != header && finger_candidate->value < value) {
            // Potential start from finger
            current_node = finger_candidate;
            search_start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header && !(finger_candidate->value < value) && !(finger_candidate->value == value)) {
            // Value is less than finger's value, finger is not useful, start from header
            // thread_local_finger = header; // Optionally reset finger earlier
            current_node = header;
            search_start_level = localCurrentLevel;
        }
        // else current_node is already header, search_start_level is localCurrentLevel

        // Phase 1: Populate 'update' array using finger or header
        // Traverse from search_start_level down to 0
        for (int i = search_start_level; i >= 0; i--) {
            SkipListNode<T>* next_node = current_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && next_node->value < value) {
                current_node = next_node;
                next_node = current_node->forward[i].load(std::memory_order_acquire);
            }
            update[i] = current_node;
        }

        // If finger was used and value was not found "to the right" at lower levels,
        // or if finger was too far down, we might need to search from header for higher levels.
        // This ensures 'update' array is correctly populated from header for levels above finger's engagement.
        if (current_node != header && search_start_level < localCurrentLevel) {
            // If we started from a finger that was at a level below localCurrentLevel
            SkipListNode<T>* current_header_scan = header;
            for (int i = localCurrentLevel; i > search_start_level; --i) {
                 // Only fill if not already better populated by finger path (unlikely for higher levels)
                if (update[i] == nullptr || update[i] == finger_candidate ) {
                    SkipListNode<T>* next_node = current_header_scan->forward[i].load(std::memory_order_acquire);
                    while (next_node != nullptr && next_node->value < value) {
                        current_header_scan = next_node;
                        next_node = current_header_scan->forward[i].load(std::memory_order_acquire);
                    }
                    update[i] = current_header_scan;
                } else { // If update[i] was somehow populated by finger search (e.g. finger was header's child)
                    current_header_scan = update[i]; // Descend along the already found path
                }
            }
        } else if (current_node == header && search_start_level < localCurrentLevel) {
             // This case implies finger was reset to header, and search_start_level was localCurrentLevel.
             // This block should ideally not be hit if logic for setting current_node and search_start_level is correct.
             // However, if it were, it implies a full header scan is needed. The loop above would have done it.
        }


        SkipListNode<T>* current_val_check_node = update[0]->forward[0].load(std::memory_order_acquire);

        // If value doesn't exist, insert it
        if (current_val_check_node == nullptr || current_val_check_node->value != value) {
            int newLevel = randomLevel(); // This now uses the mutex

            if (newLevel > localCurrentLevel) {
                // Ensure update array for these new, higher levels points to header.
                // The previous loops for update might not have reached this high if localCurrentLevel was lower.
                for (int i = localCurrentLevel + 1; i <= newLevel; i++) {
                    update[i] = header;
                }
                // This can be tricky. A simple store might not be enough if multiple threads try this.
                // For now, a compare_exchange loop for currentLevel.
                int expected = localCurrentLevel;
                while (newLevel > expected && !currentLevel.compare_exchange_weak(expected, newLevel, std::memory_order_release, std::memory_order_relaxed)) {
                    // expected is updated by CAS on failure, loop if another thread increased it but still less than newLevel
                }
                localCurrentLevel = currentLevel.load(std::memory_order_acquire); // refresh localCurrentLevel
            }

            // Create new node using the memory pool
            SkipListNode<T>* newNode = memory_pool_.allocate(value, newLevel);

            // Update forward pointers using CAS
            for (int i = 0; i <= newLevel; i++) {
                SkipListNode<T>* pred = update[i];
                SkipListNode<T>* succ = pred->forward[i].load(std::memory_order_acquire);
                newNode->forward[i].store(succ, std::memory_order_relaxed); // New node initially points to successor

                // Atomically link predecessor to new node
                while (!pred->forward[i].compare_exchange_weak(succ, newNode, std::memory_order_release, std::memory_order_relaxed)) {
                    // If CAS failed, it means 'succ' changed. We need to re-evaluate from 'pred'.
                    // This simple retry might not be enough if 'pred' itself is unlinked.
                    // A full robust solution might require re-traversing from header for this level,
                    // or more complex CAS logic. For now, assume pred remains linked.
                    // Re-load successor for the next attempt.
                    succ = pred->forward[i].load(std::memory_order_acquire);
                    newNode->forward[i].store(succ, std::memory_order_relaxed);
                }
            }

            // std::cout << "Inserted " << value << " at level " << newLevel << std::endl; // Logging removed for brevity
        }
        // Update finger to the predecessor of the value (or where it would be inserted)
        // update[0] should be the correct predecessor from the combined search logic.
        if (update[0] != nullptr) {
            thread_local_finger = update[0];
        } else {
            // This case should ideally not be reached if header exists.
            thread_local_finger = header;
        }
    }

    // Search for a value in the skip list
    bool search(T value) const {
        if (SkipList<T>::thread_local_finger == nullptr) {
            SkipList<T>::thread_local_finger = header;
        }

        SkipListNode<T>* current_search_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int start_level = localCurrentLevel;
        
        SkipListNode<T>* finger_ptr = SkipList<T>::thread_local_finger; // Use a temp var for readability

        if (finger_ptr != header && finger_ptr->value < value) {
            current_search_node = finger_ptr;
            start_level = std::min(localCurrentLevel, finger_ptr->node_level);
        } else if (finger_ptr != header && !(finger_ptr->value < value) && !(finger_ptr->value == value)) {
            // Value is less than finger's value, finger not useful for starting descent here.
            // Reset finger to header as we are about to do a full scan from header.
            SkipList<T>::thread_local_finger = header;
            current_search_node = header; // Ensure current_search_node is header
            start_level = localCurrentLevel; // And start_level is top
        }
        // else: current_search_node is header, start_level is localCurrentLevel (e.g. finger was header or value == finger value)

        SkipListNode<T>* predecessor_at_level0 = header; // Initialize to header

        // Start from determined start_level and go down
        for (int i = start_level; i >= 0; i--) {
            SkipListNode<T>* next_node = current_search_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && next_node->value < value) {
                current_search_node = next_node;
                next_node = current_search_node->forward[i].load(std::memory_order_acquire);
            }
            if (i == 0) { // When we are at the bottom level
                predecessor_at_level0 = current_search_node;
            }
        }
        
        // If the search started from a finger and didn't find the value to its right (or at a lower level),
        // it's possible the value is actually before the finger, requiring a search from the header.
        // This makes the finger heuristic more complex for 'search' if it's to be fully optimal.
        // The current logic: if finger is not helpful, it defaults to header search.
        // The predecessor_at_level0 is based on the path taken.

        SkipListNode<T>* found_node = current_search_node->forward[0].load(std::memory_order_acquire);

        // Update finger to the predecessor found at level 0 from the search path.
        SkipList<T>::thread_local_finger = predecessor_at_level0;

        return (found_node != nullptr && found_node->value == value);
    }

    // Delete a value from the skip list
    bool remove(T value) {
        if (thread_local_finger == nullptr) {
            thread_local_finger = header;
        }

        std::vector<SkipListNode<T>*> update(MAX_LEVEL + 1, nullptr);
        SkipListNode<T>* current_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int search_start_level = localCurrentLevel;

        SkipListNode<T>* finger_candidate = thread_local_finger;
        if (finger_candidate != header && finger_candidate->value < value) {
            current_node = finger_candidate;
            search_start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header && !(finger_candidate->value < value) && !(finger_candidate->value == value)) {
            // Value is less than finger's value, finger is not useful.
            // thread_local_finger = header; // Optionally reset finger earlier
            current_node = header;
            search_start_level = localCurrentLevel;
        }

        // Phase 1: Populate 'update' array using finger or header
        for (int i = search_start_level; i >= 0; i--) {
            SkipListNode<T>* next_node = current_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && next_node->value < value) {
                current_node = next_node;
                next_node = current_node->forward[i].load(std::memory_order_acquire);
            }
            update[i] = current_node;
        }

        // Phase 2: Correct 'update' for levels above finger's engagement if finger was used
        if (current_node != header && search_start_level < localCurrentLevel) {
            SkipListNode<T>* current_header_scan = header;
            for (int i = localCurrentLevel; i > search_start_level; --i) {
                if (update[i] == nullptr || update[i] == finger_candidate) {
                    SkipListNode<T>* next_node = current_header_scan->forward[i].load(std::memory_order_acquire);
                    while (next_node != nullptr && next_node->value < value) {
                        current_header_scan = next_node;
                        next_node = current_header_scan->forward[i].load(std::memory_order_acquire);
                    }
                    update[i] = current_header_scan;
                } else {
                     current_header_scan = update[i]; // Descend along this path
                }
            }
        }
        // Ensure update[0] is not null if list is not empty. If it's null, means header is the predecessor.
        if (update[0] == nullptr) update[0] = header;


        SkipListNode<T>* target_node = update[0]->forward[0].load(std::memory_order_acquire);

        // If found, attempt to delete it
        if (target_node != nullptr && target_node->value == value) {
            // Attempt to unlink the node from all levels
            for (int i = 0; i <= target_node->node_level; ++i) { // Iterate up to the target_node's actual level
                SkipListNode<T>* pred = update[i];
                 // Critical: Ensure pred is valid and correctly found for this level 'i'.
                 // If pred is null (e.g. from incomplete update array) or doesn't point to target_node, skip.
                if (pred == nullptr) { // This check might be too simplistic.
                                      // update[i] should ideally always be valid after Phase 1 & 2.
                    continue;
                }
                SkipListNode<T>* succ_of_target = target_node->forward[i].load(std::memory_order_relaxed);

                if (pred->forward[i].load(std::memory_order_acquire) == target_node) {
                    SkipListNode<T>* expected_target = target_node;
                    if (!pred->forward[i].compare_exchange_weak(expected_target, succ_of_target, std::memory_order_release, std::memory_order_relaxed)) {
                        // CAS failed.
                    }
                }
                 // If pred->forward[i] was not target_node, implies concurrent modification.
                 // The simple approach is to assume another thread dealt with it or will.
            }

            memory_pool_.deallocate(target_node);

            // Update current level if necessary - this also needs to be careful
            // This simplified version might not be perfectly atomic.
            int newLevel = localCurrentLevel;
            while (newLevel > 0 && header->forward[newLevel].load(std::memory_order_acquire) == nullptr) {
                newLevel--;
            }
            if (newLevel < localCurrentLevel) {
                 int expected = localCurrentLevel;
                 // Attempt to lower currentLevel, but only if it hasn't been increased by another thread.
                 currentLevel.compare_exchange_weak(expected, newLevel, std::memory_order_release, std::memory_order_relaxed);
            }
            
            // std::cout << "Logically deleted " << value << std::endl; // Logging modified
            // Update finger to the predecessor of the deleted node
            if (update[0] != nullptr) {
                thread_local_finger = update[0];
            } else {
                thread_local_finger = header;
            }
            return true;
        }

        // Value not found, update finger to where it would have been (its predecessor)
        if (update[0] != nullptr) {
            thread_local_finger = update[0];
        } else {
            thread_local_finger = header;
        }
        return false;
    }

    // Display the skip list structure
    void display() const {
        std::cout << "\n=== Skip List Structure ===" << std::endl;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);

        for (int i = localCurrentLevel; i >= 0; i--) {
            std::cout << "Level " << std::setw(2) << i << ": ";
            SkipListNode<T>* node = header->forward[i].load(std::memory_order_acquire);
            
            while (node != nullptr) {
                std::cout << std::setw(4) << node->value << " -> ";
                node = node->forward[i].load(std::memory_order_acquire);
            }
            std::cout << "NULL" << std::endl;
        }
        std::cout << std::endl;
    }

    // Print all values in sorted order
    void printValues() const {
        std::cout << "Values in skip list: ";
        SkipListNode<T>* node = header->forward[0].load(std::memory_order_acquire);
        
        while (node != nullptr) {
            std::cout << node->value << " ";
            node = node->forward[0].load(std::memory_order_acquire);
        }
        std::cout << std::endl;
    }

    // Get the size of the skip list
    int size() const {
        int count = 0;
        SkipListNode<T>* node = header->forward[0].load(std::memory_order_acquire);
        
        while (node != nullptr) {
            count++;
            node = node->forward[0].load(std::memory_order_acquire);
        }
        return count;
    }

    // Find the k-th smallest element (0-indexed)
    T kthElement(int k) const {
        if (k < 0) {
            throw std::invalid_argument("k must be non-negative");
        }
        
        SkipListNode<T>* node = header->forward[0].load(std::memory_order_acquire);
        
        for (int i = 0; i < k && node != nullptr; i++) {
            node = node->forward[0].load(std::memory_order_acquire);
        }
        
        if (node == nullptr) {
            throw std::out_of_range("k is larger than skip list size");
        }
        
        return node->value;
    }

    // Find elements in a range [min, max]
    std::vector<T> rangeQuery(T minVal, T maxVal) const {
        std::vector<T> result;
        SkipListNode<T>* current_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        
        // Find the starting position
        for (int i = localCurrentLevel; i >= 0; i--) {
            SkipListNode<T>* next_node = current_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && next_node->value < minVal) {
                current_node = next_node;
                next_node = current_node->forward[i].load(std::memory_order_acquire);
            }
        }
        
        current_node = current_node->forward[0].load(std::memory_order_acquire);
        
        // Collect values in range
        while (current_node != nullptr && current_node->value <= maxVal) {
            if (current_node->value >= minVal) {
                result.push_back(current_node->value);
            }
            current_node = current_node->forward[0].load(std::memory_order_acquire);
        }
        
        return result;
    }
};

// End of generic SkipList

// --- Specialization for int ---
// The thread_local static member definition for SkipList<T> covers specializations.
// No need to redefine thread_local_finger for each specialization explicitly unless its behavior differs.

template<>
class SkipListNode<int> {
public:
    int value;
    std::atomic<SkipListNode<int>*>* forward;
    int node_level;

    SkipListNode(int val, int level) : value(val), node_level(level) {
        forward = new std::atomic<SkipListNode<int>*>[level + 1];
        for (int i = 0; i <= level; ++i) {
            forward[i].store(nullptr, std::memory_order_relaxed);
        }
    }

    ~SkipListNode() {
        delete[] forward;
    }
};

template<>
class SkipList<int> {
// public: // No need to redeclare the static member, it's inherited by specialization context
    // thread_local static SkipListNode<int>* thread_local_finger; // This would be a re-declaration
private:
    static const int MAX_LEVEL = 16;
    SkipListNode<int>* header;
    std::atomic<int> currentLevel;
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;
    std::mutex random_mutex;
    SkipList<int>::MemoryPool memory_pool_; // Specialized memory pool instance

public:
    SkipList() : currentLevel(0), rng(std::random_device{}()), dist(0.0, 1.0) {
        header = new SkipListNode<int>(int{}, MAX_LEVEL); // int{} is 0, header not from pool
    }

    ~SkipList() {
        SkipListNode<int>* current_node = header->forward[0].load(std::memory_order_acquire);
        while (current_node != nullptr) {
            SkipListNode<int>* next_node = current_node->forward[0].load(std::memory_order_relaxed);
            memory_pool_.deallocate(current_node);
            current_node = next_node;
        }
        delete header;
    }

    int randomLevel() {
        std::lock_guard<std::mutex> lock(random_mutex);
        int level = 0;
        while (dist(rng) < 0.5 && level < MAX_LEVEL) {
            level++;
        }
        return level;
    }

    void insert(int value) {
        if (SkipList<int>::thread_local_finger == nullptr) {
            SkipList<int>::thread_local_finger = header;
        }

        std::vector<SkipListNode<int>*> update(MAX_LEVEL + 1, nullptr);
        SkipListNode<int>* current_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int search_start_level = localCurrentLevel;

        SkipListNode<int>* finger_candidate = SkipList<int>::thread_local_finger;
        if (finger_candidate != header && finger_candidate->value < value) {
            current_node = finger_candidate;
            search_start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header && !(finger_candidate->value < value) && !(finger_candidate->value == value)) {
            current_node = header; // Reset to header if finger is past value
            search_start_level = localCurrentLevel;
            // SkipList<int>::thread_local_finger = header; // Optional: aggressively reset finger
        }

        for (int i = search_start_level; i >= 0; i--) {
            SkipListNode<int>* next_node = current_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && next_node->value < value) {
                current_node = next_node;
                next_node = current_node->forward[i].load(std::memory_order_acquire);
            }
            update[i] = current_node;
        }

        if (current_node != header && search_start_level < localCurrentLevel) {
            SkipListNode<int>* current_header_scan = header;
            for (int i = localCurrentLevel; i > search_start_level; --i) {
                if (update[i] == nullptr || update[i] == finger_candidate) {
                    SkipListNode<int>* next_node = current_header_scan->forward[i].load(std::memory_order_acquire);
                    while (next_node != nullptr && next_node->value < value) {
                        current_header_scan = next_node;
                        next_node = current_header_scan->forward[i].load(std::memory_order_acquire);
                    }
                    update[i] = current_header_scan;
                } else {
                     current_header_scan = update[i];
                }
            }
        }
        if (update[0] == nullptr) update[0] = header;

        SkipListNode<int>* insert_pos_node = update[0]->forward[0].load(std::memory_order_acquire);

        if (insert_pos_node == nullptr || insert_pos_node->value != value) {
            int newLevel = randomLevel();
            if (newLevel > localCurrentLevel) {
                for (int i = localCurrentLevel + 1; i <= newLevel; i++) {
                    update[i] = header;
                }
                int expected = localCurrentLevel;
                while (newLevel > expected && !currentLevel.compare_exchange_weak(expected, newLevel, std::memory_order_release, std::memory_order_relaxed)) {
                    // loop
                }
                localCurrentLevel = currentLevel.load(std::memory_order_acquire);
            }

            SkipListNode<int>* newNode = memory_pool_.allocate(value, newLevel);
            for (int i = 0; i <= newLevel; i++) {
                if(update[i] == nullptr) update[i] = header; // Ensure pred is not null
                SkipListNode<int>* pred = update[i];
                SkipListNode<int>* succ = pred->forward[i].load(std::memory_order_acquire);
                newNode->forward[i].store(succ, std::memory_order_relaxed);
                while (!pred->forward[i].compare_exchange_weak(succ, newNode, std::memory_order_release, std::memory_order_relaxed)) {
                    succ = pred->forward[i].load(std::memory_order_acquire);
                    newNode->forward[i].store(succ, std::memory_order_relaxed);
                }
            }
        }
        if (update[0] != nullptr) {
            SkipList<int>::thread_local_finger = update[0];
        } else {
            SkipList<int>::thread_local_finger = header;
        }
    }

    bool search(int value) const {
        if (SkipList<int>::thread_local_finger == nullptr) {
            SkipList<int>::thread_local_finger = header;
        }

        SkipListNode<int>* current_search_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int start_level = localCurrentLevel;
        SkipListNode<int>* finger_ptr = SkipList<int>::thread_local_finger;

        if (finger_ptr != header && finger_ptr->value < value) {
            current_search_node = finger_ptr;
            start_level = std::min(localCurrentLevel, finger_ptr->node_level);
        } else if (finger_ptr != header && !(finger_ptr->value < value) && !(finger_ptr->value == value)) {
            SkipList<int>::thread_local_finger = header;
            current_search_node = header;
            start_level = localCurrentLevel;
        }

        SkipListNode<int>* predecessor_at_level0 = header;

        for (int i = start_level; i >= 0; i--) {
            SkipListNode<int>* next_node = current_search_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && next_node->value < value) {
                current_search_node = next_node;
                next_node = current_search_node->forward[i].load(std::memory_order_acquire);
            }
             if (i == 0) predecessor_at_level0 = current_search_node;
        }
        SkipListNode<int>* found_node = current_search_node->forward[0].load(std::memory_order_acquire);

        SkipList<int>::thread_local_finger = predecessor_at_level0;

        return (found_node != nullptr && found_node->value == value);
    }

    bool remove(int value) {
        if (SkipList<int>::thread_local_finger == nullptr) {
            SkipList<int>::thread_local_finger = header;
        }

        std::vector<SkipListNode<int>*> update(MAX_LEVEL + 1, nullptr);
        SkipListNode<int>* current_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int search_start_level = localCurrentLevel;

        SkipListNode<int>* finger_candidate = SkipList<int>::thread_local_finger;
        if (finger_candidate != header && finger_candidate->value < value) {
            current_node = finger_candidate;
            search_start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header && !(finger_candidate->value < value) && !(finger_candidate->value == value)) {
            current_node = header;
            search_start_level = localCurrentLevel;
            // SkipList<int>::thread_local_finger = header; // Optional: aggressive reset
        }

        for (int i = search_start_level; i >= 0; i--) {
            SkipListNode<int>* next_node = current_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && next_node->value < value) {
                current_node = next_node;
                next_node = current_node->forward[i].load(std::memory_order_acquire);
            }
            update[i] = current_node;
        }

        if (current_node != header && search_start_level < localCurrentLevel) {
            SkipListNode<int>* current_header_scan = header;
            for (int i = localCurrentLevel; i > search_start_level; --i) {
                if (update[i] == nullptr || update[i] == finger_candidate) {
                    SkipListNode<int>* next_node = current_header_scan->forward[i].load(std::memory_order_acquire);
                    while (next_node != nullptr && next_node->value < value) {
                        current_header_scan = next_node;
                        next_node = current_header_scan->forward[i].load(std::memory_order_acquire);
                    }
                    update[i] = current_header_scan;
                } else {
                    current_header_scan = update[i];
                }
            }
        }
        if (update[0] == nullptr) update[0] = header;

        SkipListNode<int>* target_node = update[0]->forward[0].load(std::memory_order_acquire);

        if (target_node != nullptr && target_node->value == value) {
            for (int i = 0; i <= target_node->node_level; ++i) {
                SkipListNode<int>* pred = update[i];
                if (pred == nullptr) continue;
                SkipListNode<int>* succ_of_target = target_node->forward[i].load(std::memory_order_relaxed);
                if (pred->forward[i].load(std::memory_order_acquire) == target_node) {
                    SkipListNode<int>* expected_target = target_node;
                    if (!pred->forward[i].compare_exchange_weak(expected_target, succ_of_target, std::memory_order_release, std::memory_order_relaxed)) {
                        // CAS failed
                    }
                }
            }
            memory_pool_.deallocate(target_node);

            int currentOverallLevel = currentLevel.load(std::memory_order_acquire);
            int newTopLevel = currentOverallLevel;
            while (newTopLevel > 0 && header->forward[newTopLevel].load(std::memory_order_acquire) == nullptr) {
                newTopLevel--;
            }
            if (newTopLevel < currentOverallLevel) {
                 int expected = currentOverallLevel;
                 currentLevel.compare_exchange_weak(expected, newTopLevel, std::memory_order_release, std::memory_order_relaxed);
            }
            if (update[0] != nullptr) {
                SkipList<int>::thread_local_finger = update[0];
            } else {
                SkipList<int>::thread_local_finger = header;
            }
            return true;
        }

        if (update[0] != nullptr) {
            SkipList<int>::thread_local_finger = update[0];
        } else {
            SkipList<int>::thread_local_finger = header;
        }
        return false;
    }

    void display() const {
        std::cout << "\n=== Skip List Structure (int) ===" << std::endl;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        for (int i = localCurrentLevel; i >= 0; i--) {
            std::cout << "Level " << std::setw(2) << i << ": ";
            SkipListNode<int>* node = header->forward[i].load(std::memory_order_acquire);
            while (node != nullptr) {
                std::cout << std::setw(4) << node->value << " -> ";
                node = node->forward[i].load(std::memory_order_acquire);
            }
            std::cout << "NULL" << std::endl;
        }
        std::cout << std::endl;
    }

    void printValues() const {
        std::cout << "Values in skip list (int): ";
        SkipListNode<int>* node = header->forward[0].load(std::memory_order_acquire);
        while (node != nullptr) {
            std::cout << node->value << " ";
            node = node->forward[0].load(std::memory_order_acquire);
        }
        std::cout << std::endl;
    }

    int size() const {
        int count = 0;
        SkipListNode<int>* node = header->forward[0].load(std::memory_order_acquire);
        while (node != nullptr) {
            count++;
            node = node->forward[0].load(std::memory_order_acquire);
        }
        return count;
    }

    int kthElement(int k) const {
        if (k < 0) throw std::invalid_argument("k must be non-negative");
        SkipListNode<int>* node = header->forward[0].load(std::memory_order_acquire);
        for (int i = 0; i < k && node != nullptr; i++) {
            node = node->forward[0].load(std::memory_order_acquire);
        }
        if (node == nullptr) throw std::out_of_range("k is larger than skip list size");
        return node->value;
    }

    std::vector<int> rangeQuery(int minVal, int maxVal) const {
        std::vector<int> result;
        SkipListNode<int>* current_node = header; // Renamed
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        for (int i = localCurrentLevel; i >= 0; i--) {
            SkipListNode<int>* next_node = current_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && next_node->value < minVal) {
                current_node = next_node;
                next_node = current_node->forward[i].load(std::memory_order_acquire);
            }
        }
        current_node = current_node->forward[0].load(std::memory_order_acquire);
        while (current_node != nullptr && current_node->value <= maxVal) {
            if (current_node->value >= minVal) {
                result.push_back(current_node->value);
            }
            current_node = current_node->forward[0].load(std::memory_order_acquire);
        }
        return result;
    }
};


// --- Specialization for std::string ---

template<>
class SkipListNode<std::string> {
public:
    std::string value;
    std::atomic<SkipListNode<std::string>*>* forward;
    int node_level;

    SkipListNode(std::string val, int level) : value(val), node_level(level) {
        forward = new std::atomic<SkipListNode<std::string>*>[level + 1];
        for (int i = 0; i <= level; ++i) {
            forward[i].store(nullptr, std::memory_order_relaxed);
        }
    }

    ~SkipListNode() {
        delete[] forward;
    }
};

template<>
class SkipList<std::string> {
private:
    static const int MAX_LEVEL = 16;
    SkipListNode<std::string>* header;
    std::atomic<int> currentLevel;
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;
    std::mutex random_mutex;
    SkipList<std::string>::MemoryPool memory_pool_; // Specialized memory pool instance
// public: // No need to redeclare static member for specialization
    // thread_local static SkipListNode<std::string>* thread_local_finger;


public:
    SkipList() : currentLevel(0), rng(std::random_device{}()), dist(0.0, 1.0) {
        header = new SkipListNode<std::string>(std::string{}, MAX_LEVEL); // std::string{} is "", header not from pool
    }

    ~SkipList() {
        SkipListNode<std::string>* current_node = header->forward[0].load(std::memory_order_acquire);
        while (current_node != nullptr) {
            SkipListNode<std::string>* next_node = current_node->forward[0].load(std::memory_order_relaxed);
            memory_pool_.deallocate(current_node);
            current_node = next_node;
        }
        delete header;
    }

    int randomLevel() {
        std::lock_guard<std::mutex> lock(random_mutex);
        int level = 0;
        while (dist(rng) < 0.5 && level < MAX_LEVEL) {
            level++;
        }
        return level;
    }

    void insert(std::string value) {
        // Finger logic adapted from generic template
        if (SkipList<std::string>::thread_local_finger == nullptr) {
            SkipList<std::string>::thread_local_finger = header;
        }

        std::vector<SkipListNode<std::string>*> update(MAX_LEVEL + 1, nullptr);
        SkipListNode<std::string>* current_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int search_start_level = localCurrentLevel;

        SkipListNode<std::string>* finger_candidate = SkipList<std::string>::thread_local_finger;
        if (finger_candidate != header && finger_candidate->value < value) {
            current_node = finger_candidate;
            search_start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header && !(finger_candidate->value < value) && !(finger_candidate->value == value)) {
            current_node = header;
            search_start_level = localCurrentLevel;
            // SkipList<std::string>::thread_local_finger = header; // Optional: aggressive reset
        }

        for (int i = search_start_level; i >= 0; i--) {
            SkipListNode<std::string>* next_node = current_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && next_node->value < value) {
                current_node = next_node;
                next_node = current_node->forward[i].load(std::memory_order_acquire);
            }
            update[i] = current_node;
        }

        if (current_node != header && search_start_level < localCurrentLevel) {
            SkipListNode<std::string>* current_header_scan = header;
            for (int i = localCurrentLevel; i > search_start_level; --i) {
                if (update[i] == nullptr || update[i] == finger_candidate) {
                    SkipListNode<std::string>* next_node = current_header_scan->forward[i].load(std::memory_order_acquire);
                    while (next_node != nullptr && next_node->value < value) {
                        current_header_scan = next_node;
                        next_node = current_header_scan->forward[i].load(std::memory_order_acquire);
                    }
                    update[i] = current_header_scan;
                } else {
                     current_header_scan = update[i];
                }
            }
        }
        if (update[0] == nullptr) update[0] = header;

        SkipListNode<std::string>* insert_pos_node = update[0]->forward[0].load(std::memory_order_acquire);

        if (insert_pos_node == nullptr || insert_pos_node->value != value) {
            int newLevel = randomLevel();
            if (newLevel > localCurrentLevel) {
                for (int i = localCurrentLevel + 1; i <= newLevel; i++) {
                    update[i] = header;
                }
                int expected = localCurrentLevel;
                while (newLevel > expected && !currentLevel.compare_exchange_weak(expected, newLevel, std::memory_order_release, std::memory_order_relaxed)) {
                    // loop
                }
                localCurrentLevel = currentLevel.load(std::memory_order_acquire);
            }

            SkipListNode<std::string>* newNode = memory_pool_.allocate(value, newLevel);
            for (int i = 0; i <= newLevel; i++) {
                if(update[i] == nullptr) update[i] = header; // Ensure pred is not null
                SkipListNode<std::string>* pred = update[i];
                SkipListNode<std::string>* succ = pred->forward[i].load(std::memory_order_acquire);
                newNode->forward[i].store(succ, std::memory_order_relaxed);
                while (!pred->forward[i].compare_exchange_weak(succ, newNode, std::memory_order_release, std::memory_order_relaxed)) {
                    succ = pred->forward[i].load(std::memory_order_acquire);
                    newNode->forward[i].store(succ, std::memory_order_relaxed);
                }
            }
        }
        if (update[0] != nullptr) {
            SkipList<std::string>::thread_local_finger = update[0];
        } else {
            SkipList<std::string>::thread_local_finger = header;
        }
    }

    bool search(std::string value) const {
        if (SkipList<std::string>::thread_local_finger == nullptr) {
            SkipList<std::string>::thread_local_finger = header;
        }

        SkipListNode<std::string>* current_search_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int start_level = localCurrentLevel;
        SkipListNode<std::string>* finger_ptr = SkipList<std::string>::thread_local_finger;

        if (finger_ptr != header && finger_ptr->value < value) {
            current_search_node = finger_ptr;
            start_level = std::min(localCurrentLevel, finger_ptr->node_level);
        } else if (finger_ptr != header && !(finger_ptr->value < value) && !(finger_ptr->value == value)) {
            SkipList<std::string>::thread_local_finger = header;
            current_search_node = header;
            start_level = localCurrentLevel;
        }

        SkipListNode<std::string>* predecessor_at_level0 = header;

        for (int i = start_level; i >= 0; i--) {
            SkipListNode<std::string>* next_node = current_search_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && next_node->value < value) {
                current_search_node = next_node;
                next_node = current_search_node->forward[i].load(std::memory_order_acquire);
            }
             if (i == 0) predecessor_at_level0 = current_search_node;
        }
        SkipListNode<std::string>* found_node = current_search_node->forward[0].load(std::memory_order_acquire);

        SkipList<std::string>::thread_local_finger = predecessor_at_level0;

        return (found_node != nullptr && found_node->value == value);
    }

    bool remove(std::string value) {
        if (SkipList<std::string>::thread_local_finger == nullptr) {
            SkipList<std::string>::thread_local_finger = header;
        }

        std::vector<SkipListNode<std::string>*> update(MAX_LEVEL + 1, nullptr);
        SkipListNode<std::string>* current_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int search_start_level = localCurrentLevel;

        SkipListNode<std::string>* finger_candidate = SkipList<std::string>::thread_local_finger;
        if (finger_candidate != header && finger_candidate->value < value) {
            current_node = finger_candidate;
            search_start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header && !(finger_candidate->value < value) && !(finger_candidate->value == value)) {
            current_node = header;
            search_start_level = localCurrentLevel;
            // SkipList<std::string>::thread_local_finger = header; // Optional: aggressive reset
        }

        for (int i = search_start_level; i >= 0; i--) {
            SkipListNode<std::string>* next_node = current_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && next_node->value < value) {
                current_node = next_node;
                next_node = current_node->forward[i].load(std::memory_order_acquire);
            }
            update[i] = current_node;
        }

        if (current_node != header && search_start_level < localCurrentLevel) {
            SkipListNode<std::string>* current_header_scan = header;
            for (int i = localCurrentLevel; i > search_start_level; --i) {
                if (update[i] == nullptr || update[i] == finger_candidate) {
                    SkipListNode<std::string>* next_node = current_header_scan->forward[i].load(std::memory_order_acquire);
                    while (next_node != nullptr && next_node->value < value) {
                        current_header_scan = next_node;
                        next_node = current_header_scan->forward[i].load(std::memory_order_acquire);
                    }
                    update[i] = current_header_scan;
                } else {
                    current_header_scan = update[i];
                }
            }
        }
        if (update[0] == nullptr) update[0] = header;

        SkipListNode<std::string>* target_node = update[0]->forward[0].load(std::memory_order_acquire);

        if (target_node != nullptr && target_node->value == value) {
            for (int i = 0; i <= target_node->node_level; ++i) {
                SkipListNode<std::string>* pred = update[i];
                if (pred == nullptr) continue;
                SkipListNode<std::string>* succ_of_target = target_node->forward[i].load(std::memory_order_relaxed);
                if (pred->forward[i].load(std::memory_order_acquire) == target_node) {
                    SkipListNode<std::string>* expected_target = target_node;
                    if (!pred->forward[i].compare_exchange_weak(expected_target, succ_of_target, std::memory_order_release, std::memory_order_relaxed)) {
                        // CAS failed
                    }
                }
            }
            memory_pool_.deallocate(target_node);

            int currentOverallLevel = currentLevel.load(std::memory_order_acquire);
            int newTopLevel = currentOverallLevel;
            while (newTopLevel > 0 && header->forward[newTopLevel].load(std::memory_order_acquire) == nullptr) {
                newTopLevel--;
            }
            if (newTopLevel < currentOverallLevel) {
                 int expected = currentOverallLevel;
                 currentLevel.compare_exchange_weak(expected, newTopLevel, std::memory_order_release, std::memory_order_relaxed);
            }
            if (update[0] != nullptr) {
                SkipList<std::string>::thread_local_finger = update[0];
            } else {
                SkipList<std::string>::thread_local_finger = header;
            }
            return true;
        }

        if (update[0] != nullptr) {
            SkipList<std::string>::thread_local_finger = update[0];
        } else {
            SkipList<std::string>::thread_local_finger = header;
        }
        return false;
    }

    void display() const {
        std::cout << "\n=== Skip List Structure (std::string) ===" << std::endl;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        for (int i = localCurrentLevel; i >= 0; i--) {
            std::cout << "Level " << std::setw(2) << i << ": ";
            SkipListNode<std::string>* node = header->forward[i].load(std::memory_order_acquire);
            while (node != nullptr) {
                std::cout << "\"" << node->value << "\"" << " -> "; // Quotes for strings
                node = node->forward[i].load(std::memory_order_acquire);
            }
            std::cout << "NULL" << std::endl;
        }
        std::cout << std::endl;
    }

    void printValues() const {
        std::cout << "Values in skip list (std::string): ";
        SkipListNode<std::string>* node = header->forward[0].load(std::memory_order_acquire);
        while (node != nullptr) {
            std::cout << "\"" << node->value << "\"" << " "; // Quotes for strings
            node = node->forward[0].load(std::memory_order_acquire);
        }
        std::cout << std::endl;
    }

    int size() const {
        int count = 0;
        SkipListNode<std::string>* node = header->forward[0].load(std::memory_order_acquire);
        while (node != nullptr) {
            count++;
            node = node->forward[0].load(std::memory_order_acquire);
        }
        return count;
    }

    std::string kthElement(int k) const {
        if (k < 0) throw std::invalid_argument("k must be non-negative");
        SkipListNode<std::string>* node = header->forward[0].load(std::memory_order_acquire);
        for (int i = 0; i < k && node != nullptr; i++) {
            node = node->forward[0].load(std::memory_order_acquire);
        }
        if (node == nullptr) throw std::out_of_range("k is larger than skip list size");
        return node->value;
    }

    std::vector<std::string> rangeQuery(std::string minVal, std::string maxVal) const {
        std::vector<std::string> result;
        SkipListNode<std::string>* current_node = header; // Renamed
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        for (int i = localCurrentLevel; i >= 0; i--) {
            SkipListNode<std::string>* next_node = current_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && next_node->value < minVal) {
                current_node = next_node;
                next_node = current_node->forward[i].load(std::memory_order_acquire);
            }
        }
        current_node = current_node->forward[0].load(std::memory_order_acquire);
        while (current_node != nullptr && current_node->value <= maxVal) {
            if (current_node->value >= minVal) {
                result.push_back(current_node->value);
            }
            current_node = current_node->forward[0].load(std::memory_order_acquire);
        }
        return result;
    }
};

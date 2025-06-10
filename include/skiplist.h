#include <iostream>
#include <vector>
#include <algorithm> // For std::sort
#include <atomic> // Added for std::atomic
#include <mutex>  // Added for std::mutex
#include <random>
// <climits> removed - not used
#include <iomanip>
#include <string> // Added for std::string
// <list> removed - not used
#include <memory> // For std::unique_ptr
#include <iterator> // For std::forward_iterator_tag and iterator traits
#include <cstddef>  // For std::ptrdiff_t
#include <utility> // For std::pair
#include <type_traits> // For std::is_same, std::decay_t, etc.
#include <functional> // For std::less
#include <cstdlib> // For malloc and free

// Helper to extract KeyType from T
template <typename T>
struct KeyTypeHelper {
    using type = T;
};
template <typename K, typename V>
struct KeyTypeHelper<std::pair<K, V>> {
    using type = K; // Key is the first element of the pair
};
template <typename T>
using KeyType_t = typename KeyTypeHelper<T>::type;

// Type trait to check if a type is std::pair
template <typename T>
struct is_std_pair : std::false_type {};

template <typename T1, typename T2>
struct is_std_pair<std::pair<T1, T2>> : std::true_type {};

template <typename T>
inline constexpr bool is_std_pair_v = is_std_pair<std::decay_t<T>>::value;

// Helper to get the value to compare (key if pair, else value itself)
template<typename U>
constexpr decltype(auto) get_comparable_value(const U& val) {
    if constexpr (is_std_pair_v<U>) {
        return val.first; // Compare based on key
    } else {
        return val; // Compare directly
    }
}

// Helper to convert value T to string for logging.
// This function can be specialized for custom types (e.g., user-defined structs)
// to provide a specific string representation for logging and display purposes.
// For example:
// template<>
// std::string value_to_log_string<MyStruct>(const MyStruct& val) {
//   return "MyStruct(id: " + std::to_string(val.id) + ")";
// }
template<typename U>
std::string value_to_log_string(const U& val) {
    if constexpr (is_std_pair_v<std::decay_t<U>>) { // Use decay_t for pairs
        return "<" + value_to_log_string(val.first) + ":" + value_to_log_string(val.second) + ">";
    } else if constexpr (std::is_same_v<std::decay_t<U>, std::string>) {
        return "\"" + val + "\"";
    } else {
        if constexpr (std::is_arithmetic_v<std::decay_t<U>>) {
            return std::to_string(val);
        } else {
            return "[non-loggable value]";
        }
    }
}


// Forward declaration for SkipListNode, used by SkipList's static member thread_local_finger
template<typename T> class SkipListNode;

// Forward declaration for SkipList, necessary for defining its static member.
template<typename T, typename Compare> class SkipList; // Default argument removed

template<typename T>
class SkipListNode {
public:
    T value;
    std::atomic<SkipListNode<T>*>* forward; // Array of atomic pointers
    int node_level; // Actual level of this node

    SkipListNode(T val, int level) : value(val), node_level(level) {
#ifdef SKIPLIST_DEBUG_LOGGING
        std::cout << "[Node Ctor] Value: " << value_to_log_string(get_comparable_value(val)) << ", Level: " << level << ", Addr: " << this << std::endl;
#endif
        forward = new std::atomic<SkipListNode<T>*>[level + 1];
        for (int i = 0; i <= level; ++i) {
            forward[i].store(nullptr, std::memory_order_relaxed);
        }
    }

    ~SkipListNode() {
#ifdef SKIPLIST_DEBUG_LOGGING
        std::cout << "[Node Dtor] Value: " << value_to_log_string(get_comparable_value(value)) << ", Level: " << node_level << ", Addr: " << this << ", Deleting forward: " << forward << std::endl;
#endif
        delete[] forward;
    }
};

template<typename T, typename Compare = std::less<KeyType_t<T>>>
class SkipList {
public:
    using KeyType = KeyType_t<T>;

private:
    Compare key_compare_;
    int effective_max_level_;
    mutable SkipListNode<T>* finger_;

public:
    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        SkipListNode<T>* current_node;

        explicit iterator(SkipListNode<T>* node) : current_node(node) {}

        reference operator*() const {
            return current_node->value;
        }
        pointer operator->() const {
            return &(current_node->value);
        }

        iterator& operator++() {
            if (current_node) {
                current_node = current_node->forward[0].load(std::memory_order_acquire);
            }
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const iterator& other) const {
            return current_node == other.current_node;
        }
        bool operator!=(const iterator& other) const {
            return current_node != other.current_node;
        }
    };

    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = const T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const T*;
        using reference         = const T&;

        SkipListNode<T>* current_node;

        explicit const_iterator(SkipListNode<T>* node) : current_node(node) {}
        const_iterator(const iterator& other) : current_node(other.current_node) {}

        reference operator*() const {
            return current_node->value;
        }
        pointer operator->() const {
            return &(current_node->value);
        }

        const_iterator& operator++() {
            if (current_node) {
                current_node = current_node->forward[0].load(std::memory_order_acquire);
            }
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const const_iterator& other) const {
            return current_node == other.current_node;
        }
        bool operator!=(const const_iterator& other) const {
            return current_node != other.current_node;
        }
    };

    iterator begin() {
        return iterator(header->forward[0].load(std::memory_order_acquire));
    }
    iterator end() {
        return iterator(nullptr);
    }
    const_iterator begin() const {
        return const_iterator(header->forward[0].load(std::memory_order_acquire));
    }
    const_iterator end() const {
        return const_iterator(nullptr);
    }
    const_iterator cbegin() const {
        return const_iterator(header->forward[0].load(std::memory_order_acquire));
    }
    const_iterator cend() const {
        return const_iterator(nullptr);
    }

private:
#ifndef SKIPLIST_USE_STD_ALLOC
    class MemoryPool {
    private:
        static std::vector<SkipListNode<T>*>& get_thread_local_cache() {
            thread_local std::vector<SkipListNode<T>*> tl_cache;
            return tl_cache;
        }

    public:
        static const size_t BLOCK_SIZE = 64;
        static const size_t THREAD_LOCAL_CACHE_MAX_SIZE = 16;

        std::atomic<SkipListNode<T>*> atomic_free_list_head_{nullptr};
        std::vector<std::unique_ptr<char[]>> blocks_; // Changed to char[]
        std::mutex pool_mutex_;

        MemoryPool() = default;

        ~MemoryPool() {
            blocks_.clear();
        }

        SkipListNode<T>* allocate(T value, int level) {
#ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[Pool Alloc] Requesting node for value: " << value_to_log_string(get_comparable_value(value)) << ", Level: " << level << std::endl;
#endif
            SkipListNode<T>* node_ptr = nullptr;
            std::vector<SkipListNode<T>*>& tl_cache = get_thread_local_cache();

            if (!tl_cache.empty()) {
                node_ptr = tl_cache.back();
                tl_cache.pop_back();
                // Node from tl_cache is raw memory (destructor already called by deallocate)
                // No need to call node_ptr->~SkipListNode<T>(); here.
                new (node_ptr) SkipListNode<T>(value, level); // Construct in place
#ifdef SKIPLIST_DEBUG_LOGGING
                std::cout << "[Pool Alloc] Allocated node (from thread-local cache, reconstructed) at: " << node_ptr << " for value: " << value_to_log_string(get_comparable_value(value)) << std::endl;
#endif
                return node_ptr;
            }

            node_ptr = atomic_free_list_head_.load(std::memory_order_acquire);
            while (node_ptr != nullptr) {
                SkipListNode<T>* next_node = node_ptr->forward[0].load(std::memory_order_relaxed);
                if (atomic_free_list_head_.compare_exchange_weak(node_ptr, next_node,
                                                               std::memory_order_release,
                                                               std::memory_order_relaxed)) {
#ifdef SKIPLIST_DEBUG_LOGGING
                    std::cout << "[Pool Alloc] Popped from global free list: " << node_ptr << std::endl;
#endif
                    node_ptr->~SkipListNode<T>();
                    new (node_ptr) SkipListNode<T>(value, level);
#ifdef SKIPLIST_DEBUG_LOGGING
                    std::cout << "[Pool Alloc] Allocated node at: " << node_ptr << " for value: " << value_to_log_string(get_comparable_value(value)) << std::endl;
#endif
                    return node_ptr;
                }
            }

            std::lock_guard<std::mutex> lock(pool_mutex_);
            node_ptr = atomic_free_list_head_.load(std::memory_order_acquire);
            while (node_ptr != nullptr) {
                SkipListNode<T>* next_node = node_ptr->forward[0].load(std::memory_order_relaxed);
                if (atomic_free_list_head_.compare_exchange_weak(node_ptr, next_node,
                                                               std::memory_order_release,
                                                               std::memory_order_relaxed)) {
#ifdef SKIPLIST_DEBUG_LOGGING
                    std::cout << "[Pool Alloc] Popped from global free list (under lock): " << node_ptr << std::endl;
#endif
                    node_ptr->~SkipListNode<T>();
                    new (node_ptr) SkipListNode<T>(value, level);
#ifdef SKIPLIST_DEBUG_LOGGING
                    std::cout << "[Pool Alloc] Allocated node at: " << node_ptr << " for value: " << value_to_log_string(get_comparable_value(value)) << std::endl;
#endif
                    return node_ptr;
                }
            }

#ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[Pool Alloc] Allocating new block." << std::endl;
#endif
            char* new_block_char_array = new char[BLOCK_SIZE * sizeof(SkipListNode<T>)];
            blocks_.emplace_back(std::unique_ptr<char[]>(new_block_char_array));
            SkipListNode<T>* new_block_raw_mem = reinterpret_cast<SkipListNode<T>*>(new_block_char_array);

            SkipListNode<T>* block_start_ptr = new_block_raw_mem; // reinterpret_cast<SkipListNode<T>*>(blocks_.back().get()); // new_block_raw_mem is already the correct type
            for (size_t i = 0; i < BLOCK_SIZE; ++i) {
                SkipListNode<T>* node_in_block = &block_start_ptr[i];
                new (node_in_block) SkipListNode<T>(T{}, 0);
                node_in_block->forward[0].store(atomic_free_list_head_.load(std::memory_order_relaxed), std::memory_order_relaxed);
                atomic_free_list_head_.store(node_in_block, std::memory_order_release);
            }

            node_ptr = atomic_free_list_head_.load(std::memory_order_acquire);
            while (node_ptr != nullptr) {
                SkipListNode<T>* next_node = node_ptr->forward[0].load(std::memory_order_relaxed);
                if (atomic_free_list_head_.compare_exchange_weak(node_ptr, next_node,
                                                               std::memory_order_release,
                                                               std::memory_order_relaxed)) {
#ifdef SKIPLIST_DEBUG_LOGGING
                    std::cout << "[Pool Alloc] Popped from free list (after new block): " << node_ptr << std::endl;
#endif
                    node_ptr->~SkipListNode<T>();
                    new (node_ptr) SkipListNode<T>(value, level);
#ifdef SKIPLIST_DEBUG_LOGGING
                    std::cout << "[Pool Alloc] Allocated node at: " << node_ptr << " for value: " << value_to_log_string(get_comparable_value(value)) << std::endl;
#endif
                    return node_ptr;
                }
            }
            throw std::bad_alloc();
        }

        void deallocate(SkipListNode<T>* node) {
    #ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[Pool Dealloc] Deallocating node: " << node << (node ? " with value " + value_to_log_string(get_comparable_value(node->value)) : "") << std::endl;
    #endif
            if (!node) return;

        // Destruct the current object's state (e.g. its forward array)
            node->~SkipListNode<T>();
    #ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[Pool Dealloc] Called destructor for node: " << node << std::endl;
    #endif

        // Try to add to thread-local cache first
        std::vector<SkipListNode<T>*>& tl_cache = get_thread_local_cache();
        if (tl_cache.size() < THREAD_LOCAL_CACHE_MAX_SIZE) {
            tl_cache.push_back(node); // Add raw, destructed node to cache
    #ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[Pool Dealloc] Added destructed node " << node << " to thread-local cache. Cache size: " << tl_cache.size() << std::endl;
    #endif
            return;
        }

    #ifdef SKIPLIST_DEBUG_LOGGING
        std::cout << "[Pool Dealloc] Thread-local cache full. Adding to global free list." << std::endl;
    #endif
        // Reconstruct as dummy node for the global free list (as it uses forward[0])
            try {
            new (node) SkipListNode<T>(T{}, 0); // Default construct T, level 0
            } catch (const std::bad_alloc& e) {
                std::cerr << "MemoryPool: Failed to reconstruct node as dummy for free list: " << e.what() << ". Memory leak." << std::endl;
            return; // Avoid adding a partially constructed node to free list
            } catch (...) {
                 std::cerr << "MemoryPool: Unknown exception during dummy node reconstruction. Memory leak." << std::endl;
                return;
            }
    #ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[Pool Dealloc] Reconstructed as dummy node: " << node << std::endl;
    #endif

        // Add to global CAS-based free list
            SkipListNode<T>* old_head = atomic_free_list_head_.load(std::memory_order_relaxed);
            do {
                node->forward[0].store(old_head, std::memory_order_relaxed);
            } while (!atomic_free_list_head_.compare_exchange_weak(old_head, node,
                                                                 std::memory_order_release,
                                                             std::memory_order_acquire)); // Acquire on success to sync with other CAS ops
    #ifdef SKIPLIST_DEBUG_LOGGING
        std::cout << "[Pool Dealloc] Pushed node " << node << " to global free list." << std::endl;
    #endif
        }
    };
#endif // SKIPLIST_USE_STD_ALLOC

    static const int DEFAULT_MAX_LEVEL = 16; // Renamed from MAX_LEVEL to avoid confusion
    SkipListNode<T>* header;
    std::atomic<int> currentLevel;
#ifndef SKIPLIST_USE_STD_ALLOC
    MemoryPool memory_pool_;
#endif
    // Compare key_compare_; // Moved up
    // int effective_max_level_; // Moved up with other private members

private:
    SkipListNode<T>* allocate_node(T value, int level) {
#ifdef SKIPLIST_USE_STD_ALLOC
        // Allocate memory using malloc
        void* raw_mem = malloc(sizeof(SkipListNode<T>));
        if (!raw_mem) {
            throw std::bad_alloc();
        }
        // Construct object using placement new
        return new (raw_mem) SkipListNode<T>(value, level);
#else
        return memory_pool_.allocate(value, level);
#endif
    }

    void deallocate_node(SkipListNode<T>* node) {
        if (!node) return;
#ifdef SKIPLIST_USE_STD_ALLOC
        // Call destructor explicitly
        node->~SkipListNode<T>();
        // Free the memory
        free(node);
#else
        memory_pool_.deallocate(node);
#endif
    }

public:
    explicit SkipList(int userMaxLevel = DEFAULT_MAX_LEVEL) : currentLevel(0), key_compare_(), effective_max_level_(userMaxLevel) {
        if (effective_max_level_ < 0) {
             // Or throw an exception, for now, cap at 0.
            effective_max_level_ = 0;
        }
        // Consider adding an upper cap for effective_max_level_ if necessary, e.g. 30 or some reasonable limit.
        header = allocate_node(T{}, effective_max_level_);
        this->finger_ = this->header;
    }

    ~SkipList() {
        SkipListNode<T>* current = header->forward[0].load(std::memory_order_acquire);
        while (current != nullptr) {
            SkipListNode<T>* next = current->forward[0].load(std::memory_order_relaxed);
            deallocate_node(current);
            current = next;
        }
        deallocate_node(header);
    }

    int randomLevel() {
#ifdef SKIPLIST_DEBUG_LOGGING
        std::cout << "[RandomLevel] Generating level." << std::endl;
#endif
        int level = 0;
        thread_local static std::mt19937 rng{std::random_device{}()};
        thread_local static std::uniform_real_distribution<double> dist{0.0, 1.0};
        while (dist(rng) < 0.5 && level < effective_max_level_) { // Use instance-specific max level
            level++;
        }
#ifdef SKIPLIST_DEBUG_LOGGING
        std::cout << "[RandomLevel] Generated level: " << level << std::endl;
#endif
        return level;
    }

    bool insert(T value) {
        this->finger_ = this->header;
#ifdef SKIPLIST_DEBUG_LOGGING
        std::cout << "[Insert] Value: " << value_to_log_string(get_comparable_value(value)) << std::endl;
#endif
        if (this->finger_ == nullptr) {
            this->finger_ = header;
        }

        std::vector<SkipListNode<T>*> update(effective_max_level_ + 1, nullptr); // Use instance-specific max level
        SkipListNode<T>* current_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int search_start_level = localCurrentLevel;

        SkipListNode<T>* finger_candidate = this->finger_;
        if (finger_candidate != header && key_compare_(get_comparable_value(finger_candidate->value), get_comparable_value(value))) {
            current_node = finger_candidate;
            search_start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header &&
                   key_compare_(get_comparable_value(value), get_comparable_value(finger_candidate->value))) { // Corrected: value > finger_candidate.value means finger_candidate.value < value is false, and value < finger_candidate.value is true
            current_node = header;
            search_start_level = localCurrentLevel;
        } else {
            current_node = header;
            search_start_level = localCurrentLevel;
        }

        for (int i = search_start_level; i >= 0; i--) {
            SkipListNode<T>* next_node = current_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && key_compare_(get_comparable_value(next_node->value), get_comparable_value(value))) {
                current_node = next_node;
                next_node = current_node->forward[i].load(std::memory_order_acquire);
            }
            update[i] = current_node;
        }

        if (current_node != header && search_start_level < localCurrentLevel) {
            SkipListNode<T>* current_header_scan = header;
            for (int i = localCurrentLevel; i > search_start_level; --i) {
                if (update[i] == nullptr || update[i] == finger_candidate ) {
                    SkipListNode<T>* next_node = current_header_scan->forward[i].load(std::memory_order_acquire);
                    while (next_node != nullptr && key_compare_(get_comparable_value(next_node->value), get_comparable_value(value))) {
                        current_header_scan = next_node;
                        next_node = current_header_scan->forward[i].load(std::memory_order_acquire);
                    }
                    update[i] = current_header_scan;
                } else {
                    current_header_scan = update[i];
                }
            }
        }

        SkipListNode<T>* current_val_check_node = update[0]->forward[0].load(std::memory_order_acquire);

        // if (current_val_check_node == nullptr || get_comparable_value(current_val_check_node->value) != get_comparable_value(value))
        if (current_val_check_node == nullptr || (key_compare_(get_comparable_value(current_val_check_node->value), get_comparable_value(value)) || key_compare_(get_comparable_value(value), get_comparable_value(current_val_check_node->value))) ) {
            // Element not found, insert it
            int newLevel = randomLevel();
#ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[Insert] New node level: " << newLevel << std::endl;
#endif

            if (newLevel > localCurrentLevel) {
                for (int i = localCurrentLevel + 1; i <= newLevel; i++) {
                    update[i] = header;
                }
                int expected = localCurrentLevel;
                while (newLevel > expected && !currentLevel.compare_exchange_weak(expected, newLevel, std::memory_order_release, std::memory_order_relaxed)) {
                }
                localCurrentLevel = currentLevel.load(std::memory_order_acquire);
            }

            SkipListNode<T>* newNode = allocate_node(value, newLevel);
#ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[Insert] Allocated newNode: " << newNode << std::endl;
#endif

            for (int i = 0; i <= newLevel; i++) {
                SkipListNode<T>* pred = update[i];
                SkipListNode<T>* succ = pred->forward[i].load(std::memory_order_acquire);
                newNode->forward[i].store(succ, std::memory_order_relaxed);
#ifdef SKIPLIST_DEBUG_LOGGING
                std::cout << "[Insert] Level " << i << ": pred=" << pred << ", succ=" << succ << ", newNode->forward[i]=" << newNode->forward[i].load() << std::endl;
#endif

                while (!pred->forward[i].compare_exchange_weak(succ, newNode, std::memory_order_release, std::memory_order_relaxed)) {
#ifdef SKIPLIST_DEBUG_LOGGING
                    std::cout << "[Insert] CAS failed for level " << i << ", retrying. pred=" << pred << " current_succ=" << succ << std::endl;
#endif
                    succ = pred->forward[i].load(std::memory_order_acquire); // Re-fetch successor
                    newNode->forward[i].store(succ, std::memory_order_relaxed); // Reset newNode's forward pointer for the next attempt
                }
#ifdef SKIPLIST_DEBUG_LOGGING
                std::cout << "[Insert] CAS successful for level " << i << std::endl;
#endif
            }
            if (update[0] != nullptr) {
                this->finger_ = update[0];
            } else {
                this->finger_ = header;
            }
            return true; // Inserted successfully
        } else {
            // Element already exists
            if (update[0] != nullptr) {
                this->finger_ = update[0];
            } else {
                this->finger_ = header;
            }
            return false; // Did not insert
        }
    }

    bool search(T value) const {
        this->finger_ = this->header;
#ifdef SKIPLIST_DEBUG_LOGGING
        std::cout << "[Search] Value: " << value_to_log_string(get_comparable_value(value)) << std::endl;
#endif
        if (this->finger_ == nullptr) {
            this->finger_ = header;
        }

        SkipListNode<T>* current_search_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int start_level = localCurrentLevel;
        
        SkipListNode<T>* finger_candidate = this->finger_;

        if (finger_candidate != header && key_compare_(get_comparable_value(finger_candidate->value), get_comparable_value(value))) {
            current_search_node = finger_candidate;
            start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header &&
                   key_compare_(get_comparable_value(value), get_comparable_value(finger_candidate->value))) { // Corrected: value > finger_candidate.value
            this->finger_ = header;
            current_search_node = header;
            start_level = localCurrentLevel;
        } else {
            current_search_node = header;
            start_level = localCurrentLevel;
        }

        SkipListNode<T>* predecessor_at_level0 = header;

        for (int i = start_level; i >= 0; i--) {
            SkipListNode<T>* next_node = current_search_node->forward[i].load(std::memory_order_acquire);
#ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[Search] Level " << i << ": current_search_node=" << current_search_node << ", next_node=" << next_node << std::endl;
#endif
            while (next_node != nullptr && key_compare_(get_comparable_value(next_node->value), get_comparable_value(value))) {
                current_search_node = next_node;
                next_node = current_search_node->forward[i].load(std::memory_order_acquire);
#ifdef SKIPLIST_DEBUG_LOGGING
                std::cout << "[Search] Level " << i << " (inner loop): current_search_node=" << current_search_node << ", next_node=" << next_node << std::endl;
#endif
            }
            if (i == 0) {
                predecessor_at_level0 = current_search_node;
            }
        }
        
        SkipListNode<T>* found_node = current_search_node->forward[0].load(std::memory_order_acquire);
        this->finger_ = predecessor_at_level0;
        // bool result = (found_node != nullptr && get_comparable_value(found_node->value) == get_comparable_value(value));
        bool result = (found_node != nullptr && !key_compare_(get_comparable_value(found_node->value), get_comparable_value(value)) && !key_compare_(get_comparable_value(value), get_comparable_value(found_node->value)));
#ifdef SKIPLIST_DEBUG_LOGGING
        std::cout << "[Search] Found node: " << found_node << ", Result: " << (result ? "true" : "false") << std::endl;
#endif
        return result;
    }

    bool remove(T value) {
        this->finger_ = this->header;
#ifdef SKIPLIST_DEBUG_LOGGING
        std::cout << "[Remove] Value: " << value_to_log_string(get_comparable_value(value)) << std::endl;
#endif
        if (this->finger_ == nullptr) {
            this->finger_ = header;
        }

        std::vector<SkipListNode<T>*> update(effective_max_level_ + 1, nullptr); // Use instance-specific max level
        SkipListNode<T>* current_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int search_start_level = localCurrentLevel;

        SkipListNode<T>* finger_candidate = this->finger_;
        if (finger_candidate != header && key_compare_(get_comparable_value(finger_candidate->value), get_comparable_value(value))) {
            current_node = finger_candidate;
            search_start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header &&
                   key_compare_(get_comparable_value(value), get_comparable_value(finger_candidate->value))) { // Corrected: value > finger_candidate.value
            current_node = header;
            search_start_level = localCurrentLevel;
        } else {
            current_node = header;
            search_start_level = localCurrentLevel;
        }

        for (int i = search_start_level; i >= 0; i--) {
            SkipListNode<T>* next_node = current_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && key_compare_(get_comparable_value(next_node->value), get_comparable_value(value))) {
                current_node = next_node;
                next_node = current_node->forward[i].load(std::memory_order_acquire);
            }
            update[i] = current_node;
        }

        if (current_node != header && search_start_level < localCurrentLevel) {
            SkipListNode<T>* current_header_scan = header;
            for (int i = localCurrentLevel; i > search_start_level; --i) {
                if (update[i] == nullptr || update[i] == finger_candidate) {
                    SkipListNode<T>* next_node = current_header_scan->forward[i].load(std::memory_order_acquire);
                    while (next_node != nullptr && key_compare_(get_comparable_value(next_node->value), get_comparable_value(value))) {
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

        SkipListNode<T>* target_node = update[0]->forward[0].load(std::memory_order_acquire);
#ifdef SKIPLIST_DEBUG_LOGGING
        std::cout << "[Remove] Found target_node: " << target_node << (target_node ? " with value " + value_to_log_string(get_comparable_value(target_node->value)) : "") << std::endl;
#endif

        // if (target_node != nullptr && get_comparable_value(target_node->value) == get_comparable_value(value))
        if (target_node != nullptr && !key_compare_(get_comparable_value(target_node->value), get_comparable_value(value)) && !key_compare_(get_comparable_value(value), get_comparable_value(target_node->value))) {
            for (int i = 0; i <= target_node->node_level; ++i) {
                SkipListNode<T>* pred = update[i];
                if (pred == nullptr) {
                    continue;
                }
#ifdef SKIPLIST_DEBUG_LOGGING
                std::cout << "[Remove] Unlinking at level " << i << ": pred=" << pred << ", target_node->forward[i]=" << target_node->forward[i].load() << std::endl;
#endif

                SkipListNode<T>* succ_of_target = target_node->forward[i].load(std::memory_order_relaxed);
                SkipListNode<T>* expected_target_at_pred = target_node;

                if (pred->forward[i].compare_exchange_weak(expected_target_at_pred, succ_of_target,
                                                           std::memory_order_release,
                                                           std::memory_order_relaxed)) {
#ifdef SKIPLIST_DEBUG_LOGGING
                    std::cout << "[Remove] Unlinked at level " << i << std::endl;
#endif
                } else {
                }
            }
#ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[Remove] Calling deallocate for target_node: " << target_node << std::endl;
#endif
            deallocate_node(target_node);

            int newLevel = localCurrentLevel;
            while (newLevel > 0 && header->forward[newLevel].load(std::memory_order_acquire) == nullptr) {
                newLevel--;
            }
            if (newLevel < localCurrentLevel) {
                 int expected = localCurrentLevel;
                 currentLevel.compare_exchange_weak(expected, newLevel, std::memory_order_release, std::memory_order_relaxed);
            }
            
            if (update[0] != nullptr) {
                this->finger_ = update[0];
            } else {
                this->finger_ = header;
            }
            return true;
        }

        if (update[0] != nullptr) {
            this->finger_ = update[0];
        } else {
            this->finger_ = header;
        }
        return false;
    }

    // T is the type stored in the SkipList (e.g., int, std::string, MyStruct, or std::pair<Key, Value>)
    // The 'value' parameter is what needs to be inserted or whose corresponding element needs to be assigned.
    // If T is std::pair<const K, V>, then 'value' will be std::pair<const K, V>.
    // The key for comparison is extracted by get_comparable_value(value).
    std::pair<iterator, bool> insert_or_assign(const T& value_to_insert_or_assign) {
        this->finger_ = this->header;
        KeyType key_to_find = get_comparable_value(value_to_insert_or_assign);

    #ifdef SKIPLIST_DEBUG_LOGGING
        std::cout << "[InsertOrAssign] Value: " << value_to_log_string(value_to_insert_or_assign) << " (Key: " << value_to_log_string(key_to_find) << ")" << std::endl;
    #endif

        if (this->finger_ == nullptr) {
            this->finger_ = header;
        }

        std::vector<SkipListNode<T>*> update(effective_max_level_ + 1, nullptr); // Use instance-specific max level
        SkipListNode<T>* current_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int search_start_level = localCurrentLevel;

        SkipListNode<T>* finger_candidate = this->finger_;

        // Optimized search using finger pointer
        if (finger_candidate != header && key_compare_(get_comparable_value(finger_candidate->value), key_to_find)) {
            current_node = finger_candidate;
            search_start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header && key_compare_(key_to_find, get_comparable_value(finger_candidate->value))) {
            current_node = header; // Overshot
            search_start_level = localCurrentLevel;
        } else if (finger_candidate != header && !key_compare_(get_comparable_value(finger_candidate->value), key_to_find) && !key_compare_(key_to_find, get_comparable_value(finger_candidate->value))) {
            // Finger might be on a node with an equivalent key or its predecessor.
            current_node = finger_candidate;
            search_start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else { // Default case or finger is header
            current_node = header;
            search_start_level = localCurrentLevel;
        }

        // Populate the update array (predecessors at each level)
        for (int i = search_start_level; i >= 0; i--) {
            SkipListNode<T>* next_node = current_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && key_compare_(get_comparable_value(next_node->value), key_to_find)) {
                current_node = next_node;
                next_node = current_node->forward[i].load(std::memory_order_acquire);
            }
            update[i] = current_node;
        }

        // Fill in update array from higher levels if finger search started lower
        if (current_node != header && search_start_level < localCurrentLevel) {
            SkipListNode<T>* current_header_scan = header;
            for (int i = localCurrentLevel; i > search_start_level; --i) {
                // Only fill if update[i] wasn't properly set by the finger-started scan
                // This condition might need refinement: update[i] would be nullptr or point to finger_candidate
                // if the main scan for that level was skipped due to finger starting low.
                if (update[i] == nullptr || update[i] == finger_candidate ) {
                    SkipListNode<T>* next_node = current_header_scan->forward[i].load(std::memory_order_acquire);
                    while (next_node != nullptr && key_compare_(get_comparable_value(next_node->value), key_to_find)) {
                        current_header_scan = next_node;
                        next_node = current_header_scan->forward[i].load(std::memory_order_acquire);
                    }
                    update[i] = current_header_scan;
                } else { // If update[i] is already set and not finger_candidate, it means it's a valid predecessor from finger scan
                     current_header_scan = update[i]; // Continue scan from this known predecessor
                }
            }
        }
        // Ensure update[0] is valid for finger pointer update
        if (update[0] == nullptr) update[0] = header;


        SkipListNode<T>* node_to_check = update[0]->forward[0].load(std::memory_order_acquire);

        // Check if key already exists
        if (node_to_check != nullptr && !key_compare_(get_comparable_value(node_to_check->value), key_to_find) && !key_compare_(key_to_find, get_comparable_value(node_to_check->value))) {
            // Key found, assign new value
        #ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[InsertOrAssign] Key found. Assigning value to node: " << node_to_check << std::endl;
        #endif
            // This assignment needs to be concurrency-safe if values can be updated by multiple threads.
            // For simple types, direct assignment might be okay if reads/writes are atomic or data races are acceptable for the value part.
            // For complex types, a lock might be needed around value modification, or use std::atomic for the value if possible.
            // Current SkipList does not provide per-node locks for value update.
            // We assume that if T is std::pair<const K, V>, only V should be mutable.
            // If T is a struct, its members might be updated.
            // The simplest approach here is direct assignment. Users must be aware of potential races on the value part.
            // node_to_check->value = value_to_insert_or_assign; // Assign the whole T
            if constexpr (is_std_pair_v<T>) {
                // If T is a pair, assume its first element is the const key and should not be assigned.
                // Assign only the second element (the value).
                // This relies on value_to_insert_or_assign also being a pair.
                // And KeyType_t<T> being T::first_type.
                // And the key (first part) of value_to_insert_or_assign matching the key of node_to_check->value.
                // This is guaranteed by the preceding search condition.
                node_to_check->value.second = value_to_insert_or_assign.second;
            } else {
                // If T is not a pair, assign the whole value directly.
                // This assumes T is assignable.
                node_to_check->value = value_to_insert_or_assign;
            }

            if (update[0] != nullptr) {
                this->finger_ = update[0];
            } else {
                this->finger_ = header;
            }
            return {iterator(node_to_check), false}; // false for assignment
        } else {
            // Key not found, insert new node (logic from `insert` method)
        #ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[InsertOrAssign] Key not found. Inserting new node." << std::endl;
        #endif
            int newLevel = randomLevel();
            if (newLevel > localCurrentLevel) {
                for (int i = localCurrentLevel + 1; i <= newLevel; i++) {
                    update[i] = header; // New levels' predecessors are the header
                }
                // Atomically update the skiplist's currentLevel
                int expected = localCurrentLevel;
                while (newLevel > expected && !currentLevel.compare_exchange_weak(expected, newLevel, std::memory_order_release, std::memory_order_relaxed)) {
                    // Loop if CAS fails, expected is updated by CAS on failure
                }
                // localCurrentLevel = currentLevel.load(std::memory_order_acquire); // Update local view, though newLevel is the target for this node
            }

            SkipListNode<T>* newNode = allocate_node(value_to_insert_or_assign, newLevel);
        #ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[InsertOrAssign] Allocated newNode: " << newNode << " with level " << newLevel << std::endl;
        #endif

            for (int i = 0; i <= newLevel; i++) {
                SkipListNode<T>* pred = update[i];
                // If pred is null (e.g. newLevel is higher than any existing path for this key), it should be header.
                // The loop for newLevel > localCurrentLevel should have set update[i] = header for these.
                if (pred == nullptr) pred = header;

                SkipListNode<T>* succ = pred->forward[i].load(std::memory_order_acquire);
                newNode->forward[i].store(succ, std::memory_order_relaxed);
            #ifdef SKIPLIST_DEBUG_LOGGING
                // std::cout << "[InsertOrAssign] Linking Level " << i << ": pred=" << pred << ", succ=" << succ << ", newNode->fwd=" << newNode->forward[i].load() << std::endl;
            #endif
                // CAS to link newNode into the list
                // Need to retry if pred->forward[i] changed (succ is stale)
                while (!pred->forward[i].compare_exchange_weak(succ, newNode, std::memory_order_release, std::memory_order_relaxed)) {
                #ifdef SKIPLIST_DEBUG_LOGGING
                    // std::cout << "[InsertOrAssign] CAS failed for level " << i << ", retrying. pred=" << pred << " current_succ=" << succ << std::endl;
                #endif
                    // Re-fetch successor for CAS retry
                    // And potentially re-traverse from pred if pred itself is too far back now (though less likely at this stage)
                    // For simplicity, just re-fetch succ from the original pred. More robust would be re-traversing from pred at level i.
                    // However, the update array should hold the *correct* immediate predecessors.
                    // The CAS fails if another thread inserted/deleted affecting pred->forward[i].
                    // We need to re-read pred->forward[i] to get the new succ.
                    succ = pred->forward[i].load(std::memory_order_acquire);
                    newNode->forward[i].store(succ, std::memory_order_relaxed); // Reset newNode's forward pointer for the next attempt
                }
            #ifdef SKIPLIST_DEBUG_LOGGING
                // std::cout << "[InsertOrAssign] CAS successful for level " << i << std::endl;
            #endif
            }

            if (update[0] != nullptr) {
                this->finger_ = update[0];
            } else {
                this->finger_ = header;
            }
            return {iterator(newNode), true}; // true for insertion
        }
    }

    iterator find(const KeyType& key_to_find) {
        this->finger_ = this->header;
    #ifdef SKIPLIST_DEBUG_LOGGING
        std::cout << "[Find] Key: " << value_to_log_string(key_to_find) << std::endl;
    #endif
        if (this->finger_ == nullptr) {
            this->finger_ = header;
        }

        SkipListNode<T>* current_search_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int start_level = localCurrentLevel;

        SkipListNode<T>* finger_candidate = this->finger_;

        // Use key_compare_ for comparisons involving key_to_find
        if (finger_candidate != header && key_compare_(get_comparable_value(finger_candidate->value), key_to_find)) {
            current_search_node = finger_candidate;
            start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header && key_compare_(key_to_find, get_comparable_value(finger_candidate->value))) {
            // thread_local_finger = header; // Don't reset finger on overshoot, let search path update it
            current_search_node = header;
            start_level = localCurrentLevel;
        } else if (finger_candidate != header && !key_compare_(get_comparable_value(finger_candidate->value), key_to_find) && !key_compare_(key_to_find, get_comparable_value(finger_candidate->value)) ) {
             // Finger might be pointing at the exact node or an equivalent key.
             // Start search from finger. If it's the node, loop below will find it quickly.
             current_search_node = finger_candidate;
             start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        }
        else { // Default case, or finger is header
            current_search_node = header;
            start_level = localCurrentLevel;
        }

        SkipListNode<T>* predecessor_at_level0 = header;

        for (int i = start_level; i >= 0; i--) {
            SkipListNode<T>* next_node = current_search_node->forward[i].load(std::memory_order_acquire);
        #ifdef SKIPLIST_DEBUG_LOGGING
            // This log can be verbose, consider if it's essential for find
            // std::cout << "[Find] Level " << i << ": current_search_node=" << current_search_node << ", next_node=" << next_node << std::endl;
        #endif
            while (next_node != nullptr && key_compare_(get_comparable_value(next_node->value), key_to_find)) {
                current_search_node = next_node;
                next_node = current_search_node->forward[i].load(std::memory_order_acquire);
            #ifdef SKIPLIST_DEBUG_LOGGING
                // std::cout << "[Find] Level " << i << " (inner loop): current_search_node=" << current_search_node << ", next_node=" << next_node << std::endl;
            #endif
            }
            if (i == 0) {
                predecessor_at_level0 = current_search_node;
            }
        }

        SkipListNode<T>* found_node = current_search_node->forward[0].load(std::memory_order_acquire);
        // Update finger to the predecessor of the found/insertion point
        if (predecessor_at_level0 != nullptr) { // predecessor_at_level0 could be header
             this->finger_ = predecessor_at_level0;
        }


        if (found_node != nullptr && !key_compare_(get_comparable_value(found_node->value), key_to_find) && !key_compare_(key_to_find, get_comparable_value(found_node->value))) {
        #ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[Find] Found node: " << found_node << " with key " << value_to_log_string(key_to_find) << std::endl;
        #endif
            return iterator(found_node);
        } else {
        #ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[Find] Key " << value_to_log_string(key_to_find) << " not found." << std::endl;
        #endif
            return end();
        }
    }

    const_iterator find(const KeyType& key_to_find) const {
        this->finger_ = this->header;
    #ifdef SKIPLIST_DEBUG_LOGGING
        std::cout << "[Find const] Key: " << value_to_log_string(key_to_find) << std::endl;
    #endif
        // Const version should not modify finger_.
        // However, finger_ is mutable.
        // For a const method, it's best practice to not rely on or modify mutable state like this if it's not truly thread-local.
        // A truly const find might not use the finger pointer or would need a const version of it.
        // For now, let's replicate the logic but be mindful of this.
        // A simple approach is to copy the non-const find but return const_iterator.
        // The aggressive reset of finger_ in a const method is unusual but per instructions.

        SkipListNode<T>* current_search_node = header;
        // Modifying finger_ (which find does by setting predecessor) is problematic for const if finger_ is not thread_local.
        // But per instructions, we are using a member finger_ and resetting it.

        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int start_level = localCurrentLevel;
        SkipListNode<T>* finger_candidate = this->finger_; // Read the current finger

        if (finger_candidate == nullptr) finger_candidate = header; // Ensure finger_candidate is valid

        if (finger_candidate != header && key_compare_(get_comparable_value(finger_candidate->value), key_to_find)) {
            current_search_node = finger_candidate;
            start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header && key_compare_(key_to_find, get_comparable_value(finger_candidate->value))) {
            current_search_node = header;
            start_level = localCurrentLevel;
        } else if (finger_candidate != header && !key_compare_(get_comparable_value(finger_candidate->value), key_to_find) && !key_compare_(key_to_find, get_comparable_value(finger_candidate->value)) ) {
             current_search_node = finger_candidate;
             start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else {
            current_search_node = header;
            start_level = localCurrentLevel;
        }

        for (int i = start_level; i >= 0; i--) {
            SkipListNode<T>* next_node = current_search_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && key_compare_(get_comparable_value(next_node->value), key_to_find)) {
                current_search_node = next_node;
                next_node = current_search_node->forward[i].load(std::memory_order_acquire);
            }
        }

        SkipListNode<T>* found_node = current_search_node->forward[0].load(std::memory_order_acquire);

        if (found_node != nullptr && !key_compare_(get_comparable_value(found_node->value), key_to_find) && !key_compare_(key_to_find, get_comparable_value(found_node->value))) {
        #ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[Find const] Found node: " << found_node << " with key " << value_to_log_string(key_to_find) << std::endl;
        #endif
            return const_iterator(found_node);
        } else {
        #ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[Find const] Key " << value_to_log_string(key_to_find) << " not found." << std::endl;
        #endif
            return cend(); // Use cend() for const_iterator
        }
    }

    void clear() {
    #ifdef SKIPLIST_DEBUG_LOGGING
        std::cout << "[Clear] Clearing all elements from the skiplist." << std::endl;
    #endif

        SkipListNode<T>* current = header->forward[0].load(std::memory_order_acquire);
        while (current != nullptr) {
            SkipListNode<T>* next = current->forward[0].load(std::memory_order_relaxed); // Relaxed is fine as we are clearing and no other thread should be modifying this path concurrently in a way that affects us just getting the next pointer for deletion.
        #ifdef SKIPLIST_DEBUG_LOGGING
            std::cout << "[Clear] Deallocating node with value: " << value_to_log_string(current->value) << " (Key: " << value_to_log_string(get_comparable_value(current->value)) << ")" << std::endl;
        #endif
            deallocate_node(current);
            current = next;
        }

        // Reset header node's forward pointers
        for (int i = 0; i <= effective_max_level_; ++i) { // Use instance-specific max level
            header->forward[i].store(nullptr, std::memory_order_release); // Release semantics for subsequent operations by other threads
        }

        // Reset currentLevel
        currentLevel.store(0, std::memory_order_release);

        // Reset finger_ for the current thread, as all nodes are gone.
        // Setting it to header is a safe choice for the current thread.
        this->finger_ = header;

    #ifdef SKIPLIST_DEBUG_LOGGING
        std::cout << "[Clear] Skiplist cleared. Current level set to 0." << std::endl;
    #endif
    }

    void display() const {
        std::cout << "\n=== Skip List Structure ===" << std::endl;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);

        for (int i = localCurrentLevel; i >= 0; i--) {
            std::cout << "Level " << std::setw(2) << i << ": ";
            SkipListNode<T>* node = header->forward[i].load(std::memory_order_acquire);
            
            while (node != nullptr) {
                std::cout << value_to_log_string(node->value) << " -> ";
                node = node->forward[i].load(std::memory_order_acquire);
            }
            std::cout << "NULL" << std::endl;
        }
        std::cout << std::endl;
    }

    void printValues() const {
        std::cout << "Values in skip list: ";
        SkipListNode<T>* node = header->forward[0].load(std::memory_order_acquire);
        
        while (node != nullptr) {
            std::cout << value_to_log_string(node->value) << " ";
            node = node->forward[0].load(std::memory_order_acquire);
        }
        std::cout << std::endl;
    }

    bool empty() const {
        return header->forward[0].load(std::memory_order_acquire) == nullptr;
    }

    std::vector<T> toVector() const {
        std::vector<T> vec;
        SkipListNode<T>* node = header->forward[0].load(std::memory_order_acquire);
        while (node != nullptr) {
            vec.push_back(node->value);
            node = node->forward[0].load(std::memory_order_acquire);
        }
        return vec;
    }

    int size() const {
        int count = 0;
        SkipListNode<T>* node = header->forward[0].load(std::memory_order_acquire);
        
        while (node != nullptr) {
            count++;
            node = node->forward[0].load(std::memory_order_acquire);
        }
        return count;
    }

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

    std::vector<T> rangeQuery(T minVal, T maxVal) const {
        std::vector<T> result;
        SkipListNode<T>* current_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        
        for (int i = localCurrentLevel; i >= 0; i--) {
            SkipListNode<T>* next_node = current_node->forward[i].load(std::memory_order_acquire);
            // while (next_node != nullptr && get_comparable_value(next_node->value) < get_comparable_value(minVal))
            while (next_node != nullptr && key_compare_(get_comparable_value(next_node->value), get_comparable_value(minVal))) {
                current_node = next_node;
                next_node = current_node->forward[i].load(std::memory_order_acquire);
            }
        }
        
        current_node = current_node->forward[0].load(std::memory_order_acquire);
        
        // while (current_node != nullptr && get_comparable_value(current_node->value) <= get_comparable_value(maxVal))
        while (current_node != nullptr && !key_compare_(get_comparable_value(maxVal), get_comparable_value(current_node->value))) {
            // if (get_comparable_value(current_node->value) >= get_comparable_value(minVal))
            if (!key_compare_(get_comparable_value(current_node->value), get_comparable_value(minVal))) {
                result.push_back(current_node->value);
            }
            current_node = current_node->forward[0].load(std::memory_order_acquire);
        }
        
        return result;
    }

    void insert_bulk(const std::vector<T>& values) {
        if (values.empty()) {
            return;
        }
        std::vector<T> sorted_values = values;
        std::sort(sorted_values.begin(), sorted_values.end());
        for (const T& value : sorted_values) {
            this->insert(value);
        }
    }

    size_t remove_bulk(const std::vector<T>& values) {
        if (values.empty()) {
            return 0;
        }
        size_t removed_count = 0;
        std::vector<T> sorted_values = values;
        std::sort(sorted_values.begin(), sorted_values.end());
        for (const T& value : sorted_values) {
            if (this->remove(value)) {
                removed_count++;
            }
        }
        return removed_count;
    }
};


#include <iostream>
#include <vector>
#include <algorithm> // For std::sort
#include <atomic> // Added for std::atomic
#include <mutex>  // Added for std::mutex
#include <random>
#include <climits>
#include <iomanip>
#include <string> // Added for std::string
#include <list>   // For MemoryPool's free list
#include <memory> // For std::unique_ptr
#include <iterator> // For std::forward_iterator_tag and iterator traits
#include <cstddef>  // For std::ptrdiff_t
#include <utility> // For std::pair
#include <type_traits> // For std::is_same, std::decay_t, etc.

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

// Helper to convert value T to string for logging
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
template<typename T> class SkipList;

template<typename T>
class SkipListNode {
public:
    T value;
    std::atomic<SkipListNode<T>*>* forward; // Array of atomic pointers
    int node_level; // Actual level of this node

    SkipListNode(T val, int level) : value(val), node_level(level) {
        std::cout << "[Node Ctor] Value: " << value_to_log_string(get_comparable_value(val)) << ", Level: " << level << ", Addr: " << this << std::endl;
        forward = new std::atomic<SkipListNode<T>*>[level + 1];
        for (int i = 0; i <= level; ++i) {
            forward[i].store(nullptr, std::memory_order_relaxed);
        }
    }

    ~SkipListNode() {
        std::cout << "[Node Dtor] Value: " << value_to_log_string(get_comparable_value(value)) << ", Level: " << node_level << ", Addr: " << this << ", Deleting forward: " << forward << std::endl;
        delete[] forward;
    }
};

template<typename T>
class SkipList {
public:
    thread_local static SkipListNode<T>* thread_local_finger;

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
            std::cout << "[Pool Alloc] Requesting node for value: " << value_to_log_string(get_comparable_value(value)) << ", Level: " << level << std::endl;
            SkipListNode<T>* node_ptr = nullptr;
            std::vector<SkipListNode<T>*>& tl_cache = get_thread_local_cache();

            if (!tl_cache.empty()) {
                node_ptr = tl_cache.back();
                tl_cache.pop_back();
                node_ptr->~SkipListNode<T>();
                new (node_ptr) SkipListNode<T>(value, level);
                std::cout << "[Pool Alloc] Allocated node (from thread-local cache) at: " << node_ptr << " for value: " << value_to_log_string(get_comparable_value(value)) << std::endl;
                return node_ptr;
            }

            node_ptr = atomic_free_list_head_.load(std::memory_order_acquire);
            while (node_ptr != nullptr) {
                SkipListNode<T>* next_node = node_ptr->forward[0].load(std::memory_order_relaxed);
                if (atomic_free_list_head_.compare_exchange_weak(node_ptr, next_node,
                                                               std::memory_order_release,
                                                               std::memory_order_relaxed)) {
                    std::cout << "[Pool Alloc] Popped from global free list: " << node_ptr << std::endl;
                    node_ptr->~SkipListNode<T>();
                    new (node_ptr) SkipListNode<T>(value, level);
                    std::cout << "[Pool Alloc] Allocated node at: " << node_ptr << " for value: " << value_to_log_string(get_comparable_value(value)) << std::endl;
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
                    std::cout << "[Pool Alloc] Popped from global free list (under lock): " << node_ptr << std::endl;
                    node_ptr->~SkipListNode<T>();
                    new (node_ptr) SkipListNode<T>(value, level);
                    std::cout << "[Pool Alloc] Allocated node at: " << node_ptr << " for value: " << value_to_log_string(get_comparable_value(value)) << std::endl;
                    return node_ptr;
                }
            }

            std::cout << "[Pool Alloc] Allocating new block." << std::endl;
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
                    std::cout << "[Pool Alloc] Popped from free list (after new block): " << node_ptr << std::endl;
                    node_ptr->~SkipListNode<T>();
                    new (node_ptr) SkipListNode<T>(value, level);
                    std::cout << "[Pool Alloc] Allocated node at: " << node_ptr << " for value: " << value_to_log_string(get_comparable_value(value)) << std::endl;
                    return node_ptr;
                }
            }
            throw std::bad_alloc();
        }

        void deallocate(SkipListNode<T>* node) {
            std::cout << "[Pool Dealloc] Deallocating node: " << node << (node ? " with value " + value_to_log_string(get_comparable_value(node->value)) : "") << std::endl;
            if (!node) return;

            node->~SkipListNode<T>();
            std::cout << "[Pool Dealloc] Called destructor for node: " << node << std::endl;

            try {
                new (node) SkipListNode<T>(T{}, 0);
            } catch (const std::bad_alloc& e) {
                std::cerr << "MemoryPool: Failed to reconstruct node as dummy for free list: " << e.what() << ". Memory leak." << std::endl;
                return;
            } catch (...) {
                 std::cerr << "MemoryPool: Unknown exception during dummy node reconstruction. Memory leak." << std::endl;
                return;
            }
            std::cout << "[Pool Dealloc] Reconstructed as dummy node: " << node << std::endl;

            SkipListNode<T>* old_head = atomic_free_list_head_.load(std::memory_order_relaxed);
            do {
                node->forward[0].store(old_head, std::memory_order_relaxed);
            } while (!atomic_free_list_head_.compare_exchange_weak(old_head, node,
                                                                 std::memory_order_release,
                                                                 std::memory_order_acquire));
            std::cout << "[Pool Dealloc] Pushing to free list: " << node << std::endl;
        }
    };

    static const int MAX_LEVEL = 16;
    SkipListNode<T>* header;
    std::atomic<int> currentLevel;
    MemoryPool memory_pool_;

public:
    SkipList() : currentLevel(0) {
        header = new SkipListNode<T>(T{}, MAX_LEVEL);
    }

    ~SkipList() {
        SkipListNode<T>* current = header->forward[0].load(std::memory_order_acquire);
        while (current != nullptr) {
            SkipListNode<T>* next = current->forward[0].load(std::memory_order_relaxed);
            memory_pool_.deallocate(current);
            current = next;
        }
        delete header;
    }

    int randomLevel() {
        std::cout << "[RandomLevel] Generating level." << std::endl;
        int level = 0;
        thread_local static std::mt19937 rng{std::random_device{}()};
        thread_local static std::uniform_real_distribution<double> dist{0.0, 1.0};
        while (dist(rng) < 0.5 && level < MAX_LEVEL) {
            level++;
        }
        std::cout << "[RandomLevel] Generated level: " << level << std::endl;
        return level;
    }

    void insert(T value) {
        std::cout << "[Insert] Value: " << value_to_log_string(get_comparable_value(value)) << std::endl;
        if (thread_local_finger == nullptr) {
            thread_local_finger = header;
        }

        std::vector<SkipListNode<T>*> update(MAX_LEVEL + 1, nullptr);
        SkipListNode<T>* current_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int search_start_level = localCurrentLevel;

        SkipListNode<T>* finger_candidate = thread_local_finger;
        if (finger_candidate != header && get_comparable_value(finger_candidate->value) < get_comparable_value(value)) {
            current_node = finger_candidate;
            search_start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header &&
                   get_comparable_value(finger_candidate->value) > get_comparable_value(value)) {
            current_node = header;
            search_start_level = localCurrentLevel;
        } else {
            current_node = header;
            search_start_level = localCurrentLevel;
        }

        for (int i = search_start_level; i >= 0; i--) {
            SkipListNode<T>* next_node = current_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && get_comparable_value(next_node->value) < get_comparable_value(value)) {
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
                    while (next_node != nullptr && get_comparable_value(next_node->value) < get_comparable_value(value)) {
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

        if (current_val_check_node == nullptr || get_comparable_value(current_val_check_node->value) != get_comparable_value(value)) {
            int newLevel = randomLevel();
            std::cout << "[Insert] New node level: " << newLevel << std::endl;

            if (newLevel > localCurrentLevel) {
                for (int i = localCurrentLevel + 1; i <= newLevel; i++) {
                    update[i] = header;
                }
                int expected = localCurrentLevel;
                while (newLevel > expected && !currentLevel.compare_exchange_weak(expected, newLevel, std::memory_order_release, std::memory_order_relaxed)) {
                }
                localCurrentLevel = currentLevel.load(std::memory_order_acquire);
            }

            SkipListNode<T>* newNode = memory_pool_.allocate(value, newLevel);
            std::cout << "[Insert] Allocated newNode: " << newNode << std::endl;

            for (int i = 0; i <= newLevel; i++) {
                SkipListNode<T>* pred = update[i];
                SkipListNode<T>* succ = pred->forward[i].load(std::memory_order_acquire);
                newNode->forward[i].store(succ, std::memory_order_relaxed);
                std::cout << "[Insert] Level " << i << ": pred=" << pred << ", succ=" << succ << ", newNode->forward[i]=" << newNode->forward[i].load() << std::endl;

                while (!pred->forward[i].compare_exchange_weak(succ, newNode, std::memory_order_release, std::memory_order_relaxed)) {
                    std::cout << "[Insert] CAS failed for level " << i << ", retrying." << std::endl;
                    newNode->forward[i].store(succ, std::memory_order_relaxed);
                }
                std::cout << "[Insert] CAS successful for level " << i << std::endl;
            }
        }
        if (update[0] != nullptr) {
            thread_local_finger = update[0];
        } else {
            thread_local_finger = header;
        }
    }

    bool search(T value) const {
        std::cout << "[Search] Value: " << value_to_log_string(get_comparable_value(value)) << std::endl;
        if (SkipList<T>::thread_local_finger == nullptr) {
            SkipList<T>::thread_local_finger = header;
        }

        SkipListNode<T>* current_search_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int start_level = localCurrentLevel;
        
        SkipListNode<T>* finger_candidate = SkipList<T>::thread_local_finger;

        if (finger_candidate != header && get_comparable_value(finger_candidate->value) < get_comparable_value(value)) {
            current_search_node = finger_candidate;
            start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header &&
                   get_comparable_value(finger_candidate->value) > get_comparable_value(value)) {
            SkipList<T>::thread_local_finger = header;
            current_search_node = header;
            start_level = localCurrentLevel;
        } else {
            current_search_node = header;
            start_level = localCurrentLevel;
        }

        SkipListNode<T>* predecessor_at_level0 = header;

        for (int i = start_level; i >= 0; i--) {
            SkipListNode<T>* next_node = current_search_node->forward[i].load(std::memory_order_acquire);
            std::cout << "[Search] Level " << i << ": current_search_node=" << current_search_node << ", next_node=" << next_node << std::endl;
            while (next_node != nullptr && get_comparable_value(next_node->value) < get_comparable_value(value)) {
                current_search_node = next_node;
                next_node = current_search_node->forward[i].load(std::memory_order_acquire);
                std::cout << "[Search] Level " << i << " (inner loop): current_search_node=" << current_search_node << ", next_node=" << next_node << std::endl;
            }
            if (i == 0) {
                predecessor_at_level0 = current_search_node;
            }
        }
        
        SkipListNode<T>* found_node = current_search_node->forward[0].load(std::memory_order_acquire);
        SkipList<T>::thread_local_finger = predecessor_at_level0;
        bool result = (found_node != nullptr && get_comparable_value(found_node->value) == get_comparable_value(value));
        std::cout << "[Search] Found node: " << found_node << ", Result: " << (result ? "true" : "false") << std::endl;
        return result;
    }

    bool remove(T value) {
        std::cout << "[Remove] Value: " << value_to_log_string(get_comparable_value(value)) << std::endl;
        if (thread_local_finger == nullptr) {
            thread_local_finger = header;
        }

        std::vector<SkipListNode<T>*> update(MAX_LEVEL + 1, nullptr);
        SkipListNode<T>* current_node = header;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);
        int search_start_level = localCurrentLevel;

        SkipListNode<T>* finger_candidate = thread_local_finger;
        if (finger_candidate != header && get_comparable_value(finger_candidate->value) < get_comparable_value(value)) {
            current_node = finger_candidate;
            search_start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header &&
                   get_comparable_value(finger_candidate->value) > get_comparable_value(value)) {
            current_node = header;
            search_start_level = localCurrentLevel;
        } else {
            current_node = header;
            search_start_level = localCurrentLevel;
        }

        for (int i = search_start_level; i >= 0; i--) {
            SkipListNode<T>* next_node = current_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && get_comparable_value(next_node->value) < get_comparable_value(value)) {
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
                    while (next_node != nullptr && get_comparable_value(next_node->value) < get_comparable_value(value)) {
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
        std::cout << "[Remove] Found target_node: " << target_node << (target_node ? " with value " + value_to_log_string(get_comparable_value(target_node->value)) : "") << std::endl;

        if (target_node != nullptr && get_comparable_value(target_node->value) == get_comparable_value(value)) {
            for (int i = 0; i <= target_node->node_level; ++i) {
                SkipListNode<T>* pred = update[i];
                if (pred == nullptr) {
                    continue;
                }
                std::cout << "[Remove] Unlinking at level " << i << ": pred=" << pred << ", target_node->forward[i]=" << target_node->forward[i].load() << std::endl;

                SkipListNode<T>* succ_of_target = target_node->forward[i].load(std::memory_order_relaxed);
                SkipListNode<T>* expected_target_at_pred = target_node;

                if (pred->forward[i].compare_exchange_weak(expected_target_at_pred, succ_of_target,
                                                           std::memory_order_release,
                                                           std::memory_order_relaxed)) {
                    std::cout << "[Remove] Unlinked at level " << i << std::endl;
                } else {
                }
            }
            std::cout << "[Remove] Calling deallocate for target_node: " << target_node << std::endl;
            memory_pool_.deallocate(target_node);

            int newLevel = localCurrentLevel;
            while (newLevel > 0 && header->forward[newLevel].load(std::memory_order_acquire) == nullptr) {
                newLevel--;
            }
            if (newLevel < localCurrentLevel) {
                 int expected = localCurrentLevel;
                 currentLevel.compare_exchange_weak(expected, newLevel, std::memory_order_release, std::memory_order_relaxed);
            }
            
            if (update[0] != nullptr) {
                thread_local_finger = update[0];
            } else {
                thread_local_finger = header;
            }
            return true;
        }

        if (update[0] != nullptr) {
            thread_local_finger = update[0];
        } else {
            thread_local_finger = header;
        }
        return false;
    }

    void display() const {
        std::cout << "\n=== Skip List Structure ===" << std::endl;
        int localCurrentLevel = currentLevel.load(std::memory_order_acquire);

        for (int i = localCurrentLevel; i >= 0; i--) {
            std::cout << "Level " << std::setw(2) << i << ": ";
            SkipListNode<T>* node = header->forward[i].load(std::memory_order_acquire);
            
            while (node != nullptr) {
                if constexpr (is_std_pair_v<T>) {
                    std::cout << "<" << node->value.first << ":" << node->value.second << "> -> ";
                } else if constexpr (std::is_same_v<T, std::string>) {
                    std::cout << "\"" << node->value << "\"" << " -> ";
                } else {
                    std::cout << std::setw(4) << node->value << " -> ";
                }
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
            if constexpr (is_std_pair_v<T>) {
                std::cout << "<" << node->value.first << ":" << node->value.second << "> ";
            } else if constexpr (std::is_same_v<T, std::string>) {
                std::cout << "\"" << node->value << "\"" << " ";
            } else {
                std::cout << node->value << " ";
            }
            node = node->forward[0].load(std::memory_order_acquire);
        }
        std::cout << std::endl;
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
            while (next_node != nullptr && get_comparable_value(next_node->value) < get_comparable_value(minVal)) {
                current_node = next_node;
                next_node = current_node->forward[i].load(std::memory_order_acquire);
            }
        }
        
        current_node = current_node->forward[0].load(std::memory_order_acquire);
        
        while (current_node != nullptr && get_comparable_value(current_node->value) <= get_comparable_value(maxVal)) {
            if (get_comparable_value(current_node->value) >= get_comparable_value(minVal)) {
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

template<typename T>
thread_local SkipListNode<T>* SkipList<T>::thread_local_finger = nullptr;

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

// No need for forward declarations of specializations if they are removed.
// template<> class SkipListNode<int>;
// template<> class SkipList<int>;
// template<> class SkipListNode<std::string>;
// template<> class SkipList<std::string>;

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

    // thread_local_finger serves as a thread-local hint to a recently accessed node,
    // typically the predecessor of the last inserted, removed, or searched-for element.
    // Its purpose is to potentially speed up subsequent operations by allowing searches
    // to start closer to the target value, rather than always from the list header.
    // The effectiveness of this heuristic can vary based on access patterns.
    // Importantly, due to concurrent operations and the absence of Safe Memory
    // Reclamation (SMR) in this implementation, the node pointed to by thread_local_finger
    // might be deleted and its memory reused (e.g., reconstructed as a "dummy" node
    // by the MemoryPool). Operations using the finger (insert, search, remove) must
    // therefore be robust against such changes. They typically achieve this by
    // re-validating the finger's state or by always being prepared to fall back to
    // a full search from the header if the finger is not helpful or valid.
    thread_local static SkipListNode<T>* thread_local_finger;

public:
    // Forward iterator for SkipList
    class iterator {
    public:
        // Iterator traits
        using iterator_category = std::forward_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        SkipListNode<T>* current_node; // Made public for const_iterator conversion and simplicity

        explicit iterator(SkipListNode<T>* node) : current_node(node) {}

        reference operator*() const {
            return current_node->value;
        }
        pointer operator->() const {
            return &(current_node->value);
        }

        // Pre-increment
        iterator& operator++() {
            if (current_node) {
                current_node = current_node->forward[0].load(std::memory_order_acquire);
            }
            return *this;
        }

        // Post-increment
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

    // Const forward iterator for SkipList
    class const_iterator {
    public:
        // Iterator traits
        using iterator_category = std::forward_iterator_tag;
        using value_type        = const T; // Note: const T
        using difference_type   = std::ptrdiff_t;
        using pointer           = const T*; // Note: const T*
        using reference         = const T&;  // Note: const T&

        SkipListNode<T>* current_node; // Made public for simplicity

        explicit const_iterator(SkipListNode<T>* node) : current_node(node) {}

        // Constructor from non-const iterator
        const_iterator(const iterator& other) : current_node(other.current_node) {}

        reference operator*() const {
            return current_node->value;
        }
        pointer operator->() const {
            return &(current_node->value);
        }

        // Pre-increment
        const_iterator& operator++() {
            if (current_node) {
                current_node = current_node->forward[0].load(std::memory_order_acquire);
            }
            return *this;
        }

        // Post-increment
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
    // The MemoryPool manages memory for SkipListNodes in blocks to reduce overhead
    // of individual allocations and to enable lock-free node reuse.
    // Overall strategy:
    // 1. Allocation in Blocks: When new nodes are needed and the free list is empty,
    //    the pool allocates a large block of memory that can hold multiple SkipListNodes.
    //    These raw memory slots are then prepared and added to a global lock-free stack.
    // 2. Lock-Free Stack for Freed Nodes: Freed nodes are returned to `atomic_free_list_head_`,
    //    which acts as a global lock-free stack. This allows threads to deallocate and
    //    allocate nodes with minimal contention.
    // 3. Dummy Node Reconstruction: Before a node is pushed onto the free list (after its
    //    `~SkipListNode()` is called by `deallocate`), it is reconstructed via placement new
    //    as a "dummy" node. This dummy node is typically initialized with a default T value
    //    and always at level 0. This crucial step ensures that its `forward[0]` pointer
    //    is valid and can be safely used by the lock-free stack mechanism to link free nodes.
    // 4. Allocation from Pool: When a node is requested via `allocate()`:
    //    a. It first tries a thread-local cache (if implemented, currently a placeholder).
    //    b. Then, it attempts to pop a node from the global `atomic_free_list_head_`.
    //    c. If a node is obtained (which was a "dummy" node), its destructor (`~SkipListNode()`)
    //       is called to clean up the dummy state (e.g., its minimal forward array).
    //    d. Then, placement new is used again to construct the actual `SkipListNode` with the
    //       user-specified value and level onto that same memory location.
    // 5. SMR Implication: This strategy of immediate reuse and altering the node's structure
    //    (value, level, forward pointers) is highly efficient but is the primary reason why
    //    a Safe Memory Reclamation (SMR) mechanism (like Hazard Pointers or Epoch-Based
    //    Reclamation) is critical for the main skip list operations (insert, remove, search).
    //    Without SMR, other threads might access a node that has been deallocated and
    //    repurposed, leading to use-after-free errors or data corruption.
    class MemoryPool {
    private:
        // Helper to access thread-local cache
        // Using a function-local static for thread_local to ensure robust header-only compatibility (pre-C++17 static member defs)
        static std::vector<SkipListNode<T>*>& get_thread_local_cache() {
            thread_local std::vector<SkipListNode<T>*> tl_cache;
            return tl_cache;
        }

    public:
        static const size_t BLOCK_SIZE = 64; // Number of nodes per block
        static const size_t THREAD_LOCAL_CACHE_MAX_SIZE = 16; // Max nodes in thread-local cache

        std::atomic<SkipListNode<T>*> atomic_free_list_head_{nullptr}; // Head of the global lock-free stack
        std::vector<std::unique_ptr<SkipListNode<T>[]>> blocks_; // Store allocated blocks
        std::mutex pool_mutex_; // Mutex now primarily protects block allocation and potentially batch refills of global pool

        MemoryPool() = default;

        ~MemoryPool() {
            // Destructor for SkipListNode objects in blocks is implicitly handled
            // by unique_ptr if they were constructed.
            // However, we explicitly call destructors in deallocate.
            // The primary role here is to free the raw memory of blocks.
            // Nodes from atomic_free_list_head_ point into memory owned by blocks_.
            blocks_.clear();
        }

        SkipListNode<T>* allocate(T value, int level) {
            SkipListNode<T>* node_ptr = nullptr;
            std::vector<SkipListNode<T>*>& tl_cache = get_thread_local_cache();

            // 1. Try thread-local cache first
            if (!tl_cache.empty()) {
                node_ptr = tl_cache.back();
                tl_cache.pop_back();

                node_ptr->~SkipListNode<T>(); // Call destructor of the dummy node
                new (node_ptr) SkipListNode<T>(value, level); // Construct actual node
                return node_ptr;
            }

            // 2. Try global lock-free stack
            node_ptr = atomic_free_list_head_.load(std::memory_order_acquire);
            while (node_ptr != nullptr) {
                SkipListNode<T>* next_node = node_ptr->forward[0].load(std::memory_order_relaxed);
                if (atomic_free_list_head_.compare_exchange_weak(node_ptr, next_node,
                                                               std::memory_order_release,
                                                               std::memory_order_relaxed)) {
                    node_ptr->~SkipListNode<T>(); // Destructor for dummy node
                    new (node_ptr) SkipListNode<T>(value, level); // Construct actual node
                    return node_ptr;
                }
            }

            // 3. If both caches are empty, fall back to block allocation (locked)
            std::lock_guard<std::mutex> lock(pool_mutex_);
            // Double-check global list first (another thread might have populated it or allocated a block)
            node_ptr = atomic_free_list_head_.load(std::memory_order_acquire);
            while (node_ptr != nullptr) {
                SkipListNode<T>* next_node = node_ptr->forward[0].load(std::memory_order_relaxed);
                if (atomic_free_list_head_.compare_exchange_weak(node_ptr, next_node,
                                                               std::memory_order_release,
                                                               std::memory_order_relaxed)) {
                    node_ptr->~SkipListNode<T>(); // Destructor for dummy
                    new (node_ptr) SkipListNode<T>(value, level); // Construct actual
                    return node_ptr;
                }
            }

            // Still empty: Allocate a new block
            // Using raw new[] and managing with unique_ptr for the block.
            // Nodes themselves are not constructed here, just memory allocated.
            SkipListNode<T>* new_block_raw = static_cast<SkipListNode<T>*>(::operator new[](BLOCK_SIZE * sizeof(SkipListNode<T>)));
            blocks_.emplace_back(std::unique_ptr<SkipListNode<T>[]>(new_block_raw));


            // Add nodes from the new block to the free list (lock-free stack)
            // Push them in reverse order so that popping gives lower addresses first (optional detail)
            for (size_t i = 0; i < BLOCK_SIZE; ++i) {
                SkipListNode<T>* current_new_node = &new_block_raw[i];
                // current_new_node->forward needs to be valid for the push operation.
                // Since SkipListNode's constructor isn't called yet, ensure forward[0] can be used.
                // The SkipListNode struct has `forward` as `std::atomic<SkipListNode<T>*>*`.
                // This raw memory needs to be interpretable as having such a pointer.
                // This is tricky. A SkipListNode isn't constructed yet.
                // A simple approach: construct a temporary SkipListNode just to get 'forward' array allocated,
                // then immediately destruct it before adding to free list. This is hacky.
                // Better: The 'forward' pointer itself within the node *must* be used by the lock-free stack.
                // Let's assume that the memory for `forward` array is allocated by placement new later.
                // For the free list, we only need one pointer.
                // We can repurpose one of the fields of SkipListNode, e.g. forward[0] if node_level allows.
                // The current SkipListNode constructor takes `level`, so `forward` is `new std::atomic<SkipListNode<T>*>[level + 1];`
                // This means `forward` itself is a pointer. The memory for this array is allocated during placement new.
                // This is problematic for using node->forward[0] when node is just raw memory.

                // Let's simplify: For the free list, we will use the node pointer itself as a linked list.
                // The actual SkipListNode::forward array will only be initialized by placement new.
                // So, when a node is in the free list, its first few bytes (where `value` would be)
                // can be temporarily repurposed to store the "next" pointer for the free list.
                // This requires careful casting and is generally unsafe if types are not compatible.

                // A cleaner way for a lock-free stack of `SkipListNode<T>*` is to have a helper struct:
                /*
                struct FreeNode {
                    SkipListNode<T>* actual_node;
                    std::atomic<FreeNode*> next_in_freelist;
                };
                std::atomic<FreeNode*> atomic_free_list_head_helper;
                */
                // This adds overhead.

                // Current strategy: use node->forward[0] as the "next" pointer in the free list.
                // This implies that the memory at node->forward[0] is valid *before* placement new.
                // The current SkipListNode allocates `forward` array in its constructor.
                // This means `new_block_raw[i].forward` is not yet valid.

                // Re-think: The free list should store raw pointers. The "next" pointer for the
                // lock-free stack must be embedded within the node's memory.
                // Let's assume `SkipListNode<T>` always has `forward` pointing to an array of at least one element.
                // When a node is raw memory (before placement new), we can't rely on its members being initialized.
                // However, after it's been constructed once and then deallocated, its `forward` pointer (the pointer itself, not the array elements)
                // would be valid if the destructor doesn't nullify it, and `forward[0]` could be used.
                // But for brand new blocks, this is an issue.

                // Let's assume that the `forward` pointer *within* the `SkipListNode` object
                // (i.e., `SkipListNode<T>::forward`) is what we use.
                // For a newly allocated block `new_block_raw`, each `&new_block_raw[i]` is just memory.
                // We cannot safely do `new_block_raw[i].forward[0].store(...)`.

                // Simplification for now: The block allocation part remains under mutex.
                // When new block is allocated, we will temporarily use a std::list to gather them,
                // then transfer to the atomic stack. This is inefficient.

                // Corrected approach for adding new block nodes to atomic stack:
                // The node that is put on the free list *must* have its `forward[0]` usable.
                // This means we must at least initialize the `forward` pointer itself.
                // The current code `new SkipListNode<T>[BLOCK_SIZE]()` value-initializes the block.
                // For a class type, this means default constructor. If SkipListNode had a default ctor
                // that initializes `forward` (e.g. to nullptr or a small array), it might work.
                // But it takes (value, level).

                // Let's go with the original assumption that node->forward[0] can be used.
                // This implies that `SkipListNode<T>` instances in `new_block_raw` are sufficiently initialized
                // for `forward[0]` to be accessed. The `new SkipListNode<T>[BLOCK_SIZE]()` might do this if
                // SkipListNode had a default constructor, but it doesn't.
                // `operator new[]` followed by placement new for each element is safer.
                // The current code `SkipListNode<T>* new_block_raw = new SkipListNode<T>[BLOCK_SIZE]();`
                // will attempt to call default constructor for each element, which will fail as it's deleted or not available.
                // It should be `static_cast<SkipListNode<T>*>(::operator new[](BLOCK_SIZE * sizeof(SkipListNode<T>)))`
                // as done above.

                // For each raw memory slot `current_new_node` in the new block:
                // We need to make `current_new_node->forward[0]` a valid atomic pointer to link it in the stack.
                // This is the core issue: `forward` is `std::atomic<SkipListNode<T>*>*`, meaning it's a pointer to an array of atomics.
                // This pointer `forward` itself is initialized in the `SkipListNode` constructor.
                // So, for raw memory, `current_new_node->forward` is uninitialized.

                // Solution: The items in the free list are `SkipListNode<T>*`.
                // We can repurpose the first field of `SkipListNode<T>` (which is `value`)
                // to store the `next` pointer if `sizeof(T)` is >= `sizeof(SkipListNode<T>*)`.
                // This is getting very complex and unsafe.

                // Safest modification for `free_list_` to lock-free stack:
                // The elements of the stack will be `SkipListNode<T>*`.
                // Each `SkipListNode<T>` object, when on the free list, will use its `forward[0]`
                // member to point to the next free node.
                // This requires that `forward[0]` is valid memory.
                // This means `SkipListNode` objects must have their `forward` array allocated even when "free".
                // This is naturally true for nodes that have been used and then deallocated.
                // For brand new nodes from a block:
                // We must construct them (perhaps with a dummy level 0) to initialize `forward`,
                // then destruct `value` part, then add to free list. This is very messy.

                // Let's try a simpler lock-free stack that doesn't rely on node internals as much initially.
                // We can have a side structure for the free list pointers if needed.
                // Or, stick to the current design: `node->forward[0]` is the next pointer.
                // This means when a node is allocated from a new block, we must ensure its `forward` pointer
                // is initialized and points to a valid `std::atomic<SkipListNode<T>*>[1]` at least.

                // For now, let's assume new_block_raw nodes are not immediately ready for lock-free list.
                // We will populate a temporary list under lock and then transfer.
                // Or, the first node from the block is returned, and others are pushed to free list.
                std::list<SkipListNode<T>*> temp_free_list;
                for (size_t i = 0; i < BLOCK_SIZE; ++i) {
                    temp_free_list.push_back(&new_block_raw[i]);
                }

                // Populate the atomic_free_list_head_ from temp_free_list (still under lock)
                // This is suboptimal as it serializes the population.
                for (SkipListNode<T>* n : temp_free_list) {
                    // To push 'n' onto the atomic_free_list_head_
                    // Its 'forward[0]' needs to point to the current head.
                    // And then head needs to point to 'n'.
                    // This requires `n->forward` to be a valid pointer to an array of atomics.
                    // This is not true for raw memory from `::operator new[]`.

                    // THIS IS THE BLOCKER for easy conversion of current structure.
                    // The `forward` member of `SkipListNode` is `std::atomic<SkipListNode<T>*>*`,
                    // which is initialized in the constructor. Raw memory from `::operator new[]` doesn't have this.

                    // If we can't use node->forward[0], we need an alternative "next" pointer for the free list.
                    // Could allocate a small control block alongside each node, or change SkipListNode.
                    // Changing SkipListNode to always have `forward` pre-allocated (e.g. fixed small array or unique_ptr to it)
                    // could work but changes its structure.

                    // Fallback: The new block allocation remains fully under mutex and uses a temporary std::list.
                    // Only once nodes are constructed (and thus have `forward` allocated) and then deallocated,
                    // can they participate in the lock-free list using their `forward[0]`.
                    // This means the first allocation from a new block cannot be lock-free.

                    // Let's assume for now that `allocate` will only use the lock-free path if nodes
                    // are already on `atomic_free_list_head_`. If not, it locks, allocates a block,
                    // and for simplicity, it will take one node and push the rest onto the
                    // `atomic_free_list_head_` while still under the lock. This is not fully lock-free
                    // for new block scenarios but improves the common case.

                    // To push `n` (from new block) to `atomic_free_list_head_`:
                    // 1. `n` is raw memory. It cannot be used as `SkipListNode` yet to access `forward[0]`.
                    // The solution is to construct it with a minimal level (e.g., 0), then it can be on free list.
                    // Then when it's truly allocated for use, it's re-placement-new'd with proper level.
                    // This means an extra constructor/destructor cycle for nodes from new blocks.
                    new (n) SkipListNode<T>(T{}, 0); // Dummy construct to initialize `forward`
                                                     // T{} might be expensive for some T.
                                                     // A special constructor for memory pool nodes might be better.
                }

                // Now that all nodes in new_block_raw have `forward` initialized by dummy construction:
                for (size_t i = 0; i < BLOCK_SIZE; ++i) {
                    SkipListNode<T>* current_new_node = &new_block_raw[i];
                    current_new_node->forward[0].store(atomic_free_list_head_.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    atomic_free_list_head_.store(current_new_node, std::memory_order_release);
                }
                // The dummy objects' destructors should be called if not used.
                // This is getting very complex.

                // Simplest path for now: if list is empty, allocate block, take one, add rest to a *separate* locked list.
                // This means two free lists: one lock-free, one locked. Complexity.

                // Backtrack: The initial idea of using `node->forward[0]` for the lock-free stack's `next` pointer
                // is sound IF the node placed on the free list already has its `forward` member initialized
                // and pointing to a valid array of atomics of at least size 1.
                // This is true for nodes that have been constructed and then `deallocate`d.
                // For brand new blocks:
                // When a new block is allocated (`new_block_raw`), it's just raw memory.
                // If we are to return `node_ptr = new_block_raw[0]` and add `new_block_raw[1..BLOCK_SIZE-1]`
                // to the `atomic_free_list_head_`, then those `[1..BLOCK_SIZE-1]` nodes must have their `forward` pointer initialized.

                // Let's refine the block allocation part (under pool_mutex_):
                // 1. Allocate raw memory for the block.
                // 2. The first node of the block is taken for the current request.
                //    It will be constructed with placement new outside the lock if possible, or just before return.
                // 3. For the remaining nodes in the block:
                //    Initialize them minimally so their `forward` pointer is valid and `forward[0]` can be used.
                //    This means calling placement new with a dummy level (e.g., 0) and default value for T.
                //    Then, push them onto the `atomic_free_list_head_`.
                //    This implies these nodes are "constructed" then immediately made "free".
                //    The `deallocate` logic expects `~SkipListNode` to be called.

                // Revised block allocation (still under pool_mutex_):
                SkipListNode<T>* new_block_raw_mem = static_cast<SkipListNode<T>*>(::operator new[](BLOCK_SIZE * sizeof(SkipListNode<T>)));
                blocks_.emplace_back(std::unique_ptr<SkipListNode<T>[]>(new_block_raw_mem));

                // Initialize all nodes in the new block with a dummy constructor (level 0)
                // so their `forward` array is allocated.
                for (size_t i = 0; i < BLOCK_SIZE; ++i) {
                    new (&new_block_raw_mem[i]) SkipListNode<T>(T{}, 0); // Placement new with level 0
                }

                // Now, add all these "initialized-as-level-0" nodes to the global atomic free list.
                // (Except the one we will return, if we decide to return one from here directly).
                for (size_t i = 0; i < BLOCK_SIZE; ++i) {
                    SkipListNode<T>* n = &new_block_raw_mem[i];
                    // Effectively "deallocating" this dummy node to the free list.
                    // Destructor for T should be called if T{} created something substantial.
                    // But SkipListNode destructor only deletes forward array, which we need.
                    // This is tricky. Let's assume T{} is cheap or has no side effects needing explicit destruction here.
                    n->forward[0].store(atomic_free_list_head_.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    atomic_free_list_head_.store(n, std::memory_order_release);
                }

                // After populating, try to grab one again (as per original logic)
                node_ptr = atomic_free_list_head_.load(std::memory_order_acquire);
                while (node_ptr != nullptr) {
                    SkipListNode<T>* next_node = node_ptr->forward[0].load(std::memory_order_relaxed);
                    if (atomic_free_list_head_.compare_exchange_weak(node_ptr, next_node,
                                                                   std::memory_order_release,
                                                                   std::memory_order_relaxed)) {
                        // Successfully popped. Now call the real placement new with actual value and level.
                        // The previous dummy SkipListNode<T>(T{}, 0) at node_ptr needs its destructor called
                        // before placement new, to clean up its forward array.
                        node_ptr->~SkipListNode<T>(); // Call destructor of the dummy node.
                        node_ptr->forward[0].store(nullptr, std::memory_order_relaxed); // Clear for new object
                        new (node_ptr) SkipListNode<T>(value, level); // Real construction
                        return node_ptr;
                    }
                }
                // If still null, something is wrong (should have found one from new block).
                // This path should ideally not be reached if block allocation succeeded.
                // For safety, though:
                return nullptr; // Should indicate error or throw.
            }

            // This part should not be reached if logic above is correct.
            // If node_ptr is non-null here, it was successfully popped from the lock-free list.
            // The real placement new is done outside the CAS loop.
            // This was missing in the first part of the function.
            // Corrected logic: if pop succeeds, then placement new.

            // The first CAS loop already handles successful pop and returns.
            // So if we reach here, node_ptr must be from the block allocation path,
            // which should also return directly.

            // Fallthrough indicates an issue or unhandled case.
            // This design is getting complicated due to initialization requirements of `forward`.

            // Let's simplify:
            // Allocate path:
            // 1. Try lock-free pop. If success:
            //    Call destructor for the node in its "free list state" (if it had one, e.g. dummy level 0).
            //    No, this is wrong. Destructor is called in deallocate().
            //    When we pop, the memory is raw but `forward` was set up.
            //    We just need to call placement new.
            //    The `node_ptr->forward[0].store(nullptr, ...)` is to clean up the "next" pointer of free list.
            //    Then placement new. This seems correct.

            // The issue is solely with NEW blocks from ::operator new[]:
            // How to initialize `forward` for them so they can join the `atomic_free_list_head_`.
            // Solution: When a block is allocated, and we are under `pool_mutex_`:
            //  - Allocate raw memory `char* block_mem = new char[BLOCK_SIZE * sizeof(SkipListNode<T>)]`.
            //  - For each node slot in this block:
            //    - `SkipListNode<T>* node = reinterpret_cast<SkipListNode<T>*>(&block_mem[i * sizeof(SkipListNode<T>)])`.
            //    - Manually initialize just enough for `forward[0]` to be usable:
            //      - `node->forward = new std::atomic<SkipListNode<T>*>[1];` // Allocate forward array of size 1 (for level 0)
            //      - `node->node_level = 0;` // So destructor knows what to delete.
            //    - Then push to `atomic_free_list_head_`.
            // This way, nodes on free list are "minimal" SkipListNodes.
            // When `allocate` pops such a node:
            //  - It calls `node->~SkipListNode<T>()` to delete the minimal `forward` array.
            //  - Then `new (node) SkipListNode<T>(value, level)` which allocates new `forward` array of correct size.
            // When `deallocate` is called:
            //  - It calls `node->~SkipListNode<T>()` (deletes the real `forward` array).
            //  - Then it must re-initialize `node->forward = new std::atomic<SkipListNode<T>*>[1]; node->node_level = 0;`
            //    before pushing to `atomic_free_list_head_`. This ensures consistency.

            // This is a viable strategy.

            // Revised allocate:
            // (Lock-free pop attempt as before)
            // If empty, lock `pool_mutex_`:
            //  (Double check pop as before)
            //  If still empty:
            //   Allocate new block (raw memory). Add to `blocks_`.
            //   For each node slot in new block:
            //     `node_in_block->forward = new std::atomic<SkipListNode<T>*>[1];`
            //     `node_in_block->node_level = 0;` // Mark as minimal node for free list
            //     Push `node_in_block` to `atomic_free_list_head_` (using its `forward[0]` as next).
            //   Pop one from `atomic_free_list_head_` (must succeed).
            //   `node_to_return->~SkipListNode<T>()` // Destroys the minimal forward array.
            //   `new (node_to_return) SkipListNode<T>(value, level);` // Constructs real node.
            //   Return `node_to_return`.

            // Revised deallocate:
            //  `node->~SkipListNode<T>()`; // Destroys real forward array.
            //  `node->forward = new std::atomic<SkipListNode<T>*>[1];` // Re-init for free list.
            //  `node->node_level = 0;`
            //  Push `node` to `atomic_free_list_head_`.

            // This seems like a consistent model.

            // SkipListNode<T>* new_block_raw = new SkipListNode<T>[BLOCK_SIZE](); // Problematic
            // The line above should be:
            // SkipListNode<T>* new_block_raw_mem = static_cast<SkipListNode<T>*>(::operator new[](BLOCK_SIZE * sizeof(SkipListNode<T>))); // This was part of a comment block
            // blocks_.emplace_back(std::unique_ptr<SkipListNode<T>[]>(new_block_raw_mem));

            // Initialize all nodes in the new block (new_block_raw, which is blocks_.back().get())
            // with a dummy constructor (level 0) so their `forward` array is allocated.
            SkipListNode<T>* block_start_ptr = blocks_.back().get();
            for (size_t i = 0; i < BLOCK_SIZE; ++i) {
                SkipListNode<T>* node_in_block = &block_start_ptr[i];
                // Note: If T is std::pair<const Key, Value>, T{} implies Key and Value must be default-constructible.
                new (node_in_block) SkipListNode<T>(T{}, 0); // Placement new with level 0 (dummy node)
                // Now push this dummy node to atomic_free_list_head_
                // Its forward[0] is used as the 'next' pointer in the free list stack.
                node_in_block->forward[0].store(atomic_free_list_head_.load(std::memory_order_relaxed), std::memory_order_relaxed);
                atomic_free_list_head_.store(node_in_block, std::memory_order_release);
            }

            // Retry pop after filling (still under lock)
            // This will pop one of the dummy nodes just added.
            node_ptr = atomic_free_list_head_.load(std::memory_order_acquire);
            while (node_ptr != nullptr) {
                SkipListNode<T>* next_node = node_ptr->forward[0].load(std::memory_order_relaxed);
                if (atomic_free_list_head_.compare_exchange_weak(node_ptr, next_node,
                                                               std::memory_order_release,
                                                               std::memory_order_relaxed)) {
                    // Successfully popped a "dummy" node.
                    node_ptr->~SkipListNode<T>(); // Destroy dummy node (deletes its forward[level 0] array)
                    // No need to clear forward[0] here as destructor took care of the array.
                    new (node_ptr) SkipListNode<T>(value, level); // Construct real node with actual level and value
                    return node_ptr;
                }
            }
            // Should not be reached if block allocation worked
            // Consider throwing an exception if node_ptr is still null.
            throw std::bad_alloc(); // Or handle error appropriately
        }

        void deallocate(SkipListNode<T>* node) {
            if (!node) return;

            node->~SkipListNode<T>(); // Call destructor for the actual SkipListNode object, freeing its forward array.

            // Re-construct as a "dummy" node (level 0, default T value) before adding to the lock-free list.
            // This strategy prepares the node for the free list by ensuring its `forward[0]` can be
            // reliably used as a 'next' pointer for the stack-like free list.
            // However, this immediate reuse and alteration of the node's state (level, value, forward array structure)
            // is precisely what necessitates a Safe Memory Reclamation (SMR) mechanism for the main skip list
            // operations (insert, remove, search). Without SMR, other threads might still be trying to access
            // the node based on its old state, leading to use-after-free or data corruption.
            // This ensures it has a valid (minimal) forward array for the free list's next pointer.
            try {
                // Note: If T is std::pair<const Key, Value>, T{} implies Key and Value must be default-constructible.
                new (node) SkipListNode<T>(T{}, 0); // Placement new: construct dummy node for free list
            } catch (const std::bad_alloc& e) {
                // If construction of dummy node fails (e.g., allocation of its minimal forward array),
                // we cannot safely add it to the free list. This is a critical error.
                std::cerr << "MemoryPool: Failed to reconstruct node as dummy for free list: " << e.what() << ". Memory leak." << std::endl;
                // This node cannot be reused. The memory slot will be reclaimed only when the whole block is.
                return;
            } catch (...) {
                 std::cerr << "MemoryPool: Unknown exception during dummy node reconstruction. Memory leak." << std::endl;
                return;
            }

            // Push the newly constructed dummy node to the lock-free stack
            SkipListNode<T>* old_head = atomic_free_list_head_.load(std::memory_order_relaxed); // Can be relaxed for first load
            do {
                // The dummy node's forward[0] will point to the current head (old_head)
                node->forward[0].store(old_head, std::memory_order_relaxed);
            } while (!atomic_free_list_head_.compare_exchange_weak(old_head, node,
                                                                 std::memory_order_release, // Release CAS
                                                                 std::memory_order_acquire)); // Acquire on failure (old_head reloaded)
        }
    };

    static const int MAX_LEVEL = 16; // Max level for skip list
    SkipListNode<T>* header;
    std::atomic<int> currentLevel; // Made currentLevel atomic
    MemoryPool memory_pool_; // Added memory pool instance

public:
    SkipList() : currentLevel(0) {
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
        // Uses thread-local static random number generator and distribution.
        // No mutex needed as each thread has its own RNG state.
        int level = 0;
        // Initialize on first use per thread if not already.
        // Check if rng_ and dist_ need explicit initialization here or if class-level init is enough.
        // For thread_local static members, initialization happens before first use in a thread.
        // If default constructor of mt19937 is not seeded, or if dist is not set, it's an issue.
        // The prompt implies direct initialization:
        // thread_local static std::mt19937 rng{std::random_device{}()};
        // thread_local static std::uniform_real_distribution<double> dist{0.0, 1.0};
        // These would need to be defined outside if not C++17 inline, or as function-local statics.
        // Let's try defining them as requested, assuming compiler support.
        // If they are declared as above (rng_tls_generator_, dist_tls_distribution_),
        // they need to be defined outside the class.

        // Re-evaluating based on prompt: "replace ... with:"
        // This means the definitions should be directly where the old ones were, or similar scope.
        // The prompt showed them as if they are *inside the method*.
        // Let's try that approach first, as function-local static thread_local.
        // This is the most common and robust pattern for thread-local RNGs.

        thread_local static std::mt19937 rng{std::random_device{}()};
        thread_local static std::uniform_real_distribution<double> dist{0.0, 1.0};

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

        // Heuristic for starting search using the thread-local finger.
        // The finger is a hint to a recently accessed predecessor node.
        SkipListNode<T>* finger_candidate = thread_local_finger;

        // Note on finger safety without SMR:
        // If finger_candidate points to a node that is concurrently deleted, the memory pool
        // reconstructs it as a "dummy" node (value T{}, level 0).
        // Accessing finger_candidate->value or finger_candidate->node_level below
        // might therefore read from this dummy node. This is safe from crashes (no dangling pointers)
        // but means the heuristic might use T{} or level 0 for its decision.
        // The subsequent traversal logic, using atomic operations, will always find the correct path.
        if (finger_candidate != header && get_comparable_value(finger_candidate->value) < get_comparable_value(value)) {
            // Finger is valid, not header, and its value is less than the target: potential start from finger.
            current_node = finger_candidate;
            // Start search from the finger's level, capped by the list's current max level.
            search_start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header &&
                   get_comparable_value(finger_candidate->value) > get_comparable_value(value)) { // Simplified from (!(A<B) && !(A==B))
            // Finger is valid, but its value is greater than the target: finger is not useful. Start from header.
            // Optionally, one could reset thread_local_finger = header; here if this case indicates a significant search jump.
            current_node = header;
            search_start_level = localCurrentLevel;
        } else {
            // Finger is header, or finger's value is equal to target (checked with get_comparable_value implicitly by falling through),
            // or some other edge case: default to starting from header.
            current_node = header;
            search_start_level = localCurrentLevel;
        }

        // Phase 1: Populate 'update' array using finger or header
        // Traverse from search_start_level down to 0
        for (int i = search_start_level; i >= 0; i--) {
            SkipListNode<T>* next_node = current_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && get_comparable_value(next_node->value) < get_comparable_value(value)) {
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
                    while (next_node != nullptr && get_comparable_value(next_node->value) < get_comparable_value(value)) {
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
        if (current_val_check_node == nullptr || get_comparable_value(current_val_check_node->value) != get_comparable_value(value)) {
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
                    // If CAS failed, 'succ' is updated by CAS with the current value of pred->forward[i].
                    // This means the link from 'pred' to its original 'succ' has changed.
                    // The current simple retry assumes 'newNode' still belongs between 'pred' and the new 'succ'.
                    // This might not be true if the new 'succ' has a value smaller than 'newNode->value',
                    // which implies 'pred' itself is no longer the correct predecessor for 'newNode'.
                    // A fully robust solution would require re-traversing from 'pred' or even header for this level 'i'
                    // to find the correct 'pred' again. This is a common simplification in basic lock-free lists.
                    // Additionally, 'pred' itself could have been unlinked by another operation.
                    newNode->forward[i].store(succ, std::memory_order_relaxed); // Update newNode's next pointer to the new successor
                }
            }
            // ABA Problem Note: The CAS loop above is susceptible to the ABA problem. If 'succ' is unlinked,
            // its memory recycled by the pool, and a new node allocated at the same address which then becomes
            // the successor of 'pred', the CAS might succeed incorrectly. A robust solution requires
            // techniques like tagged pointers or proper safe memory reclamation (SMR) that prevents premature memory reuse.

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
        
        SkipListNode<T>* finger_candidate = SkipList<T>::thread_local_finger;

        // Finger usage notes from insert() apply here too regarding concurrent deletion
        // and reading from a "dummy" node.
        if (finger_candidate != header && get_comparable_value(finger_candidate->value) < get_comparable_value(value)) {
            current_search_node = finger_candidate;
            start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header &&
                   get_comparable_value(finger_candidate->value) > get_comparable_value(value)) { // Simplified from (!(A<B) && !(A==B))
            // Value is less than finger's value, finger not useful. Reset finger and start from header.
            SkipList<T>::thread_local_finger = header; // Resetting finger as it's significantly off.
            current_search_node = header;
            start_level = localCurrentLevel;
        } else {
            // Finger is header, or finger's value is equal to target. Start from header.
            current_search_node = header;
            start_level = localCurrentLevel;
        }

        SkipListNode<T>* predecessor_at_level0 = header; // Initialize to header

        // Start from determined start_level and go down
        for (int i = start_level; i >= 0; i--) {
            SkipListNode<T>* next_node = current_search_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && get_comparable_value(next_node->value) < get_comparable_value(value)) {
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

        return (found_node != nullptr && get_comparable_value(found_node->value) == get_comparable_value(value));
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
        // Finger usage notes from insert() apply here too regarding concurrent deletion
        // and reading from a "dummy" node.
        if (finger_candidate != header && get_comparable_value(finger_candidate->value) < get_comparable_value(value)) {
            current_node = finger_candidate;
            search_start_level = std::min(localCurrentLevel, finger_candidate->node_level);
        } else if (finger_candidate != header &&
                   get_comparable_value(finger_candidate->value) > get_comparable_value(value)) { // Simplified from (!(A<B) && !(A==B))
            // Value is less than finger's value, finger is not useful. Start from header.
            // Optionally, one could reset thread_local_finger = header;
            current_node = header;
            search_start_level = localCurrentLevel;
        } else {
            // Finger is header or its value is equal to target. Start from header.
            current_node = header;
            search_start_level = localCurrentLevel;
        }

        // Phase 1: Populate 'update' array using finger or header
        for (int i = search_start_level; i >= 0; i--) {
            SkipListNode<T>* next_node = current_node->forward[i].load(std::memory_order_acquire);
            while (next_node != nullptr && get_comparable_value(next_node->value) < get_comparable_value(value)) {
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
                    while (next_node != nullptr && get_comparable_value(next_node->value) < get_comparable_value(value)) {
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
        if (target_node != nullptr && get_comparable_value(target_node->value) == get_comparable_value(value)) {
            // Attempt to unlink the node from all levels
            for (int i = 0; i <= target_node->node_level; ++i) { // Iterate up to the target_node's actual level
                SkipListNode<T>* pred = update[i];
                 // Critical: Ensure pred is valid. update[i] should ideally always be valid.
                if (pred == nullptr) {
                    // This should not happen if update array is correctly populated.
                    // If it does, it implies a logic error in search phase or concurrent modification
                    // that invalidated pred beyond simple CAS failure handling.
                    continue;
                }

                // Try to unlink target_node from pred's list at level i.
                // succ_of_target is what target_node points to, which will become pred's new successor.
                SkipListNode<T>* succ_of_target = target_node->forward[i].load(std::memory_order_relaxed);
                SkipListNode<T>* expected_target_at_pred = target_node;

                if (pred->forward[i].compare_exchange_weak(expected_target_at_pred, succ_of_target,
                                                           std::memory_order_release,
                                                           std::memory_order_relaxed)) {
                    // Successfully unlinked at this level i.
                } else {
                    // CAS failed. This means pred->forward[i] is not (or no longer) target_node.
                    // Reasons:
                    // 1. target_node was already unlinked at this level by another thread.
                    // 2. Another node was inserted between pred and target_node.
                    // 3. pred itself was unlinked/modified. (update[i] was stale).
                    // Current strategy: "lazy" removal - if we fail to unlink at a level, we assume
                    // another operation has modified the list, and we don't retry this level.
                    // The node might remain partially linked, which is a characteristic of some lock-free algorithms.
                    // The node will eventually be fully unlinked by subsequent operations or is already effectively removed.
                }
            }
            // ABA Problem Note: The CAS loop above is susceptible to the ABA problem, similar to insert.

            // CRITICAL CONCURRENCY ISSUE: Immediate deallocation and memory reclamation.
            // The memory_pool_.deallocate(target_node) call below will typically call the destructor
            // on target_node and then reconstruct it as a "dummy node" for the free list.
            // If other threads are concurrently traversing the skip list and hold a pointer
            // to target_node (e.g., they read it just before it was unlinked), they might subsequently
            // access target_node->value or target_node->forward[i].
            // - Accessing target_node->value would read T{} (the dummy node's value).
            // - Accessing target_node->forward[i] (especially for i > 0) could be a use-after-free
            //   because the original forward array was deleted by ~SkipListNode() and replaced by a
            //   minimal one for the dummy node.
            // This can lead to crashes or silent data corruption.
            // A robust lock-free skip list requires a Safe Memory Reclamation (SMR) mechanism
            // (e.g., Hazard Pointers, Epoch-Based Reclamation) to ensure that target_node's memory
            // is not reclaimed or altered (destructor, reconstruction as dummy) until no thread can
            // possibly be accessing it. Implementing SMR is a complex task beyond this scope.
            // Without SMR, this immediate deallocation is a significant concurrency flaw.
            memory_pool_.deallocate(target_node);

            // Update current level if necessary - this also needs to be careful.
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

    // Print all values in sorted order
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
            while (next_node != nullptr && get_comparable_value(next_node->value) < get_comparable_value(minVal)) {
                current_node = next_node;
                next_node = current_node->forward[i].load(std::memory_order_acquire);
            }
        }
        
        current_node = current_node->forward[0].load(std::memory_order_acquire);
        
        // Collect values in range
        while (current_node != nullptr && get_comparable_value(current_node->value) <= get_comparable_value(maxVal)) {
            if (get_comparable_value(current_node->value) >= get_comparable_value(minVal)) {
                result.push_back(current_node->value);
            }
            current_node = current_node->forward[0].load(std::memory_order_acquire);
        }
        
        return result;
    }

    // Bulk insert operation
    // This operation is not atomic. It inserts elements one by one.
    // If concurrent operations are happening, the final state of the skip list
    // will be as if these individual insertions happened in some sequence.
    // True atomic bulk insertion would typically require more complex mechanisms
    // like multi-key locking protocols or transactional semantics, which are
    // beyond the current scope of this implementation.
    void insert_bulk(const std::vector<T>& values) {
        if (values.empty()) {
            return;
        }
        std::vector<T> sorted_values = values; // Create a copy
        std::sort(sorted_values.begin(), sorted_values.end());
        // Optional: remove duplicates from sorted_values if that semantic is desired.
        // std::unique(sorted_values.begin(), sorted_values.end());
        // For now, allow multiple insertions if duplicates are in input.

        for (const T& value : sorted_values) {
            this->insert(value);
        }
    }

    // Bulk remove operation
    // This operation is not atomic. It removes elements one by one.
    // Returns the count of elements successfully removed.
    // As with insert_bulk, this operation is not atomic for the entire set of values;
    // concurrent operations can interleave. It also inherits all concurrency
    // caveats of the single remove() operation, especially the lack of Safe Memory
    // Reclamation (SMR), which can lead to issues if nodes are accessed by one
    // thread while being deallocated and reused by another.
    size_t remove_bulk(const std::vector<T>& values) {
        if (values.empty()) {
            return 0;
        }
        size_t removed_count = 0;
        // Sorting might improve finger performance if values are clustered,
        // but can be skipped if input order is preferred for some reason or for simplicity.
        // Let's sort for potential minor optimization.
        std::vector<T> sorted_values = values; // Create a copy
        std::sort(sorted_values.begin(), sorted_values.end());
        // Optional: remove duplicates from sorted_values if desired.
        // std::unique(sorted_values.begin(), sorted_values.end());

        for (const T& value : sorted_values) {
            if (this->remove(value)) {
                removed_count++;
            }
        }
        return removed_count;
    }
};

// End of generic SkipList

// All specializations for SkipListNode<int>, SkipList<int>,
// SkipListNode<std::string>, and SkipList<std::string> have been removed
// as they were identical in logic to the primary template.
// The primary SkipList<T> template will now handle these types.
// Minor display differences (quoting strings) have been incorporated
// into the primary template's display() and printValues() methods using `if constexpr`.

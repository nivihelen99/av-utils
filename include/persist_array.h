#pragma once

#include <memory>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <cassert>

// Performance Notes:
// - Creating a new PersistentArray by copying an existing one (e.g., PersistentArray v2 = v1;)
//   is an O(1) operation (shared_ptr copy).
// - Operations that modify data (set, push_back, etc.) and return a new version
//   incur an O(N) cost for data copying if the current version's data is shared
//   or if it's the first modification making the data unique (due to ensure_unique()).
//   If data is already unique, the complexity is that of the underlying std::vector operation.
// - Element access (operator[], at()) is O(1).

// Thread Safety:
// - All const member functions are thread-safe for concurrent read access, even if
//   multiple PersistentArray objects share the same underlying data.
// - Modifying operations (in-place methods like set_inplace, or const methods like 'set'
//   if their result is assigned back to the same variable) are not thread-safe if
//   called on the same PersistentArray object from multiple threads without external synchronization.
//   This is standard behavior for C++ objects.

/**
 * @brief A persistent array implementation with copy-on-write semantics.
 *
 * PersistentArray provides an immutable array-like data structure.
 * Modifying operations return a new version of the array, leaving the original unchanged.
 * Data is shared between array versions whenever possible to save memory and copying time,
 * with actual data duplication (copy-on-write) occurring only when a shared version is modified.
 *
 * @tparam T The type of elements stored in the array. T must be at least CopyConstructible.
 *           For full functionality (like insert, erase), T should meet all requirements
 *           for elements in `std::vector`.
 * @see Node
 * @see ensure_unique
 */
template<typename T>
class PersistentArray {
private:
    /// @brief Internal node structure holding the actual data vector.
    struct Node {
        std::vector<T> data;
        
        Node(size_t size) : data(size) {}
        Node(const std::vector<T>& vec) : data(vec) {}
        Node(std::vector<T>&& vec) : data(std::move(vec)) {}
        
        // Deep copy constructor for Node's data, used during copy-on-write.
        Node(const Node& other) : data(other.data) {}
    };
    
    mutable std::shared_ptr<Node> root;
    
    // Ensures that the current instance has a unique copy of the data for modification.
    // This method is central to the copy-on-write (CoW) semantics.
    // It checks if `root` is null (a safety measure, though standard constructors ensure `root` is initialized;
    // a moved-from PersistentArray instance would have `root` as null) or if the data is shared (i.e., `root.use_count() > 1`).
    // If either condition is true, it means the current instance either doesn't own the data exclusively or doesn't have data.
    // In such cases, a new `Node` is created by copying the data from the existing `*root`.
    // Note: If `root` were `nullptr` (e.g., if called on a moved-from instance), `*root` would be a dereference of a nullptr.
    //       However, methods calling `ensure_unique` typically operate on valid objects or are CoW operations
    //       that create a new valid state. In-place methods on moved-from objects are problematic.
    // The `root` pointer is then updated to point to this newly created, unique `Node`.
    // This method is declared `const` because it modifies `root`, which is a `mutable std::shared_ptr<Node>`.
    // This allows methods that conceptually offer read-only access from the user's perspective (if they were to call it)
    // to still perform a CoW operation if needed, but it's primarily used by non-const (in-place) methods
    // or CoW methods that return a new version to ensure modifications do not affect other `PersistentArray` instances.
    // Performance: This method is O(N) if a data copy is performed (where N is array size),
    // otherwise it's O(1) (checking use_count).
    void ensure_unique() const {
        if (!root || root.use_count() > 1) {
            // If root is null, create a new Node with an empty vector.
            // Otherwise, copy the existing root's data.
            // This prevents dereferencing a nullptr if ensure_unique is called on a (e.g. moved-from) object where root is null.
            root = std::make_shared<Node>(root ? root->data : std::vector<T>());
        }
    }
    
public:
    /** @name Constructors and Assignment */
    ///@{

    /**
     * @brief Default constructor. Creates an empty PersistentArray.
     * Complexity: O(1).
     */
    PersistentArray() : root(std::make_shared<Node>(0)) {}
    
    /**
     * @brief Constructs a PersistentArray with a given number of default-initialized elements.
     * @param size The initial number of elements.
     * Complexity: O(N) where N is the size, due to vector allocation and default initialization.
     */
    explicit PersistentArray(size_t size) 
        : root(std::make_shared<Node>(size)) {}
    
    /**
     * @brief Constructs a PersistentArray with `size` copies of `value`.
     * @param size The number of elements.
     * @param value The value to initialize elements with.
     * Complexity: O(N) where N is the size, due to vector allocation and value copying.
     */
    PersistentArray(size_t size, const T& value) 
        : root(std::make_shared<Node>(std::vector<T>(size, value))) {}
    
    /**
     * @brief Constructs a PersistentArray from an initializer list.
     * @param init An initializer_list of elements.
     * Complexity: O(N) where N is the number of elements in the list.
     */
    PersistentArray(std::initializer_list<T> init) 
        : root(std::make_shared<Node>(std::vector<T>(init))) {}
    
    /**
     * @brief Copy constructor. Creates a new PersistentArray that shares data with `other`.
     * This is a shallow copy operation (O(1) complexity for shared_ptr copy).
     * @param other The PersistentArray to copy from.
     */
    PersistentArray(const PersistentArray& other) : root(other.root) {}
    
    /**
     * @brief Copy assignment operator. Makes this PersistentArray share data with `other`.
     * This is a shallow copy operation (O(1) complexity for shared_ptr copy).
     * @param other The PersistentArray to assign from.
     * @return A reference to this PersistentArray.
     */
    PersistentArray& operator=(const PersistentArray& other) {
        if (this != &other) {
            root = other.root;
        }
        return *this;
    }
    
    /**
     * @brief Move constructor. Transfers ownership of data from `other` to this PersistentArray.
     * `other` is left in a valid but unspecified (typically empty) state.
     * Complexity: O(1).
     * @param other The PersistentArray to move from.
     */
    PersistentArray(PersistentArray&& other) noexcept : root(std::move(other.root)) {}
    
    /**
     * @brief Move assignment operator. Transfers ownership of data from `other` to this PersistentArray.
     * `other` is left in a valid but unspecified (typically empty) state.
     * Releases any data currently held by this PersistentArray.
     * Complexity: O(1).
     * @param other The PersistentArray to move assign from.
     * @return A reference to this PersistentArray.
     */
    PersistentArray& operator=(PersistentArray&& other) noexcept {
        if (this != &other) {
            root = std::move(other.root);
        }
        return *this;
    }
    ///@}

    /** @name Element Access */
    ///@{

    /**
     * @brief Accesses the element at the specified index. No bounds checking is performed if NDEBUG is defined.
     * If `root` is null (e.g., for a moved-from object), behavior is undefined.
     * For safety, a check for `!root` and index bounds is included, throwing `std::out_of_range`.
     * @param index The index of the element to access.
     * @return A const reference to the element at `index`.
     * @throw std::out_of_range if `index` is out of bounds or array is uninitialized/moved-from.
     * @note Complexity: O(1).
     */
    const T& operator[](size_t index) const {
       // !root check is a safety, as constructors generally initialize root.
       // A moved-from PersistentArray will have root == nullptr. Accessing it is undefined.
       if (!root) {
           throw std::out_of_range("Array is uninitialized (potentially moved-from)");
       }
       if (index >= root->data.size()) { // root->data.size() is safe due to !root check above.
           throw std::out_of_range("Index out of range");
       }
       return root->data[index];
    }
    
    /**
     * @brief Accesses the element at the specified index with bounds checking.
     * @param index The index of the element to access.
     * @return A const reference to the element at `index`.
     * @throw std::out_of_range if `index` is out of bounds.
     * @note Complexity: O(1).
     */
    const T& at(size_t index) const {
        // size() handles nullptr root returning 0.
        if (index >= size()) {
            throw std::out_of_range("Index out of range");
        }
        // If root were null, size() would be 0, and any index >= 0 would throw.
        // If root is not null, root->data is safe to access.
        return root->data[index];
    }
    ///@}

    /** @name Modifying Operations (Copy-on-Write)
     * These operations return a new PersistentArray with the modification,
     * leaving the original array unchanged. Iterators on the original array remain valid.
     * Iterators on the new array are valid for the new array.
     * Complexity is generally O(N) if a data copy (CoW) is triggered,
     * otherwise it's the complexity of the underlying std::vector operation on unique data.
    */
    ///@{

    /**
     * @brief Returns a new PersistentArray with the element at `index` changed to `value`.
     * Original array remains unchanged.
     * @param index The index of the element to change.
     * @param value The new value for the element.
     * @return A new PersistentArray with the updated element.
     * @throw std::out_of_range if `index` is out of bounds.
     * @note Complexity: O(N) if CoW copy occurs (due to ensure_unique), else O(1) for the set operation itself.
     *       Iterators on the original array remain valid.
     */
    PersistentArray set(size_t index, const T& value) const {
        if (index >= size()) { // size() handles null root case
            throw std::out_of_range("Index out of range");
        }
        
        PersistentArray new_version(*this); // Shallow copy (shares data initially)
        new_version.ensure_unique();       // Perform CoW if data is shared
        new_version.root->data[index] = value;
        return new_version;
    }
    
    /**
     * @brief Returns a new PersistentArray with `value` added to the end.
     * Original array remains unchanged.
     * @param value The value to append.
     * @return A new PersistentArray with the appended element.
     * @note Complexity: O(N) if CoW copy occurs, else amortized O(1) for std::vector::push_back.
     *       Iterators on the original array remain valid.
     */
    PersistentArray push_back(const T& value) const {
        PersistentArray new_version(*this);
        new_version.ensure_unique();
        new_version.root->data.push_back(value);
        return new_version;
    }
    
    /**
     * @brief Returns a new PersistentArray with the last element removed.
     * Original array remains unchanged.
     * @return A new PersistentArray with the last element removed.
     * @throw std::runtime_error if the array is empty.
     * @note Complexity: O(N) if CoW copy occurs, else O(1) for std::vector::pop_back.
     *       Iterators on the original array remain valid.
     */
    PersistentArray pop_back() const {
        if (empty()) {
            throw std::runtime_error("Cannot pop from empty array");
        }
        
        PersistentArray new_version(*this);
        new_version.ensure_unique();
        new_version.root->data.pop_back();
        return new_version;
    }
    
    /**
     * @brief Returns a new PersistentArray with `value` inserted at `index`.
     * Original array remains unchanged.
     * @param index The position at which to insert the new element.
     * @param value The value to insert.
     * @return A new PersistentArray with the inserted element.
     * @throw std::out_of_range if `index` is greater than the current size.
     * @note Complexity: O(N) due to potential CoW copy and std::vector::insert.
     *       Iterators on the original array remain valid.
     */
    PersistentArray insert(size_t index, const T& value) const {
        if (index > size()) { // size() handles null root case
            throw std::out_of_range("Index out of range for insert");
        }
        
        PersistentArray new_version(*this);
        new_version.ensure_unique();
        new_version.root->data.insert(new_version.root->data.begin() + index, value);
        return new_version;
    }
    
    /**
     * @brief Returns a new PersistentArray with the element at `index` removed.
     * Original array remains unchanged.
     * @param index The index of the element to remove.
     * @return A new PersistentArray with the element removed.
     * @throw std::out_of_range if `index` is out of bounds.
     * @note Complexity: O(N) due to potential CoW copy and std::vector::erase.
     *       Iterators on the original array remain valid.
     */
    PersistentArray erase(size_t index) const {
        if (index >= size()) { // size() handles null root case
            throw std::out_of_range("Index out of range for erase");
        }
        
        PersistentArray new_version(*this);
        new_version.ensure_unique();
        new_version.root->data.erase(new_version.root->data.begin() + index);
        return new_version;
    }
    ///@}

    /** @name In-Place Modifying Operations
     * These operations modify the current PersistentArray instance.
     * If the data is shared with other instances, a copy-on-write occurs first to ensure
     * other instances are not affected. Iterator invalidation rules apply as per std::vector
     * if no CoW occurs, or all iterators for the current instance are invalidated if CoW occurs.
     * See class-level iterator invalidation notes.
    */
    ///@{

    /**
     * @brief Modifies the element at `index` to `value` in the current array.
     * Performs copy-on-write if the data is shared.
     * @param index The index of the element to change.
     * @param value The new value.
     * @throw std::out_of_range if `index` is out of bounds.
     * @note Complexity: O(N) if CoW copy occurs, else O(1). Iterators may be invalidated.
     */
    void set_inplace(size_t index, const T& value) {
        if (index >= size()) { // size() handles null root
            throw std::out_of_range("Index out of range");
        }
        ensure_unique(); // CoW if shared, makes root non-null
        root->data[index] = value;
    }
    
    /**
     * @brief Appends `value` to the end of the current array.
     * Performs copy-on-write if the data is shared.
     * @param value The value to append.
     * @note Complexity: O(N) if CoW copy occurs, else amortized O(1). Iterators may be invalidated.
     */
    void push_back_inplace(const T& value) {
        ensure_unique(); // CoW if shared, makes root non-null
        root->data.push_back(value);
    }
    
    /**
     * @brief Removes the last element from the current array.
     * Performs copy-on-write if the data is shared.
     * @throw std::runtime_error if the array is empty.
     * @note Complexity: O(N) if CoW copy occurs, else O(1). Iterators may be invalidated.
     */
    void pop_back_inplace() {
        if (empty()) { // empty() handles null root
            throw std::runtime_error("Cannot pop from empty array");
        }
        ensure_unique(); // CoW if shared
        root->data.pop_back();
    }
    ///@}

    /** @name Utility Methods */
    ///@{

    /**
     * @brief Returns the number of elements in the array.
     * @return The number of elements.
     * @note Complexity: O(1). Returns 0 if array is moved-from (root is null).
     */
    size_t size() const {
        return root ? root->data.size() : 0;
    }
    
    /**
     * @brief Checks if the array is empty.
     * @return True if the array is empty, false otherwise.
     * @note Complexity: O(1). Returns true if array is moved-from.
     */
    bool empty() const {
        return size() == 0;
    }
    
    /**
     * @brief Clears the array, making it empty.
     * This operation makes the current instance point to a new, empty data node.
     * If the data was shared with other PersistentArray instances, those instances
     * remain unaffected and retain their view of the data before this clear() operation.
     * @note Iterators for this instance are invalidated. Other versions' iterators are unaffected.
     */
    void clear() {
        // This operation makes the current instance point to a new, empty data node.
        // If the data was shared with other PersistentArray instances, those instances
        // remain unaffected and retain their view of the data before this clear() operation.
        // A new Node with a zero-sized vector is created and assigned to root.
        root = std::make_shared<Node>(0);
    }
    ///@}
    
    // --- Iterator Invalidation ---
    // (Existing detailed comments are kept here, Doxygen for begin/end will refer to this section)
    // For in-place modification operations (e.g., `set_inplace`, `push_back_inplace`, `pop_back_inplace`, `clear`):
    // - If `ensure_unique()` is called and does *not* cause a data copy (i.e., `root.use_count() == 1` before the call),
    //   iterator invalidation rules are identical to `std::vector`'s rules for its modifying operations on the *same* underlying data.
    //   For example, `push_back_inplace` might invalidate all iterators if reallocation occurs. `clear` invalidates all iterators.
    // - If `ensure_unique()` *does* cause a data copy (i.e., `root.use_count() > 1`),
    //   iterators (and references/pointers) to the *original* shared data (held by other `PersistentArray` instances) remain valid and are unaffected.
    //   Iterators obtained for the *current* `PersistentArray` instance *before* such an operation become invalid because the instance now points to new, unique data. Iterators obtained *after* the operation will refer to this new data.
    //   The `clear()` method always assigns a new node, so iterators to the previous data (if unique) are invalidated.
    // For "CoW-explicit" operations (e.g., `set`, `push_back`, `pop_back`, `insert`, `erase` which return a new `PersistentArray`):
    // - Iterators (and references/pointers) to the original `PersistentArray` instance (on which the method was called) remain fully valid, as these operations do not modify the original instance. They operate on a new or copied data segment for the returned instance.

    /** @name Iterator Support
     * Provides const iterators for accessing array elements.
     * @note Iterators are `std::vector<T>::const_iterator`.
     * @note Accessing iterators on a moved-from array (where `root` is `nullptr`) results in undefined behavior (likely a crash).
     * @see The main "Iterator Invalidation" comment block in the class for detailed rules.
    */
    ///@{

    /**
     * @brief Returns a const iterator to the beginning of the array.
     * @return A const iterator to the first element.
     * @warning Accessing `begin()` on a moved-from array will likely cause a crash.
     */
    typename std::vector<T>::const_iterator begin() const {
        // If root is nullptr (e.g. for a moved-from object), this will cause a crash.
        // Accessing begin() on a moved-from object is problematic.
        // A robust check 'if (!root) return some_empty_iterator;' could be added,
        // but typical usage expects a valid object.
        if (!root) {
            // Or, to return a valid empty range iterator:
            // static const std::vector<T> empty_vec; return empty_vec.begin();
            // However, this changes behavior for valid empty arrays vs moved-from ones.
            // Crashing is perhaps more indicative of misuse for moved-from objects.
            throw std::logic_error("Attempt to access begin() on a moved-from or uninitialized PersistentArray");
        }
        return root->data.begin();
    }
    
    /**
     * @brief Returns a const iterator to the end of the array.
     * @return A const iterator to the element following the last element.
     * @warning Accessing `end()` on a moved-from array will likely cause a crash.
     */
    typename std::vector<T>::const_iterator end() const {
        // If root is nullptr (e.g. for a moved-from object), this will cause a crash.
        // Similar to begin(), robust check 'if (!root) return some_empty_iterator;' could be added.
        if (!root) {
            // static const std::vector<T> empty_vec; return empty_vec.end();
            throw std::logic_error("Attempt to access end() on a moved-from or uninitialized PersistentArray");
        }
        return root->data.end();
    }
    ///@}

    /** @name Debug Information */
    ///@{

    /**
     * @brief Returns the current use count of the shared data node.
     * This indicates how many PersistentArray instances share the same underlying data.
     * If `root` is `nullptr` (e.g. for a moved-from object), behavior might be undesirable
     * (e.g. it might try to dereference null if not handled, though shared_ptr::use_count on a null shared_ptr is 0).
     * @return The use count of the shared data. Returns 0 if `root` is `nullptr`.
     */
    long use_count() const {
        return root.use_count(); // std::shared_ptr::use_count() returns 0 if it's empty.
    }
    
    /**
     * @brief Prints basic debug information about the array to std::cout.
     * Information includes size and reference count.
     */
    void print_debug_info() const {
        std::cout << "Array size: " << size() 
                  << ", Reference count: " << use_count() << '\n';
    }
    ///@}

    /** @name Comparison Operators */
    ///@{

    /**
     * @brief Compares this PersistentArray with another for equality.
     * Two arrays are equal if they have the same size and all corresponding elements are equal.
     * Handles comparisons with moved-from (empty) objects correctly.
     * @param other The PersistentArray to compare with.
     * @return True if the arrays are equal, false otherwise.
     */
    bool operator==(const PersistentArray& other) const {
        if (root == other.root) { // Handles two nullptrs (e.g. both moved from) or same shared data
            return true;
        }
        // If one is null and the other isn't, they can only be equal if both are logically empty.
        // size() correctly returns 0 if root is nullptr.
        if (this->size() != other.size()) {
            return false;
        }
        // If both are size 0 (regardless of whether root is null or points to empty data), they are equal.
        if (this->size() == 0) {
            return true;
        }
        // At this point, both root pointers must be valid (neither is null because if one was,
        // and size was 0, it would have returned true; if size was non-zero, it would have
        // failed the size check earlier if the other was null).
        // Both root pointers must be valid and sizes are equal and non-zero. Compare data.
        assert(root && other.root); // Both should be non-null here
        return root->data == other.root->data;
    }
    
    /**
     * @brief Compares this PersistentArray with another for inequality.
     * @param other The PersistentArray to compare with.
     * @return True if the arrays are not equal, false otherwise.
     */
    bool operator!=(const PersistentArray& other) const {
        return !(*this == other);
    }
    ///@}
};

// General note on T for PersistentArray:
// The type T stored in PersistentArray must meet the requirements for elements
// stored in std::vector, such as being CopyConstructible and CopyAssignable
// if vector operations that require these are used (which they are, e.g. in Node copy constructor,
// vector::insert, vector::erase, etc.).
// For operations like set/push_back, T must be copyable or movable into the vector.

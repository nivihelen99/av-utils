#ifndef CHAIN_MAP_HPP
#define CHAIN_MAP_HPP

#include <vector>
#include <string> // For common key types, though K can be anything
#include <unordered_map> // Default map type
#include <map>           // Alternative map type
#include <stdexcept>     // For exceptions like out_of_range
#include <algorithm>     // For std::find_if, std::remove_if
#include <set>           // For managing unique keys in iteration/size
#include <memory>        // For std::add_pointer_t or similar if needed for maps_
#include <list>          // std::list to store map pointers for stable iterators

// Forward declaration of the iterator is not strictly needed if defined as a nested class before use.
// However, it doesn't hurt.
// template<typename K, typename V, typename MapType> class ChainMapIterator; // Not needed if nested

template<typename K, typename V, typename MapType = std::unordered_map<K, V>>
class ChainMap {
private:
    // Private Nested Iterator Class
    template<bool IsConst>
    class ChainMapIterator {
    public:
        // Iterator traits
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;

        // value_type is what operator* returns *by value*. For map iterators, this is std::pair<const K, V>.
        using value_type = std::pair<const K, V>;
        // reference is the type of *operator*()
        using reference = std::conditional_t<IsConst,
                            const std::pair<const K, V>&,
                            std::pair<const K, V>&>;
        // pointer is the type of operator->()
        using pointer = std::conditional_t<IsConst,
                          const std::pair<const K, V>*,
                          std::pair<const K, V>*>;

        // Type of the pointer to the vector of map pointers we are iterating over.
        using ParentMapsVectorPtrType = std::conditional_t<IsConst,
                                           const std::vector<MapType*>*,
                                           std::vector<MapType*>*>;
        // Iterator for the vector of map pointers. This iterates over the actual `maps_` vector.
        // It's always a const_iterator for the vector itself, as the ChainMapIterator doesn't change the vector structure.
        using MapVectorIterType = typename std::vector<MapType*>::const_iterator;

        // Iterator for elements within one of the maps in the chain.
        using CurrentMapElementIterType = std::conditional_t<IsConst,
                                          typename MapType::const_iterator,
                                          typename MapType::iterator>;

    private:
        ParentMapsVectorPtrType maps_vec_ptr_{nullptr}; // Pointer to the ChainMap's maps_ vector
        MapVectorIterType current_map_iter_;          // Iterator for traversing maps_vec_ptr_ (points into *maps_vec_ptr_)
        CurrentMapElementIterType current_element_iter_; // Iterator for the current map's elements (points into *(*current_map_iter_))
        std::set<K> visited_keys_;                    // Stores keys already yielded by this iterator instance.

        void initialize_element_iterator_for_current_map() {
            // Precondition: current_map_iter_ is valid (not end) and *current_map_iter_ points to a non-null map.
            if (maps_vec_ptr_ && current_map_iter_ != (*maps_vec_ptr_).end() && *current_map_iter_ != nullptr) {
                if constexpr (IsConst) {
                    current_element_iter_ = (*current_map_iter_)->cbegin();
                } else {
                    current_element_iter_ = (*current_map_iter_)->begin();
                }
            }
        }

        void advance_until_valid_unvisited_item() {
            if (!maps_vec_ptr_) return; // Iterator is already at end or uninitialized.

            while (current_map_iter_ != (*maps_vec_ptr_).end()) {
                MapType* current_map_ptr = *current_map_iter_; // Get the pointer to the current map.

                if (!current_map_ptr) { // Skip null maps in the chain.
                    ++current_map_iter_;
                    // If we moved to a new map, try to initialize its element iterator.
                    if (current_map_iter_ != (*maps_vec_ptr_).end() && *current_map_iter_ != nullptr) {
                         initialize_element_iterator_for_current_map();
                    }
                    continue; // Continue to the next map in maps_vec_ptr_.
                }

                // Determine the end iterator for the current map, respecting const-correctness.
                auto end_of_current_map_iter = IsConst ? current_map_ptr->cend() : current_map_ptr->end();

                while (current_element_iter_ != end_of_current_map_iter) {
                    if (visited_keys_.find(current_element_iter_->first) == visited_keys_.end()) {
                        // Found an unvisited key. The iterator is now positioned at a valid item.
                        return;
                    }
                    ++current_element_iter_; // Move to the next element in the current map.
                }

                // Reached the end of the current map. Move to the next map in the chain.
                ++current_map_iter_;
                // If there's a next map and it's not null, initialize its element iterator.
                if (current_map_iter_ != (*maps_vec_ptr_).end() && *current_map_iter_ != nullptr) {
                    initialize_element_iterator_for_current_map();
                }
            }
            // Reached the end of all maps in maps_vec_ptr_. Mark this iterator as an end iterator.
            maps_vec_ptr_ = nullptr;
        }

    public:
        // Default constructor creates an end iterator.
        ChainMapIterator() = default;

        // Main constructor to create a begin or intermediate iterator.
        ChainMapIterator(ParentMapsVectorPtrType maps_vec, MapVectorIterType start_map_iter)
            : maps_vec_ptr_(maps_vec), current_map_iter_(start_map_iter) {
            if (maps_vec_ptr_ && current_map_iter_ != (*maps_vec_ptr_).end()) {
                // If the map pointed by start_map_iter is null, advance_until_valid_unvisited_item will skip it.
                if (*current_map_iter_ != nullptr) { // Only initialize if current map is not null
                    initialize_element_iterator_for_current_map();
                }
                advance_until_valid_unvisited_item(); // Find the first actual item.
            } else {
                // If maps_vec is empty or start_map_iter is already at the end, this becomes an end iterator.
                maps_vec_ptr_ = nullptr;
            }
        }

        reference operator*() const {
            // Precondition: The iterator is dereferenceable (i.e., not an end iterator).
            // advance_until_valid_unvisited_item ensures current_element_iter_ is valid.
            return *current_element_iter_;
        }

        pointer operator->() const {
            return &(*current_element_iter_);
        }

        // Prefix increment.
        ChainMapIterator& operator++() {
            if (maps_vec_ptr_ && current_map_iter_ != (*maps_vec_ptr_).end()) {
                MapType* current_map_ptr = *current_map_iter_;
                 // Ensure current_map_ptr is not null before dereferencing
                if (current_map_ptr) {
                    auto end_of_current_map_iter = IsConst ? current_map_ptr->cend() : current_map_ptr->end();
                    if (current_element_iter_ != end_of_current_map_iter) {
                        visited_keys_.insert(current_element_iter_->first); // Mark current key as visited.
                        ++current_element_iter_; // Advance the element iterator within the current map.
                    }
                }
            }
            // Find the next valid, unvisited item across all maps.
            advance_until_valid_unvisited_item();
            return *this;
        }

        // Postfix increment.
        ChainMapIterator operator++(int) {
            ChainMapIterator tmp = *this;
            ++(*this); // Call prefix increment.
            return tmp;
        }

        friend bool operator==(const ChainMapIterator& a, const ChainMapIterator& b) {
            // Both are end iterators (or default constructed).
            if (!a.maps_vec_ptr_ && !b.maps_vec_ptr_) return true;
            // One is an end iterator, the other is not.
            if (!a.maps_vec_ptr_ || !b.maps_vec_ptr_) return false;
            // Both are valid (non-end) iterators. Compare their current positions.
            return a.maps_vec_ptr_ == b.maps_vec_ptr_ &&
                   a.current_map_iter_ == b.current_map_iter_ &&
                   a.current_element_iter_ == b.current_element_iter_;
        }

        friend bool operator!=(const ChainMapIterator& a, const ChainMapIterator& b) {
            return !(a == b);
        }
    }; // End of ChainMapIterator

public:
    using key_type = K;
    using mapped_type = V;
    // value_type for ChainMap itself, consistent with map's value_type
    using value_type = std::pair<const K, V>;
    using map_type = MapType;
    using map_reference = MapType&;
    using map_const_reference = const MapType&;
    using map_pointer = MapType*;
    using const_map_pointer = const MapType*;

    // Public iterator typedefs
    using iterator = ChainMapIterator<false>;
    using const_iterator = ChainMapIterator<true>;

private:
    // maps_ stores non-const pointers to MapType.
    // const_iterator will treat these as const MapType*.
    std::vector<map_pointer> maps_;
    // Using std::list<map_pointer> maps_; could be an alternative if frequent prepend/add is a concern
    // and iterator stability of maps_ itself is critical. For now, vector is fine.

public:
    // Default constructor
    ChainMap() = default;

    // Constructor with a single map
    explicit ChainMap(map_pointer first_map) {
        if (first_map) {
            maps_.push_back(first_map);
        }
        // Note: nullptr maps are skipped. Operations might fail if maps_ remains empty
        // or if all maps are null and then accessed.
    }

    // Constructor with an initializer_list of maps
    ChainMap(std::initializer_list<map_pointer> maps_list) {
        for (map_pointer mp : maps_list) {
            if (mp) { // Skips nullptr maps
                maps_.push_back(mp);
            }
        }
    }

    // Variadic template constructor for multiple map references
    // The maps are added in the order they are provided.
    // maps_args parameter pack will ensure all arguments are of MapType& or compatible.
    template <typename... MapArgs,
              typename = std::enable_if_t<(std::is_same_v<MapType&, MapArgs&> && ...)> >
    explicit ChainMap(MapArgs&... maps_args) {
        // Store pointers to the maps. The first argument is maps_[0].
        (maps_.push_back(&maps_args), ...);
    }


    // Method to add a new map to the front of the chain (becomes the new primary map).
    // Nullptr maps are skipped.
    void prepend_layer(map_pointer new_map) {
        if (new_map) {
            maps_.insert(maps_.begin(), new_map);
        }
        // Note: Current behavior is to skip nullptr maps.
        // Consider if throwing an exception (e.g. std::invalid_argument) is preferred
        // if a nullptr is passed, depending on desired strictness.
    }

    // Appends a map to the chain (becomes the new lowest-priority map).
    // Nullptr maps are skipped.
    void add_layer(map_pointer new_map) {
        if (new_map) { // Maintain consistency: skip nullptr
            maps_.push_back(new_map);
        }
        // Note: Current behavior is to skip nullptr maps.
    }

    // Iterator methods
    iterator begin() {
        // Find the first non-null map to start iteration
        auto first_valid_map_iter = std::find_if(maps_.begin(), maps_.end(),
                                                 [](map_pointer mp){ return mp != nullptr; });
        return iterator(&maps_, first_valid_map_iter);
    }

    iterator end() {
        return iterator(); // Default constructed iterator is the end iterator
    }

    const_iterator begin() const {
        auto first_valid_map_iter = std::find_if(maps_.cbegin(), maps_.cend(),
                                                 [](const_map_pointer cmp){ return cmp != nullptr; });
        // For const_iterator, ParentMapsVectorPtrType is const std::vector<MapType*>*.
        // &maps_ is std::vector<MapType*>*. This is compatible.
        // The iterator needs a pointer to the vector to know its bounds and iterate over map pointers.
        // The const-correctness for map elements is handled by IsConst template parameter.
        return const_iterator(&maps_, first_valid_map_iter);
    }

    const_iterator end() const {
        return const_iterator();
    }

    const_iterator cbegin() const {
        // Standard implementation: call the const version of begin()
        return static_cast<const ChainMap*>(this)->begin();
    }

    const_iterator cend() const {
        // Standard implementation: call the const version of end()
        return static_cast<const ChainMap*>(this)->end();
    }

    // Returns a vector of all unique visible keys in order of precedence.
    std::vector<K> keys() const {
        std::vector<K> key_list;
        // The current size() iterates to count unique keys, so calling it here
        // for reserve() would mean iterating twice. Only reserve if size() was cheap.
        // if (size() > 0) key_list.reserve(size());

        for (const auto& item : *this) { // Uses cbegin()/cend() due to const context of this method
            key_list.push_back(item.first);
        }
        return key_list;
    }

    // Returns a vector of values for all unique visible keys, in order of precedence.
    std::vector<V> values() const {
        std::vector<V> value_list;
        // if (size() > 0) value_list.reserve(size());

        for (const auto& item : *this) {
            value_list.push_back(item.second);
        }
        return value_list;
    }

    // Returns a vector of unique visible key-value pairs in order of precedence.
    // ChainMap::value_type for iterators is std::pair<const K, V>.
    std::vector<typename ChainMap<K, V, MapType>::value_type> items() const {
        std::vector<typename ChainMap<K, V, MapType>::value_type> item_list;
        // if (size() > 0) item_list.reserve(size());

        for (const auto& item : *this) {
            item_list.push_back(item);
        }
        return item_list;
    }


    // Accesses the value associated with the key.
    // If the key is not found, it's inserted into the first (writable) map.
    mapped_type& operator[](const key_type& key) {
        // Check if key exists in any map first.
        // If it exists, and we want Python's behavior where chain[key] = new_val
        // modifies the first map if key is present anywhere, this logic needs adjustment.
        // Current operator[]: finds and returns from existing map, or inserts in first.
        for (map_pointer mp : maps_) {
            if (mp) {
                auto it = mp->find(key);
                if (it != mp->end()) {
                    // Key found in one of the maps.
                    // If we want to ensure that writing to this reference
                    // writes to the first map, we might need to copy it here.
                    // However, standard ChainMap behavior is to modify in place if found.
                    // For operator[], if found in a deeper map, and we want to write to
                    // the first map, this behavior is different.
                    // Python's ChainMap.maps[0][key] = value for setting.
                    // Let's assume operator[] always writes to the first map if key is new,
                    // or modifies in place if key exists.
                    // To ensure it writes to the first map if key is found in a subsequent map
                    // one might copy the value to the first map.
                    // The current implementation will modify in place.
                    // A more Pythonic operator[] for setting would be:
                    // if (!maps_.empty() && maps_.front()->count(key) == 0 && contains(key)) {
                    //    V val = get_value_from_chain(key); // Helper to get value
                    //    return (*maps_.front())[key] = val; // Promote to first map
                    // }
                    // This is complex. For now, let's stick to: find and return, or insert into first.
                    return it->second;
                }
            }
        }
        // Key not found in any map, insert into the first map.
        if (maps_.empty()) {
            throw std::logic_error("Cannot insert into an empty ChainMap using operator[].");
        }
        return (*maps_.front())[key]; // Inserts if key doesn't exist in the first map
    }

    // Accesses the value associated with the key (const version).
    // Throws std::out_of_range if the key is not found.
    const mapped_type& at(const key_type& key) const {
        for (const_map_pointer cmp : maps_) {
            if (cmp) {
                auto it = cmp->find(key);
                if (it != cmp->end()) {
                    return it->second;
                }
            }
        }
        throw std::out_of_range("Key not found in ChainMap.");
    }

    // Accesses the value associated with the key (non-const version).
    // Throws std::out_of_range if the key is not found.
    // Note: This allows modification of values in whichever map they are found.
    // This differs from operator[] which always affects the first map on insertion.
    mapped_type& at(const key_type& key) {
        for (map_pointer mp : maps_) {
            if (mp) {
                auto it = mp->find(key);
                if (it != mp->end()) {
                    return it->second;
                }
            }
        }
        throw std::out_of_range("Key not found in ChainMap.");
    }

    // Retrieves the value associated with the key (const version).
    // Throws std::out_of_range if the key is not found.
    // Synonym for at(const key_type&) const.
    const mapped_type& get(const key_type& key) const {
        for (const_map_pointer cmp : maps_) {
            if (cmp) { // Ensure map pointer is not null
                auto it = cmp->find(key);
                if (it != cmp->end()) {
                    return it->second;
                }
            }
        }
        // Added differentiation in message for clarity, though behavior is same as at()
        throw std::out_of_range("Key not found in ChainMap (via get).");
    }

    // Checks if a key exists in any of the maps.
    // Correctly implements: "Return true if any layer contains the key."
    bool contains(const key_type& key) const {
        for (const_map_pointer cmp : maps_) {
            if (cmp && cmp->count(key)) {
                return true;
            }
        }
        return false;
    }

    // Returns all maps in the chain.
    const std::vector<map_pointer>& get_maps() const {
        return maps_;
    }

    // Inserts or updates the key-value pair in the first (writable) map.
    // If no maps are in the chain, this will throw std::logic_error via get_writable_map().
    void insert(const key_type& key, const mapped_type& value) {
        map_pointer writable_map = get_writable_map(); // Throws if maps_ is empty
        // Using operator[] for simplicity of insert or update.
        (*writable_map)[key] = value;
    }

    // Overload for rvalue value
    void insert(const key_type& key, mapped_type&& value) {
        map_pointer writable_map = get_writable_map();
        (*writable_map)[key] = std::move(value);
    }

    // Overload for rvalue key and rvalue value
    void insert(key_type&& key, mapped_type&& value) {
        map_pointer writable_map = get_writable_map();
        (*writable_map)[std::move(key)] = std::move(value);
    }

    // Overload for rvalue key and lvalue value
    void insert(key_type&& key, const mapped_type& value) {
        map_pointer writable_map = get_writable_map();
        (*writable_map)[std::move(key)] = value;
    }

    // Erases the key from the first (writable) map only.
    // Returns the number of elements erased (0 or 1).
    // If no maps are in the chain, this will throw std::logic_error via get_writable_map().
    size_t erase(const key_type& key) {
        map_pointer writable_map = get_writable_map(); // Throws if maps_ is empty
        return writable_map->erase(key);
    }

    // Checks if the ChainMap holds any maps.
    bool empty() const noexcept {
        return maps_.empty();
    }

    // Returns the number of maps in the chain.
    // Note: This is not the number of unique items in the ChainMap.
    // For that, one would typically iterate or use a method that collects all keys.
    // size_t size() const noexcept { // This is the OLD one, to be replaced
    //     return maps_.size();
    // }

    // Calculates the total number of unique keys visible in the ChainMap.
    // This involves iterating through all maps and collecting unique keys.
    // It is const but not noexcept due to potential allocations for the set
    // and key copying.
    size_t size() const {
        // Using std::set to handle uniqueness.
        // If K is hashable and std::unordered_set is preferred for performance,
        // that could be an alternative, but std::set has broader compatibility
        // (requires only operator< for K).
        std::set<key_type> unique_keys;
        for (const_map_pointer cmp : maps_) {
            if (cmp) { // Ensure map pointer is not null
                for (const auto& pair : *cmp) {
                    unique_keys.insert(pair.first);
                }
            }
        }
        return unique_keys.size();
    }

    // Removes all maps from the ChainMap.
    void clear() {
        maps_.clear();
    }

    // Placeholder for the first map (writable map)
    // This simplifies some mutating operations but needs careful handling if maps_ is empty.
    map_pointer get_writable_map() {
        if (maps_.empty()) {
            // This case needs to be handled: either throw, or document that ChainMap must have at least one map
            // for mutable operations, or automatically create one.
            // For now, let's assume it's an error to call mutating ops on an empty ChainMap.
            throw std::logic_error("ChainMap has no layers to operate on.");
        }
        return maps_.front();
    }

    const_map_pointer get_writable_map() const {
         if (maps_.empty()) {
            throw std::logic_error("ChainMap has no layers to operate on.");
        }
        return maps_.front();
    }


};

#endif // CHAIN_MAP_HPP

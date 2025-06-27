#pragma once

#include <unordered_map>
#include <list>
#include <utility> // For std::pair, std::move
#include <stdexcept> // For std::out_of_range
#include <functional> // For std::hash, std::equal_to
#include <memory>     // For std::allocator, std::allocator_traits
#include <iterator>   // For iterator tags

namespace std_ext {

template<
    typename Key,
    typename Value,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    typename Allocator = std::allocator<std::pair<const Key, Value>>
>
class OrderedDict {
public:
    // Type aliases
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>; // Stored in the list
    using allocator_type = Allocator;
    using reference = value_type&; // Reference to pair in list
    using const_reference = const value_type&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

private:
    // Internal list to maintain order and store key-value pairs
    using list_type = std::list<value_type, Allocator>;
    using list_iterator = typename list_type::iterator;
    using const_list_iterator = typename list_type::const_iterator;

    // Internal map for quick lookup: Key -> iterator in list_type
    // The allocator for the map needs to be for std::pair<const Key, list_iterator>
    using map_value_type = list_iterator;
    using map_allocator_traits = std::allocator_traits<Allocator>;
    using map_allocator_type = typename map_allocator_traits::template rebind_alloc<std::pair<const Key, map_value_type>>;

    using map_type = std::unordered_map<
        Key,
        map_value_type,
        Hash,
        KeyEqual,
        map_allocator_type
    >;

    list_type item_list_; // Stores items in insertion order
    map_type item_map_;   // Maps keys to iterators in item_list_

public:
    // Iterators
    template<bool IsConst>
    class OrderedDictIterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = OrderedDict::value_type; // The std::pair<const Key, Value>
        using difference_type   = OrderedDict::difference_type;

        using list_iter_type = std::conditional_t<IsConst,
                                                  OrderedDict::const_list_iterator,
                                                  OrderedDict::list_iterator>;

        using pointer           = std::conditional_t<IsConst,
                                                  typename OrderedDict::const_list_iterator::pointer,
                                                  typename OrderedDict::list_iterator::pointer>;
        using reference         = std::conditional_t<IsConst,
                                                  typename OrderedDict::const_list_iterator::reference,
                                                  typename OrderedDict::list_iterator::reference>;

    private:
        list_iter_type current_list_iter_;

        // Friend declaration to allow OrderedDict to construct iterators
        friend class OrderedDict<Key, Value, Hash, KeyEqual, Allocator>;

        // Private constructor for OrderedDict to use
        // Made non-explicit to help with std::pair construction
        OrderedDictIterator(list_iter_type it) : current_list_iter_(it) {}

    public:
        // Default constructor
        OrderedDictIterator() = default;

        // Explicitly default copy and move operations
        OrderedDictIterator(const OrderedDictIterator&) = default;
        OrderedDictIterator(OrderedDictIterator&&) noexcept = default;
        OrderedDictIterator& operator=(const OrderedDictIterator&) = default;
        OrderedDictIterator& operator=(OrderedDictIterator&&) noexcept = default;

        // Allow non-const to const iterator conversion
        OrderedDictIterator(const OrderedDictIterator<false>& other) requires IsConst
            : current_list_iter_(other.current_list_iter_) {}

        reference operator*() const {
            return *current_list_iter_;
        }

        pointer operator->() const {
            return current_list_iter_.operator->();
        }

        // Prefix increment
        OrderedDictIterator& operator++() {
            ++current_list_iter_;
            return *this;
        }

        // Postfix increment
        OrderedDictIterator operator++(int) {
            OrderedDictIterator temp = *this;
            ++(*this);
            return temp;
        }

        // Prefix decrement
        OrderedDictIterator& operator--() {
            --current_list_iter_;
            return *this;
        }

        // Postfix decrement
        OrderedDictIterator operator--(int) {
            OrderedDictIterator temp = *this;
            --(*this);
            return temp;
        }

        bool operator==(const OrderedDictIterator& other) const {
            return current_list_iter_ == other.current_list_iter_;
        }

        bool operator!=(const OrderedDictIterator& other) const {
            return !(*this == other);
        }

        // Getter for internal iterator, mainly for OrderedDict internal use (e.g. erase)
        list_iter_type get_internal_list_iterator() const {
            return current_list_iter_;
        }
    };

    using iterator = OrderedDictIterator<false>;
    using const_iterator = OrderedDictIterator<true>;

    // Reverse iterators
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // Public methods will be added in subsequent steps

    // Iterators
    iterator begin() noexcept {
        return iterator(item_list_.begin());
    }

    const_iterator begin() const noexcept {
        return const_iterator(item_list_.cbegin());
    }

    const_iterator cbegin() const noexcept {
        return const_iterator(item_list_.cbegin());
    }

    iterator end() noexcept {
        return iterator(item_list_.end());
    }

    const_iterator end() const noexcept {
        return const_iterator(item_list_.cend());
    }

    const_iterator cend() const noexcept {
        return const_iterator(item_list_.cend());
    }

    // Reverse Iterators
    reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(cend());
    }

    const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(cend());
    }

    reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(cbegin());
    }

    const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(cbegin());
    }

    // Basic capacity
    bool empty() const noexcept {
        return item_list_.empty();
    }

    size_type size() const noexcept {
        return item_list_.size();
    }

    // TODO: max_size()

    // Constructors & Destructor
    explicit OrderedDict(const Allocator& alloc = Allocator())
        : item_list_(alloc), item_map_(0, Hash(), KeyEqual(), map_allocator_type(alloc)) {}

    OrderedDict(size_type bucket_count,
                const Hash& hash = Hash(),
                const KeyEqual& equal = KeyEqual(),
                const Allocator& alloc = Allocator())
        : item_list_(alloc), item_map_(bucket_count, hash, equal, map_allocator_type(alloc)) {}

    OrderedDict(size_type bucket_count,
                const Allocator& alloc)
        : item_list_(alloc), item_map_(bucket_count, Hash(), KeyEqual(), map_allocator_type(alloc)) {}

    OrderedDict(size_type bucket_count,
                const Hash& hash,
                const Allocator& alloc)
        : item_list_(alloc), item_map_(bucket_count, hash, KeyEqual(), map_allocator_type(alloc)) {}

    template<typename InputIt>
    OrderedDict(InputIt first, InputIt last,
                size_type bucket_count = 0,
                const Hash& hash = Hash(),
                const KeyEqual& equal = KeyEqual(),
                const Allocator& alloc = Allocator())
        : item_list_(alloc), item_map_(bucket_count, hash, equal, map_allocator_type(alloc)) {
        // Reserve map size if possible, though list doesn't have reserve.
        if (bucket_count == 0) {
             if constexpr (std::is_base_of_v<std::forward_iterator_tag,
                           typename std::iterator_traits<InputIt>::iterator_category>) {
                item_map_.reserve(static_cast<size_type>(std::distance(first, last)));
            }
        }
        for (auto it = first; it != last; ++it) {
            insert_or_assign(it->first, it->second); // Use insert_or_assign to handle potential duplicates in input range (last one wins)
                                                 // or simple insert if duplicates should be ignored.
                                                 // For now, let's use a simple insert logic that only adds if not present.
                                                 // This will be properly implemented with insert() later.
                                                 // For constructor, typically, last one wins or it's an error.
                                                 // Python dict constructor from iterable of pairs: last one wins.
            // Simplified insert for constructor for now, real insert logic will be more complex.
            // This will be refined when insert() is fully implemented.
            // For now, this just pushes to list and map if key is new.
            // Proper insert logic is needed. Let's defer to a proper insert method once available.
            // For now, we'll use a temporary emplace logic.
            try_emplace_back(it->first, it->second);
        }
    }

    template<typename InputIt>
    OrderedDict(InputIt first, InputIt last,
                size_type bucket_count,
                const Allocator& alloc)
        : OrderedDict(first, last, bucket_count, Hash(), KeyEqual(), alloc) {}

    template<typename InputIt>
    OrderedDict(InputIt first, InputIt last,
                const Allocator& alloc)
        : OrderedDict(first, last, 0, Hash(), KeyEqual(), alloc) {}


    OrderedDict(std::initializer_list<value_type> init,
                size_type bucket_count = 0,
                const Hash& hash = Hash(),
                const KeyEqual& equal = KeyEqual(),
                const Allocator& alloc = Allocator())
        : OrderedDict(init.begin(), init.end(), bucket_count, hash, equal, alloc) {}

    OrderedDict(std::initializer_list<value_type> init,
                size_type bucket_count,
                const Allocator& alloc)
        : OrderedDict(init.begin(), init.end(), bucket_count, Hash(), KeyEqual(), alloc) {}

    OrderedDict(std::initializer_list<value_type> init,
                const Allocator& alloc)
        : OrderedDict(init.begin(), init.end(), 0, Hash(), KeyEqual(), alloc) {}

    ~OrderedDict() = default;

    // Copy constructor
    OrderedDict(const OrderedDict& other)
        : item_list_(other.item_list_), // List copy is straightforward
          item_map_(other.item_map_.bucket_count(), other.item_map_.hash_function(), other.item_map_.key_eq(), other.get_map_allocator())
    {
        // Need to rebuild the map iterators to point to *this* list
        item_map_.clear(); // Clear the copied map iterators, they are invalid.
                           // The list item_list_ is already a copy of other.item_list_
        for (auto list_it = item_list_.begin(); list_it != item_list_.end(); ++list_it) {
            item_map_.emplace_hint(item_map_.end(), list_it->first, list_it);
        }
    }

    OrderedDict(const OrderedDict& other, const Allocator& alloc)
        : item_list_(other.item_list_, alloc), // Copy list with new allocator
          item_map_(other.item_map_.bucket_count(), other.item_map_.hash_function(), other.item_map_.key_eq(), map_allocator_type(alloc))
    {
        item_map_.clear();
        for (auto list_it = item_list_.begin(); list_it != item_list_.end(); ++list_it) {
            item_map_.emplace_hint(item_map_.end(), list_it->first, list_it);
        }
    }

    // Move constructor
    OrderedDict(OrderedDict&& other) noexcept
        : item_list_(std::move(other.item_list_)),
          item_map_(std::move(other.item_map_)) {}

    OrderedDict(OrderedDict&& other, const Allocator& alloc)
        noexcept(std::allocator_traits<Allocator>::is_always_equal::value &&
                 std::allocator_traits<map_allocator_type>::is_always_equal::value) // Simplified condition
        : item_list_(std::move(other.item_list_), alloc), // May throw if allocators differ and move not possible
          item_map_(std::move(other.item_map_), map_allocator_type(alloc)) // May throw
    {
        // If allocators are different and elements were moved (not copied),
        // the map iterators might still be valid if the list nodes themselves were moved
        // in a way that doesn't invalidate iterators (std::list::splice or node handle move).
        // However, std::list move constructor with different allocator might reallocate.
        // If allocators are equal, it's a cheap move.
        // If allocators are different, std::list's move constructor will element-wise move.
        // After elements are moved to the new list `item_list_`, the iterators in `other.item_map_`
        // are now invalid for `other.item_list_` and also potentially for `this->item_list_`
        // if the move involved reallocation and reconstruction.
        // Safest is to rebuild the map if allocators are different and a true move might not have
        // preserved iterator validity relative to the new list's memory.
        // std::unordered_map's move constructor with a different allocator will also element-wise move.
        if constexpr (!std::allocator_traits<Allocator>::is_always_equal::value) {
            if (get_allocator() != other.get_allocator() || get_map_allocator() != other.get_map_allocator()) {
                // Allocators differ, map iterators are likely invalid and need rebuilding.
                // This assumes item_list_ has been successfully moved/copied.
                item_map_.clear(); // Clear moved-from map's (now this->item_map_) potentially invalid iterators.
                for (auto list_it = item_list_.begin(); list_it != item_list_.end(); ++list_it) {
                    item_map_.emplace_hint(item_map_.end(), list_it->first, list_it);
                }
            }
        }
        // If allocators are the same, the direct move of map and list is fine.
    }


    // Copy assignment
    OrderedDict& operator=(const OrderedDict& other) {
        if (this == &other) {
            return *this;
        }

        Allocator new_list_alloc = get_allocator(); // Default to own allocator
        map_allocator_type new_map_alloc = get_map_allocator(); // Default to own allocator

        // Determine new allocators based on POCMA
        if constexpr (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value) {
            if (get_allocator() != other.get_allocator()) {
                new_list_alloc = other.get_allocator(); // Take other's list allocator
            }
        }
        if constexpr (std::allocator_traits<map_allocator_type>::propagate_on_container_copy_assignment::value) {
            if (get_map_allocator() != other.get_map_allocator()) {
                new_map_alloc = other.get_map_allocator(); // Take other's map allocator
            }
        }

        // Create a temporary list with the correct allocator and content, then swap.
        // This handles non-assignable elements correctly by copy-constructing them.
        list_type temp_list(new_list_alloc);
        // Use insert to copy elements from other.item_list_
        temp_list.insert(temp_list.end(), other.item_list_.begin(), other.item_list_.end());
        item_list_.swap(temp_list); // Swap contents with the temporary list

        // Rebuild map using the new item_list_ and target map_allocator
        item_map_.clear(); // Clear old map content
        // Create a temporary map with the correct structure and allocator, then swap
        map_type temp_map(other.item_map_.bucket_count(), // Hint bucket count
                          other.item_map_.hash_function(),
                          other.item_map_.key_eq(),
                          new_map_alloc);
        item_map_.swap(temp_map); // Swap to get new map structure with correct allocator

        // Populate the new map
        item_map_.reserve(item_list_.size()); // Reserve space for efficiency
        for (auto list_it = item_list_.begin(); list_it != item_list_.end(); ++list_it) {
            item_map_.emplace_hint(item_map_.end(), list_it->first, list_it);
        }
        return *this;
    }

    // Move assignment
    OrderedDict& operator=(OrderedDict&& other) noexcept (
        std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value ||
        std::allocator_traits<Allocator>::is_always_equal::value
    ) {
        if (this == &other) {
            return *this;
        }

        constexpr bool pocma_list = std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value;
        constexpr bool pocma_map = std::allocator_traits<map_allocator_type>::propagate_on_container_move_assignment::value;
        constexpr bool list_always_equal = std::allocator_traits<Allocator>::is_always_equal::value;
        constexpr bool map_always_equal = std::allocator_traits<map_allocator_type>::is_always_equal::value;

        const Allocator other_list_alloc = other.get_allocator();
        const map_allocator_type other_map_alloc = other.get_map_allocator();

        if constexpr (pocma_list && pocma_map) { // Both propagate
            item_list_ = std::move(other.item_list_);
            item_map_ = std::move(other.item_map_);
        } else if constexpr (list_always_equal && map_always_equal) { // Allocators are always equal
            item_list_ = std::move(other.item_list_);
            item_map_ = std::move(other.item_map_);
        } else { // More complex cases: allocators might differ and not propagate
            if (get_allocator() == other_list_alloc && get_map_allocator() == other_map_alloc) {
                // Allocators are equal, simple move
                item_list_ = std::move(other.item_list_);
                item_map_ = std::move(other.item_map_);
            } else {
                // Allocators differ and don't propagate or aren't always equal.
                // This requires element-wise move for the list, and then rebuild the map.
                // This is similar to copy assignment but with move semantics for elements.
                // For simplicity, let's implement a clear and then rebuild.
                // A more optimized version would try to reuse nodes if possible.

                // Clear current state
                clear(); // Defined later, for now assume it clears list and map

                // If list allocator needs to propagate and can, or is same
                if constexpr (pocma_list) {
                    item_list_ = std::move(other.item_list_); // List gets other's allocator
                } else if (get_allocator() == other_list_alloc) {
                    item_list_ = std::move(other.item_list_); // List keeps its allocator, element-wise move
                } else {
                    // List allocators differ and won't propagate. Element-wise move.
                    // This is tricky. For now, assign (which copies/moves elements)
                    // then rebuild map.
                    // This path implies potential performance cost or specific handling.
                    // A full clear and rebuild from other is safest if not optimizing deeply here.
                    item_list_.assign(std::make_move_iterator(other.item_list_.begin()),
                                      std::make_move_iterator(other.item_list_.end()));
                }

                // Rebuild map using this->item_list_
                // Map allocator propagation or equality check
                if constexpr (pocma_map) {
                     item_map_ = map_type(other.item_map_.bucket_count(),
                                         other.item_map_.hash_function(),
                                         other.item_map_.key_eq(),
                                         other_map_alloc); // Map gets other's allocator
                } else if (get_map_allocator() == other_map_alloc) {
                    // Map keeps its allocator, can try to move from other.item_map_
                    // but iterators will be wrong. So, just set up structure.
                     item_map_ = map_type(other.item_map_.bucket_count(),
                                         other.item_map_.hash_function(),
                                         other.item_map_.key_eq(),
                                         get_map_allocator());
                } else {
                    // Map allocators differ and won't propagate.
                    // item_map_ keeps its allocator. Set up structure.
                     item_map_ = map_type(other.item_map_.bucket_count(),
                                         other.item_map_.hash_function(),
                                         other.item_map_.key_eq(),
                                         get_map_allocator());
                }

                item_map_.clear(); // Ensure map is empty before repopulating
                item_map_.reserve(item_list_.size());
                for (auto list_it = item_list_.begin(); list_it != item_list_.end(); ++list_it) {
                    item_map_.emplace_hint(item_map_.end(), list_it->first, list_it);
                }
                other.clear(); // Ensure other is left in a valid empty state
            }
        }
        return *this;
    }

    // Allocator access
    allocator_type get_allocator() const noexcept {
        return item_list_.get_allocator();
    }

    map_allocator_type get_map_allocator() const noexcept {
        return item_map_.get_allocator();
    }

    // Element access (at, operator[])
    mapped_type& operator[](const key_type& key) {
        auto map_it = item_map_.find(key);
        if (map_it == item_map_.end()) {
            // Key doesn't exist, create it with a default-constructed value
            item_list_.emplace_back(key, mapped_type{});
            list_iterator new_node_iter = std::prev(item_list_.end());
            auto result = item_map_.emplace_hint(item_map_.end(), key, new_node_iter);
            return result.first->second->second; // return reference to the new value
        } else {
            // Key exists, return reference to its value
            return map_it->second->second;
        }
    }

    mapped_type& operator[](key_type&& key) {
        auto map_it = item_map_.find(key);
        if (map_it == item_map_.end()) {
            // Key doesn't exist, create it with a default-constructed value
            // Move key into list
            item_list_.emplace_back(std::move(key), mapped_type{});
            list_iterator new_node_iter = std::prev(item_list_.end());
            // Key has been moved, so when inserting into map, we use list_node_iter->first
            auto map_emplaced_iter = item_map_.emplace_hint(item_map_.end(), new_node_iter->first, new_node_iter);
            return map_emplaced_iter->second->second;
        } else {
            // Key exists
            return map_it->second->second;
        }
    }

    mapped_type& at(const key_type& key) {
        auto map_it = item_map_.find(key);
        if (map_it == item_map_.end()) {
            throw std::out_of_range("OrderedDict::at: key not found");
        }
        return map_it->second->second;
    }

    const mapped_type& at(const key_type& key) const {
        auto map_it = item_map_.find(key);
        if (map_it == item_map_.end()) {
            throw std::out_of_range("OrderedDict::at: key not found");
        }
        return map_it->second->second;
    }

    // Modifiers
    std::pair<iterator, bool> insert(const value_type& value) {
        auto map_it = item_map_.find(value.first);
        if (map_it == item_map_.end()) {
            item_list_.push_back(value);
            list_iterator new_node_iter = std::prev(item_list_.end());
            item_map_.emplace_hint(item_map_.end(), value.first, new_node_iter);
            return std::pair<iterator, bool>(iterator(new_node_iter), true);
        }
        return std::pair<iterator, bool>(iterator(map_it->second), false);
    }

    std::pair<iterator, bool> insert(value_type&& value) {
        auto map_it = item_map_.find(value.first);
        if (map_it == item_map_.end()) {
            // key from rvalue value needs to be const for map key if it's directly used.
            // list stores const key, so value.first can be moved if it's non-const in pair.
            // However, value_type is std::pair<const Key, Value>, so value.first is const.
            item_list_.push_back(std::move(value));
            list_iterator new_node_iter = std::prev(item_list_.end());
            // Use new_node_iter->first as the key for the map as it's taken from the list
            item_map_.emplace_hint(item_map_.end(), new_node_iter->first, new_node_iter);
            return std::pair<iterator, bool>(iterator(new_node_iter), true);
        }
        return std::pair<iterator, bool>(iterator(map_it->second), false);
    }

    // insert with hint
    iterator insert(const_iterator hint, const value_type& value) {
        // Hint could be used to optimize finding the key in map or insertion point in list,
        // but for OrderedDict, list insertion is always at the end for new elements.
        // Map hint can be used.
        auto map_it = item_map_.find(value.first);
        if (map_it == item_map_.end()) {
            item_list_.push_back(value);
            list_iterator new_node_iter = std::prev(item_list_.end());
            // Use hint for map emplace if possible, though std::list::end is fixed for new items.
            // The hint for unordered_map refers to a location where search for the key should start.
            // We don't have a map iterator from const_iterator directly.
            item_map_.emplace(value.first, new_node_iter); // emplace_hint could be better if we had a map iterator
            return iterator(new_node_iter);
        }
        return iterator(map_it->second); // Key already exists
    }

    iterator insert(const_iterator hint, value_type&& value) {
        auto map_it = item_map_.find(value.first);
        if (map_it == item_map_.end()) {
            item_list_.push_back(std::move(value));
            list_iterator new_node_iter = std::prev(item_list_.end());
            item_map_.emplace(new_node_iter->first, new_node_iter);
            return iterator(new_node_iter);
        }
        return iterator(map_it->second);
    }

    template<typename M>
    std::pair<iterator, bool> insert_or_assign(const key_type& k, M&& obj) {
        auto map_it = item_map_.find(k);
        if (map_it == item_map_.end()) {
            // Insert new: add to end of list
            item_list_.emplace_back(k, std::forward<M>(obj));
            list_iterator new_node_iter = std::prev(item_list_.end());
            item_map_.emplace_hint(item_map_.end(), k, new_node_iter);
            return std::pair<iterator, bool>(iterator(new_node_iter), true);
        } else {
            // Assign existing: update value, no change in order
            list_iterator list_node_iter = map_it->second;
            list_node_iter->second = std::forward<M>(obj);
            return std::pair<iterator, bool>(iterator(list_node_iter), false);
        }
    }

    template<typename M>
    std::pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj) {
        auto map_it = item_map_.find(k); // k is an rvalue reference here
        if (map_it == item_map_.end()) {
            // Insert new: add to end of list, move k
            item_list_.emplace_back(std::move(k), std::forward<M>(obj));
            list_iterator new_node_iter = std::prev(item_list_.end());
            // k has been moved, use new_node_iter->first for map key
            item_map_.emplace_hint(item_map_.end(), new_node_iter->first, new_node_iter);
            return std::pair<iterator, bool>(iterator(new_node_iter), true);
        } else {
            // Assign existing: update value, no change in order
            list_iterator list_node_iter = map_it->second;
            list_node_iter->second = std::forward<M>(obj);
            // k (original argument) is not used if key found, map_it->first is const Key&
            return std::pair<iterator, bool>(iterator(list_node_iter), false);
        }
    }

    template<typename M>
    iterator insert_or_assign(const_iterator hint, const key_type& k, M&& obj) {
        // Hint can be used for map, but order is fixed for new elements.
        return insert_or_assign(k, std::forward<M>(obj)).first;
    }

    template<typename M>
    iterator insert_or_assign(const_iterator hint, key_type&& k, M&& obj) {
        return insert_or_assign(std::move(k), std::forward<M>(obj)).first;
    }


    template<typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        // Construct a temporary value_type to extract the key for lookup
        // This is a bit tricky as we don't want to construct it if the key already exists.
        // A more efficient emplace would try to construct key first, then value if key is new.
        // For now, let's use a slightly less efficient approach for simplicity or
        // construct directly into the list and then conditionally add to map.

        // Create the object in the list first.
        // This might be inefficient if the key already exists, as we create an object unnecessarily.
        // A better way might be to check map first, but then constructing key for lookup.

        // Let's try to build key first if possible, or use a temporary pair.
        // If Args can construct value_type:
        value_type temp_val(std::forward<Args>(args)...); // This constructs the pair

        auto map_it = item_map_.find(temp_val.first);
        if (map_it == item_map_.end()) {
            item_list_.emplace_back(std::move(temp_val)); // Move the temp_val into list
            list_iterator new_node_iter = std::prev(item_list_.end());
            item_map_.emplace_hint(item_map_.end(), new_node_iter->first, new_node_iter);
            return std::pair<iterator, bool>(iterator(new_node_iter), true);
        }
        // Key already exists
        return std::pair<iterator, bool>(iterator(map_it->second), false);
    }

    template <class... Args>
    std::pair<iterator,bool> try_emplace(const key_type& k, Args&&... args) {
        auto map_it = item_map_.find(k);
        if (map_it == item_map_.end()) {
            item_list_.emplace_back(std::piecewise_construct,
                                   std::forward_as_tuple(k),
                                   std::forward_as_tuple(std::forward<Args>(args)...));
            list_iterator new_node_iter = std::prev(item_list_.end());
            item_map_.emplace_hint(item_map_.end(), k, new_node_iter);
            return std::pair<iterator, bool>(iterator(new_node_iter), true);
        }
        return std::pair<iterator, bool>(iterator(map_it->second), false);
    }

    template <class... Args>
    std::pair<iterator,bool> try_emplace(key_type&& k, Args&&... args) {
        auto map_it = item_map_.find(k);
        if (map_it == item_map_.end()) {
            item_list_.emplace_back(std::piecewise_construct,
                                   std::forward_as_tuple(std::move(k)),
                                   std::forward_as_tuple(std::forward<Args>(args)...));
            list_iterator new_node_iter = std::prev(item_list_.end());
            item_map_.emplace_hint(item_map_.end(), new_node_iter->first, new_node_iter);
            return std::pair<iterator, bool>(iterator(new_node_iter), true);
        }
        return std::pair<iterator, bool>(iterator(map_it->second), false);
    }

    // try_emplace with hint
    template <class... Args>
    iterator try_emplace(const_iterator hint, const key_type& k, Args&&... args) {
        // Hint is for map. Order of insertion in list is fixed at end for new elements.
        return try_emplace(k, std::forward<Args>(args)...).first;
    }

    template <class... Args>
    iterator try_emplace(const_iterator hint, key_type&& k, Args&&... args) {
        return try_emplace(std::move(k), std::forward<Args>(args)...).first;
    }


    size_type erase(const key_type& key) {
        auto map_it = item_map_.find(key);
        if (map_it != item_map_.end()) {
            list_iterator list_node_to_erase = map_it->second;
            item_list_.erase(list_node_to_erase);
            item_map_.erase(map_it); // map_it is iterator to map element
            return 1;
        }
        return 0;
    }

    iterator erase(iterator pos) {
        if (pos == end()) return end(); // Cannot erase end iterator

        // Get the key from the iterator to remove from map
        // The iterator `pos` wraps a list_iterator.
        list_iterator list_node_to_erase = pos.get_internal_list_iterator();
        const key_type& key = list_node_to_erase->first;

        // Erase from list, get iterator to next element
        list_iterator next_list_iter = item_list_.erase(list_node_to_erase);

        // Erase from map
        item_map_.erase(key);

        return iterator(next_list_iter);
    }

    iterator erase(const_iterator pos) { // For const_iterator
         if (pos == cend()) return end();
        list_iterator list_node_to_erase = const_cast<OrderedDict<Key, Value, Hash, KeyEqual, Allocator>*>(this)->item_list_.erase(pos.get_internal_list_iterator());
        // This const_cast is a bit ugly. A better way is to make iterator store a pointer to the parent
        // or have a non-const version of get_internal_list_iterator if the iterator itself is non-const.
        // For now, this works if erase(const_iterator) is called on a non-const OrderedDict.
        // A common pattern is to implement erase(const_iterator) by finding the equivalent non-const iterator.
        // Let's use key for map erase.
        const key_type& key = pos->first; // pos is const_iterator, pos->first is const Key&
        item_map_.erase(key);
        return iterator(list_node_to_erase); // Return non-const iterator
    }


    void swap(OrderedDict& other) noexcept (
        std::allocator_traits<Allocator>::propagate_on_container_swap::value &&
        std::allocator_traits<map_allocator_type>::propagate_on_container_swap::value
    ) {
        using std::swap;
        // Swap allocators if they propagate
        if constexpr (std::allocator_traits<Allocator>::propagate_on_container_swap::value) {
            swap(item_list_.get_allocator(), other.item_list_.get_allocator());
        }
        if constexpr (std::allocator_traits<map_allocator_type>::propagate_on_container_swap::value) {
            swap(item_map_.get_allocator(), other.item_map_.get_allocator());
        }
        // Actual swap of contents
        item_list_.swap(other.item_list_);
        item_map_.swap(other.item_map_);
    }

    // Clear (make public)
    void clear() noexcept {
        item_list_.clear();
        item_map_.clear();
    }

    // Lookup
    iterator find(const key_type& key) {
        auto map_it = item_map_.find(key);
        if (map_it == item_map_.end()) {
            return end();
        }
        return iterator(map_it->second);
    }

    const_iterator find(const key_type& key) const {
        auto map_it = item_map_.find(key);
        if (map_it == item_map_.end()) {
            return cend(); // Use cend() for const_iterator
        }
        // Explicitly convert list_iterator to const_list_iterator to resolve ambiguity
        typename list_type::const_iterator const_list_it = map_it->second;
        return const_iterator(const_list_it);
    }

    size_type count(const key_type& key) const {
        return item_map_.count(key); // Returns 0 or 1
    }

    bool contains(const key_type& key) const {
        return item_map_.contains(key);
    }

    // Capacity (empty() and size() are already implemented near the top)
    size_type max_size() const noexcept {
        // The maximum size is limited by both the list and the map.
        // std::list::max_size() and std::unordered_map::max_size().
        // The smaller of the two is the practical limit.
        // Also allocator limits apply.
        return std::min(item_list_.max_size(), item_map_.max_size());
    }

    // Python-like methods
    std::pair<key_type, mapped_type> popitem(bool last = true) {
        if (empty()) {
            throw std::out_of_range("OrderedDict::popitem: dictionary is empty");
        }

        list_iterator item_to_pop_iter;
        if (last) {
            item_to_pop_iter = std::prev(item_list_.end()); // Last element
        } else {
            item_to_pop_iter = item_list_.begin(); // First element
        }

        // Copy key and value before erasing.
        // Key must be copied because item_to_pop_iter->first is const Key&.
        // Value can be moved from.
        key_type key = item_to_pop_iter->first;
        mapped_type value = std::move(item_to_pop_iter->second);

        // Erase from map first (using the copied key)
        item_map_.erase(key);
        // Erase from list
        item_list_.erase(item_to_pop_iter);

        // Return the copied key and moved value.
        // The key_type for std::pair will be a copy.
        return {std::move(key), std::move(value)};
    }

private:
    // Helper for range constructor and initializer_list constructor
    // Tries to emplace if key doesn't exist. If key exists, it updates the value
    // and moves the element to the end of the list to respect last-one-wins semantics for constructors.
    template<typename K, typename V_construct>
    void try_emplace_back(K&& key_arg, V_construct&& value_arg) {
        auto map_it = item_map_.find(key_arg);
        if (map_it == item_map_.end()) {
            // Key does not exist, add to end of list and then to map
            // Use const_cast for key if K is not const Key, because list stores const Key
            item_list_.emplace_back(std::forward<K>(key_arg), std::forward<V_construct>(value_arg));
            item_map_.emplace_hint(item_map_.end(), item_list_.back().first, std::prev(item_list_.end()));
        } else {
            // Key exists, update value and move to end
            list_iterator list_node_iter = map_it->second;
            list_node_iter->second = std::forward<V_construct>(value_arg); // Update value
            // Move node to the end of the list
            if (std::next(list_node_iter) != item_list_.end()) { // only move if not already last
                item_list_.splice(item_list_.end(), item_list_, list_node_iter);
                // map_it->second is already correct as splice doesn't invalidate iterators to the moved element
            }
        }
    }

public:
    // Clear method is already defined in the public section earlier (around line 778)
    // Removing this duplicate or misplaced private one.
    /*
    void clear() noexcept {
        item_list_.clear();
        item_map_.clear();
    }
    */


}; // OrderedDict class end


// Non-member functions (swap, comparison operators)
// TODO: Implement these

template<typename K, typename V, typename H, typename KE, typename A>
void swap(OrderedDict<K, V, H, KE, A>& lhs, OrderedDict<K, V, H, KE, A>& rhs) noexcept {
    lhs.swap(rhs); // Forward to member swap (to be implemented)
}

// Comparison operators (equality, inequality)
// Two OrderedDicts are equal if they have the same elements in the same order.
template<typename K, typename V, typename H, typename KE, typename A>
bool operator==(const OrderedDict<K, V, H, KE, A>& lhs, const OrderedDict<K, V, H, KE, A>& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    // Compare element by element in order
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template<typename K, typename V, typename H, typename KE, typename A>
bool operator!=(const OrderedDict<K, V, H, KE, A>& lhs, const OrderedDict<K, V, H, KE, A>& rhs) {
    return !(lhs == rhs);
}

} // namespace std_ext

#pragma once

#include <unordered_map>
#include <list>
#include <utility>      // For std::pair, std::move
#include <functional>   // For std::hash, std::equal_to
#include <stdexcept>    // For std::out_of_range
#include <optional>     // For pop_front/pop_back
#include <memory>       // For std::allocator_traits
#include <initializer_list>

namespace cpp_collections {

template <
    typename Key,
    typename Value,
    typename Hasher = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    typename Allocator = std::allocator<std::pair<const Key, Value>>
>
class InsertionOrderedMap {
public:
    // Types
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>;
    using hasher = Hasher;
    using key_equal = KeyEqual;
    using allocator_type = Allocator;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

private:
    using node_list_type = std::list<value_type, Allocator>;
    using map_value_type = typename node_list_type::iterator;
    using key_map_type = std::unordered_map<
        key_type,
        map_value_type,
        hasher,
        key_equal,
        typename std::allocator_traits<Allocator>::template rebind_alloc<std::pair<const key_type, map_value_type>>
    >;

    node_list_type list_;
    key_map_type map_;

public:
    // Iterators
    template <bool IsConst>
    class CommonIterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = InsertionOrderedMap::value_type;
        using difference_type = InsertionOrderedMap::difference_type;
        using list_iterator_type = std::conditional_t<IsConst, typename node_list_type::const_iterator, typename node_list_type::iterator>;
        using pointer = std::conditional_t<IsConst, InsertionOrderedMap::const_pointer, InsertionOrderedMap::pointer>;
        using reference = std::conditional_t<IsConst, InsertionOrderedMap::const_reference, InsertionOrderedMap::reference>;

    private:
        list_iterator_type list_iter_;

        friend class InsertionOrderedMap;
        // Made non-explicit to simplify pair returns and internal conversions
        CommonIterator(list_iterator_type it) : list_iter_(it) {}

    public:
        CommonIterator() = default;
        CommonIterator(const CommonIterator& other) = default;
        CommonIterator(CommonIterator&& other) noexcept = default;
        CommonIterator& operator=(const CommonIterator& other) = default;
        CommonIterator& operator=(CommonIterator&& other) noexcept = default;

        // Allow conversion from non-const to const iterator
        CommonIterator(const CommonIterator<false>& other) requires IsConst : list_iter_(other.list_iter_) {}


        reference operator*() const { return *list_iter_; }
        pointer operator->() const { return &(*list_iter_); }

        CommonIterator& operator++() { ++list_iter_; return *this; }
        CommonIterator operator++(int) { CommonIterator temp = *this; ++list_iter_; return temp; }
        CommonIterator& operator--() { --list_iter_; return *this; }
        CommonIterator operator--(int) { CommonIterator temp = *this; --list_iter_; return temp; }

        bool operator==(const CommonIterator& other) const { return list_iter_ == other.list_iter_; }
        bool operator!=(const CommonIterator& other) const { return list_iter_ != other.list_iter_; }
    };

    using iterator = CommonIterator<false>;
    using const_iterator = CommonIterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // Constructors
    InsertionOrderedMap() : InsertionOrderedMap(Hasher(), KeyEqual(), Allocator()) {}

    explicit InsertionOrderedMap(const Allocator& alloc)
        : list_(alloc), map_(0, Hasher(), KeyEqual(), typename std::allocator_traits<Allocator>::template rebind_alloc<std::pair<const key_type, map_value_type>>(alloc)) {}

    explicit InsertionOrderedMap(const Hasher& hash, const KeyEqual& equal = KeyEqual(), const Allocator& alloc = Allocator())
        : list_(alloc), map_(0, hash, equal, typename std::allocator_traits<Allocator>::template rebind_alloc<std::pair<const key_type, map_value_type>>(alloc)) {}

    explicit InsertionOrderedMap(size_type bucket_count, const Hasher& hash = Hasher(), const KeyEqual& equal = KeyEqual(), const Allocator& alloc = Allocator())
        : list_(alloc), map_(bucket_count, hash, equal, typename std::allocator_traits<Allocator>::template rebind_alloc<std::pair<const key_type, map_value_type>>(alloc)) {}


    template <typename InputIt>
    InsertionOrderedMap(InputIt first, InputIt last,
                        size_type bucket_count = 0,
                        const Hasher& hash = Hasher(),
                        const KeyEqual& equal = KeyEqual(),
                        const Allocator& alloc = Allocator())
        : InsertionOrderedMap(bucket_count, hash, equal, alloc) {
        insert(first, last);
    }

    template <typename InputIt>
    InsertionOrderedMap(InputIt first, InputIt last, const Allocator& alloc)
        : InsertionOrderedMap(first, last, 0, Hasher(), KeyEqual(), alloc) {}


    InsertionOrderedMap(std::initializer_list<value_type> ilist,
                        size_type bucket_count = 0,
                        const Hasher& hash = Hasher(),
                        const KeyEqual& equal = KeyEqual(),
                        const Allocator& alloc = Allocator())
        : InsertionOrderedMap(bucket_count, hash, equal, alloc) {
        insert(ilist.begin(), ilist.end());
    }

    InsertionOrderedMap(std::initializer_list<value_type> ilist, const Allocator& alloc)
      : InsertionOrderedMap(ilist, 0, Hasher(), KeyEqual(), alloc) {}


    InsertionOrderedMap(const InsertionOrderedMap& other)
        : list_(other.list_.get_allocator()),
          map_(other.map_.bucket_count(), other.map_.hash_function(), other.map_.key_eq(), other.map_.get_allocator()) {
        for (const auto& pair : other.list_) {
            // Emplace directly to avoid potential issues if Key or Value are not copy-assignable
            // but are copy-constructible for the list, and then set up the map.
            list_.push_back(pair);
            map_.emplace(pair.first, std::prev(list_.end()));
        }
    }

    InsertionOrderedMap(InsertionOrderedMap&& other) noexcept
        : list_(std::move(other.list_)), map_(std::move(other.map_)) {}

    InsertionOrderedMap& operator=(const InsertionOrderedMap& other) {
        if (this == &other) return *this;
        clear(); // Clear current content
        // Use other's allocator properties for the new structures if propagate_on_container_copy_assignment is true
        // For simplicity, we'll re-construct with other's allocators/properties then copy elements.
        // This is a basic copy assignment; more advanced allocator handling could be added.
        list_ = node_list_type(other.list_.get_allocator());
        map_ = key_map_type(other.map_.bucket_count(), other.map_.hash_function(), other.map_.key_eq(), other.map_.get_allocator());

        for (const auto& pair : other.list_) {
            list_.push_back(pair);
            map_.emplace(pair.first, std::prev(list_.end()));
        }
        return *this;
    }

    InsertionOrderedMap& operator=(InsertionOrderedMap&& other) noexcept {
        if (this == &other) return *this;
        clear(); // Important to release resources
        list_ = std::move(other.list_);
        map_ = std::move(other.map_);
        return *this;
    }

    // Iterators
    iterator begin() noexcept { return iterator(list_.begin()); }
    const_iterator begin() const noexcept { return const_iterator(list_.begin()); }
    const_iterator cbegin() const noexcept { return const_iterator(list_.cbegin()); }
    iterator end() noexcept { return iterator(list_.end()); }
    const_iterator end() const noexcept { return const_iterator(list_.end()); }
    const_iterator cend() const noexcept { return const_iterator(list_.cend()); }

    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(cend()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(cbegin()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

    // Capacity
    bool empty() const noexcept { return list_.empty(); }
    size_type size() const noexcept { return list_.size(); }
    size_type max_size() const noexcept { return list_.max_size(); }

    // Modifiers
    void clear() noexcept {
        list_.clear();
        map_.clear();
    }

    // Insert
    std::pair<iterator, bool> insert(const value_type& value) {
        auto map_it = map_.find(value.first);
        if (map_it != map_.end()) {
            return std::make_pair(iterator(map_it->second), false); // Key already exists
        }
        list_.push_back(value);
        auto list_it = std::prev(list_.end());
        map_.emplace(value.first, list_it);
        return std::make_pair(iterator(list_it), true);
    }

    std::pair<iterator, bool> insert(value_type&& value) {
        auto map_it = map_.find(value.first);
        if (map_it != map_.end()) {
            return std::make_pair(iterator(map_it->second), false); // Key already exists
        }
        // Need to capture key for map emplace if value is moved from.
        // If key_type is not the same as value.first's type after move.
        // However, value.first is const Key, so it won't be moved from.
        list_.push_back(std::move(value));
        auto list_it = std::prev(list_.end());
        // list_it->first is the key to emplace in the map
        map_.emplace(list_it->first, list_it);
        return std::make_pair(iterator(list_it), true);
    }

    template <typename P>
    std::enable_if_t<std::is_constructible_v<value_type, P&&>, std::pair<iterator, bool>>
    insert(P&& value) {
        // This overload is tricky due to the key being const.
        // We must construct a value_type first to extract the key.
        // If P is value_type& or const value_type&, it calls the above.
        // If P is value_type&&, it calls the above.
        // This is for convertible types like std::pair<Key, Value> (non-const Key).
        value_type val_to_insert(std::forward<P>(value));
        return insert(std::move(val_to_insert)); // Use move to avoid another copy if val_to_insert is temporary
    }


    iterator insert(const_iterator hint, const value_type& value) {
        // Hint for unordered_map is not as effective, but list part can use it.
        // For simplicity, we ignore hint for map_ and check existence.
        auto map_it = map_.find(value.first);
        if (map_it != map_.end()) {
            return iterator(map_it->second); // Key already exists, return iterator to it
        }
        // Use hint for list insertion. std::list::insert takes a const_iterator.
        typename node_list_type::const_iterator list_hint_iter = hint.list_iter_;
        auto list_it = list_.insert(list_hint_iter, value);
        map_.emplace(value.first, list_it);
        return iterator(list_it);
    }

    iterator insert(const_iterator hint, value_type&& value) {
        auto map_it = map_.find(value.first);
        if (map_it != map_.end()) {
            return iterator(map_it->second);
        }
        typename node_list_type::const_iterator list_hint_iter = hint.list_iter_;
        auto list_it = list_.insert(list_hint_iter, std::move(value));
        map_.emplace(list_it->first, list_it); // list_it->first is safe as value.first is const Key
        return iterator(list_it);
    }

    template <typename InputIt>
    void insert(InputIt first, InputIt last) {
        for (auto it = first; it != last; ++it) {
            insert(*it); // Calls the pair insert
        }
    }

    void insert(std::initializer_list<value_type> ilist) {
        insert(ilist.begin(), ilist.end());
    }

    // Emplace
    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        // Construct the value_type in the list first, then check map.
        // This is slightly different from std::map::emplace which constructs node first.
        // A more complex emplace would try to construct key first, check map, then construct in list.
        // For now, simpler: construct in a temporary list node to get key, check map.
        // If successful, move to main list. If not, discard.
        // This is inefficient. A better way:
        // Emplace into list, then try to emplace into map. If map fails, erase from list.

        // Create a temporary value to extract the key.
        // This requires value_type to be constructible from Args...
        // And Key to be extractable. This is complex.
        // A common pattern for emplace in such structures:
        // 1. Construct key from args if possible, or use a temporary object.
        // 2. Check if key exists in map.
        // 3. If not, emplace into list, then emplace iterator into map.

        // Let's try to construct the element directly at the end of the list
        // and then deal with the map.
        list_.emplace_back(std::forward<Args>(args)...);
        auto list_it = std::prev(list_.end());
        const key_type& key_ref = list_it->first; // Key from the newly emplaced element

        auto map_emplace_result = map_.try_emplace(key_ref, list_it);
        if (map_emplace_result.second) { // Emplace in map succeeded
            return std::make_pair(iterator(list_it), true);
        } else { // Key already existed in map, so current emplace fails. Rollback list.
            list_.erase(list_it); // Erase the prematurely added element
            // map_emplace_result.first is iterator to existing element in map
            return std::make_pair(iterator(map_emplace_result.first->second), false);
        }
    }

    template <typename... Args>
    iterator emplace_hint(const_iterator hint, Args&&... args) {
        // Similar to emplace, but with hint for list.
        // For now, let's simplify and ignore hint for map, use list hint.
        // Create value in list using hint. std::list::emplace takes a const_iterator.
        typename node_list_type::const_iterator list_hint_iter = hint.list_iter_;

        auto list_it = list_.emplace(list_hint_iter, std::forward<Args>(args)...);
        const key_type& key_ref = list_it->first;

        auto map_emplace_result = map_.try_emplace(key_ref, list_it);
        if (map_emplace_result.second) {
            return iterator(list_it);
        } else {
            // Key already existed. Erase the one we just added to list.
            // Return iterator to the existing element.
            list_.erase(list_it);
            return iterator(map_emplace_result.first->second);
        }
    }

    // Insert or assign
    template <typename M>
    std::pair<iterator, bool> insert_or_assign(const Key& k, M&& obj) {
        auto map_it = map_.find(k);
        if (map_it != map_.end()) { // Key exists, assign
            map_it->second->second = std::forward<M>(obj); // Update value in list
            return std::make_pair(iterator(map_it->second), false);
        } else { // Key does not exist, insert
            list_.emplace_back(k, std::forward<M>(obj));
            auto list_it = std::prev(list_.end());
            map_.emplace(k, list_it);
            return std::make_pair(iterator(list_it), true);
        }
    }

    template <typename M>
    std::pair<iterator, bool> insert_or_assign(Key&& k, M&& obj) {
        auto map_it = map_.find(k); // Find with k before potential move
        if (map_it != map_.end()) { // Key exists, assign
            map_it->second->second = std::forward<M>(obj);
            return std::make_pair(iterator(map_it->second), false);
        } else { // Key does not exist, insert
            // k is rvalue, can be moved for list construction
            list_.emplace_back(std::move(k), std::forward<M>(obj));
            auto list_it = std::prev(list_.end());
            // list_it->first is the key (potentially moved-from k)
            map_.emplace(list_it->first, list_it);
            return std::make_pair(iterator(list_it), true);
        }
    }

    // Erase
    iterator erase(iterator pos) {
        if (pos == end()) return end();
        auto list_iter_to_erase = pos.list_iter_;
        key_type key_to_erase = list_iter_to_erase->first;

        auto next_list_iter = list_.erase(list_iter_to_erase);
        map_.erase(key_to_erase);
        return iterator(next_list_iter);
    }

    iterator erase(const_iterator pos) {
         if (pos == cend()) return end();
        // Need to convert const_iterator to non-const list iterator if list_::erase needs non-const
        // std::list::erase(const_iterator) returns iterator.
        auto list_iter_to_erase = pos.list_iter_; // list_iter_ is already the correct type from CommonIterator
        key_type key_to_erase = list_iter_to_erase->first;

        auto next_list_iter = list_.erase(list_iter_to_erase);
        map_.erase(key_to_erase);
        return iterator(next_list_iter);
    }

    size_type erase(const Key& key) {
        auto map_it = map_.find(key);
        if (map_it == map_.end()) {
            return 0; // Key not found
        }
        list_.erase(map_it->second); // Erase from list using stored iterator
        map_.erase(map_it);          // Erase from map
        return 1;
    }

    void swap(InsertionOrderedMap& other) noexcept {
        using std::swap;
        swap(list_, other.list_);
        swap(map_, other.map_);
    }

    // Lookup
    mapped_type& at(const Key& key) {
        auto map_it = map_.find(key);
        if (map_it == map_.end()) {
            throw std::out_of_range("InsertionOrderedMap::at: key not found");
        }
        return map_it->second->second; // value part of pair in list
    }

    const mapped_type& at(const Key& key) const {
        auto map_it = map_.find(key);
        if (map_it == map_.end()) {
            throw std::out_of_range("InsertionOrderedMap::at: key not found");
        }
        return map_it->second->second;
    }

    mapped_type& operator[](const Key& key) {
        auto map_it = map_.find(key);
        if (map_it != map_.end()) {
            return map_it->second->second;
        }
        // Key not found, insert with default-constructed value
        list_.emplace_back(key, mapped_type{});
        auto list_it = std::prev(list_.end());
        map_.emplace(key, list_it);
        return list_it->second;
    }

    mapped_type& operator[](Key&& key) {
        auto map_it = map_.find(key); // Find with key before potential move
        if (map_it != map_.end()) {
            return map_it->second->second;
        }
        // Key not found, insert with default-constructed value
        // k is rvalue, can be moved for list construction
        list_.emplace_back(std::move(key), mapped_type{});
        auto list_it = std::prev(list_.end());
        map_.emplace(list_it->first, list_it); // list_it->first is the (potentially moved-from) key
        return list_it->second;
    }

    iterator find(const Key& key) {
        auto map_it = map_.find(key);
        if (map_it == map_.end()) {
            return end();
        }
        return iterator(map_it->second);
    }

    const_iterator find(const Key& key) const {
        auto map_it = map_.find(key);
        if (map_it == map_.end()) {
            return cend();
        }
        // Explicitly cast underlying list iterator to const_iterator to resolve ambiguity
        return const_iterator(static_cast<typename node_list_type::const_iterator>(map_it->second));
    }

    bool contains(const Key& key) const {
        return map_.count(key) > 0;
    }

    // Special operations
    void to_front(const Key& key) {
        auto map_it = map_.find(key);
        if (map_it == map_.end() || map_it->second == list_.begin()) {
            return; // Key not found or already at front
        }
        // Splice the node to the beginning of the list
        list_.splice(list_.begin(), list_, map_it->second);
        // map_it->second remains valid and now points to the new front.
        // No need to update map_it->second itself as it's an iterator into list_.
    }

    void to_back(const Key& key) {
        auto map_it = map_.find(key);
        if (map_it == map_.end() || map_it->second == std::prev(list_.end())) {
            return; // Key not found or already at back
        }
        list_.splice(list_.end(), list_, map_it->second);
        // map_it->second remains valid and now points to the new end.
    }

    std::optional<value_type> pop_front() {
        if (list_.empty()) {
            return std::nullopt;
        }
        value_type val = std::move(list_.front()); // Move out the value
        map_.erase(val.first);
        list_.pop_front();
        return val;
    }

    std::optional<value_type> pop_back() {
        if (list_.empty()) {
            return std::nullopt;
        }
        value_type val = std::move(list_.back()); // Move out the value
        map_.erase(val.first);
        list_.pop_back();
        return val;
    }

    // Observers
    hasher hash_function() const { return map_.hash_function(); }
    key_equal key_eq() const { return map_.key_eq(); }
    allocator_type get_allocator() const noexcept { return list_.get_allocator(); }


    // Comparison
    bool operator==(const InsertionOrderedMap& other) const {
        if (size() != other.size()) return false;
        // The primary check for equality in an insertion-ordered map is that
        // the sequence of elements is identical. std::list::operator== handles this.
        // Comparing the unordered_maps directly is problematic because their value_type
        // is an iterator into their respective lists, which will always differ for distinct map objects
        // even if they represent the same logical sequence of key-value pairs.
        return list_ == other.list_;
    }

    bool operator!=(const InsertionOrderedMap& other) const {
        return !(*this == other);
    }
};

// Non-member swap
template <typename K, typename V, typename H, typename E, typename A>
void swap(InsertionOrderedMap<K, V, H, E, A>& lhs, InsertionOrderedMap<K, V, H, E, A>& rhs) noexcept {
    lhs.swap(rhs);
}

} // namespace cpp_collections

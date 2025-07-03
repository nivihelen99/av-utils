#pragma once

#include <list>
#include <unordered_map>
#include <utility>      // For std::pair, std::move
#include <stdexcept>    // For std::out_of_range
#include <functional>   // For std::hash, std::equal_to
#include <memory>       // For std::allocator, std::allocator_traits
#include <iterator>     // For iterator tags
#include <algorithm>    // For std::min, std::equal in comparison operators

namespace std_ext {

template<
    typename Key,
    typename Value,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    typename Allocator = std::allocator<std::pair<const Key, Value>>
>
class DequeMap {
public:
    // Type aliases
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>;
    using allocator_type = Allocator;
    using reference = value_type&;
    using const_reference = const value_type&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

private:
    using list_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<value_type>;
    using list_type = std::list<value_type, list_allocator_type>;

public: // list_iterator types need to be public for DequeMapIterator
    using list_iterator = typename list_type::iterator;
    using const_list_iterator = typename list_type::const_iterator;
private:
    using map_value_type = list_iterator;
    using map_internal_pair_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<std::pair<const Key, map_value_type>>;
    using map_type = std::unordered_map<Key, map_value_type, Hash, KeyEqual, map_internal_pair_allocator_type>;

    list_type item_list_;
    map_type item_map_;

public:
    template<bool IsConst>
    class DequeMapIterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = DequeMap::value_type; // Refers to outer DequeMap
        using difference_type   = typename DequeMap::difference_type; // Corrected

        using internal_list_iter_type = std::conditional_t<IsConst,
                                                  typename DequeMap::const_list_iterator,
                                                  typename DequeMap::list_iterator>;

        using pointer           = std::conditional_t<IsConst,
                                                  typename DequeMap::const_list_iterator::pointer,
                                                  typename DequeMap::list_iterator::pointer>;
        using reference         = std::conditional_t<IsConst,
                                                  typename DequeMap::const_list_iterator::reference,
                                                  typename DequeMap::list_iterator::reference>;
    private:
        internal_list_iter_type current_list_iter_;
        friend class DequeMap<Key, Value, Hash, KeyEqual, Allocator>;
        DequeMapIterator(internal_list_iter_type it) : current_list_iter_(it) {}

    public:
        DequeMapIterator() = default;
        DequeMapIterator(const DequeMapIterator&) = default;
        DequeMapIterator& operator=(const DequeMapIterator&) = default;
        DequeMapIterator(DequeMapIterator&&) noexcept = default;
        DequeMapIterator& operator=(DequeMapIterator&&) noexcept = default;

        DequeMapIterator(const DequeMapIterator<false>& other) requires IsConst
            : current_list_iter_(other.current_list_iter_) {}

        reference operator*() const { return *current_list_iter_; }
        pointer operator->() const { return current_list_iter_.operator->(); } // Or &(*current_list_iter_)
        DequeMapIterator& operator++() { ++current_list_iter_; return *this; }
        DequeMapIterator operator++(int) { DequeMapIterator temp = *this; ++(*this); return temp; }
        DequeMapIterator& operator--() { --current_list_iter_; return *this; }
        DequeMapIterator operator--(int) { DequeMapIterator temp = *this; --(*this); return temp; }
        bool operator==(const DequeMapIterator& other) const { return current_list_iter_ == other.current_list_iter_; }
        bool operator!=(const DequeMapIterator& other) const { return !(*this == other); }

        internal_list_iter_type get_internal_list_iterator() const { return current_list_iter_; }
    };

    using iterator = DequeMapIterator<false>;
    using const_iterator = DequeMapIterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // Constructors & Destructor
    explicit DequeMap(const Allocator& alloc = Allocator())
        : item_list_(list_allocator_type(alloc)),
          item_map_(0, Hash(), KeyEqual(), map_internal_pair_allocator_type(alloc)) {}

    DequeMap(size_type bucket_count,
             const Hash& hash = Hash(),
             const KeyEqual& equal = KeyEqual(),
             const Allocator& alloc = Allocator())
        : item_list_(list_allocator_type(alloc)),
          item_map_(bucket_count, hash, equal, map_internal_pair_allocator_type(alloc)) {}

    DequeMap(size_type bucket_count, const Allocator& alloc)
        : DequeMap(bucket_count, Hash(), KeyEqual(), alloc) {}

    DequeMap(size_type bucket_count, const Hash& hash, const Allocator& alloc)
        : DequeMap(bucket_count, hash, KeyEqual(), alloc) {}

    DequeMap(std::initializer_list<value_type> init,
             size_type bucket_count = 0,
             const Hash& hash = Hash(),
             const KeyEqual& equal = KeyEqual(),
             const Allocator& alloc = Allocator())
        : item_list_(list_allocator_type(alloc)),
          item_map_(bucket_count == 0 ? init.size() : bucket_count, hash, equal, map_internal_pair_allocator_type(alloc)) {
        for (const auto& item : init) {
            emplace_back(item.first, item.second); // "first wins"
        }
    }

    DequeMap(std::initializer_list<value_type> init, const Allocator& alloc)
    : DequeMap(init.begin(), init.end(), init.size(), Hash(), KeyEqual(), alloc) {}

    template<typename InputIt>
    DequeMap(InputIt first, InputIt last,
             size_type bucket_count = 0,
             const Hash& hash_fn = Hash(),
             const KeyEqual& equal_fn = KeyEqual(),
             const Allocator& alloc = Allocator())
        : item_list_(list_allocator_type(alloc)),
          item_map_(bucket_count == 0 ? static_cast<size_type>(std::distance(first, last)) : bucket_count, hash_fn, equal_fn, map_internal_pair_allocator_type(alloc)) {
        if (bucket_count == 0 && item_map_.bucket_count() < size_type(std::distance(first, last))) {
            // Ensure map has enough initial capacity if distance was used for bucket_count directly
             item_map_.rehash(std::distance(first, last));
        }
        for (auto it = first; it != last; ++it) {
            emplace_back(it->first, it->second); // "first wins"
        }
    }

    ~DequeMap() = default;

    DequeMap(const DequeMap& other)
        : item_list_(other.item_list_, std::allocator_traits<list_allocator_type>::select_on_container_copy_construction(other.item_list_.get_allocator())),
          item_map_(other.item_map_.bucket_count(),
                    other.item_map_.hash_function(),
                    other.item_map_.key_eq(),
                    std::allocator_traits<map_internal_pair_allocator_type>::select_on_container_copy_construction(other.item_map_.get_allocator())) {
        item_map_.clear();
        item_map_.reserve(item_list_.size());
        for (auto list_it = item_list_.begin(); list_it != item_list_.end(); ++list_it) {
            item_map_.emplace_hint(item_map_.end(), list_it->first, list_it);
        }
    }

    DequeMap(const DequeMap& other, const Allocator& alloc)
        : item_list_(other.item_list_, list_allocator_type(alloc)),
          item_map_(other.item_map_.bucket_count(),
                    other.item_map_.hash_function(),
                    other.item_map_.key_eq(),
                    map_internal_pair_allocator_type(alloc)) {
        item_map_.clear();
        item_map_.reserve(item_list_.size());
        for (auto list_it = item_list_.begin(); list_it != item_list_.end(); ++list_it) {
            item_map_.emplace_hint(item_map_.end(), list_it->first, list_it);
        }
    }

    DequeMap(DequeMap&& other) noexcept
        : item_list_(std::move(other.item_list_)),
          item_map_(std::move(other.item_map_)) {}

    DequeMap(DequeMap&& other, const Allocator& alloc)
        noexcept(std::allocator_traits<list_allocator_type>::is_always_equal::value &&
                 std::allocator_traits<map_internal_pair_allocator_type>::is_always_equal::value)
        : item_list_(std::move(other.item_list_), list_allocator_type(alloc)),
          item_map_(map_internal_pair_allocator_type(alloc)) {
        if (list_allocator_type(alloc) == other.item_list_.get_allocator() &&
            map_internal_pair_allocator_type(alloc) == other.item_map_.get_allocator()) {
            item_map_ = std::move(other.item_map_);
        } else {
            item_map_.rehash(other.item_map_.bucket_count());
            item_map_.reserve(item_list_.size());
            for (auto list_it = item_list_.begin(); list_it != item_list_.end(); ++list_it) {
                item_map_.emplace_hint(item_map_.end(), list_it->first, list_it);
            }
        }
    }

    DequeMap& operator=(const DequeMap& other) {
        if (this == &other) return *this;

        list_allocator_type new_list_alloc = item_list_.get_allocator();
        if (std::allocator_traits<list_allocator_type>::propagate_on_container_copy_assignment::value) {
            if (new_list_alloc != other.item_list_.get_allocator()) {
                new_list_alloc = other.item_list_.get_allocator();
            }
        }
        map_internal_pair_allocator_type new_map_alloc = item_map_.get_allocator();
        if (std::allocator_traits<map_internal_pair_allocator_type>::propagate_on_container_copy_assignment::value) {
             if (new_map_alloc != other.item_map_.get_allocator()) {
                new_map_alloc = other.item_map_.get_allocator();
            }
        }

        list_type temp_list(other.item_list_, new_list_alloc);
        map_type temp_map(other.item_map_.bucket_count(), other.item_map_.hash_function(), other.item_map_.key_eq(), new_map_alloc);
        temp_map.reserve(temp_list.size());
        for (auto list_it = temp_list.begin(); list_it != temp_list.end(); ++list_it) {
            temp_map.emplace_hint(temp_map.end(), list_it->first, list_it);
        }

        item_list_.swap(temp_list);
        item_map_.swap(temp_map);
        return *this;
    }

    DequeMap& operator=(DequeMap&& other) noexcept (
        (std::allocator_traits<list_allocator_type>::propagate_on_container_move_assignment::value ||
         std::allocator_traits<list_allocator_type>::is_always_equal::value) &&
        (std::allocator_traits<map_internal_pair_allocator_type>::propagate_on_container_move_assignment::value ||
         std::allocator_traits<map_internal_pair_allocator_type>::is_always_equal::value)
    ) {
        if (this == &other) return *this;

        constexpr bool pocma_list = std::allocator_traits<list_allocator_type>::propagate_on_container_move_assignment::value;
        constexpr bool list_always_equal = std::allocator_traits<list_allocator_type>::is_always_equal::value;
        constexpr bool pocma_map = std::allocator_traits<map_internal_pair_allocator_type>::propagate_on_container_move_assignment::value;
        constexpr bool map_always_equal = std::allocator_traits<map_internal_pair_allocator_type>::is_always_equal::value;

        const list_allocator_type current_list_alloc = item_list_.get_allocator();
        const map_internal_pair_allocator_type current_map_alloc = item_map_.get_allocator();
        const list_allocator_type other_list_alloc = other.item_list_.get_allocator();
        const map_internal_pair_allocator_type other_map_alloc = other.item_map_.get_allocator();

        if ((pocma_list && current_list_alloc != other_list_alloc) || (pocma_map && current_map_alloc != other_map_alloc)) {
            // Allocators will change, need to reconstruct with other's allocators
            clear();
            if (pocma_list) item_list_ = list_type(other_list_alloc);
            if (pocma_map) item_map_ = map_type(other_map_alloc);
            // Then move elements (done below if allocators are equal after propagation, or by element-wise move)
        }


        if ((pocma_list || list_always_equal || current_list_alloc == other_list_alloc) &&
            (pocma_map || map_always_equal || current_map_alloc == other_map_alloc)) {
            // Optimized path: allocators allow direct move or are same
            item_list_ = std::move(other.item_list_);
            item_map_ = std::move(other.item_map_);
        } else {
            // Element-wise move: copy keys, move values
            item_list_.clear();
            for (auto& item_in_other : other.item_list_) {
                item_list_.emplace_back(item_in_other.first, std::move(item_in_other.second));
            }
            item_map_.clear();
            item_map_.rehash(item_list_.size());
            for (auto list_it = item_list_.begin(); list_it != item_list_.end(); ++list_it) {
                item_map_.emplace_hint(item_map_.end(), list_it->first, list_it);
            }
            other.clear();
        }
        return *this;
    }

    void swap(DequeMap& other) noexcept (
        std::allocator_traits<list_allocator_type>::propagate_on_container_swap::value &&
        std::allocator_traits<map_internal_pair_allocator_type>::propagate_on_container_swap::value
    ) {
        using std::swap;
        item_list_.swap(other.item_list_);
        item_map_.swap(other.item_map_);
    }

    allocator_type get_allocator() const noexcept {
        return allocator_type(item_list_.get_allocator());
    }

    iterator begin() noexcept { return iterator(item_list_.begin()); }
    const_iterator begin() const noexcept { return const_iterator(item_list_.cbegin()); }
    const_iterator cbegin() const noexcept { return const_iterator(item_list_.cbegin()); }
    iterator end() noexcept { return iterator(item_list_.end()); }
    const_iterator end() const noexcept { return const_iterator(item_list_.cend()); }
    const_iterator cend() const noexcept { return const_iterator(item_list_.cend()); }

    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(cend()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(cbegin()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

    bool empty() const noexcept { return item_list_.empty(); }
    size_type size() const noexcept { return item_list_.size(); }

    std::pair<iterator, bool> push_front(const key_type& key, const mapped_type& value) {
        return emplace_front(key, value);
    }
    std::pair<iterator, bool> push_front(const key_type& key, mapped_type&& value) {
        return emplace_front(key, std::move(value));
    }
    std::pair<iterator, bool> push_front(key_type&& key, const mapped_type& value) {
        return emplace_front(std::move(key), value);
    }
    std::pair<iterator, bool> push_front(key_type&& key, mapped_type&& value) {
        return emplace_front(std::move(key), std::move(value));
    }

    template<typename... Args>
    std::pair<iterator, bool> emplace_front(const key_type& key, Args&&... args) {
        auto map_it = item_map_.find(key);
        if (map_it == item_map_.end()) {
            item_list_.emplace_front(std::piecewise_construct,
                                    std::forward_as_tuple(key),
                                    std::forward_as_tuple(std::forward<Args>(args)...));
            return std::make_pair(iterator(item_map_.emplace(key, item_list_.begin()).first->second), true);
        }
        return std::make_pair(iterator(map_it->second), false);
    }

    template<typename... Args>
    std::pair<iterator, bool> emplace_front(key_type&& key, Args&&... args) {
        auto map_it = item_map_.find(key);
        if (map_it == item_map_.end()) {
            item_list_.emplace_front(std::piecewise_construct,
                                    std::forward_as_tuple(std::move(key)),
                                    std::forward_as_tuple(std::forward<Args>(args)...));
            return std::make_pair(iterator(item_map_.emplace(item_list_.begin()->first, item_list_.begin()).first->second), true);
        }
        return std::make_pair(iterator(map_it->second), false);
    }

    std::pair<iterator, bool> push_back(const key_type& key, const mapped_type& value) {
        return emplace_back(key, value);
    }
    std::pair<iterator, bool> push_back(const key_type& key, mapped_type&& value) {
        return emplace_back(key, std::move(value));
    }
    std::pair<iterator, bool> push_back(key_type&& key, const mapped_type& value) {
        return emplace_back(std::move(key), value);
    }
     std::pair<iterator, bool> push_back(key_type&& key, mapped_type&& value) {
        return emplace_back(std::move(key), std::move(value));
    }

    template<typename... Args>
    std::pair<iterator, bool> emplace_back(const key_type& key, Args&&... args) {
        auto map_it = item_map_.find(key);
        if (map_it == item_map_.end()) {
            item_list_.emplace_back(std::piecewise_construct,
                                   std::forward_as_tuple(key),
                                   std::forward_as_tuple(std::forward<Args>(args)...));
            return std::make_pair(iterator(item_map_.emplace(key, std::prev(item_list_.end())).first->second), true);
        }
        return std::make_pair(iterator(map_it->second), false);
    }

    template<typename... Args>
    std::pair<iterator, bool> emplace_back(key_type&& key, Args&&... args) {
        auto map_it = item_map_.find(key);
        if (map_it == item_map_.end()) {
            item_list_.emplace_back(std::piecewise_construct,
                                   std::forward_as_tuple(std::move(key)),
                                   std::forward_as_tuple(std::forward<Args>(args)...));
            return std::make_pair(iterator(item_map_.emplace(std::prev(item_list_.end())->first, std::prev(item_list_.end())).first->second), true);
        }
        return std::make_pair(iterator(map_it->second), false);
    }

    value_type pop_front() {
        if (empty()) throw std::out_of_range("DequeMap::pop_front: map is empty");
        value_type popped_value = std::move(item_list_.front());
        item_map_.erase(popped_value.first);
        item_list_.pop_front();
        return popped_value;
    }

    value_type pop_back() {
        if (empty()) throw std::out_of_range("DequeMap::pop_back: map is empty");
        value_type popped_value = std::move(item_list_.back());
        item_map_.erase(popped_value.first);
        item_list_.pop_back();
        return popped_value;
    }

    reference front() {
        if (empty()) throw std::out_of_range("DequeMap::front: map is empty");
        return item_list_.front();
    }
    const_reference front() const {
        if (empty()) throw std::out_of_range("DequeMap::front: map is empty");
        return item_list_.front();
    }
    reference back() {
        if (empty()) throw std::out_of_range("DequeMap::back: map is empty");
        return item_list_.back();
    }
    const_reference back() const {
        if (empty()) throw std::out_of_range("DequeMap::back: map is empty");
        return item_list_.back();
    }

    mapped_type& operator[](const key_type& key) {
        auto map_it = item_map_.find(key);
        if (map_it == item_map_.end()) {
            return emplace_back(key, mapped_type{}).first->second;
        }
        return map_it->second->second;
    }

    mapped_type& operator[](key_type&& key) {
        auto map_it = item_map_.find(key);
        if (map_it == item_map_.end()) {
            return emplace_back(std::move(key), mapped_type{}).first->second;
        }
        return map_it->second->second;
    }

    mapped_type& at(const key_type& key) {
        auto map_it = item_map_.find(key);
        if (map_it == item_map_.end()) throw std::out_of_range("DequeMap::at: key not found");
        return map_it->second->second;
    }
    const mapped_type& at(const key_type& key) const {
        auto map_it = item_map_.find(key);
        if (map_it == item_map_.end()) throw std::out_of_range("DequeMap::at: key not found");
        return map_it->second->second;
    }

    std::pair<iterator, bool> insert(const value_type& value) {
        return emplace_back(value.first, value.second);
    }
    std::pair<iterator, bool> insert(value_type&& value) {
        return emplace_back(std::move(value.first), std::move(value.second));
    }
    iterator insert(const_iterator hint, const value_type& value) {
        auto map_it = item_map_.find(value.first);
        if (map_it == item_map_.end()) {
            item_list_.push_back(value); // push_back for const value_type&
            return iterator(item_map_.emplace_hint(map_it_to_map_iterator(hint), value.first, std::prev(item_list_.end()))->second);
        }
        return iterator(map_it->second);
    }
    iterator insert(const_iterator hint, value_type&& value) {
        auto map_it = item_map_.find(value.first);
        if (map_it == item_map_.end()) {
            item_list_.push_back(std::move(value)); // push_back for value_type&&
            return iterator(item_map_.emplace_hint(map_it_to_map_iterator(hint), item_list_.back().first, std::prev(item_list_.end()))->second);
        }
        return iterator(map_it->second);
    }

    template<typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        // This emplace is tricky. For now, assume it constructs value_type and calls emplace_back.
        // A more robust emplace would try to construct key first or use piecewise.
        value_type temp_val(std::forward<Args>(args)...);
        return emplace_back(std::move(temp_val.first), std::move(temp_val.second));
    }

    template <class... Args>
    std::pair<iterator,bool> try_emplace(const key_type& k, Args&&... args) {
        return emplace_back(k, std::forward<Args>(args)...);
    }
    template <class... Args>
    std::pair<iterator,bool> try_emplace(key_type&& k, Args&&... args) {
        return emplace_back(std::move(k), std::forward<Args>(args)...);
    }
    template <class... Args>
    iterator try_emplace(const_iterator hint, const key_type& k, Args&&... args) {
        auto map_it = item_map_.find(k);
        if (map_it == item_map_.end()) {
             item_list_.emplace_back(std::piecewise_construct,
                                   std::forward_as_tuple(k),
                                   std::forward_as_tuple(std::forward<Args>(args)...));
            return iterator(item_map_.emplace_hint(map_it_to_map_iterator(hint), k, std::prev(item_list_.end()))->second);
        }
        return iterator(map_it->second);
    }
    template <class... Args>
    iterator try_emplace(const_iterator hint, key_type&& k, Args&&... args) {
        auto map_it = item_map_.find(k);
        if (map_it == item_map_.end()) {
            item_list_.emplace_back(std::piecewise_construct,
                                   std::forward_as_tuple(std::move(k)),
                                   std::forward_as_tuple(std::forward<Args>(args)...));
            return iterator(item_map_.emplace_hint(map_it_to_map_iterator(hint), std::prev(item_list_.end())->first, std::prev(item_list_.end()))->second);
        }
        return iterator(map_it->second);
    }

    size_type erase(const key_type& key) {
        auto map_it = item_map_.find(key);
        if (map_it != item_map_.end()) {
            item_list_.erase(map_it->second);
            item_map_.erase(map_it);
            return 1;
        }
        return 0;
    }

    iterator erase(iterator pos) {
        if (pos == end()) return end();
        const key_type& key = pos->first;
        list_iterator next_list_iter = item_list_.erase(pos.get_internal_list_iterator());
        item_map_.erase(key);
        return iterator(next_list_iter);
    }

    iterator erase(const_iterator pos) {
        if (pos == cend()) return end();
        const key_type& key_to_erase = pos->first;
        // std::list::erase takes const_iterator and returns non-const iterator
        list_iterator next_list_iter = item_list_.erase(pos.get_internal_list_iterator());
        item_map_.erase(key_to_erase);
        return iterator(next_list_iter);
    }

    iterator find(const key_type& key) {
        auto map_it = item_map_.find(key);
        return (map_it == item_map_.end()) ? end() : iterator(map_it->second);
    }
    const_iterator find(const key_type& key) const {
        auto map_it = item_map_.find(key);
        if (map_it == item_map_.end()) {
            return cend();
        }
        typename DequeMap::const_list_iterator c_list_it = map_it->second;
        return const_iterator(c_list_it);
    }

    bool contains(const key_type& key) const {
        return item_map_.count(key) > 0;
    }

    void clear() noexcept {
        item_list_.clear();
        item_map_.clear();
    }

    size_type max_size() const noexcept {
        return std::min(item_list_.max_size(), item_map_.max_size());
    }

private:
    // Helper to convert DequeMap const_iterator to map_type::iterator (for hints)
    // This is a bit tricky as map hint needs non-const iterator.
    // For simplicity, if hint is cend(), return map's end(). Otherwise, try to find.
    // This might not be the most efficient hint conversion.
    typename map_type::iterator map_it_to_map_iterator(const_iterator hint_iter) {
        if (hint_iter == cend()) {
            return item_map_.end();
        }
        // This finds the element, not necessarily the "hint position" for map.
        // A true hint would be an iterator to the map itself.
        // For now, this is a placeholder or simplified approach.
        return item_map_.find(hint_iter->first);
    }

}; // class DequeMap

template<typename K, typename V, typename H, typename KE, typename A>
bool operator==(const DequeMap<K, V, H, KE, A>& lhs, const DequeMap<K, V, H, KE, A>& rhs) {
    if (lhs.size() != rhs.size()) return false;
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template<typename K, typename V, typename H, typename KE, typename A>
bool operator!=(const DequeMap<K, V, H, KE, A>& lhs, const DequeMap<K, V, H, KE, A>& rhs) {
    return !(lhs == rhs);
}

template<typename K, typename V, typename H, typename KE, typename A>
void swap(DequeMap<K, V, H, KE, A>& lhs, DequeMap<K, V, H, KE, A>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace std_ext

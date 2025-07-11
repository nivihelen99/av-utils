#pragma once

#include <vector>
#include <functional>
#include <iterator> // For std::iterator_traits
#include <type_traits> // For std::invoke_result_t

namespace cpp_collections {

// Helper struct to represent a group
template <typename Key, typename Value>
struct Group {
    Key key;
    std::vector<Value> items;

    // Default constructor
    Group() = default;

    // Constructor
    Group(Key k, std::vector<Value> i) : key(std::move(k)), items(std::move(i)) {}

    // Equality operator for easy testing
    bool operator==(const Group<Key, Value>& other) const {
        return key == other.key && items == other.items;
    }
};

/**
 * @brief Groups consecutive elements in a range that share the same key.
 *
 * This function iterates through the provided range [first, last) and groups
 * consecutive elements for which the getKey function returns the same key.
 *
 * @tparam InputIt Type of the input iterator. Must meet the requirements of ForwardIterator.
 * @tparam GetKey Unary function object that takes an element from the input range
 *                and returns its key. The key type must be comparable with operator==.
 * @tparam ValueType The type of the elements in the input range. Deduced from InputIt.
 * @tparam KeyType The type of the key returned by getKey. Deduced from GetKey.
 *
 * @param first The beginning of the range to group.
 * @param last The end of the range to group.
 * @param getKey A function object that extracts a key from an element.
 *
 * @return A std::vector of Group<KeyType, ValueType> objects. Each Group object
 *         contains the common key and a std::vector of items belonging to that group.
 *         Returns an empty vector if the input range is empty.
 */
template <typename InputIt, typename GetKey>
auto group_by_consecutive(InputIt first, InputIt last, GetKey getKey)
    -> std::vector<Group<
           typename std::decay_t<std::invoke_result_t<GetKey, typename std::iterator_traits<InputIt>::value_type>>,
           typename std::iterator_traits<InputIt>::value_type>> {

    using ValueType = typename std::iterator_traits<InputIt>::value_type;
    using DeducedKeyType = std::invoke_result_t<GetKey, ValueType>;
    using KeyType = typename std::decay_t<DeducedKeyType>; // Store key by value
    using ResultGroupType = Group<KeyType, ValueType>;
    std::vector<ResultGroupType> result;

    if (first == last) {
        return result;
    }

    // For the first element, we initialize the first group
    KeyType current_key = getKey(*first);
    std::vector<ValueType> current_items;
    current_items.push_back(*first);

    InputIt it = std::next(first);
    while (it != last) {
        KeyType next_key = getKey(*it);
        if (next_key == current_key) {
            current_items.push_back(*it);
        } else {
            // Key changed, finalize the current group and start a new one
            result.emplace_back(std::move(current_key), std::move(current_items));

            // Reset for the new group
            current_key = std::move(next_key); // Use the already computed next_key
            // current_items is already empty due to std::move, but clear() is good for explicit safety if not moved
            current_items.clear();
            current_items.push_back(*it);
        }
        ++it;
    }

    // Add the last processed group
    // This check ensures that if the input range had elements, the last group is always added.
    if (!current_items.empty()) {
        result.emplace_back(std::move(current_key), std::move(current_items));
    }

    return result;
}

// Overload for a binary predicate
/**
 * @brief Groups consecutive elements in a range based on a binary predicate.
 *
 * This function iterates through the provided range [first, last) and groups
 * consecutive elements. A new group is started when the 'are_in_same_group'
 * predicate returns false for the current element and the previous element.
 * The "key" of the group will be the first element of that group.
 *
 * @tparam InputIt Type of the input iterator. Must meet the requirements of ForwardIterator.
 * @tparam AreInSameGroup Binary predicate that takes two elements (previous element, current element)
 *                        and returns true if they belong to the same group.
 * @tparam ValueType The type of the elements in the input range. Deduced from InputIt.
 *
 * @param first The beginning of the range to group.
 * @param last The end of the range to group.
 * @param are_in_same_group A binary predicate to determine if the current element
 *                          should be grouped with the previous one.
 *
 * @return A std::vector of Group<ValueType, ValueType> objects. Each Group object
 *         contains the first element of the group as its key, and a std::vector
 *         of items belonging to that group.
 *         Returns an empty vector if the input range is empty.
 */
template <typename InputIt, typename AreInSameGroup>
auto group_by_consecutive_pred(InputIt first, InputIt last, AreInSameGroup are_in_same_group)
    -> std::vector<Group<
           typename std::iterator_traits<InputIt>::value_type,
           typename std::iterator_traits<InputIt>::value_type>> {

    using ValueType = typename std::iterator_traits<InputIt>::value_type;
    using ResultGroupType = Group<ValueType, ValueType>;
    std::vector<ResultGroupType> result;

    if (first == last) {
        return result;
    }

    // The first element always starts a new group. Its value is used as the key for this group.
    ValueType current_group_key_representative = *first;
    std::vector<ValueType> current_items;
    current_items.push_back(*first);

    InputIt prev_it = first; // Keep track of the previous element for the predicate
    InputIt it = std::next(first);

    while (it != last) {
        // The predicate checks if the current element (*it) belongs to the same group as the previous one (*prev_it)
        if (are_in_same_group(*prev_it, *it)) {
            current_items.push_back(*it);
        } else {
            // Predicate returned false, finalize the current group
            result.emplace_back(std::move(current_group_key_representative), std::move(current_items));

            // Start a new group with the current element
            current_group_key_representative = *it; // New group's key is the current element
            // current_items is already empty due to std::move, but clear() is good for explicit safety
            current_items.clear();
            current_items.push_back(*it);
        }
        prev_it = it;
        ++it;
    }

    // Add the last processed group
    if (!current_items.empty()) {
        result.emplace_back(std::move(current_group_key_representative), std::move(current_items));
    }

    return result;
}

} // namespace cpp_collections

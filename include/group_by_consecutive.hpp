#ifndef GROUP_BY_CONSECUTIVE_HPP
#define GROUP_BY_CONSECUTIVE_HPP

#include <vector>
#include <utility> // For std::pair
#include <type_traits> // For std::decay_t
#include <iterator> // For std::iterator_traits

namespace utils {

// Iterator-based version
template<typename Iterator, typename KeyFunc>
auto group_by_consecutive(Iterator begin, Iterator end, KeyFunc key_func)
    -> std::vector<std::pair<
           typename std::decay_t<decltype(key_func(*begin))>,
           std::vector<typename std::iterator_traits<Iterator>::value_type>
       >>
{
    using KeyType = typename std::decay_t<decltype(key_func(*begin))>;
    using ValueType = typename std::iterator_traits<Iterator>::value_type;
    using GroupType = std::vector<ValueType>;
    using ResultType = std::vector<std::pair<KeyType, GroupType>>;

    ResultType result;
    if (begin == end) {
        return result;
    }

    KeyType current_key = key_func(*begin);
    GroupType current_group;
    current_group.push_back(*begin);

    for (Iterator it = std::next(begin); it != end; ++it) {
        KeyType key = key_func(*it);
        if (key == current_key) {
            current_group.push_back(*it);
        } else {
            result.emplace_back(std::move(current_key), std::move(current_group));
            current_key = std::move(key); // Use std::move for key if it's a movable type
            current_group.clear();
            current_group.push_back(*it);
        }
    }
    // Add the last group
    result.emplace_back(std::move(current_key), std::move(current_group));

    return result;
}

// Container-based wrapper version
template<typename Container, typename KeyFunc>
auto group_by_consecutive(const Container& container, KeyFunc key_func)
    -> decltype(group_by_consecutive(container.begin(), container.end(), key_func))
{
    return group_by_consecutive(container.begin(), container.end(), key_func);
}

} // namespace utils

#endif // GROUP_BY_CONSECUTIVE_HPP

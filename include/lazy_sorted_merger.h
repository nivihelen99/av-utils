#ifndef LAZY_SORTED_MERGER_H
#define LAZY_SORTED_MERGER_H

#include <vector>
#include <utility> // For std::pair
#include <queue>   // For std::priority_queue
#include <functional> // For std::function and comparators
#include <optional>   // For std::optional

template <typename Iterator, typename Value = typename std::iterator_traits<Iterator>::value_type, typename Compare = std::less<Value>>
class LazySortedMerger {
public:
    using IteratorPair = std::pair<Iterator, Iterator>;
    using ValueType = Value; // Alias for the value type being merged

    bool hasNext() const {
        return !очередь_с_приоритетом_.empty();
    }

    std::optional<Value> next() {
        if (!hasNext()) {
            return std::nullopt;
        }

        // Get the element with the highest priority (smallest according to Compare)
        ElementPq top_element = очередь_с_приоритетом_.top();
        очередь_с_приоритетом_.pop();

        Value result = *top_element.iter;
        size_t source_idx = top_element.source_index;

        // Advance the iterator for the source from which the element was taken
        источников_[source_idx].first++;

        // If the advanced iterator is not at the end of its sequence,
        // push its new head onto the priority queue
        if (источников_[source_idx].first != источников_[source_idx].second) {
            очередь_с_приоритетом_.push({источников_[source_idx].first, source_idx});
        }

        return result;
    }

private:
    // Stores an iterator and its original source index
    struct ElementPq {
        Iterator iter;
        size_t source_index; // To know which source this iterator belongs to
    };

    // Custom comparator for the priority queue
    // It takes two ElementPq objects and compares them based on the values they point to,
    // using the user-provided comparison function `comp_func`.
    // std::priority_queue is a max-heap by default. It pops the 'largest' element.
    // To make it behave as a min-heap (pop the 'smallest' element according to `comp_func`),
    // this comparator should return true if `a` is "greater" than `b` (i.e., `comp_func(*b.iter, *a.iter)` is true).
    struct PqCompare {
        Compare comp_func; // User-provided comparison function (e.g., std::less for ascending order)
        PqCompare(Compare c) : comp_func(c) {}
        bool operator()(const ElementPq& a, const ElementPq& b) const {
            // if comp_func(*a.iter, *b.iter) is true, 'a' is smaller than 'b'.
            // For a min-heap, 'a' should have higher priority.
            // The priority_queue's comparator returns true if left operand has strictly lower priority
            // than the right operand. So, if 'a' is smaller (higher priority), we want this to return false.
            // If 'b' is smaller than 'a' (comp_func(*b.iter, *a.iter) is true), then 'a' has lower priority than 'b',
            // so we return true.
            return comp_func(*b.iter, *a.iter);
        }
    };

    std::vector<IteratorPair> источников_; // Stores {current, end} iterators for each source
    std::priority_queue<ElementPq, std::vector<ElementPq>, PqCompare> очередь_с_приоритетом_;

public:
    LazySortedMerger(const std::vector<IteratorPair>& sources, Compare comp = Compare())
        : источников_(sources),
          очередь_с_приоритетом_(PqCompare(comp)) // Initialize pq with the custom comparator
    {
        for (size_t i = 0; i < источников_.size(); ++i) {
            // The member `источников_` is a copy of the input `sources`.
            // We can modify `источников_[i].first` as it's our internal copy.
            if (источников_[i].first != источников_[i].second) { // Check if the source is not empty
                очередь_с_приоритетом_.push({источников_[i].first, i});
            }
        }
    }
};

// Functional interface to create a LazySortedMerger instance
template <typename Iterator,
          typename Value = typename std::iterator_traits<Iterator>::value_type,
          typename Compare = std::less<Value>>
LazySortedMerger<Iterator, Value, Compare>
lazy_merge(const std::vector<std::pair<Iterator, Iterator>>& sources, Compare comp = Compare()) {
    return LazySortedMerger<Iterator, Value, Compare>(sources, comp);
}

#endif // LAZY_SORTED_MERGER_H

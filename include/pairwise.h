#pragma once

#include <iterator>
#include <utility>
#include <type_traits>
#include <stdexcept> // For std::logic_error

namespace std_ext {

template<typename Iterator>
class pairwise_iterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::pair<typename std::iterator_traits<Iterator>::value_type,
                                 typename std::iterator_traits<Iterator>::value_type>;
    using difference_type = typename std::iterator_traits<Iterator>::difference_type;
    using pointer = value_type*;
    using reference = value_type;

private:
    Iterator current_;
    Iterator next_;
    Iterator end_;
    bool valid_;

    void advance_to_valid() {
        if (current_ != end_ && next_ != end_) {
            valid_ = true;
        } else {
            valid_ = false;
        }
    }

public:
    pairwise_iterator() : valid_(false) {}

    pairwise_iterator(Iterator begin, Iterator end)
        : current_(begin), end_(end), valid_(false) {
        if (begin != end) {
            next_ = begin;
            ++next_;
        }
        advance_to_valid();
    }

    reference operator*() const {
        if (!valid_) {
            throw std::logic_error("Dereferencing invalid pairwise_iterator");
        }
        return value_type(*current_, *next_);
    }

    pairwise_iterator& operator++() {
        if (valid_) {
            ++current_;
            if (next_ != end_) { // Prevent advancing next_ past end if current_ is now end_
                 ++next_;
            }
            advance_to_valid();
        } else {
            current_ = end_;
            next_ = end_;
            valid_ = false;
        }
        return *this;
    }

    pairwise_iterator operator++(int) {
        pairwise_iterator temp = *this;
        ++(*this);
        return temp;
    }

    friend bool operator==(const pairwise_iterator& a, const pairwise_iterator& b) {
        if (!a.valid_ && !b.valid_) return true;
        if (a.valid_ != b.valid_) return false;
        return a.current_ == b.current_;
    }

    friend bool operator!=(const pairwise_iterator& a, const pairwise_iterator& b) {
        return !(a == b);
    }
};

template<typename Iter>
class PairwiseIterView {
public:
    using iterator = pairwise_iterator<Iter>;
    using value_type = typename iterator::value_type;

private:
    Iter begin_iter_;
    Iter end_iter_;

public:
    PairwiseIterView(Iter begin_iter, Iter end_iter)
        : begin_iter_(begin_iter), end_iter_(end_iter) {}

    iterator begin() const {
        return iterator(begin_iter_, end_iter_);
    }
    iterator end() const {
        return iterator();
    }

    bool empty() const {
        if (begin_iter_ == end_iter_) return true;
        Iter temp_it = begin_iter_;
        // Check if there is at least one element after begin_iter_
        // to form a pair.
        if (temp_it != end_iter_) { // Ensure begin_iter_ is not already end
            ++temp_it;
            return temp_it == end_iter_; // True if only one element (so no pairs)
        }
        return true; // If begin_iter_ was already end_iter_
    }
};

template<typename Iterable>
auto pairwise(Iterable&& iterable) -> PairwiseIterView<decltype(std::begin(std::forward<Iterable>(iterable)))> {
    // WARNING: Lifetime management is crucial here.
    // If `iterable` is an rvalue temporary, the iterators stored in `PairwiseIterView`
    // will dangle after this function returns and the temporary is destroyed.
    return PairwiseIterView(std::begin(std::forward<Iterable>(iterable)), std::end(std::forward<Iterable>(iterable)));
}

template<typename T, std::size_t N>
auto pairwise(T (&array)[N]) -> PairwiseIterView<T*> {
    return PairwiseIterView(array, array + N);
}

} // namespace std_ext

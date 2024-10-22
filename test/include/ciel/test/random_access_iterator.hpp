#ifndef CIELLAB_INCLUDE_CIEL_RANDOM_ACCESS_ITERATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_RANDOM_ACCESS_ITERATOR_HPP_

#include <ciel/config.hpp>
#include <ciel/iterator_base.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>

// Simulate random_access_iterator using T array base.

template<class T>
class RandomAccessIterator : public ciel::random_access_iterator_base<RandomAccessIterator<T>> {
public:
    using difference_type   = ptrdiff_t;
    using value_type        = T;
    using pointer           = T*;
    using reference         = T&;
    using iterator_category = std::random_access_iterator_tag;
    using iterator_concept  = std::random_access_iterator_tag;

private:
    pointer ptr;

public:
    RandomAccessIterator(const pointer p) noexcept
        : ptr(p) {}

    void
    go_next() noexcept {
        CIEL_PRECONDITION(ptr != nullptr);
        ++ptr;
    }

    void
    go_prev() noexcept {
        CIEL_PRECONDITION(ptr != nullptr);
        --ptr;
    }

    void
    advance(difference_type n) noexcept {
        CIEL_PRECONDITION(ptr != nullptr);
        ptr += n;
    }

    reference
    operator*() const noexcept {
        CIEL_PRECONDITION(ptr != nullptr);
        return *ptr;
    }

    pointer
    operator->() const noexcept {
        CIEL_PRECONDITION(ptr != nullptr);
        return ptr;
    }

    pointer
    base() const noexcept {
        return ptr;
    }

    CIEL_NODISCARD friend bool
    operator==(const RandomAccessIterator& lhs, const RandomAccessIterator& rhs) noexcept {
        return lhs.base() == rhs.base();
    }

    CIEL_NODISCARD friend difference_type
    operator-(const RandomAccessIterator& lhs, const RandomAccessIterator& rhs) noexcept {
        return lhs.base() - rhs.base();
    }

}; // class RandomAccessIterator

#endif // CIELLAB_INCLUDE_CIEL_RANDOM_ACCESS_ITERATOR_HPP_

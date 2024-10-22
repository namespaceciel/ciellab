#ifndef CIELLAB_INCLUDE_CIEL_FORWARD_ITERATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_FORWARD_ITERATOR_HPP_

#include <ciel/config.hpp>
#include <ciel/iterator_base.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>

// Simulate forward_iterator using T array base.

template<class T>
class ForwardIterator : public ciel::input_iterator_base<ForwardIterator<T>> {
public:
    using difference_type   = ptrdiff_t;
    using value_type        = T;
    using pointer           = T*;
    using reference         = T&;
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept  = std::forward_iterator_tag;

private:
    pointer ptr;

public:
    ForwardIterator(const pointer p) noexcept
        : ptr(p) {}

    void
    go_next() noexcept {
        CIEL_PRECONDITION(ptr != nullptr);
        ++ptr;
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
    operator==(const ForwardIterator& lhs, const ForwardIterator& rhs) noexcept {
        return lhs.base() == rhs.base();
    }

}; // class ForwardIterator

#endif // CIELLAB_INCLUDE_CIEL_FORWARD_ITERATOR_HPP_

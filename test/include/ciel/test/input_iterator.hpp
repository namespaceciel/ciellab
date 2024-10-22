#ifndef CIELLAB_INCLUDE_CIEL_INPUT_ITERATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_INPUT_ITERATOR_HPP_

#include <ciel/config.hpp>
#include <ciel/iterator_base.hpp>

#include <cstddef>
#include <iterator>

// Simulate input_iterator using T array base.

template<class T>
class InputIterator : public ciel::input_iterator_base<InputIterator<T>> {
public:
    using difference_type   = ptrdiff_t;
    using value_type        = T;
    using pointer           = T*;
    using reference         = T&;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept  = std::input_iterator_tag;

private:
    pointer ptr;

public:
    InputIterator(pointer p) noexcept
        : ptr(p) {
        CIEL_PRECONDITION(p != nullptr);
    }

    void
    go_next() noexcept {
        CIEL_PRECONDITION(*ptr != -1);
        *ptr = -1;
        ++ptr;
    }

    reference
    operator*() const noexcept {
        CIEL_PRECONDITION(*ptr != -1);
        return *ptr;
    }

    pointer
    operator->() const noexcept {
        CIEL_PRECONDITION(*ptr != -1);
        return ptr;
    }

    pointer
    base() const noexcept {
        return ptr;
    }

    CIEL_NODISCARD friend bool
    operator==(const InputIterator& lhs, const InputIterator& rhs) noexcept {
        return lhs.base() == rhs.base();
    }

}; // class InputIterator

#endif // CIELLAB_INCLUDE_CIEL_INPUT_ITERATOR_HPP_

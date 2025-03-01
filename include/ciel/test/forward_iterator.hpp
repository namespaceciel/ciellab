#ifndef CIELLAB_INCLUDE_CIEL_TEST_FORWARD_ITERATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_TEST_FORWARD_ITERATOR_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/iterator_base.hpp>
#include <ciel/core/message.hpp>

#include <cstddef>
#include <iterator>

NAMESPACE_CIEL_BEGIN

// Simulate forward_iterator using T array base.

template<class T>
class ForwardIterator : public input_iterator_base<ForwardIterator<T>> {
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
    ForwardIterator() = default;

    ForwardIterator(const pointer p) noexcept
        : ptr(p) {}

    void go_next() noexcept {
        CIEL_ASSERT(ptr != nullptr);
        ++ptr;
    }

    CIEL_NODISCARD reference operator*() const noexcept {
        CIEL_ASSERT(ptr != nullptr);
        return *ptr;
    }

    CIEL_NODISCARD pointer operator->() const noexcept {
        CIEL_ASSERT(ptr != nullptr);
        return ptr;
    }

    CIEL_NODISCARD pointer base() const noexcept {
        return ptr;
    }

    CIEL_NODISCARD friend bool operator==(const ForwardIterator& lhs, const ForwardIterator& rhs) noexcept {
        return lhs.base() == rhs.base();
    }

    template<class U>
    void operator,(const U&) = delete;

}; // class ForwardIterator

#if CIEL_STD_VER >= 20
static_assert(std::forward_iterator<ForwardIterator<int>>);
#endif

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TEST_FORWARD_ITERATOR_HPP_

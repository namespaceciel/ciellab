#ifndef CIELLAB_INCLUDE_CIEL_TEST_RANDOM_ACCESS_ITERATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_TEST_RANDOM_ACCESS_ITERATOR_HPP_

#include <ciel/compare.hpp>
#include <ciel/core/config.hpp>
#include <ciel/core/iterator_base.hpp>
#include <ciel/core/message.hpp>

#include <cstddef>
#include <iterator>

NAMESPACE_CIEL_BEGIN

// Simulate random_access_iterator using T array base.

template<class T>
class RandomAccessIterator : public random_access_iterator_base<RandomAccessIterator<T>> {
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
    RandomAccessIterator() = default;

    RandomAccessIterator(const pointer p) noexcept
        : ptr(p) {}

    void go_next() noexcept {
        CIEL_ASSERT(ptr != nullptr);
        ++ptr;
    }

    void go_prev() noexcept {
        CIEL_ASSERT(ptr != nullptr);
        --ptr;
    }

    void advance(difference_type n) noexcept {
        CIEL_ASSERT(ptr != nullptr);
        ptr += n;
    }

    CIEL_NODISCARD reference operator*() const noexcept {
        CIEL_ASSERT(ptr != nullptr);
        return *ptr;
    }

    CIEL_NODISCARD pointer operator->() const noexcept {
        CIEL_ASSERT(ptr != nullptr);
        return ptr;
    }

    CIEL_NODISCARD reference operator[](difference_type n) const noexcept {
        CIEL_ASSERT(ptr != nullptr);
        return *(ptr + n);
    }

    CIEL_NODISCARD pointer base() const noexcept {
        return ptr;
    }

    CIEL_NODISCARD friend bool operator==(const RandomAccessIterator& lhs, const RandomAccessIterator& rhs) noexcept {
        return lhs.base() == rhs.base();
    }

    CIEL_NODISCARD friend bool operator<(const RandomAccessIterator& lhs, const RandomAccessIterator& rhs) noexcept {
        return lhs.base() < rhs.base();
    }

    CIEL_NODISCARD friend RandomAccessIterator operator+(RandomAccessIterator iter, difference_type n) noexcept {
        iter += n;
        return iter;
    }

    CIEL_NODISCARD friend RandomAccessIterator operator+(difference_type n, RandomAccessIterator iter) noexcept {
        iter += n;
        return iter;
    }

    CIEL_NODISCARD friend RandomAccessIterator operator-(RandomAccessIterator iter, difference_type n) noexcept {
        iter -= n;
        return iter;
    }

    CIEL_NODISCARD friend difference_type operator-(const RandomAccessIterator& lhs,
                                                    const RandomAccessIterator& rhs) noexcept {
        return lhs.base() - rhs.base();
    }

    template<class U>
    void operator,(const U&) = delete;

}; // class RandomAccessIterator

#if CIEL_STD_VER >= 20
static_assert(std::random_access_iterator<RandomAccessIterator<int>>);
#endif

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TEST_RANDOM_ACCESS_ITERATOR_HPP_

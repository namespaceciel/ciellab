#ifndef CIELLAB_INCLUDE_CIEL_TEST_INPUT_ITERATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_TEST_INPUT_ITERATOR_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/iterator_base.hpp>
#include <ciel/core/message.hpp>

#include <cstddef>
#include <iterator>

NAMESPACE_CIEL_BEGIN

// Simulate input_iterator using T array base.

template<class T>
class InputIterator : public input_iterator_base<InputIterator<T>> {
public:
    using difference_type   = ptrdiff_t;
    using value_type        = T;
    using pointer           = T*;
    using reference         = T&;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept  = std::input_iterator_tag;

private:
    pointer ptr;

    static value_type invalid() noexcept {
        return value_type{-1234};
    }

public:
    InputIterator() = default;

    InputIterator(const pointer p) noexcept
        : ptr(p) {}

    void go_next() noexcept {
        CIEL_ASSERT(ptr != nullptr);
        CIEL_ASSERT(*ptr != invalid());
        if (*ptr != -1) {
            *ptr = invalid();
        }
        ++ptr;
    }

    CIEL_NODISCARD reference operator*() const noexcept {
        CIEL_ASSERT(ptr != nullptr);
        CIEL_ASSERT(*ptr != invalid());
        return *ptr;
    }

    CIEL_NODISCARD pointer operator->() const noexcept {
        CIEL_ASSERT(ptr != nullptr);
        CIEL_ASSERT(*ptr != invalid());
        return ptr;
    }

    CIEL_NODISCARD pointer base() const noexcept {
        return ptr;
    }

    CIEL_NODISCARD friend bool operator==(const InputIterator& lhs, const InputIterator& rhs) noexcept {
        return lhs.base() == rhs.base();
    }

    template<class U>
    void operator,(const U&) = delete;

}; // class InputIterator

#if CIEL_STD_VER >= 20
static_assert(std::input_iterator<InputIterator<int>>);
#endif

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TEST_INPUT_ITERATOR_HPP_

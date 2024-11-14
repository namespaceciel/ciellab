#ifndef CIELLAB_INCLUDE_CIEL_TEST_RANGE_HPP_
#define CIELLAB_INCLUDE_CIEL_TEST_RANGE_HPP_

#include <ciel/core/config.hpp>

#include <cstddef>

NAMESPACE_CIEL_BEGIN

template<class Iter, bool HasSize>
class range {
    static_assert(!std::is_reference<Iter>::value, "");

private:
    Iter begin_;
    Iter end_;

public:
    range(Iter begin, Iter end) noexcept
        : begin_(begin), end_(end) {}

    range(const range&)            = default;
    range& operator=(const range&) = default;

    Iter begin() const noexcept {
        return begin_;
    }

    Iter end() const noexcept {
        return end_;
    }

}; // class range

template<class Iter>
class range<Iter, true> {
    static_assert(!std::is_reference<Iter>::value, "");

private:
    Iter begin_;
    Iter end_;
    size_t size_;

public:
    range(Iter begin, Iter end, size_t size) noexcept
        : begin_(begin), end_(end), size_(size) {}

    range(const range&)            = default;
    range& operator=(const range&) = default;

    Iter begin() const noexcept {
        return begin_;
    }

    Iter end() const noexcept {
        return end_;
    }

    size_t size() const noexcept {
        return size_;
    }

}; // class range<Iter, true>

template<class Iter>
range<Iter, false> make_range(Iter begin, Iter end) noexcept {
    return range<Iter, false>(begin, end);
}

template<class Iter>
range<Iter, true> make_range(Iter begin, Iter end, size_t size) noexcept {
    return range<Iter, true>(begin, end, size);
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TEST_RANGE_HPP_

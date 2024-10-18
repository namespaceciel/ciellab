#ifndef CIELLAB_INCLUDE_CIEL_ITERATOR_BASE_HPP_
#define CIELLAB_INCLUDE_CIEL_ITERATOR_BASE_HPP_

#include <ciel/config.hpp>

#include <cstddef>

NAMESPACE_CIEL_BEGIN

// iterator_base
template<class Derived>
struct input_iterator_base {
    Derived&
    operator++() noexcept {
        Derived& self = static_cast<Derived&>(*this);
        self.go_next();
        return self;
    }

    CIEL_NODISCARD Derived
    operator++(int) noexcept {
        Derived& self = static_cast<Derived&>(*this);
        Derived res(self);
        ++self;
        return res;
    }

}; // struct input_iterator_base

template<class Derived>
struct bidirectional_iterator_base : input_iterator_base<Derived> {
    Derived&
    operator--() noexcept {
        Derived& self = static_cast<Derived&>(*this);
        self.go_prev();
        return self;
    }

    CIEL_NODISCARD Derived
    operator--(int) noexcept {
        Derived& self = static_cast<Derived&>(*this);
        Derived res(self);
        --self;
        return res;
    }

}; // struct bidirectional_iterator_base

template<class Derived>
struct random_access_iterator_base : bidirectional_iterator_base<Derived> {
    using difference_type = ptrdiff_t;

    Derived&
    operator+=(difference_type n) noexcept {
        Derived& self = static_cast<Derived&>(*this);
        self.advance(n);
        return self;
    }

    Derived&
    operator-=(difference_type n) noexcept {
        Derived& self = static_cast<Derived&>(*this);
        return self += -n;
    }

    CIEL_NODISCARD Derived
    operator+(difference_type n) noexcept {
        Derived& self = static_cast<Derived&>(*this);
        Derived res(self);
        res += n;
        return res;
    }

    CIEL_NODISCARD Derived
    operator-(difference_type n) noexcept {
        Derived& self = static_cast<Derived&>(*this);
        Derived res(self);
        res -= n;
        return res;
    }

    // TODO: friend

}; // struct random_access_iterator_base

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_ITERATOR_BASE_HPP_

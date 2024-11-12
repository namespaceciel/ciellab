#ifndef CIELLAB_INCLUDE_CIEL_AS_CONST_HPP_
#define CIELLAB_INCLUDE_CIEL_AS_CONST_HPP_

#include <ciel/config.hpp>

NAMESPACE_CIEL_BEGIN

template<class T>
add_const_t<T>& as_const(T& t) noexcept {
    return t;
}

template<class T>
void as_const(const T&&) = delete;

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_AS_CONST_HPP_

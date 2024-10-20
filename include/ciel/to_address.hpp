#ifndef CIELLAB_INCLUDE_CIEL_TO_ADDRESS_HPP_
#define CIELLAB_INCLUDE_CIEL_TO_ADDRESS_HPP_

#include <ciel/config.hpp>

#include <utility>

NAMESPACE_CIEL_BEGIN

template<class T>
T*
to_address(T* p) noexcept {
    return p;
}

template<class T>
auto
to_address(T&& p) noexcept -> decltype(ciel::to_address(std::forward<T>(p).base())) {
    return ciel::to_address(std::forward<T>(p).base());
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TO_ADDRESS_HPP_

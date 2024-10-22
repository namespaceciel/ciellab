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
decltype(std::declval<T&&>().operator->())
to_address(T&& p) noexcept {
    return ciel::to_address(p.operator->());
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TO_ADDRESS_HPP_

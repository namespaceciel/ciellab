#ifndef CIELLAB_INCLUDE_CIEL_TO_ADDRESS_HPP_
#define CIELLAB_INCLUDE_CIEL_TO_ADDRESS_HPP_

#include <ciel/config.hpp>

#include <type_traits>

NAMESPACE_CIEL_BEGIN

template<class T>
T*
to_address(T* p) noexcept {
    static_assert(!std::is_function<T>::value, "");
    return p;
}

template<class T>
auto
to_address(const T& p) noexcept -> decltype(ciel::to_address(p.operator->())) {
    return ciel::to_address(p.operator->());
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TO_ADDRESS_HPP_

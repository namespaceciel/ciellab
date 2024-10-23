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

CIEL_DIAGNOSTIC_PUSH
#if CIEL_STD_VER >= 20
// std::move_iterator's operator-> has been deprecated in C++20
CIEL_CLANG_DIAGNOSTIC_IGNORED("-Wdeprecated-declarations")
#endif

template<class T>
decltype(std::declval<T&&>().operator->())
to_address(T&& p) noexcept {
    return ciel::to_address(p.operator->());
}

CIEL_DIAGNOSTIC_POP

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TO_ADDRESS_HPP_

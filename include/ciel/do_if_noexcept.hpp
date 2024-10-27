#ifndef CIELLAB_INCLUDE_CIEL_DO_IF_NOEXCEPT_HPP_
#define CIELLAB_INCLUDE_CIEL_DO_IF_NOEXCEPT_HPP_

#include <ciel/config.hpp>
#include <ciel/logical.hpp>

#include <type_traits>
#include <utility>

NAMESPACE_CIEL_BEGIN

template<class T>
#ifdef CIEL_HAS_EXCEPTIONS
typename std::conditional<
    conjunction<negation<std::is_nothrow_move_constructible<T>>, std::is_copy_constructible<T>>::value, const T&,
    T&&>::type
#else
T&&
#endif
move_if_noexcept(T& x) noexcept {
    return std::move(x);
}

template<class T, class RT = typename std::remove_reference<T>::type>
#ifdef CIEL_HAS_EXCEPTIONS
typename std::conditional<
    conjunction<negation<std::is_nothrow_move_constructible<RT>>, std::is_copy_constructible<RT>>::value, const RT&,
    T&&>::type
#else
T&&
#endif
forward_if_noexcept(T&& x) noexcept {
    return std::forward<T>(x);
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_DO_IF_NOEXCEPT_HPP_

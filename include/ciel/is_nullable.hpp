#ifndef CIELLAB_INCLUDE_CIEL_IS_NULLABLE_HPP_
#define CIELLAB_INCLUDE_CIEL_IS_NULLABLE_HPP_

#include <ciel/config.hpp>
#include <ciel/logical.hpp>

#include <type_traits>

NAMESPACE_CIEL_BEGIN

// std::is_convertible can't work with explicit conversion operator.
template<class T>
struct is_nullable : conjunction<std::is_default_constructible<T>, std::is_constructible<T, nullptr_t>,
                                 std::is_constructible<bool, T>, std::is_assignable<T&, nullptr_t>> {};

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_IS_NULLABLE_HPP_

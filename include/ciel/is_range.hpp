#ifndef CIELLAB_INCLUDE_CIEL_IS_RANGE_HPP_
#define CIELLAB_INCLUDE_CIEL_IS_RANGE_HPP_

#include <ciel/config.hpp>
#include <ciel/void_t.hpp>

#include <type_traits>
#include <utility>

NAMESPACE_CIEL_BEGIN

// is_range
template<class T, class = void>
struct is_range : std::false_type {};

template<class T>
struct is_range<T, ciel::void_t<decltype(std::declval<T>().begin(), std::declval<T>().end())>> : std::true_type {};

template<class T, class = void>
struct is_range_with_size : std::false_type {};

template<class T>
struct is_range_with_size<
    T, ciel::void_t<decltype(std::declval<T>().begin(), std::declval<T>().end(), std::declval<T>().size())>>
    : std::true_type {};

template<class T>
struct is_range_without_size : std::integral_constant<bool, is_range<T>::value && !is_range_with_size<T>::value> {};

// from_range_t
struct from_range_t {};

static constexpr from_range_t from_range;

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_IS_RANGE_HPP_

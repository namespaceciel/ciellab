#ifndef CIELLAB_INCLUDE_CIEL_CORE_IS_REFERENCE_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_IS_REFERENCE_HPP_

#include <ciel/core/config.hpp>

#include <type_traits>

NAMESPACE_CIEL_BEGIN

// is_const_lvalue_reference

template<class T>
struct is_const_lvalue_reference : std::false_type {};

template<class T>
struct is_const_lvalue_reference<const T&> : std::true_type {};

// is_const_rvalue_reference

template<class T>
struct is_const_rvalue_reference : std::false_type {};

template<class T>
struct is_const_rvalue_reference<const T&&> : std::true_type {};

// is_const_reference

template<class T>
struct is_const_reference
    : std::integral_constant<bool, is_const_lvalue_reference<T>::value || is_const_rvalue_reference<T>::value> {};

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_IS_REFERENCE_HPP_

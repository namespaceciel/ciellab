#ifndef CIELLAB_INCLUDE_CIEL_TYPE_TRAITS_HPP_
#define  CIELLAB_INCLUDE_CIEL_TYPE_TRAITS_HPP_

#include <iterator>
#include <type_traits>

#include <ciel/config.hpp>

NAMESPACE_CIEL_BEGIN

// void_t
template<class...>
using void_t = void;

// is_exactly_input_iterator
template<class Iter, class = void>
struct is_exactly_input_iterator : std::false_type {};

template<class Iter>
struct is_exactly_input_iterator<Iter, void_t<typename std::iterator_traits<Iter>::iterator_category>>
    : std::is_same<typename std::iterator_traits<Iter>::iterator_category, std::input_iterator_tag> {};

// is_forward_iterator
template<class Iter, class = void>
struct is_forward_iterator : std::false_type {};

template<class Iter>
struct is_forward_iterator<Iter, void_t<typename std::iterator_traits<Iter>::iterator_category>>
    : std::is_convertible<typename std::iterator_traits<Iter>::iterator_category, std::forward_iterator_tag> {};

// is_input_iterator
template<class Iter, class = void>
struct is_input_iterator : std::false_type {};

template<class Iter>
struct is_input_iterator<Iter, void_t<typename std::iterator_traits<Iter>::iterator_category>>
    : std::is_convertible<typename std::iterator_traits<Iter>::iterator_category, std::input_iterator_tag> {};

// is_trivially_relocatable
template<class T, bool = std::is_trivially_copyable<T>::value>
struct is_trivially_relocatable : std::false_type {};

template<class T>
struct is_trivially_relocatable<T, true> : std::true_type {};

// useless_tag
struct useless_tag {
    useless_tag(...) noexcept {}
};

// owner
template<class T, class = typename std::enable_if<std::is_pointer<T>::value>::type>
using owner = T;

// is_final
#if CIEL_STD_VER >= 14
template<class T>
using is_final = std::is_final<T>;

#elif __has_builtin(__is_final)
template<class T>
struct is_final : public std::integral_constant<bool, __is_final(T)> {};

#else
template<class T>
struct is_final = std::true_type;
#endif

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TYPE_TRAITS_HPP_
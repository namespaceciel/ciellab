#ifndef CIELLAB_INCLUDE_CIEL_ITERATOR_CATEGORY_HPP_
#define CIELLAB_INCLUDE_CIEL_ITERATOR_CATEGORY_HPP_

#include <ciel/config.hpp>
#include <ciel/void_t.hpp>

#include <iterator>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

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

// is_contiguous_iterator

template<class Iter, class = void>
struct is_contiguous_iterator : std::false_type {};

#if CIEL_STD_VER >= 20
template<class Iter>
struct is_contiguous_iterator<Iter, void_t<typename std::iterator_traits<Iter>::iterator_concept>>
    : std::is_convertible<typename std::iterator_traits<Iter>::iterator_concept, std::contiguous_iterator_tag> {};
#endif

template<class T>
struct is_contiguous_iterator<T*> : std::true_type {};

template<class T>
struct is_contiguous_iterator<std::move_iterator<T>> : is_contiguous_iterator<T> {};

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_ITERATOR_CATEGORY_HPP_

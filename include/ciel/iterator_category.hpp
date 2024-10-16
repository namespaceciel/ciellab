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
struct is_exactly_input_iterator<Iter, ciel::void_t<typename std::iterator_traits<Iter>::iterator_category>>
    : std::is_same<typename std::iterator_traits<Iter>::iterator_category, std::input_iterator_tag> {};

// is_forward_iterator
template<class Iter, class = void>
struct is_forward_iterator : std::false_type {};

template<class Iter>
struct is_forward_iterator<Iter, ciel::void_t<typename std::iterator_traits<Iter>::iterator_category>>
    : std::is_convertible<typename std::iterator_traits<Iter>::iterator_category, std::forward_iterator_tag> {};

// is_input_iterator
template<class Iter, class = void>
struct is_input_iterator : std::false_type {};

template<class Iter>
struct is_input_iterator<Iter, ciel::void_t<typename std::iterator_traits<Iter>::iterator_category>>
    : std::is_convertible<typename std::iterator_traits<Iter>::iterator_category, std::input_iterator_tag> {};

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_ITERATOR_CATEGORY_HPP_

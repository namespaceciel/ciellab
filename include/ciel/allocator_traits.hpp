#ifndef CIELLAB_INCLUDE_CIEL_ALLOCATOR_TRAITS_HPP_
#define CIELLAB_INCLUDE_CIEL_ALLOCATOR_TRAITS_HPP_

#include <ciel/config.hpp>
#include <ciel/logical.hpp>
#include <ciel/void_t.hpp>

#include <memory>
#include <type_traits>
#include <utility>

NAMESPACE_CIEL_BEGIN

namespace detail {

// allocator_has_construct

template<class, class Alloc, class... Args>
struct allocator_has_construct_impl : std::false_type {};

template<class Alloc, class... Args>
struct allocator_has_construct_impl<ciel::void_t<decltype(std::declval<Alloc>().construct(std::declval<Args>()...))>,
                                    Alloc, Args...> : std::true_type {};

template<class Alloc, class... Args>
struct allocator_has_construct : allocator_has_construct_impl<void, Alloc, Args...> {};

// allocator_has_destroy

template<class Alloc, class Pointer, class = void>
struct allocator_has_destroy : std::false_type {};

template<class Alloc, class Pointer>
struct allocator_has_destroy<Alloc, Pointer,
                             ciel::void_t<decltype(std::declval<Alloc>().destroy(std::declval<Pointer>()))>>
    : std::true_type {};

} // namespace detail

// allocator_has_trivial_default_construct

template<class Alloc, class T = typename Alloc::value_type>
struct allocator_has_trivial_default_construct : ciel::negation<detail::allocator_has_construct<Alloc, T*>> {};

template<class Alloc, class T = typename Alloc::value_type>
struct allocator_has_trivial_copy_construct : ciel::negation<detail::allocator_has_construct<Alloc, T*, const T&>> {};

template<class Alloc, class T = typename Alloc::value_type>
struct allocator_has_trivial_move_construct : ciel::negation<detail::allocator_has_construct<Alloc, T*, T&&>> {};

template<class Alloc, class Pointer, class... Args>
struct allocator_has_trivial_construct : ciel::negation<detail::allocator_has_construct<Alloc, Pointer, Args...>> {};

// allocator_has_trivial_destroy

template<class Alloc, class T = typename Alloc::value_type>
struct allocator_has_trivial_destroy : ciel::negation<detail::allocator_has_destroy<Alloc, T*>> {};

// specializations for std::allocator

template<class T>
struct allocator_has_trivial_default_construct<std::allocator<T>> : std::true_type {};

template<class T>
struct allocator_has_trivial_copy_construct<std::allocator<T>> : std::true_type {};

template<class T>
struct allocator_has_trivial_move_construct<std::allocator<T>> : std::true_type {};

template<class T, class... Args>
struct allocator_has_trivial_construct<std::allocator<T>, T*, Args...> : std::true_type {};

template<class T>
struct allocator_has_trivial_destroy<std::allocator<T>> : std::true_type {};

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_ALLOCATOR_TRAITS_HPP_

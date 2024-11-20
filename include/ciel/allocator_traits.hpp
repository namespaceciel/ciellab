#ifndef CIELLAB_INCLUDE_CIEL_ALLOCATOR_TRAITS_HPP_
#define CIELLAB_INCLUDE_CIEL_ALLOCATOR_TRAITS_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/logical.hpp>

#include <memory>
#include <type_traits>
#include <utility>

NAMESPACE_CIEL_BEGIN

namespace detail {

// allocator_has_construct

template<class, class Alloc, class... Args>
struct allocator_has_construct_impl : std::false_type {};

template<class Alloc, class... Args>
struct allocator_has_construct_impl<void_t<decltype(std::declval<Alloc>().construct(std::declval<Args>()...))>, Alloc,
                                    Args...> : std::true_type {};

template<class Alloc, class... Args>
struct allocator_has_construct : allocator_has_construct_impl<void, Alloc, Args...> {};

// allocator_has_destroy

template<class Alloc, class Pointer, class = void>
struct allocator_has_destroy : std::false_type {};

template<class Alloc, class Pointer>
struct allocator_has_destroy<Alloc, Pointer, void_t<decltype(std::declval<Alloc>().destroy(std::declval<Pointer>()))>>
    : std::true_type {};

} // namespace detail

// allocator_has_trivial_construct
// allocator_has_trivial_destroy
// Note that Pointer type may not be same as the pointer to typename Alloc::value_type.

template<class Alloc, class Pointer, class... Args>
struct allocator_has_trivial_construct : negation<detail::allocator_has_construct<Alloc, Pointer, Args...>> {};

template<class Alloc, class Pointer>
struct allocator_has_trivial_destroy : negation<detail::allocator_has_destroy<Alloc, Pointer>> {};

// specializations for std::allocator

template<class T, class Pointer, class... Args>
struct allocator_has_trivial_construct<std::allocator<T>, Pointer, Args...> : std::true_type {};

template<class T, class Pointer>
struct allocator_has_trivial_destroy<std::allocator<T>, Pointer> : std::true_type {};

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_ALLOCATOR_TRAITS_HPP_

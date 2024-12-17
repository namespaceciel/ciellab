#ifndef CIELLAB_INCLUDE_CIEL_CORE_IS_TRIVIALLY_RELOCATABLE_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_IS_TRIVIALLY_RELOCATABLE_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/logical.hpp>

#include <tuple>
#include <type_traits>
#include <utility>

NAMESPACE_CIEL_BEGIN

// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1144r10.html

template<class T>
struct is_trivially_relocatable : disjunction<std::is_trivially_copyable<T>,
#ifdef _LIBCPP___TYPE_TRAITS_IS_TRIVIALLY_RELOCATABLE_H
                                              std::__libcpp_is_trivially_relocatable<T>,
#elif __has_builtin(__is_trivially_relocatable)
                                              std::integral_constant<bool, __is_trivially_relocatable(T)>,
#endif
                                              std::false_type> {
};

template<class First, class Second>
struct is_trivially_relocatable<std::pair<First, Second>>
    : conjunction<is_trivially_relocatable<First>, is_trivially_relocatable<Second>> {};

template<class... Types>
struct is_trivially_relocatable<std::tuple<Types...>> : conjunction<is_trivially_relocatable<Types>...> {};

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_IS_TRIVIALLY_RELOCATABLE_HPP_

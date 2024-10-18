#ifndef CIELLAB_INCLUDE_CIEL_IS_TRIVIALLY_RELOCATABLE_HPP_
#define CIELLAB_INCLUDE_CIEL_IS_TRIVIALLY_RELOCATABLE_HPP_

#include <ciel/config.hpp>
#include <ciel/logical.hpp>

#include <tuple>
#include <type_traits>
#include <utility>

NAMESPACE_CIEL_BEGIN

template<class T>
struct is_trivially_relocatable : ciel::disjunction<std::is_trivially_move_constructible<T>,
#ifdef _LIBCPP___TYPE_TRAITS_IS_TRIVIALLY_RELOCATABLE_H
                                                    std::__libcpp_is_trivially_relocatable<T>,
#elif __has_builtin(__is_trivially_relocatable)
                                                    std::integral_constant<bool, __is_trivially_relocatable(T)>,
#endif
                                                    std::false_type> {
};

template<class First, class Second>
struct is_trivially_relocatable<std::pair<First, Second>>
    : ciel::conjunction<is_trivially_relocatable<First>, is_trivially_relocatable<Second>> {};

template<class... Types>
struct is_trivially_relocatable<std::tuple<Types...>> : ciel::conjunction<is_trivially_relocatable<Types>...> {};

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_IS_TRIVIALLY_RELOCATABLE_HPP_

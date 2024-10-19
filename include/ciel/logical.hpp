#ifndef CIELLAB_INCLUDE_CIEL_LOGICAL_HPP_
#define CIELLAB_INCLUDE_CIEL_LOGICAL_HPP_

#include <ciel/config.hpp>

#include <type_traits>

NAMESPACE_CIEL_BEGIN

// conjunction
// When the template pack is empty, derive from true_type.
template<class...>
struct conjunction : std::true_type {};

// Otherwise, derive from the first false template member (if all true, choose the last one).
template<class B1, class... Bn>
struct conjunction<B1, Bn...> : std::conditional<static_cast<bool>(B1::value), conjunction<Bn...>, B1>::type {};

// disjunction
template<class...>
struct disjunction : std::false_type {};

template<class B1, class... Bn>
struct disjunction<B1, Bn...> : std::conditional<static_cast<bool>(B1::value), B1, disjunction<Bn...>>::type {};

// negation
template<class B>
struct negation : std::integral_constant<bool, !static_cast<bool>(B::value)> {};

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_LOGICAL_HPP_

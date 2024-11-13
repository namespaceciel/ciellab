#ifndef CIELLAB_INCLUDE_CIEL_CORE_IS_FINAL_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_IS_FINAL_HPP_

#include <ciel/core/config.hpp>

#include <type_traits>

NAMESPACE_CIEL_BEGIN

#if CIEL_STD_VER >= 14
template<class T>
using is_final = std::is_final<T>;
#elif __has_builtin(__is_final)
template<class T>
struct is_final : std::integral_constant<bool, __is_final(T)> {};
#else
// If is_final is not available, it may be better to manually write explicit template specializations.
// e.g.
// template<>
// struct is_final<NotFinalObject> : std::false_type {};
//
template<class T>
struct is_final;
#endif

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_IS_FINAL_HPP_

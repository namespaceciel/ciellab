#ifndef CIELLAB_INCLUDE_CIEL_CORE_IS_COMPLETE_TYPE_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_IS_COMPLETE_TYPE_HPP_

#include <ciel/core/config.hpp>

NAMESPACE_CIEL_BEGIN

#if CIEL_STD_VER >= 20
template<class T, auto = [] {}>
inline constexpr bool is_complete_type_v = requires { sizeof(T); };
#endif

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_IS_COMPLETE_TYPE_HPP_

#ifndef CIELLAB_INCLUDE_CIEL_IS_COMPLETE_TYPE_HPP_
#define CIELLAB_INCLUDE_CIEL_IS_COMPLETE_TYPE_HPP_

#include <ciel/config.hpp>

NAMESPACE_CIEL_BEGIN

#if CIEL_STD_VER >= 20
template<class T, auto = [] {}>
inline constexpr bool is_complete_type_v = requires { sizeof(T); };
#endif // CIEL_STD_VER >= 20

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_IS_COMPLETE_TYPE_HPP_

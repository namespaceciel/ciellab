#ifndef CIELLAB_INCLUDE_CIEL_CORE_CAN_BE_DESTROYED_FROM_BASE_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_CAN_BE_DESTROYED_FROM_BASE_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/logical.hpp>

#include <type_traits>

NAMESPACE_CIEL_BEGIN

template<class Base, class Derived>
struct can_be_destroyed_from_base
    : conjunction<std::is_destructible<Base>, std::is_base_of<Base, Derived>,
                  disjunction<std::has_virtual_destructor<Base>, std::is_trivially_destructible<Derived>>> {};

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_CAN_BE_DESTROYED_FROM_BASE_HPP_

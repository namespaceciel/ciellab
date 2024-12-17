#ifndef CIELLAB_INCLUDE_CIEL_CORE_CAN_BE_DESTROYED_FROM_BASE_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_CAN_BE_DESTROYED_FROM_BASE_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/logical.hpp>

#include <type_traits>

NAMESPACE_CIEL_BEGIN

// https://en.cppreference.com/w/cpp/memory/unique_ptr
// If T is a derived class of some base B, then std::unique_ptr<T> is implicitly convertible to std::unique_ptr<B>.
// The default deleter of the resulting std::unique_ptr<B> will use operator delete for B,
// leading to undefined behavior unless the destructor of B is virtual.
//
// It may still be UB when T is trivially destructible, but temporarily leave it here.

template<class Base, class Derived>
struct can_be_destroyed_from_base
    : conjunction<std::is_destructible<Base>, std::is_base_of<Base, Derived>,
                  disjunction<std::has_virtual_destructor<Base>, std::is_trivially_destructible<Derived>>> {};

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_CAN_BE_DESTROYED_FROM_BASE_HPP_

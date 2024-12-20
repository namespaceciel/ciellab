#ifndef CIELLAB_INCLUDE_CIEL_CORE_CAN_BE_DESTROYED_FROM_BASE_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_CAN_BE_DESTROYED_FROM_BASE_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/logical.hpp>

#include <cstddef>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

// See https://eel.is/c++draft/intro.defs#defns.dynamic.type-example-1 for definitions of static and dynamic type:
// If a pointer p whose static type is “pointer to class B” is pointing to an object of class D,
// derived from B, the dynamic type of the expression *p is “D”.
//
// See https://eel.is/c++draft/conv.qual#2 for definitions of similar:
// Two types T1 and T2 are similar if they have qualification-decompositions with the same n...
//
// See https://eel.is/c++draft/expr.delete#3:
// In a single-object delete expression, if the static type of the object to be deleted is not similar to
// its dynamic type and the selected deallocation function is not a destroying operator delete,
// the static type shall be a base class of the dynamic type of the object to be deleted
// and the static type shall have a virtual destructor or the behavior is undefined.
//
// In conclusion: This class is useful for std::default_delete<T> (used by std::unique_ptr<T>),
// if D is a derived class of some base B, then std::unique_ptr<D> is implicitly convertible to std::unique_ptr<B>.
// The default deleter of the resulting std::unique_ptr<B> will use operator delete for B,
// leading to undefined behavior unless the destructor of B is virtual.
//
// Note: The same issue doesn't apply on std::shared_ptr<T> since the managed object will
// always be deleted from the Derived's side by control_block.
//
// Note: It may be safe deleting trivially destructible objects from Base, but for some reasons
// standard refused to make it well-defined, see https://cplusplus.github.io/EWG/ewg-closed.html#99.

// Inspired by: https://github.com/Quuxplusone/llvm-project/commit/d40169c2feebfbddf48df4e4e81cc0b62fa884be

template<class T, class U, class RT = remove_cv_t<T>, class RU = remove_cv_t<U>>
struct is_similar : std::is_same<RT, RU> {
    static_assert(std::is_same<RT, remove_cv_t<T>>::value, "Don't touch default parameters");
    static_assert(std::is_same<RU, remove_cv_t<U>>::value, "Don't touch default parameters");
};

template<class T, class U>
struct is_similar<T*, U*> : is_similar<T, U> {};

template<class T, class U, class C>
struct is_similar<T C::*, U C::*> : is_similar<T, U> {};

template<class T, class U>
struct is_similar<T[], U[]> : is_similar<T, U> {};

template<class T, class U, size_t N>
struct is_similar<T[], U[N]> : is_similar<T, U> {};

template<class T, class U, size_t N>
struct is_similar<T[N], U[]> : is_similar<T, U> {};

template<class T, class U, size_t N>
struct is_similar<T[N], U[N]> : is_similar<T, U> {};

template<class Base, class Derived>
struct can_be_destroyed_from_base
    : conjunction<std::is_destructible<Base>, std::is_base_of<Base, Derived>,
                  disjunction<is_similar<Base, Derived>, std::has_virtual_destructor<Base>>> {};

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_CAN_BE_DESTROYED_FROM_BASE_HPP_

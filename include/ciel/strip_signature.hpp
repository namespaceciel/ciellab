#ifndef CIELLAB_INCLUDE_CIEL_STRIP_SIGNATURE_HPP_
#define CIELLAB_INCLUDE_CIEL_STRIP_SIGNATURE_HPP_

#include <ciel/config.hpp>

NAMESPACE_CIEL_BEGIN

#if CIEL_STD_VER >= 17

template<class>
struct strip_signature;

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...)> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) const> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) volatile> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) const volatile> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) &> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) const&> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) volatile&> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) const volatile&> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) noexcept> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) const noexcept> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) volatile noexcept> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) const volatile noexcept> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) & noexcept> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) const & noexcept> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) volatile & noexcept> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) const volatile & noexcept> {
    using type = R(Args...);
};

template<class F>
using strip_signature_t = typename strip_signature<F>::type;

#endif

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_STRIP_SIGNATURE_HPP_

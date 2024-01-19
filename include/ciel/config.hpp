#ifndef CIELLAB_INCLUDE_CIEL_CONFIG_HPP_
#define CIELLAB_INCLUDE_CIEL_CONFIG_HPP_

#include <cassert>
#include <initializer_list>

// exception
#ifdef __cpp_exceptions
#define CIEL_HAS_EXCEPTIONS
#endif

#ifdef CIEL_HAS_EXCEPTIONS
#define CIEL_TRY      try
#define CIEL_CATCH(X) catch (X)
#define CIEL_THROW    throw
#else
#define CIEL_TRY      if constexpr (true)
#define CIEL_CATCH(X) if constexpr (false)
#define CIEL_THROW
#endif

// rtti
#ifdef __cpp_rtti
#define CIEL_HAS_RTTI
#endif

// standard_version
#if __cplusplus <= 201103L
#define CIEL_STD_VER 11
#elif __cplusplus <= 201402L
#define CIEL_STD_VER 14
#elif __cplusplus <= 201703L
#define CIEL_STD_VER 17
#elif __cplusplus <= 202002L
#define CIEL_STD_VER 20
#elif __cplusplus <= 202302L
#define CIEL_STD_VER 23
#else
#define CIEL_STD_VER 26
#endif

// constexpr
#if CIEL_STD_VER >= 14
#define CIEL_CONSTEXPR_SINCE_CXX14 constexpr
#else
#define CIEL_CONSTEXPR_SINCE_CXX14
#endif

#if CIEL_STD_VER >= 17
#define CIEL_CONSTEXPR_SINCE_CXX17 constexpr
#else
#define CIEL_CONSTEXPR_SINCE_CXX17
#endif

#if CIEL_STD_VER >= 20
#define CIEL_CONSTEXPR_SINCE_CXX20 constexpr
#else
#define CIEL_CONSTEXPR_SINCE_CXX20
#endif

#if CIEL_STD_VER >= 23
#define CIEL_CONSTEXPR_SINCE_CXX23 constexpr
#else
#define CIEL_CONSTEXPR_SINCE_CXX23
#endif

// nodiscard
#if CIEL_STD_VER >= 17
#define CIEL_NODISCARD [[nodiscard]]
#else
#define CIEL_NODISCARD
#endif

// likely, unlikely
#if CIEL_STD_VER >= 20
#define CIEL_LIKELY [[likely]]
#define CIEL_UNLIKELY [[unlikely]]
#else
#define CIEL_LIKELY
#define CIEL_UNLIKELY
#endif

// namespace ciel
#define NAMESPACE_CIEL_BEGIN namespace ciel {

#define NAMESPACE_CIEL_END }

NAMESPACE_CIEL_BEGIN

[[noreturn]] inline void unreachable() {

#if defined(_MSC_VER) && !defined(__clang__)    // MSVC
    __assume(false);

#else    // GCC, Clang
    __builtin_unreachable();

#endif
}

template<class Exception>
[[noreturn]] inline void THROW(Exception&& e) {

#ifdef CIEL_HAS_EXCEPTIONS
    throw e;

#else
    static_cast<void>(e);
    std::terminate();

#endif
}

NAMESPACE_CIEL_END

#define CIEL_PRECONDITION(cond) assert(cond)
#define CIEL_POSTCONDITION(cond) assert(cond)

#if CIEL_STD_VER >= 17

namespace std {

template<class T>
initializer_list(initializer_list<T>) -> initializer_list<T>;

}   // namespace std

#endif

#endif // CIELLAB_INCLUDE_CIEL_CONFIG_HPP_
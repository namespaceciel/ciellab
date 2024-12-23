#ifndef CIELLAB_INCLUDE_CIEL_CORE_CONFIG_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_CONFIG_HPP_

#if !defined(__cplusplus) || (__cplusplus < 201103L)
#  error "Please use C++ with standard of at least 11"
#endif

#include <cassert>
#include <cstddef>
#include <type_traits>

// exception

#ifdef __cpp_exceptions
#  define CIEL_HAS_EXCEPTIONS
#endif

// rtti

#ifdef __cpp_rtti
#  define CIEL_HAS_RTTI
#endif

// debug_mode

#ifndef NDEBUG
#  define CIEL_IS_DEBUGGING
#endif

// standard_version

#if __cplusplus <= 201103L
#  define CIEL_STD_VER 11
#elif __cplusplus <= 201402L
#  define CIEL_STD_VER 14
#elif __cplusplus <= 201703L
#  define CIEL_STD_VER 17
#elif __cplusplus <= 202002L
#  define CIEL_STD_VER 20
#elif __cplusplus <= 202302L
#  define CIEL_STD_VER 23
#else
#  define CIEL_STD_VER 26
#endif

// constexpr

#if CIEL_STD_VER >= 14
#  define CIEL_CONSTEXPR_SINCE_CXX14 constexpr
#else
#  define CIEL_CONSTEXPR_SINCE_CXX14
#endif

#if CIEL_STD_VER >= 17
#  define CIEL_CONSTEXPR_SINCE_CXX17 constexpr
#else
#  define CIEL_CONSTEXPR_SINCE_CXX17
#endif

#if CIEL_STD_VER >= 20
#  define CIEL_CONSTEXPR_SINCE_CXX20 constexpr
#else
#  define CIEL_CONSTEXPR_SINCE_CXX20
#endif

#if CIEL_STD_VER >= 23
#  define CIEL_CONSTEXPR_SINCE_CXX23 constexpr
#else
#  define CIEL_CONSTEXPR_SINCE_CXX23
#endif

// nodiscard

#if CIEL_STD_VER >= 17
#  define CIEL_NODISCARD [[nodiscard]]
#elif (defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__) // clang, icc, clang-cl
#  define CIEL_NODISCARD __attribute__((warn_unused_result))
#elif defined(_HAS_NODISCARD)
#  define CIEL_NODISCARD _NODISCARD
#elif _MSC_VER >= 1700
#  define CIEL_NODISCARD _Check_return_
#else
#  define CIEL_NODISCARD
#endif

// likely, unlikely

#if CIEL_STD_VER >= 20 || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#  define CIEL_LIKELY(x)   (x) [[likely]]
#  define CIEL_UNLIKELY(x) (x) [[unlikely]]
#elif defined(__GNUC__) || defined(__clang__)
#  define CIEL_LIKELY(x)   (__builtin_expect(!!(x), true))
#  define CIEL_UNLIKELY(x) (__builtin_expect(!!(x), false))
#else
#  define CIEL_LIKELY(x)   (x)
#  define CIEL_UNLIKELY(x) (x)
#endif

// __has_builtin

#ifndef __has_builtin
#  define __has_builtin(x) false
#endif

// __has_include

#ifndef __has_include
#  define __has_include(x) false
#endif

// try catch throw

#ifdef CIEL_HAS_EXCEPTIONS
#  define CIEL_TRY      try
#  define CIEL_CATCH(x) catch (x)
#  define CIEL_THROW    throw
#else
#  define CIEL_TRY      if (true)
#  define CIEL_CATCH(x) if (false)
#  define CIEL_THROW
#endif

// to_string

#define CIEL_TO_STRING2(x) #x
#define CIEL_TO_STRING(x)  CIEL_TO_STRING2(x)

// compiler diagnostic ignored

#if defined(__clang__)
#  define CIEL_DIAGNOSTIC_PUSH               _Pragma("clang diagnostic push")
#  define CIEL_DIAGNOSTIC_POP                _Pragma("clang diagnostic pop")
#  define CIEL_CLANG_DIAGNOSTIC_IGNORED(str) _Pragma(CIEL_TO_STRING(clang diagnostic ignored str))
#  define CIEL_GCC_DIAGNOSTIC_IGNORED(str)
#elif defined(__GNUC__)
#  define CIEL_DIAGNOSTIC_PUSH _Pragma("GCC diagnostic push")
#  define CIEL_DIAGNOSTIC_POP  _Pragma("GCC diagnostic pop")
#  define CIEL_CLANG_DIAGNOSTIC_IGNORED(str)
#  define CIEL_GCC_DIAGNOSTIC_IGNORED(str) _Pragma(CIEL_TO_STRING(GCC diagnostic ignored str))
#else
#  define CIEL_DIAGNOSTIC_PUSH
#  define CIEL_DIAGNOSTIC_POP
#  define CIEL_CLANG_DIAGNOSTIC_IGNORED(str)
#  define CIEL_GCC_DIAGNOSTIC_IGNORED(str)
#endif

// namespace ciel

#define NAMESPACE_CIEL_BEGIN namespace ciel {
#define NAMESPACE_CIEL_END   } // namespace ciel

using std::nullptr_t;
using std::ptrdiff_t;
using std::size_t;

NAMESPACE_CIEL_BEGIN

// useless_tag

struct useless_tag {
    template<class... Args>
    useless_tag(Args&&...) noexcept {}

}; // struct useless_tag

// void_cast

template<class... Args>
void void_cast(Args&&...) noexcept {}

// unreachable

[[noreturn]] inline void unreachable() noexcept {
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
    __assume(false);
#else                                        // GCC, Clang
    __builtin_unreachable();
#endif
}

// Used to isolate values on cache lines to prevent false sharing.
static constexpr size_t cacheline_size = 64;

NAMESPACE_CIEL_END

// unused
// simple (void) cast won't stop gcc

#define CIEL_UNUSED(...) ciel::void_cast(__VA_ARGS__)

// assume

#if CIEL_STD_VER >= 23 && ((defined(__clang__) && __clang__ >= 19) || (defined(__GNUC__) && __GNUC__ >= 13))
#  define CIEL_ASSUME(cond) [[assume(cond)]]
#elif defined(__clang__)
#  if __has_builtin(__builtin_assume)
#    define CIEL_ASSUME(cond) __builtin_assume(cond)
#  else
#    define CIEL_ASSUME(cond) CIEL_UNUSED(cond)
#  endif
#elif defined(_MSC_VER)
#  define CIEL_ASSUME(cond) __assume(cond)
#elif defined(__GNUC__) && __GNUC__ >= 13
#  define CIEL_ASSUME(cond) __attribute__((assume(cond)))
#else
#  define CIEL_ASSUME(cond) CIEL_UNUSED(cond)
#endif

NAMESPACE_CIEL_BEGIN

// alias for type_traits

template<class...>
using void_t = void;

template<bool B, class T, class F>
using conditional_t = typename std::conditional<B, T, F>::type;

template<bool B, class T = int>
using enable_if_t = typename std::enable_if<B, T>::type;

template<class T>
using decay_t = typename std::decay<T>::type;

template<class T>
using add_lvalue_reference_t = typename std::add_lvalue_reference<T>::type;

template<class T>
using add_rvalue_reference_t = typename std::add_rvalue_reference<T>::type;

template<class... T>
using common_type_t = typename std::common_type<T...>::type;

template<class T>
using remove_extent_t = typename std::remove_extent<T>::type;

template<class T>
using remove_const_t = typename std::remove_const<T>::type;

template<class T>
using remove_volatile_t = typename std::remove_volatile<T>::type;

template<class T>
using remove_cv_t = typename std::remove_cv<T>::type;

template<class T>
using remove_reference_t = typename std::remove_reference<T>::type;

template<class T>
using remove_cvref_t = remove_cv_t<remove_reference_t<T>>;

template<class T>
using add_const_t = typename std::add_const<T>::type;

template<class T>
using add_volatile_t = typename std::add_volatile<T>::type;

template<class T>
using add_cv_t = typename std::add_cv<T>::type;

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_CONFIG_HPP_

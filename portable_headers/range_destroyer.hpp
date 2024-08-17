#ifndef CIELLAB_INCLUDE_CIEL_RANGE_DESTROYER_HPP_
#define CIELLAB_INCLUDE_CIEL_RANGE_DESTROYER_HPP_

#include <memory>
#include <type_traits>

#ifndef CIELLAB_INCLUDE_CIEL_CONFIG_HPP_
#define CIELLAB_INCLUDE_CIEL_CONFIG_HPP_

#if !defined(__cplusplus) || (__cplusplus < 201103L)
#error "Please use C++ with standard of at least 11"
#endif

#include <cassert>
#include <exception>
#include <iostream>
#include <type_traits>

// exception
#ifdef __cpp_exceptions
#define CIEL_HAS_EXCEPTIONS
#endif

// rtti
#ifdef __cpp_rtti
#define CIEL_HAS_RTTI
#endif

// debug_mode
#ifndef NDEBUG
#define CIEL_IS_DEBUGGING
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
#elif (defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__) // clang, icc, clang-cl
#define CIEL_NODISCARD __attribute__((warn_unused_result))
#elif defined(_HAS_NODISCARD)
#define CIEL_NODISCARD _NODISCARD
#elif _MSC_VER >= 1700
#define CIEL_NODISCARD _Check_return_
#else
#define CIEL_NODISCARD
#endif

// likely, unlikely
#if CIEL_STD_VER >= 20 || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#define CIEL_LIKELY(x)   (x) [[likely]]
#define CIEL_UNLIKELY(x) (x) [[unlikely]]
#elif defined(__GNUC__) || defined(__clang__)
#define CIEL_LIKELY(x)   (__builtin_expect(!!(x), true))
#define CIEL_UNLIKELY(x) (__builtin_expect(!!(x), false))
#else
#define CIEL_LIKELY(x)   (x)
#define CIEL_UNLIKELY(x) (x)
#endif

// __has_builtin
#ifndef __has_builtin
#define __has_builtin(x) false
#endif

// try catch throw
#ifdef CIEL_HAS_EXCEPTIONS
#define CIEL_TRY      try
#define CIEL_CATCH(X) catch (X)
#define CIEL_THROW    throw
#else
#define CIEL_TRY      if CIEL_CONSTEXPR_SINCE_CXX17 (true)
#define CIEL_CATCH(X) else
#define CIEL_THROW
#endif

// unused
#if defined(__GNUC__) && !defined(__clang__) // simple (void) cast won't stop gcc
#define CIEL_UNUSED(x) [](...) {}(x)
#else
#define CIEL_UNUSED(x) static_cast<void>(x)
#endif

// namespace ciel
#define NAMESPACE_CIEL_BEGIN namespace ciel {
#define NAMESPACE_CIEL_END   } // namespace ciel

NAMESPACE_CIEL_BEGIN

[[noreturn]] inline void
unreachable() noexcept {
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
    __assume(false);

#else // GCC, Clang
    __builtin_unreachable();
#endif
}

template<class Exception, typename std::enable_if<std::is_base_of<std::exception, Exception>::value, int>::type = 0>
[[noreturn]] inline void
throw_exception(Exception&& e) {
#ifdef CIEL_HAS_EXCEPTIONS
    throw e;

#else
    std::cerr << e.what() << "\n";
    std::terminate();
#endif
}

NAMESPACE_CIEL_END

// assume
#if CIEL_STD_VER >= 23
#define CIEL_ASSUME(cond) [[assume(cond)]]
#elif defined(__clang__)
#define CIEL_ASSUME(cond) __builtin_assume(cond)
#elif defined(_MSC_VER)
#define CIEL_ASSUME(cond) __assume(cond)
#elif defined(__GNUC__)
#define CIEL_ASSUME(cond) __attribute__((assume(cond)))
#else
#define CIEL_ASSUME(cond) CIEL_UNUSED(cond)
#endif

// assert
#ifdef CIEL_IS_DEBUGGING
#define CIEL_ASSERT(cond) assert(cond)
#else
#define CIEL_ASSERT(cond) CIEL_ASSUME(cond)
#endif

#define CIEL_PRECONDITION(cond)  CIEL_ASSERT(cond)
#define CIEL_POSTCONDITION(cond) CIEL_ASSERT(cond)

// deduction guide for initializer_list
#if CIEL_STD_VER >= 17
#include <initializer_list>

namespace std {

template<class T>
initializer_list(initializer_list<T>) -> initializer_list<T>;

} // namespace std
#endif

#endif // CIELLAB_INCLUDE_CIEL_CONFIG_HPP_

NAMESPACE_CIEL_BEGIN

// Destroy ranges in destructor for exception handling.

template<class T, class Allocator, class = void>
class range_destroyer : private Allocator {
    static_assert(std::is_same<typename Allocator::value_type, T>::value, "");

private:
    using allocator_type = Allocator;
    using pointer        = typename std::allocator_traits<allocator_type>::pointer;
    using alloc_traits   = std::allocator_traits<allocator_type>;

    pointer begin_;
    pointer end_;

    allocator_type&
    allocator_() noexcept {
        return static_cast<allocator_type&>(*this);
    }

public:
    range_destroyer(pointer begin, pointer end, const allocator_type& alloc) noexcept
        : allocator_type{alloc}, begin_{begin}, end_{end} {}

    range_destroyer(const range_destroyer&) = delete;
    range_destroyer&
    operator=(const range_destroyer&)
        = delete;

    ~range_destroyer() {
        CIEL_PRECONDITION(begin_ <= end_);

        while (end_ != begin_) {
            alloc_traits::destroy(allocator_(), --end_);
        }
    }

    void
    release() noexcept {
        end_ = begin_;
    }

}; // class range_destroyer

template<class T, class Allocator>
class range_destroyer<T, Allocator,
                      void_t<typename std::enable_if<std::is_trivially_destructible<T>::value, int>::type>> {
public:
    range_destroyer(...) noexcept {}

    range_destroyer(const range_destroyer&) = delete;
    range_destroyer&
    operator=(const range_destroyer&)
        = delete;

    void
    release() noexcept {}

}; // class range_destroyer

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_RANGE_DESTROYER_HPP_

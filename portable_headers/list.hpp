#ifndef CIELLAB_INCLUDE_CIEL_LIST_HPP_
#define CIELLAB_INCLUDE_CIEL_LIST_HPP_

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>

#ifndef CIELLAB_INCLUDE_CIEL_COMPRESSED_PAIR_HPP_
#define CIELLAB_INCLUDE_CIEL_COMPRESSED_PAIR_HPP_

#include <tuple>
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

// namespace ciel
#define NAMESPACE_CIEL_BEGIN namespace ciel {
#define NAMESPACE_CIEL_END   } // namespace ciel

using std::ptrdiff_t;
using std::size_t;

NAMESPACE_CIEL_BEGIN

template<class... Args>
void
void_cast(Args&&...) noexcept {}

[[noreturn]] inline void
unreachable() noexcept {
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
    __assume(false);
#else                                        // GCC, Clang
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

// unused
// simple (void) cast won't stop gcc
#define CIEL_UNUSED(x) ciel::void_cast(x)

// assume
#if CIEL_STD_VER >= 23 && ((defined(__clang__) && __clang__ >= 19) || (defined(__GNUC__) && __GNUC__ >= 13))
#define CIEL_ASSUME(cond) [[assume(cond)]]
#elif defined(__clang__)
#if __has_builtin(__builtin_assume)
#define CIEL_ASSUME(cond) __builtin_assume(cond)
#else
#define CIEL_ASSUME(cond) CIEL_UNUSED(cond)
#endif // defined(__clang__)
#elif defined(_MSC_VER)
#define CIEL_ASSUME(cond) __assume(cond)
#elif defined(__GNUC__) && __GNUC__ >= 13
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

#define CIEL_PRECONDITION(cond)  CIEL_ASSERT(static_cast<bool>(cond))
#define CIEL_POSTCONDITION(cond) CIEL_ASSERT(static_cast<bool>(cond))

// deduction guide for initializer_list
#if CIEL_STD_VER >= 17
#include <initializer_list>

namespace std {

template<class T>
initializer_list(initializer_list<T>) -> initializer_list<T>;

} // namespace std
#endif

#endif // CIELLAB_INCLUDE_CIEL_CONFIG_HPP_
#ifndef CIELLAB_INCLUDE_CIEL_INTEGER_SEQUENCE_HPP_
#define CIELLAB_INCLUDE_CIEL_INTEGER_SEQUENCE_HPP_

#include <cstddef>


NAMESPACE_CIEL_BEGIN

template<class T, T... Ints>
struct integer_sequence {
    static_assert(std::is_integral<T>::value, "");

    using value_type = T;

    static constexpr size_t
    size() noexcept {
        return sizeof...(Ints);
    }

}; // struct integer_sequence

template<size_t... Ints>
using index_sequence = integer_sequence<size_t, Ints...>;

namespace details {

// https://stackoverflow.com/questions/17424477/implementation-c14-make-integer-sequence

template<class, class>
struct merge_and_renumber;

template<size_t... I1, size_t... I2>
struct merge_and_renumber<index_sequence<I1...>, index_sequence<I2...>> {
    using type = index_sequence<I1..., (sizeof...(I1) + I2)...>;
};

template<size_t N>
struct make_index_sequence_helper : merge_and_renumber<typename make_index_sequence_helper<N / 2>::type,
                                                       typename make_index_sequence_helper<N - N / 2>::type> {};

template<>
struct make_index_sequence_helper<0> {
    using type = index_sequence<>;
};

template<>
struct make_index_sequence_helper<1> {
    using type = index_sequence<0>;
};

} // namespace details

template<size_t N>
using make_index_sequence = typename details::make_index_sequence_helper<N>::type;

namespace details {

template<class T, size_t N, class Indices = make_index_sequence<N>>
struct make_integer_sequence_helper;

template<class T, size_t N, size_t... Indices>
struct make_integer_sequence_helper<T, N, index_sequence<Indices...>> {
    using type = integer_sequence<T, static_cast<T>(Indices)...>;
};

} // namespace details

template<class T, T N>
using make_integer_sequence = typename details::make_integer_sequence_helper<T, N>::type;

template<class... T>
using index_sequence_for = make_index_sequence<sizeof...(T)>;

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_INTEGER_SEQUENCE_HPP_
#ifndef CIELLAB_INCLUDE_CIEL_TYPE_TRAITS_HPP_
#define CIELLAB_INCLUDE_CIEL_TYPE_TRAITS_HPP_

#include <cstring>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>


NAMESPACE_CIEL_BEGIN

// void_t
template<class...>
using void_t = void;

// conjunction
// When the template pack is empty, derive from true_type.
template<class...>
struct conjunction : std::true_type {};

// Otherwise, derive from the first false template member (if all true, choose the last one).
template<class B1, class... Bn>
struct conjunction<B1, Bn...> : std::conditional<static_cast<bool>(B1::value), conjunction<Bn...>, B1>::type {};

// disjunction
template<class...>
struct disjunction : std::false_type {};

template<class B1, class... Bn>
struct disjunction<B1, Bn...> : std::conditional<static_cast<bool>(B1::value), B1, disjunction<Bn...>>::type {};

// is_exactly_input_iterator
template<class Iter, class = void>
struct is_exactly_input_iterator : std::false_type {};

template<class Iter>
struct is_exactly_input_iterator<Iter, void_t<typename std::iterator_traits<Iter>::iterator_category>>
    : std::is_same<typename std::iterator_traits<Iter>::iterator_category, std::input_iterator_tag> {};

// is_forward_iterator
template<class Iter, class = void>
struct is_forward_iterator : std::false_type {};

template<class Iter>
struct is_forward_iterator<Iter, void_t<typename std::iterator_traits<Iter>::iterator_category>>
    : std::is_convertible<typename std::iterator_traits<Iter>::iterator_category, std::forward_iterator_tag> {};

// is_input_iterator
template<class Iter, class = void>
struct is_input_iterator : std::false_type {};

template<class Iter>
struct is_input_iterator<Iter, void_t<typename std::iterator_traits<Iter>::iterator_category>>
    : std::is_convertible<typename std::iterator_traits<Iter>::iterator_category, std::input_iterator_tag> {};

// is_trivially_relocatable
template<class T, class = void>
struct is_trivially_relocatable : disjunction<std::is_empty<T>, std::is_trivially_copyable<T>> {};

#ifdef _LIBCPP___TYPE_TRAITS_IS_TRIVIALLY_RELOCATABLE_H
template<class T>
struct is_trivially_relocatable<T, typename std::enable_if<std::__libcpp_is_trivially_relocatable<T>::value>::type>
    : std::true_type {};
#else
template<class First, class Second>
struct is_trivially_relocatable<std::pair<First, Second>>
    : conjunction<is_trivially_relocatable<First>, is_trivially_relocatable<Second>> {};

template<class... Types>
struct is_trivially_relocatable<std::tuple<Types...>> : conjunction<is_trivially_relocatable<Types>...> {};
#endif

// useless_tag
struct useless_tag {
    useless_tag(...) noexcept {}
}; // struct useless_tag

// owner
template<class T, class = typename std::enable_if<std::is_pointer<T>::value>::type>
using owner = T;

// is_final
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

// is_const_lvalue_reference
template<class T>
struct is_const_lvalue_reference : std::false_type {};

template<class T>
struct is_const_lvalue_reference<const T&> : std::true_type {};

// is_const_rvalue_reference
template<class T>
struct is_const_rvalue_reference : std::false_type {};

template<class T>
struct is_const_rvalue_reference<const T&&> : std::true_type {};

// is_const_reference
template<class T>
struct is_const_reference
    : std::integral_constant<bool, is_const_lvalue_reference<T>::value || is_const_rvalue_reference<T>::value> {};

// worth_move
// FIXME: Current implementation returns true for const&& constructor and assignment.
template<class T>
struct worth_move {
    static_assert(!std::is_const<T>::value, "");

private:
    using U = typename std::decay<T>::type;

    struct helper {
        operator const U&() noexcept;
        operator U&&() noexcept;
    }; // struct helper

public:
    static constexpr bool construct = std::is_class<T>::value && !std::is_trivial<T>::value
                                   && std::is_move_constructible<T>::value && !std::is_constructible<T, helper>::value;
    static constexpr bool assign = std::is_class<T>::value && !std::is_trivial<T>::value
                                && std::is_move_assignable<T>::value && !std::is_assignable<T, helper>::value;
    static constexpr bool value = construct || assign;

}; // struct worth_move_constructing

template<class T>
struct worth_move_constructing {
    static constexpr bool value = worth_move<T>::construct;

}; // worth_move_constructing

template<class T>
struct worth_move_assigning {
    static constexpr bool value = worth_move<T>::assign;

}; // worth_move_assigning

#if CIEL_STD_VER >= 20
// is_complete_type
template<class T, auto = [] {}>
inline constexpr bool is_complete_type_v = requires { sizeof(T); };
#endif // CIEL_STD_VER >= 20

// aligned_storage
template<size_t size, size_t alignment>
struct aligned_storage {
    static_assert(sizeof(unsigned char) == 1, "");

    class type {
        alignas(alignment) unsigned char buffer_[size]{};
    };

}; // aligned_storage

// buffer_cast
template<class Pointer, typename std::enable_if<std::is_pointer<Pointer>::value, int>::type = 0>
CIEL_NODISCARD Pointer
buffer_cast(const void* ptr) noexcept {
    return static_cast<Pointer>(const_cast<void*>(ptr));
}

// exchange
template<class T, class U = T>
CIEL_NODISCARD T
exchange(T& obj, U&& new_value) noexcept(std::is_nothrow_move_constructible<T>::value
                                         && std::is_nothrow_assignable<T&, U>::value) {
    T old_value = std::move(obj);
    obj         = std::forward<U>(new_value);
    return old_value;
}

// Is a pointer aligned?
CIEL_NODISCARD inline bool
is_aligned(void* ptr, const size_t alignment) noexcept {
    CIEL_PRECONDITION(ptr != nullptr);
    CIEL_PRECONDITION(alignment != 0);

    return ((uintptr_t)ptr % alignment) == 0;
}

// Align upwards
CIEL_NODISCARD inline uintptr_t
align_up(uintptr_t sz, const size_t alignment) noexcept {
    CIEL_PRECONDITION(alignment != 0);

    const uintptr_t mask = alignment - 1;

    if CIEL_LIKELY ((alignment & mask) == 0) { // power of two?
        return (sz + mask) & ~mask;

    } else {
        return ((sz + mask) / alignment) * alignment;
    }
}

// Align downwards
CIEL_NODISCARD inline uintptr_t
align_down(uintptr_t sz, const size_t alignment) noexcept {
    CIEL_PRECONDITION(alignment != 0);

    uintptr_t mask = alignment - 1;

    if CIEL_LIKELY ((alignment & mask) == 0) { // power of two?
        return (sz & ~mask);

    } else {
        return ((sz / alignment) * alignment);
    }
}

// sizeof_without_tail_padding
//
// Derived can reuse Base's tail padding.
// e.g.
// struct Base {
//     alignas(8) unsigned char buf[1]{};
// };
// struct Derived : Base {
//     int i{};
// };
// static_assert(sizeof(Base)    == 8, "");
// static_assert(sizeof(Derived) == 8, "");
//
template<class T, size_t TailPadding = alignof(T)>
struct sizeof_without_tail_padding {
    static_assert(std::is_class<T>::value && !is_final<T>::value, "");

    struct S : T {
        unsigned char buf[TailPadding]{};
    };

    using type = typename std::conditional<sizeof(S) == sizeof(T), sizeof_without_tail_padding,
                                           typename sizeof_without_tail_padding<T, TailPadding - 1>::type>::type;

    static constexpr size_t Byte = TailPadding;

    static constexpr size_t value = sizeof(T) - type::Byte;

}; // struct sizeof_without_tail_padding

template<class T>
struct sizeof_without_tail_padding<T, 0> {
    static_assert(std::is_class<T>::value && !is_final<T>::value, "");

    using type = sizeof_without_tail_padding;

    static constexpr size_t Byte = 0;

    static constexpr size_t value = sizeof(T);

}; // struct sizeof_without_tail_padding<T, 0>

// is_overaligned_for_new
CIEL_NODISCARD inline bool
is_overaligned_for_new(const size_t alignment) noexcept {
#ifdef __STDCPP_DEFAULT_NEW_ALIGNMENT__
    return alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__;
#else
    return alignment > alignof(std::max_align_t);
#endif
}

// allocate
template<class T>
CIEL_NODISCARD T*
allocate(const size_t n) {
#if CIEL_STD_VER >= 17
    if CIEL_UNLIKELY (ciel::is_overaligned_for_new(alignof(T))) {
        return static_cast<T*>(::operator new(sizeof(T) * n, static_cast<std::align_val_t>(alignof(T))));
    }
#endif
    return static_cast<T*>(::operator new(sizeof(T) * n));
}

// deallocate
template<class T>
void
deallocate(T* ptr) noexcept {
#if CIEL_STD_VER >= 17
    if CIEL_UNLIKELY (ciel::is_overaligned_for_new(alignof(T))) {
        ::operator delete(ptr, static_cast<std::align_val_t>(alignof(T)));
    }
#endif
    ::operator delete(ptr);
}

// relocatable_swap
template<class T, bool Valid = is_trivially_relocatable<T>::value>
void
relocatable_swap(T& lhs, T& rhs) noexcept {
    static_assert(Valid, "T must be trivially relocatable, you can explicitly assume it.");

    typename aligned_storage<sizeof(T), alignof(T)>::type buffer;

    std::memcpy(&buffer, &rhs, sizeof(T));
    std::memcpy(&rhs, &lhs, sizeof(T));
    std::memcpy(&lhs, &buffer, sizeof(T));
}

// is_range
template<class T, class = void>
struct is_range : std::false_type {};

template<class T>
struct is_range<T, void_t<decltype(std::declval<T>().begin(), std::declval<T>().end())>> : std::true_type {};

template<class T, class = void>
struct is_range_with_size : std::false_type {};

template<class T>
struct is_range_with_size<
    T, void_t<decltype(std::declval<T>().begin(), std::declval<T>().end(), std::declval<T>().size())>>
    : std::true_type {};

// compare
template<class T, class U,
         typename std::enable_if<is_range_with_size<T>::value && is_range_with_size<U>::value, int>::type = 0>
CIEL_NODISCARD bool
operator==(const T& lhs, const U& rhs) noexcept {
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template<class T, class U, typename std::enable_if<is_range<T>::value && is_range<U>::value, int>::type = 0>
CIEL_NODISCARD bool
operator<(const T& lhs, const U& rhs) noexcept {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template<class T, class U>
CIEL_NODISCARD bool
operator!=(const T& lhs, const U& rhs) noexcept {
    return !(lhs == rhs);
}

template<class T, class U>
CIEL_NODISCARD bool
operator>(const T& lhs, const U& rhs) noexcept {
    return rhs < lhs;
}

template<class T, class U>
CIEL_NODISCARD bool
operator<=(const T& lhs, const U& rhs) noexcept {
    return !(rhs < lhs);
}

template<class T, class U>
CIEL_NODISCARD bool
operator>=(const T& lhs, const U& rhs) noexcept {
    return !(lhs < rhs);
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TYPE_TRAITS_HPP_

NAMESPACE_CIEL_BEGIN

struct default_init_t {};

constexpr default_init_t default_init_tag;

struct value_init_t {};

constexpr value_init_t value_init_tag;

template<class T, size_t Index, bool = std::is_class<T>::value && !is_final<T>::value>
struct compressed_pair_elem {
public:
    using reference       = T&;
    using const_reference = const T&;

private:
    T value_;

public:
    explicit compressed_pair_elem(default_init_t) {}

    explicit compressed_pair_elem(value_init_t)
        : value_() {}

    template<class U, typename std::enable_if<!std::is_same<compressed_pair_elem, typename std::decay<U>::type>::value,
                                              int>::type
                      = 0>
    explicit compressed_pair_elem(U&& u)
        : value_(std::forward<U>(u)) {}

    template<class... Args, size_t... Ints>
    explicit compressed_pair_elem(std::piecewise_construct_t, std::tuple<Args...> args, index_sequence<Ints...>)
        : value_(std::forward<Args>(std::get<Ints>(args))...) {}

    reference
    get() noexcept {
        return value_;
    }

    const_reference
    get() const noexcept {
        return value_;
    }

}; // struct compressed_pair_elem

template<class T, size_t Index>
struct compressed_pair_elem<T, Index, true> : private T {
public:
    using reference       = T&;
    using const_reference = const T&;
    using value_          = T;

public:
    explicit compressed_pair_elem(default_init_t) {}

    explicit compressed_pair_elem(value_init_t)
        : value_() {}

    template<class U, typename std::enable_if<!std::is_same<compressed_pair_elem, typename std::decay<U>::type>::value,
                                              int>::type
                      = 0>
    explicit compressed_pair_elem(U&& u)
        : value_(std::forward<U>(u)) {}

    template<class... Args, size_t... Ints>
    explicit compressed_pair_elem(std::piecewise_construct_t, std::tuple<Args...> args, index_sequence<Ints...>)
        : value_(std::forward<Args>(std::get<Ints>(args))...) {}

    reference
    get() noexcept {
        return *this;
    }

    const_reference
    get() const noexcept {
        return *this;
    }

}; // struct compressed_pair_elem<T, Index, true>

template<class T1, class T2>
class compressed_pair : private compressed_pair_elem<T1, 0>,
                        private compressed_pair_elem<T2, 1> {
    static_assert(!std::is_same<T1, T2>::value, "");

    using base1 = compressed_pair_elem<T1, 0>;
    using base2 = compressed_pair_elem<T2, 1>;

public:
    template<class U1 = T1, class U2 = T2,
             typename std::enable_if<
                 std::is_default_constructible<U1>::value && std::is_default_constructible<U2>::value, int>::type
             = 0>
    explicit compressed_pair()
        : base1(value_init_tag), base2(value_init_tag) {}

    template<class U1, class U2>
    explicit compressed_pair(U1&& u1, U2&& u2)
        : base1(std::forward<U1>(u1)), base2(std::forward<U2>(u2)) {}

    template<class... Args1, class... Args2>
    explicit compressed_pair(std::piecewise_construct_t pc, std::tuple<Args1...> first_args,
                             std::tuple<Args2...> second_args)
        : base1(pc, std::move(first_args), index_sequence_for<Args1...>()),
          base2(pc, std::move(second_args), index_sequence_for<Args2...>()) {}

    typename base1::reference
    first() noexcept {
        return static_cast<base1&>(*this).get();
    }

    typename base1::const_reference
    first() const noexcept {
        return static_cast<const base1&>(*this).get();
    }

    typename base2::reference
    second() noexcept {
        return static_cast<base2&>(*this).get();
    }

    typename base2::const_reference
    second() const noexcept {
        return static_cast<const base2&>(*this).get();
    }

    // TODO: Since std::is_nothrow_swappable is available in C++17...
    void
    swap(compressed_pair& other) noexcept {
        using std::swap;

        swap(first(), other.first());
        swap(second(), other.second());
    }

}; // class compressed_pair

template<class First, class Second>
struct is_trivially_relocatable<compressed_pair<First, Second>>
    : conjunction<is_trivially_relocatable<First>, is_trivially_relocatable<Second>> {};

NAMESPACE_CIEL_END

namespace std {

template<class T1, class T2>
void
swap(ciel::compressed_pair<T1, T2>& lhs, ciel::compressed_pair<T1, T2>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_COMPRESSED_PAIR_HPP_
#ifndef CIELLAB_INCLUDE_CIEL_MOVE_PROXY_HPP_
#define CIELLAB_INCLUDE_CIEL_MOVE_PROXY_HPP_

#include <type_traits>
#include <utility>


NAMESPACE_CIEL_BEGIN

template<class T>
class move_proxy {
public:
    template<class... Args, typename std::enable_if<std::is_constructible<T, Args&&...>::value, int>::type = 0>
    move_proxy(Args&&... args) noexcept(std::is_nothrow_constructible<T, Args&&...>::value)
        : data_(std::forward<Args>(args)...) {}

    template<class U, class... Args,
             typename std::enable_if<std::is_constructible<T, std::initializer_list<U>, Args&&...>::value, int>::type
             = 0>
    move_proxy(std::initializer_list<U> il,
               Args&&... args) noexcept(std::is_nothrow_constructible<T, std::initializer_list<U>, Args&&...>::value)
        : data_(il, std::forward<Args>(args)...) {}

    operator T&&() const noexcept {
        return std::move(data_);
    }

private:
    mutable T data_;

}; // class move_proxy

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_MOVE_PROXY_HPP_

NAMESPACE_CIEL_BEGIN

// Differences between std::list and this class:
// 1. It keeps hold of allocations to avoid repeated heap allocations and frees
//    when frequently inserting and removing elements.

struct list_node_base {
    list_node_base* prev_;
    list_node_base* next_;

    list_node_base() noexcept
        : prev_(this), next_(this) {}

    list_node_base(list_node_base* p, list_node_base* n) noexcept
        : prev_(p), next_(n) {}

    void
    clear() noexcept {
        prev_ = this;
        next_ = this;
    }

}; // struct list_node_base

template<class T>
struct list_node : list_node_base {
    T value_;

    template<class... Args>
    list_node(list_node_base* p, list_node_base* n, Args&&... args)
        : list_node_base(p, n), value_(std::forward<Args>(args)...) {}

}; // struct list_node

template<class T, class Pointer, class Reference>
class list_iterator {
public:
    using difference_type   = ptrdiff_t;
    using value_type        = T;
    using pointer           = Pointer;
    using reference         = Reference;
    using iterator_category = std::bidirectional_iterator_tag;
    using iterator_concept  = std::bidirectional_iterator_tag;

private:
    using base_node_type = list_node_base;
    using node_type      = list_node<value_type>;

    base_node_type* it_;

public:
    list_iterator() noexcept
        : it_(nullptr) {}

    explicit list_iterator(const base_node_type* p) noexcept
        : it_(const_cast<base_node_type*>(p)) {}

    list_iterator(const list_iterator&) noexcept = default;
    list_iterator(list_iterator&&) noexcept      = default;

    template<class P, class R>
    list_iterator(const list_iterator<T, P, R>& other) noexcept
        : it_(const_cast<base_node_type*>(other.base())) {}

    ~list_iterator() = default;

    list_iterator&
    operator=(const list_iterator&) noexcept
        = default;
    list_iterator&
    operator=(list_iterator&&) noexcept
        = default;

    CIEL_NODISCARD list_iterator
    next() const noexcept {
        return list_iterator(it_->next_);
    }

    CIEL_NODISCARD list_iterator
    prev() const noexcept {
        return list_iterator(it_->prev_);
    }

    CIEL_NODISCARD reference
    operator*() const noexcept {
        return static_cast<node_type*>(it_)->value_;
    }

    CIEL_NODISCARD pointer
    operator->() const noexcept {
        return &static_cast<node_type*>(it_)->value_;
    }

    list_iterator&
    operator++() noexcept {
        it_ = it_->next_;
        return *this;
    }

    CIEL_NODISCARD list_iterator
    operator++(int) noexcept {
        list_iterator res(it_);
        ++(*this);
        return res;
    }

    list_iterator&
    operator--() noexcept {
        it_ = it_->prev_;
        return *this;
    }

    CIEL_NODISCARD list_iterator
    operator--(int) noexcept {
        list_iterator res(it_);
        --(*this);
        return res;
    }

    CIEL_NODISCARD base_node_type*
    base() const noexcept {
        return it_;
    }

    CIEL_NODISCARD explicit
    operator bool() const noexcept {
        return it_ != nullptr;
    }

}; // class list_iterator

template<class T, class Pointer1, class Pointer2, class Reference1, class Reference2>
CIEL_NODISCARD bool
operator==(const list_iterator<T, Pointer1, Reference1>& lhs,
           const list_iterator<T, Pointer2, Reference2>& rhs) noexcept {
    return lhs.base() == rhs.base();
}

template<class T, class Allocator = std::allocator<T>>
class list {
public:
    using value_type             = T;
    using allocator_type         = Allocator;
    using size_type              = size_t;
    using difference_type        = ptrdiff_t;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using pointer                = typename std::allocator_traits<allocator_type>::pointer;
    using const_pointer          = typename std::allocator_traits<allocator_type>::const_pointer;
    using iterator               = list_iterator<value_type, pointer, reference>;
    using const_iterator         = list_iterator<value_type, const_pointer, const_reference>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    using base_node_type    = list_node_base;
    using node_type         = list_node<value_type>;
    using alloc_traits      = std::allocator_traits<allocator_type>;
    using node_allocator    = typename alloc_traits::template rebind_alloc<node_type>;
    using node_alloc_traits = typename alloc_traits::template rebind_traits<node_type>;

    base_node_type end_node_;
    node_type* free_node_;
    compressed_pair<size_type, node_allocator> size_node_allocator_;

    void
    do_destroy() noexcept {
        iterator it = begin();
        iterator e  = end();

        while (it != e) {
            auto* to_be_destroyed = static_cast<node_type*>(it.base());
            ++it;
            node_alloc_traits::destroy(allocator_(), to_be_destroyed);
            node_alloc_traits::deallocate(allocator_(), to_be_destroyed, 1);
        }

        while (free_node_) {
            auto next = static_cast<node_type*>(free_node_->next_);
            node_alloc_traits::deallocate(allocator_(), free_node_, 1);
            free_node_ = next;
        }
    }

    node_type*
    get_one_free_node() {
        if (free_node_) {
            node_type* res = free_node_;
            free_node_     = static_cast<node_type*>(free_node_->next_);

            return res;
        }

        return node_alloc_traits::allocate(allocator_(), 1);
    }

    void
    store_one_free_node(node_type* free) noexcept {
        free->next_ = free_node_;
        free_node_  = free;
    }

    size_type&
    size_() noexcept {
        return size_node_allocator_.first();
    }

    const size_type&
    size_() const noexcept {
        return size_node_allocator_.first();
    }

    node_allocator&
    allocator_() noexcept {
        return size_node_allocator_.second();
    }

    const node_allocator&
    allocator_() const noexcept {
        return size_node_allocator_.second();
    }

    iterator
    alloc_range_destroy(iterator begin, iterator end) noexcept {
        iterator loop         = begin;
        iterator before_begin = begin.prev();

        while (loop != end) {
            auto* to_be_destroyed = static_cast<node_type*>(loop.base());
            ++loop;

            node_alloc_traits::destroy(allocator_(), to_be_destroyed);
            --size_();
            store_one_free_node(to_be_destroyed);
        }

        before_begin.base()->next_ = end.base();
        end.base()->prev_          = before_begin.base();

        return end;
    }

    // insert before begin
    template<class... Arg>
    iterator
    alloc_range_construct_n(iterator begin, const size_type n, Arg&&... arg) {
        iterator before_begin          = begin.prev();
        iterator original_before_begin = before_begin;

        CIEL_TRY {
            for (size_type i = 0; i < n; ++i) {
                node_type* construct_place = get_one_free_node();

                CIEL_TRY {
                    node_alloc_traits::construct(allocator_(), construct_place, before_begin.base(), begin.base(),
                                                 std::forward<Arg>(arg)...);
                    ++size_();

                    before_begin.base()->next_ = construct_place;
                    begin.base()->prev_        = construct_place;
                    ++before_begin;
                }
                CIEL_CATCH (...) {
                    store_one_free_node(construct_place);
                    CIEL_THROW;
                }
            }
            return original_before_begin.next();
        }
        CIEL_CATCH (...) {
            alloc_range_destroy(original_before_begin.next(), begin);
            CIEL_THROW;
        }
    }

    template<class Iter, typename std::enable_if<is_input_iterator<Iter>::value, int>::type = 0>
    iterator
    alloc_range_construct(iterator begin, Iter first, Iter last) {
        iterator before_begin          = begin.prev();
        iterator original_before_begin = before_begin;

        CIEL_TRY {
            while (first != last) {
                node_type* construct_place = get_one_free_node();

                CIEL_TRY {
                    node_alloc_traits::construct(allocator_(), construct_place, before_begin.base(), begin.base(),
                                                 *first);
                    ++size_();
                    ++first;

                    before_begin.base()->next_ = construct_place;
                    begin.base()->prev_        = construct_place;
                    ++before_begin;
                }
                CIEL_CATCH (...) {
                    store_one_free_node(construct_place);
                    CIEL_THROW;
                }
            }
            return original_before_begin.next();
        }
        CIEL_CATCH (...) {
            alloc_range_destroy(original_before_begin.next(), begin);
            CIEL_THROW;
        }
    }

public:
    list()
        : free_node_(nullptr), size_node_allocator_(0, default_init_tag) {}

    explicit list(const allocator_type& alloc)
        : free_node_(nullptr), size_node_allocator_(0, alloc) {}

    list(const size_type count, const value_type& value, const allocator_type& alloc = allocator_type())
        : list(alloc) {
        alloc_range_construct_n(end(), count, value);
    }

    explicit list(const size_type count, const allocator_type& alloc = allocator_type())
        : list(alloc) {
        alloc_range_construct_n(end(), count);
    }

    template<class Iter, typename std::enable_if<is_input_iterator<Iter>::value, int>::type = 0>
    list(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : list(alloc) {
        alloc_range_construct(end(), first, last);
    }

    list(const list& other)
        : list(other.begin(), other.end(), alloc_traits::select_on_container_copy_construction(other.get_allocator())) {
    }

    list(const list& other, const allocator_type& alloc)
        : list(other.begin(), other.end(), alloc) {}

    list(list&& other) noexcept
        : end_node_(other.end_node_),
          free_node_(other.free_node_),
          size_node_allocator_(other.size_(), std::move(other.allocator_())) {
        end_node_.next_->prev_ = &end_node_;
        end_node_.prev_->next_ = &end_node_;

        other.end_node_.clear();
        other.free_node_ = nullptr;
        other.size_()    = 0;
    }

    list(list&& other, const allocator_type& alloc)
        : list() {
        if (alloc == other.get_allocator()) {
            end_node_    = other.end_node_;
            free_node_   = other.free_node_;
            size_()      = other.size_();
            allocator_() = alloc;

            end_node_.next_->prev_ = &end_node_;
            end_node_.prev_->next_ = &end_node_;

            other.end_node_.clear();
            other.free_node_ = nullptr;
            other.size_()    = 0;

        } else {
            list(other, alloc).swap(*this);
        }
    }

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    list(InitializerList init, const allocator_type& alloc = allocator_type())
        : list(init.begin(), init.end(), alloc) {}

    template<class U = value_type, typename std::enable_if<worth_move_constructing<U>::value, int>::type = 0>
    list(std::initializer_list<move_proxy<value_type>> init, const allocator_type& alloc = allocator_type())
        : list(init.begin(), init.end(), alloc) {}

    template<class U = value_type, typename std::enable_if<!worth_move_constructing<U>::value, int>::type = 0>
    list(std::initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
        : list(init.begin(), init.end(), alloc) {}

    ~list() {
        do_destroy();
    }

    list&
    operator=(const list& other) {
        if CIEL_UNLIKELY (this == addressof(other)) {
            return *this;
        }

        if (alloc_traits::propagate_on_container_copy_assignment::value) {
            if (allocator_() != other.allocator_()) {
                list(other.allocator_()).swap(*this);
                assign(other.begin(), other.end());
                return *this;
            }

            allocator_() = other.allocator_();
        }

        assign(other.begin(), other.end());

        return *this;
    }

    list&
    operator=(list&& other) noexcept(alloc_traits::is_always_equal::value) {
        if CIEL_UNLIKELY (this == addressof(other)) {
            return *this;
        }

        if (!alloc_traits::propagate_on_container_move_assignment::value && allocator_() != other.allocator_()) {
            assign(other.begin(), other.end());
            return *this;
        }

        if (alloc_traits::propagate_on_container_move_assignment::value) {
            allocator_() = std::move(other.allocator_());
        }

        do_destroy();

        end_node_  = other.end_node_;
        free_node_ = other.free_node_;
        size_()    = other.size_();

        end_node_.next_->prev_ = &end_node_;
        end_node_.prev_->next_ = &end_node_;

        other.end_node_.clear();
        other.free_node_ = nullptr;
        other.size_()    = 0;

        return *this;
    }

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    list&
    operator=(InitializerList ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    template<class U = value_type, typename std::enable_if<ciel::worth_move<U>::value, int>::type = 0>
    list&
    operator=(std::initializer_list<move_proxy<value_type>> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    template<class U = value_type, typename std::enable_if<!ciel::worth_move<U>::value, int>::type = 0>
    list&
    operator=(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    void
    assign(size_type count, const value_type& value) {
        iterator it = begin();
        iterator e  = end();

        for (; count > 0 && it != e; --count, ++it) {
            *it = value;
        }

        if (it == e) {
            insert(e, count, value);

        } else {
            erase(it, e);
        }
    }

    template<class Iter, typename std::enable_if<is_input_iterator<Iter>::value, int>::type = 0>
    void
    assign(Iter first, Iter last) {
        iterator it = begin();
        iterator e  = end();

        for (; first != last && it != e; ++first, ++it) {
            *it = *first;
        }

        if (it == e) {
            insert(e, std::move(first), std::move(last));

        } else {
            erase(it, e);
        }
    }

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    void
    assign(InitializerList ilist) {
        assign(ilist.begin(), ilist.end());
    }

    template<class U = value_type, typename std::enable_if<ciel::worth_move<U>::value, int>::type = 0>
    void
    assign(std::initializer_list<move_proxy<value_type>> ilist) {
        assign(ilist.begin(), ilist.end());
    }

    template<class U = value_type, typename std::enable_if<!ciel::worth_move<U>::value, int>::type = 0>
    void
    assign(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end());
    }

    CIEL_NODISCARD allocator_type
    get_allocator() const noexcept {
        return allocator_();
    }

    CIEL_NODISCARD reference
    front() {
        CIEL_PRECONDITION(!empty());

        return *begin();
    }

    CIEL_NODISCARD const_reference
    front() const {
        CIEL_PRECONDITION(!empty());

        return *begin();
    }

    CIEL_NODISCARD reference
    back() {
        CIEL_PRECONDITION(!empty());

        return *(--end());
    }

    CIEL_NODISCARD const_reference
    back() const {
        CIEL_PRECONDITION(!empty());

        return *(--end());
    }

    CIEL_NODISCARD iterator
    begin() noexcept {
        return iterator(end_node_.next_);
    }

    CIEL_NODISCARD const_iterator
    begin() const noexcept {
        return const_iterator(end_node_.next_);
    }

    CIEL_NODISCARD const_iterator
    cbegin() const noexcept {
        return begin();
    }

    CIEL_NODISCARD iterator
    end() noexcept {
        return iterator(&end_node_);
    }

    CIEL_NODISCARD const_iterator
    end() const noexcept {
        return const_iterator(&end_node_);
    }

    CIEL_NODISCARD const_iterator
    cend() const noexcept {
        return end();
    }

    CIEL_NODISCARD reverse_iterator
    rbegin() noexcept {
        return reverse_iterator(end());
    }

    CIEL_NODISCARD const_reverse_iterator
    rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    CIEL_NODISCARD const_reverse_iterator
    crbegin() const noexcept {
        return rbegin();
    }

    CIEL_NODISCARD reverse_iterator
    rend() noexcept {
        return reverse_iterator(begin());
    }

    CIEL_NODISCARD const_reverse_iterator
    rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    CIEL_NODISCARD const_reverse_iterator
    crend() const noexcept {
        return rend();
    }

    CIEL_NODISCARD bool
    empty() const noexcept {
        return size_() == 0;
    }

    CIEL_NODISCARD size_type
    size() const noexcept {
        return size_();
    }

    CIEL_NODISCARD size_type
    max_size() const noexcept {
        return node_alloc_traits::max_size(allocator_());
    }

    void
    clear() noexcept {
        alloc_range_destroy(begin(), end());
    }

    iterator
    insert(iterator pos, const T& value) {
        return alloc_range_construct_n(pos, 1, value);
    }

    iterator
    insert(iterator pos, T&& value) {
        return alloc_range_construct_n(pos, 1, std::move(value));
    }

    iterator
    insert(iterator pos, const size_type count, const T& value) {
        return alloc_range_construct_n(pos, count, value);
    }

    template<class Iter, typename std::enable_if<is_input_iterator<Iter>::value, int>::type = 0>
    iterator
    insert(iterator pos, Iter first, Iter last) {
        return alloc_range_construct(pos, first, last);
    }

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    iterator
    insert(iterator pos, InitializerList ilist) {
        return alloc_range_construct(pos, ilist.begin(), ilist.end());
    }

    template<class U = value_type, typename std::enable_if<ciel::worth_move_constructing<U>::value, int>::type = 0>
    iterator
    insert(iterator pos, std::initializer_list<move_proxy<value_type>> ilist) {
        return alloc_range_construct(pos, ilist.begin(), ilist.end());
    }

    template<class U = value_type, typename std::enable_if<!ciel::worth_move_constructing<U>::value, int>::type = 0>
    iterator
    insert(iterator pos, std::initializer_list<value_type> ilist) {
        return alloc_range_construct(pos, ilist.begin(), ilist.end());
    }

    template<class... Args>
    iterator
    emplace(iterator pos, Args&&... args) {
        return alloc_range_construct_n(pos, 1, std::forward<Args>(args)...);
    }

    iterator
    erase(iterator pos) {
        return alloc_range_destroy(pos, pos.next());
    }

    iterator
    erase(iterator first, iterator last) {
        return alloc_range_destroy(first, last);
    }

    void
    push_back(const value_type& value) {
        emplace_back(value);
    }

    void
    push_back(value_type&& value) {
        emplace_back(std::move(value));
    }

    template<class... Args>
    reference
    emplace_back(Args&&... args) {
        return *alloc_range_construct_n(end(), 1, std::forward<Args>(args)...);
    }

    void
    pop_back() noexcept {
        CIEL_PRECONDITION(!empty());

        alloc_range_destroy(end().prev(), end());
    }

    void
    push_front(const value_type& value) {
        emplace_front(value);
    }

    void
    push_front(value_type&& value) {
        emplace_front(std::move(value));
    }

    template<class... Args>
    reference
    emplace_front(Args&&... args) {
        return *alloc_range_construct_n(begin(), 1, std::forward<Args>(args)...);
    }

    void
    pop_front() noexcept {
        CIEL_PRECONDITION(!empty());

        alloc_range_destroy(begin(), begin().next());
    }

    void
    resize(const size_type count) {
        if (size() >= count) {
            iterator tmp = std::prev(end(), size() - count);
            alloc_range_destroy(tmp, end());

        } else {
            alloc_range_construct_n(end(), count - size());
        }
    }

    void
    resize(const size_type count, const value_type& value) {
        if (size() >= count) {
            iterator tmp = std::prev(end(), size() - count);
            alloc_range_destroy(tmp, end());

        } else {
            alloc_range_construct_n(end(), count - size(), value);
        }
    }

    void
    swap(list& other) noexcept(alloc_traits::is_always_equal::value) {
        using std::swap;

        swap(end_node_, other.end_node_);

        end_node_.next_->prev_ = &end_node_;
        end_node_.prev_->next_ = &end_node_;

        other.end_node_.next_->prev_ = &other.end_node_;
        other.end_node_.prev_->next_ = &other.end_node_;

        swap(free_node_, other.free_node_);
        swap(size_node_allocator_, other.size_node_allocator_);
    }

}; // class list

template<class T, class Allocator>
struct is_trivially_relocatable<list<T, Allocator>> : std::false_type {};

#if CIEL_STD_VER >= 17

template<class Iter, class Alloc = std::allocator<typename std::iterator_traits<Iter>::value_type>>
list(Iter, Iter, Alloc = Alloc()) -> list<typename std::iterator_traits<Iter>::value_type, Alloc>;

#endif

NAMESPACE_CIEL_END

namespace std {

template<class T, class Alloc>
void
swap(ciel::list<T, Alloc>& lhs, ciel::list<T, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_LIST_HPP_

#ifndef CIELLAB_INCLUDE_CIEL_SPLIT_BUFFER_HPP_
#define CIELLAB_INCLUDE_CIEL_SPLIT_BUFFER_HPP_

#include <algorithm>
#include <cstddef>
#include <cstring>
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

// sizeof_without_back_padding
//
// Derived can reuse Base's back padding.
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
template<class T, size_t BackPadding = alignof(T)>
struct sizeof_without_back_padding {
    static_assert(std::is_class<T>::value && !is_final<T>::value, "");

    struct S : T {
        unsigned char buf[BackPadding]{};
    };

    using type = typename std::conditional<sizeof(S) == sizeof(T), sizeof_without_back_padding,
                                           typename sizeof_without_back_padding<T, BackPadding - 1>::type>::type;

    static constexpr size_t Byte = BackPadding;

    static constexpr size_t value = sizeof(T) - type::Byte;

}; // struct sizeof_without_back_padding

template<class T>
struct sizeof_without_back_padding<T, 0> {
    static_assert(std::is_class<T>::value && !is_final<T>::value, "");

    using type = sizeof_without_back_padding;

    static constexpr size_t Byte = 0;

    static constexpr size_t value = sizeof(T);

}; // struct sizeof_without_back_padding<T, 0>

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

template<class T, bool Valid = is_trivially_relocatable<T>::value>
void
relocatable_swap(T& lhs, T& rhs) noexcept {
    static_assert(Valid, "T must be trivially relocatable, you can explicitly assume it.");

    typename aligned_storage<sizeof(T), alignof(T)>::type buffer;

    std::memcpy(&buffer, &rhs, sizeof(T));
    std::memcpy(&rhs, &lhs, sizeof(T));
    std::memcpy(&lhs, &buffer, sizeof(T));
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

// This class is used in std::vector implementation and it's like a double-ended vector.
// When std::vector inserts beyond its capacity, it defines a temp split_buffer to store insertions
// and push vector's elements into two sides, and swap out at last,
// so that it can keep basic exception safety.
// We complete its functionality(except for insert and erase) so that it can be used as a normal container.
// When pushing elements and there is no space this side, we try to shift to other side if there is plenty of space,
// or just expand.
// When it comes to expansion, we try to move old elements to the middle of new space
// and leave some free space at both sides.

template<class, class>
class vector;
template<class, size_t, class>
class small_vector;

// Note that Allocator can be reference type as being used by vector,
// however in this case, the assignment operator of split_buffer may be invalid.
template<class T, class Allocator = std::allocator<T>>
class split_buffer {
    static_assert(!std::is_rvalue_reference<Allocator>::value, "");

public:
    using value_type             = T;
    using allocator_type         = typename std::remove_reference<Allocator>::type;
    using size_type              = size_t;
    using difference_type        = ptrdiff_t;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using pointer                = typename std::allocator_traits<allocator_type>::pointer;
    using const_pointer          = typename std::allocator_traits<allocator_type>::const_pointer;
    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static_assert(std::is_same<typename allocator_type::value_type, T>::value, "");

private:
    using alloc_traits = std::allocator_traits<allocator_type>;

    pointer begin_cap_{nullptr};
    pointer begin_{nullptr};
    pointer end_{nullptr};
    compressed_pair<Allocator, pointer> end_cap_alloc_{default_init_tag, nullptr};

    template<class, class>
    friend class split_buffer;
    template<class, class>
    friend class vector;
    template<class, size_t, class>
    friend class small_vector;

    void
    reserve_cap_and_offset_to(const size_type cap, const size_type offset) {
        CIEL_PRECONDITION(begin_cap_ == nullptr);
        CIEL_PRECONDITION(cap != 0);
        CIEL_PRECONDITION(cap >= offset);

        begin_cap_ = alloc_traits::allocate(allocator_(), cap);
        end_cap_() = begin_cap_ + cap;
        begin_     = begin_cap_ + offset;
        end_       = begin_;
    }

    pointer&
    end_cap_() noexcept {
        return end_cap_alloc_.second();
    }

    const pointer&
    end_cap_() const noexcept {
        return end_cap_alloc_.second();
    }

    allocator_type&
    allocator_() noexcept {
        return end_cap_alloc_.first();
    }

    const allocator_type&
    allocator_() const noexcept {
        return end_cap_alloc_.first();
    }

    size_type
    recommend_cap(const size_type new_size) const {
        CIEL_PRECONDITION(new_size > 0);

        const size_type ms = max_size();

        if CIEL_UNLIKELY (new_size > ms) {
            ciel::throw_exception(std::length_error("ciel::split_buffer reserving size is beyond max_size"));
        }

        const size_type cap = capacity();

        if CIEL_UNLIKELY (cap >= ms / 2) {
            return ms;
        }

        return std::max(cap * 2, new_size);
    }

    void
    construct_at_end(const size_type n) {
        CIEL_PRECONDITION(end_ + n <= end_cap_());

        for (size_type i = 0; i < n; ++i) {
            unchecked_emplace_back();
        }
    }

    void
    construct_at_end(const size_type n, const value_type& value) {
        CIEL_PRECONDITION(end_ + n <= end_cap_());

        for (size_type i = 0; i < n; ++i) {
            unchecked_emplace_back(value);
        }
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    void
    construct_at_end(Iter first, Iter last) {
        CIEL_PRECONDITION(end_ + std::distance(first, last) <= end_cap_());

        while (first != last) {
            unchecked_emplace_back(*first);
            ++first;
        }
    }

    template<class U = value_type, typename std::enable_if<std::is_trivially_destructible<U>::value, int>::type = 0>
    pointer
    alloc_range_destroy(pointer begin, pointer end) noexcept {
        CIEL_PRECONDITION(begin <= end);

        return begin;
    }

    template<class U = value_type, typename std::enable_if<!std::is_trivially_destructible<U>::value, int>::type = 0>
    pointer
    alloc_range_destroy(pointer begin, pointer end) noexcept {
        CIEL_PRECONDITION(begin <= end);

        while (end != begin) {
            alloc_traits::destroy(allocator_(), --end);
        }

        return begin;
    }

    template<class U = value_type, typename std::enable_if<is_trivially_relocatable<U>::value, int>::type = 0>
    void
    swap_out_buffer(split_buffer<value_type, allocator_type&>&& sb, pointer pos) noexcept {
        // Used by emplace_front and emplace_back respectively.
        CIEL_PRECONDITION(pos == begin_ || pos == end_);

        // If either dest or src is an invalid or null pointer, memcpy's behavior is undefined, even if count is zero.
        if (begin_cap_) {
            const size_type front_count = pos - begin_;
            const size_type back_count  = end_ - pos;

            CIEL_PRECONDITION(sb.front_spare() >= front_count);
            CIEL_PRECONDITION(sb.back_spare() >= back_count);

            sb.begin_ -= front_count;
            memcpy(sb.begin_, begin_, sizeof(value_type) / sizeof(unsigned char) * front_count);

            memcpy(sb.end_, pos, sizeof(value_type) / sizeof(unsigned char) * back_count);
            sb.end_ += back_count;

            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
        }

        begin_cap_ = sb.begin_cap_;
        begin_     = sb.begin_;
        end_       = sb.end_;
        end_cap_() = sb.end_cap_();

        sb.set_nullptr();
    }

    template<class U = value_type, typename std::enable_if<!is_trivially_relocatable<U>::value, int>::type = 0>
    void
    swap_out_buffer(split_buffer<value_type, allocator_type&>&& sb,
                    pointer pos) noexcept(std::is_nothrow_move_constructible<value_type>::value) {
        // Used by emplace_front and emplace_back respectively.
        CIEL_PRECONDITION(pos == begin_ || pos == end_);

        if (begin_cap_) {
            const size_type front_count = pos - begin_;
            const size_type back_count  = end_ - pos;

            CIEL_PRECONDITION(sb.front_spare() >= front_count);
            CIEL_PRECONDITION(sb.back_spare() >= back_count);

            for (pointer p = pos - 1; p >= begin_; --p) {
#ifdef CIEL_HAS_EXCEPTIONS
                sb.unchecked_emplace_front(std::move_if_noexcept(*p));
#else
                sb.unchecked_emplace_front(std::move(*p));
#endif
            }

            for (pointer p = pos; p < end_; ++p) {
#ifdef CIEL_HAS_EXCEPTIONS
                sb.unchecked_emplace_back(std::move_if_noexcept(*p));
#else
                sb.unchecked_emplace_back(std::move(*p));
#endif
            }

            clear();
            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
        }

        begin_cap_ = sb.begin_cap_;
        begin_     = sb.begin_;
        end_       = sb.end_;
        end_cap_() = sb.end_cap_();

        sb.set_nullptr();
    }

    size_type
    front_spare() const noexcept {
        CIEL_PRECONDITION(begin_cap_ <= begin_);

        return begin_ - begin_cap_;
    }

    size_type
    back_spare() const noexcept {
        CIEL_PRECONDITION(end_ <= end_cap_());

        return end_cap_() - end_;
    }

    template<class U = value_type, typename std::enable_if<is_trivially_relocatable<U>::value, int>::type = 0>
    void
    left_shift_n(const size_type n) noexcept {
        CIEL_PRECONDITION(front_spare() >= n);

        std::memmove(begin_ - n, begin_, sizeof(value_type) / sizeof(unsigned char) * size());
        begin_ -= n;
        end_ -= n;
    }

    // Note that this will invalidate iterators
    template<class U = value_type, typename std::enable_if<!is_trivially_relocatable<U>::value, int>::type = 0>
    void
    left_shift_n(const size_type n) noexcept {
        CIEL_PRECONDITION(front_spare() >= n);

        const size_type old_size = size();

        pointer new_begin = begin_ - n;
        pointer new_end   = new_begin;

        if (old_size >= n) { // n placement new, size - n move assign, n destroy

            // ----------
            //
            // ----------
            // |      | |       |
            // placement new
            // move assign
            //   destroy

            size_type i = 0;
            for (; i < n; ++i) {
                alloc_traits::construct(allocator_(), new_end, std::move(*(begin_ + i)));
                ++new_end;
            }

            for (; i < old_size; ++i) {
                *new_end = std::move(*(begin_ + i));
                ++new_end;
            }

            end_   = alloc_range_destroy(new_end, end_);
            begin_ = new_begin;

        } else { // size placement new, size destroy

            // ----------
            //
            // ----------
            // |        |    |        |
            // placement new
            //  destroy

            for (size_type i = 0; i < old_size; ++i) {
                alloc_traits::construct(allocator_(), new_end, std::move(*(begin_ + i)));
                ++new_end;
            }

            alloc_range_destroy(begin_, end_);
            begin_ = new_begin;
            end_   = new_end;
        }
    }

    template<class U = value_type, typename std::enable_if<is_trivially_relocatable<U>::value, int>::type = 0>
    void
    right_shift_n(const size_type n) noexcept {
        CIEL_PRECONDITION(back_spare() >= n);

        std::memmove(begin_ + n, begin_, sizeof(value_type) / sizeof(unsigned char) * size());
        begin_ += n;
        end_ += n;
    }

    // Note that this will invalidate iterators
    template<class U = value_type, typename std::enable_if<!is_trivially_relocatable<U>::value, int>::type = 0>
    void
    right_shift_n(const size_type n) noexcept {
        CIEL_PRECONDITION(back_spare() >= n);

        const size_type old_size = size();

        pointer new_end   = end_ + n;
        pointer new_begin = new_end;

        if (old_size >= n) { // n placement new, size - n move assign, n destroy

            // ----------
            //
            //         ----------
            // |       | |      |
            //        placement new
            //     move assign
            // destroy

            size_type i = 1;
            for (; i <= n; ++i) {
                alloc_traits::construct(allocator_(), --new_begin, std::move(*(end_ - i)));
            }

            for (; i <= old_size; ++i) {
                *(--new_begin) = std::move(*(end_ - i));
            }

            alloc_range_destroy(begin_, new_begin);
            begin_ = new_begin;
            end_   = new_end;

        } else { // size placement new, size destroy

            // ----------
            //
            //               ----------
            // |        |    |        |
            //              placement new
            //  destroy

            for (size_type i = 1; i <= old_size; ++i) {
                alloc_traits::construct(allocator_(), --new_begin, std::move(*(end_ - i)));
            }

            alloc_range_destroy(begin_, end_);
            begin_ = new_begin;
            end_   = new_end;
        }
    }

    void
    do_destroy() noexcept {
        if (begin_cap_) {
            clear();
            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
        }
    }

    void
    move_range(pointer from_s, pointer from_e, pointer to) {
        pointer old_end   = end_;
        difference_type n = old_end - to;

        for (pointer p = from_s + n; p < from_e; ++p) {
            unchecked_emplace_back(std::move(*p));
        }

        std::move_backward(from_s, from_s + n, old_end);
    }

    void
    set_nullptr() noexcept {
        begin_cap_ = nullptr;
        begin_     = nullptr;
        end_       = nullptr;
        end_cap_() = nullptr;
    }

    // Comparing with vector growing factor: get n * 2 memory, move n elements and get n new space,
    // it's terrible if we shift one (move n elements) to get 1 vacant space for emplace,
    // so only if there is plenty of space at other side will we consider shifting.
    // This situation may be seen when it's used as queue's base container.
    template<class... Args>
    reference
    emplace_back_aux(Args&&... args) {
        if (back_spare() == 0) {
            if CIEL_UNLIKELY (front_spare() > size()) { // move size elements to get more than size / 2 vacant space
                // To support self reference operations like v.emplace_back(v[0]),
                // we must construct temp object here and move it afterwards.
                value_type tmp(std::forward<Args>(args)...);

                left_shift_n(std::max<size_type>(front_spare() / 2, 1));

                unchecked_emplace_back(std::move(tmp));

            } else {
                split_buffer<value_type, allocator_type&> sb(allocator_());
                // end_ - begin_cap_ == front_spare() + size()
                sb.reserve_cap_and_offset_to(recommend_cap(end_ - begin_cap_ + 1), end_ - begin_cap_);

                sb.unchecked_emplace_back(std::forward<Args>(args)...);

                swap_out_buffer(std::move(sb), end_);
            }

        } else {
            unchecked_emplace_back(std::forward<Args>(args)...);
        }

        return back();
    }

    // Check out emplace_back_aux for annotations.
    template<class... Args>
    reference
    emplace_front_aux(Args&&... args) {
        if (front_spare() == 0) {
            if CIEL_UNLIKELY (back_spare() > size()) {
                value_type tmp(std::forward<Args>(args)...);

                right_shift_n(std::max<size_type>(back_spare() / 2, 1));

                unchecked_emplace_front(std::move(tmp));

            } else {
                split_buffer<value_type, allocator_type&> sb(allocator_());
                // end_cap_() - begin_ == back_spare() + size()
                const size_type new_cap = recommend_cap(end_cap_() - begin_ + 1);
                sb.reserve_cap_and_offset_to(new_cap, new_cap - (end_cap_() - begin_));

                sb.unchecked_emplace_front(std::forward<Args>(args)...);

                swap_out_buffer(std::move(sb), begin_);
            }

        } else {
            unchecked_emplace_front(std::forward<Args>(args)...);
        }

        return front();
    }

    template<class... Args>
    void
    unchecked_emplace_back_aux(Args&&... args) {
        CIEL_PRECONDITION(end_ < end_cap_());

        alloc_traits::construct(allocator_(), end_, std::forward<Args>(args)...);
        ++end_;
    }

    template<class... Args>
    void
    unchecked_emplace_front_aux(Args&&... args) {
        CIEL_PRECONDITION(begin_cap_ < begin_);

        alloc_traits::construct(allocator_(), begin_ - 1, std::forward<Args>(args)...);
        --begin_;
    }

public:
    split_buffer() noexcept(noexcept(allocator_type())) = default;

    explicit split_buffer(allocator_type& alloc) noexcept
        : end_cap_alloc_(alloc, nullptr) {}

    explicit split_buffer(const allocator_type& alloc) noexcept
        : end_cap_alloc_(alloc, nullptr) {}

    split_buffer(const size_type count, const value_type& value, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        if CIEL_LIKELY (count > 0) {
            begin_cap_ = alloc_traits::allocate(allocator_(), count);
            end_cap_() = begin_cap_ + count;
            begin_     = begin_cap_;
            end_       = begin_;

            construct_at_end(count, value);
        }
    }

    explicit split_buffer(const size_type count, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        if CIEL_LIKELY (count > 0) {
            begin_cap_ = alloc_traits::allocate(allocator_(), count);
            end_cap_() = begin_cap_ + count;
            begin_     = begin_cap_;
            end_       = begin_;

            construct_at_end(count);
        }
    }

    template<class Iter, typename std::enable_if<is_exactly_input_iterator<Iter>::value, int>::type = 0>
    split_buffer(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        while (first != last) {
            emplace_back(*first);
            ++first;
        }
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    split_buffer(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        const auto count = std::distance(first, last);

        if CIEL_LIKELY (count > 0) {
            begin_cap_ = alloc_traits::allocate(allocator_(), count);
            end_cap_() = begin_cap_ + count;
            begin_     = begin_cap_;
            end_       = begin_;

            construct_at_end(first, last);
        }
    }

    split_buffer(const split_buffer& other)
        : split_buffer(other.begin(), other.end(),
                       alloc_traits::select_on_container_copy_construction(other.get_allocator())) {}

    split_buffer(const split_buffer& other, const allocator_type& alloc)
        : split_buffer(other.begin(), other.end(), alloc) {}

    split_buffer(split_buffer&& other) noexcept
        : begin_cap_(other.begin_cap_),
          begin_(other.begin_),
          end_(other.end_),
          end_cap_alloc_(std::move(other.allocator_()), other.end_cap_()) {
        other.set_nullptr();
    }

    split_buffer(split_buffer&& other, const allocator_type& alloc) {
        if (alloc == other.get_allocator()) {
            allocator_() = alloc;
            begin_cap_   = other.begin_cap_;
            begin_       = other.begin_;
            end_         = other.end_;
            end_cap_()   = other.end_cap_();

            other.set_nullptr();

        } else {
            split_buffer(other, alloc).swap(*this);
        }
    }

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    split_buffer(InitializerList init, const allocator_type& alloc = allocator_type())
        : split_buffer(init.begin(), init.end(), alloc) {}

    template<class U = value_type, typename std::enable_if<worth_move_constructing<U>::value, int>::type = 0>
    split_buffer(std::initializer_list<move_proxy<value_type>> init, const allocator_type& alloc = allocator_type())
        : split_buffer(init.begin(), init.end(), alloc) {}

    template<class U = value_type, typename std::enable_if<!worth_move_constructing<U>::value, int>::type = 0>
    split_buffer(std::initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
        : split_buffer(init.begin(), init.end(), alloc) {}

    ~split_buffer() {
        do_destroy();
    }

    split_buffer&
    operator=(const split_buffer& other) {
        if CIEL_UNLIKELY (this == std::addressof(other)) {
            return *this;
        }

        if (alloc_traits::propagate_on_container_copy_assignment::value) {
            if (allocator_() != other.allocator_()) {
                split_buffer(other.begin(), other.end(), other.allocator_()).swap(*this);
                return *this;
            }

            allocator_() = other.allocator_();
        }

        assign(other.begin(), other.end());

        return *this;
    }

    split_buffer&
    operator=(split_buffer&& other) noexcept(alloc_traits::propagate_on_container_move_assignment::value
                                             || alloc_traits::is_always_equal::value) {
        if CIEL_UNLIKELY (this == std::addressof(other)) {
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

        begin_cap_ = other.begin_cap_;
        begin_     = other.begin_;
        end_       = other.end_;
        end_cap_() = other.end_cap_();

        other.set_nullptr();

        return *this;
    }

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    split_buffer&
    operator=(InitializerList ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    template<class U = value_type, typename std::enable_if<ciel::worth_move<U>::value, int>::type = 0>
    split_buffer&
    operator=(std::initializer_list<move_proxy<value_type>> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    template<class U = value_type, typename std::enable_if<!ciel::worth_move<U>::value, int>::type = 0>
    split_buffer&
    operator=(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    void
    assign(const size_type count, const value_type& value) {
        if (back_spare() + size() < count) {
            const size_type diff = count - back_spare() - size();

            if (front_spare() >= diff) {
                left_shift_n(diff);

            } else {
                split_buffer{count, value, allocator_()}.swap(*this);
                return;
            }

        } else if (size() > count) {
            end_ = alloc_range_destroy(begin_ + count, end_);
        }

        CIEL_POSTCONDITION(size() <= count);

        std::fill_n(begin_, size(), value);
        // if count > size()
        construct_at_end(count - size(), value);

        CIEL_POSTCONDITION(size() == count);
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    void
    assign(Iter first, Iter last) {
        const size_type count = std::distance(first, last);

        if (back_spare() + size() < count) {
            const size_type diff = count - back_spare() - size();

            if (front_spare() >= diff) {
                left_shift_n(diff);

            } else {
                split_buffer{first, last, allocator_()}.swap(*this);
                return;
            }

        } else if (size() > count) {
            end_ = alloc_range_destroy(begin_ + count, end_);
        }

        CIEL_POSTCONDITION(size() <= count);

        Iter mid = std::next(first, size());

        std::copy(first, mid, begin_);
        // if mid < last
        construct_at_end(mid, last);

        CIEL_POSTCONDITION(size() == count);
    }

    template<class Iter, typename std::enable_if<is_exactly_input_iterator<Iter>::value, int>::type = 0>
    void
    assign(Iter first, Iter last) {
        clear();

        while (first != last) {
            emplace_back(*first);
            ++first;
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
    at(const size_type pos) {
        if CIEL_UNLIKELY (pos >= size()) {
            ciel::throw_exception(std::out_of_range("pos is not within the range of ciel::split_buffer"));
        }

        return begin_[pos];
    }

    CIEL_NODISCARD const_reference
    at(const size_type pos) const {
        if CIEL_UNLIKELY (pos >= size()) {
            ciel::throw_exception(std::out_of_range("pos is not within the range of ciel::split_buffer"));
        }

        return begin_[pos];
    }

    CIEL_NODISCARD reference
    operator[](const size_type pos) {
        CIEL_PRECONDITION(pos < size());

        return begin_[pos];
    }

    CIEL_NODISCARD const_reference
    operator[](const size_type pos) const {
        CIEL_PRECONDITION(pos < size());

        return begin_[pos];
    }

    CIEL_NODISCARD reference
    front() {
        CIEL_PRECONDITION(!empty());

        return begin_[0];
    }

    CIEL_NODISCARD const_reference
    front() const {
        CIEL_PRECONDITION(!empty());

        return begin_[0];
    }

    CIEL_NODISCARD reference
    back() {
        CIEL_PRECONDITION(!empty());

        return *(end_ - 1);
    }

    CIEL_NODISCARD const_reference
    back() const {
        CIEL_PRECONDITION(!empty());

        return *(end_ - 1);
    }

    CIEL_NODISCARD T*
    data() noexcept {
        return begin_;
    }

    CIEL_NODISCARD const T*
    data() const noexcept {
        return begin_;
    }

    CIEL_NODISCARD iterator
    begin() noexcept {
        return iterator(begin_);
    }

    CIEL_NODISCARD const_iterator
    begin() const noexcept {
        return const_iterator(begin_);
    }

    CIEL_NODISCARD const_iterator
    cbegin() const noexcept {
        return begin();
    }

    CIEL_NODISCARD iterator
    end() noexcept {
        return iterator(end_);
    }

    CIEL_NODISCARD const_iterator
    end() const noexcept {
        return const_iterator(end_);
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
        return begin_ == end_;
    }

    CIEL_NODISCARD size_type
    size() const noexcept {
        return end_ - begin_;
    }

    CIEL_NODISCARD size_type
    max_size() const noexcept {
        return alloc_traits::max_size(allocator_());
    }

    void
    reserve_front_spare(const size_type new_spare) {
        if (new_spare <= front_spare()) {
            return;
        }

        if (new_spare <= front_spare() + back_spare()) {
            right_shift_n(new_spare - front_spare());

            CIEL_POSTCONDITION(new_spare <= front_spare());
            return;
        }

        split_buffer<value_type, allocator_type&> sb(allocator_());
        sb.reserve_cap_and_offset_to(new_spare + size() + back_spare(), new_spare);

        swap_out_buffer(std::move(sb), begin_);

        CIEL_POSTCONDITION(new_spare <= front_spare());
    }

    void
    reserve_back_spare(const size_type new_spare) {
        if (new_spare <= back_spare()) {
            return;
        }

        if (new_spare <= front_spare() + back_spare()) {
            left_shift_n(new_spare - back_spare());

            CIEL_POSTCONDITION(new_spare <= back_spare());
            return;
        }

        split_buffer<value_type, allocator_type&> sb(allocator_());
        sb.reserve_cap_and_offset_to(new_spare + size() + front_spare(), front_spare());

        swap_out_buffer(std::move(sb), begin_);

        CIEL_POSTCONDITION(new_spare <= back_spare());
    }

    CIEL_NODISCARD size_type
    capacity() const noexcept {
        return end_cap_() - begin_cap_;
    }

    void
    shrink_to_fit() {
        if CIEL_UNLIKELY (front_spare() == 0 && back_spare() == 0) {
            return;
        }

        if (size() > 0) {
            split_buffer<value_type, allocator_type&> sb(allocator_());

            CIEL_TRY {
                sb.reserve_cap_and_offset_to(size(), 0);

                swap_out_buffer(std::move(sb), begin_);
            }
            CIEL_CATCH (...) {}

        } else if (begin_cap_) {
            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
            set_nullptr();
        }
    }

    void
    clear() noexcept {
        end_ = alloc_range_destroy(begin_, end_);
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
        return emplace_back_aux(std::forward<Args>(args)...);
    }

    template<class U, class... Args>
    reference
    emplace_back(std::initializer_list<U> il, Args&&... args) {
        return emplace_back_aux(il, std::forward<Args>(args)...);
    }

    void
    pop_back() noexcept {
        CIEL_PRECONDITION(!empty());

        end_ = alloc_range_destroy(end_ - 1, end_);
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
        return emplace_front_aux(std::forward<Args>(args)...);
    }

    template<class U, class... Args>
    reference
    emplace_front(std::initializer_list<U> il, Args&&... args) {
        return emplace_front_aux(il, std::forward<Args>(args)...);
    }

    void
    pop_front() noexcept {
        CIEL_PRECONDITION(!empty());

        alloc_range_destroy(begin_, begin_ + 1);
        ++begin_;
    }

    void
    resize(const size_type count) {
        if (size() >= count) {
            end_ = alloc_range_destroy(begin_ + count, end_);
            return;
        }

        reserve_back_spare(count - size());

        construct_at_end(count - size());
    }

    void
    resize(const size_type count, const value_type& value) {
        if (size() >= count) {
            end_ = alloc_range_destroy(begin_ + count, end_);
            return;
        }

        reserve_back_spare(count - size());

        construct_at_end(count - size(), value);
    }

    template<class A = Allocator, typename std::enable_if<!std::is_reference<A>::value, int>::type = 0>
    void
    swap(split_buffer& other) noexcept(alloc_traits::propagate_on_container_swap::value
                                       || alloc_traits::is_always_equal::value) {
        using std::swap;

        swap(begin_cap_, other.begin_cap_);
        swap(begin_, other.begin_);
        swap(end_, other.end_);
        swap(end_cap_(), other.end_cap_());
        swap(allocator_(), other.allocator_());
    }

    template<class... Args>
    void
    unchecked_emplace_back(Args&&... args) {
        unchecked_emplace_back_aux(std::forward<Args>(args)...);
    }

    template<class U, class... Args>
    void
    unchecked_emplace_back(std::initializer_list<U> il, Args&&... args) {
        unchecked_emplace_back_aux(il, std::forward<Args>(args)...);
    }

    template<class... Args>
    void
    unchecked_emplace_front(Args&&... args) {
        unchecked_emplace_front_aux(std::forward<Args>(args)...);
    }

    template<class U, class... Args>
    void
    unchecked_emplace_front(std::initializer_list<U> il, Args&&... args) {
        unchecked_emplace_front_aux(il, std::forward<Args>(args)...);
    }

}; // class split_buffer

template<class T, class Allocator>
struct is_trivially_relocatable<split_buffer<T, Allocator>> : is_trivially_relocatable<Allocator> {};

template<class T, class Alloc>
CIEL_NODISCARD bool
operator==(const split_buffer<T, Alloc>& lhs, const split_buffer<T, Alloc>& rhs) noexcept {
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template<class T, class Alloc>
CIEL_NODISCARD bool
operator!=(const split_buffer<T, Alloc>& lhs, const split_buffer<T, Alloc>& rhs) noexcept {
    return !(lhs == rhs);
}

#if CIEL_STD_VER >= 17

template<class Iter, class Alloc = std::allocator<typename std::iterator_traits<Iter>::value_type>>
split_buffer(Iter, Iter, Alloc = Alloc()) -> split_buffer<typename std::iterator_traits<Iter>::value_type, Alloc>;

#endif

NAMESPACE_CIEL_END

namespace std {

template<class T, class Alloc>
void
swap(ciel::split_buffer<T, Alloc>& lhs, ciel::split_buffer<T, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_SPLIT_BUFFER_HPP_

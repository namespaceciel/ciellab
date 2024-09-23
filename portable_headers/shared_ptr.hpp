#ifndef CIELLAB_INCLUDE_CIEL_SHARED_PTR_HPP_
#define CIELLAB_INCLUDE_CIEL_SHARED_PTR_HPP_

#include <atomic>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>

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

NAMESPACE_CIEL_BEGIN

template<class>
class weak_ptr;
template<class>
class enable_shared_from_this;
template<class>
class atomic_shared_ptr;

class shared_weak_count {
protected:
    // The object will be destroyed after decrementing to zero.
    std::atomic<size_t> shared_count_{1};
    // weak_ref + (shared_count_ != 0), The control block will be destroyed after decrementing to zero.
    std::atomic<size_t> weak_count_{1};

    template<class>
    friend class shared_ptr;
    template<class>
    friend class weak_ptr;
    template<class>
    friend class atomic_shared_ptr;

    shared_weak_count() noexcept = default;

    // We never call the deleter on the base class pointer,
    // so it's not necessary to mark virtual on destructor.
    ~shared_weak_count() = default;

public:
    shared_weak_count(const shared_weak_count&) = delete;
    // clang-format off
    shared_weak_count& operator=(const shared_weak_count&) = delete;
    // clang-format on

    CIEL_NODISCARD size_t
    use_count() const noexcept {
        return shared_count_.load(std::memory_order_relaxed);
    }

    void
    shared_add_ref(const size_t count = 1) noexcept {
        const size_t previous = shared_count_.fetch_add(count, std::memory_order_relaxed);

        CIEL_POSTCONDITION(count == 0 || previous != 0);
    }

    void
    weak_add_ref() noexcept {
        const size_t previous = weak_count_.fetch_add(1, std::memory_order_relaxed);

        CIEL_POSTCONDITION(previous != 0);
    }

    void
    shared_count_release() noexcept {
        constexpr size_t off = 1;
        // A decrement-release + an acquire fence is recommended by Boost's documentation:
        // https://www.boost.org/doc/libs/1_57_0/doc/html/atomic/usage_examples.html
        // Alternatively, an acquire-release decrement would work, but might be less efficient
        // since the acquire is only relevant if the decrement zeros the counter.
        if (shared_count_.fetch_sub(off, std::memory_order_release) == off) {
            std::atomic_thread_fence(std::memory_order_acquire);

            delete_pointer();
            weak_count_release(); // weak_count_ == weak_ref + (shared_count_ != 0)
        }
    }

    void
    weak_count_release() noexcept {
        constexpr size_t off = 1;
        // Avoid expensive atomic stores inspired by LLVM:
        // https://github.com/llvm/llvm-project/commit/ac9eec8602786b13a2bea685257d4f25b36030ff
        if (weak_count_.load(std::memory_order_acquire) == off) {
            delete_control_block();

        } else if (weak_count_.fetch_sub(off, std::memory_order_release) == off) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete_control_block();
        }
    }

    CIEL_NODISCARD bool
    increment_if_not_zero() noexcept {
        size_t old_count = shared_count_.load(std::memory_order_relaxed);

        do {
            if (old_count == 0) {
                return false;
            }

        } while (!shared_count_.compare_exchange_weak(old_count, old_count + 1, std::memory_order_relaxed));

        return true;
    }

    CIEL_NODISCARD virtual void*
    get_deleter(const std::type_info&) noexcept {
        return nullptr;
    }

    virtual void
    delete_pointer() noexcept
        = 0;
    virtual void
    delete_control_block() noexcept
        = 0;
    virtual void*
    managed_pointer() const noexcept
        = 0;

}; // class shared_weak_count

template<class element_type, class Deleter, class Allocator>
class control_block_with_pointer final : public shared_weak_count {
    static_assert(std::is_same<element_type, typename Allocator::value_type>::value, "");

public:
    using pointer        = element_type*;
    using deleter_type   = Deleter;
    using allocator_type = Allocator;

private:
    using alloc_traits               = std::allocator_traits<allocator_type>;
    using control_block_allocator    = typename alloc_traits::template rebind_alloc<control_block_with_pointer>;
    using control_block_alloc_traits = typename alloc_traits::template rebind_traits<control_block_with_pointer>;

    ciel::compressed_pair<ciel::compressed_pair<pointer, deleter_type>, control_block_allocator> compressed_;

    CIEL_NODISCARD pointer&
    ptr_() noexcept {
        return compressed_.first().first();
    }

    CIEL_NODISCARD const pointer&
    ptr_() const noexcept {
        return compressed_.first().first();
    }

    CIEL_NODISCARD deleter_type&
    deleter_() noexcept {
        return compressed_.first().second();
    }

    CIEL_NODISCARD const deleter_type&
    deleter_() const noexcept {
        return compressed_.first().second();
    }

    CIEL_NODISCARD control_block_allocator&
    allocator_() noexcept {
        return compressed_.second();
    }

    CIEL_NODISCARD const control_block_allocator&
    allocator_() const noexcept {
        return compressed_.second();
    }

public:
    control_block_with_pointer(pointer ptr, deleter_type&& deleter, control_block_allocator&& alloc)
        : compressed_(ciel::compressed_pair<pointer, deleter_type>(ptr, std::move(deleter)), std::move(alloc)) {}

#ifdef CIEL_HAS_RTTI
    CIEL_NODISCARD virtual void*
    get_deleter(const std::type_info& type) noexcept override {
        return (type == typeid(deleter_type)) ? static_cast<void*>(&deleter_()) : nullptr;
    }

#endif

    virtual void
    delete_pointer() noexcept override {
        deleter_()(ptr_());
        deleter_().~deleter_type();
    }

    virtual void
    delete_control_block() noexcept override {
        control_block_allocator allocator = std::move(allocator_());
        allocator_().~control_block_allocator();

        control_block_alloc_traits::deallocate(allocator, this, 1);
    }

    virtual void*
    managed_pointer() const noexcept override {
        return ptr_();
    }

}; // class control_block_with_pointer

static_assert(sizeof(control_block_with_pointer<int, std::default_delete<int>, std::allocator<int>>)
                      - sizeof(shared_weak_count)
                  == 8,
              "Empty Base Optimization is not working.");

template<class element_type, class Allocator>
class control_block_with_instance final : public shared_weak_count {
    static_assert(std::is_same<element_type, typename Allocator::value_type>::value, "");

public:
    using pointer        = element_type*;
    using allocator_type = Allocator;

private:
    using alloc_traits               = std::allocator_traits<allocator_type>;
    using control_block_allocator    = typename alloc_traits::template rebind_alloc<control_block_with_instance>;
    using control_block_alloc_traits = typename alloc_traits::template rebind_traits<control_block_with_instance>;

    compressed_pair<typename aligned_storage<sizeof(element_type), alignof(element_type)>::type,
                    control_block_allocator>
        compressed_;

    CIEL_NODISCARD pointer
    ptr_() noexcept {
        return static_cast<pointer>(static_cast<void*>(&compressed_.first()));
    }

    CIEL_NODISCARD const element_type*
    ptr_() const noexcept {
        return static_cast<const element_type*>(static_cast<const void*>(&compressed_.first()));
    }

    CIEL_NODISCARD control_block_allocator&
    allocator_() noexcept {
        return compressed_.second();
    }

    CIEL_NODISCARD const control_block_allocator&
    allocator_() const noexcept {
        return compressed_.second();
    }

public:
    template<class... Args>
    control_block_with_instance(allocator_type alloc, Args&&... args)
        : compressed_(default_init_tag, alloc) {
        alloc_traits::construct(alloc, ptr_(), std::forward<Args>(args)...);
    }

    virtual void
    delete_pointer() noexcept override {
        allocator_type alloc(allocator_());

        alloc_traits::destroy(alloc, ptr_());
    }

    virtual void
    delete_control_block() noexcept override {
        control_block_allocator allocator = std::move(allocator_());
        allocator_().~control_block_allocator();

        control_block_alloc_traits::deallocate(allocator, this, 1);
    }

    virtual void*
    managed_pointer() const noexcept override {
        return const_cast<void*>(static_cast<const void*>(&compressed_.first()));
    }

}; // class control_block_with_instance

template<class T>
class shared_ptr {
public:
    using element_type = typename std::remove_extent<T>::type;
    using pointer      = element_type*;
    using weak_type    = weak_ptr<T>;

private:
    template<class>
    friend class shared_ptr;
    template<class>
    friend class weak_ptr;
    template<class>
    friend class atomic_shared_ptr;
    template<class U, class Alloc, class... Args>
    friend shared_ptr<U>
    allocate_shared(const Alloc&, Args&&...);

    element_type* ptr_;
    shared_weak_count* control_block_;

    template<class Y, class Deleter, class Allocator>
    CIEL_NODISCARD shared_weak_count*
    alloc_control_block(Y* ptr, Deleter&& dlt, Allocator&& alloc) {
        using alloc_traits               = std::allocator_traits<Allocator>;
        using control_block_type         = control_block_with_pointer<Y, Deleter, Allocator>;
        using control_block_allocator    = typename alloc_traits::template rebind_alloc<control_block_type>;
        using control_block_alloc_traits = typename alloc_traits::template rebind_traits<control_block_type>;

        control_block_allocator allocator(alloc);
        control_block_type* control_block = control_block_alloc_traits::allocate(allocator, 1);

        CIEL_TRY {
            control_block_alloc_traits::construct(allocator, control_block, ptr, std::forward<Deleter>(dlt),
                                                  std::forward<Allocator>(alloc));
            return control_block;
        }
        CIEL_CATCH (...) {
            control_block_alloc_traits::deallocate(allocator, control_block, 1);
            CIEL_THROW;
        }
    }

    // Serves for enable_shared_from_this.
    template<
        class Now, class Original,
        typename std::enable_if<std::is_convertible<Original*, const enable_shared_from_this<Now>*>::value, int>::type
        = 0>
    void
    enable_weak_this(const enable_shared_from_this<Now>* now_ptr, Original* original_ptr) noexcept {
        using RawNow = typename std::remove_cv<Now>::type;

        // If now_ptr is not initialized, let it points to the right control block.
        if (now_ptr && now_ptr->weak_this_.expired()) {
            now_ptr->weak_this_ = shared_ptr<RawNow>(*this, const_cast<RawNow*>(static_cast<const Now*>(original_ptr)));
        }
    }

    void
    enable_weak_this(...) noexcept {}

    // Only be used by atomic_shared_ptr.
    void
    clear() noexcept {
        ptr_           = nullptr;
        control_block_ = nullptr;
    }

    shared_ptr(shared_weak_count* control_block) noexcept
        : ptr_(control_block ? static_cast<pointer>(control_block->managed_pointer()) : nullptr),
          control_block_(control_block) {}

public:
    shared_ptr() noexcept
        : ptr_(nullptr), control_block_(nullptr) {}

    shared_ptr(std::nullptr_t) noexcept
        : ptr_(nullptr), control_block_(nullptr) {}

    template<class Y, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    explicit shared_ptr(Y* ptr)
        : ptr_(ptr) {
        std::unique_ptr<Y> holder(ptr);

        control_block_ = alloc_control_block(ptr, std::default_delete<Y>(), std::allocator<Y>());

        CIEL_UNUSED(holder.release());

        enable_weak_this(ptr, ptr);
    }

    template<class Y, class Deleter, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    shared_ptr(Y* ptr, Deleter d)
        : ptr_(ptr) {
        CIEL_TRY {
            control_block_ = alloc_control_block(ptr, std::move(d), std::allocator<Y>());
        }
        CIEL_CATCH (...) {
            d(ptr);
            CIEL_THROW;
        }

        enable_weak_this(ptr, ptr);
    }

    template<class Deleter>
    shared_ptr(std::nullptr_t ptr, Deleter d)
        : ptr_(nullptr) {
        CIEL_TRY {
            control_block_ = alloc_control_block(ptr_, std::move(d), std::allocator<T>());
        }
        CIEL_CATCH (...) {
            d(ptr);
            CIEL_THROW;
        }
    }

    template<class Y, class Deleter, class Alloc,
             typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    shared_ptr(Y* ptr, Deleter d, Alloc alloc)
        : ptr_(ptr) {
        CIEL_TRY {
            control_block_ = alloc_control_block(ptr, std::move(d), std::move(alloc));
        }
        CIEL_CATCH (...) {
            d(ptr);
            CIEL_THROW;
        }

        enable_weak_this(ptr, ptr);
    }

    template<class Deleter, class Alloc>
    shared_ptr(std::nullptr_t ptr, Deleter d, Alloc alloc)
        : ptr_(nullptr) {
        CIEL_TRY {
            control_block_ = alloc_control_block(ptr_, std::move(d), std::move(alloc));
        }
        CIEL_CATCH (...) {
            d(ptr);
            CIEL_THROW;
        }
    }

    template<class Y>
    shared_ptr(const shared_ptr<Y>& r, element_type* ptr) noexcept
        : ptr_(ptr), control_block_(r.control_block_) {
        if (control_block_ != nullptr) {
            control_block_->shared_add_ref();
        }
    }

    template<class Y>
    shared_ptr(shared_ptr<Y>&& r, element_type* ptr) noexcept
        : ptr_(ptr), control_block_(r.control_block_) {
        r.ptr_           = nullptr;
        r.control_block_ = nullptr;
    }

    shared_ptr(const shared_ptr& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        if (control_block_ != nullptr) {
            control_block_->shared_add_ref();
        }
    }

    template<class Y, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    shared_ptr(const shared_ptr<Y>& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        if (control_block_ != nullptr) {
            control_block_->shared_add_ref();
        }
    }

    shared_ptr(shared_ptr&& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        r.ptr_           = nullptr;
        r.control_block_ = nullptr;
    }

    template<class Y, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    shared_ptr(shared_ptr<Y>&& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        r.ptr_           = nullptr;
        r.control_block_ = nullptr;
    }

    template<class Y, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    explicit shared_ptr(const weak_ptr<Y>& r)
        : ptr_(r.ptr_),
          control_block_(r.control_block_ ? (r.control_block_->increment_if_not_zero() ? r.control_block_ : nullptr)
                                          : nullptr) {
        if (control_block_ == nullptr) {
            ciel::throw_exception(std::bad_weak_ptr());
        }
    }

    template<class Y, class Deleter, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    shared_ptr(std::unique_ptr<Y, Deleter>&& r)
        : ptr_(r.get()) {
        if (ptr_ != nullptr) {
            control_block_ = alloc_control_block(ptr_, std::move(r.get_deleter()), std::allocator<T>());

        } else {
            control_block_ = nullptr;
        }

        enable_weak_this(r.get(), r.get());

        CIEL_UNUSED(r.release());
    }

    ~shared_ptr() {
        if (control_block_ != nullptr) {
            control_block_->shared_count_release();
        }
    }

    shared_ptr&
    operator=(const shared_ptr& r) noexcept {
        shared_ptr(r).swap(*this);
        return *this;
    }

    template<class Y>
    shared_ptr&
    operator=(const shared_ptr<Y>& r) noexcept {
        shared_ptr(r).swap(*this);
        return *this;
    }

    shared_ptr&
    operator=(shared_ptr&& r) noexcept {
        shared_ptr(std::move(r)).swap(*this);
        return *this;
    }

    template<class Y>
    shared_ptr&
    operator=(shared_ptr<Y>&& r) noexcept {
        shared_ptr(std::move(r)).swap(*this);
        return *this;
    }

    template<class Y, class Deleter>
    shared_ptr&
    operator=(std::unique_ptr<Y, Deleter>&& r) {
        shared_ptr(std::move(r)).swap(*this);
        return *this;
    }

    void
    reset() noexcept {
        shared_ptr().swap(*this);
    }

    template<class Y, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    void
    reset(Y* ptr) {
        shared_ptr(ptr).swap(*this);
    }

    template<class Y, class Deleter, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    void
    reset(Y* ptr, Deleter d) {
        shared_ptr(ptr, std::move(d)).swap(*this);
    }

    template<class Y, class Deleter, class Alloc,
             typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    void
    reset(Y* ptr, Deleter d, Alloc alloc) {
        shared_ptr(ptr, std::move(d), std::move(alloc)).swap(*this);
    }

    void
    swap(shared_ptr& r) noexcept {
        using std::swap;

        swap(ptr_, r.ptr_);
        swap(control_block_, r.control_block_);
    }

    CIEL_NODISCARD element_type*
    get() const noexcept {
        return ptr_;
    }

    CIEL_NODISCARD T&
    operator*() const noexcept {
        CIEL_PRECONDITION(*this);

        return *get();
    }

    CIEL_NODISCARD T*
    operator->() const noexcept {
        CIEL_PRECONDITION(*this);

        return get();
    }

    CIEL_NODISCARD element_type&
    operator[](const size_t idx) const {
        static_assert(std::is_array<T>::value, "ciel::shared_ptr::operator[] is valid only when T is array");

        return get()[idx];
    }

    CIEL_NODISCARD size_t
    use_count() const noexcept {
        return control_block_ != nullptr ? control_block_->use_count() : 0;
    }

    CIEL_NODISCARD explicit
    operator bool() const noexcept {
        return get() != nullptr;
    }

    template<class Y>
    CIEL_NODISCARD bool
    owner_before(const shared_ptr<Y>& other) const noexcept {
        return control_block_ < other.control_block_;
    }

    template<class Y>
    CIEL_NODISCARD bool
    owner_before(const weak_ptr<Y>& other) const noexcept {
        return control_block_ < other.control_block_;
    }

    template<class D>
    CIEL_NODISCARD D*
    get_deleter() const noexcept {
#ifdef CIEL_HAS_RTTI
        return control_block_ ? static_cast<D*>(control_block_->get_deleter(typeid(typename std::remove_cv<D>::type)))
                              : nullptr;
#else
        return nullptr;
#endif
    }

}; // class shared_ptr

template<class T>
struct is_trivially_relocatable<shared_ptr<T>> : std::true_type {};

template<class T, class U>
CIEL_NODISCARD bool
operator==(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept {
    return lhs.get() == rhs.get();
}

template<class T, class U>
CIEL_NODISCARD bool
operator!=(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept {
    return !(lhs == rhs);
}

template<class T>
CIEL_NODISCARD bool
operator==(const shared_ptr<T>& lhs, std::nullptr_t) noexcept {
    return !lhs;
}

template<class T>
CIEL_NODISCARD bool
operator==(std::nullptr_t, const shared_ptr<T>& rhs) noexcept {
    return !rhs;
}

template<class T>
CIEL_NODISCARD bool
operator!=(const shared_ptr<T>& lhs, std::nullptr_t) noexcept {
    return static_cast<bool>(lhs);
}

template<class T>
CIEL_NODISCARD bool
operator!=(std::nullptr_t, const shared_ptr<T>& rhs) noexcept {
    return static_cast<bool>(rhs);
}

template<class Deleter, class T>
CIEL_NODISCARD Deleter*
get_deleter(const shared_ptr<T>& p) noexcept {
    return p.template get_deleter<Deleter>();
}

#if CIEL_STD_VER >= 17
template<class T>
shared_ptr(weak_ptr<T>) -> shared_ptr<T>;

template<class T, class D>
shared_ptr(std::unique_ptr<T, D>) -> shared_ptr<T>;

#endif // CIEL_STD_VER >= 17

template<class T>
class weak_ptr {
public:
    using element_type = typename std::remove_extent<T>::type;
    using pointer      = element_type*;

private:
    element_type* ptr_;
    shared_weak_count* control_block_;

public:
    template<class>
    friend class shared_ptr;
    template<class>
    friend class weak_ptr;

    constexpr weak_ptr() noexcept
        : ptr_(nullptr), control_block_(nullptr) {}

    weak_ptr(const weak_ptr& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        if (control_block_ != nullptr) {
            control_block_->weak_add_ref();
        }
    }

    template<class Y, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    weak_ptr(const weak_ptr<Y>& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        if (control_block_ != nullptr) {
            control_block_->weak_add_ref();
        }
    }

    template<class Y, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    weak_ptr(const shared_ptr<Y>& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        if (control_block_ != nullptr) {
            control_block_->weak_add_ref();
        }
    }

    weak_ptr(weak_ptr&& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        r.ptr_           = nullptr;
        r.control_block_ = nullptr;
    }

    template<class Y, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    weak_ptr(weak_ptr<Y>&& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        r.ptr_           = nullptr;
        r.control_block_ = nullptr;
    }

    ~weak_ptr() {
        if (control_block_ != nullptr) {
            control_block_->weak_count_release();
        }
    }

    weak_ptr&
    operator=(const weak_ptr& r) noexcept {
        weak_ptr(r).swap(*this);
        return *this;
    }

    template<class Y>
    weak_ptr&
    operator=(const weak_ptr<Y>& r) noexcept {
        weak_ptr(r).swap(*this);
        return *this;
    }

    template<class Y>
    weak_ptr&
    operator=(const shared_ptr<Y>& r) noexcept {
        weak_ptr(r).swap(*this);
        return *this;
    }

    weak_ptr&
    operator=(weak_ptr&& r) noexcept {
        weak_ptr(std::move(r)).swap(*this);
        return *this;
    }

    template<class Y>
    weak_ptr&
    operator=(weak_ptr<Y>&& r) noexcept {
        weak_ptr(std::move(r)).swap(*this);
        return *this;
    }

    void
    reset() noexcept {
        if (control_block_ != nullptr) {
            control_block_->weak_count_release();

            ptr_           = nullptr;
            control_block_ = nullptr;
        }
    }

    void
    swap(weak_ptr& r) noexcept {
        using std::swap;

        swap(ptr_, r.ptr_);
        swap(control_block_, r.control_block_);
    }

    CIEL_NODISCARD size_t
    use_count() const noexcept {
        return control_block_ != nullptr ? control_block_->use_count() : 0;
    }

    CIEL_NODISCARD bool
    expired() const noexcept {
        return use_count() == 0;
    }

    CIEL_NODISCARD shared_ptr<T>
    lock() const noexcept {
        if (control_block_ == nullptr) {
            return shared_ptr<T>();
        }

        // private constructor used here and atomic_shared_ptr.
        return control_block_->increment_if_not_zero() ? shared_ptr<T>(control_block_) : shared_ptr<T>();
    }

    template<class Y>
    CIEL_NODISCARD bool
    owner_before(const weak_ptr<Y>& other) const noexcept {
        return control_block_ < other.control_block_;
    }

    template<class Y>
    CIEL_NODISCARD bool
    owner_before(const shared_ptr<Y>& other) const noexcept {
        return control_block_ < other.control_block_;
    }

}; // class weak_ptr

template<class T>
struct is_trivially_relocatable<weak_ptr<T>> : std::true_type {};

#if CIEL_STD_VER >= 17
template<class T>
weak_ptr(shared_ptr<T>) -> weak_ptr<T>;

#endif // #if CIEL_STD_VER >= 17

template<class T>
class enable_shared_from_this {
private:
    mutable weak_ptr<T> weak_this_;

protected:
    constexpr enable_shared_from_this() noexcept                     = default;
    enable_shared_from_this(const enable_shared_from_this&) noexcept = default;
    ~enable_shared_from_this()                                       = default;
    enable_shared_from_this&
    operator=(const enable_shared_from_this&) noexcept
        = default;

public:
    template<class>
    friend class shared_ptr;

    CIEL_NODISCARD shared_ptr<T>
    shared_from_this() {
        return shared_ptr<T>(weak_this_);
    }

    CIEL_NODISCARD shared_ptr<const T>
    shared_from_this() const {
        return shared_ptr<const T>(weak_this_);
    }

    CIEL_NODISCARD weak_ptr<T>
    weak_from_this() noexcept {
        return weak_this_;
    }

    CIEL_NODISCARD weak_ptr<const T>
    weak_from_this() const noexcept {
        return weak_this_;
    }

}; // class enable_shared_from_this

template<class T>
struct is_trivially_relocatable<enable_shared_from_this<T>> : std::true_type {};

template<class T, class Alloc, class... Args>
shared_ptr<T>
allocate_shared(const Alloc& alloc, Args&&... args) {
    static_assert(std::is_same<T, typename Alloc::value_type>::value, "");

    using control_block_type = control_block_with_instance<T, Alloc>;
    using alloc_traits       = std::allocator_traits<std::allocator<control_block_type>>;

    std::allocator<control_block_type> control_block_alloc(alloc);

    struct alloc_deleter {
    private:
        std::allocator<control_block_type> alloc_;

    public:
        alloc_deleter(std::allocator<control_block_type> a)
            : alloc_(std::move(a)) {}

        void
        operator()(control_block_type* ptr) noexcept {
            alloc_traits::deallocate(alloc_, ptr, 1);
        }

    }; // struct alloc_deleter

    std::unique_ptr<control_block_type, alloc_deleter> control_block{alloc_traits::allocate(control_block_alloc, 1),
                                                                     control_block_alloc};

    alloc_traits::construct(control_block_alloc, control_block.get(), alloc, std::forward<Args>(args)...);

    return shared_ptr<T>(control_block.release());
}

template<class T, class... Args>
shared_ptr<T>
make_shared(Args&&... args) {
    return ciel::allocate_shared<T>(std::allocator<T>(), std::forward<Args>(args)...);
}

NAMESPACE_CIEL_END

namespace std {

template<class T>
void
swap(ciel::shared_ptr<T>& lhs, ciel::shared_ptr<T>& rhs) noexcept {
    lhs.swap(rhs);
}

template<class T>
void
swap(ciel::weak_ptr<T>& lhs, ciel::weak_ptr<T>& rhs) noexcept {
    lhs.swap(rhs);
}

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_SHARED_PTR_HPP_

#ifndef CIELLAB_INCLUDE_CIEL_TYPE_TRAITS_HPP_
#define CIELLAB_INCLUDE_CIEL_TYPE_TRAITS_HPP_

#include <cstring>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

#include <ciel/config.hpp>

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

// useless_t
struct useless_t {
    useless_t(...) noexcept {}
}; // struct useless_t

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

    union type {
        alignas(alignment) unsigned char buffer_[(size + alignment - 1) / alignment * alignment];
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

template<class T>
struct is_range_without_size : std::integral_constant<bool, is_range<T>::value && !is_range_with_size<T>::value> {};

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

// from_range_t
struct from_range_t {};

static constexpr from_range_t from_range;

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TYPE_TRAITS_HPP_

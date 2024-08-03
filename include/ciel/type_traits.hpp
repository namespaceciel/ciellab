#ifndef CIELLAB_INCLUDE_CIEL_TYPE_TRAITS_HPP_
#define CIELLAB_INCLUDE_CIEL_TYPE_TRAITS_HPP_

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
template<class T>
struct is_trivially_relocatable : disjunction<std::is_empty<T>, std::is_trivially_copyable<T>> {};

template<class First, class Second>
struct is_trivially_relocatable<std::pair<First, Second>>
    : conjunction<is_trivially_relocatable<First>, is_trivially_relocatable<Second>> {};

template<class... Types>
struct is_trivially_relocatable<std::tuple<Types...>> : conjunction<is_trivially_relocatable<Types>...> {};

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
template<class T>
using is_final = std::true_type;
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

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TYPE_TRAITS_HPP_

#ifndef CIELLAB_INCLUDE_CIEL_TO_ADDRESS_HPP_
#define CIELLAB_INCLUDE_CIEL_TO_ADDRESS_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/logical.hpp>

#include <memory>
#include <type_traits>
#include <utility>

NAMESPACE_CIEL_BEGIN

CIEL_DIAGNOSTIC_PUSH
#if CIEL_STD_VER >= 20
// std::move_iterator's operator-> has been deprecated in C++20
CIEL_CLANG_DIAGNOSTIC_IGNORED("-Wdeprecated-declarations")
#endif

template<class Pointer, class = void>
struct to_address_helper;

template<class T>
T* to_address(T* p) noexcept {
    static_assert(!std::is_function<T>::value, "T is a function type");
    return p;
}

// has_to_address

template<class Pointer, class = void>
struct has_to_address : std::false_type {};

template<class Pointer>
struct has_to_address<Pointer, decltype((void)std::pointer_traits<Pointer>::to_address(std::declval<const Pointer&>()))>
    : std::true_type {};

// has_arrow

template<class Pointer, class = void>
struct has_arrow : std::false_type {};

template<class Pointer>
struct has_arrow<Pointer, decltype((void)std::declval<const Pointer&>().operator->())> : std::true_type {};

// is_fancy_pointer

template<class Pointer>
struct is_fancy_pointer {
    static constexpr bool value = has_arrow<Pointer>::value || has_to_address<Pointer>::value;
};

// to_address

// enable_if is needed here to avoid instantiating checks for fancy pointers on raw pointers.
template<class Pointer, enable_if_t<conjunction<std::is_class<Pointer>, is_fancy_pointer<Pointer>>::value> = 0>
decay_t<decltype(to_address_helper<Pointer>::call(std::declval<const Pointer&>()))>
to_address(const Pointer& p) noexcept {
    return to_address_helper<Pointer>::call(p);
}

// to_address_helper

template<class Pointer, class>
struct to_address_helper {
    static decltype(ciel::to_address(std::declval<const Pointer&>().operator->())) call(const Pointer& p) noexcept {
        return ciel::to_address(p.operator->());
    }
};

template<class Pointer>
struct to_address_helper<Pointer,
                         decltype((void)std::pointer_traits<Pointer>::to_address(std::declval<const Pointer&>()))> {
    static decltype(std::pointer_traits<Pointer>::to_address(std::declval<const Pointer&>()))
    call(const Pointer& p) noexcept {
        return std::pointer_traits<Pointer>::to_address(p);
    }
};

CIEL_DIAGNOSTIC_POP

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TO_ADDRESS_HPP_

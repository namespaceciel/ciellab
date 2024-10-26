#ifndef CIELLAB_INCLUDE_CIEL_DATASIZEOF_HPP_
#define CIELLAB_INCLUDE_CIEL_DATASIZEOF_HPP_

#include <ciel/config.hpp>
#include <ciel/is_final.hpp>

#include <type_traits>

NAMESPACE_CIEL_BEGIN

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
// Swapping for trivially relocatable types can be performed using std::memcpy,
// in which case there is no need to modify the tail padding.
//
CIEL_DIAGNOSTIC_PUSH
CIEL_CLANG_DIAGNOSTIC_IGNORED("-Winvalid-offsetof")
CIEL_GCC_DIAGNOSTIC_IGNORED("-Winvalid-offsetof")

#if CIEL_STD_VER >= 20
template<class T>
struct datasizeof {
    struct FirstPaddingByte {
        [[no_unique_address]] T v;
        char first_padding_byte;
    };

    static constexpr size_t value = offsetof(FirstPaddingByte, first_padding_byte);

}; // struct datasizeof
#else
template<class T>
struct datasizeof {
    template<class U, bool = std::is_class<U>::value && !is_final<U>::value>
    struct FirstPaddingByte {
        U v;
        char first_padding_byte;
    };

    template<class U>
    struct FirstPaddingByte<U, true> : U {
        char first_padding_byte;
    };

    static constexpr size_t value = offsetof(FirstPaddingByte<T>, first_padding_byte);

}; // struct datasizeof
#endif // CIEL_STD_VER >= 20

CIEL_DIAGNOSTIC_POP

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_DATASIZEOF_HPP_

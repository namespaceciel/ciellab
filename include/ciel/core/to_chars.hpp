#ifndef CIELLAB_INCLUDE_CIEL_CORE_TO_CHARS_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_TO_CHARS_HPP_

#include <ciel/core/config.hpp>

#include <cstdint>
#include <limits>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

// Inspired by LLVM libc++'s implementation.

namespace detail {

inline char* append1(char* first, uint32_t value) noexcept {
    *first = '0' + static_cast<char>(value);
    return first + 1;
}

inline char* append2(char* first, uint32_t value) noexcept {
    constexpr char digits_base_10[200] = {
        '0', '0', '0', '1', '0', '2', '0', '3', '0', '4', '0', '5', '0', '6', '0', '7', '0', '8', '0', '9',
        '1', '0', '1', '1', '1', '2', '1', '3', '1', '4', '1', '5', '1', '6', '1', '7', '1', '8', '1', '9',
        '2', '0', '2', '1', '2', '2', '2', '3', '2', '4', '2', '5', '2', '6', '2', '7', '2', '8', '2', '9',
        '3', '0', '3', '1', '3', '2', '3', '3', '3', '4', '3', '5', '3', '6', '3', '7', '3', '8', '3', '9',
        '4', '0', '4', '1', '4', '2', '4', '3', '4', '4', '4', '5', '4', '6', '4', '7', '4', '8', '4', '9',
        '5', '0', '5', '1', '5', '2', '5', '3', '5', '4', '5', '5', '5', '6', '5', '7', '5', '8', '5', '9',
        '6', '0', '6', '1', '6', '2', '6', '3', '6', '4', '6', '5', '6', '6', '6', '7', '6', '8', '6', '9',
        '7', '0', '7', '1', '7', '2', '7', '3', '7', '4', '7', '5', '7', '6', '7', '7', '7', '8', '7', '9',
        '8', '0', '8', '1', '8', '2', '8', '3', '8', '4', '8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
        '9', '0', '9', '1', '9', '2', '9', '3', '9', '4', '9', '5', '9', '6', '9', '7', '9', '8', '9', '9'};
    *first       = digits_base_10[value * 2];
    *(first + 1) = digits_base_10[value * 2 + 1];
    return first + 2;
}

inline char* append3(char* first, uint32_t value) noexcept {
    return ciel::detail::append2(ciel::detail::append1(first, value / 100), value % 100);
}

inline char* append4(char* first, uint32_t value) noexcept {
    return ciel::detail::append2(ciel::detail::append2(first, value / 100), value % 100);
}

inline char* append5(char* first, uint32_t value) noexcept {
    return ciel::detail::append4(ciel::detail::append1(first, value / 10000), value % 10000);
}

inline char* append6(char* first, uint32_t value) noexcept {
    return ciel::detail::append4(ciel::detail::append2(first, value / 10000), value % 10000);
}

inline char* append7(char* first, uint32_t value) noexcept {
    return ciel::detail::append6(ciel::detail::append1(first, value / 1000000), value % 1000000);
}

inline char* append8(char* first, uint32_t value) noexcept {
    return ciel::detail::append6(ciel::detail::append2(first, value / 1000000), value % 1000000);
}

inline char* append9(char* first, uint32_t value) noexcept {
    return ciel::detail::append8(ciel::detail::append1(first, value / 100000000), value % 100000000);
}

template<class T>
char* append10(char* first, T value) noexcept {
    return ciel::detail::append8(ciel::detail::append2(first, value / 100000000), value % 100000000);
}

} // namespace detail

inline char* to_chars(char* first, const bool value) {
    if (value) {
        *first       = 't';
        *(first + 1) = 'r';
        *(first + 2) = 'u';
        *(first + 3) = 'e';
        return first + 4;
    }
    *first       = 'f';
    *(first + 1) = 'a';
    *(first + 2) = 'l';
    *(first + 3) = 's';
    *(first + 4) = 'e';
    return first + 5;
}

template<class T, enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value && sizeof(T) <= 4
                              && !std::is_same<T, bool>::value> = 0>
char* to_chars(char* first, const T value) noexcept {
    if (value < 1000000) {
        if (value < 10000) {
            if (value < 100) {
                if (value < 10) {
                    // 0 <= value < 10
                    return ciel::detail::append1(first, value);
                }

                // 10 <= value < 100
                return ciel::detail::append2(first, value);
            }

            if (value < 1000) {
                // 100 <= value < 1000
                return ciel::detail::append3(first, value);
            }

            // 1000 <= value < 10000
            return ciel::detail::append4(first, value);
        }

        // 10000 <= value < 100000
        if (value < 100000) {
            return ciel::detail::append5(first, value);
        }

        // 100000 <= value < 1000000
        return ciel::detail::append6(first, value);
    }

    if (value < 100000000) {
        if (value < 10000000) {
            // 1000000 <= value < 10000000
            return ciel::detail::append7(first, value);
        }

        // 10000000 <= value < 100000000
        return ciel::detail::append8(first, value);
    }

    if (value < 1000000000) {
        // 100000000 <= value < 1000000000
        return ciel::detail::append9(first, value);
    }

    // 1000000000 <= value < uint32_max
    return ciel::detail::append10(first, value);
}

// Note: uint64_t is unsigned long long, different from unsigned long.
template<class T, enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value && sizeof(T) == 8> = 0>
char* to_chars(char* first, T value) noexcept {
    if (value <= std::numeric_limits<uint32_t>::max()) {
        return ciel::to_chars(first, static_cast<uint32_t>(value));
    }

    if (value >= 10000000000) {
        first = ciel::to_chars(first, static_cast<uint32_t>(value / 10000000000));
        value %= 10000000000;
    }
    return ciel::detail::append10(first, value);
}

template<class T, enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value> = 0>
char* to_chars(char* first, const T value) noexcept {
    auto x = ciel::unsigned_cast(value);
    if (value < 0) {
        *first++ = '-';
        x        = ciel::complement(x);
    }

    return ciel::to_chars(first, x);
}

// inline char* to_chars(char* first, float value) noexcept;
// inline char* to_chars(char* first, double value) noexcept;
// inline char* to_chars(char* first, long double value) noexcept;

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_TO_CHARS_HPP_

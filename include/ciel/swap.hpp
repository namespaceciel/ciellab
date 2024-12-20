#ifndef CIELLAB_INCLUDE_CIEL_SWAP_HPP_
#define CIELLAB_INCLUDE_CIEL_SWAP_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/cstring.hpp>
#include <ciel/core/datasizeof.hpp>
#include <ciel/core/is_trivially_relocatable.hpp>

#include <iterator>
#include <memory>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

// Inspired by Arthur O’Dwyer's libc++ fork:
// https://github.com/Quuxplusone/llvm-project/blob/6af94af4d0351649eaef8454e7fb58a1175a9580/libcxx/include/__utility/swap.h

// relocatable_swap
template<class T>
void relocatable_swap(T& lhs, T& rhs) noexcept {
    constexpr size_t buffer_bytes = datasizeof<T>::value;
    unsigned char buffer[buffer_bytes];

    ciel::memcpy(std::addressof(buffer), std::addressof(lhs), buffer_bytes);
    ciel::memmove(std::addressof(lhs), std::addressof(rhs), buffer_bytes);
    ciel::memcpy(std::addressof(rhs), std::addressof(buffer), buffer_bytes);
}

template<class T, size_t N>
void relocatable_swap(T (&lhs)[N], T (&rhs)[N]) noexcept {
    constexpr size_t buffer_bytes = sizeof(lhs);
    unsigned char buffer[buffer_bytes];

    ciel::memcpy(std::addressof(buffer), std::addressof(lhs), buffer_bytes);
    ciel::memmove(std::addressof(lhs), std::addressof(rhs), buffer_bytes);
    ciel::memcpy(std::addressof(rhs), std::addressof(buffer), buffer_bytes);
}

inline void relocatable_swap(void* f1, void* f2, size_t bytes) noexcept {
    constexpr size_t buffer_bytes = 128;
    unsigned char buffer[buffer_bytes];

    unsigned char* first1 = static_cast<unsigned char*>(f1);
    unsigned char* first2 = static_cast<unsigned char*>(f2);

    while (bytes >= buffer_bytes) {
        ciel::memcpy(std::addressof(buffer), first1, buffer_bytes);
        ciel::memmove(first1, first2, buffer_bytes);
        ciel::memcpy(first2, std::addressof(buffer), buffer_bytes);

        first1 += buffer_bytes;
        first2 += buffer_bytes;
        bytes -= buffer_bytes;
    }

    if (bytes != 0) {
        ciel::memcpy(std::addressof(buffer), first1, bytes);
        ciel::memmove(first1, first2, bytes);
        ciel::memcpy(first2, std::addressof(buffer), bytes);
    }
}

NAMESPACE_CIEL_END

namespace std {

#if CIEL_STD_VER < 20
template<class T, ciel::enable_if_t<ciel::is_trivially_relocatable<T>::value> = 0>
T* swap_ranges(T* first1, T* last1, T* first2) noexcept {
    const size_t N          = last1 - first1;
    const size_t swap_bytes = N * sizeof(T);

    ciel::relocatable_swap(first1, first2, swap_bytes);

    return first2 + N;
}
#else
template<class T>
    requires ciel::is_trivially_relocatable<T>::value
void swap(T& a, T& b) noexcept {
    ciel::relocatable_swap(a, b);
}

template<class T, size_t N>
    requires ciel::is_trivially_relocatable<T>::value
void swap(T (&a)[N], T (&b)[N]) noexcept {
    ciel::relocatable_swap(a, b);
}

template<std::contiguous_iterator ForwardIt1, std::contiguous_iterator ForwardIt2,
         class T = typename std::iterator_traits<ForwardIt1>::value_type,
         class U = typename std::iterator_traits<ForwardIt2>::value_type>
    requires std::is_same_v<T, U> && ciel::is_trivially_relocatable<T>::value
ForwardIt2 swap_ranges(ForwardIt1 first1, ForwardIt1 last1, ForwardIt2 first2) noexcept {
    const size_t N          = last1 - first1;
    const size_t swap_bytes = N * sizeof(T);

    ciel::relocatable_swap(std::to_address(first1), std::to_address(first2), swap_bytes);

    return first2 + N;
}
#endif

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_SWAP_HPP_

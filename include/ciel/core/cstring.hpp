#ifndef CIELLAB_INCLUDE_CIEL_CORE_CSTRING_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_CSTRING_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/message.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

// memcpy

inline void memcpy(void* dest, const void* src, const size_t count) noexcept {
    CIEL_ASSERT(dest != nullptr);
    CIEL_ASSERT(src != nullptr);
    CIEL_ASSERT(reinterpret_cast<uintptr_t>(dest) + count <= reinterpret_cast<uintptr_t>(src)
                || reinterpret_cast<uintptr_t>(src) + count <= reinterpret_cast<uintptr_t>(dest));

    std::memcpy(dest, src, count);
}

// memmove

inline void memmove(void* dest, const void* src, const size_t count) noexcept {
    CIEL_ASSERT(dest != nullptr);
    CIEL_ASSERT(src != nullptr);

    std::memmove(dest, src, count);
}

// find
// Corner case:
// Return first1  if substring is empty.
// Return nullptr if substring is not found.

template<class Iter, enable_if_t<std::is_same<Iter, char*>::value || std::is_same<Iter, const char*>::value> = 0>
Iter find(const Iter first1, const char* const last1, const char c) noexcept {
    return static_cast<Iter>(std::memchr(first1, c, last1 - first1));
}

template<class Iter, enable_if_t<std::is_same<Iter, char*>::value || std::is_same<Iter, const char*>::value> = 0>
Iter find(const Iter first1, const char* const last1, const char* const first2, const char* const last2) noexcept {
    const size_t len1 = last1 - first1;
    const size_t len2 = last2 - first2;

    if CIEL_UNLIKELY (len2 == 0) {
        return first1;
    }

    if CIEL_UNLIKELY (len1 < len2) {
        return nullptr;
    }

    auto match = first1;
    while (true) {
        match = ciel::find(match, last1, *first2);
        if (match == nullptr) {
            return nullptr;
        }

        // It's faster to compare from the first byte of first2 even if we already know that it matches,
        // this is because first2 is most likely aligned, as it's user's "pattern" string.
        if (std::memcmp(match, first2, len2) == 0) {
            return match;
        }

        ++match;
    }
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_CSTRING_HPP_

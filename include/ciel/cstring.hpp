#ifndef CIELLAB_INCLUDE_CIEL_CSTRING_HPP_
#define CIELLAB_INCLUDE_CIEL_CSTRING_HPP_

#include <ciel/config.hpp>

#include <cstddef>
#include <cstring>

NAMESPACE_CIEL_BEGIN

// memcpy

inline void
memcpy(void* dest, const void* src, const size_t count) noexcept {
    CIEL_PRECONDITION(dest != nullptr);
    CIEL_PRECONDITION(src != nullptr);
    CIEL_PRECONDITION((uintptr_t)dest + count <= (uintptr_t)src || (uintptr_t)src + count <= (uintptr_t)dest);

    std::memcpy(dest, src, count);
}

// memmove

inline void
memmove(void* dest, const void* src, const size_t count) noexcept {
    CIEL_PRECONDITION(dest != nullptr);
    CIEL_PRECONDITION(src != nullptr);

    std::memmove(dest, src, count);
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CSTRING_HPP_

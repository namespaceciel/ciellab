#ifndef CIELLAB_INCLUDE_CIEL_CORE_CSTRING_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_CSTRING_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/message.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>

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

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_CSTRING_HPP_

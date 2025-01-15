#ifndef CIELLAB_INCLUDE_CIEL_CORE_ALIGNMENT_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_ALIGNMENT_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/message.hpp>

#include <cstddef>
#include <cstdint>

NAMESPACE_CIEL_BEGIN

// is_pow2

CIEL_NODISCARD CIEL_CONSTEXPR_SINCE_CXX20 inline bool is_pow2(const size_t x) noexcept {
    CIEL_PRECONDITION(x != 0);

    return (x & (x - 1)) == 0;
}

// is_aligned

CIEL_NODISCARD CIEL_CONSTEXPR_SINCE_CXX20 inline bool is_aligned(const uintptr_t ptr, const size_t alignment) noexcept {
    CIEL_PRECONDITION(ptr != 0);
    CIEL_PRECONDITION(ciel::is_pow2(alignment));

    return ptr % alignment == 0;
}

CIEL_NODISCARD inline bool is_aligned(const void* ptr, const size_t alignment) noexcept {
    return ciel::is_aligned(reinterpret_cast<uintptr_t>(ptr), alignment);
}

// align_up

CIEL_NODISCARD CIEL_CONSTEXPR_SINCE_CXX20 inline uintptr_t align_up(const uintptr_t sz,
                                                                    const size_t alignment) noexcept {
    CIEL_PRECONDITION(ciel::is_pow2(alignment));

    const uintptr_t mask = alignment - 1;

    return (sz + mask) & ~mask;
}

// align_down

CIEL_NODISCARD CIEL_CONSTEXPR_SINCE_CXX20 inline uintptr_t align_down(const uintptr_t sz,
                                                                      const size_t alignment) noexcept {
    CIEL_PRECONDITION(ciel::is_pow2(alignment));

    const uintptr_t mask = alignment - 1;

    return sz & ~mask;
}

// max_align

static constexpr size_t max_align =
#ifdef __STDCPP_DEFAULT_NEW_ALIGNMENT__
    __STDCPP_DEFAULT_NEW_ALIGNMENT__
#else
    alignof(std::max_align_t)
#endif
    ;

// is_overaligned_for_new

CIEL_NODISCARD inline bool is_overaligned_for_new(const size_t alignment) noexcept {
    return alignment > max_align;
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_ALIGNMENT_HPP_

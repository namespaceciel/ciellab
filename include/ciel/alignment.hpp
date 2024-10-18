#ifndef CIELLAB_INCLUDE_CIEL_ALIGNMENT_HPP_
#define CIELLAB_INCLUDE_CIEL_ALIGNMENT_HPP_

#include <ciel/config.hpp>

#include <cstddef>
#include <cstdint>

NAMESPACE_CIEL_BEGIN

// is_aligned
CIEL_NODISCARD inline bool
is_aligned(void* ptr, const size_t alignment) noexcept {
    CIEL_PRECONDITION(ptr != nullptr);
    CIEL_PRECONDITION(alignment != 0);

    return ((uintptr_t)ptr % alignment) == 0;
}

// align_up
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

// align_down
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

// max_align
static constexpr size_t max_align =
#ifdef __STDCPP_DEFAULT_NEW_ALIGNMENT__
    __STDCPP_DEFAULT_NEW_ALIGNMENT__
#else
    alignof(std::max_align_t)
#endif
    ;

// is_overaligned_for_new
CIEL_NODISCARD inline bool
is_overaligned_for_new(const size_t alignment) noexcept {
    return alignment > max_align;
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_ALIGNMENT_HPP_

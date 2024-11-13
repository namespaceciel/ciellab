#ifndef CIELLAB_INCLUDE_CIEL_MEMORY_HPP_
#define CIELLAB_INCLUDE_CIEL_MEMORY_HPP_

#include <ciel/core/alignment.hpp>
#include <ciel/core/config.hpp>

#include <new>

NAMESPACE_CIEL_BEGIN

// allocate

template<class T>
CIEL_NODISCARD T* allocate(const size_t n) {
#if CIEL_STD_VER >= 17
    if CIEL_UNLIKELY (ciel::is_overaligned_for_new(alignof(T))) {
        return static_cast<T*>(::operator new(sizeof(T) * n, static_cast<std::align_val_t>(alignof(T))));
    }
#endif
    return static_cast<T*>(::operator new(sizeof(T) * n));
}

// deallocate

template<class T>
void deallocate(T* ptr) noexcept {
#if CIEL_STD_VER >= 17
    if CIEL_UNLIKELY (ciel::is_overaligned_for_new(alignof(T))) {
        ::operator delete(ptr, static_cast<std::align_val_t>(alignof(T)));
    }
#endif
    ::operator delete(ptr);
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_MEMORY_HPP_

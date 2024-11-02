#ifndef CIELLAB_INCLUDE_CIEL_SAFE_ALLOCATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_SAFE_ALLOCATOR_HPP_

#include <ciel/config.hpp>

#include <cstddef>
#include <cstring>
#include <memory>

NAMESPACE_CIEL_BEGIN

template<class T>
class safe_allocator {
public:
    using value_type = T;

    safe_allocator() noexcept = default;

    template<class U>
    safe_allocator(safe_allocator<U>) noexcept {}

    CIEL_NODISCARD T*
    allocate(const size_t n) {
        T* memory = std::allocator<T>().allocate(n);
        std::memset(memory, 0, sizeof(T) * n);

        return memory;
    }

    void
    deallocate(T* p, const size_t n) noexcept {
        std::memset(p, 0, sizeof(T) * n);
        std::allocator<T>().deallocate(p, n);
    }

    CIEL_NODISCARD friend bool
    operator==(safe_allocator, safe_allocator) noexcept {
        return true;
    }
}; // class safe_allocator

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_SAFE_ALLOCATOR_HPP_

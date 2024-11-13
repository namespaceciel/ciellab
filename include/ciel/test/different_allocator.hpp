#ifndef CIELLAB_INCLUDE_CIEL_TEST_DIFFERENT_ALLOCATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_TEST_DIFFERENT_ALLOCATOR_HPP_

#include <ciel/core/config.hpp>

#include <memory>

NAMESPACE_CIEL_BEGIN

template<class T>
class different_allocator {
public:
    using value_type = T;

    different_allocator() = default;

    template<class U>
    different_allocator(different_allocator<U>) noexcept {}

    CIEL_NODISCARD T* allocate(ptrdiff_t n) {
        return std::allocator<T>().allocate(n);
    }

    void deallocate(T* p, ptrdiff_t n) noexcept {
        std::allocator<T>().deallocate(p, n);
    }

    CIEL_NODISCARD friend bool operator==(different_allocator, different_allocator) noexcept {
        return false;
    }

}; // class different_allocator

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TEST_DIFFERENT_ALLOCATOR_HPP_

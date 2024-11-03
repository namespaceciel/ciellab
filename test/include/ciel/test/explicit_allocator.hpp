#ifndef CIELLAB_INCLUDE_CIEL_EXPLICIT_ALLOCATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_EXPLICIT_ALLOCATOR_HPP_

#include <ciel/config.hpp>

#include <cstddef>
#include <memory>

NAMESPACE_CIEL_BEGIN

template<class T>
class explicit_allocator {
public:
    using value_type = T;

    explicit_allocator() noexcept = default;

    template<class U>
    explicit explicit_allocator(explicit_allocator<U>) noexcept {}

    CIEL_NODISCARD T*
    allocate(const size_t n) {
        return static_cast<T*>(std::allocator<T>().allocate(n));
    }

    void
    deallocate(T* p, const size_t n) {
        std::allocator<T>().deallocate(p, n);
    }

    CIEL_NODISCARD friend bool
    operator==(explicit_allocator, explicit_allocator) {
        return true;
    }

}; // class explicit_allocator

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_EXPLICIT_ALLOCATOR_HPP_

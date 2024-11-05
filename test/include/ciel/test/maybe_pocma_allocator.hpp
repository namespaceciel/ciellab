#ifndef CIELLAB_INCLUDE_CIEL_MAYBE_POCMA_ALLOCATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_MAYBE_POCMA_ALLOCATOR_HPP_

#include <ciel/config.hpp>

#include <cstddef>
#include <memory>

NAMESPACE_CIEL_BEGIN

template<class T, bool POCMAValue>
class maybe_pocma_allocator {
private:
    int id_ = 0;

    template<class, bool>
    friend class maybe_pocma_allocator;

public:
    using value_type                             = T;
    using propagate_on_container_copy_assignment = std::integral_constant<bool, POCMAValue>;

    template<class U>
    struct rebind {
        using other = maybe_pocma_allocator<U, POCMAValue>;
    };

    maybe_pocma_allocator() noexcept = default;

    maybe_pocma_allocator(const int id) noexcept
        : id_(id) {}

    maybe_pocma_allocator(const maybe_pocma_allocator&) noexcept = default;

    template<class U>
    maybe_pocma_allocator(const maybe_pocma_allocator<U, POCMAValue>& that) noexcept
        : id_(that.id_) {}

    maybe_pocma_allocator&
    operator=(const maybe_pocma_allocator& a) noexcept {
        id_ = a.id();

        return *this;
    }

    CIEL_NODISCARD T*
    allocate(const size_t n) {
        return std::allocator<T>().allocate(n);
    }

    void
    deallocate(T* ptr, const size_t n) noexcept {
        std::allocator<T>().deallocate(ptr, n);
    }

    CIEL_NODISCARD int
    id() const noexcept {
        return id_;
    }

    template<class U>
    CIEL_NODISCARD friend bool
    operator==(const maybe_pocma_allocator& lhs, const maybe_pocma_allocator<U, POCMAValue>& rhs) noexcept {
        return lhs.id() == rhs.id();
    }

}; // class maybe_pocma_allocator

template<class T>
using pocma_allocator = maybe_pocma_allocator<T, true>;
template<class T>
using non_pocma_allocator = maybe_pocma_allocator<T, false>;

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_MAYBE_POCMA_ALLOCATOR_HPP_

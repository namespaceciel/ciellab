#ifndef CIELLAB_INCLUDE_CIEL_MAYBE_POCCA_ALLOCATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_MAYBE_POCCA_ALLOCATOR_HPP_

#include <ciel/config.hpp>

#include <cstddef>
#include <memory>

NAMESPACE_CIEL_BEGIN

template<class T, bool POCCAValue>
class maybe_pocca_allocator {
private:
    int id_ = 0;

    template<class, bool>
    friend class maybe_pocca_allocator;

public:
    using value_type                             = T;
    using propagate_on_container_copy_assignment = std::integral_constant<bool, POCCAValue>;

    template<class U>
    struct rebind {
        using other = maybe_pocca_allocator<U, POCCAValue>;
    };

    maybe_pocca_allocator() noexcept = default;

    maybe_pocca_allocator(const int id) noexcept
        : id_(id) {}

    maybe_pocca_allocator(const maybe_pocca_allocator&) noexcept = default;

    template<class U>
    maybe_pocca_allocator(const maybe_pocca_allocator<U, POCCAValue>& that) noexcept
        : id_(that.id_) {}

    maybe_pocca_allocator&
    operator=(const maybe_pocca_allocator& a) noexcept {
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
    operator==(const maybe_pocca_allocator& lhs, const maybe_pocca_allocator<U, POCCAValue>& rhs) noexcept {
        return !POCCAValue || lhs.id() == rhs.id();
    }

}; // class maybe_pocca_allocator

template<class T>
using pocca_allocator = maybe_pocca_allocator<T, true>;
template<class T>
using non_pocca_allocator = maybe_pocca_allocator<T, false>;

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_MAYBE_POCCA_ALLOCATOR_HPP_

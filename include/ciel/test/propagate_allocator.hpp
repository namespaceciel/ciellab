#ifndef CIELLAB_INCLUDE_CIEL_TEST_MAYBE_PROPAGATE_ALLOCATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_TEST_MAYBE_PROPAGATE_ALLOCATOR_HPP_

#include <ciel/core/config.hpp>

#include <cstddef>
#include <memory>

NAMESPACE_CIEL_BEGIN

template<class T, bool POCCAValue, bool POCMAValue, bool POCSValue>
class propagate_allocator {
private:
    int id_ = 0;

    template<class, bool, bool, bool>
    friend class propagate_allocator;

public:
    using value_type                             = T;
    using propagate_on_container_copy_assignment = bool_constant<POCCAValue>;
    using propagate_on_container_move_assignment = bool_constant<POCMAValue>;
    using propagate_on_container_swap            = bool_constant<POCSValue>;

    template<class U>
    struct rebind {
        using other = propagate_allocator<U, POCCAValue, POCMAValue, POCSValue>;
    };

    propagate_allocator() = default;

    propagate_allocator(const int id) noexcept
        : id_(id) {}

    propagate_allocator(const propagate_allocator&) = default;

    template<class U>
    propagate_allocator(const propagate_allocator<U, POCCAValue, POCMAValue, POCSValue>& that) noexcept
        : id_(that.id_) {}

    propagate_allocator& operator=(const propagate_allocator& a) = default;

    CIEL_NODISCARD T* allocate(const size_t n) {
        return std::allocator<T>().allocate(n);
    }

    void deallocate(T* ptr, const size_t n) noexcept {
        std::allocator<T>().deallocate(ptr, n);
    }

    CIEL_NODISCARD int id() const noexcept {
        return id_;
    }

    template<class U>
    CIEL_NODISCARD friend bool operator==(
        const propagate_allocator& lhs, const propagate_allocator<U, POCCAValue, POCMAValue, POCSValue>& rhs) noexcept {
        return lhs.id() == rhs.id();
    }

}; // class propagate_allocator

template<class T>
using pocca_allocator = propagate_allocator<T, true, false, false>;
template<class T>
using non_pocca_allocator = propagate_allocator<T, false, false, false>;

template<class T>
using pocma_allocator = propagate_allocator<T, false, true, false>;
template<class T>
using non_pocma_allocator = propagate_allocator<T, false, false, false>;

template<class T>
using pocs_allocator = propagate_allocator<T, false, false, true>;
template<class T>
using non_pocs_allocator = propagate_allocator<T, false, false, false>;

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TEST_MAYBE_PROPAGATE_ALLOCATOR_HPP_

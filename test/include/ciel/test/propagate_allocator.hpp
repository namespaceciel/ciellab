#ifndef CIELLAB_INCLUDE_CIEL_MAYBE_PROPAGATE_ALLOCATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_MAYBE_PROPAGATE_ALLOCATOR_HPP_

#include <ciel/config.hpp>

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
    using propagate_on_container_copy_assignment = std::integral_constant<bool, POCCAValue>;
    using propagate_on_container_move_assignment = std::integral_constant<bool, POCMAValue>;
    using propagate_on_container_swap            = std::integral_constant<bool, POCSValue>;

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

    // clang-format off
    propagate_allocator& operator=(const propagate_allocator& a) = default;
    // clang-format on

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
    operator==(const propagate_allocator& lhs,
               const propagate_allocator<U, POCCAValue, POCMAValue, POCSValue>& rhs) noexcept {
        return !POCCAValue || !POCMAValue || !POCSValue || lhs.id() == rhs.id();
    }

}; // class propagate_allocator

template<class T>
using pocca_allocator = propagate_allocator<T, true, true, true>;
template<class T>
using non_pocca_allocator = propagate_allocator<T, false, true, true>;

template<class T>
using pocma_allocator = propagate_allocator<T, true, true, true>;
template<class T>
using non_pocma_allocator = propagate_allocator<T, true, false, true>;

template<class T>
using pocs_allocator = propagate_allocator<T, true, true, true>;
template<class T>
using non_pocs_allocator = propagate_allocator<T, true, true, false>;

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_MAYBE_PROPAGATE_ALLOCATOR_HPP_

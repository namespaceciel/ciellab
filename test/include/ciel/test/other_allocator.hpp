#ifndef CIELLAB_INCLUDE_CIEL_OTHER_ALLOCATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_OTHER_ALLOCATOR_HPP_

#include <ciel/config.hpp>

#include <cstddef>
#include <memory>

NAMESPACE_CIEL_BEGIN

template<class T>
class other_allocator {
    int data_ = -1;

    template<class U>
    friend class other_allocator;

public:
    using value_type                             = T;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap            = std::true_type;

    other_allocator() noexcept = default;

    explicit other_allocator(int i) noexcept
        : data_(i) {}

    template<class U>
    other_allocator(const other_allocator<U>& a) noexcept
        : data_(a.data_) {}

    CIEL_NODISCARD T*
    allocate(size_t n) {
        return std::allocator<value_type>().allocate(n);
    }

    void
    deallocate(T* p, size_t s) noexcept {
        std::allocator<value_type>().deallocate(p, s);
    }

    CIEL_NODISCARD other_allocator
    select_on_container_copy_construction() const noexcept {
        return other_allocator(-2);
    }

    CIEL_NODISCARD int
    get_data() const noexcept {
        return data_;
    }

    CIEL_NODISCARD friend bool
    operator==(const other_allocator& x, const other_allocator& y) noexcept {
        return x.data_ == y.data_;
    }

}; // class other_allocator

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_OTHER_ALLOCATOR_HPP_

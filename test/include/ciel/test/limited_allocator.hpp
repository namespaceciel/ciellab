#ifndef CIELLAB_INCLUDE_CIEL_LIMITED_ALLOCATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_LIMITED_ALLOCATOR_HPP_

#include <ciel/config.hpp>

#include <cstddef>
#include <memory>

NAMESPACE_CIEL_BEGIN

template<size_t MaxAllocs>
struct limited_alloc_handle {
    size_t outstanding_ = 0;
    void* last_alloc_   = nullptr;

    template<class T>
    CIEL_NODISCARD T*
    allocate(size_t N) {
        if (N + outstanding_ > MaxAllocs) {
            CIEL_THROW_EXCEPTION(std::bad_alloc());
        }

        auto alloc  = std::allocator<T>().allocate(N);
        last_alloc_ = alloc;
        outstanding_ += N;

        return alloc;
    }

    template<class T>
    void
    deallocate(T* ptr, size_t N) noexcept {
        if (ptr == last_alloc_) {
            last_alloc_ = nullptr;
            CIEL_PRECONDITION(outstanding_ >= N);
            outstanding_ -= N;
        }

        std::allocator<T>().deallocate(ptr, N);
    }

}; // struct limited_alloc_handle

template<class T, size_t N>
class limited_allocator {
    template<class U, size_t UN>
    friend class limited_allocator;

    using BuffT = limited_alloc_handle<N>;

    std::shared_ptr<BuffT> handle_;

public:
    using value_type      = T;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

    template<class U>
    struct rebind {
        typedef limited_allocator<U, N> other;
    };

    limited_allocator()
        : handle_(std::make_shared<BuffT>()) {}

    limited_allocator(const limited_allocator&) noexcept = default;

    template<class U>
    explicit limited_allocator(const limited_allocator<U, N>& other) noexcept
        : handle_(other.handle_) {}

    // clang-format off
    limited_allocator& operator=(const limited_allocator&) = delete;
    // clang-format on

    CIEL_NODISCARD pointer
    allocate(size_type n) {
        return handle_->template allocate<T>(n);
    }

    void
    deallocate(pointer p, size_type n) noexcept {
        handle_->template deallocate<T>(p, n);
    }

    CIEL_NODISCARD size_type
    max_size() const noexcept {
        return N;
    }

    CIEL_NODISCARD BuffT*
    getHandle() const noexcept {
        return handle_.get();
    }

    CIEL_NODISCARD friend bool
    operator==(limited_allocator, limited_allocator) noexcept {
        return true;
    }

}; // class limited_allocator

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_LIMITED_ALLOCATOR_HPP_

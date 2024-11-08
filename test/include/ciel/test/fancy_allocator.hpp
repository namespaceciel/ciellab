#ifndef CIELLAB_INCLUDE_CIEL_FANCY_ALLOCATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_FANCY_ALLOCATOR_HPP_

#include <ciel/compare.hpp>
#include <ciel/config.hpp>

#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

template<class T>
class min_pointer;
template<class T>
class min_pointer<const T>;
template<>
class min_pointer<void>;
template<>
class min_pointer<const void>;
template<class>
class fancy_allocator;

template<>
class min_pointer<void> {
    void* ptr_;

    template<class>
    friend class min_pointer;

public:
    min_pointer() noexcept = default;

    min_pointer(nullptr_t) noexcept
        : ptr_(nullptr) {}

    template<class T, enable_if_t<!std::is_const<T>::value> = 0>
    min_pointer(min_pointer<T> p) noexcept
        : ptr_(p.ptr_) {}

    CIEL_NODISCARD explicit
    operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    CIEL_NODISCARD friend bool
    operator==(min_pointer x, min_pointer y) noexcept {
        return x.ptr_ == y.ptr_;
    }

}; // class min_pointer<void>

template<>
class min_pointer<const void> {
    const void* ptr_;

    template<class>
    friend class min_pointer;

public:
    min_pointer() noexcept = default;

    min_pointer(nullptr_t) noexcept
        : ptr_(nullptr) {}

    template<class T>
    min_pointer(min_pointer<T> p) noexcept
        : ptr_(p.ptr_) {}

    CIEL_NODISCARD explicit
    operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    CIEL_NODISCARD friend bool
    operator==(min_pointer x, min_pointer y) noexcept {
        return x.ptr_ == y.ptr_;
    }

}; // class min_pointer<const void>

template<class T>
class min_pointer {
    T* ptr_;

    explicit min_pointer(T* p) noexcept
        : ptr_(p) {}

    template<class>
    friend class min_pointer;
    template<class>
    friend class fancy_allocator;

public:
    using value_type        = T;
    using pointer           = T*;
    using reference         = T&;
    using difference_type   = ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

    min_pointer() noexcept = default;

    min_pointer(nullptr_t) noexcept
        : ptr_(nullptr) {}

    explicit min_pointer(min_pointer<void> p) noexcept
        : ptr_(static_cast<T*>(p.ptr_)) {}

    CIEL_NODISCARD explicit
    operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    CIEL_NODISCARD reference
    operator*() const noexcept {
        return *ptr_;
    }

    CIEL_NODISCARD pointer
    operator->() const noexcept {
        return ptr_;
    }

    min_pointer&
    operator++() noexcept {
        ++ptr_;
        return *this;
    }

    CIEL_NODISCARD min_pointer
    operator++(int) noexcept {
        min_pointer tmp(*this);
        ++(*this);
        return tmp;
    }

    min_pointer&
    operator--() noexcept {
        --ptr_;
        return *this;
    }

    CIEL_NODISCARD min_pointer
    operator--(int) noexcept {
        min_pointer tmp(*this);
        --(*this);
        return tmp;
    }

    min_pointer&
    operator+=(difference_type n) noexcept {
        ptr_ += n;
        return *this;
    }

    min_pointer&
    operator-=(difference_type n) noexcept {
        (*this) += (-n);
        return *this;
    }

    CIEL_NODISCARD min_pointer
    operator+(difference_type n) const noexcept {
        min_pointer tmp(*this);
        tmp += n;
        return tmp;
    }

    CIEL_NODISCARD friend min_pointer
    operator+(difference_type n, min_pointer x) noexcept {
        return x + n;
    }

    CIEL_NODISCARD min_pointer
    operator-(difference_type n) const noexcept {
        min_pointer tmp(*this);
        tmp -= n;
        return tmp;
    }

    CIEL_NODISCARD friend difference_type
    operator-(min_pointer x, min_pointer y) noexcept {
        return x.ptr_ - y.ptr_;
    }

    CIEL_NODISCARD reference
    operator[](difference_type n) const noexcept {
        return ptr_[n];
    }

    CIEL_NODISCARD static min_pointer
    pointer_to(T& t) noexcept {
        return min_pointer(std::addressof(t));
    }

    CIEL_NODISCARD friend bool
    operator==(min_pointer x, min_pointer y) noexcept {
        return x.ptr_ == y.ptr_;
    }

    CIEL_NODISCARD friend bool
    operator<(min_pointer x, min_pointer y) noexcept {
        return x.ptr_ < y.ptr_;
    }

}; // class min_pointer

template<class T>
class min_pointer<const T> {
    const T* ptr_;

    explicit min_pointer(const T* p)
        : ptr_(p) {}

    template<class>
    friend class min_pointer;

public:
    using value_type        = const T;
    using pointer           = const T*;
    using reference         = const T&;
    using difference_type   = ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

    min_pointer() noexcept = default;

    min_pointer(nullptr_t) noexcept
        : ptr_(nullptr) {}

    min_pointer(min_pointer<T> p) noexcept
        : ptr_(p.ptr_) {}

    explicit min_pointer(min_pointer<const void> p) noexcept
        : ptr_(static_cast<const T*>(p.ptr_)) {}

    CIEL_NODISCARD explicit
    operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    CIEL_NODISCARD reference
    operator*() const noexcept {
        return *ptr_;
    }

    CIEL_NODISCARD pointer
    operator->() const noexcept {
        return ptr_;
    }

    min_pointer&
    operator++() noexcept {
        ++ptr_;
        return *this;
    }

    CIEL_NODISCARD min_pointer
    operator++(int) noexcept {
        min_pointer tmp(*this);
        ++(*this);
        return tmp;
    }

    min_pointer&
    operator--() noexcept {
        --ptr_;
        return *this;
    }

    CIEL_NODISCARD min_pointer
    operator--(int) noexcept {
        min_pointer tmp(*this);
        --(*this);
        return tmp;
    }

    min_pointer&
    operator+=(difference_type n) noexcept {
        ptr_ += n;
        return *this;
    }

    min_pointer&
    operator-=(difference_type n) noexcept {
        (*this) += (-n);
        return *this;
    }

    CIEL_NODISCARD min_pointer
    operator+(difference_type n) const noexcept {
        min_pointer tmp(*this);
        tmp += n;
        return tmp;
    }

    CIEL_NODISCARD friend min_pointer
    operator+(difference_type n, min_pointer x) noexcept {
        return x + n;
    }

    CIEL_NODISCARD min_pointer
    operator-(difference_type n) const noexcept {
        min_pointer tmp(*this);
        tmp -= n;
        return tmp;
    }

    CIEL_NODISCARD friend difference_type
    operator-(min_pointer x, min_pointer y) noexcept {
        return x.ptr_ - y.ptr_;
    }

    CIEL_NODISCARD reference
    operator[](difference_type n) const noexcept {
        return ptr_[n];
    }

    CIEL_NODISCARD friend bool
    operator<(min_pointer x, min_pointer y) noexcept {
        return x.ptr_ < y.ptr_;
    }

    CIEL_NODISCARD static min_pointer
    pointer_to(const T& t) noexcept {
        return min_pointer(std::addressof(t));
    }

    CIEL_NODISCARD friend bool
    operator==(min_pointer x, min_pointer y) noexcept {
        return x.ptr_ == y.ptr_;
    }

    CIEL_NODISCARD friend bool
    operator==(min_pointer x, nullptr_t) noexcept {
        return x.ptr_ == nullptr;
    }

    CIEL_NODISCARD friend bool
    operator==(nullptr_t, min_pointer x) noexcept {
        return x.ptr_ == nullptr;
    }

}; // class min_pointer<const T>

template<class T>
class fancy_allocator {
public:
    using value_type = T;
    using pointer    = min_pointer<T>;

    explicit fancy_allocator() noexcept = default;

    template<class U>
    explicit fancy_allocator(fancy_allocator<U>) noexcept {}

    CIEL_NODISCARD pointer
    allocate(ptrdiff_t n) {
        T* memory = std::allocator<T>().allocate(n);
        std::memset((void*)memory, 0, sizeof(T) * n);

        return pointer(memory);
    }

    void
    deallocate(pointer p, ptrdiff_t n) noexcept {
        std::memset((void*)p.ptr_, 0, sizeof(T) * n);
        std::allocator<T>().deallocate(p.ptr_, n);
    }

    CIEL_NODISCARD friend bool
    operator==(fancy_allocator, fancy_allocator) noexcept {
        return true;
    }

}; // class fancy_allocator

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_FANCY_ALLOCATOR_HPP_

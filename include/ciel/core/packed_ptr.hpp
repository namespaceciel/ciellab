#ifndef CIELLAB_INCLUDE_CIEL_CORE_PACKED_PTR_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_PACKED_PTR_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/message.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>

NAMESPACE_CIEL_BEGIN

template<class T>
struct alignas(size_t) packed_ptr {
private:
    uintptr_t ptr_ : 48;
    size_t count_  : 16;

public:
    packed_ptr(T* ptr = nullptr, const size_t count = 0) noexcept
        : ptr_(reinterpret_cast<uintptr_t>(ptr)), count_(count) {
        CIEL_PRECONDITION(reinterpret_cast<uintptr_t>(ptr) < (1ULL << 48));
        CIEL_PRECONDITION(count < (1ULL << 16));
    }

    CIEL_NODISCARD T* ptr() const noexcept {
        return reinterpret_cast<T*>(ptr_);
    }

    CIEL_NODISCARD size_t count() const noexcept {
        return count_;
    }

    void set_ptr(T* ptr) noexcept {
        ptr_ = reinterpret_cast<uintptr_t>(ptr);
    }

    void set_count(const size_t count) noexcept {
        count_ = count;
    }

    void increment_count() noexcept {
        ++count_;
    }

    void decrement_count() noexcept {
        --count_;
    }

    CIEL_NODISCARD friend bool operator==(const packed_ptr& lhs, const packed_ptr& rhs) noexcept {
        return lhs.ptr() == rhs.ptr() && lhs.count() == rhs.count();
    }

    CIEL_NODISCARD friend bool operator!=(const packed_ptr& lhs, const packed_ptr& rhs) noexcept {
        return !(lhs == rhs);
    }

}; // struct packed_ptr

template<class T>
using atomic_packed_ptr = std::atomic<packed_ptr<T>>;

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_PACKED_PTR_HPP_

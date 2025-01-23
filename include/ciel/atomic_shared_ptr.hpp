#ifndef CIELLAB_INCLUDE_CIEL_ATOMIC_SHARED_PTR_HPP_
#define CIELLAB_INCLUDE_CIEL_ATOMIC_SHARED_PTR_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/exchange.hpp>
#include <ciel/core/message.hpp>
#include <ciel/core/spinlock_ptr.hpp>
#include <ciel/shared_ptr.hpp>

#include <atomic>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

// Inspired by GNU libstdc++'s implementation.

template<class T>
class atomic_shared_ptr {
private:
    using element_type = remove_extent_t<T>;
    using pointer      = element_type*;

    pointer ptr_{nullptr};
    mutable spinlock_ptr<control_block_base> control_block_{nullptr};

public:
    using value_type = shared_ptr<T>;

public:
    atomic_shared_ptr() = default;

    atomic_shared_ptr(nullptr_t) noexcept {}

    atomic_shared_ptr(value_type desired) noexcept
        : ptr_(ciel::exchange(desired.ptr_, nullptr)),
          control_block_(ciel::exchange(desired.control_block_, nullptr)) {}

    atomic_shared_ptr(const atomic_shared_ptr&)            = delete;
    atomic_shared_ptr& operator=(const atomic_shared_ptr&) = delete;

    ~atomic_shared_ptr() {
        store(nullptr);
    }

    atomic_shared_ptr& operator=(value_type desired) noexcept {
        store(std::move(desired));
        return *this;
    }

    atomic_shared_ptr& operator=(nullptr_t) noexcept {
        store(nullptr);
        return *this;
    }

    CIEL_NODISCARD bool is_lock_free() const noexcept {
        return false;
    }

    void store(value_type desired, const std::memory_order order = std::memory_order_seq_cst) noexcept {
        CIEL_ASSERT(order != std::memory_order_consume);
        CIEL_ASSERT(order != std::memory_order_acquire);
        CIEL_ASSERT(order != std::memory_order_acq_rel);

        CIEL_UNUSED(exchange(std::move(desired), order));
    }

    CIEL_NODISCARD value_type load(std::memory_order order = std::memory_order_seq_cst) const noexcept {
        CIEL_ASSERT(order != std::memory_order_release);
        CIEL_ASSERT(order != std::memory_order_acq_rel);

        if (order != std::memory_order_seq_cst) {
            order = std::memory_order_acquire;
        }

        control_block_base* cb = control_block_.lock(order);
        if (cb) {
            cb->shared_add_ref();
        }
        value_type res{ptr_, cb};
        control_block_.unlock(std::memory_order_relaxed);

        return res;
    }

    CIEL_NODISCARD operator value_type() const noexcept {
        return load();
    }

    CIEL_NODISCARD value_type exchange(value_type desired,
                                       const std::memory_order order = std::memory_order_seq_cst) noexcept {
        CIEL_UNUSED(control_block_.lock(std::memory_order_acquire));
        std::swap(ptr_, desired.ptr_);
        control_block_.swap_unlock(desired.control_block_, order);
        return desired;
    }

    CIEL_NODISCARD bool compare_exchange_strong(value_type& expected, value_type desired,
                                                const std::memory_order success,
                                                const std::memory_order failure) noexcept {
        CIEL_ASSERT(failure != std::memory_order_release);
        CIEL_ASSERT(failure != std::memory_order_acq_rel);

        control_block_base* cb = control_block_.lock(std::memory_order_acquire);
        if (ptr_ == expected.ptr_ && cb == expected.control_block_) {
            std::swap(ptr_, desired.ptr_);
            control_block_.swap_unlock(desired.control_block_, success);
            return true;
        }

        expected = value_type{ptr_, cb};
        if (cb) {
            cb->shared_add_ref();
        }
        control_block_.unlock(failure);
        return false;
    }

    CIEL_NODISCARD bool compare_exchange_weak(value_type& expected, value_type desired, const std::memory_order success,
                                              const std::memory_order failure) noexcept {
        return compare_exchange_strong(expected, std::move(desired), success, failure);
    }

    CIEL_NODISCARD bool compare_exchange_strong(value_type& expected, value_type desired,
                                                std::memory_order order = std::memory_order_seq_cst) noexcept {
        switch (order) {
            case std::memory_order_acq_rel :
                return compare_exchange_strong(expected, std::move(desired), order, std::memory_order_acquire);
            case std::memory_order_release :
                return compare_exchange_strong(expected, std::move(desired), order, std::memory_order_relaxed);
            default :
                return compare_exchange_strong(expected, std::move(desired), order, order);
        }
    }

    CIEL_NODISCARD bool compare_exchange_weak(value_type& expected, value_type desired,
                                              std::memory_order order = std::memory_order_seq_cst) noexcept {
        return compare_exchange_strong(expected, std::move(desired), order);
    }

#if CIEL_STD_VER >= 17
    static constexpr bool is_always_lock_free = false;
#endif

}; // class atomic_shared_ptr

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_ATOMIC_SHARED_PTR_HPP_

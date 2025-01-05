#ifndef CIELLAB_INCLUDE_CIEL_CORE_SPINLOCK_PTR_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_SPINLOCK_PTR_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/message.hpp>

#include <atomic>
#include <cstdint>
#include <thread>

NAMESPACE_CIEL_BEGIN

// Inspired by GNU libstdc++'s implementation.

template<class T>
class spinlock_ptr {
    static_assert(alignof(T) > 1, "We can't use the LSB as the lock bit if alignof(T) == 1");

private:
    using pointer = T*;

    mutable std::atomic<uintptr_t> ptr_{0};

    static constexpr uintptr_t lock_bit{1};

public:
    spinlock_ptr() = default;

    spinlock_ptr(nullptr_t) noexcept {}

    spinlock_ptr(pointer ptr) noexcept
        : ptr_(reinterpret_cast<uintptr_t>(ptr)) {}

    spinlock_ptr(const spinlock_ptr&)            = delete;
    spinlock_ptr& operator=(const spinlock_ptr&) = delete;

    ~spinlock_ptr() {
        CIEL_PRECONDITION(!is_locked());
    }

    CIEL_NODISCARD bool is_locked(const uintptr_t value) const noexcept {
        return value & lock_bit;
    }

    CIEL_NODISCARD bool is_locked() const noexcept {
        return ptr_.load(std::memory_order_relaxed) & lock_bit;
    }

    CIEL_NODISCARD pointer lock(const std::memory_order order = std::memory_order_seq_cst) const noexcept {
        uintptr_t cur;
        do {
            while (is_locked(cur = ptr_.load(std::memory_order_relaxed))) {
                std::this_thread::yield();
            }

        } while (!ptr_.compare_exchange_strong(cur, cur | lock_bit, order, std::memory_order_relaxed));

        return reinterpret_cast<pointer>(cur);
    }

    void unlock(const std::memory_order order = std::memory_order_seq_cst) const noexcept {
        CIEL_PRECONDITION(is_locked());

        ptr_.fetch_sub(1, order);
    }

    void swap_unlock(pointer& p, std::memory_order order = std::memory_order_seq_cst) const noexcept {
        CIEL_PRECONDITION(is_locked());

        if (order != std::memory_order_seq_cst) {
            order = std::memory_order_release;
        }

        uintptr_t temp = reinterpret_cast<uintptr_t>(p);
        temp           = ptr_.exchange(temp, order);
        p              = reinterpret_cast<pointer>(temp & ~lock_bit);
    }

    CIEL_NODISCARD pointer ptr() const noexcept {
        CIEL_PRECONDITION(is_locked());

        uintptr_t cur = ptr_.load(std::memory_order_relaxed);
        --cur;
        return reinterpret_cast<pointer>(cur);
    }

    void store(pointer p, std::memory_order order = std::memory_order_seq_cst) const noexcept {
        CIEL_PRECONDITION(is_locked());

        uintptr_t temp = reinterpret_cast<uintptr_t>(p);
        ++temp;
        ptr_.store(temp, order);
    }

}; // class spinlock_ptr

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_SPINLOCK_PTR_HPP_

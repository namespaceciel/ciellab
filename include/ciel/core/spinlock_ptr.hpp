#ifndef CIELLAB_INCLUDE_CIEL_CORE_SPINLOCK_PTR_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_SPINLOCK_PTR_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/message.hpp>

#include <atomic>
#include <cstdint>

NAMESPACE_CIEL_BEGIN

template<class T>
class spinlock_ptr {
    static_assert(alignof(T) > 1, "We can't use the LSB as the lock bit if alignof(T) == 1");

private:
    mutable std::atomic<uintptr_t> ptr_{0};

    static constexpr uintptr_t lock_bit{1};

public:
    spinlock_ptr() = default;

    spinlock_ptr(nullptr_t) noexcept {}

    spinlock_ptr(T* ptr) noexcept
        : ptr_(reinterpret_cast<uintptr_t>(ptr)) {}

    spinlock_ptr(const spinlock_ptr&)            = delete;
    spinlock_ptr& operator=(const spinlock_ptr&) = delete;

    ~spinlock_ptr() {
        CIEL_PRECONDITION(!is_locked(ptr_.load(std::memory_order_relaxed)));
    }

    CIEL_NODISCARD bool is_locked(const uintptr_t value) const noexcept {
        return value & lock_bit;
    }

    CIEL_NODISCARD T* lock(const std::memory_order mo = std::memory_order_seq_cst) const noexcept {
        uintptr_t cur = ptr_.load(std::memory_order_relaxed);
        while (is_locked(cur)) {
            ciel::yield();
            cur = ptr_.load(std::memory_order_relaxed);
        }

        while (!ptr_.compare_exchange_strong(cur, cur | lock_bit, mo, std::memory_order_relaxed)) {
            ciel::yield();
            cur &= ~lock_bit; // Clear LSB if exists.
        }

        return reinterpret_cast<T*>(cur);
    }

    void unlock(const std::memory_order mo = std::memory_order_seq_cst) const noexcept {
        CIEL_PRECONDITION(is_locked(ptr_.load(std::memory_order_relaxed)));

        ptr_.fetch_sub(1, mo);
    }

}; // class spinlock_ptr

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_SPINLOCK_PTR_HPP_

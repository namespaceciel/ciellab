#ifndef CIELLAB_INCLUDE_CIEL_CORE_SPINLOCK_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_SPINLOCK_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/finally.hpp>
#include <ciel/core/message.hpp>

#include <atomic>
#include <thread>

NAMESPACE_CIEL_BEGIN

class spinlock {
private:
    std::atomic<bool> flag_{false};

public:
    CIEL_NODISCARD bool is_locked(std::memory_order order = std::memory_order_seq_cst) const noexcept {
        return flag_.load(order);
    }

    void lock(std::memory_order order = std::memory_order_seq_cst) {
        do {
            while (is_locked(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
        } while (flag_.exchange(true, order));
    }

    void unlock(std::memory_order order = std::memory_order_seq_cst) {
        CIEL_PRECONDITION(is_locked(std::memory_order_relaxed));

        flag_.store(false, order);
    }

}; // class spinlock

template<class F>
void with(spinlock& lock, F&& f) {
    lock.lock(std::memory_order_acquire);
    CIEL_DEFER({ lock.unlock(std::memory_order_release); });

    f();
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_SPINLOCK_HPP_

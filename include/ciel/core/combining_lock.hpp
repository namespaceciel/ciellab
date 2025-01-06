#ifndef CIELLAB_INCLUDE_CIEL_CORE_COMBINING_LOCK_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_COMBINING_LOCK_HPP_

#include <ciel/core/config.hpp>

#include <atomic>
#include <thread>

NAMESPACE_CIEL_BEGIN

// Inspired by snmalloc's doc: https://github.com/microsoft/snmalloc/blob/main/docs/combininglock.md
// Exception throwing will result in dead lock, so functions are not supposed to.

class combining_lock_node;

class combining_lock {
    std::atomic<combining_lock_node*> last{nullptr};
    // Lock would only be held by an entire queue or each fast path threads.
    std::atomic<bool> flag{false};

    friend class combining_lock_node;
    template<class F>
    friend void with(combining_lock&, F&&) noexcept;

}; // class combining_lock

class combining_lock_node {
    enum struct lock_status {
        Waiting,
        Done,
        Head

    }; // enum struct lock_status

    std::atomic<lock_status> status{lock_status::Waiting};
    std::atomic<combining_lock_node*> next{nullptr};
    void (*f_)(combining_lock_node*);

    combining_lock_node(void (*f)(combining_lock_node*)) noexcept
        : f_(f) {}

    // Do work.
    void operator()() noexcept {
        f_(this);
    }

    void attach(combining_lock& lock) noexcept {
        auto prev = lock.last.exchange(this, std::memory_order_acq_rel);
        if (prev == nullptr) {
            // We are head of the queue, grab the lock.
            do {
                while (lock.flag.load(std::memory_order_relaxed)) {
                    std::this_thread::yield();
                }
            } while (lock.flag.exchange(true, std::memory_order_acquire));

        } else {
            // Append us to the queue.
            prev->next.store(this, std::memory_order_release);

            // Wait for our turn.
            while (status.load(std::memory_order_acquire) == lock_status::Waiting) {
                std::this_thread::yield();
            }

            // If other threads have helped us complete our work, return.
            if (status.load(std::memory_order_relaxed) == lock_status::Done) {
                return;
            }
            // Otherwise, we as head of the queue, go on.
        }

        // We are head of the queue, trying to finish our and subsequent works.
        auto cur = this;
        while (true) {
            // Do cur's work.
            (*cur)();

            auto next = cur->next.load(std::memory_order_acquire);
            if (next == nullptr) {
                break;
            }

            cur->status.store(lock_status::Done, std::memory_order_release);
            cur = next;
        }

        // We may be at the end of the queue, try to close the queue.
        auto temp = cur;
        if (lock.last.compare_exchange_strong(temp, nullptr, std::memory_order_release, std::memory_order_relaxed)) {
            cur->status.store(lock_status::Done, std::memory_order_release);
            // Release the lock.
            lock.flag.store(false, std::memory_order_release);
            return;
        }

        // Failed to close the queue, so some threads are appending them to the queue, wait for them.
        while (cur->next.load(std::memory_order_relaxed) == nullptr) {
            std::this_thread::yield();
        }
        auto next = cur->next.load(std::memory_order_acquire);

        // Let next be the new head of the queue.
        next->status.store(lock_status::Head, std::memory_order_release);
        // Status setting must be done after getting next, as cur will get destoryed.
        cur->status.store(lock_status::Done, std::memory_order_release);
    }

    template<class F>
    friend class combining_lock_node_impl;

}; // class combining_lock_node

template<class F>
class combining_lock_node_impl : combining_lock_node {
    F f_;

    combining_lock_node_impl(combining_lock& lock, F&& f) noexcept
        : combining_lock_node([](combining_lock_node* s) {
              static_cast<combining_lock_node_impl*>(s)->f_();
          }),
          f_(f) {
        attach(lock);
    }

    template<class FF>
    friend void with(combining_lock&, FF&&) noexcept;

}; // class combining_lock_node_impl

template<class F>
void with(combining_lock& lock, F&& f) noexcept {
    // Fast path, try not to start the queue.
    if CIEL_LIKELY (lock.last.load(std::memory_order_relaxed) == nullptr
                    && lock.flag.exchange(true, std::memory_order_acquire) == false) {
        f();

        lock.flag.store(false, std::memory_order_release);
        return;
    }

    combining_lock_node_impl<F> node(lock, std::forward<F>(f));
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_COMBINING_LOCK_HPP_

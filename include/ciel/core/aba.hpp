#ifndef CIELLAB_INCLUDE_CIEL_CORE_ABA_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_ABA_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/message.hpp>
#include <ciel/core/packed_ptr.hpp>
#include <ciel/core/spinlock_ptr.hpp>

#include <atomic>

NAMESPACE_CIEL_BEGIN

enum struct aba_implementation {
    PackedPtr,
    SpinlockPtr

}; // enum struct aba_implementation

template<class, aba_implementation = aba_implementation::PackedPtr>
class aba;

// The current main implementation of ABA uses packed_ptr to avoid DWCAS,
// which limits it to support only 2^16 operations. So theoretically, in some absurdly pathological case,
// the ABA problem could still happen if one thread is pre-empted and then
// another thread(s) somehow performs more than 2^16 operations before the original thread wakes up again.
template<class T>
class aba<T, aba_implementation::PackedPtr> {
private:
    atomic_packed_ptr<T> ptr_{nullptr};

    using linked_type = typename decltype(ptr_)::value_type;

    struct impl {
        linked_type old_;
        aba* parent_;

        impl(const linked_type old, aba* parent) noexcept
            : old_(old), parent_(parent) {}

        CIEL_NODISCARD T* ptr() const noexcept {
            return old_.ptr();
        }

        CIEL_NODISCARD bool store_conditional(T* ptr) noexcept {
            const linked_type temp{ptr, old_.count() + 1};
            return parent_->ptr_.compare_exchange_weak(old_, temp, std::memory_order_acq_rel,
                                                       std::memory_order_relaxed);
        }

    }; // struct impl

public:
    CIEL_NODISCARD impl read() noexcept {
        return {ptr_.load(std::memory_order_relaxed), this};
    }

#if CIEL_STD_VER >= 17
    static constexpr bool is_always_lock_free = decltype(ptr_)::is_always_lock_free;
    static_assert(is_always_lock_free);
#endif

}; // class aba<T, aba_implementation::PackedPtr>

// Use spinlock_based ABA as a backup.
template<class T>
class aba<T, aba_implementation::SpinlockPtr> {
private:
    spinlock_ptr<T> ptr_;

    struct impl {
        aba* parent_;

        impl(aba* parent) noexcept
            : parent_(parent) {
            CIEL_ASSERT(parent_->ptr_.is_locked());
        }

        ~impl() {
            parent_->ptr_.unlock(std::memory_order_release);
        }

        CIEL_NODISCARD T* ptr() const noexcept {
            return parent_->ptr_.ptr();
        }

        CIEL_NODISCARD bool store_conditional(T* ptr) const noexcept {
            parent_->ptr_.store(ptr, std::memory_order_relaxed);
            return true;
        }

    }; // struct impl

public:
    CIEL_NODISCARD impl read() noexcept {
        CIEL_UNUSED(ptr_.lock(std::memory_order_acquire));

        return {this};
    }

#if CIEL_STD_VER >= 17
    static constexpr bool is_always_lock_free = false;
#endif

}; // class aba<T, aba_implementation::SpinlockPtr>

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_ABA_HPP_

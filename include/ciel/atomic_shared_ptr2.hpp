#ifndef CIELLAB_INCLUDE_CIEL_ATOMIC_SHARED_PTR2_HPP_
#define CIELLAB_INCLUDE_CIEL_ATOMIC_SHARED_PTR2_HPP_

#include <ciel/config.hpp>

#if CIEL_STD_VER < 20
#warning "So far Deferred Reclamation atomic_shared_ptr implementation is valid after C++20"

#else

#define CIEL_DEFERRED_RECLAMATION_ATOMIC_SHARED_PTR_IMPLEMENTED
#include <ciel/shared_ptr.hpp>
#undef CIEL_DEFERRED_RECLAMATION_ATOMIC_SHARED_PTR_IMPLEMENTED

#include <ciel/hazard_pointers.hpp>

NAMESPACE_CIEL_BEGIN
namespace deferred_reclamation {

template<class T>
class atomic_shared_ptr {
private:
    mutable std::atomic<shared_weak_count*> control_block_;

public:
    atomic_shared_ptr() noexcept
        : control_block_(nullptr) {}

    atomic_shared_ptr(std::nullptr_t) noexcept
        : control_block_(nullptr) {}

    // Not an atomic operation, like any other atomics.
    atomic_shared_ptr(shared_ptr<T> desired) noexcept
        : control_block_(desired.control_block_) {

        desired.clear();
    }

    atomic_shared_ptr(const atomic_shared_ptr&) = delete;
    void operator=(const atomic_shared_ptr&) = delete;

    ~atomic_shared_ptr() {
        store(nullptr);
    }

    void operator=(shared_ptr<T> desired) noexcept {
        store(desired);
    }

    void operator=(std::nullptr_t) noexcept {
        store(nullptr);
    }

    CIEL_NODISCARD bool is_lock_free() const noexcept {
        CIEL_PRECONDITION(control_block_.is_lock_free() == true);

        return control_block_.is_lock_free();
    }

    void store(shared_ptr<T> desired) noexcept {
        shared_weak_count* control_block = desired.control_block_;
        desired.clear();

        shared_weak_count* old_control_block = control_block_.exchange(control_block);

        if (old_control_block != nullptr) {
            old_control_block->shared_count_release();
        }
    }

    CIEL_NODISCARD shared_ptr<T> load() const noexcept {
        shared_weak_count* current_control_block;

        auto& hp = get_hazard_pointers<shared_weak_count>();

        do {
            current_control_block = hp.protect(control_block_);

        } while (current_control_block != nullptr && !current_control_block->increment_if_not_zero());

        return shared_ptr<T>{current_control_block};
    }

    CIEL_NODISCARD operator shared_ptr<T>() const noexcept {
        return load();
    }

    CIEL_NODISCARD shared_ptr<T> exchange(shared_ptr<T> desired) noexcept {
        shared_weak_count* current_control_block = desired.control_block_;
        desired.clear();

        shared_weak_count* old_control_block = control_block_.exchange(current_control_block);

        return shared_ptr<T>{old_control_block};
    }

    CIEL_NODISCARD bool compare_exchange_weak(shared_ptr<T>& expected, shared_ptr<T> desired) noexcept {
        shared_weak_count* expected_control_block = expected.control_block_;
        shared_weak_count* desired_control_block = desired.control_block_;

        if (control_block_.compare_exchange_weak(expected_control_block, desired_control_block)) {
            if (expected_control_block != nullptr) {
                expected_control_block->shared_count_release();
            }

            desired.clear();
            return true;
        }

        expected = load();
        return false;
    }

    CIEL_NODISCARD bool compare_exchange_strong(shared_ptr<T>& expected, shared_ptr<T> desired) noexcept {
        shared_weak_count* expected_control_block = expected.control_block_;

        do {
            if (compare_exchange_weak(expected, desired)) {
                return true;
            }

        } while (expected_control_block == expected.control_block_);

        return false;
    }

#if CIEL_STD_VER >= 17
    static constexpr bool is_always_lock_free = std::atomic<shared_weak_count*>::is_always_lock_free;
    static_assert(is_always_lock_free == true, "");
#endif // CIEL_STD_VER >= 17

};  // class atomic_shared_ptr

}   // namespace deferred_reclamation
NAMESPACE_CIEL_END

#endif // CIEL_STD_VER >= 20
#endif // CIELLAB_INCLUDE_CIEL_ATOMIC_SHARED_PTR2_HPP_
#ifndef CIELLAB_INCLUDE_CIEL_ATOMIC_SHARED_PTR_HPP_
#define CIELLAB_INCLUDE_CIEL_ATOMIC_SHARED_PTR_HPP_

#include <ciel/compare.hpp>
#include <ciel/config.hpp>
#include <ciel/is_trivially_relocatable.hpp>
#include <ciel/shared_ptr.hpp>

NAMESPACE_CIEL_BEGIN

// This is an over simplified split_reference_count implementation of atomic<shared_ptr<T>> only for educational
// purposes. We don't consider any memory_orders, hence all are seq_cst.
//
template<class T>
class atomic_shared_ptr {
private:
    struct counted_control_block {
        uintptr_t control_block_ : 48;
        size_t local_count_      : 16; // TODO: Use spin_lock as a backup when local_count_ is beyond 2 ^ 16

        counted_control_block(ciel::shared_weak_count* other, const size_t local_count = 0) noexcept
            : control_block_((uintptr_t)other), local_count_(local_count) {
            CIEL_PRECONDITION((uintptr_t)other < (1ULL << 48));
        }

        CIEL_NODISCARD friend bool
        operator==(const counted_control_block& lhs, const counted_control_block& rhs) noexcept {
            return lhs.control_block_ == rhs.control_block_ && lhs.local_count_ == rhs.local_count_;
        }

    }; // struct counted_control_block

    // TODO: local pointer?
    mutable std::atomic<counted_control_block> counted_control_block_;

    CIEL_NODISCARD counted_control_block
    increment_local_ref_count() const noexcept {
        counted_control_block old_control_block = counted_control_block_;
        counted_control_block new_control_block{nullptr};

        do {
            new_control_block = old_control_block;
            ++new_control_block.local_count_;

        } while (!counted_control_block_.compare_exchange_weak(old_control_block, new_control_block));

        CIEL_POSTCONDITION(new_control_block.local_count_ > 0);

        return new_control_block;
    }

    void
    decrement_local_ref_count(counted_control_block prev_control_block) const noexcept {
        CIEL_PRECONDITION(prev_control_block.local_count_ > 0);

        counted_control_block old_control_block = counted_control_block_;
        counted_control_block new_control_block{nullptr};

        do {
            new_control_block = old_control_block;
            --new_control_block.local_count_;

        } while (old_control_block.control_block_ == prev_control_block.control_block_
                 && !counted_control_block_.compare_exchange_weak(old_control_block, new_control_block));

        // Already pointing to another control_block by store().
        // store() already help us update the remote ref count, so we just decrement that.
        if (old_control_block.control_block_ != prev_control_block.control_block_
            && (ciel::shared_weak_count*)prev_control_block.control_block_ != nullptr) {
            ((ciel::shared_weak_count*)prev_control_block.control_block_)->shared_count_release();
        }
    }

public:
    atomic_shared_ptr() noexcept
        : counted_control_block_(nullptr) {}

    atomic_shared_ptr(std::nullptr_t) noexcept
        : counted_control_block_(nullptr) {}

    // Not an atomic operation, like any other atomics.
    atomic_shared_ptr(ciel::shared_ptr<T> desired) noexcept
        : counted_control_block_(desired.control_block_) {
        desired.clear();
    }

    atomic_shared_ptr(const atomic_shared_ptr&) = delete;
    // clang-format off
    atomic_shared_ptr& operator=(const atomic_shared_ptr&) = delete;
    // clang-format on

    ~atomic_shared_ptr() {
        store(nullptr);
    }

    void
    operator=(ciel::shared_ptr<T> desired) noexcept {
        store(desired);
    }

    void
    operator=(std::nullptr_t) noexcept {
        store(nullptr);
    }

    CIEL_NODISCARD bool
    is_lock_free() const noexcept {
        CIEL_PRECONDITION(counted_control_block_.is_lock_free() == true);

        return counted_control_block_.is_lock_free();
    }

    void
    store(ciel::shared_ptr<T> desired) noexcept {
        counted_control_block new_control_block{desired.control_block_};
        desired.clear();

        counted_control_block old_control_block = counted_control_block_.exchange(new_control_block);

        // Help inflight loads to update those local refcounts to the global.
        if ((ciel::shared_weak_count*)old_control_block.control_block_ != nullptr) {
            ((ciel::shared_weak_count*)old_control_block.control_block_)
                ->shared_add_ref(old_control_block.local_count_);
            ((ciel::shared_weak_count*)old_control_block.control_block_)->shared_count_release();
        }
    }

    CIEL_NODISCARD ciel::shared_ptr<T>
    load() const noexcept {
        // Atomically increment local ref count, so that store() after this can be safe.
        counted_control_block cur_control_block = increment_local_ref_count();

        if ((ciel::shared_weak_count*)cur_control_block.control_block_ != nullptr) {
            ((ciel::shared_weak_count*)cur_control_block.control_block_)->shared_add_ref();
        }

        ciel::shared_ptr<T> result{(ciel::shared_weak_count*)cur_control_block.control_block_}; // private constructor

        decrement_local_ref_count(cur_control_block);

        return result;
    }

    CIEL_NODISCARD
    operator ciel::shared_ptr<T>() const noexcept {
        return load();
    }

    CIEL_NODISCARD ciel::shared_ptr<T>
    exchange(ciel::shared_ptr<T> desired) noexcept {
        counted_control_block new_control_block(desired.control_block_);
        desired.clear();

        counted_control_block old_control_block = counted_control_block_.exchange(new_control_block);

        return ciel::shared_ptr<T>((ciel::shared_weak_count*)old_control_block.control_block_);
    }

    CIEL_NODISCARD bool
    compare_exchange_weak(ciel::shared_ptr<T>& expected, ciel::shared_ptr<T> desired) noexcept {
        counted_control_block expected_control_block(expected.control_block_);
        counted_control_block desired_control_block(desired.control_block_);

        if (counted_control_block_.compare_exchange_weak(expected_control_block, desired_control_block)) {
            if ((ciel::shared_weak_count*)expected_control_block.control_block_ != nullptr) {
                ((ciel::shared_weak_count*)expected_control_block.control_block_)->shared_count_release();
            }

            desired.clear();
            return true;
        }

        expected = load();
        return false;
    }

    CIEL_NODISCARD bool
    compare_exchange_strong(ciel::shared_ptr<T>& expected, ciel::shared_ptr<T> desired) noexcept {
        counted_control_block expected_control_block(expected.control_block_);

        do {
            if (compare_exchange_weak(expected, desired)) {
                return true;
            }

        } while (expected_control_block == expected.control_block_);

        return false;
    }

#if CIEL_STD_VER >= 17
    static constexpr bool is_always_lock_free = std::atomic<counted_control_block>::is_always_lock_free;
    static_assert(is_always_lock_free == true, "");
#endif // CIEL_STD_VER >= 17

}; // class atomic_shared_ptr

template<class T>
struct is_trivially_relocatable<atomic_shared_ptr<T>> : std::true_type {};

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_ATOMIC_SHARED_PTR_HPP_

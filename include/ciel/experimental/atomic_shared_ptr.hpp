#ifndef CIELLAB_INCLUDE_CIEL_EXPERIMENTAL_ATOMIC_SHARED_PTR_HPP_
#define CIELLAB_INCLUDE_CIEL_EXPERIMENTAL_ATOMIC_SHARED_PTR_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/message.hpp>
#include <ciel/core/packed_ptr.hpp>
#include <ciel/shared_ptr.hpp>

#include <atomic>
#include <utility>

NAMESPACE_CIEL_BEGIN

// Inspired by Timur Doumler's CppCon talk: https://www.youtube.com/watch?v=gTpubZ8N0no
//
// This is an over simplified split_reference_count implementation of atomic<shared_ptr<T>>
// only for educational purposes. We don't consider any memory_orders, hence all are seq_cst.

template<class T>
class atomic_shared_ptr {
public:
    using value_type = shared_ptr<T>;

private:
    // TODO: local pointer?
    mutable atomic_packed_ptr<control_block_base> packed_control_block_{nullptr};

    using packed_type = typename decltype(packed_control_block_)::value_type;

public:
    atomic_shared_ptr() = default;

    atomic_shared_ptr(nullptr_t) noexcept {}

    atomic_shared_ptr(value_type desired) noexcept
        : packed_control_block_(desired.control_block_) {
        desired.release();
    }

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
        return packed_control_block_.is_lock_free();
    }

    CIEL_NODISCARD
    operator value_type() const noexcept {
        return load();
    }

private:
    // Atomically increment local ref count, so that store() after this can be safe.
    // Return new packed_control_block.
    CIEL_NODISCARD packed_type increment_local_ref_count() const noexcept {
        packed_type cur_packed = packed_control_block_.load();
        packed_type new_packed;

        do {
            if (cur_packed.ptr() == nullptr) {
                return cur_packed;
            }

            new_packed = cur_packed;
            new_packed.increment_count();

        } while (!packed_control_block_.compare_exchange_weak(cur_packed, new_packed));

        CIEL_ASSERT(new_packed.count() > 0);

        return new_packed;
    }

    // Atomically decrement the local ref count if old_packed.ptr() == cur_packed.ptr(),
    // or decrement the remote ref count.
    void decrement_local_ref_count(packed_type old_packed) const noexcept {
        CIEL_ASSERT(old_packed.count() > 0);

        const auto old_packed_ptr = old_packed.ptr();
        CIEL_ASSERT(old_packed_ptr != nullptr);

        packed_type cur_packed = packed_control_block_.load();
        packed_type new_packed;

        do {
            CIEL_ASSERT(cur_packed.count() > 0 || cur_packed.ptr() != old_packed_ptr);

            new_packed = cur_packed;
            new_packed.decrement_count();

        } while (cur_packed.ptr() == old_packed_ptr
                 && !packed_control_block_.compare_exchange_weak(cur_packed, new_packed));

        // Already pointing to another control_block by store().
        // store() has already helped us update the remote ref count, so we just decrement that.
        if (cur_packed.ptr() != old_packed_ptr) {
            old_packed_ptr->shared_count_release();
        }
    }

public:
    CIEL_NODISCARD value_type load() const noexcept {
        const packed_type cur_packed = increment_local_ref_count();

        const auto cur_cb = cur_packed.ptr();
        if (cur_cb == nullptr) {
            return {nullptr};
        }

        cur_cb->shared_add_ref();

        decrement_local_ref_count(cur_packed);

        return {cur_cb};
    }

    void store(value_type desired) noexcept {
        CIEL_UNUSED(exchange(std::move(desired)));
    }

    CIEL_NODISCARD value_type exchange(value_type desired) noexcept {
        const packed_type new_packed(desired.control_block_, 0);
        desired.release();

        const packed_type cur_packed = packed_control_block_.exchange(new_packed);

        auto cur_cb = cur_packed.ptr();
        if (cur_cb == nullptr) {
            return {nullptr};
        }

        // Help inflight loads to update those local ref counts to the global.
        cur_cb->shared_add_ref(cur_packed.count());

        return {cur_cb};
    }

    CIEL_NODISCARD bool compare_exchange_weak(value_type& expected, value_type desired) noexcept {
        const packed_type cur_packed = packed_control_block_.load();
        packed_type expected_packed(expected.control_block_, cur_packed.count());
        const packed_type desired_packed(desired.control_block_, 0);

        if (packed_control_block_.compare_exchange_weak(expected_packed, desired_packed)) {
            // Help inflight loads to update those local ref counts to the global.
            auto expected_cb = expected_packed.ptr();
            if (expected_cb != nullptr) {
                expected_cb->shared_add_ref(expected_packed.count());
                expected_cb->shared_count_release();
            }

            desired.release();
            return true;
        }

        expected = load();
        return false;
    }

    CIEL_NODISCARD bool compare_exchange_strong(value_type& expected, value_type desired) noexcept {
        const auto expected_cb = expected.control_block_;

        do {
            if (compare_exchange_weak(expected, desired)) {
                return true;
            }

        } while (expected_cb == expected.control_block_);

        return false;
    }

#if CIEL_STD_VER >= 17
    static constexpr bool is_always_lock_free = decltype(packed_control_block_)::is_always_lock_free;
    static_assert(is_always_lock_free);
#endif

}; // class atomic_shared_ptr

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_EXPERIMENTAL_ATOMIC_SHARED_PTR_HPP_

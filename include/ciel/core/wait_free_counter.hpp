#ifndef CIELLAB_INCLUDE_CIEL_CORE_WAIT_FREE_COUNTER_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_WAIT_FREE_COUNTER_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/message.hpp>

#include <atomic>
#include <cstddef>
#include <limits>

NAMESPACE_CIEL_BEGIN

// This class is useful for reference counting, i.e. shared_ptr's shared_count, when the count hits zero,
// the managed object will be deleted and the count shall not increment from zero thereafter.
//
// Contracts:
// 1. The counter starts at one.
// 2. Once the counter decrements to zero, it will stuck at it and never increment again.
// 3. Before the counter hits zero, the number of increment and decrement calls should be equal.
//    After the counter hits zero, no further decrement calls should be invoked.
//    This aligns with reference counting semantics, where the one that calls decrement
//    should be the same one that previously called increment, and there is no possibilities to decrement from zero.

struct wait_free_counter {
private:
    static constexpr size_t zero_flag = size_t(1) << (std::numeric_limits<size_t>::digits - 1);
    std::atomic<size_t> impl_{1};

public:
    wait_free_counter() = default;

    ~wait_free_counter() {
        CIEL_PRECONDITION(load() == 0);
    }

    wait_free_counter(const wait_free_counter&)            = delete;
    wait_free_counter& operator=(const wait_free_counter&) = delete;

    // Returns zero only if zero_flag is being set, returns one in zero_pending stage.
    size_t load(const std::memory_order order = std::memory_order_seq_cst) const noexcept {
        const size_t res = impl_.load(order);
        if (res & zero_flag) {
            return 0;
        }

        return res != 0 ? res : 1;
    }

    // Returns false if counter is already stuck to zero.
    bool increment_if_not_zero(const size_t diff, const std::memory_order order = std::memory_order_seq_cst) noexcept {
        const size_t res = impl_.fetch_add(diff, order);
        return (res & zero_flag) == 0;
    }

    // Returns true only if this operation is responsible for afterwards cleanup, i.e. shared_ptr object deletion.
    bool decrement(const size_t diff, const std::memory_order order = std::memory_order_seq_cst) noexcept {
        const size_t res = impl_.fetch_sub(diff, order);
        CIEL_PRECONDITION(res >= diff);

        if (res == diff) {
            size_t expected = 0;
            return impl_.compare_exchange_strong(expected, zero_flag);
        }

        return false;
    }

    operator size_t() const noexcept {
        return load();
    }

#if CIEL_STD_VER >= 17
    static constexpr bool is_always_lock_free = decltype(impl_)::is_always_lock_free;
    static_assert(is_always_lock_free);
#endif

}; // wait_free_counter

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_WAIT_FREE_COUNTER_HPP_

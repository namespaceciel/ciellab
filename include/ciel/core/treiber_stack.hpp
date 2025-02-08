#ifndef CIELLAB_INCLUDE_CIEL_CORE_TREIBER_STACK_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_TREIBER_STACK_HPP_

#include <ciel/core/aba.hpp>
#include <ciel/core/config.hpp>
#include <ciel/core/message.hpp>

#include <atomic>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

// Inspired by Microsoft snmalloc's implementation.

template<class T, aba_implementation Impl = aba_implementation::PackedPtr>
class treiber_stack {
    static_assert(std::is_same<decltype(T::next), std::atomic<T*>>::value, "");

private:
    alignas(cacheline_size) aba<T, Impl> stack_;

public:
    treiber_stack() = default;

    treiber_stack(const treiber_stack&)            = delete;
    treiber_stack& operator=(const treiber_stack&) = delete;

    void push(T* t) noexcept {
        push(t, t);
    }

    void push(T* first, T* last) noexcept {
        CIEL_ASSERT(first != nullptr);
        CIEL_ASSERT(last != nullptr);

        auto impl = stack_.read();

        do {
            T* top = impl.ptr();
            last->next.store(top, std::memory_order_release);

        } while (!impl.store_conditional(first));
    }

    CIEL_NODISCARD T* pop() noexcept {
        auto impl = stack_.read();
        T* top    = nullptr;
        T* next   = nullptr;

        do {
            top = impl.ptr();

            if (top == nullptr) {
                break;
            }

            // The returned `top` shall not be unmapped immediately as the losers of CAS
            // may read uninitialized memory by loading `top->next`, leading to crash.
            // Simply reusing the memory for something else wouldn't be a problem in the real world,
            // but it's still undefined behavior for this atomic load to race with a non-atomic store.
            next = top->next.load(std::memory_order_relaxed);

        } while (!impl.store_conditional(next));

        return top;
    }

    CIEL_NODISCARD T* pop_all() noexcept {
        auto impl = stack_.read();
        T* top    = nullptr;

        do {
            top = impl.ptr();

            if (top == nullptr) {
                break;
            }

        } while (!impl.store_conditional(nullptr));

        return top;
    }

}; // class treiber_stack

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_TREIBER_STACK_HPP_

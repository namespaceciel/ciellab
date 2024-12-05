#ifndef CIELLAB_INCLUDE_CIEL_CORE_TREIBER_STACK_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_TREIBER_STACK_HPP_

#include <ciel/core/aba.hpp>
#include <ciel/core/config.hpp>

#include <atomic>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

template<class T>
class treiber_stack {
    static_assert(std::is_same<decltype(T::next), std::atomic<T*>>::value, "");

private:
    alignas(cacheline_size) aba<T> stack_;

public:
    treiber_stack() = default;

    treiber_stack(const treiber_stack&)            = delete;
    treiber_stack& operator=(const treiber_stack&) = delete;

    void push(T* t) noexcept {
        push(t, t);
    }

    void push(T* first, T* last) noexcept {
        auto impl = stack_.read();

        do {
            T* top = impl.ptr();
            last->next.store(top, std::memory_order_release);

        } while (!impl.store_conditional(first));
    }

    CIEL_NODISCARD T* pop() noexcept {
        auto impl = stack_.read();
        T* top;
        T* next;

        do {
            top = impl.ptr();

            if (top == nullptr) {
                break;
            }

            next = top->next.load(std::memory_order_relaxed);

        } while (!impl.store_conditional(next));

        return top;
    }

    CIEL_NODISCARD T* pop_all() noexcept {
        auto impl = stack_.read();
        T* top;

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

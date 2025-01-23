#ifndef CIELLAB_INCLUDE_CIEL_CORE_MPSC_QUEUE_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_MPSC_QUEUE_HPP_

#include <ciel/core/aligned_storage.hpp>
#include <ciel/core/config.hpp>
#include <ciel/core/message.hpp>

#include <atomic>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

// Inspired by Microsoft snmalloc's implementation.
// https://github.com/microsoft/snmalloc/blob/main/snmalloc.pdf
// See Figure 3 in snmalloc paper for thorough explanations.

template<class T>
class mpsc_queue {
    static_assert(std::is_same<decltype(T::next), std::atomic<T*>>::value, "");

private:
    alignas(cacheline_size) std::atomic<T*> front_{nullptr};
    alignas(cacheline_size) std::atomic<T*> back_{nullptr};
    aligned_storage<sizeof(T), alignof(T)> stub_;

public:
    mpsc_queue() noexcept {
        T* stub_ptr      = reinterpret_cast<T*>(&stub_);
        using AtomicTPtr = std::atomic<T*>;
        ::new (&(stub_ptr->next)) AtomicTPtr(nullptr);
        front_.store(stub_ptr, std::memory_order_relaxed);
    }

    mpsc_queue(const mpsc_queue&)            = delete;
    mpsc_queue& operator=(const mpsc_queue&) = delete;

    ~mpsc_queue() {
        T* stub_ptr      = reinterpret_cast<T*>(&stub_);
        using AtomicTPtr = std::atomic<T*>;
        stub_ptr->next.~AtomicTPtr();
    }

    void push(T* t) noexcept {
        push(t, t);
    }

    void push(T* first, T* last) noexcept {
        CIEL_ASSERT(first != nullptr);
        CIEL_ASSERT(last != nullptr);

        last->next.store(nullptr, std::memory_order_relaxed);
        // It needs to be release, so nullptr in next is visible, and needs to be acquire,
        // so linking into the list does not race with other threads nullptr-initing of the next field.
        T* prev = back_.exchange(last, std::memory_order_acq_rel);

        if CIEL_LIKELY (prev != nullptr) { // not at stub state
            prev->next.store(first, std::memory_order_relaxed);
            return;
        }

        // at stub state, remove stub
        front_.store(first, std::memory_order_relaxed);
    }

    // ProcessEachNode is a monadic predicate callback type, to properly process each node.
    // The callback may return false to stop the iteration early, but must have processed the element it was given.
    template<class ProcessEachNode>
    void process(ProcessEachNode&& process_each_node) noexcept {
        T* cur = front_.load(std::memory_order_relaxed);
        T* b   = back_.load(std::memory_order_relaxed);

        if CIEL_UNLIKELY (cur == reinterpret_cast<T*>(&stub_)) { // at stub state
            return;
        }

        while (cur != b) {
            T* next = cur->next.load(std::memory_order_relaxed);

            // If the push is happening halfway (back_ is updated while prev->next is not updated yet).
            if CIEL_UNLIKELY (next == nullptr) {
                break;
            }

            if CIEL_UNLIKELY (!process_each_node(cur)) {
                front_.store(next, std::memory_order_relaxed);
                return;
            }

            cur = next;
        }

        // Probably only one node left.
        front_.store(cur, std::memory_order_relaxed);
    }

    // ProcessEachNode callback must process all nodes left.
    // No other related threads shall exist here.
    template<class ProcessEachNode>
    void destructive_process(ProcessEachNode&& process_each_node) noexcept {
        T* cur = front_.load(std::memory_order_relaxed);

        if CIEL_UNLIKELY (cur == reinterpret_cast<T*>(&stub_)) { // at stub state
            return;
        }

        while (cur != nullptr) {
            T* next = cur->next.load(std::memory_order_relaxed);
            process_each_node(cur);
            cur = next;
        }
    }

}; // class mpsc_queue

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_MPSC_QUEUE_HPP_

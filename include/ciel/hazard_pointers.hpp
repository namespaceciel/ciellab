#ifndef CIELLAB_INCLUDE_CIEL_HAZARD_POINTERS_HPP_
#define CIELLAB_INCLUDE_CIEL_HAZARD_POINTERS_HPP_

#include <ciel/config.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <thread>
#include <unordered_set>
#include <utility>

NAMESPACE_CIEL_BEGIN

template<typename T>
concept GarbageCollectible = requires(T* t, T* tp) {
    { t->get_next() } -> std::convertible_to<T*>;
    { t->set_next(tp) };
    { t->destroy() };
};

template<GarbageCollectible GarbageType>
class hazard_pointers;

// Global singleton containing the list of hazard pointers. We store it in raw
// storage so that it is never destructed.
//
// (a detached thread might grab a HazardSlot entry and not relinquish it until
// static destruction, at which point this global static would have already been
// destroyed. We avoid that using this pattern.)
//
// This does technically mean that we leak the HazardSlots, but that is
// a price we are willing to pay.
template<GarbageCollectible GarbageType>
extern inline hazard_pointers<GarbageType>& get_hazard_pointers() {
    alignas(hazard_pointers<GarbageType>) static char buffer[sizeof(hazard_pointers<GarbageType>)];
    static auto* list = new (&buffer) hazard_pointers<GarbageType>{};
    return *list;
}

// retired_list is an intrusive linked list of retired blocks. It takes advantage
// of the available managed object pointer in the control block to store the next pointers.
// (since, after retirement, it is guaranteed that the object has been freed, and thus
// the managed object pointer is no longer used.  Furthermore, it does not have to be
// kept as null since threads never read the pointer unless they own a reference count.)
template<GarbageCollectible GarbageType>
class retired_list {
public:
    using garbage_type = GarbageType;

private:
    garbage_type* head_{nullptr};
    garbage_type* tail_{nullptr};   // So that we can easily append another retired_list after tail_.

public:
    constexpr retired_list() noexcept = default;

    explicit retired_list(garbage_type* head) noexcept
        : head_(head), tail_(head) {}

    retired_list& operator=(garbage_type* head) noexcept {
        CIEL_PRECONDITION(head_ == nullptr);
        CIEL_PRECONDITION(tail_ == nullptr);

        head_ = head;
        tail_ = head;
    }

    retired_list(const retired_list&) = delete;
    retired_list& operator=(const retired_list&) = delete;

    retired_list(retired_list&& other) noexcept
        : head_(std::exchange(other.head_, nullptr)), tail_(std::exchange(other.tail_, nullptr)) {}

    ~retired_list() {
        cleanup([](auto&&) noexcept { return false; });     // Every node is not protected anymore.
    }

    // p -> head_ -> ...
    // p will be new head_
    void push(garbage_type* p) noexcept {
        p->set_next(std::exchange(head_, p));

        if CIEL_UNLIKELY(p->get_next() == nullptr) {   // head_ and tail_ are nullptr
            tail_ = p;
        }
    }

    void append(retired_list&& other) noexcept {
        if (head_ == nullptr) {
            CIEL_PRECONDITION(tail_ == nullptr);

            head_ = std::exchange(other.head_, nullptr);
            tail_ = std::exchange(other.tail_, nullptr);

        } else if (other.head_ != nullptr) {
            CIEL_PRECONDITION(tail_ != nullptr);
            CIEL_PRECONDITION(other.tail_ != nullptr);

            tail_->set_next(std::exchange(other.head_, nullptr));
            tail_ = std::exchange(other.tail_, nullptr);
        }
    }

    void swap(retired_list& other) noexcept {
        std::swap(head_, other.head_);
        std::swap(tail_, other.tail_);
    }

    // For each element x currently in the retired list, if is_protected(x) == false,
    // then x->destroy() and remove x from the retired list.
    // Otherwise, keep x on the retired list waiting for the next cleanup.
    template<class F>
    void cleanup(F&& is_protected) noexcept {
        // Destroy continuously unprotected nodes starting from head_
        while (head_ && !is_protected(head_)) {
            garbage_type* old = std::exchange(head_, head_->get_next());
            old->destroy();
        }

        if (head_) {
            garbage_type* prev = head_;
            garbage_type* current = head_->get_next();

            while (current) {
                if (!is_protected(current)) {
                    garbage_type* old = std::exchange(current, current->get_next());
                    old->destroy();
                    prev->set_next(current);

                } else {
                    prev = std::exchange(current, current->get_next());
                }
            }

            tail_ = prev;

        } else {
            tail_ = nullptr;    // empty
        }
    }

    // Cleanup at most n retired objects. For up to n elements x currently in the retired list,
    // if is_protected(x) == false, then x->destroy() and remove x from the retired list.
    // Otherwise, move x onto the "into" list.
    template<class F>
    void eject_and_move(size_t n, retired_list& into, F&& is_protected) noexcept {
        for (; head_ && n > 0; --n) {
            garbage_type* current = std::exchange(head_, head_->get_next());

            if (is_protected(current)) {
                into.push(current);

            } else {
                current->destroy();
            }
        }

        if (head_ == nullptr) {
            tail_ = nullptr;
        }
    }

};  // class retired_list

inline constexpr std::size_t CACHE_LINE_ALIGNMENT = 128;

template<GarbageCollectible>
class deamortized_reclaimer;

// Each thread owns a hazard entry slot which contains a single hazard pointer
// (called protected_pointer) and the thread's local retired list.
//
// The slots are linked together to form a linked list so that threads can scan
// for the set of currently protected pointers.
template<GarbageCollectible GarbageType>
class alignas(CACHE_LINE_ALIGNMENT) hazard_slot {
public:
    using garbage_type = GarbageType;
    using protected_set_type = std::unordered_set<garbage_type*>;

    explicit hazard_slot(const bool in_use) noexcept
        : in_use_(in_use) {}

// TODO
public:
    // The *actual* "Hazard Pointer" that protects the object that it points to.
    // Other threads scan for the set of all such pointers before they clean up.
    std::atomic<garbage_type*> protected_ptr_{nullptr};

    // Link together all existing slots into a big global linked list
    std::atomic<hazard_slot*> next_{nullptr};

    // (Intrusive) linked list of retired objects. Does not allocate memory since it
    // just uses the next pointer from inside the retired block.
    retired_list<garbage_type> retired_list_{};

    // Count the number of retires since the last cleanup. When this value exceeds
    // cleanup_threshold, we will perform cleanup.
    size_t num_retires_since_cleanup_{0};

    // True if this hazard pointer slow is owned by a thread.
    std::atomic<bool> in_use_;

    // Set of protected objects used by cleanup(). Re-used between cleanups so that
    // we don't have to allocate new memory unless the table gets full, which would
    // only happen if the user spawns substantially more threads than were active
    // during the previous call to cleanup(). Therefore cleanup is always lock free
    // unless the number of threads has doubled since last time.
    protected_set_type protected_set_{std::thread::hardware_concurrency() * 2};

    std::unique_ptr<deamortized_reclaimer<garbage_type>> deamortized_reclaimer_{nullptr};

};  // class alignas(CACHE_LINE_ALIGNMENT) hazard_slot

// A HazardSlotOwner owns exactly one HazardSlot entry in the global linked list of HazardSlots.
// On creation, it acquires a free slot from the list, or appends a new slot if all of them are in use.
// On destruction, it makes the slot available for another thread to pick up.
template<GarbageCollectible GarbageType>
struct HazardSlotOwner {
    explicit HazardSlotOwner(hazard_pointers<GarbageType>& list) noexcept
        : list_(list), my_slot_(list.get_slot()) {}

    ~HazardSlotOwner() {
        list_.relinquish_slot(my_slot_);
    }

private:
    hazard_pointers<GarbageType>& list_;

public:
    hazard_slot<GarbageType>* const my_slot_;
};

template<GarbageCollectible GarbageType>
class deamortized_reclaimer {
public:
    using garbage_type = GarbageType;
    using protected_set_type = std::unordered_set<garbage_type*>;

    explicit deamortized_reclaimer(hazard_slot<garbage_type>& slot, hazard_slot<garbage_type>* const head) noexcept
        : my_slot_(slot), head_slot_(head) {}

    void do_reclamation_work() {
        ++num_retires_;

        if (current_slot_ == nullptr) {
            if (num_retires_ < 2 * num_hazard_ptrs_) {
                // Need to batch 2P retires before scanning hazard pointers to ensure
                // that we eject at least P blocks to make it worth the work.
                return;
            }

            // There are at least 2 * num_hazard_pointers objects awaiting reclamation
            num_retires_ = 0;
            num_hazard_ptrs_ = std::exchange(next_num_hazard_ptrs_, 0);
            current_slot_ = head_slot_;
            protected_set_.swap(next_protected_set_);
            next_protected_set_.clear();// The only not-O(1) operation, but its fast.

            eligible_.append(std::move(next_eligible_));
            next_eligible_.swap(my_slot_.retired_list_);
        }

        // Eject up to two elements from the eligible_ set. It has to be two because we waited until
        // we had 2 * num_hazard_ptrs eligible objects, so we want that to be processed by the time
        // we get through the hazard_pointer list again.
        eligible_.eject_and_move(2, my_slot_.retired_list_, [&](auto p) noexcept { return protected_set_.count(p) > 0; });

        ++next_num_hazard_ptrs_;
        next_protected_set_.insert(current_slot_->protected_ptr_.load());
        current_slot_ = current_slot_->next_;
    }

private:
    hazard_slot<garbage_type>& my_slot_;
    hazard_slot<garbage_type>* const head_slot_;
    hazard_slot<garbage_type>* current_slot_{nullptr};

    protected_set_type protected_set_{2 * std::thread::hardware_concurrency()};
    protected_set_type next_protected_set_{2 * std::thread::hardware_concurrency()};

    retired_list<garbage_type> eligible_{};
    retired_list<garbage_type> next_eligible_{};

    // A local estimate of the number of active hazard pointers
    size_t num_hazard_ptrs_{std::thread::hardware_concurrency()};
    size_t next_num_hazard_ptrs_{std::thread::hardware_concurrency()};

    size_t num_retires_{0};

};  // struct deamortized_reclaimer

enum class ReclamationMethod {
    AmortizedReclamation, // Reclamation happens in bulk in the retiring thread
    DeamortizedReclamation// Reclamation happens spread out over the retiring thread

};  // enum class ReclamationMethod

// A simple and efficient implementation of Hazard Pointer deferred reclamation
//
// Each live thread owns *exactly one* Hazard Pointer, which is sufficient for most
// (but not all) algorithms that use them. In particular, it is sufficient for lock-
// free atomic shared ptrs. This makes it much simpler and slightly more efficient
// than a general-purpose Hazard Pointer implementation, like the one in Folly, which
// supports each thread having an arbitrary number of Hazard Pointers.
//
// Each thread keeps a local retired list of objects that are pending deletion.
// This means that a stalled thread can delay the destruction of its retired objects
// indefinitely, however, since each thread is only allowed to protect a single object
// at a time, it is guaranteed that there are at most O(P^2) total unreclaimed objects
// at any given point, so the memory usage is theoretically bounded.
template<GarbageCollectible GarbageType>
class hazard_pointers {
public:
    // After this many retires, a thread will attempt to clean up the contents of
    // its local retired list, deleting any retired objects that are not protected.
    constexpr static size_t cleanup_threshold = 2000;

    using garbage_type = GarbageType;
    using protected_set_type = std::unordered_set<garbage_type*>;

public:
    static inline const thread_local HazardSlotOwner local_slot{get_hazard_pointers<garbage_type>()};

    // Find an available hazard slot, or allocate a new one if none available.
    hazard_slot<garbage_type>* get_slot() {
        auto current = list_head_;

        while (true) {
            if (!current->in_use_.load() && !current->in_use_.exchange(true)) {
                return current;
            }

            if (current->next_.load() == nullptr) {
                auto my_slot = new hazard_slot<garbage_type>{true};

                if (mode_ == ReclamationMethod::DeamortizedReclamation) {
                    my_slot->deamortized_reclaimer_ = std::make_unique<deamortized_reclaimer<garbage_type>>(*my_slot, list_head_);
                }

                hazard_slot<garbage_type>* next = nullptr;

                while (!current->next_.compare_exchange_weak(next, my_slot)) {
                    current = next;
                    next = nullptr;
                }

                return my_slot;

            } else {
                current = current->next_.load();
            }
        }
    }

    // Give a slot back to the world so another thread can re-use it
    void relinquish_slot(hazard_slot<garbage_type>* slot) {
        slot->in_use_.store(false);
    }

    template<typename F>
    void for_each_slot(F&& f) noexcept(std::is_nothrow_invocable_v<F&, hazard_slot<garbage_type>&>) {
        auto current = list_head_;

        while (current) {
            f(*current);
            current = current->next_.load();
        }
    }

    // Apply the function f to all currently announced hazard pointers
    template<typename F>
    void scan_hazard_pointers(F&& f) noexcept(std::is_nothrow_invocable_v<F&, garbage_type*>) {
        for_each_slot([&, f = std::forward<F>(f)](hazard_slot<garbage_type>& slot) {
            auto p = slot.protected_ptr_.load();
            if (p) {
                f(p);
            }
        });
    }

    void cleanup(hazard_slot<garbage_type>& slot) {
        slot.num_retires_since_cleanup_ = 0;
        std::atomic_thread_fence(std::memory_order_seq_cst);
        scan_hazard_pointers([&](auto p) { slot.protected_set_.insert(p); });
        slot.retired_list_.cleanup([&](auto p) { return slot.protected_set_.count(p) > 0; });
        slot.protected_set_.clear();     // Does not free memory, only clears contents
    }

    ReclamationMethod mode_{ReclamationMethod::AmortizedReclamation};
    std::memory_order protection_order_{std::memory_order_relaxed};
    hazard_slot<garbage_type>* const list_head_;

public:
    // Pre-populate the slot list with P slots, one for each hardware thread
    hazard_pointers()
        : list_head_(new hazard_slot<garbage_type>{false}) {

        auto current = list_head_;
        for (unsigned i = 1; i < std::thread::hardware_concurrency(); ++i) {
            current->next_ = new hazard_slot<garbage_type>{false};
            current = current->next_;
        }
    }

    ~hazard_pointers() {
        auto current = list_head_;

        while (current) {
            auto old = std::exchange(current, current->next.load());
            delete old;
        }
    }

    // Protect the object pointed to by the pointer currently stored at src.
    //
    // The second argument allows the protected pointer to be deduced from
    // the value stored at src, for example, if src stores a pair containing
    // the pointer to protect and some other value. In this case, the value of
    // f(ptr) is protected instead, but the full value *ptr is still returned.
    template<template<typename> typename Atomic, typename U, typename F>
    U protect(const Atomic<U>& src, F&& f) {
        static_assert(std::is_convertible_v<std::invoke_result_t<F, U>, garbage_type*>);

        auto& slot = local_slot.my_slot_->protected_ptr_;

        U result = src.load(std::memory_order_acquire);

        while (true) {
            auto ptr_to_protect = f(result);
            if (ptr_to_protect == nullptr) {
                return result;
            }

            slot.store(ptr_to_protect, protection_order_);
            std::atomic_thread_fence(std::memory_order_seq_cst);

            U current_value = src.load(std::memory_order_acquire);

            if CIEL_LIKELY (current_value == result) {
                return result;

            } else {
                result = std::move(current_value);
            }
        }
    }

    // Protect the object pointed to by the pointer currently stored at src.
    template<template<typename> typename Atomic, typename U>
    U protect(const Atomic<U>& src) {
        return protect(src, [](auto&& x) { return std::forward<decltype(x)>(x); });
    }

    // Unprotect the currently protected object
    void release() {
        local_slot.my_slot->protected_ptr.store(nullptr, std::memory_order_release);
    }

    // Retire the given object
    //
    // The object managed by p must have reference count zero.
    void retire(garbage_type* p) noexcept {
        hazard_slot<garbage_type>& my_slot = *local_slot.my_slot_;
        my_slot.retired_list_.push(p);

        if (mode_ == ReclamationMethod::DeamortizedReclamation) {
            assert(my_slot.deamortized_reclaimer_ != nullptr);
            my_slot.deamortized_reclaimer_->do_reclamation_work();

        } else if CIEL_UNLIKELY (++my_slot.num_retires_since_cleanup_ >= cleanup_threshold) {
            cleanup(my_slot);
        }
    }

    void enable_deamortized_reclamation() {
        CIEL_PRECONDITION(mode_ == ReclamationMethod::AmortizedReclamation);

        for_each_slot([&](hazard_slot<garbage_type>& slot) {
            slot.deamortized_reclaimer = std::make_unique<deamortized_reclaimer>(slot, list_head_);
        });

        mode_ = ReclamationMethod::DeamortizedReclamation;
        protection_order_ = std::memory_order_seq_cst;
    }

};  // class hazard_pointers

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_HAZARD_POINTERS_HPP_
#ifndef CIELLAB_INCLUDE_CIEL_HAZARD_POINTER_HPP_
#define CIELLAB_INCLUDE_CIEL_HAZARD_POINTER_HPP_

#include <ciel/core/aligned_storage.hpp>
#include <ciel/core/config.hpp>
#include <ciel/core/exchange.hpp>
#include <ciel/core/is_final.hpp>
#include <ciel/core/message.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <new>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>

NAMESPACE_CIEL_BEGIN

// Synopsis: https://eel.is/c++draft/saferecl.hp
// TODO: Slowly change to standard-compatible interface.
//
// Inspired by Daniel Anderson's hazard_pointer implementation:
// https://github.com/DanielLiamAnderson/atomic_shared_ptr/blob/master/include/parlay/details/hazard_pointers.hpp

class hazard_pointer_obj_base_link {
private:
    hazard_pointer_obj_base_link* next_{nullptr};

public:
    hazard_pointer_obj_base_link* hp_next() const noexcept {
        return next_;
    }

    void hp_set_next(hazard_pointer_obj_base_link* n) noexcept {
        next_ = n;
    }

    virtual void hp_destroy() noexcept = 0;

}; // class hazard_pointer_obj_base_link

namespace detail {
namespace retired_list_detail {

// Stuck in hazard_slot, and each slot is owned by one thread at one time,
// so it don't need synchronizations.
class retired_list {
private:
    using garbage_type = hazard_pointer_obj_base_link;

    garbage_type* head_{nullptr};

public:
    retired_list() = default;

    retired_list(const retired_list&)            = delete;
    retired_list& operator=(const retired_list&) = delete;

    ~retired_list() {
        cleanup([](garbage_type*) {
            return false;
        });
    }

    void push(garbage_type* p) noexcept {
        CIEL_PRECONDITION(p != nullptr);

        p->hp_set_next(ciel::exchange(head_, p));
    }

    template<class F>
    void cleanup(F&& is_protected) {
        while (head_ && !is_protected(head_)) {
            garbage_type* old = ciel::exchange(head_, head_->hp_next());
            old->hp_destroy();
        }

        if (head_) {
            garbage_type* prev = head_;
            garbage_type* cur  = head_->hp_next();

            while (cur) {
                if (!is_protected(cur)) {
                    garbage_type* old = ciel::exchange(cur, cur->hp_next());
                    old->hp_destroy();
                    prev->hp_set_next(cur);

                } else {
                    prev = ciel::exchange(cur, cur->hp_next());
                }
            }
        }
    }

}; // class retired_list

} // namespace retired_list_detail

// Each thread owns a hazard_slot, they are linked together to form a linked list
// so that threads can scan for the set of current protected pointers.
//
// Except for next and in_use, members should not be touched by more than one thread at one time.
struct
#if CIEL_STD_VER >= 17
    alignas(cacheline_size)
#endif
        hazard_slot {
    using garbage_type = hazard_pointer_obj_base_link;

    hazard_slot(const bool iu) noexcept
        : in_use(iu) {}

    std::atomic<hazard_slot*> next{nullptr};

    // True if this hazard_slot is currently owned by a thread.
    std::atomic<bool> in_use;

    // Count the number of retires since the last cleanup.
    // When this value exceeds cleanup_threshold, cleanup will be performed.
    unsigned int num_retires_since_cleanup{0};

    // Protects the object that it points to.
    // Other threads scan for the set of all such pointers before they clean up.
    std::atomic<garbage_type*> protected_ptr{nullptr};

    // Store all protected_ptrs in it.
    // TODO: change to flat type to get rid of memory allocations.
    std::unordered_set<garbage_type*> protected_set;

    // Garbage collected by this slot.
    retired_list_detail::retired_list retired_list;

}; // struct alignas(cacheline_size) hazard_slot

} // namespace detail

class hazard_pointer {
private:
    using hazard_slot  = detail::hazard_slot;

    static constexpr size_t cleanup_threshold = 1000;

    // Each thread owns a hazard_slot.
    // Make the slot available for another thread to pick up in destructor.
    struct hazard_slot_owner {
        hazard_slot* const my_slot;

        hazard_slot_owner()
            : my_slot(get_pair().first.get_slot()) {
            CIEL_PRECONDITION(my_slot != nullptr);
            get_pair().second.fetch_add(1, std::memory_order_relaxed);
        }

        hazard_slot_owner(const hazard_slot_owner&)            = delete;
        hazard_slot_owner& operator=(const hazard_slot_owner&) = delete;

        ~hazard_slot_owner() {
            my_slot->protected_ptr.store(nullptr, std::memory_order_release);
            my_slot->in_use.store(false, std::memory_order_relaxed);

            using Pair = std::pair<hazard_pointer, std::atomic<size_t>>;
            Pair& get  = get_pair();

            constexpr size_t off = 1;
            // Same pattern as control_block_base::weak_count_release
            if (get.second.load(std::memory_order_acquire) == off) {
                get.~Pair();

            } else if (get.second.fetch_sub(off, std::memory_order_release) == off) {
                std::atomic_thread_fence(std::memory_order_acquire);
                get.~Pair();
            }
        }

    }; // struct hazard_slot_owner

    static const thread_local hazard_slot_owner threadlocal_slot;

    hazard_slot* const hazard_slot_list_head;

    CIEL_NODISCARD hazard_slot* get_slot() {
        hazard_slot* cur = hazard_slot_list_head;
        CIEL_PRECONDITION(cur != nullptr);

        while (true) {
            if (!cur->in_use.load(std::memory_order_relaxed)
                && !cur->in_use.exchange(true, std::memory_order_relaxed)) {
                return cur;
            }

            hazard_slot* next = cur->next.load(std::memory_order_relaxed);
            if (next == nullptr) {
                hazard_slot* new_slot = new hazard_slot{true};

                while (!cur->next.compare_exchange_weak(next, new_slot, std::memory_order_relaxed)) {
                    cur  = next;
                    next = nullptr;
                }

                return new_slot;
            }

            cur = next;
        }
    }

private:
    hazard_pointer()
        : hazard_slot_list_head(new hazard_slot{false}) {
        // std::thread::hardware_concurrency() may return 0.
        hazard_slot* cur = hazard_slot_list_head;
        for (unsigned int i = 1; i < std::thread::hardware_concurrency(); ++i) {
            hazard_slot* next = new hazard_slot{false};
            cur->next.store(next, std::memory_order_relaxed);
            cur = next;
        }
    }

    friend std::pair<hazard_pointer, std::atomic<size_t>>;

    // It can't be `static hazard_pointer res;` since static hazard_slot_owner uses hazard_pointer in destructors,
    // so hazard_pointer must outlive them. Use a binding reference_counter to conditionally manually destroy it.
    CIEL_NODISCARD static std::pair<hazard_pointer, std::atomic<size_t>>& get_pair() {
        using Pair = std::pair<hazard_pointer, std::atomic<size_t>>;
        static typename aligned_storage<sizeof(Pair), alignof(Pair)>::type buffer;
        static auto* ptr =
            new (&buffer) Pair(std::piecewise_construct, std::forward_as_tuple(), std::forward_as_tuple(0));
        return *ptr;
    }

public:
    CIEL_NODISCARD static hazard_pointer& get() {
        return get_pair().first;
    }

    ~hazard_pointer() {
        hazard_slot* cur = hazard_slot_list_head;
        while (cur) {
            hazard_slot* old = ciel::exchange(cur, cur->next.load(std::memory_order_relaxed));
            delete old;
        }
    }

    hazard_pointer(const hazard_pointer&)            = delete;
    hazard_pointer& operator=(const hazard_pointer&) = delete;

    template<class T>
    CIEL_NODISCARD T* protect(const std::atomic<T*>& src) noexcept {
        T* res = src.load(std::memory_order_relaxed);

        while (true) {
            if (res == nullptr) {
                return nullptr;
            }

            threadlocal_slot.my_slot->protected_ptr.store(res, std::memory_order_release);

            T* cur = src.load(std::memory_order_acquire);

            if CIEL_LIKELY (res == cur) {
                return res;
            }

            res = cur;
        }
    }

    void release() noexcept {
        threadlocal_slot.my_slot->protected_ptr.store(nullptr, std::memory_order_release);
    }

private:
    void cleanup(hazard_slot& slot) {
        slot.num_retires_since_cleanup = 0;

        // Scan over every hazard_slot, store protected_ptrs into slot.protected_set
        hazard_slot* cur = hazard_slot_list_head;
        while (cur) {
            auto p = cur->protected_ptr.load(std::memory_order_acquire);
            if (p) {
                slot.protected_set.insert(p);
            }

            cur = cur->next.load(std::memory_order_relaxed);
        }

        // Cleanup every garbage that is not being protected.
        slot.retired_list.cleanup([&](hazard_pointer_obj_base_link* p) {
            return slot.protected_set.count(p) > 0;
        });

        slot.protected_set.clear();
    }

}; // class hazard_pointer

inline const thread_local typename hazard_pointer::hazard_slot_owner hazard_pointer::threadlocal_slot;

template<class T, class D = std::default_delete<T>, bool = std::is_class<D>::value && !is_final<D>::value>
class hazard_pointer_obj_base : public hazard_pointer_obj_base_link {
private:
    D deleter_;

    D& deleter() noexcept {
        return deleter_;
    }

protected:
    hazard_pointer_obj_base()                                          = default;
    hazard_pointer_obj_base(const hazard_pointer_obj_base&)            = default;
    hazard_pointer_obj_base(hazard_pointer_obj_base&&)                 = default;
    hazard_pointer_obj_base& operator=(const hazard_pointer_obj_base&) = default;
    hazard_pointer_obj_base& operator=(hazard_pointer_obj_base&&)      = default;
    ~hazard_pointer_obj_base()                                         = default;

public:
    void retire(D d = D()) noexcept {
        deleter() = std::move(d);
    }

    void hp_destroy() noexcept override {
        deleter()(static_cast<T*>(this));
    }

}; // class hazard_pointer_obj_base

template<class T, class D>
class hazard_pointer_obj_base<T, D, true> : public hazard_pointer_obj_base_link,
                                            public D {
private:
    D& deleter() noexcept {
        return static_cast<D&>(*this);
    }

protected:
    hazard_pointer_obj_base()                                          = default;
    hazard_pointer_obj_base(const hazard_pointer_obj_base&)            = default;
    hazard_pointer_obj_base(hazard_pointer_obj_base&&)                 = default;
    hazard_pointer_obj_base& operator=(const hazard_pointer_obj_base&) = default;
    hazard_pointer_obj_base& operator=(hazard_pointer_obj_base&&)      = default;
    ~hazard_pointer_obj_base()                                         = default;

public:
    void retire(D d = D()) noexcept {
        deleter() = std::move(d);

        hazard_slot& my_slot = *hazard_pointer.get().threadlocal_slot.my_slot;
        my_slot.retired_list.push(static_cast<T*>(this));

        if CIEL_UNLIKELY (++my_slot.num_retires_since_cleanup >= cleanup_threshold) {
            cleanup(my_slot);
        }
    }

    void hp_destroy() noexcept override {
        deleter()(static_cast<T*>(this));
    }

}; // class hazard_pointer_obj_base<T, D, true>

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_HAZARD_POINTER_HPP_

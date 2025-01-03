#ifndef CIELLAB_INCLUDE_CIEL_HAZARD_POINTER_HPP_
#define CIELLAB_INCLUDE_CIEL_HAZARD_POINTER_HPP_

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

// Inspired by Daniel Anderson's hazard_pointer implementation:
// https://github.com/DanielLiamAnderson/atomic_shared_ptr/blob/master/include/parlay/details/hazard_pointers.hpp
//
// Synopsis: https://eel.is/c++draft/saferecl.hp

class hazard_pointer_obj_base_link {
private:
    hazard_pointer_obj_base_link* next_{nullptr};

public:
    CIEL_NODISCARD hazard_pointer_obj_base_link* hp_next() const noexcept {
        return next_;
    }

    void hp_set_next(hazard_pointer_obj_base_link* n) noexcept {
        next_ = n;
    }

    virtual void hp_destroy() noexcept = 0;

}; // class hazard_pointer_obj_base_link

// Each hazard_pointer owns a hazard_slot, they are linked together to form a linked list
// so that threads can scan for the set of current protected pointers.
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

    } retired_list;

}; // struct alignas(cacheline_size) hazard_slot

class hazard_pointer_headquarter {
private:
    hazard_slot* const hazard_slot_list_head;

    friend class hazard_pointer;

    hazard_pointer_headquarter()
        : hazard_slot_list_head(new hazard_slot{false}) {
        // std::thread::hardware_concurrency() may return 0.
        hazard_slot* cur = hazard_slot_list_head;
        for (unsigned int i = 1; i < std::thread::hardware_concurrency() * 2; ++i) {
            hazard_slot* next = new hazard_slot{false};
            cur->next.store(next, std::memory_order_relaxed);
            cur = next;
        }
    }

public:
    CIEL_NODISCARD static hazard_pointer_headquarter& get() {
        static hazard_pointer_headquarter res;
        return res;
    }

    hazard_pointer_headquarter(const hazard_pointer_headquarter&)            = delete;
    hazard_pointer_headquarter& operator=(const hazard_pointer_headquarter&) = delete;

    ~hazard_pointer_headquarter() {
        hazard_slot* cur = hazard_slot_list_head;
        while (cur) {
            hazard_slot* old = ciel::exchange(cur, cur->next.load(std::memory_order_relaxed));
            delete old;
        }
    }

    CIEL_NODISCARD hazard_slot* get_slot() {
        hazard_slot* cur = hazard_slot_list_head;
        CIEL_PRECONDITION(cur != nullptr);

        while (true) {
            if (!cur->in_use.load(std::memory_order_relaxed)
                && !cur->in_use.exchange(true, std::memory_order_relaxed)) {
                return cur;
            }

            hazard_slot* next = cur->next.load(std::memory_order_relaxed);
            if CIEL_UNLIKELY (next == nullptr) {
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

    void return_slot(hazard_slot* slot) noexcept {
        slot->in_use.store(false, std::memory_order_relaxed);
    }

}; // class hazard_pointer_headquarter

class hazard_pointer {
private:
    hazard_slot* slot_;

    template<class, class, bool>
    friend class hazard_pointer_obj_base;

    friend hazard_pointer make_hazard_pointer();

    void clear() noexcept {
        if (!empty()) {
            hazard_pointer_headquarter::get().return_slot(ciel::exchange(slot_, nullptr));
        }
    }

    // Used by make_hazard_pointer().
    hazard_pointer(hazard_slot* slot) noexcept
        : slot_(slot) {}

public:
    hazard_pointer() noexcept
        : slot_(nullptr) {}

    hazard_pointer(hazard_pointer&& other) noexcept
        : slot_(ciel::exchange(other.slot_, nullptr)) {}

    hazard_pointer& operator=(hazard_pointer&& other) noexcept {
        if CIEL_UNLIKELY (this == std::addressof(other)) {
            return *this;
        }

        clear();
        slot_ = ciel::exchange(other.slot_, nullptr);
        return *this;
    }

    hazard_pointer(const hazard_pointer&)            = delete;
    hazard_pointer& operator=(const hazard_pointer&) = delete;

    ~hazard_pointer() {
        clear();
    }

    CIEL_NODISCARD bool empty() const noexcept {
        return slot_ == nullptr;
    }

    template<class T>
    CIEL_NODISCARD T* protect(const std::atomic<T*>& src) noexcept {
        T* ptr = src.load(std::memory_order_relaxed);

        while (!try_protect(ptr, src)) {}

        return ptr;
    }

    template<class T>
    CIEL_NODISCARD bool try_protect(T*& ptr, const std::atomic<T*>& src) noexcept {
        CIEL_PRECONDITION(!empty());

        T* p = ptr;
        reset_protection(p);

        ptr = src.load(std::memory_order_acquire);
        if CIEL_UNLIKELY (p != ptr) {
            reset_protection();
            return false;
        }

        return true;
    }

    template<class T>
    void reset_protection(const T* ptr) noexcept {
        CIEL_PRECONDITION(!empty());

        slot_->protected_ptr.store(const_cast<T*>(ptr), std::memory_order_release);
    }

    void reset_protection(nullptr_t = nullptr) noexcept {
        CIEL_PRECONDITION(!empty());

        slot_->protected_ptr.store(nullptr, std::memory_order_release);
    }

    void swap(hazard_pointer& other) noexcept {
        std::swap(slot_, other.slot_);
    }

private: // Used by hazard_pointer_obj_base.
    static constexpr size_t cleanup_threshold = 1000;

    void retire(hazard_pointer_obj_base_link* p) {
        CIEL_PRECONDITION(p != nullptr);

        slot_->retired_list.push(p);

        if CIEL_UNLIKELY (++slot_->num_retires_since_cleanup >= cleanup_threshold) {
            cleanup();
        }
    }

    void cleanup() {
        slot_->num_retires_since_cleanup = 0;

        // Scan over every hazard_slot, store protected_ptrs into slot.protected_set.
        hazard_slot* cur = hazard_pointer_headquarter::get().hazard_slot_list_head;
        while (cur) {
            auto p = cur->protected_ptr.load(std::memory_order_acquire);
            if (p) {
                slot_->protected_set.insert(p);
            }

            cur = cur->next.load(std::memory_order_relaxed);
        }

        // Cleanup every garbage that is not being protected.
        slot_->retired_list.cleanup([&](hazard_pointer_obj_base_link* p) {
            return slot_->protected_set.count(p) > 0;
        });

        slot_->protected_set.clear();
    }

}; // class hazard_pointer

CIEL_NODISCARD hazard_pointer make_hazard_pointer() {
    hazard_pointer res(hazard_pointer_headquarter::get().get_slot());
    return res;
}

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

        auto hp = make_hazard_pointer();
        hp.retire(this);
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

        auto hp = make_hazard_pointer();
        hp.retire(this);
    }

    void hp_destroy() noexcept override {
        deleter()(static_cast<T*>(this));
    }

}; // class hazard_pointer_obj_base<T, D, true>

NAMESPACE_CIEL_END

namespace std {

void swap(ciel::hazard_pointer& lhs, ciel::hazard_pointer& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_HAZARD_POINTER_HPP_

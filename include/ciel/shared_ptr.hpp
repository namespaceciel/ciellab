#ifndef CIELLAB_INCLUDE_CIEL_SHARED_PTR_HPP_
#define CIELLAB_INCLUDE_CIEL_SHARED_PTR_HPP_

#include <atomic>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include <ciel/compressed_pair.hpp>
#include <ciel/config.hpp>

NAMESPACE_CIEL_BEGIN

template<class>
class weak_ptr;
template<class>
class enable_shared_from_this;
template<class>
class atomic_shared_ptr;

class shared_weak_count {
protected:
    // The object will be destroyed after decrementing to zero.
    std::atomic<size_t> shared_count_{1};
    // weak_ref + (shared_count_ != 0), The control block will be destroyed after decrementing to zero.
    std::atomic<size_t> weak_count_{1};

    template<class>
    friend class shared_ptr;
    template<class>
    friend class weak_ptr;
    template<class>
    friend class atomic_shared_ptr;

    shared_weak_count() noexcept = default;

    // We never call the deleter on the base class pointer,
    // so it's not necessary to mark virtual on destructor.
    ~shared_weak_count() = default;

public:
    shared_weak_count(const shared_weak_count&) = delete;
    // clang-format off
    shared_weak_count& operator=(const shared_weak_count&) = delete;
    // clang-format on

    CIEL_NODISCARD size_t
    use_count() const noexcept {
        return shared_count_.load(std::memory_order_relaxed);
    }

    void
    shared_add_ref(const size_t count = 1) noexcept {
        const size_t previous = shared_count_.fetch_add(count, std::memory_order_relaxed);

        CIEL_POSTCONDITION(count == 0 || previous != 0);
    }

    void
    weak_add_ref() noexcept {
        const size_t previous = weak_count_.fetch_add(1, std::memory_order_relaxed);

        CIEL_POSTCONDITION(previous != 0);
    }

    void
    shared_count_release() noexcept {
        constexpr size_t off = 1;
        // A decrement-release + an acquire fence is recommended by Boost's documentation:
        // https://www.boost.org/doc/libs/1_57_0/doc/html/atomic/usage_examples.html
        // Alternatively, an acquire-release decrement would work, but might be less efficient
        // since the acquire is only relevant if the decrement zeros the counter.
        if (shared_count_.fetch_sub(off, std::memory_order_release) == off) {
            std::atomic_thread_fence(std::memory_order_acquire);

            delete_pointer();
            weak_count_release(); // weak_count_ == weak_ref + (shared_count_ != 0)
        }
    }

    void
    weak_count_release() noexcept {
        constexpr size_t off = 1;
        // Avoid expensive atomic stores inspired by LLVM:
        // https://github.com/llvm/llvm-project/commit/ac9eec8602786b13a2bea685257d4f25b36030ff
        if (weak_count_.load(std::memory_order_acquire) == off) {
            delete_control_block();

        } else if (weak_count_.fetch_sub(off, std::memory_order_release) == off) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete_control_block();
        }
    }

    CIEL_NODISCARD bool
    increment_if_not_zero() noexcept {
        size_t old_count = shared_count_.load(std::memory_order_relaxed);

        do {
            if (old_count == 0) {
                return false;
            }

        } while (!shared_count_.compare_exchange_weak(old_count, old_count + 1, std::memory_order_relaxed));

        return true;
    }

    CIEL_NODISCARD virtual void*
    get_deleter(const std::type_info&) noexcept {
        return nullptr;
    }

    virtual void
    delete_pointer() noexcept
        = 0;
    virtual void
    delete_control_block() noexcept
        = 0;
    virtual void*
    managed_pointer() const noexcept
        = 0;

}; // class shared_weak_count

template<class element_type, class Deleter, class Allocator>
class control_block_with_pointer final : public shared_weak_count {
    static_assert(std::is_same<element_type, typename Allocator::value_type>::value, "");

public:
    using pointer        = element_type*;
    using deleter_type   = Deleter;
    using allocator_type = Allocator;

private:
    using alloc_traits               = std::allocator_traits<allocator_type>;
    using control_block_allocator    = typename alloc_traits::template rebind_alloc<control_block_with_pointer>;
    using control_block_alloc_traits = typename alloc_traits::template rebind_traits<control_block_with_pointer>;

    ciel::compressed_pair<ciel::compressed_pair<pointer, deleter_type>, control_block_allocator> compressed_;

    CIEL_NODISCARD pointer&
    ptr_() noexcept {
        return compressed_.first().first();
    }

    CIEL_NODISCARD const pointer&
    ptr_() const noexcept {
        return compressed_.first().first();
    }

    CIEL_NODISCARD deleter_type&
    deleter_() noexcept {
        return compressed_.first().second();
    }

    CIEL_NODISCARD const deleter_type&
    deleter_() const noexcept {
        return compressed_.first().second();
    }

    CIEL_NODISCARD control_block_allocator&
    allocator_() noexcept {
        return compressed_.second();
    }

    CIEL_NODISCARD const control_block_allocator&
    allocator_() const noexcept {
        return compressed_.second();
    }

public:
    control_block_with_pointer(pointer ptr, deleter_type&& deleter, control_block_allocator&& alloc)
        : compressed_(ciel::compressed_pair<pointer, deleter_type>(ptr, std::move(deleter)), std::move(alloc)) {}

#ifdef CIEL_HAS_RTTI
    CIEL_NODISCARD virtual void*
    get_deleter(const std::type_info& type) noexcept override {
        return (type == typeid(deleter_type)) ? static_cast<void*>(&deleter_()) : nullptr;
    }

#endif

    virtual void
    delete_pointer() noexcept override {
        deleter_()(ptr_());
        deleter_().~deleter_type();
    }

    virtual void
    delete_control_block() noexcept override {
        control_block_allocator allocator = std::move(allocator_());
        allocator_().~control_block_allocator();

        control_block_alloc_traits::deallocate(allocator, this, 1);
    }

    virtual void*
    managed_pointer() const noexcept override {
        return ptr_();
    }

}; // class control_block_with_pointer

static_assert(sizeof(control_block_with_pointer<int, std::default_delete<int>, std::allocator<int>>)
                      - sizeof(shared_weak_count)
                  == 8,
              "Empty Base Optimization is not working.");

template<class element_type, class Allocator>
class control_block_with_instance final : public shared_weak_count {
    static_assert(std::is_same<element_type, typename Allocator::value_type>::value, "");

public:
    using pointer        = element_type*;
    using allocator_type = Allocator;

private:
    using alloc_traits               = std::allocator_traits<allocator_type>;
    using control_block_allocator    = typename alloc_traits::template rebind_alloc<control_block_with_instance>;
    using control_block_alloc_traits = typename alloc_traits::template rebind_traits<control_block_with_instance>;

    compressed_pair<typename aligned_storage<sizeof(element_type), alignof(element_type)>::type,
                    control_block_allocator>
        compressed_;

    CIEL_NODISCARD pointer
    ptr_() noexcept {
        return static_cast<pointer>(static_cast<void*>(&compressed_.first()));
    }

    CIEL_NODISCARD const element_type*
    ptr_() const noexcept {
        return static_cast<const element_type*>(static_cast<const void*>(&compressed_.first()));
    }

    CIEL_NODISCARD control_block_allocator&
    allocator_() noexcept {
        return compressed_.second();
    }

    CIEL_NODISCARD const control_block_allocator&
    allocator_() const noexcept {
        return compressed_.second();
    }

public:
    template<class... Args>
    control_block_with_instance(allocator_type alloc, Args&&... args)
        : compressed_(default_init, alloc) {
        alloc_traits::construct(alloc, ptr_(), std::forward<Args>(args)...);
    }

    virtual void
    delete_pointer() noexcept override {
        allocator_type alloc(allocator_());

        alloc_traits::destroy(alloc, ptr_());
    }

    virtual void
    delete_control_block() noexcept override {
        control_block_allocator allocator = std::move(allocator_());
        allocator_().~control_block_allocator();

        control_block_alloc_traits::deallocate(allocator, this, 1);
    }

    virtual void*
    managed_pointer() const noexcept override {
        return const_cast<void*>(static_cast<const void*>(&compressed_.first()));
    }

}; // class control_block_with_instance

template<class T>
class shared_ptr {
public:
    using element_type = typename std::remove_extent<T>::type;
    using pointer      = element_type*;
    using weak_type    = weak_ptr<T>;

private:
    template<class>
    friend class shared_ptr;
    template<class>
    friend class weak_ptr;
    template<class>
    friend class atomic_shared_ptr;
    template<class U, class Alloc, class... Args>
    friend shared_ptr<U>
    allocate_shared(const Alloc&, Args&&...);

    element_type* ptr_;
    shared_weak_count* control_block_;

    template<class Y, class Deleter, class Allocator>
    CIEL_NODISCARD shared_weak_count*
    alloc_control_block(Y* ptr, Deleter&& dlt, Allocator&& alloc) {
        using alloc_traits               = std::allocator_traits<Allocator>;
        using control_block_type         = control_block_with_pointer<Y, Deleter, Allocator>;
        using control_block_allocator    = typename alloc_traits::template rebind_alloc<control_block_type>;
        using control_block_alloc_traits = typename alloc_traits::template rebind_traits<control_block_type>;

        control_block_allocator allocator(alloc);
        control_block_type* control_block = control_block_alloc_traits::allocate(allocator, 1);

        CIEL_TRY {
            control_block_alloc_traits::construct(allocator, control_block, ptr, std::forward<Deleter>(dlt),
                                                  std::forward<Allocator>(alloc));
            return control_block;
        }
        CIEL_CATCH (...) {
            control_block_alloc_traits::deallocate(allocator, control_block, 1);
            CIEL_THROW;
        }
    }

    // Serves for enable_shared_from_this.
    template<
        class Now, class Original,
        typename std::enable_if<std::is_convertible<Original*, const enable_shared_from_this<Now>*>::value, int>::type
        = 0>
    void
    enable_weak_this(const enable_shared_from_this<Now>* now_ptr, Original* original_ptr) noexcept {
        using RawNow = typename std::remove_cv<Now>::type;

        // If now_ptr is not initialized, let it points to the right control block.
        if (now_ptr && now_ptr->weak_this_.expired()) {
            now_ptr->weak_this_ = shared_ptr<RawNow>(*this, const_cast<RawNow*>(static_cast<const Now*>(original_ptr)));
        }
    }

    void
    enable_weak_this(...) noexcept {}

    // Only be used by atomic_shared_ptr.
    void
    clear() noexcept {
        ptr_           = nullptr;
        control_block_ = nullptr;
    }

    shared_ptr(shared_weak_count* control_block) noexcept
        : ptr_(control_block ? static_cast<pointer>(control_block->managed_pointer()) : nullptr),
          control_block_(control_block) {}

public:
    shared_ptr() noexcept
        : ptr_(nullptr), control_block_(nullptr) {}

    shared_ptr(std::nullptr_t) noexcept
        : ptr_(nullptr), control_block_(nullptr) {}

    template<class Y, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    explicit shared_ptr(Y* ptr)
        : ptr_(ptr) {
        std::unique_ptr<Y> holder(ptr);

        control_block_ = alloc_control_block(ptr, std::default_delete<Y>(), std::allocator<Y>());

        CIEL_UNUSED(holder.release());

        enable_weak_this(ptr, ptr);
    }

    template<class Y, class Deleter, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    shared_ptr(Y* ptr, Deleter d)
        : ptr_(ptr) {
        CIEL_TRY {
            control_block_ = alloc_control_block(ptr, std::move(d), std::allocator<Y>());
        }
        CIEL_CATCH (...) {
            d(ptr);
            CIEL_THROW;
        }

        enable_weak_this(ptr, ptr);
    }

    template<class Deleter>
    shared_ptr(std::nullptr_t ptr, Deleter d)
        : ptr_(nullptr) {
        CIEL_TRY {
            control_block_ = alloc_control_block(ptr_, std::move(d), std::allocator<T>());
        }
        CIEL_CATCH (...) {
            d(ptr);
            CIEL_THROW;
        }
    }

    template<class Y, class Deleter, class Alloc,
             typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    shared_ptr(Y* ptr, Deleter d, Alloc alloc)
        : ptr_(ptr) {
        CIEL_TRY {
            control_block_ = alloc_control_block(ptr, std::move(d), std::move(alloc));
        }
        CIEL_CATCH (...) {
            d(ptr);
            CIEL_THROW;
        }

        enable_weak_this(ptr, ptr);
    }

    template<class Deleter, class Alloc>
    shared_ptr(std::nullptr_t ptr, Deleter d, Alloc alloc)
        : ptr_(nullptr) {
        CIEL_TRY {
            control_block_ = alloc_control_block(ptr_, std::move(d), std::move(alloc));
        }
        CIEL_CATCH (...) {
            d(ptr);
            CIEL_THROW;
        }
    }

    template<class Y>
    shared_ptr(const shared_ptr<Y>& r, element_type* ptr) noexcept
        : ptr_(ptr), control_block_(r.control_block_) {
        if (control_block_ != nullptr) {
            control_block_->shared_add_ref();
        }
    }

    template<class Y>
    shared_ptr(shared_ptr<Y>&& r, element_type* ptr) noexcept
        : ptr_(ptr), control_block_(r.control_block_) {
        r.ptr_           = nullptr;
        r.control_block_ = nullptr;
    }

    shared_ptr(const shared_ptr& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        if (control_block_ != nullptr) {
            control_block_->shared_add_ref();
        }
    }

    template<class Y, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    shared_ptr(const shared_ptr<Y>& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        if (control_block_ != nullptr) {
            control_block_->shared_add_ref();
        }
    }

    shared_ptr(shared_ptr&& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        r.ptr_           = nullptr;
        r.control_block_ = nullptr;
    }

    template<class Y, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    shared_ptr(shared_ptr<Y>&& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        r.ptr_           = nullptr;
        r.control_block_ = nullptr;
    }

    template<class Y, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    explicit shared_ptr(const weak_ptr<Y>& r)
        : ptr_(r.ptr_),
          control_block_(r.control_block_ ? (r.control_block_->increment_if_not_zero() ? r.control_block_ : nullptr)
                                          : nullptr) {
        if (control_block_ == nullptr) {
            ciel::throw_exception(std::bad_weak_ptr());
        }
    }

    template<class Y, class Deleter, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    shared_ptr(std::unique_ptr<Y, Deleter>&& r)
        : ptr_(r.get()) {
        if (ptr_ != nullptr) {
            control_block_ = alloc_control_block(ptr_, std::move(r.get_deleter()), std::allocator<T>());

        } else {
            control_block_ = nullptr;
        }

        enable_weak_this(r.get(), r.get());

        CIEL_UNUSED(r.release());
    }

    ~shared_ptr() {
        if (control_block_ != nullptr) {
            control_block_->shared_count_release();
        }
    }

    shared_ptr&
    operator=(const shared_ptr& r) noexcept {
        shared_ptr(r).swap(*this);
        return *this;
    }

    template<class Y>
    shared_ptr&
    operator=(const shared_ptr<Y>& r) noexcept {
        shared_ptr(r).swap(*this);
        return *this;
    }

    shared_ptr&
    operator=(shared_ptr&& r) noexcept {
        shared_ptr(std::move(r)).swap(*this);
        return *this;
    }

    template<class Y>
    shared_ptr&
    operator=(shared_ptr<Y>&& r) noexcept {
        shared_ptr(std::move(r)).swap(*this);
        return *this;
    }

    template<class Y, class Deleter>
    shared_ptr&
    operator=(std::unique_ptr<Y, Deleter>&& r) {
        shared_ptr(std::move(r)).swap(*this);
        return *this;
    }

    void
    reset() noexcept {
        shared_ptr().swap(*this);
    }

    template<class Y, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    void
    reset(Y* ptr) {
        shared_ptr(ptr).swap(*this);
    }

    template<class Y, class Deleter, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    void
    reset(Y* ptr, Deleter d) {
        shared_ptr(ptr, std::move(d)).swap(*this);
    }

    template<class Y, class Deleter, class Alloc,
             typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    void
    reset(Y* ptr, Deleter d, Alloc alloc) {
        shared_ptr(ptr, std::move(d), std::move(alloc)).swap(*this);
    }

    void
    swap(shared_ptr& r) noexcept {
        using std::swap;

        swap(ptr_, r.ptr_);
        swap(control_block_, r.control_block_);
    }

    CIEL_NODISCARD element_type*
    get() const noexcept {
        return ptr_;
    }

    CIEL_NODISCARD T&
    operator*() const noexcept {
        CIEL_PRECONDITION(*this);

        return *get();
    }

    CIEL_NODISCARD T*
    operator->() const noexcept {
        CIEL_PRECONDITION(*this);

        return get();
    }

    CIEL_NODISCARD element_type&
    operator[](const size_t idx) const {
        static_assert(std::is_array<T>::value, "ciel::shared_ptr::operator[] is valid only when T is array");

        return get()[idx];
    }

    CIEL_NODISCARD size_t
    use_count() const noexcept {
        return control_block_ != nullptr ? control_block_->use_count() : 0;
    }

    CIEL_NODISCARD explicit
    operator bool() const noexcept {
        return get() != nullptr;
    }

    template<class Y>
    CIEL_NODISCARD bool
    owner_before(const shared_ptr<Y>& other) const noexcept {
        return control_block_ < other.control_block_;
    }

    template<class Y>
    CIEL_NODISCARD bool
    owner_before(const weak_ptr<Y>& other) const noexcept {
        return control_block_ < other.control_block_;
    }

    template<class D>
    CIEL_NODISCARD D*
    get_deleter() const noexcept {
#ifdef CIEL_HAS_RTTI
        return control_block_ ? static_cast<D*>(control_block_->get_deleter(typeid(typename std::remove_cv<D>::type)))
                              : nullptr;
#else
        return nullptr;
#endif
    }

}; // class shared_ptr

template<class T>
struct is_trivially_relocatable<shared_ptr<T>> : std::true_type {};

template<class T, class U>
CIEL_NODISCARD bool
operator==(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept {
    return lhs.get() == rhs.get();
}

template<class T>
CIEL_NODISCARD bool
operator==(const shared_ptr<T>& lhs, std::nullptr_t) noexcept {
    return !lhs;
}

template<class T>
CIEL_NODISCARD bool
operator==(std::nullptr_t, const shared_ptr<T>& rhs) noexcept {
    return !rhs;
}

template<class Deleter, class T>
CIEL_NODISCARD Deleter*
get_deleter(const shared_ptr<T>& p) noexcept {
    return p.template get_deleter<Deleter>();
}

#if CIEL_STD_VER >= 17
template<class T>
shared_ptr(weak_ptr<T>) -> shared_ptr<T>;

template<class T, class D>
shared_ptr(std::unique_ptr<T, D>) -> shared_ptr<T>;

#endif // CIEL_STD_VER >= 17

template<class T>
class weak_ptr {
public:
    using element_type = typename std::remove_extent<T>::type;
    using pointer      = element_type*;

private:
    element_type* ptr_;
    shared_weak_count* control_block_;

public:
    template<class>
    friend class shared_ptr;
    template<class>
    friend class weak_ptr;

    constexpr weak_ptr() noexcept
        : ptr_(nullptr), control_block_(nullptr) {}

    weak_ptr(const weak_ptr& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        if (control_block_ != nullptr) {
            control_block_->weak_add_ref();
        }
    }

    template<class Y, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    weak_ptr(const weak_ptr<Y>& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        if (control_block_ != nullptr) {
            control_block_->weak_add_ref();
        }
    }

    template<class Y, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    weak_ptr(const shared_ptr<Y>& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        if (control_block_ != nullptr) {
            control_block_->weak_add_ref();
        }
    }

    weak_ptr(weak_ptr&& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        r.ptr_           = nullptr;
        r.control_block_ = nullptr;
    }

    template<class Y, typename std::enable_if<std::is_convertible<Y*, pointer>::value, int>::type = 0>
    weak_ptr(weak_ptr<Y>&& r) noexcept
        : ptr_(r.ptr_), control_block_(r.control_block_) {
        r.ptr_           = nullptr;
        r.control_block_ = nullptr;
    }

    ~weak_ptr() {
        if (control_block_ != nullptr) {
            control_block_->weak_count_release();
        }
    }

    weak_ptr&
    operator=(const weak_ptr& r) noexcept {
        weak_ptr(r).swap(*this);
        return *this;
    }

    template<class Y>
    weak_ptr&
    operator=(const weak_ptr<Y>& r) noexcept {
        weak_ptr(r).swap(*this);
        return *this;
    }

    template<class Y>
    weak_ptr&
    operator=(const shared_ptr<Y>& r) noexcept {
        weak_ptr(r).swap(*this);
        return *this;
    }

    weak_ptr&
    operator=(weak_ptr&& r) noexcept {
        weak_ptr(std::move(r)).swap(*this);
        return *this;
    }

    template<class Y>
    weak_ptr&
    operator=(weak_ptr<Y>&& r) noexcept {
        weak_ptr(std::move(r)).swap(*this);
        return *this;
    }

    void
    reset() noexcept {
        if (control_block_ != nullptr) {
            control_block_->weak_count_release();

            ptr_           = nullptr;
            control_block_ = nullptr;
        }
    }

    void
    swap(weak_ptr& r) noexcept {
        using std::swap;

        swap(ptr_, r.ptr_);
        swap(control_block_, r.control_block_);
    }

    CIEL_NODISCARD size_t
    use_count() const noexcept {
        return control_block_ != nullptr ? control_block_->use_count() : 0;
    }

    CIEL_NODISCARD bool
    expired() const noexcept {
        return use_count() == 0;
    }

    CIEL_NODISCARD shared_ptr<T>
    lock() const noexcept {
        if (control_block_ == nullptr) {
            return shared_ptr<T>();
        }

        // private constructor used here and atomic_shared_ptr.
        return control_block_->increment_if_not_zero() ? shared_ptr<T>(control_block_) : shared_ptr<T>();
    }

    template<class Y>
    CIEL_NODISCARD bool
    owner_before(const weak_ptr<Y>& other) const noexcept {
        return control_block_ < other.control_block_;
    }

    template<class Y>
    CIEL_NODISCARD bool
    owner_before(const shared_ptr<Y>& other) const noexcept {
        return control_block_ < other.control_block_;
    }

}; // class weak_ptr

template<class T>
struct is_trivially_relocatable<weak_ptr<T>> : std::true_type {};

#if CIEL_STD_VER >= 17
template<class T>
weak_ptr(shared_ptr<T>) -> weak_ptr<T>;

#endif // #if CIEL_STD_VER >= 17

template<class T>
class enable_shared_from_this {
private:
    mutable weak_ptr<T> weak_this_;

protected:
    constexpr enable_shared_from_this() noexcept                     = default;
    enable_shared_from_this(const enable_shared_from_this&) noexcept = default;
    ~enable_shared_from_this()                                       = default;
    // clang-format off
    enable_shared_from_this& operator=(const enable_shared_from_this&) noexcept = default;
    // clang-format on

public:
    template<class>
    friend class shared_ptr;

    CIEL_NODISCARD shared_ptr<T>
    shared_from_this() {
        return shared_ptr<T>(weak_this_);
    }

    CIEL_NODISCARD shared_ptr<const T>
    shared_from_this() const {
        return shared_ptr<const T>(weak_this_);
    }

    CIEL_NODISCARD weak_ptr<T>
    weak_from_this() noexcept {
        return weak_this_;
    }

    CIEL_NODISCARD weak_ptr<const T>
    weak_from_this() const noexcept {
        return weak_this_;
    }

}; // class enable_shared_from_this

template<class T>
struct is_trivially_relocatable<enable_shared_from_this<T>> : std::true_type {};

template<class T, class Alloc, class... Args>
shared_ptr<T>
allocate_shared(const Alloc& alloc, Args&&... args) {
    static_assert(std::is_same<T, typename Alloc::value_type>::value, "");

    using control_block_type = control_block_with_instance<T, Alloc>;
    using alloc_traits       = std::allocator_traits<std::allocator<control_block_type>>;

    std::allocator<control_block_type> control_block_alloc(alloc);

    struct alloc_deleter {
    private:
        std::allocator<control_block_type> alloc_;

    public:
        alloc_deleter(std::allocator<control_block_type> a)
            : alloc_(std::move(a)) {}

        void
        operator()(control_block_type* ptr) noexcept {
            alloc_traits::deallocate(alloc_, ptr, 1);
        }

    }; // struct alloc_deleter

    std::unique_ptr<control_block_type, alloc_deleter> control_block{alloc_traits::allocate(control_block_alloc, 1),
                                                                     control_block_alloc};

    alloc_traits::construct(control_block_alloc, control_block.get(), alloc, std::forward<Args>(args)...);

    return shared_ptr<T>(control_block.release());
}

template<class T, class... Args>
shared_ptr<T>
make_shared(Args&&... args) {
    return ciel::allocate_shared<T>(std::allocator<T>(), std::forward<Args>(args)...);
}

NAMESPACE_CIEL_END

namespace std {

template<class T>
void
swap(ciel::shared_ptr<T>& lhs, ciel::shared_ptr<T>& rhs) noexcept {
    lhs.swap(rhs);
}

template<class T>
void
swap(ciel::weak_ptr<T>& lhs, ciel::weak_ptr<T>& rhs) noexcept {
    lhs.swap(rhs);
}

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_SHARED_PTR_HPP_

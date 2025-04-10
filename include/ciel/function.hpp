#ifndef CIELLAB_INCLUDE_CIEL_FUNCTION_HPP_
#define CIELLAB_INCLUDE_CIEL_FUNCTION_HPP_

#include <ciel/compare.hpp>
#include <ciel/core/aligned_storage.hpp>
#include <ciel/core/as_const.hpp>
#include <ciel/core/config.hpp>
#include <ciel/core/cstring.hpp>
#include <ciel/core/exchange.hpp>
#include <ciel/core/is_trivially_relocatable.hpp>
#include <ciel/core/message.hpp>
#include <ciel/core/strip_signature.hpp>
#include <ciel/swap.hpp>

#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <typeinfo>

NAMESPACE_CIEL_BEGIN

// Inspired by LLVM libc++'s implementation.

class bad_function_call : public std::exception {
public:
    CIEL_NODISCARD const char* what() const noexcept override {
        return "ciel::bad_function_call";
    }

}; // class bad_function_call

namespace details {

template<class>
class func_base;

template<class R, class... Args>
class func_base<R(Args...)> {
public:
    func_base(const func_base&)            = delete;
    func_base& operator=(const func_base&) = delete;

protected:
    func_base()  = default;
    ~func_base() = default;

public:
    CIEL_NODISCARD virtual func_base* clone() const = 0;
    virtual void clone_to(void*) const              = 0;
    virtual void destroy() noexcept                 = 0;
    virtual void destroy_and_deallocate() noexcept  = 0;
    virtual R operator()(Args&&...) const           = 0;
#ifdef CIEL_HAS_RTTI
    CIEL_NODISCARD virtual const void* target(const std::type_info&) const noexcept = 0;
    CIEL_NODISCARD virtual const std::type_info& target_type() const noexcept       = 0;
#endif

}; // class func_base<R(Args...)>

template<class, class>
class func;

template<class F, class R, class... Args>
class func<F, R(Args...)> final : public func_base<R(Args...)> {
public:
    func(const func&)            = delete;
    func& operator=(const func&) = delete;

private:
    F f_;

public:
    explicit func(const F& f)
        : f_(f) {}

    explicit func(F&& f) noexcept
        : f_(std::move(f)) {}

    // When callable object is large.
    CIEL_NODISCARD func_base<R(Args...)>* clone() const override {
        return new func(f_);
    }

    // When stored object is large.
    void destroy_and_deallocate() noexcept override {
        delete this;
    }

    // When callable object is small, construct on other's buffer_.
    void clone_to(void* buffer) const override {
        ::new (buffer) func(f_);
    }

    // When stored object is small and stored on buffer_.
    void destroy() noexcept override {
        this->~func();
    }

    R operator()(Args&&... args) const override {
        return f_(std::forward<Args>(args)...);
    }

#ifdef CIEL_HAS_RTTI
    CIEL_NODISCARD const void* target(const std::type_info& ti) const noexcept override {
        if (ti == typeid(F)) {
            return std::addressof(f_);
        }

        return nullptr;
    }

    CIEL_NODISCARD const std::type_info& target_type() const noexcept override {
        return typeid(F);
    }
#endif

}; // class func<F, R(Args...)>

} // namespace details

template<class T>
struct is_small_object : bool_constant<sizeof(T) <= sizeof(void*) * 3 && alignof(void*) % alignof(T) == 0
                                       && is_trivially_relocatable<T>::value> {};

struct assume_trivially_relocatable_t {};

static constexpr assume_trivially_relocatable_t assume_trivially_relocatable;

template<class>
class function;

template<class R, class... Args>
class function<R(Args...)> {
public:
    using result_type = R;

private:
    using buffer_type = aligned_storage<sizeof(void*) * 3, alignof(void*)>::type;
    using base_type   = details::func_base<R(Args...)>;

    buffer_type buffer_;
    uintptr_t f_{0};

    enum struct state {
        Null,
        Small,
        Large
    };

    CIEL_NODISCARD state check_state() const noexcept {
        if (f_ == 1) {
            return state::Small;
        }

        return f_ == 0 ? state::Null : state::Large;
    }

    CIEL_NODISCARD base_type* stack_ptr() const noexcept {
        return const_cast<base_type*>(reinterpret_cast<const base_type*>(std::addressof(buffer_)));
    }

    CIEL_NODISCARD base_type* heap_ptr() const noexcept {
        return reinterpret_cast<base_type*>(f_);
    }

    CIEL_NODISCARD base_type* ptr() const noexcept {
        if (f_ == 1) {
            return stack_ptr();
        }

        return heap_ptr();
    }

    void clear() noexcept {
        switch (check_state()) {
            case state::Null :
                return;
            case state::Small :
                stack_ptr()->destroy();
                break;
            case state::Large :
                heap_ptr()->destroy_and_deallocate();
        }

        f_ = 0;
    }

    template<class F>
    CIEL_NODISCARD static bool not_null(const F&) noexcept {
        return true;
    }

    template<class F>
    CIEL_NODISCARD static bool not_null(F* ptr) noexcept {
        return ptr;
    }

    template<class Ret, class Class>
    CIEL_NODISCARD static bool not_null(Ret Class::* ptr) noexcept {
        return ptr;
    }

    template<class F>
    CIEL_NODISCARD static bool not_null(const function<F>& f) noexcept {
        return static_cast<bool>(f);
    }

public:
    function() = default;

    function(nullptr_t) noexcept {}

    function(const function& other) {
        switch (other.check_state()) {
            case state::Null :
                break;
            case state::Small :
                other.stack_ptr()->clone_to(std::addressof(buffer_));
                f_ = 1;
                break;
            case state::Large :
                f_ = reinterpret_cast<uintptr_t>(other.heap_ptr()->clone());
        }
    }

    function(function&& other) noexcept {
        switch (other.check_state()) {
            case state::Null :
                break;
            case state::Small :
                ciel::memcpy(std::addressof(buffer_), std::addressof(other.buffer_), sizeof(buffer_type));
                other.f_ = 0;
                f_       = 1;
                break;
            case state::Large :
                f_ = ciel::exchange(other.f_, 0);
        }
    }

    template<class F, class DecayF = decay_t<F>, enable_if_t<!std::is_same<DecayF, function>::value> = 0>
    function(F&& f) {
        using func_type = details::func<DecayF, R(Args...)>;

        if CIEL_LIKELY (function::not_null(f)) {
            if (is_small_object<DecayF>::value) {
                ::new (stack_ptr()) func_type(std::forward<F>(f));
                f_ = 1;

            } else {
                f_ = reinterpret_cast<uintptr_t>(new func_type(std::forward<F>(f)));
            }
        }
    }

    template<class F, class DecayF = decay_t<F>,
             enable_if_t<sizeof(DecayF) <= sizeof(void*) * 3 && alignof(void*) % alignof(DecayF) == 0> = 0>
    function(assume_trivially_relocatable_t, F&& f) {
        using func_type = details::func<DecayF, R(Args...)>;

        if CIEL_LIKELY (function::not_null(f)) {
            ::new (stack_ptr()) func_type(std::forward<F>(f));
            f_ = 1;
        }
    }

    ~function() {
        clear();
    }

    void assign(const function& other) {
        if CIEL_UNLIKELY (this == std::addressof(other)) {
            return;
        }

        clear();

        switch (other.check_state()) {
            case state::Null :
                break;
            case state::Small :
                other.stack_ptr()->clone_to(std::addressof(buffer_));
                f_ = 1;
                break;
            case state::Large :
                f_ = reinterpret_cast<uintptr_t>(other.heap_ptr()->clone());
        }
    }

    void assign(function&& other) noexcept {
        if CIEL_UNLIKELY (this == std::addressof(other)) {
            return;
        }

        clear();

        switch (other.check_state()) {
            case state::Null :
                break;
            case state::Small :
                ciel::memcpy(std::addressof(buffer_), std::addressof(other.buffer_), sizeof(buffer_type));
                other.f_ = 0;
                f_       = 1;
                break;
            case state::Large :
                f_ = ciel::exchange(other.f_, 0);
        }
    }

    void assign(nullptr_t) noexcept {
        clear();
    }

    template<class F, class DecayF = decay_t<F>>
    void assign(F&& f) {
        clear();

        using func_type = details::func<DecayF, R(Args...)>;

        if CIEL_LIKELY (function::not_null(f)) {
            if (is_small_object<DecayF>::value) {
                ::new (stack_ptr()) func_type(std::forward<F>(f));
                f_ = 1;

            } else {
                f_ = reinterpret_cast<uintptr_t>(new func_type(std::forward<F>(f)));
            }
        }
    }

    template<class F, class DecayF = decay_t<F>,
             enable_if_t<sizeof(DecayF) <= sizeof(void*) * 3 && alignof(void*) % alignof(DecayF) == 0> = 0>
    void assign(assume_trivially_relocatable_t, F&& f) {
        clear();

        using func_type = details::func<DecayF, R(Args...)>;

        if CIEL_LIKELY (function::not_null(f)) {
            ::new (stack_ptr()) func_type(std::forward<F>(f));
            f_ = 1;
        }
    }

    function& operator=(const function& other) {
        assign(other);
        return *this;
    }

    function& operator=(function&& other) noexcept {
        assign(std::move(other));
        return *this;
    }

    function& operator=(nullptr_t) noexcept {
        clear();
        return *this;
    }

    template<class F>
    function& operator=(F&& f) {
        assign(std::forward<F>(f));
        return *this;
    }

    void swap(function& other) noexcept {
        ciel::relocatable_swap(*this, other);
    }

    CIEL_NODISCARD explicit operator bool() const noexcept {
        return f_ != 0;
    }

    R operator()(Args... args) const {
        const base_type* p = ptr();

        if CIEL_UNLIKELY (p == nullptr) {
            CIEL_THROW_EXCEPTION(ciel::bad_function_call{});
        }

        return (*p)(std::forward<Args>(args)...);
    }

#ifdef CIEL_HAS_RTTI
    CIEL_NODISCARD const std::type_info& target_type() const noexcept {
        const base_type* p = ptr();

        if (p == nullptr) {
            return typeid(void);
        }

        return p->target_type();
    }
#endif

    template<class T>
    CIEL_NODISCARD T* target() noexcept {
        return const_cast<T*>(ciel::as_const(*this).target());
    }

    template<class T>
    CIEL_NODISCARD const T* target() const noexcept {
#ifdef CIEL_HAS_RTTI
        const base_type* p = ptr();

        if (p == nullptr) {
            return nullptr;
        }

        return static_cast<const T*>(p->target(typeid(T)));
#else
        return nullptr;
#endif
    }

}; // class function<R(Args...)>

static_assert(sizeof(function<void()>) == 32, "");

template<class R, class... Args>
struct is_trivially_relocatable<function<R(Args...)>> : std::true_type {};

template<class R, class... ArgTypes>
CIEL_NODISCARD bool operator==(const function<R(ArgTypes...)>& f, nullptr_t) noexcept {
    return !f;
}

// When the target is a function pointer or a std::reference_wrapper, small object optimization is guaranteed.
template<class T>
struct is_trivially_relocatable<std::reference_wrapper<T>> : std::true_type {};

static_assert(is_small_object<std::reference_wrapper<void()>>::value,
              "SOO should be guaranteed for std::reference_wrapper");

#if CIEL_STD_VER >= 17

template<class R, class... Args>
function(R (*)(Args...)) -> function<R(Args...)>;

template<class F>
function(F) -> function<strip_signature_t<decltype(&F::operator())>>;

#endif

NAMESPACE_CIEL_END

namespace std {

template<class R, class... Args>
void swap(ciel::function<R(Args...)>& lhs, ciel::function<R(Args...)>& rhs) noexcept {
    lhs.swap(rhs);
}

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_FUNCTION_HPP_

#ifndef CIELLAB_INCLUDE_CIEL_FUNCTION_HPP_
#define CIELLAB_INCLUDE_CIEL_FUNCTION_HPP_

#include <cstddef>
#include <cstring>
#include <functional>
#include <memory>
#include <typeinfo>

#ifndef CIELLAB_INCLUDE_CIEL_CONFIG_HPP_
#define CIELLAB_INCLUDE_CIEL_CONFIG_HPP_

#if !defined(__cplusplus) || (__cplusplus < 201103L)
#error "Please use C++ with standard of at least 11"
#endif

#include <cassert>
#include <exception>
#include <iostream>
#include <type_traits>

// exception
#ifdef __cpp_exceptions
#define CIEL_HAS_EXCEPTIONS
#endif

// rtti
#ifdef __cpp_rtti
#define CIEL_HAS_RTTI
#endif

// debug_mode
#ifndef NDEBUG
#define CIEL_IS_DEBUGGING
#endif

// standard_version
#if __cplusplus <= 201103L
#define CIEL_STD_VER 11
#elif __cplusplus <= 201402L
#define CIEL_STD_VER 14
#elif __cplusplus <= 201703L
#define CIEL_STD_VER 17
#elif __cplusplus <= 202002L
#define CIEL_STD_VER 20
#elif __cplusplus <= 202302L
#define CIEL_STD_VER 23
#else
#define CIEL_STD_VER 26
#endif

// constexpr
#if CIEL_STD_VER >= 14
#define CIEL_CONSTEXPR_SINCE_CXX14 constexpr
#else
#define CIEL_CONSTEXPR_SINCE_CXX14
#endif

#if CIEL_STD_VER >= 17
#define CIEL_CONSTEXPR_SINCE_CXX17 constexpr
#else
#define CIEL_CONSTEXPR_SINCE_CXX17
#endif

#if CIEL_STD_VER >= 20
#define CIEL_CONSTEXPR_SINCE_CXX20 constexpr
#else
#define CIEL_CONSTEXPR_SINCE_CXX20
#endif

#if CIEL_STD_VER >= 23
#define CIEL_CONSTEXPR_SINCE_CXX23 constexpr
#else
#define CIEL_CONSTEXPR_SINCE_CXX23
#endif

// nodiscard
#if CIEL_STD_VER >= 17
#define CIEL_NODISCARD [[nodiscard]]
#elif (defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__) // clang, icc, clang-cl
#define CIEL_NODISCARD __attribute__((warn_unused_result))
#elif defined(_HAS_NODISCARD)
#define CIEL_NODISCARD _NODISCARD
#elif _MSC_VER >= 1700
#define CIEL_NODISCARD _Check_return_
#else
#define CIEL_NODISCARD
#endif

// likely, unlikely
#if CIEL_STD_VER >= 20 || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#define CIEL_LIKELY(x)   (x) [[likely]]
#define CIEL_UNLIKELY(x) (x) [[unlikely]]
#elif defined(__GNUC__) || defined(__clang__)
#define CIEL_LIKELY(x)   (__builtin_expect(!!(x), true))
#define CIEL_UNLIKELY(x) (__builtin_expect(!!(x), false))
#else
#define CIEL_LIKELY(x)   (x)
#define CIEL_UNLIKELY(x) (x)
#endif

// __has_builtin
#ifndef __has_builtin
#define __has_builtin(x) false
#endif

// try catch throw
#ifdef CIEL_HAS_EXCEPTIONS
#define CIEL_TRY      try
#define CIEL_CATCH(X) catch (X)
#define CIEL_THROW    throw
#else
#define CIEL_TRY      if CIEL_CONSTEXPR_SINCE_CXX17 (true)
#define CIEL_CATCH(X) else
#define CIEL_THROW
#endif

// namespace ciel
#define NAMESPACE_CIEL_BEGIN namespace ciel {
#define NAMESPACE_CIEL_END   } // namespace ciel

using std::ptrdiff_t;
using std::size_t;

NAMESPACE_CIEL_BEGIN

template<class... Args>
void
void_cast(Args&&...) noexcept {}

[[noreturn]] inline void
unreachable() noexcept {
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
    __assume(false);
#else                                        // GCC, Clang
    __builtin_unreachable();
#endif
}

template<class Exception, typename std::enable_if<std::is_base_of<std::exception, Exception>::value, int>::type = 0>
[[noreturn]] inline void
throw_exception(Exception&& e) {
#ifdef CIEL_HAS_EXCEPTIONS
    throw e;
#else
    std::cerr << e.what() << "\n";
    std::terminate();
#endif
}

NAMESPACE_CIEL_END

// unused
// simple (void) cast won't stop gcc
#define CIEL_UNUSED(x) ciel::void_cast(x)

// assume
#if CIEL_STD_VER >= 23 && ((defined(__clang__) && __clang__ >= 19) || (defined(__GNUC__) && __GNUC__ >= 13))
#define CIEL_ASSUME(cond) [[assume(cond)]]
#elif defined(__clang__)
#if __has_builtin(__builtin_assume)
#define CIEL_ASSUME(cond) __builtin_assume(cond)
#else
#define CIEL_ASSUME(cond) CIEL_UNUSED(cond)
#endif // defined(__clang__)
#elif defined(_MSC_VER)
#define CIEL_ASSUME(cond) __assume(cond)
#elif defined(__GNUC__) && __GNUC__ >= 13
#define CIEL_ASSUME(cond) __attribute__((assume(cond)))
#else
#define CIEL_ASSUME(cond) CIEL_UNUSED(cond)
#endif

// assert
#ifdef CIEL_IS_DEBUGGING
#define CIEL_ASSERT(cond) assert(cond)
#else
#define CIEL_ASSERT(cond) CIEL_ASSUME(cond)
#endif

#define CIEL_PRECONDITION(cond)  CIEL_ASSERT(static_cast<bool>(cond))
#define CIEL_POSTCONDITION(cond) CIEL_ASSERT(static_cast<bool>(cond))

// deduction guide for initializer_list
#if CIEL_STD_VER >= 17
#include <initializer_list>

namespace std {

template<class T>
initializer_list(initializer_list<T>) -> initializer_list<T>;

} // namespace std
#endif

#endif // CIELLAB_INCLUDE_CIEL_CONFIG_HPP_
#ifndef CIELLAB_INCLUDE_CIEL_STRIP_SIGNATURE_HPP_
#define CIELLAB_INCLUDE_CIEL_STRIP_SIGNATURE_HPP_


NAMESPACE_CIEL_BEGIN

#if CIEL_STD_VER >= 17

template<class>
struct strip_signature;

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...)> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) const> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) volatile> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) const volatile> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...)&> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) const&> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) volatile&> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) const volatile&> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) noexcept> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) const noexcept> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) volatile noexcept> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) const volatile noexcept> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) & noexcept> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) const & noexcept> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) volatile & noexcept> {
    using type = R(Args...);
};

template<class R, class Class, class... Args>
struct strip_signature<R (Class::*)(Args...) const volatile & noexcept> {
    using type = R(Args...);
};

template<class F>
using strip_signature_t = typename strip_signature<F>::type;

#endif

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_STRIP_SIGNATURE_HPP_
#ifndef CIELLAB_INCLUDE_CIEL_TYPE_TRAITS_HPP_
#define CIELLAB_INCLUDE_CIEL_TYPE_TRAITS_HPP_

#include <cstring>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>


NAMESPACE_CIEL_BEGIN

// void_t
template<class...>
using void_t = void;

// conjunction
// When the template pack is empty, derive from true_type.
template<class...>
struct conjunction : std::true_type {};

// Otherwise, derive from the first false template member (if all true, choose the last one).
template<class B1, class... Bn>
struct conjunction<B1, Bn...> : std::conditional<static_cast<bool>(B1::value), conjunction<Bn...>, B1>::type {};

// disjunction
template<class...>
struct disjunction : std::false_type {};

template<class B1, class... Bn>
struct disjunction<B1, Bn...> : std::conditional<static_cast<bool>(B1::value), B1, disjunction<Bn...>>::type {};

// is_exactly_input_iterator
template<class Iter, class = void>
struct is_exactly_input_iterator : std::false_type {};

template<class Iter>
struct is_exactly_input_iterator<Iter, void_t<typename std::iterator_traits<Iter>::iterator_category>>
    : std::is_same<typename std::iterator_traits<Iter>::iterator_category, std::input_iterator_tag> {};

// is_forward_iterator
template<class Iter, class = void>
struct is_forward_iterator : std::false_type {};

template<class Iter>
struct is_forward_iterator<Iter, void_t<typename std::iterator_traits<Iter>::iterator_category>>
    : std::is_convertible<typename std::iterator_traits<Iter>::iterator_category, std::forward_iterator_tag> {};

// is_input_iterator
template<class Iter, class = void>
struct is_input_iterator : std::false_type {};

template<class Iter>
struct is_input_iterator<Iter, void_t<typename std::iterator_traits<Iter>::iterator_category>>
    : std::is_convertible<typename std::iterator_traits<Iter>::iterator_category, std::input_iterator_tag> {};

// is_trivially_relocatable
template<class T, class = void>
struct is_trivially_relocatable : disjunction<std::is_empty<T>, std::is_trivially_copyable<T>> {};

#ifdef _LIBCPP___TYPE_TRAITS_IS_TRIVIALLY_RELOCATABLE_H
template<class T>
struct is_trivially_relocatable<T, typename std::enable_if<std::__libcpp_is_trivially_relocatable<T>::value>::type>
    : std::true_type {};
#else
template<class First, class Second>
struct is_trivially_relocatable<std::pair<First, Second>>
    : conjunction<is_trivially_relocatable<First>, is_trivially_relocatable<Second>> {};

template<class... Types>
struct is_trivially_relocatable<std::tuple<Types...>> : conjunction<is_trivially_relocatable<Types>...> {};
#endif

// useless_tag
struct useless_tag {
    useless_tag(...) noexcept {}
}; // struct useless_tag

// owner
template<class T, class = typename std::enable_if<std::is_pointer<T>::value>::type>
using owner = T;

// is_final
#if CIEL_STD_VER >= 14
template<class T>
using is_final = std::is_final<T>;
#elif __has_builtin(__is_final)
template<class T>
struct is_final : std::integral_constant<bool, __is_final(T)> {};
#else
// If is_final is not available, it may be better to manually write explicit template specializations.
// e.g.
// template<>
// struct is_final<NotFinalObject> : std::false_type {};
//
template<class T>
struct is_final;
#endif

// is_const_lvalue_reference
template<class T>
struct is_const_lvalue_reference : std::false_type {};

template<class T>
struct is_const_lvalue_reference<const T&> : std::true_type {};

// is_const_rvalue_reference
template<class T>
struct is_const_rvalue_reference : std::false_type {};

template<class T>
struct is_const_rvalue_reference<const T&&> : std::true_type {};

// is_const_reference
template<class T>
struct is_const_reference
    : std::integral_constant<bool, is_const_lvalue_reference<T>::value || is_const_rvalue_reference<T>::value> {};

// worth_move
// FIXME: Current implementation returns true for const&& constructor and assignment.
template<class T>
struct worth_move {
    static_assert(!std::is_const<T>::value, "");

private:
    using U = typename std::decay<T>::type;

    struct helper {
        operator const U&() noexcept;
        operator U&&() noexcept;
    }; // struct helper

public:
    static constexpr bool construct = std::is_class<T>::value && !std::is_trivial<T>::value
                                   && std::is_move_constructible<T>::value && !std::is_constructible<T, helper>::value;
    static constexpr bool assign = std::is_class<T>::value && !std::is_trivial<T>::value
                                && std::is_move_assignable<T>::value && !std::is_assignable<T, helper>::value;
    static constexpr bool value = construct || assign;

}; // struct worth_move_constructing

template<class T>
struct worth_move_constructing {
    static constexpr bool value = worth_move<T>::construct;

}; // worth_move_constructing

template<class T>
struct worth_move_assigning {
    static constexpr bool value = worth_move<T>::assign;

}; // worth_move_assigning

#if CIEL_STD_VER >= 20
// is_complete_type
template<class T, auto = [] {}>
inline constexpr bool is_complete_type_v = requires { sizeof(T); };
#endif // CIEL_STD_VER >= 20

// aligned_storage
template<size_t size, size_t alignment>
struct aligned_storage {
    static_assert(sizeof(unsigned char) == 1, "");

    class type {
        alignas(alignment) unsigned char buffer_[size]{};
    };

}; // aligned_storage

// buffer_cast
template<class Pointer, typename std::enable_if<std::is_pointer<Pointer>::value, int>::type = 0>
Pointer
buffer_cast(const void* ptr) noexcept {
    return static_cast<Pointer>(const_cast<void*>(ptr));
}

// exchange
template<class T, class U = T>
T
exchange(T& obj, U&& new_value) noexcept(std::is_nothrow_move_constructible<T>::value
                                         && std::is_nothrow_assignable<T&, U>::value) {
    T old_value = std::move(obj);
    obj         = std::forward<U>(new_value);
    return old_value;
}

// Is a pointer aligned?
inline bool
is_aligned(void* ptr, const size_t alignment) noexcept {
    CIEL_PRECONDITION(ptr != nullptr);
    CIEL_PRECONDITION(alignment != 0);

    return ((uintptr_t)ptr % alignment) == 0;
}

// Align upwards
inline uintptr_t
align_up(uintptr_t sz, const size_t alignment) noexcept {
    CIEL_PRECONDITION(alignment != 0);

    const uintptr_t mask = alignment - 1;

    if CIEL_LIKELY ((alignment & mask) == 0) { // power of two?
        return (sz + mask) & ~mask;

    } else {
        return ((sz + mask) / alignment) * alignment;
    }
}

// Align downwards
inline uintptr_t
align_down(uintptr_t sz, const size_t alignment) noexcept {
    CIEL_PRECONDITION(alignment != 0);

    uintptr_t mask = alignment - 1;

    if CIEL_LIKELY ((alignment & mask) == 0) { // power of two?
        return (sz & ~mask);

    } else {
        return ((sz / alignment) * alignment);
    }
}

// sizeof_without_back_padding
//
// Derived can reuse Base's back padding.
// e.g.
// struct Base {
//     alignas(8) unsigned char buf[1]{};
// };
// struct Derived : Base {
//     int i{};
// };
// static_assert(sizeof(Base)    == 8, "");
// static_assert(sizeof(Derived) == 8, "");
//
template<class T, size_t BackPadding = alignof(T)>
struct sizeof_without_back_padding {
    static_assert(std::is_class<T>::value && !is_final<T>::value, "");

    struct S : T {
        unsigned char buf[BackPadding]{};
    };

    using type = typename std::conditional<sizeof(S) == sizeof(T), sizeof_without_back_padding,
                                           typename sizeof_without_back_padding<T, BackPadding - 1>::type>::type;

    static constexpr size_t Byte = BackPadding;

    static constexpr size_t value = sizeof(T) - type::Byte;

}; // struct sizeof_without_back_padding

template<class T>
struct sizeof_without_back_padding<T, 0> {
    static_assert(std::is_class<T>::value && !is_final<T>::value, "");

    using type = sizeof_without_back_padding;

    static constexpr size_t Byte = 0;

    static constexpr size_t value = sizeof(T);

}; // struct sizeof_without_back_padding<T, 0>

// is_overaligned_for_new
inline bool
is_overaligned_for_new(const size_t alignment) noexcept {
#ifdef __STDCPP_DEFAULT_NEW_ALIGNMENT__
    return alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__;
#else
    return alignment > alignof(std::max_align_t);
#endif
}

// allocate
template<class T>
T*
allocate(const size_t n) {
#if CIEL_STD_VER >= 17
    if CIEL_UNLIKELY (ciel::is_overaligned_for_new(alignof(T))) {
        return static_cast<T*>(::operator new(sizeof(T) * n, static_cast<std::align_val_t>(alignof(T))));
    }
#endif
    return static_cast<T*>(::operator new(sizeof(T) * n));
}

// deallocate
template<class T>
void
deallocate(T* ptr) noexcept {
#if CIEL_STD_VER >= 17
    if CIEL_UNLIKELY (ciel::is_overaligned_for_new(alignof(T))) {
        ::operator delete(ptr, static_cast<std::align_val_t>(alignof(T)));
    }
#endif
    ::operator delete(ptr);
}

template<class T, bool Valid = is_trivially_relocatable<T>::value>
void
relocatable_swap(T& lhs, T& rhs) noexcept {
    static_assert(Valid, "T must be trivially relocatable, you can explicitly assume it.");

    typename aligned_storage<sizeof(T), alignof(T)>::type buffer;

    std::memcpy(&buffer, &rhs, sizeof(T));
    std::memcpy(&rhs, &lhs, sizeof(T));
    std::memcpy(&lhs, &buffer, sizeof(T));
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TYPE_TRAITS_HPP_

NAMESPACE_CIEL_BEGIN

template<class>
class function;

namespace details {

template<class>
class func_base;

template<class R, class... Args>
class func_base<R(Args...)> {
public:
    func_base(const func_base&) = delete;
    func_base(func_base&&)      = delete;
    // clang-format off
    func_base& operator=(const func_base&) = delete;
    func_base& operator=(func_base&&)      = delete;
    // clang-format on

protected:
    func_base() noexcept = default;
    ~func_base()         = default;

public:
    CIEL_NODISCARD virtual func_base*
    clone() const
        = 0;
    virtual void
    clone_to(void*) const
        = 0;
    virtual void
    destroy() noexcept
        = 0;
    virtual void
    destroy_and_deallocate() noexcept
        = 0;
    virtual R
    operator()(Args&&...) const
        = 0;
    CIEL_NODISCARD virtual const void*
    target(const std::type_info&) const noexcept
        = 0;
#ifdef CIEL_HAS_RTTI
    CIEL_NODISCARD virtual const std::type_info&
    target_type() const noexcept
        = 0;
#endif

}; // class func_base<R(Args...)>

template<class, class>
class func;

template<class F, class R, class... Args>
class func<F, R(Args...)> final : public func_base<R(Args...)> {
public:
    func(const func&) = delete;
    func(func&&)      = delete;
    // clang-format off
    func& operator=(const func&) = delete;
    func& operator=(func&&)      = delete;
    // clang-format on

private:
    F f_;

public:
    explicit func(const F& f)
        : f_(f) {}

    explicit func(F&& f) noexcept
        : f_(std::move(f)) {}

    // When callable object is large.
    CIEL_NODISCARD virtual func_base<R(Args...)>*
    clone() const override {
        func* res = ciel::allocate<func>(1);

        CIEL_TRY {
            ::new (res) func(f_);
        }
        CIEL_CATCH (...) {
            ciel::deallocate<func>(res);
            CIEL_THROW;
        }

        return res;
    }

    // When stored object is large.
    virtual void
    destroy_and_deallocate() noexcept override {
        this->~func();
        ciel::deallocate<func>(this);
    }

    // When callable object is small, construct on other's buffer_.
    virtual void
    clone_to(void* buffer) const override {
        ::new (buffer) func(f_);
    }

    // When stored object is small and stored on buffer_.
    virtual void
    destroy() noexcept override {
        this->~func();
    }

    virtual R
    operator()(Args&&... args) const override {
        return f_(std::forward<Args>(args)...);
    }

    CIEL_NODISCARD virtual const void*
    target(const std::type_info& ti) const noexcept override {
        CIEL_UNUSED(ti);

        if (true
#ifdef CIEL_HAS_RTTI
            && ti == typeid(F)
#endif
        ) {
            return std::addressof(f_);
        }

        return nullptr;
    }

#ifdef CIEL_HAS_RTTI
    CIEL_NODISCARD virtual const std::type_info&
    target_type() const noexcept override {
        return typeid(F);
    }
#endif

}; // class func<F, R(Args...)>

} // namespace details

template<class T>
struct is_small_object : std::integral_constant<bool, sizeof(T) <= sizeof(void*) * 3 && alignof(void*) % alignof(T) == 0
                                                          && is_trivially_relocatable<T>::value> {};

struct assume_trivially_relocatable_t {};

constexpr assume_trivially_relocatable_t assume_trivially_relocatable_tag;

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

    CIEL_NODISCARD state
    check_state() const noexcept {
        if (f_ == 1) {
            return state::Small;
        }

        return f_ == 0 ? state::Null : state::Large;
    }

    CIEL_NODISCARD base_type*
    stack_ptr() const noexcept {
        return buffer_cast<base_type*>(&buffer_);
    }

    CIEL_NODISCARD base_type*
    heap_ptr() const noexcept {
        return (base_type*)f_;
    }

    CIEL_NODISCARD base_type*
    ptr() const noexcept {
        if (f_ == 1) {
            return stack_ptr();
        }

        return heap_ptr();
    }

    void
    clear() noexcept {
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
    CIEL_NODISCARD static bool
    not_null(const F&) noexcept {
        return true;
    }

    template<class F>
    CIEL_NODISCARD static bool
    not_null(F* ptr) noexcept {
        return ptr;
    }

    template<class Ret, class Class>
    CIEL_NODISCARD static bool
    not_null(Ret Class::* ptr) noexcept {
        return ptr;
    }

    template<class F>
    CIEL_NODISCARD static bool
    not_null(const function<F>& f) noexcept {
        return static_cast<bool>(f);
    }

public:
    function() noexcept = default;

    function(std::nullptr_t) noexcept {}

    function(const function& other) {
        switch (other.check_state()) {
            case state::Null :
                break;
            case state::Small :
                other.stack_ptr()->clone_to(&buffer_);
                f_ = 1;
                break;
            case state::Large :
                f_ = (uintptr_t)(other.heap_ptr()->clone());
        }
    }

    function(function&& other) noexcept {
        switch (other.check_state()) {
            case state::Null :
                break;
            case state::Small :
                std::memcpy(&buffer_, &other.buffer_, sizeof(buffer_type));
                other.f_ = 0;
                f_       = 1;
                break;
            case state::Large :
                f_ = ciel::exchange(other.f_, 0);
        }
    }

    template<class F, class DecayF = typename std::decay<F>::type,
             typename std::enable_if<!std::is_same<DecayF, function>::value, int>::type = 0>
    function(F&& f) {
        using func_type = details::func<DecayF, R(Args...)>;

        if CIEL_LIKELY (function::not_null(f)) {
            if CIEL_CONSTEXPR_SINCE_CXX17 (is_small_object<DecayF>::value) {
                ::new (stack_ptr()) func_type(std::forward<F>(f));
                f_ = 1;

            } else {
                func_type* res = ciel::allocate<func_type>(1);

                CIEL_TRY {
                    ::new (res) func_type(std::forward<F>(f));
                }
                CIEL_CATCH (...) {
                    ciel::deallocate<func_type>(res);
                    CIEL_THROW;
                }

                f_ = (uintptr_t)res;
            }
        }
    }

    template<
        class F, class DecayF = typename std::decay<F>::type,
        typename std::enable_if<sizeof(DecayF) <= sizeof(void*) * 3 && alignof(void*) % alignof(DecayF) == 0, int>::type
        = 0>
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

    void
    assign(const function& other) {
        if CIEL_UNLIKELY (this == std::addressof(other)) {
            return;
        }

        clear();

        switch (other.check_state()) {
            case state::Null :
                break;
            case state::Small :
                other.stack_ptr()->clone_to(&buffer_);
                f_ = 1;
                break;
            case state::Large :
                f_ = (uintptr_t)(other.heap_ptr()->clone());
        }
    }

    void
    assign(function&& other) noexcept {
        if CIEL_UNLIKELY (this == std::addressof(other)) {
            return;
        }

        clear();

        switch (other.check_state()) {
            case state::Null :
                break;
            case state::Small :
                std::memcpy(&buffer_, &other.buffer_, sizeof(buffer_type));
                other.f_ = 0;
                f_       = 1;
                break;
            case state::Large :
                f_ = ciel::exchange(other.f_, 0);
        }
    }

    void
    assign(std::nullptr_t) noexcept {
        clear();
    }

    template<class F, class DecayF = typename std::decay<F>::type>
    void
    assign(F&& f) {
        clear();

        using func_type = details::func<DecayF, R(Args...)>;

        if CIEL_LIKELY (function::not_null(f)) {
            if CIEL_CONSTEXPR_SINCE_CXX17 (is_small_object<DecayF>::value) {
                ::new (stack_ptr()) func_type(std::forward<F>(f));
                f_ = 1;

            } else {
                func_type* res = ciel::allocate<func_type>(1);

                CIEL_TRY {
                    ::new (res) func_type(std::forward<F>(f));
                }
                CIEL_CATCH (...) {
                    ciel::deallocate<func_type>(res);
                    CIEL_THROW;
                }

                f_ = (uintptr_t)res;
            }
        }
    }

    template<
        class F, class DecayF = typename std::decay<F>::type,
        typename std::enable_if<sizeof(DecayF) <= sizeof(void*) * 3 && alignof(void*) % alignof(DecayF) == 0, int>::type
        = 0>
    void
    assign(assume_trivially_relocatable_t, F&& f) {
        clear();

        using func_type = details::func<DecayF, R(Args...)>;

        if CIEL_LIKELY (function::not_null(f)) {
            ::new (stack_ptr()) func_type(std::forward<F>(f));
            f_ = 1;
        }
    }

    function&
    operator=(const function& other) {
        assign(other);
        return *this;
    }

    function&
    operator=(function&& other) noexcept {
        assign(std::move(other));
        return *this;
    }

    function&
    operator=(std::nullptr_t) noexcept {
        clear();
        return *this;
    }

    template<class F>
    function&
    operator=(F&& f) {
        assign(std::forward<F>(f));
        return *this;
    }

    void
    swap(function& other) noexcept {
        ciel::relocatable_swap(*this, other);
    }

    CIEL_NODISCARD explicit
    operator bool() const noexcept {
        return f_ != 0;
    }

    R
    operator()(Args... args) const {
        const base_type* p = ptr();

        if CIEL_UNLIKELY (p == nullptr) {
            ciel::throw_exception(std::bad_function_call());
        }

        return (*p)(std::forward<Args>(args)...);
    }

#ifdef CIEL_HAS_RTTI
    CIEL_NODISCARD const std::type_info&
    target_type() const noexcept {
        const base_type* p = ptr();

        if (p == nullptr) {
            return typeid(void);
        }

        return p->target_type();
    }
#endif

    template<class T>
    CIEL_NODISCARD T*
    target() noexcept {
#ifdef CIEL_HAS_RTTI
        const base_type* p = ptr();

        if (p == nullptr) {
            return nullptr;
        }

        return const_cast<T*>(static_cast<const T*>(p->target(typeid(T))));
#else
        return nullptr;
#endif
    }

    template<class T>
    CIEL_NODISCARD const T*
    target() const noexcept {
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
CIEL_NODISCARD bool
operator==(const function<R(ArgTypes...)>& f, std::nullptr_t) noexcept {
    return !f;
}

#if CIEL_STD_VER >= 17

template<class R, class... Args>
function(R (*)(Args...)) -> function<R(Args...)>;

template<class F>
function(F) -> function<strip_signature_t<decltype(&F::operator())>>;

#endif

NAMESPACE_CIEL_END

namespace std {

template<class R, class... Args>
void
swap(ciel::function<R(Args...)>& lhs, ciel::function<R(Args...)>& rhs) noexcept {
    lhs.swap(rhs);
}

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_FUNCTION_HPP_

#ifndef CIELLAB_INCLUDE_CIEL_OBSERVER_PTR_HPP_
#define CIELLAB_INCLUDE_CIEL_OBSERVER_PTR_HPP_

#include <ciel/compare.hpp>
#include <ciel/config.hpp>
#include <ciel/is_trivially_relocatable.hpp>

#include <functional>
#include <type_traits>
#include <utility>

NAMESPACE_CIEL_BEGIN

template<class W>
class observer_ptr {
    static_assert(!std::is_reference<W>::value, "W shall not be a reference type");

public:
    using element_type = W;

private:
    element_type* ptr_;

public:
    observer_ptr() noexcept
        : ptr_{nullptr} {}

    observer_ptr(nullptr_t) noexcept
        : ptr_{nullptr} {}

    explicit observer_ptr(element_type* p) noexcept
        : ptr_{p} {}

    template<class W2, enable_if_t<std::is_convertible<W*, element_type*>::value> = 0>
    observer_ptr(observer_ptr<W2> other) noexcept
        : ptr_{other.ptr_} {}

    CIEL_NODISCARD element_type*
    release() noexcept {
        element_type* res = ptr_;
        ptr_              = nullptr;
        return res;
    }

    void
    reset(element_type* p = nullptr) noexcept {
        ptr_ = p;
    }

    void
    swap(observer_ptr& other) noexcept {
        std::swap(ptr_, other.ptr_);
    }

    CIEL_NODISCARD element_type*
    get() const noexcept {
        return ptr_;
    }

    CIEL_NODISCARD explicit
    operator bool() const noexcept {
        return get() != nullptr;
    }

    CIEL_NODISCARD add_lvalue_reference_t<element_type>
    operator*() const noexcept {
        CIEL_PRECONDITION(*this);

        return *get();
    }

    CIEL_NODISCARD element_type*
    operator->() const noexcept {
        CIEL_PRECONDITION(*this);

        return get();
    }

    CIEL_NODISCARD explicit
    operator element_type*() const noexcept {
        return ptr_;
    }

}; // class observer_ptr

template<class T>
struct is_trivially_relocatable<observer_ptr<T>> : std::true_type {};

template<class W>
CIEL_NODISCARD observer_ptr<W>
make_observer(W* p) noexcept {
    return observer_ptr<W>{p};
}

template<class W1, class W2>
CIEL_NODISCARD bool
operator==(const observer_ptr<W1>& p1, const observer_ptr<W2>& p2) {
    return p1.get() == p2.get();
}

template<class W>
CIEL_NODISCARD bool
operator==(const observer_ptr<W>& p, nullptr_t) noexcept {
    return !p;
}

template<class W>
CIEL_NODISCARD bool
operator==(nullptr_t, const observer_ptr<W>& p) noexcept {
    return !p;
}

template<class W1, class W2>
CIEL_NODISCARD bool
operator<(const observer_ptr<W1>& p1, const observer_ptr<W2>& p2) {
    using W3 = common_type_t<W1, W2>;
    return std::less<W3>()(p1.get(), p2.get());
}

NAMESPACE_CIEL_END

namespace std {

template<class W>
void
swap(ciel::observer_ptr<W>& lhs, ciel::observer_ptr<W>& rhs) noexcept {
    lhs.swap(rhs);
}

template<class T>
struct hash<ciel::observer_ptr<T>> {
    CIEL_NODISCARD size_t
    operator()(ciel::observer_ptr<T> p) const noexcept {
        return hash<T*>()(p.get());
    }

}; // struct hash<ciel::observer_ptr<T>>

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_OBSERVER_PTR_HPP_

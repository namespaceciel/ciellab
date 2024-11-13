#ifndef CIELLAB_INCLUDE_CIEL_TEST_OPERATOR_HIJACKER_HPP_
#define CIELLAB_INCLUDE_CIEL_TEST_OPERATOR_HIJACKER_HPP_

#include <ciel/core/config.hpp>

#include <cstddef>
#include <functional>
#include <memory>

NAMESPACE_CIEL_BEGIN

struct operator_hijacker {
    CIEL_NODISCARD bool operator==(const operator_hijacker&) const {
        return true;
    }

    CIEL_NODISCARD bool operator<(const operator_hijacker&) const {
        return true;
    }

    template<class T>
    friend void operator&(T&&) = delete;
    template<class T, class U>
    friend void operator,(T&&, U&&) = delete;
    template<class T, class U>
    friend void operator&&(T&&, U&&) = delete;
    template<class T, class U>
    friend void operator||(T&&, U&&) = delete;

}; // struct operator_hijacker

template<class T>
struct operator_hijacker_allocator : std::allocator<T>,
                                     operator_hijacker {
#if CIEL_STD_VER <= 17
    struct rebind {
        using other = operator_hijacker_allocator<T>;
    };
#endif

}; // struct operator_hijacker_allocator

NAMESPACE_CIEL_END

namespace std {

template<>
struct hash<ciel::operator_hijacker> {
    CIEL_NODISCARD size_t operator()(ciel::operator_hijacker) const noexcept {
        return 0;
    }
};

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_TEST_OPERATOR_HIJACKER_HPP_

#ifndef CIELLAB_INCLUDE_CIEL_EXPERIMENTAL_MOVE_PROXY_HPP_
#define CIELLAB_INCLUDE_CIEL_EXPERIMENTAL_MOVE_PROXY_HPP_

#include <ciel/core/config.hpp>

#include <type_traits>
#include <utility>

NAMESPACE_CIEL_BEGIN

template<class T>
class move_proxy {
public:
    template<class... Args, enable_if_t<std::is_constructible<T, Args&&...>::value> = 0>
    move_proxy(Args&&... args) noexcept(std::is_nothrow_constructible<T, Args&&...>::value)
        : data_(std::forward<Args>(args)...) {}

    template<class U, class... Args,
             enable_if_t<std::is_constructible<T, std::initializer_list<U>, Args&&...>::value> = 0>
    move_proxy(std::initializer_list<U> il,
               Args&&... args) noexcept(std::is_nothrow_constructible<T, std::initializer_list<U>, Args&&...>::value)
        : data_(il, std::forward<Args>(args)...) {}

    operator T&&() const noexcept {
        return std::move(data_);
    }

private:
    mutable T data_;

}; // class move_proxy

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_EXPERIMENTAL_MOVE_PROXY_HPP_

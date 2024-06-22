#ifndef CIELLAB_INCLUDE_CIEL_MOVE_PROXY_HPP_
#define CIELLAB_INCLUDE_CIEL_MOVE_PROXY_HPP_

#include <type_traits>
#include <utility>

#include <ciel/config.hpp>

NAMESPACE_CIEL_BEGIN

template<class T>
class move_proxy {
public:
    template<class... Args, typename std::enable_if<std::is_constructible<T, Args&&...>::value, int>::type = 0>
    move_proxy(Args&&... args) noexcept(std::is_nothrow_constructible<T, Args&&...>::value)
        : data_(std::forward<Args>(args)...) {}

    operator T() const noexcept(std::is_nothrow_move_constructible<T>::value) {
        return std::move(data_);
    }

private:
    mutable T data_;

}; // class move_proxy

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_MOVE_PROXY_HPP_

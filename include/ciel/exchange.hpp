#ifndef CIELLAB_INCLUDE_CIEL_EXCHANGE_HPP_
#define CIELLAB_INCLUDE_CIEL_EXCHANGE_HPP_

#include <ciel/config.hpp>

#include <type_traits>
#include <utility>

NAMESPACE_CIEL_BEGIN

template<class T, class U = T>
CIEL_NODISCARD T exchange(T& obj, U&& new_value) noexcept(std::is_nothrow_move_constructible<T>::value
                                                          && std::is_nothrow_assignable<T&, U>::value) {
    T old_value = std::move(obj);
    obj         = std::forward<U>(new_value);
    return old_value;
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_EXCHANGE_HPP_

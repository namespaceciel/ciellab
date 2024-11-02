#ifndef CIELLAB_INCLUDE_CIEL_WORTH_MOVE_HPP_
#define CIELLAB_INCLUDE_CIEL_WORTH_MOVE_HPP_

#include <ciel/config.hpp>

#include <type_traits>

NAMESPACE_CIEL_BEGIN

// FIXME: Current implementation returns true for const&& constructor and assignment.
template<class T>
struct worth_move {
    static_assert(!std::is_const<T>::value, "");

private:
    using U = decay_t<T>;

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

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_WORTH_MOVE_HPP_

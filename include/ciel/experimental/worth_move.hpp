#ifndef CIELLAB_INCLUDE_CIEL_EXPERIMENTAL_WORTH_MOVE_HPP_
#define CIELLAB_INCLUDE_CIEL_EXPERIMENTAL_WORTH_MOVE_HPP_

#include <ciel/core/config.hpp>

#include <type_traits>

NAMESPACE_CIEL_BEGIN

// Inspired by:
// https://stackoverflow.com/questions/51901837/how-to-get-if-a-type-is-truly-move-constructible/51912859#51912859
//
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
    static constexpr bool construct = std::is_class<T>::value && !std::is_trivially_copyable<T>::value
                                   && std::is_move_constructible<T>::value && !std::is_constructible<T, helper>::value;
    static constexpr bool assign = std::is_class<T>::value && !std::is_trivially_copyable<T>::value
                                && std::is_move_assignable<T>::value && !std::is_assignable<T, helper>::value;
    static constexpr bool value = construct || assign;

}; // struct worth_move

template<class T>
struct worth_move_constructing {
    static constexpr bool value = worth_move<T>::construct;

}; // struct worth_move_constructing

template<class T>
struct worth_move_assigning {
    static constexpr bool value = worth_move<T>::assign;

}; // struct worth_move_assigning

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_EXPERIMENTAL_WORTH_MOVE_HPP_

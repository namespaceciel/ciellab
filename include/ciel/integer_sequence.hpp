#ifndef CIELLAB_INCLUDE_CIEL_INTEGER_SEQUENCE_HPP_
#define  CIELLAB_INCLUDE_CIEL_INTEGER_SEQUENCE_HPP_

#include <cstddef>

#include <ciel/config.hpp>

NAMESPACE_CIEL_BEGIN

template<class T, T... Ints>
struct integer_sequence {
    static_assert(std::is_integral<T>::value, "");

    using value_type = T;

    static constexpr auto size() noexcept -> size_t {
        return sizeof...(Ints);
    }

};  // struct integer_sequence

template<size_t... Ints>
using index_sequence = integer_sequence<size_t, Ints...>;

namespace details {

// https://stackoverflow.com/questions/17424477/implementation-c14-make-integer-sequence

template<class, class>
struct merge_and_renumber;

template<size_t... I1, size_t... I2>
struct merge_and_renumber<index_sequence<I1...>, index_sequence<I2...>> {
    using type = index_sequence<I1..., (sizeof...(I1) + I2)...>;
};

template<size_t N>
struct make_index_sequence_helper : merge_and_renumber<typename make_index_sequence_helper<N / 2>::type,
        typename make_index_sequence_helper<N - N / 2>::type> {};

template<>
struct make_index_sequence_helper<0> {
    using type = index_sequence<>;
};

template<>
struct make_index_sequence_helper<1> {
    using type = index_sequence<0>;
};

}   // namespace details

template<size_t N>
using make_index_sequence = typename details::make_index_sequence_helper<N>::type;

namespace details {

template<typename T, size_t N, typename Indices = make_index_sequence<N>>
struct make_integer_sequence_helper;

template<typename T, size_t N, size_t... Indices>
struct make_integer_sequence_helper<T, N, index_sequence<Indices...>> {
    using type = integer_sequence<T, static_cast<T>(Indices)...>;
};

}   // namespace details

template<class T, T N>
using make_integer_sequence = typename details::make_integer_sequence_helper<T, N>::type;

template<class... T>
using index_sequence_for = make_index_sequence<sizeof...(T)>;

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_INTEGER_SEQUENCE_HPP_
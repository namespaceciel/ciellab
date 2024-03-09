#ifndef CIELLAB_INCLUDE_CIEL_COMPRESSED_PAIR_HPP_
#define CIELLAB_INCLUDE_CIEL_COMPRESSED_PAIR_HPP_

#include <tuple>
#include <type_traits>

#include <ciel/config.hpp>
#include <ciel/integer_sequence.hpp>
#include <ciel/type_traits.hpp>

NAMESPACE_CIEL_BEGIN

struct default_init_tag {};
struct value_init_tag {};

template<class T, size_t Index, bool = std::is_empty<T>::value && !is_final<T>::value>
struct compressed_pair_elem {
public:
    using reference       = T&;
    using const_reference = const T&;

private:
    T value_;

public:
    explicit compressed_pair_elem(default_init_tag) {}

    explicit compressed_pair_elem(value_init_tag) : value_() {}

    template <class U, typename std::enable_if<!std::is_same<compressed_pair_elem,
                                                             typename std::decay<U>::type>::value, int>::type = 0>
    explicit compressed_pair_elem(U&& u) : value_(std::forward<U>(u)) {}

    template<class... Args, size_t... Ints>
    explicit compressed_pair_elem(std::piecewise_construct_t, std::tuple<Args...> args, index_sequence<Ints...>)
        : value_(std::forward<Args>(std::get<Ints>(args))...) {}

    reference get() noexcept {
        return value_;
    }

    const_reference get() const noexcept {
        return value_;
    }

};  // struct compressed_pair_elem

template<class T, size_t Index>
struct compressed_pair_elem<T, Index, true> : private T {
public:
    using reference       = T&;
    using const_reference = const T&;
    using value_          = T;

public:
    explicit compressed_pair_elem(default_init_tag) {}

    explicit compressed_pair_elem(value_init_tag) : value_() {}

    template <class U, typename std::enable_if<!std::is_same<compressed_pair_elem,
                                                             typename std::decay<U>::type>::value, int>::type = 0>
    explicit compressed_pair_elem(U&& u) : value_(std::forward<U>(u)) {}

    template<class... Args, size_t... Ints>
    explicit compressed_pair_elem(std::piecewise_construct_t, std::tuple<Args...> args, index_sequence<Ints...>)
        : value_(std::forward<Args>(std::get<Ints>(args))...) {}

    reference get() noexcept {
        return *this;
    }

    const_reference get() const noexcept {
        return *this;
    }

};  // struct compressed_pair_elem<T, Index, true>

template<class T1, class T2>
class compressed_pair : private compressed_pair_elem<T1, 0>,
                        private compressed_pair_elem<T2, 1> {

    static_assert(!std::is_same<T1, T2>::value, "");

    using base1 = compressed_pair_elem<T1, 0>;
    using base2 = compressed_pair_elem<T2, 1>;

public:
    template<class U1 = T1, class U2 = T2,
            typename std::enable_if<std::is_default_constructible<U1>::value &&
                                    std::is_default_constructible<U2>::value, int>::type = 0>
    explicit compressed_pair() : base1(value_init_tag{}), base2(value_init_tag{}) {}

    template <class U1, class U2>
    explicit compressed_pair(U1&& u1, U2&& u2) : base1(std::forward<U1>(u1)), base2(std::forward<U2>(u2)) {}

    template <class... Args1, class... Args2>
    explicit compressed_pair(std::piecewise_construct_t pc, std::tuple<Args1...> first_args,
                                                            std::tuple<Args2...> second_args)
        : base1(pc, std::move(first_args), index_sequence_for<Args1...>()),
          base2(pc, std::move(second_args), index_sequence_for<Args2...>()) {}

    auto first() noexcept -> typename base1::reference {
        return static_cast<base1&>(*this).get();
    }

    auto first() const noexcept -> typename base1::const_reference {
        return static_cast<const base1&>(*this).get();
    }

    auto second() noexcept -> typename base2::reference {
        return static_cast<base2&>(*this).get();
    }

    auto second() const noexcept -> typename base2::const_reference {
        return static_cast<const base2&>(*this).get();
    }

    // TODO: Since std::is_nothrow_swappable is available in C++17...
    auto swap(compressed_pair& other) noexcept -> void {
        using std::swap;

        swap(first(), other.first());
        swap(second(), other.second());
    }

};  // class compressed_pair

NAMESPACE_CIEL_END

namespace std {

template<class T1, class T2>
auto swap(ciel::compressed_pair<T1, T2>& lhs,
          ciel::compressed_pair<T1, T2>& rhs) noexcept(noexcept(lhs.swap(rhs))) -> void {
    lhs.swap(rhs);
}

}   // namespace std

#endif // CIELLAB_INCLUDE_CIEL_COMPRESSED_PAIR_HPP_
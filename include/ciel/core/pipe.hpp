#ifndef CIELLAB_INCLUDE_CIEL_CORE_PIPE_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_PIPE_HPP_

#include <ciel/core/config.hpp>

NAMESPACE_CIEL_BEGIN

// Inspired by Microsoft snmalloc's implementation.

namespace detail {

template<class...>
struct pipe_impl;

template<class Input>
struct pipe_impl<Input> {
    using type = Input;
};

template<class Input, class Transformer, class... RestTransformer>
struct pipe_impl<Input, Transformer, RestTransformer...> {
    using Transformed = typename Transformer::template type<Input>;

    using type = typename pipe_impl<Transformed, RestTransformer...>::type;
};

} // namespace detail

template<class... Args>
using pipe = typename detail::pipe_impl<Args...>::type;

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_PIPE_HPP_

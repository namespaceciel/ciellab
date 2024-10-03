#ifndef CIELLAB_INCLUDE_CIEL_COPY_N_HPP_
#define CIELLAB_INCLUDE_CIEL_COPY_N_HPP_

#include <ciel/config.hpp>

NAMESPACE_CIEL_BEGIN

// To extract the input iterator `first`
template<class InputIt, class Size, class OutputIt>
InputIt
copy_n(InputIt first, Size count, OutputIt result) {
    for (Size i = 0; i < count; ++i) {
        *result = *first;
        ++result;
        ++first;
    }

    return first;
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_COPY_N_HPP_

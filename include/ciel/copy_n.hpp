#ifndef CIELLAB_INCLUDE_CIEL_COPY_N_HPP_
#define CIELLAB_INCLUDE_CIEL_COPY_N_HPP_

#include <ciel/allocator_traits.hpp>
#include <ciel/config.hpp>
#include <ciel/cstring.hpp>
#include <ciel/iterator_category.hpp>
#include <ciel/to_address.hpp>

#include <iterator>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

// To extract the input iterator `first`

template<class InputIt, class Size, class OutputIt>
InputIt
copy_n(InputIt first, Size count, OutputIt result) {
    using T = typename std::iterator_traits<InputIt>::value_type;
    using U = typename std::iterator_traits<OutputIt>::value_type;

    if (ciel::is_contiguous_iterator<InputIt>::value && ciel::is_contiguous_iterator<OutputIt>::value
        && std::is_same<T, U>::value && std::is_trivially_copy_assignable<T>::value) {
        if (count != 0) {
            // assume no overlap
            ciel::memcpy(ciel::to_address(result), ciel::to_address(first), count * sizeof(T));
        }

        return std::next(first, count);

    } else {
        for (Size i = 0; i < count; ++i) {
            *result = *first;
            ++result;
            ++first;
        }

        return first;
    }
}

template<class Alloc, class InputIt, class Size, class OutputIt>
InputIt
uninitialized_copy_n(Alloc& alloc, InputIt first, Size count, OutputIt result) {
    using T = typename std::iterator_traits<InputIt>::value_type;
    using U = typename std::iterator_traits<OutputIt>::value_type;

    if (ciel::is_contiguous_iterator<InputIt>::value && ciel::is_contiguous_iterator<OutputIt>::value
        && std::is_same<T, U>::value && std::is_trivially_copy_constructible<T>::value) {
        if (count != 0) {
            ciel::memcpy(ciel::to_address(result), ciel::to_address(first), count * sizeof(T));
        }

        return std::next(first, count);

    } else {
        for (Size i = 0; i < count; ++i) {
            if (ciel::allocator_has_trivial_construct<Alloc, T*, decltype(*first)>::value) {
                new (ciel::to_address(result)) T(*first);

            } else {
                using alloc_traits = std::allocator_traits<Alloc>;

                alloc_traits::construct(alloc, ciel::to_address(result), *first);
            }

            ++result;
            ++first;
        }

        return first;
    }
}

template<class Alloc, class InputIt, class OutputIt>
void
uninitialized_copy(Alloc& alloc, InputIt first, InputIt last, OutputIt& result) {
    using T = typename std::iterator_traits<InputIt>::value_type;
    using U = typename std::iterator_traits<OutputIt>::value_type;

    if (ciel::is_contiguous_iterator<InputIt>::value && ciel::is_contiguous_iterator<OutputIt>::value
        && std::is_same<T, U>::value && std::is_trivially_copy_constructible<T>::value) {
        const size_t count = std::distance(first, last);
        if (count != 0) {
            ciel::memcpy(ciel::to_address(result), ciel::to_address(first), count * sizeof(T));
            result += count;
        }

    } else {
        for (; first != last; ++first, ++result) {
            if (ciel::allocator_has_trivial_construct<Alloc, T*, decltype(*first)>::value) {
                new (ciel::to_address(result)) T(*first);

            } else {
                using alloc_traits = std::allocator_traits<Alloc>;

                alloc_traits::construct(alloc, ciel::to_address(result), *first);
            }
        }
    }
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_COPY_N_HPP_

#ifndef CIELLAB_INCLUDE_CIEL_COMPARE_HPP_
#define CIELLAB_INCLUDE_CIEL_COMPARE_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/is_range.hpp>

#include <algorithm>

NAMESPACE_CIEL_BEGIN

template<class T, class U, enable_if_t<is_range_with_size<T>::value && is_range_with_size<U>::value> = 0>
CIEL_NODISCARD bool operator==(const T& lhs, const U& rhs) noexcept {
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template<class T, class U, enable_if_t<is_range<T>::value && is_range<U>::value> = 0>
CIEL_NODISCARD bool operator<(const T& lhs, const U& rhs) noexcept {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template<class T, class U>
CIEL_NODISCARD bool operator!=(const T& lhs, const U& rhs) noexcept {
    return !(lhs == rhs);
}

template<class T, class U>
CIEL_NODISCARD bool operator>(const T& lhs, const U& rhs) noexcept {
    return rhs < lhs;
}

template<class T, class U>
CIEL_NODISCARD bool operator<=(const T& lhs, const U& rhs) noexcept {
    return !(rhs < lhs);
}

template<class T, class U>
CIEL_NODISCARD bool operator>=(const T& lhs, const U& rhs) noexcept {
    return !(lhs < rhs);
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_COMPARE_HPP_

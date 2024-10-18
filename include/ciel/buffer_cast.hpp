#ifndef CIELLAB_INCLUDE_CIEL_BUFFER_CAST_HPP_
#define CIELLAB_INCLUDE_CIEL_BUFFER_CAST_HPP_

#include <ciel/config.hpp>

#include <type_traits>

NAMESPACE_CIEL_BEGIN

template<class Pointer, typename std::enable_if<std::is_pointer<Pointer>::value, int>::type = 0>
CIEL_NODISCARD Pointer
buffer_cast(const void* ptr) noexcept {
    return static_cast<Pointer>(const_cast<void*>(ptr));
}

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_BUFFER_CAST_HPP_

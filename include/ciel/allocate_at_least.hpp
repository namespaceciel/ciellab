#ifndef CIELLAB_INCLUDE_CIEL_ALLOCATE_AT_LEAST_HPP_
#define CIELLAB_INCLUDE_CIEL_ALLOCATE_AT_LEAST_HPP_

#include <ciel/core/config.hpp>

#include <cstddef>
#include <memory>

NAMESPACE_CIEL_BEGIN

#ifdef __cpp_lib_allocate_at_least

template<class Allocator, class SizeType = typename std::allocator_traits<Allocator>::size_type>
CIEL_NODISCARD auto allocate_at_least(Allocator& allocator, const SizeType size) {
    return std::allocator_traits<Allocator>::allocate_at_least(allocator, size);
}

#else

template<class Pointer, class SizeType = size_t>
struct allocation_result {
    Pointer ptr;
    SizeType count;

}; // struct allocation_result

template<class Allocator, class Pointer = typename std::allocator_traits<Allocator>::pointer,
         class SizeType = typename std::allocator_traits<Allocator>::size_type>
CIEL_NODISCARD allocation_result<Pointer, SizeType> allocate_at_least(Allocator& allocator, const SizeType size) {
    return {std::allocator_traits<Allocator>::allocate(allocator, size), size};
}

#endif

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_ALLOCATE_AT_LEAST_HPP_

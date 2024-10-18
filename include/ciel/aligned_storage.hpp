#ifndef CIELLAB_INCLUDE_CIEL_ALIGNED_STORAGE_HPP_
#define CIELLAB_INCLUDE_CIEL_ALIGNED_STORAGE_HPP_

#include <ciel/config.hpp>

NAMESPACE_CIEL_BEGIN

template<size_t size, size_t alignment>
struct aligned_storage {
    union type {
        alignas(alignment) unsigned char buffer_[(size + alignment - 1) / alignment * alignment];
        unsigned char do_not_use_this_;
    };

}; // aligned_storage

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_ALIGNED_STORAGE_HPP_

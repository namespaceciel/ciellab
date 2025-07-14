#include <gtest/gtest.h>

#include <ciel/core/pipe.hpp>

#include <type_traits>

using namespace ciel;

namespace {

struct TransformToConst {
    template<class T>
    using type = const T;
};

struct TransformToPtr {
    template<class T>
    using type = T*;
};

} // namespace

TEST(pipe, all) {
    using T = ciel::pipe<int, TransformToPtr, TransformToConst, TransformToPtr>;

    static_assert(std::is_same<T, int* const*>::value, "");
}

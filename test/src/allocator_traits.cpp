#include <gtest/gtest.h>

#include <ciel/allocator_traits.hpp>

#include <memory>

using namespace ciel;

namespace {

template<class T>
struct EmptyAllocator {
    using value_type = T;
};

template<class T>
struct Allocator {
    using value_type = T;

    template<class U, class... Args>
    void construct(U*, Args&&...) const noexcept {}

    template<class U>
    void destroy(U*) const noexcept {}
};

} // namespace

TEST(allocator_traits, trivial) {
    static_assert(allocator_has_trivial_construct<EmptyAllocator<int>, int*>::value, "");
    static_assert(allocator_has_trivial_construct<EmptyAllocator<int>, int*, const int&>::value, "");
    static_assert(allocator_has_trivial_construct<EmptyAllocator<int>, int*, int&&>::value, "");
    static_assert(allocator_has_trivial_destroy<EmptyAllocator<int>, int*>::value, "");
}

TEST(allocator_traits, not_trivial) {
    static_assert(not allocator_has_trivial_construct<Allocator<int>, int*>::value, "");
    static_assert(not allocator_has_trivial_construct<Allocator<int>, int*, const int&>::value, "");
    static_assert(not allocator_has_trivial_construct<Allocator<int>, int*, int&&>::value, "");
    static_assert(not allocator_has_trivial_destroy<Allocator<int>, int*>::value, "");
}

TEST(allocator_traits, std) {
    static_assert(allocator_has_trivial_construct<std::allocator<int>, int*>::value, "");
    static_assert(allocator_has_trivial_construct<std::allocator<int>, int*, const int&>::value, "");
    static_assert(allocator_has_trivial_construct<std::allocator<int>, int*, int&&>::value, "");
    static_assert(allocator_has_trivial_destroy<std::allocator<int>, int*>::value, "");
}

#include <gtest/gtest.h>

#include <ciel/allocator_traits.hpp>

namespace {

template<class T>
struct EmptyAllocator {
    using value_type = T;
};

template<class T>
struct Allocator {
    using value_type = T;

    template<class... Args>
    void
    construct(T*, Args&&...) const noexcept {}

    void
    destroy(T*) const noexcept {}
};

} // namespace

TEST(allocator_traits_tests, trivial) {
    static_assert(ciel::allocator_has_trivial_default_construct<EmptyAllocator<int>>::value, "");
    static_assert(ciel::allocator_has_trivial_copy_construct<EmptyAllocator<int>>::value, "");
    static_assert(ciel::allocator_has_trivial_move_construct<EmptyAllocator<int>>::value, "");
    static_assert(ciel::allocator_has_trivial_destroy<EmptyAllocator<int>>::value, "");
}

TEST(allocator_traits_tests, not_trivial) {
    static_assert(not ciel::allocator_has_trivial_default_construct<Allocator<int>>::value, "");
    static_assert(not ciel::allocator_has_trivial_copy_construct<Allocator<int>>::value, "");
    static_assert(not ciel::allocator_has_trivial_move_construct<Allocator<int>>::value, "");
    static_assert(not ciel::allocator_has_trivial_destroy<Allocator<int>>::value, "");
}

TEST(allocator_traits_tests, std) {
    static_assert(ciel::allocator_has_trivial_default_construct<std::allocator<int>>::value, "");
    static_assert(ciel::allocator_has_trivial_copy_construct<std::allocator<int>>::value, "");
    static_assert(ciel::allocator_has_trivial_move_construct<std::allocator<int>>::value, "");
    static_assert(ciel::allocator_has_trivial_destroy<std::allocator<int>>::value, "");
}

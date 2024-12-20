#include <gtest/gtest.h>

#include <ciel/core/can_be_destroyed_from_base.hpp>

using namespace ciel;

namespace {

struct B1 {};

struct D1 : B1 {};

struct B2 {
    ~B2() {}
};

struct D2 : B2 {};

struct B3 {};

struct D3 : B3 {
    ~D3() {}
};

struct B4 {
    virtual ~B4() = default;
};

struct D4 : B4 {};

struct B5 {
    virtual ~B5() = default;
};

struct D5 : B5 {
    ~D5() override {}
};

struct B6 {
    virtual ~B6() = default;
};

struct D6 : private B6 {};

} // namespace

TEST(can_be_destroyed_from_base, similar) {
    static_assert(is_similar<int, int>::value, "");
    static_assert(is_similar<int*, int*>::value, "");
    static_assert(is_similar<int[], int[]>::value, "");
    static_assert(is_similar<int[9], int[]>::value, "");
    static_assert(is_similar<int[], int[9]>::value, "");
    static_assert(is_similar<int[9], int[9]>::value, "");

    static_assert(is_similar<const int, int>::value, "");
    static_assert(is_similar<const int*, int*>::value, "");
    static_assert(is_similar<int* const, int*>::value, "");
    static_assert(is_similar<const int[], int[]>::value, "");
    static_assert(is_similar<const int[9], int[]>::value, "");
    static_assert(is_similar<const int[], int[9]>::value, "");
    static_assert(is_similar<const int[9], int[9]>::value, "");

    static_assert(is_similar<int, const int>::value, "");
    static_assert(is_similar<int*, const int*>::value, "");
    static_assert(is_similar<int*, int* const>::value, "");
    static_assert(is_similar<int[], const int[]>::value, "");
    static_assert(is_similar<int[9], const int[]>::value, "");
    static_assert(is_similar<int[], const int[9]>::value, "");
    static_assert(is_similar<int[9], const int[9]>::value, "");

    static_assert(not is_similar<int&, int>::value, "");
    static_assert(not is_similar<int, int&>::value, "");
    static_assert(not is_similar<int&&, int>::value, "");
    static_assert(not is_similar<int, int&&>::value, "");
    static_assert(not is_similar<int&&, int&>::value, "");
    static_assert(not is_similar<int&, int&&>::value, "");

    static_assert(is_similar<int B1::*, int B1::*>::value, "");
    static_assert(is_similar<int& (B1::*)(), int& (B1::*)()>::value, "");
    static_assert(is_similar<int& (B1::*)(int*), int& (B1::*)(int*)>::value, "");

    static_assert(is_similar<int B1::*, int B1::* const>::value, "");
    static_assert(is_similar<int& (B1::*)(), int& (B1::* const)()>::value, "");
    static_assert(is_similar<int& (B1::*)(int*), int& (B1::* const)(int*)>::value, "");

    static_assert(not is_similar<int B1::*, char B1::*>::value, "");
    static_assert(not is_similar<int& (B1::*)(), const int& (B1::*)()>::value, "");
    static_assert(not is_similar<int& (B1::*)(), int (B1::*)()>::value, "");
    static_assert(not is_similar<int& (B1::*)(), int (B1::*)()>::value, "");
    static_assert(not is_similar<int (B1::*)(char), int (B1::*)(int)>::value, "");
    static_assert(not is_similar<int& (B1::*)(int*), int& (B1::*)(const int*)>::value, "");
}

TEST(can_be_destroyed_from_base, can_be_destroyed_from_base) {
    static_assert(can_be_destroyed_from_base<B1, B1>::value, "");
    static_assert(can_be_destroyed_from_base<B2, B2>::value, "");
    static_assert(can_be_destroyed_from_base<B3, B3>::value, "");
    static_assert(can_be_destroyed_from_base<B4, B4>::value, "");
    static_assert(can_be_destroyed_from_base<B5, B5>::value, "");
    static_assert(can_be_destroyed_from_base<B6, B6>::value, "");

    static_assert(not can_be_destroyed_from_base<B1, D1>::value, "");
    static_assert(not can_be_destroyed_from_base<B2, D2>::value, "");
    static_assert(not can_be_destroyed_from_base<B3, D3>::value, "");
    static_assert(can_be_destroyed_from_base<B4, D4>::value, "");
    static_assert(can_be_destroyed_from_base<B5, D5>::value, "");
    static_assert(can_be_destroyed_from_base<B6, D6>::value, "");
}

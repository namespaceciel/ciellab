#include <gtest/gtest.h>

#include <ciel/can_be_destroyed_from_base.hpp>

using namespace ciel;

namespace {

struct B1 {};

struct D1 : B1 {};

struct B2 {
    ~B2() {} // NOLINT(modernize-use-equals-default)
};

struct D2 : B2 {};

struct B3 {};

struct D3 : B3 {
    ~D3() {} // NOLINT(modernize-use-equals-default)
};

struct B4 {
    virtual ~B4() = default;
};

struct D4 : B4 {};

struct B5 {
    virtual ~B5() = default;
};

struct D5 : B5 {
    ~D5() override {} // NOLINT(modernize-use-equals-default)
};

} // namespace

TEST(can_be_destroyed_from_base, all) {
    static_assert(can_be_destroyed_from_base<B1, D1>::value, "");
    static_assert(not can_be_destroyed_from_base<B2, D2>::value, "");
    static_assert(not can_be_destroyed_from_base<B3, D3>::value, "");
    static_assert(can_be_destroyed_from_base<B4, D4>::value, "");
    static_assert(can_be_destroyed_from_base<B5, D5>::value, "");
}

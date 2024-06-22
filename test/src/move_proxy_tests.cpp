#include "tools.h"
#include <gtest/gtest.h>

#include <ciel/move_proxy.hpp>

TEST(move_proxy_tests, all) {
    ConstructAndAssignCounter::reset();

    MoveProxyTestClass a;

    a = {{}};

    ASSERT_EQ(ConstructAndAssignCounter::move(), 1);

    std::initializer_list<ConstructAndAssignCounter> il{{}};
    a = il;
    a = std::move(il);

    const std::initializer_list<ConstructAndAssignCounter> il2{{}};
    a = il2;
    a = std::move(il2);

    ASSERT_EQ(ConstructAndAssignCounter::copy(), 4);
}

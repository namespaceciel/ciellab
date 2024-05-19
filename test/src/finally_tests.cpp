#include <gtest/gtest.h>

#include <ciel/finally.hpp>

TEST(finally_tests, defer) {
    bool deferCalled = false;
    { CIEL_DEFER(deferCalled = true); }
    ASSERT_TRUE(deferCalled);
}

TEST(finally_tests, defer_order) {
    int counter = 0;
    int a = 0, b = 0, c = 0;
    {
        CIEL_DEFER(a = ++counter);
        CIEL_DEFER(b = ++counter);
        CIEL_DEFER(c = ++counter);
    }
    ASSERT_EQ(a, 3);
    ASSERT_EQ(b, 2);
    ASSERT_EQ(c, 1);
}

#include <gtest/gtest.h>

#include <ciel/compressed_pair.hpp>

TEST(compressed_pair_tests, constructor) {
    using IntPair = ciel::compressed_pair<int, int>;

    IntPair p1;
    ASSERT_EQ(p1.first(), 0);
    ASSERT_EQ(p1.second(), 0);

    p1.first()  = 1;
    p1.second() = 2;
    ::new (&p1) IntPair;
    ASSERT_EQ(p1.first(), 0);
    ASSERT_EQ(p1.second(), 0);
}

TEST(compressed_pair_tests, default_init) {
    using IntPair = ciel::compressed_pair<int, int>;

    IntPair p1;
    p1.first()  = 1;
    p1.second() = 2;

    ::new (&p1) IntPair(ciel::default_init, 3);
    ASSERT_EQ(p1.first(), 1);
    ASSERT_EQ(p1.second(), 3);

    ::new (&p1) IntPair(4, ciel::default_init);
    ASSERT_EQ(p1.first(), 4);
    ASSERT_EQ(p1.second(), 3);

    ::new (&p1) IntPair(ciel::default_init, ciel::default_init);
    ASSERT_EQ(p1.first(), 4);
    ASSERT_EQ(p1.second(), 3);
}

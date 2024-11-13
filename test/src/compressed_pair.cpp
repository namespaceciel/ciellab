#include <gtest/gtest.h>

#include <ciel/core/compressed_pair.hpp>

using namespace ciel;

namespace {

struct Empty {};

} // namespace

TEST(compressed_pair, constructor) {
    using IntPair = compressed_pair<int, int>;

    IntPair p1;
    ASSERT_EQ(p1.first(), 0);
    ASSERT_EQ(p1.second(), 0);

    p1.first()  = 1;
    p1.second() = 2;
    ::new (&p1) IntPair;
    ASSERT_EQ(p1.first(), 0);
    ASSERT_EQ(p1.second(), 0);
}

TEST(compressed_pair, default_init) {
    using IntPair = compressed_pair<int, int>;

    IntPair p1;
    p1.first()  = 1;
    p1.second() = 2;

    ::new (&p1) IntPair(default_init, 3);
    ASSERT_EQ(p1.first(), 1);
    ASSERT_EQ(p1.second(), 3);

    ::new (&p1) IntPair(4, default_init);
    ASSERT_EQ(p1.first(), 4);
    ASSERT_EQ(p1.second(), 3);

    ::new (&p1) IntPair(default_init, default_init);
    ASSERT_EQ(p1.first(), 4);
    ASSERT_EQ(p1.second(), 3);
}

TEST(compressed_pair, both_same_empty_bases) {
    static_assert(sizeof(compressed_pair<Empty, Empty>) == 2, "");
}

#ifdef CIEL_HAS_EXCEPTIONS
#  include <ciel/core/config.hpp>
#  include <ciel/test/exception_generator.hpp>

TEST(compressed_pair, exception_safety) {
    using EG =
        ExceptionGenerator<2, DefaultConstructor | CopyConstructor | MoveConstructor | CopyAssignment | MoveAssignment,
                           false>;
    EG::reset();
    EG::enabled = true;

    CIEL_TRY {
        const compressed_pair<EG, EG> p(1, 2);

        ASSERT_TRUE(false);
    }
    CIEL_CATCH (...) {}
}
#endif

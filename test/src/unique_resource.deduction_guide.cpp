#include <gtest/gtest.h>

#include <ciel/unique_resource.hpp>

using namespace ciel;

namespace {

int global_int = 0;

void
increment_global_int() noexcept {
    ++global_int;
}

} // namespace

#if CIEL_STD_VER >= 17
TEST(unique_resource, deduction_guide) {
    int i  = 0;
    auto f = [&i] {
        ++i;
    };
    {
        unique_resource ur(increment_global_int, invoker{});

        ur.get()();
        ASSERT_EQ(i, 1);
    }
    ASSERT_EQ(i, 2);
}
#endif

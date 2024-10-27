#include <gtest/gtest.h>

#include <ciel/test/int_wrapper.hpp>
#include <ciel/unique_resource.hpp>

using namespace ciel;

namespace {

int global_int = 0;

void
increment_global_int() noexcept {
    ++global_int;
}

} // namespace

TEST(unique_resource, function) {
    int i  = 0;
    auto f = [&i] {
        ++i;
    };
    {
        unique_resource<void (*)(), invoker<void (*)()>> ur(increment_global_int, invoker<void (*)()>{});

        ur.get()();
        ASSERT_EQ(i, 1);
    }
    ASSERT_EQ(i, 2);
}

TEST(unique_resource, lambda) {
    int i  = 0;
    auto f = [&i] {
        ++i;
    };
    using type = decltype(f);
    {
        unique_resource<type, invoker<type>> ur(f, invoker<type>{});

        ur.get()();
        ASSERT_EQ(i, 1);
    }
    ASSERT_EQ(i, 2);
}

TEST(unique_resource, array) {
    Int arr[5]{0, 1, 2, 3, 4};

    unique_resource<Int(&)[5], no_op> ur(arr, no_op{});
    arr[0] = 123;
    ASSERT_EQ(ur.get()[0], 123);

    unique_resource<Int[5], no_op> ur2(arr, no_op{});
    arr[1] = 123;
    ASSERT_EQ(ur.get()[1], 1);
}

TEST(unique_resource, trivially_destructible) {
    unique_resource<int, destroyer<int>> ur(1, destroyer<int>{});

    static_assert(std::is_trivially_destructible<decltype(ur)>::value, "");
}

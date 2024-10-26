#include <gtest/gtest.h>

#include <ciel/function.hpp>

#include <deque>

using namespace ciel;

namespace {

void
test1() noexcept {}

int
test2(double, float, long) noexcept {
    return 1;
}

} // namespace

TEST(function, constructors_and_assignments) {
    const function<void()> f0{nullptr};

#ifdef CIEL_HAS_EXCEPTIONS
    ASSERT_THROW(f0(), std::bad_function_call);
#endif

    const function<void()> f1{test1};
    f1();

    const function<int(double, float, long)> f2{test2};
    static_assert(std::is_same<decltype(f2(1.0, 1.f, 1)), int>::value, "");
    CIEL_UNUSED(f2(1.0, 1.f, 1));

    function<void()> f3{[] {}};
    f3();

    int i = 1;
    function<int()> f4{[&] {
        return i;
    }};
    static_assert(std::is_same<decltype(f4()), int>::value, "");
    CIEL_UNUSED(f4());

    const function<int()> f5{f4};
    CIEL_UNUSED(f5());

    function<int()> f6{std::move(f4)};
    ASSERT_FALSE(f4);
    CIEL_UNUSED(f6());

    f4 = nullptr;

    f4 = [] {
        return 1;
    };
    CIEL_UNUSED(f4());

    f4 = f5;
    CIEL_UNUSED(f4());

    f4 = std::move(f6);
    ASSERT_FALSE(f6);
    CIEL_UNUSED(f4());

    static_assert(not is_small_object<std::deque<int>>::value, "");
    std::deque<int> deque{1, 2, 3, 4, 5};
    function<void()> f7{[deque] {
        CIEL_UNUSED(deque);
    }};
    f7();

    f3 = f7;
    f3();

    f3 = f0;
#ifdef CIEL_HAS_EXCEPTIONS
    ASSERT_THROW(f3(), std::bad_function_call);
#endif

    f3 = f1;
    f3();

    f7 = std::move(f3);
    ASSERT_FALSE(f3);
    f7();
}

TEST(function, swap) {
    const std::deque<int> d{1, 2, 3, 4, 5};
    const std::vector<int> v{6, 7, 8, 9, 10};
    auto large_lambda = [d] {
        return std::vector<int>{d.begin(), d.end()};
    };
    auto small_lambda = [v] {
        return v;
    };

    function<std::vector<int>()> large_function{large_lambda};
    function<std::vector<int>()> small_function{assume_trivially_relocatable, small_lambda};

    ASSERT_EQ(large_function(), std::vector<int>({1, 2, 3, 4, 5}));
    ASSERT_EQ(small_function(), std::vector<int>({6, 7, 8, 9, 10}));

    std::swap(large_function, small_function);

    ASSERT_EQ(large_function(), std::vector<int>({6, 7, 8, 9, 10}));
    ASSERT_EQ(small_function(), std::vector<int>({1, 2, 3, 4, 5}));
}

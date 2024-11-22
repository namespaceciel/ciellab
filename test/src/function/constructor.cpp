#include <gtest/gtest.h>

#include <ciel/core/message.hpp>
#include <ciel/function.hpp>
#include <ciel/vector.hpp>

#include <list>
#include <utility>

using namespace ciel;

namespace {

struct TriviallyRelocatable {
    vector<int> v{42};

    void operator()() const noexcept {
        CIEL_POSTCONDITION(v.size() == 1);
        CIEL_POSTCONDITION(v[0] == 42);
    }
};

struct NonTriviallyRelocatable {
    std::list<int> list{42};

    void operator()() const noexcept {
        auto it = list.begin();
        ++it;

        CIEL_POSTCONDITION(it == list.end());
    }
};

} // namespace

TEST(function, copy_stack) {
    function<void()> f1(assume_trivially_relocatable, TriviallyRelocatable{});
    f1();

    const function<void()> f2(f1);

    f1();
    f2();
}

TEST(function, copy_heap) {
    function<void()> f1(NonTriviallyRelocatable{});
    f1();

    const function<void()> f2(f1);

    f1();
    f2();
}

TEST(function, move_stack) {
    function<void()> f1(assume_trivially_relocatable, TriviallyRelocatable{});
    f1();

    const function<void()> f2(std::move(f1));
    ASSERT_TRUE(!f1); // NOLINT(bugprone-use-after-move)

    f2();
}

TEST(function, move_heap) {
    function<void()> f1(NonTriviallyRelocatable{});
    f1();

    const function<void()> f2(std::move(f1));
    ASSERT_TRUE(!f1); // NOLINT(bugprone-use-after-move)

    f2();
}

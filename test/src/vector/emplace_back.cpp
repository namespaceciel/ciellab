#include <gtest/gtest.h>

#include <ciel/test/fancy_allocator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

namespace {

template<class C>
void
test_emplace_back_impl(::testing::Test*) {
    constexpr int N = 64;

    C v;

    for (int i = 0; i < N; ++i) {
        v.emplace_back(i);
    }

    ASSERT_EQ(v.size(), N);
    for (int i = 0; i < N; ++i) {
        ASSERT_EQ(v[i], Int(i));
    }
}

template<class C>
void
test_emplace_back_self_reference_impl(::testing::Test*) {
    C v{0, 1, 2, 3, 4};

    v.resize(v.capacity(), 123);
    v.emplace_back(v[0]);
    ASSERT_EQ(v.back(), v[0]);

    v.resize(v.capacity(), 234);
    v.emplace_back(v[1]);
    ASSERT_EQ(v.back(), v[1]);
}

} // namespace

TEST(vector, emplace_back) {
    test_emplace_back_impl<vector<Int>>(this);
    test_emplace_back_impl<vector<TRInt>>(this);
    test_emplace_back_impl<vector<TMInt>>(this);
    test_emplace_back_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_emplace_back_impl<vector<TRInt, fancy_allocator<TRInt>>>(this);
    test_emplace_back_impl<vector<TMInt, fancy_allocator<TMInt>>>(this);
}

TEST(vector, emplace_back_self_reference) {
    test_emplace_back_self_reference_impl<vector<Int>>(this);
    test_emplace_back_self_reference_impl<vector<TRInt>>(this);
    test_emplace_back_self_reference_impl<vector<TMInt>>(this);
    test_emplace_back_self_reference_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_emplace_back_self_reference_impl<vector<TRInt, fancy_allocator<TRInt>>>(this);
    test_emplace_back_self_reference_impl<vector<TMInt, fancy_allocator<TMInt>>>(this);
}

TEST(vector, emplace_back_initializer_list) {
    {
        vector<vector<int>> v1;
        v1.emplace_back({0, 1, 2, 3, 4});
        ASSERT_EQ(v1, std::initializer_list<std::initializer_list<int>>({
                          {0, 1, 2, 3, 4}
        }));
    }
}

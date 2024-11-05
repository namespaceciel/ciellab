#include <gtest/gtest.h>

#include <ciel/test/fancy_allocator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

namespace {

template<class C>
void
test_insert_size_value_impl(::testing::Test*) {
    using T = typename C::value_type;

    // expansion
    {
        C v{0, 1, 2, 3, 4};
        v.resize(v.capacity());

        v.insert(v.begin() + 2, 4, v[1]);
        v.resize(9);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 1, 1, 1, 1, 2, 3, 4}));
    }
    // emplace, count > pos_end_dis
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.end() - 1, 4, v[1]);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 1, 1, 1, 1, 4}));
    }
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.end() - 1, 4, v.back());
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 4, 4, 4, 4}));
    }
    // emplace, count < pos_end_dis
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin(), 2, v[1]);
        ASSERT_EQ(v, std::initializer_list<T>({1, 1, 0, 1, 2, 3, 4}));
    }
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.begin(), 2, v.back());
        ASSERT_EQ(v, std::initializer_list<T>({4, 4, 0, 1, 2, 3, 4}));
    }
    // emplace at end()
    {
        C v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.insert(v.end(), 4, v[1]);
        ASSERT_EQ(v, std::initializer_list<T>({0, 1, 2, 3, 4, 1, 1, 1, 1}));
    }
}

} // namespace

TEST(vector, insert_size_value) {
    test_insert_size_value_impl<vector<Int>>(this);
    test_insert_size_value_impl<vector<TRInt>>(this);
    test_insert_size_value_impl<vector<TMInt>>(this);
    test_insert_size_value_impl<vector<Int, fancy_allocator<Int>>>(this);
    test_insert_size_value_impl<vector<TRInt, fancy_allocator<TRInt>>>(this);
    test_insert_size_value_impl<vector<TMInt, fancy_allocator<TMInt>>>(this);
}

#include <gtest/gtest.h>

#include <ciel/test/forward_iterator.hpp>
#include <ciel/test/input_iterator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/test/min_allocator.hpp>
#include <ciel/test/random_access_iterator.hpp>
#include <ciel/vector.hpp>

#include <array>
#include <memory>

using namespace ciel;

TEST(vector, default_constructor_2) {
    {
        vector<Int> v;

        ASSERT_TRUE(v.empty());
        ASSERT_EQ(v.size(), 0);
        ASSERT_EQ(v.capacity(), 0);
    }
    {
        vector<Int, min_allocator<Int>> v;

        ASSERT_TRUE(v.empty());
        ASSERT_EQ(v.size(), 0);
        ASSERT_EQ(v.capacity(), 0);
    }
}

TEST(vector, default_constructor_with_allocator) {
    {
        vector<Int> v(std::allocator<int>{});

        ASSERT_TRUE(v.empty());
        ASSERT_EQ(v.size(), 0);
        ASSERT_EQ(v.capacity(), 0);
    }
    {
        vector<Int, min_allocator<Int>> v(min_allocator<Int>{});

        ASSERT_TRUE(v.empty());
        ASSERT_EQ(v.size(), 0);
        ASSERT_EQ(v.capacity(), 0);
    }
}

TEST(vector, constructor_n) {
    {
        vector<Int> v(3, Int{1});

        ASSERT_EQ(v, std::initializer_list<Int>({1, 1, 1}));
    }
    {
        vector<Int, min_allocator<Int>> v(4, Int{1});

        ASSERT_EQ(v, std::initializer_list<Int>({1, 1, 1, 1}));
    }
    {
        // distinguish from iterator range constructor
        vector<size_t> v(size_t{5}, size_t{5});

        ASSERT_EQ(v, std::initializer_list<size_t>({5, 5, 5, 5, 5}));
    }
}

TEST(vector, constructor_n_value) {
    {
        vector<Int> v(3);

        ASSERT_EQ(v, std::initializer_list<Int>({0, 0, 0}));
    }
    {
        vector<Int, min_allocator<Int>> v(4);

        ASSERT_EQ(v, std::initializer_list<Int>({0, 0, 0, 0}));
    }
}

TEST(vector, constructor_iterator_range) {
    // InputIterator
    {
        std::array<Int, 5> arr{0, 1, 2, 3, 4};
        vector<Int> v(InputIterator<Int>{arr.data()}, InputIterator<Int>{arr.data() + arr.size()});

        ASSERT_EQ(v, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        std::array<Int, 5> arr{0, 1, 2, 3, 4};
        vector<Int, min_allocator<Int>> v(InputIterator<Int>{arr.data()}, InputIterator<Int>{arr.data() + arr.size()});

        ASSERT_EQ(v, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        // empty range
        vector<Int> v(InputIterator<Int>{nullptr}, InputIterator<Int>{nullptr});

        ASSERT_TRUE(v.empty());
    }
    {
        // empty range
        vector<Int, min_allocator<Int>> v(InputIterator<Int>{nullptr}, InputIterator<Int>{nullptr});

        ASSERT_TRUE(v.empty());
    }

    // ForwardIterator
    {
        std::array<Int, 5> arr{0, 1, 2, 3, 4};
        vector<Int> v(ForwardIterator<Int>{arr.data()}, ForwardIterator<Int>{arr.data() + arr.size()});

        ASSERT_EQ(v, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        std::array<Int, 5> arr{0, 1, 2, 3, 4};
        vector<Int, min_allocator<Int>> v(ForwardIterator<Int>{arr.data()},
                                          ForwardIterator<Int>{arr.data() + arr.size()});

        ASSERT_EQ(v, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        // empty range
        vector<Int> v(ForwardIterator<Int>{nullptr}, ForwardIterator<Int>{nullptr});

        ASSERT_TRUE(v.empty());
    }
    {
        // empty range
        vector<Int, min_allocator<Int>> v(ForwardIterator<Int>{nullptr}, ForwardIterator<Int>{nullptr});

        ASSERT_TRUE(v.empty());
    }

    // random_access_iterator
    {
        std::array<Int, 5> arr{0, 1, 2, 3, 4};
        vector<Int> v(RandomAccessIterator<Int>{arr.data()}, RandomAccessIterator<Int>{arr.data() + arr.size()});

        ASSERT_EQ(v, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        std::array<Int, 5> arr{0, 1, 2, 3, 4};
        vector<Int, min_allocator<Int>> v(RandomAccessIterator<Int>{arr.data()},
                                          RandomAccessIterator<Int>{arr.data() + arr.size()});

        ASSERT_EQ(v, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        // empty range
        vector<Int> v(RandomAccessIterator<Int>{nullptr}, RandomAccessIterator<Int>{nullptr});

        ASSERT_TRUE(v.empty());
    }
    {
        // empty range
        vector<Int, min_allocator<Int>> v(RandomAccessIterator<Int>{nullptr}, RandomAccessIterator<Int>{nullptr});

        ASSERT_TRUE(v.empty());
    }

    // contiguous_iterator
    {
        std::array<Int, 5> arr{0, 1, 2, 3, 4};
        vector<Int> v(arr.data(), arr.data() + arr.size());

        ASSERT_EQ(v, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        std::array<Int, 5> arr{0, 1, 2, 3, 4};
        vector<Int, min_allocator<Int>> v(arr.data(), arr.data() + arr.size());

        ASSERT_EQ(v, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        // empty range
        vector<Int> v(static_cast<Int*>(nullptr), static_cast<Int*>(nullptr));

        ASSERT_TRUE(v.empty());
    }
    {
        // empty range
        vector<Int, min_allocator<Int>> v(static_cast<Int*>(nullptr), static_cast<Int*>(nullptr));

        ASSERT_TRUE(v.empty());
    }
}

TEST(vector, copy_constructor) {
    {
        vector<Int> v1({0, 1, 2, 3, 4});
        vector<Int> v2(v1);

        ASSERT_EQ(v2, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        vector<TRInt> v1({0, 1, 2, 3, 4});
        vector<TRInt> v2(v1);

        ASSERT_EQ(v2, std::initializer_list<TRInt>({0, 1, 2, 3, 4}));
    }
    {
        vector<Int, min_allocator<Int>> v1({0, 1, 2, 3, 4});
        vector<Int, min_allocator<Int>> v2(v1);

        ASSERT_EQ(v2, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        vector<TRInt, min_allocator<TRInt>> v1({0, 1, 2, 3, 4});
        vector<TRInt, min_allocator<TRInt>> v2(v1);

        ASSERT_EQ(v2, std::initializer_list<TRInt>({0, 1, 2, 3, 4}));
    }
}

TEST(vector, move_constructor) {
    {
        vector<Int> v1({0, 1, 2, 3, 4});
        vector<Int> v2(std::move(v1));

        ASSERT_EQ(v2, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        vector<Int, min_allocator<Int>> v1({0, 1, 2, 3, 4});
        vector<Int, min_allocator<Int>> v2(std::move(v1));

        ASSERT_EQ(v2, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
}

TEST(vector, constructor_initializer_list) {
    {
        vector<Int> v({0, 1, 2, 3, 4});

        ASSERT_EQ(v, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        vector<Int, min_allocator<Int>> v({0, 1, 2, 3, 4});

        ASSERT_EQ(v, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
}

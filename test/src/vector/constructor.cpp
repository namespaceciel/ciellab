#include <gtest/gtest.h>

#include <ciel/test/different_allocator.hpp>
#include <ciel/test/fancy_allocator.hpp>
#include <ciel/test/forward_iterator.hpp>
#include <ciel/test/input_iterator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/test/random_access_iterator.hpp>
#include <ciel/vector.hpp>

#include <array>
#include <memory>
#include <type_traits>

using namespace ciel;

TEST(vector, default_constructor) {
    {
        using C = vector<Int>;
        {
            C c;
            ASSERT_TRUE(c.empty());
            ASSERT_EQ(c.capacity(), 0);
            ASSERT_EQ(c.get_allocator(), C::allocator_type());
        }
        {
            C c = {};
            ASSERT_TRUE(c.empty());
            ASSERT_EQ(c.capacity(), 0);
            ASSERT_EQ(c.get_allocator(), C::allocator_type());
        }
    }
    {
        using C = vector<Int, fancy_allocator<Int>>;
        {
            C c;
            ASSERT_TRUE(c.empty());
            ASSERT_EQ(c.capacity(), 0);
            ASSERT_EQ(c.get_allocator(), C::allocator_type());
        }
        {
            C c = {};
            ASSERT_TRUE(c.empty());
            ASSERT_EQ(c.capacity(), 0);
            ASSERT_EQ(c.get_allocator(), C::allocator_type());
        }
    }
}

TEST(vector, default_constructor_with_allocator) {
    {
        vector<Int> v(std::allocator<Int>{});
        ASSERT_TRUE(v.empty());
        ASSERT_EQ(v.capacity(), 0);
    }
    {
        vector<Int, fancy_allocator<Int>> v(fancy_allocator<Int>{});
        ASSERT_TRUE(v.empty());
        ASSERT_EQ(v.capacity(), 0);
    }
}

TEST(vector, constructor_size) {
    {
        vector<Int> v(3, Int{1});
        ASSERT_EQ(v, std::initializer_list<Int>({1, 1, 1}));
    }
    {
        vector<Int, fancy_allocator<Int>> v(4, Int{1});
        ASSERT_EQ(v, std::initializer_list<Int>({1, 1, 1, 1}));
    }
    {
        // distinguish from iterator range constructor
        vector<size_t> v(size_t{5}, size_t{5});
        ASSERT_EQ(v, std::initializer_list<size_t>({5, 5, 5, 5, 5}));
    }
}

TEST(vector, constructor_size_value) {
    {
        vector<Int> v(3);
        ASSERT_EQ(v, std::initializer_list<Int>({0, 0, 0}));
    }
    {
        vector<Int, fancy_allocator<Int>> v(4);
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
        vector<Int, fancy_allocator<Int>> v(InputIterator<Int>{arr.data()},
                                            InputIterator<Int>{arr.data() + arr.size()});
        ASSERT_EQ(v, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        // empty range
        vector<Int> v(InputIterator<Int>{nullptr}, InputIterator<Int>{nullptr});
        ASSERT_TRUE(v.empty());
    }
    {
        // empty range
        vector<Int, fancy_allocator<Int>> v(InputIterator<Int>{nullptr}, InputIterator<Int>{nullptr});
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
        vector<Int, fancy_allocator<Int>> v(ForwardIterator<Int>{arr.data()},
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
        vector<Int, fancy_allocator<Int>> v(ForwardIterator<Int>{nullptr}, ForwardIterator<Int>{nullptr});
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
        vector<Int, fancy_allocator<Int>> v(RandomAccessIterator<Int>{arr.data()},
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
        vector<Int, fancy_allocator<Int>> v(RandomAccessIterator<Int>{nullptr}, RandomAccessIterator<Int>{nullptr});
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
        vector<Int, fancy_allocator<Int>> v(arr.data(), arr.data() + arr.size());
        ASSERT_EQ(v, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        // empty range
        vector<Int> v(static_cast<Int*>(nullptr), static_cast<Int*>(nullptr));
        ASSERT_TRUE(v.empty());
    }
    {
        // empty range
        vector<Int, fancy_allocator<Int>> v(static_cast<Int*>(nullptr), static_cast<Int*>(nullptr));
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
        vector<Int, fancy_allocator<Int>> v1({0, 1, 2, 3, 4});
        vector<Int, fancy_allocator<Int>> v2(v1);
        ASSERT_EQ(v2, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
}

TEST(vector, copy_constructor_with_allocator) {
    {
        vector<Int> v1({0, 1, 2, 3, 4});
        vector<Int> v2(v1, std::allocator<Int>{});
        ASSERT_EQ(v2, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        vector<Int, fancy_allocator<Int>> v1({0, 1, 2, 3, 4});
        vector<Int, fancy_allocator<Int>> v2(v1, fancy_allocator<Int>{});
        ASSERT_EQ(v2, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        vector<Int, different_allocator<Int>> v1({0, 1, 2, 3, 4});
        vector<Int, different_allocator<Int>> v2(v1, different_allocator<Int>{});
        ASSERT_EQ(v2, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
}

TEST(vector, move_constructor) {
    {
        vector<Int> v1({0, 1, 2, 3, 4});
        vector<Int> v2(std::move(v1));
        ASSERT_EQ(v2, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        vector<Int, fancy_allocator<Int>> v1({0, 1, 2, 3, 4});
        vector<Int, fancy_allocator<Int>> v2(std::move(v1));
        ASSERT_EQ(v2, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
}

TEST(vector, move_constructor_with_allocator) {
    {
        vector<Int> v1({0, 1, 2, 3, 4});
        vector<Int> v2(std::move(v1), std::allocator<Int>{});
        ASSERT_EQ(v2, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        vector<Int, fancy_allocator<Int>> v1({0, 1, 2, 3, 4});
        vector<Int, fancy_allocator<Int>> v2(std::move(v1), fancy_allocator<Int>{});
        ASSERT_EQ(v2, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        vector<Int, different_allocator<Int>> v1({0, 1, 2, 3, 4});
        vector<Int, different_allocator<Int>> v2(std::move(v1), different_allocator<Int>{});
        ASSERT_EQ(v2, std::initializer_list<Int>({0, 1, 2, 3, 4}));
        ASSERT_EQ(v1, std::initializer_list<Int>({-1, -1, -1, -1, -1}));
    }
}

TEST(vector, constructor_initializer_list) {
    {
        vector<Int> v({0, 1, 2, 3, 4});
        ASSERT_EQ(v, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        vector<Int, fancy_allocator<Int>> v({0, 1, 2, 3, 4});
        ASSERT_EQ(v, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
}

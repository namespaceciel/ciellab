#include <gtest/gtest.h>

#include <ciel/test/fancy_allocator.hpp>
#include <ciel/test/forward_iterator.hpp>
#include <ciel/test/input_iterator.hpp>
#include <ciel/test/int_wrapper.hpp>
#include <ciel/test/random_access_iterator.hpp>
#include <ciel/vector.hpp>

#include <array>
#include <memory>
#include <type_traits>

// TODO: simplify this file

using namespace ciel;

TEST(vector, default_constructor) {
    {
        using C = vector<int>;
        static_assert((noexcept(C()) == noexcept(C::allocator_type())), "");

        C c;
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.get_allocator(), C::allocator_type());

        C c1 = {};
        ASSERT_TRUE(c1.empty());
        ASSERT_EQ(c1.get_allocator(), C::allocator_type());
    }
    {
        using C = vector<int, fancy_allocator<int>>;
        static_assert((noexcept(C()) == noexcept(C::allocator_type())), "");

        C c;
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.get_allocator(), C::allocator_type());

        C c1 = {};
        ASSERT_TRUE(c1.empty());
        ASSERT_EQ(c1.get_allocator(), C::allocator_type());
    }
    {
        using C = vector<int, fancy_allocator<int>>;
        static_assert((noexcept(C(typename C::allocator_type()))
                       == std::is_nothrow_copy_constructible<typename C::allocator_type>::value),
                      "");

        typename C::allocator_type a{};
        C c(a);
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.get_allocator(), a);
    }
    {
        vector<int, fancy_allocator<int>> v;
        ASSERT_TRUE(v.empty());
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
        vector<Int, fancy_allocator<Int>> v(fancy_allocator<Int>{});

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
        vector<Int, fancy_allocator<Int>> v(4, Int{1});

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
        vector<TRInt> v1({0, 1, 2, 3, 4});
        vector<TRInt> v2(v1);

        ASSERT_EQ(v2, std::initializer_list<TRInt>({0, 1, 2, 3, 4}));
    }
    {
        vector<Int, fancy_allocator<Int>> v1({0, 1, 2, 3, 4});
        vector<Int, fancy_allocator<Int>> v2(v1);

        ASSERT_EQ(v2, std::initializer_list<Int>({0, 1, 2, 3, 4}));
    }
    {
        vector<TRInt, fancy_allocator<TRInt>> v1({0, 1, 2, 3, 4});
        vector<TRInt, fancy_allocator<TRInt>> v2(v1);

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
        vector<Int, fancy_allocator<Int>> v1({0, 1, 2, 3, 4});
        vector<Int, fancy_allocator<Int>> v2(std::move(v1));

        ASSERT_EQ(v2, std::initializer_list<Int>({0, 1, 2, 3, 4}));
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

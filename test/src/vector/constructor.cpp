#include <gtest/gtest.h>

#include <ciel/test/explicit_allocator.hpp>
#include <ciel/test/limited_allocator.hpp>
#include <ciel/test/min_allocator.hpp>
#include <ciel/test/not_constructible.h>
#include <ciel/test/test_allocator.hpp>
#include <ciel/vector.hpp>

#include <type_traits>

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
        using C = vector<NotConstructible>;
        static_assert((noexcept(C()) == noexcept(C::allocator_type())), "");

        C c;
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.get_allocator(), C::allocator_type());

        C c1 = {};
        ASSERT_TRUE(c1.empty());
        ASSERT_EQ(c1.get_allocator(), C::allocator_type());
    }
    {
        using C = vector<int, test_allocator<int>>;
        static_assert((noexcept(C(typename C::allocator_type()))
                       == std::is_nothrow_copy_constructible<typename C::allocator_type>::value),
                      "");

        typename C::allocator_type a(3);
        C c(a);
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.get_allocator(), a);
    }
    {
        using C = vector<NotConstructible, test_allocator<NotConstructible>>;
        static_assert((noexcept(C(typename C::allocator_type()))
                       == std::is_nothrow_copy_constructible<typename C::allocator_type>::value),
                      "");

        typename C::allocator_type a(5);
        C c(a);
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.get_allocator(), a);
    }
    {
        using C = vector<int, min_allocator<int>>;
        static_assert((noexcept(C()) == noexcept(C::allocator_type())), "");

        C c;
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.get_allocator(), C::allocator_type());

        C c1 = {};
        ASSERT_TRUE(c1.empty());
        ASSERT_EQ(c1.get_allocator(), C::allocator_type());
    }
    {
        using C = vector<NotConstructible, min_allocator<NotConstructible>>;
        static_assert((noexcept(C()) == noexcept(C::allocator_type())), "");

        C c;
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.get_allocator(), C::allocator_type());

        C c1 = {};
        ASSERT_TRUE(c1.empty());
        ASSERT_EQ(c1.get_allocator(), C::allocator_type());
    }
    {
        using C = vector<int, min_allocator<int>>;
        static_assert((noexcept(C(typename C::allocator_type()))
                       == std::is_nothrow_copy_constructible<typename C::allocator_type>::value),
                      "");

        typename C::allocator_type a{};
        C c(a);
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.get_allocator(), a);
    }
    {
        using C = vector<NotConstructible, min_allocator<NotConstructible>>;
        static_assert((noexcept(C(typename C::allocator_type()))
                       == std::is_nothrow_copy_constructible<typename C::allocator_type>::value),
                      "");

        typename C::allocator_type a{};
        C c(a);
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.get_allocator(), a);
    }
    {
        using C = vector<int, explicit_allocator<int>>;
        static_assert((noexcept(C()) == noexcept(C::allocator_type())), "");

        C c;
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.get_allocator(), C::allocator_type());

        C c1 = {};
        ASSERT_TRUE(c1.empty());
        ASSERT_EQ(c1.get_allocator(), C::allocator_type());
    }
    {
        using C = vector<NotConstructible, explicit_allocator<NotConstructible>>;
        static_assert((noexcept(C()) == noexcept(C::allocator_type())), "");

        C c;
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.get_allocator(), C::allocator_type());

        C c1 = {};
        ASSERT_TRUE(c1.empty());
        ASSERT_EQ(c1.get_allocator(), C::allocator_type());
    }
    {
        using C = vector<int, explicit_allocator<int>>;
        static_assert((noexcept(C(typename C::allocator_type()))
                       == std::is_nothrow_copy_constructible<typename C::allocator_type>::value),
                      "");

        typename C::allocator_type a{};
        C c(a);
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.get_allocator(), a);
    }
    {
        using C = vector<NotConstructible, explicit_allocator<NotConstructible>>;
        static_assert((noexcept(C(typename C::allocator_type()))
                       == std::is_nothrow_copy_constructible<typename C::allocator_type>::value),
                      "");

        typename C::allocator_type a{};
        C c(a);
        ASSERT_TRUE(c.empty());
        ASSERT_EQ(c.get_allocator(), a);
    }
    {
        vector<int, limited_allocator<int, 10>> v;
        ASSERT_TRUE(v.empty());
    }
    {
        vector<int, min_allocator<int>> v;
        ASSERT_TRUE(v.empty());
    }
    {
        vector<int, explicit_allocator<int>> v;
        ASSERT_TRUE(v.empty());
    }
}

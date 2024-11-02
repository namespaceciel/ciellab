#include <gtest/gtest.h>

#include <ciel/test/emplace_constructible.hpp>
#include <ciel/test/forward_iterator.hpp>
#include <ciel/test/input_iterator.hpp>
#include <ciel/test/maybe_pocca_allocator.hpp>
#include <ciel/test/min_allocator.hpp>
#include <ciel/test/operator_hijacker.hpp>
#include <ciel/test/other_allocator.hpp>
#include <ciel/test/safe_allocator.hpp>
#include <ciel/test/test_allocator.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

TEST(vector, assign_operator_hijacker) {
    vector<operator_hijacker> vo;
    vector<operator_hijacker> v;
    v = vo;
}

TEST(vector, assign_copy) {
    {
        vector<int, test_allocator<int>> l(3, 2, test_allocator<int>(5));
        vector<int, test_allocator<int>> l2(l, test_allocator<int>(3));

        l2 = l;
        ASSERT_EQ(l2, l);
        ASSERT_EQ(l2.get_allocator(), test_allocator<int>(3));
    }
    {
        vector<int, other_allocator<int>> l(3, 2, other_allocator<int>(5));
        vector<int, other_allocator<int>> l2(l, other_allocator<int>(3));

        l2 = l;
        ASSERT_EQ(l2, l);
        ASSERT_EQ(l2.get_allocator(), other_allocator<int>(5));
    }
    {
        // Test with Allocator::propagate_on_container_copy_assignment, false_type
        bool copy_assigned_into = false;
        vector<int, non_pocca_allocator<int>> l(3, 2, non_pocca_allocator<int>(5, nullptr));
        vector<int, non_pocca_allocator<int>> l2(l, non_pocca_allocator<int>(3, &copy_assigned_into));
        ASSERT_FALSE(copy_assigned_into);

        l2 = l;
        ASSERT_FALSE(copy_assigned_into);
        ASSERT_EQ(l2, l);
        ASSERT_EQ(l2.get_allocator(), non_pocca_allocator<int>(3, nullptr));
    }
    {
        // Test with Allocator::propagate_on_container_copy_assignment, true_type and equal allocators
        bool copy_assigned_into = false;
        vector<int, pocca_allocator<int>> l(3, 2, pocca_allocator<int>(5, nullptr));
        vector<int, pocca_allocator<int>> l2(l, pocca_allocator<int>(5, &copy_assigned_into));
        ASSERT_FALSE(copy_assigned_into);

        l2 = l;
        ASSERT_TRUE(copy_assigned_into);
        ASSERT_EQ(l2, l);
        ASSERT_EQ(l2.get_allocator(), pocca_allocator<int>(5, nullptr));
    }
    {
        // Test with Allocator::propagate_on_container_copy_assignment, true_type and unequal allocators
        bool copy_assigned_into = false;
        vector<int, pocca_allocator<int>> l(3, 2, pocca_allocator<int>(5, nullptr));
        vector<int, pocca_allocator<int>> l2(l, pocca_allocator<int>(3, &copy_assigned_into));
        ASSERT_FALSE(copy_assigned_into);

        l2 = l;
        ASSERT_TRUE(copy_assigned_into);
        ASSERT_EQ(l2, l);
        ASSERT_EQ(l2.get_allocator(), pocca_allocator<int>(5, nullptr));
    }
    {
        vector<int, min_allocator<int>> l(3, 2, min_allocator<int>());
        vector<int, min_allocator<int>> l2(l, min_allocator<int>());

        l2 = l;
        ASSERT_EQ(l2, l);
        ASSERT_EQ(l2.get_allocator(), min_allocator<int>());
    }
    {
        vector<int, safe_allocator<int>> l(3, 2, safe_allocator<int>());
        vector<int, safe_allocator<int>> l2(l, safe_allocator<int>());

        l2 = l;
        ASSERT_EQ(l2, l);
        ASSERT_EQ(l2.get_allocator(), safe_allocator<int>());
    }
}

TEST(vector, assign_initializer_list) {
    {
        vector<int> v;
        v.assign({3, 4, 5, 6});
        ASSERT_EQ(v, std::initializer_list<int>({3, 4, 5, 6}));
    }
    {
        vector<int> v;
        v.reserve(10);
        v.assign({3, 4, 5, 6});
        ASSERT_EQ(v, std::initializer_list<int>({3, 4, 5, 6}));
    }
    {
        vector<int, min_allocator<int>> v;
        v.assign({3, 4, 5, 6});
        ASSERT_EQ(v, std::initializer_list<int>({3, 4, 5, 6}));
    }
    {
        vector<int, min_allocator<int>> v;
        v.reserve(10);
        v.assign({3, 4, 5, 6});
        ASSERT_EQ(v, std::initializer_list<int>({3, 4, 5, 6}));
    }
}

TEST(vector, assign_iterator_range) {
    int arr1[] = {42};
    int arr2[] = {1, 101, 42};
    {
        using T  = EmplaceConstructibleMoveableAndAssignable<int>;
        using It = ForwardIterator<int>;
        {
            vector<T> v;
            v.assign(It(std::begin(arr1)), It(std::end(arr1)));
            ASSERT_EQ(v.size(), 1);
            ASSERT_EQ(v[0].value, 42);
        }
        {
            vector<T> v;
            v.assign(It(std::begin(arr2)), It(std::end(arr2)));
            ASSERT_EQ(v.size(), 3);
            ASSERT_EQ(v[0].value, 1);
            ASSERT_EQ(v[1].value, 101);
            ASSERT_EQ(v[2].value, 42);
        }
    }
    {
        using T  = EmplaceConstructibleMoveableAndAssignable<int>;
        using It = InputIterator<int>;
        {
            vector<T> v;
            v.assign(It(std::begin(arr1)), It(std::end(arr1)));
            ASSERT_EQ(v.size(), 1);
            ASSERT_EQ(v[0].value, 42);
            ASSERT_EQ(v[0].copied, 0);
        }
        {
            vector<T> v;
            v.assign(It(std::begin(arr2)), It(std::end(arr2)));
            ASSERT_EQ(v.size(), 3);
            ASSERT_EQ(v[0].value, 1);
            ASSERT_EQ(v[1].value, 101);
            ASSERT_EQ(v[2].value, 42);

            ASSERT_EQ(v[2].copied, 0);
        }
    }
    // Test with a number of elements in the source range that is greater than capacity
    {
        using It = ForwardIterator<int>;

        vector<int> dst(10);
        size_t n = dst.capacity() * 2;
        vector<int> src(n);

        dst.assign(It(src.data()), It(src.data() + src.size()));
        ASSERT_EQ(dst, src);
    }
}

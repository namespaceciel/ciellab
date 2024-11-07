#include <gtest/gtest.h>

#include <ciel/test/sbv_assign_tests.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

TEST(vector, assign_operator_hijacker) {
    vector<operator_hijacker> vo;
    vector<operator_hijacker> v;
    v = vo;
    v = std::move(vo);
}

TEST(vector, operator_copy) {
    {
        // propagate_on_container_copy_assignment: false_type, equal
        vector<Int, non_pocca_allocator<Int>> l(3, 2, non_pocca_allocator<Int>(5));
        vector<Int, non_pocca_allocator<Int>> l2(5, 1, non_pocca_allocator<Int>(5));
        test_operator_copy_impl(this, l, l2);
    }
    {
        // propagate_on_container_copy_assignment: false_type, unequal
        vector<Int, non_pocca_allocator<Int>> l(3, 2, non_pocca_allocator<Int>(5));
        vector<Int, non_pocca_allocator<Int>> l2(5, 1, non_pocca_allocator<Int>(3));
        test_operator_copy_impl(this, l, l2);
    }
    {
        // propagate_on_container_copy_assignment: true_type, equal
        vector<Int, pocca_allocator<Int>> l(3, 2, pocca_allocator<Int>(5));
        vector<Int, pocca_allocator<Int>> l2(5, 1, pocca_allocator<Int>(5));
        test_operator_copy_impl(this, l, l2);
    }
    {
        // propagate_on_container_copy_assignment: true_type, unequal
        vector<Int, pocca_allocator<Int>> l(3, 2, pocca_allocator<Int>(5));
        vector<Int, pocca_allocator<Int>> l2(5, 1, pocca_allocator<Int>(3));
        test_operator_copy_impl(this, l, l2);
    }
}

TEST(vector, operator_move) {
    {
        // propagate_on_container_move_assignment: false_type, equal
        vector<Int, non_pocma_allocator<Int>> l(3, 2, non_pocma_allocator<Int>(5));
        vector<Int, non_pocma_allocator<Int>> l2(5, 1, non_pocma_allocator<Int>(5));
        test_operator_move_impl(this, l, l2);
    }
    {
        // propagate_on_container_move_assignment: false_type, unequal
        vector<Int, non_pocma_allocator<Int>> l(3, 2, non_pocma_allocator<Int>(5));
        vector<Int, non_pocma_allocator<Int>> l2(5, 1, non_pocma_allocator<Int>(3));
        test_operator_move_impl(this, l, l2);

        // ASSERT_EQ(l, std::initializer_list<Int>({-1, -1, -1}));
    }
    {
        // propagate_on_container_move_assignment: true_type, equal
        vector<Int, pocma_allocator<Int>> l(3, 2, pocma_allocator<Int>(5));
        vector<Int, pocma_allocator<Int>> l2(5, 1, pocma_allocator<Int>(5));
        test_operator_move_impl(this, l, l2);
    }
    {
        // propagate_on_container_move_assignment: true_type, unequal
        vector<Int, pocma_allocator<Int>> l(3, 2, pocma_allocator<Int>(5));
        vector<Int, pocma_allocator<Int>> l2(5, 1, pocma_allocator<Int>(3));
        test_operator_move_impl(this, l, l2);
    }
}

TEST(vector, assign_iterator_range) {
    // assign 5 elements

    // capacity < 5
    {
        const vector<Int> v(1, 1);
        ASSERT_LT(v.capacity(), 5); // assume
        test_assign_iterator_range_impl<InputIterator<Int>>(this, v);
        test_assign_iterator_range_impl<ForwardIterator<Int>>(this, v);
        test_assign_iterator_range_impl<RandomAccessIterator<Int>>(this, v);
        test_assign_iterator_range_impl<Int*>(this, v);
    }
    {
        const vector<Int, fancy_allocator<Int>> v(1, 1);
        ASSERT_LT(v.capacity(), 5); // assume
        test_assign_iterator_range_impl<InputIterator<Int>>(this, v);
        test_assign_iterator_range_impl<ForwardIterator<Int>>(this, v);
        test_assign_iterator_range_impl<RandomAccessIterator<Int>>(this, v);
        test_assign_iterator_range_impl<Int*>(this, v);
    }
    // capacity >= 5, size < 5
    {
        vector<Int> v(1, 1);
        v.reserve(6);
        test_assign_iterator_range_impl<InputIterator<Int>>(this, v);
        test_assign_iterator_range_impl<ForwardIterator<Int>>(this, v);
        test_assign_iterator_range_impl<RandomAccessIterator<Int>>(this, v);
        test_assign_iterator_range_impl<Int*>(this, v);
    }
    {
        vector<Int, fancy_allocator<Int>> v(1, 1);
        v.reserve(6);
        test_assign_iterator_range_impl<InputIterator<Int>>(this, v);
        test_assign_iterator_range_impl<ForwardIterator<Int>>(this, v);
        test_assign_iterator_range_impl<RandomAccessIterator<Int>>(this, v);
        test_assign_iterator_range_impl<Int*>(this, v);
    }
    // size >= 5
    {
        const vector<Int> v(6, 1);
        test_assign_iterator_range_impl<InputIterator<Int>>(this, v);
        test_assign_iterator_range_impl<ForwardIterator<Int>>(this, v);
        test_assign_iterator_range_impl<RandomAccessIterator<Int>>(this, v);
        test_assign_iterator_range_impl<Int*>(this, v);
    }
    {
        const vector<Int, fancy_allocator<Int>> v(6, 1);
        test_assign_iterator_range_impl<InputIterator<Int>>(this, v);
        test_assign_iterator_range_impl<ForwardIterator<Int>>(this, v);
        test_assign_iterator_range_impl<RandomAccessIterator<Int>>(this, v);
        test_assign_iterator_range_impl<Int*>(this, v);
    }
}

TEST(vector, assign_size_value) {
    // assign 5 elements

    // capacity < 5
    {
        vector<Int> v(1, 1);
        test_assign_size_value_impl(this, v);
    }
    {
        vector<Int, fancy_allocator<Int>> v(1, 1);
        test_assign_size_value_impl(this, v);
    }
    // capacity >= 5, size < 5
    {
        vector<Int> v(1, 1);
        v.reserve(6);
        test_assign_size_value_impl(this, v);
    }
    {
        vector<Int, fancy_allocator<Int>> v(1, 1);
        v.reserve(6);
        test_assign_size_value_impl(this, v);
    }
    // size >= 5
    {
        vector<Int> v(6, 1);
        test_assign_size_value_impl(this, v);
    }
    {
        vector<Int, fancy_allocator<Int>> v(6, 1);
        test_assign_size_value_impl(this, v);
    }
}

TEST(vector, assign_size_self_value) {
    {
        // shrink size
        vector<Int> v{0, 1, 2, 3, 4};
        ASSERT_EQ(v.size(), v.capacity());

        v.assign(2, v[2]);
        ASSERT_EQ(v, std::initializer_list<Int>({2, 2}));
    }
    {
        // shrink size 2
        vector<Int> v{0, 1, 2, 3, 4};
        ASSERT_EQ(v.size(), v.capacity());

        v.assign(3, v[1]);
        ASSERT_EQ(v, std::initializer_list<Int>({1, 1, 1}));
    }
    {
        // increase size
        vector<Int> v{0, 1, 2, 3, 4};
        v.reserve(10);

        v.assign(6, v[2]);
        ASSERT_EQ(v, std::initializer_list<Int>({2, 2, 2, 2, 2, 2}));
    }
    {
        // expansion
        vector<Int> v{0, 1, 2, 3, 4};

        const auto new_size = v.capacity() + 1;
        v.assign(new_size, v[2]);

        ASSERT_EQ(v.size(), new_size);
        ASSERT_TRUE(std::all_of(v.begin(), v.end(), [](int i) {
            return i == 2;
        }));
    }
}

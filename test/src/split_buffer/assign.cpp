#include <gtest/gtest.h>

#include <ciel/split_buffer.hpp>
#include <ciel/test/sbv_assign_tests.hpp>

using namespace ciel;

TEST(split_buffer, assign_operator_hijacker) {
    split_buffer<operator_hijacker> vo;
    split_buffer<operator_hijacker> v;
    v = vo;
    v = std::move(vo);
}

TEST(split_buffer, operator_copy) {
    {
        // propagate_on_container_copy_assignment: false_type, equal
        split_buffer<Int, non_pocca_allocator<Int>> l(3, 2, non_pocca_allocator<Int>(5));
        split_buffer<Int, non_pocca_allocator<Int>> l2(5, 1, non_pocca_allocator<Int>(5));
        test_operator_copy_impl(this, l, l2);
    }
    {
        // propagate_on_container_copy_assignment: false_type, unequal
        split_buffer<Int, non_pocca_allocator<Int>> l(3, 2, non_pocca_allocator<Int>(5));
        split_buffer<Int, non_pocca_allocator<Int>> l2(5, 1, non_pocca_allocator<Int>(3));
        test_operator_copy_impl(this, l, l2);
    }
    {
        // propagate_on_container_copy_assignment: true_type, equal
        split_buffer<Int, pocca_allocator<Int>> l(3, 2, pocca_allocator<Int>(5));
        split_buffer<Int, pocca_allocator<Int>> l2(5, 1, pocca_allocator<Int>(5));
        test_operator_copy_impl(this, l, l2);
    }
    {
        // propagate_on_container_copy_assignment: true_type, unequal
        split_buffer<Int, pocca_allocator<Int>> l(3, 2, pocca_allocator<Int>(5));
        split_buffer<Int, pocca_allocator<Int>> l2(5, 1, pocca_allocator<Int>(3));
        test_operator_copy_impl(this, l, l2);
    }
}

TEST(split_buffer, operator_move) {
    {
        // propagate_on_container_move_assignment: false_type, equal
        split_buffer<Int, non_pocma_allocator<Int>> l(3, 2, non_pocma_allocator<Int>(5));
        split_buffer<Int, non_pocma_allocator<Int>> l2(5, 1, non_pocma_allocator<Int>(5));
        test_operator_move_impl(this, l, l2);
    }
    {
        // propagate_on_container_move_assignment: false_type, unequal
        split_buffer<Int, non_pocma_allocator<Int>> l(3, 2, non_pocma_allocator<Int>(5));
        split_buffer<Int, non_pocma_allocator<Int>> l2(5, 1, non_pocma_allocator<Int>(3));
        test_operator_move_impl(this, l, l2);

        // ASSERT_EQ(l, std::initializer_list<Int>({-1, -1, -1}));
    }
    {
        // propagate_on_container_move_assignment: true_type, equal
        split_buffer<Int, pocma_allocator<Int>> l(3, 2, pocma_allocator<Int>(5));
        split_buffer<Int, pocma_allocator<Int>> l2(5, 1, pocma_allocator<Int>(5));
        test_operator_move_impl(this, l, l2);
    }
    {
        // propagate_on_container_move_assignment: true_type, unequal
        split_buffer<Int, pocma_allocator<Int>> l(3, 2, pocma_allocator<Int>(5));
        split_buffer<Int, pocma_allocator<Int>> l2(5, 1, pocma_allocator<Int>(3));
        test_operator_move_impl(this, l, l2);
    }
}

TEST(split_buffer, assign_iterator_range) {
    // assign 5 elements

    // back spare < 3, no front spare
    {
        const split_buffer<Int> v(1, 1);
        ASSERT_LT(v.back_spare(), 3);  // assume
        ASSERT_EQ(v.front_spare(), 0); // assume
        test_assign_iterator_range_impl<InputIterator<Int>>(this, v);
        test_assign_iterator_range_impl<ForwardIterator<Int>>(this, v);
        test_assign_iterator_range_impl<RandomAccessIterator<Int>>(this, v);
        test_assign_iterator_range_impl<Int*>(this, v);
    }
    {
        const split_buffer<Int, fancy_allocator<Int>> v(1, 1);
        ASSERT_LT(v.back_spare(), 3);  // assume
        ASSERT_EQ(v.front_spare(), 0); // assume
        test_assign_iterator_range_impl<InputIterator<Int>>(this, v);
        test_assign_iterator_range_impl<ForwardIterator<Int>>(this, v);
        test_assign_iterator_range_impl<RandomAccessIterator<Int>>(this, v);
        test_assign_iterator_range_impl<Int*>(this, v);
    }
    // back_spare < 3, front+back spare >= 3
    {
        split_buffer<Int> v(1, 1);
        v.reserve_front_spare(3);
        ASSERT_LT(v.back_spare(), 3); // assume
        test_assign_iterator_range_impl<InputIterator<Int>>(this, v);
        test_assign_iterator_range_impl<ForwardIterator<Int>>(this, v);
        test_assign_iterator_range_impl<RandomAccessIterator<Int>>(this, v);
        test_assign_iterator_range_impl<Int*>(this, v);
    }
    {
        split_buffer<Int, fancy_allocator<Int>> v(1, 1);
        v.reserve_front_spare(3);
        ASSERT_LT(v.back_spare(), 3); // assume
        test_assign_iterator_range_impl<InputIterator<Int>>(this, v);
        test_assign_iterator_range_impl<ForwardIterator<Int>>(this, v);
        test_assign_iterator_range_impl<RandomAccessIterator<Int>>(this, v);
        test_assign_iterator_range_impl<Int*>(this, v);
    }
    // back_spare >= 5, size < 5
    {
        split_buffer<Int> v(1, 1);
        v.reserve_back_spare(6);
        test_assign_iterator_range_impl<InputIterator<Int>>(this, v);
        test_assign_iterator_range_impl<ForwardIterator<Int>>(this, v);
        test_assign_iterator_range_impl<RandomAccessIterator<Int>>(this, v);
        test_assign_iterator_range_impl<Int*>(this, v);
    }
    {
        split_buffer<Int, fancy_allocator<Int>> v(1, 1);
        v.reserve_back_spare(6);
        test_assign_iterator_range_impl<InputIterator<Int>>(this, v);
        test_assign_iterator_range_impl<ForwardIterator<Int>>(this, v);
        test_assign_iterator_range_impl<RandomAccessIterator<Int>>(this, v);
        test_assign_iterator_range_impl<Int*>(this, v);
    }
    // size >= 5
    {
        const split_buffer<Int> v(6, 1);
        test_assign_iterator_range_impl<InputIterator<Int>>(this, v);
        test_assign_iterator_range_impl<ForwardIterator<Int>>(this, v);
        test_assign_iterator_range_impl<RandomAccessIterator<Int>>(this, v);
        test_assign_iterator_range_impl<Int*>(this, v);
    }
    {
        const split_buffer<Int, fancy_allocator<Int>> v(6, 1);
        test_assign_iterator_range_impl<InputIterator<Int>>(this, v);
        test_assign_iterator_range_impl<ForwardIterator<Int>>(this, v);
        test_assign_iterator_range_impl<RandomAccessIterator<Int>>(this, v);
        test_assign_iterator_range_impl<Int*>(this, v);
    }
}

TEST(split_buffer, assign_size_value) {
    // assign 5 elements

    // back spare < 3, no front spare
    {
        split_buffer<Int> v(1, 1);
        ASSERT_LT(v.back_spare(), 3);  // assume
        ASSERT_EQ(v.front_spare(), 0); // assume
        test_assign_size_value_impl(this, v);
    }
    {
        split_buffer<Int, fancy_allocator<Int>> v(1, 1);
        ASSERT_LT(v.back_spare(), 3);  // assume
        ASSERT_EQ(v.front_spare(), 0); // assume
        test_assign_size_value_impl(this, v);
    }
    // back_spare < 3, front+back spare >= 3
    {
        split_buffer<Int> v(1, 1);
        v.reserve_front_spare(3);
        ASSERT_LT(v.back_spare(), 3); // assume
        test_assign_size_value_impl(this, v);
    }
    {
        split_buffer<Int, fancy_allocator<Int>> v(1, 1);
        v.reserve_front_spare(3);
        ASSERT_LT(v.back_spare(), 3); // assume
        test_assign_size_value_impl(this, v);
    }
    // back_spare >= 5, size < 5
    {
        split_buffer<Int> v(1, 1);
        v.reserve_back_spare(6);
        test_assign_size_value_impl(this, v);
    }
    {
        split_buffer<Int, fancy_allocator<Int>> v(1, 1);
        v.reserve_back_spare(6);
        test_assign_size_value_impl(this, v);
    }
    // size >= 5
    {
        split_buffer<Int> v(6, 1);
        test_assign_size_value_impl(this, v);
    }
    {
        split_buffer<Int, fancy_allocator<Int>> v(6, 1);
        test_assign_size_value_impl(this, v);
    }
}

TEST(split_buffer, assign_size_self_value) {
    {
        // shrink size
        split_buffer<Int> v{0, 1, 2, 3, 4};
        ASSERT_EQ(v.size(), v.capacity());

        v.assign(2, v[2]);
        ASSERT_EQ(v, std::initializer_list<Int>({2, 2}));
    }
    {
        // shrink size 2
        split_buffer<Int> v{0, 1, 2, 3, 4};
        ASSERT_EQ(v.size(), v.capacity());

        v.assign(3, v[1]);
        ASSERT_EQ(v, std::initializer_list<Int>({1, 1, 1}));
    }
    {
        // increase size
        split_buffer<Int> v{0, 1, 2, 3, 4};
        v.reserve_back_spare(10);

        v.assign(6, v[2]);
        ASSERT_EQ(v, std::initializer_list<Int>({2, 2, 2, 2, 2, 2}));
    }
    {
        // expansion
        split_buffer<Int> v{0, 1, 2, 3, 4};

        const auto new_size = v.capacity() + 1;
        v.assign(new_size, v[2]);

        ASSERT_EQ(v.size(), new_size);
        ASSERT_TRUE(std::all_of(v.begin(), v.end(), [](int i) {
            return i == 2;
        }));
    }
}

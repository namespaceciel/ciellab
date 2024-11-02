#include <gtest/gtest.h>

#include <ciel/test/limited_allocator.hpp>
#include <ciel/vector.hpp>

using namespace ciel;

TEST(vector, max_size) {
    const size_t ptrdiff_t_max = static_cast<size_t>(std::numeric_limits<ptrdiff_t>::max());
    {
        vector<int, limited_allocator<int, 10>> c;
        ASSERT_EQ(c.max_size(), 10);
    }
    {
        vector<int, limited_allocator<int, static_cast<size_t>(-1)>> c;
        ASSERT_EQ(c.max_size(), ptrdiff_t_max);
    }
    {
        vector<char> c;
        ASSERT_LE(c.max_size(), ptrdiff_t_max);
        ASSERT_LE(c.max_size(), std::allocator_traits<std::allocator<char>>::max_size(c.get_allocator()));
    }
}

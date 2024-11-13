#include <gtest/gtest.h>

#include <ciel/vector.hpp>

#include <cstddef>
#include <limits>
#include <memory>

using namespace ciel;

TEST(vector, max_size) {
    const size_t ptrdiff_t_max = static_cast<size_t>(std::numeric_limits<ptrdiff_t>::max());
    {
        const vector<char> c;
        ASSERT_LE(c.max_size(), ptrdiff_t_max);
        ASSERT_LE(c.max_size(), std::allocator_traits<std::allocator<char>>::max_size(c.get_allocator()));
    }
}

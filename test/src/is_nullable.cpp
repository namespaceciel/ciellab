#include <gtest/gtest.h>

#include <ciel/is_nullable.hpp>

#include <memory>

using namespace ciel;

TEST(is_nullable, all) {
    static_assert(is_nullable<void*>::value, "");
    static_assert(is_nullable<int*>::value, "");

    static_assert(is_nullable<std::unique_ptr<int>>::value, "");
    static_assert(is_nullable<std::shared_ptr<int>>::value, "");
}

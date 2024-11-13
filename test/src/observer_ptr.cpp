#include <gtest/gtest.h>

#include <ciel/observer_ptr.hpp>

#include <functional>
#include <memory>
#include <utility>

using namespace ciel;

TEST(observer_ptr, all) {
    struct Base {};

    struct Derived : Base {};

    observer_ptr<int> p1;
    const observer_ptr<int> p2{nullptr};

    ASSERT_FALSE(p1);
    ASSERT_FALSE(p2);

    const std::unique_ptr<int> up1{new int{123}};
    observer_ptr<int> p3{up1.get()};

    ASSERT_TRUE(p3);
    ASSERT_EQ(up1.get(), p3.get());
    ASSERT_EQ(*p3, 123);

    ASSERT_EQ(std::hash<observer_ptr<int>>()(p3), std::hash<int*>()(p3.get()));

    std::swap(p1, p3);

    ASSERT_FALSE(p3);
    ASSERT_EQ(up1.get(), p1.get());

    const std::unique_ptr<Derived> up2{new Derived{}};
    observer_ptr<Base> p4{up2.get()};

    ASSERT_TRUE(p4);
    ASSERT_EQ(up2.get(), p4.get());
    ASSERT_EQ(up2.get(), p4.release());
    ASSERT_FALSE(p4);

    p4.reset();
    ASSERT_FALSE(p4);

    p4.reset(up2.get());
    ASSERT_EQ(up2.get(), p4.get());
}

#include <gtest/gtest.h>

#include <ciel/core/cstring.hpp>

#include <string>

using namespace ciel;

TEST(cstring, find) {
    const std::string s    = "This is a string.";
    const std::string sub1 = "is a";
    const std::string sub2 = "";
    const std::string sub3 = "This a";
    const std::string sub4 = "This is a string...";
    const std::string sub5 = "This is a string.";

    ASSERT_EQ(ciel::find(s.data(), s.data() + s.size(), '.'), s.data() + 16);
    ASSERT_EQ(ciel::find(s.data(), s.data() + s.size(), 'c'), nullptr);

    ASSERT_EQ(ciel::find(s.data(), s.data() + s.size(), sub1.data(), sub1.data() + sub1.size()), s.data() + 5);
    ASSERT_EQ(ciel::find(s.data(), s.data() + s.size(), sub2.data(), sub2.data() + sub2.size()), s.data() + 0);
    ASSERT_EQ(ciel::find(s.data(), s.data() + s.size(), sub3.data(), sub3.data() + sub3.size()), nullptr);
    ASSERT_EQ(ciel::find(s.data(), s.data() + s.size(), sub4.data(), sub4.data() + sub4.size()), nullptr);
    ASSERT_EQ(ciel::find(s.data(), s.data() + s.size(), sub5.data(), sub5.data() + sub5.size()), s.data() + 0);
}

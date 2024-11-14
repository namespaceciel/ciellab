#include <gtest/gtest.h>

#include <ciel/core/message.hpp>

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

using namespace ciel;

TEST(message, pure_text) {
    const message_builder<128> mb("This is pure text.");
    ASSERT_EQ(std::strcmp(mb.get(), "This is pure text."), 0);
}

TEST(message, text_with_integer) {
    {
        const message_builder<128> mb("Test integer: {}, {}. Is this correct?", std::numeric_limits<int64_t>::max(),
                                      std::numeric_limits<int64_t>::min());
        ASSERT_EQ(std::strcmp(mb.get(), "Test integer: 9223372036854775807, -9223372036854775808. Is this correct?"),
                  0);
    }
    {
        const message_builder<128> mb("Test integer: {}. Is this correct?", 0);
        ASSERT_EQ(std::strcmp(mb.get(), "Test integer: 0. Is this correct?"), 0);
    }
    {
        const message_builder<25> mb("Test integer: {}. Is this correct?", std::numeric_limits<int64_t>::max());
        ASSERT_EQ(std::strcmp(mb.get(), "Test integer: 9223372036"), 0);
    }
    {
        const message_builder<25> mb("Test integer: {}. Is this correct?", std::numeric_limits<int64_t>::min());
        ASSERT_EQ(std::strcmp(mb.get(), "Test integer: -922337203"), 0);
    }
}

TEST(message, print) {
    {
        std::array<char, 64> buf; // NOLINT(cppcoreguidelines-pro-type-member-init)
        ciel::print(buf.data(), "This is {} testing!", "message.hpp");
        ASSERT_EQ(std::strcmp(buf.data(), "This is message.hpp testing!"), 0);
    }
    { ciel::println("This is {} testing!", "message.hpp"); }
}

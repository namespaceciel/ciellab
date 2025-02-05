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

TEST(message, text_with_pointer) {
    {
        const message_builder<512> mb("{}", reinterpret_cast<void*>(static_cast<uintptr_t>(0)));
        ASSERT_EQ(std::strcmp(mb.get(), "(nullptr)"), 0);
    }
    {
        const message_builder<512> mb("{}", reinterpret_cast<void*>(static_cast<uintptr_t>(0xffffffffffff)));
        ASSERT_EQ(std::strcmp(mb.get(), "0xffff'ffff'ffff"), 0);
    }
    {
        const message_builder<512> mb("{}", reinterpret_cast<void*>(static_cast<uintptr_t>(127)));
        ASSERT_EQ(std::strcmp(mb.get(), "0x0000'0000'007f"), 0);
    }
}

TEST(message, print) {
    {
        std::array<char, 64> buf;
        ciel::print(buf.data(), "This is {} testing!", "message.hpp");
        ASSERT_EQ(std::strcmp(buf.data(), "This is message.hpp testing!"), 0);
    }
    { ciel::println("This is {} testing!", "message.hpp"); }
}

TEST(message, print_address) {
    struct {
        int a{};
        int b{};
        int c{};
    } s;

    ciel::println("Test addresses printing:\n{}\n{}\n{}", &s.a, &s.b, &s.c);
}

/*
TEST(message, expect_message) {
#ifndef CIEL_HAS_EXCEPTIONS
    CIEL_THROW_EXCEPTION(std::bad_array_new_length{});
#endif

    // CIEL_ASSERT(false);
    CIEL_ASSERT_M(false, "Test string: {}", "This is string.");
}
*/

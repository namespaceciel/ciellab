#include <gtest/gtest.h>

#include <ciel/core/message.hpp>

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

using namespace ciel;

TEST(message, print) {
    ciel::println("This is {} testing! This is {} testing! This is {} testing! This is {} testing!", "message.hpp",
                  "message.hpp", "message.hpp", "message.hpp");
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

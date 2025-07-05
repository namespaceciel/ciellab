#include <gtest/gtest.h>

#include <ciel/core/to_chars.hpp>

#include <array>
#include <cstdint>
#include <limits>

using namespace ciel;

namespace {

bool is_string_equal(const char* lhs, const char* rhs) noexcept {
    while (*lhs != 0 && *rhs != 0) {
        if (*lhs != *rhs) {
            return false;
        }
        ++lhs;
        ++rhs;
    }

    if (*lhs != 0 || *rhs != 0) {
        return false;
    }

    return true;
}

} // namespace

TEST(to_chars, bool) {
    std::array<char, 128> buffer{};
    char* it = buffer.data();
    it       = ciel::to_chars(it, true);
    *it++    = ',';
    it       = ciel::to_chars(it, false);

    ASSERT_TRUE(is_string_equal(buffer.data(), "true,false"));
}

TEST(to_chars, uint64_t) {
    std::array<char, 128> buffer{};
    char* it = buffer.data();
    it       = ciel::to_chars(it, static_cast<uint64_t>(0));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<uint64_t>(9999999999));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()));
    *it++    = ',';
    it       = ciel::to_chars(it, std::numeric_limits<uint64_t>::max());

    ASSERT_TRUE(is_string_equal(buffer.data(), "0,9999999999,4294967295,18446744073709551615"));
}

TEST(to_chars, int64_t) {
    std::array<char, 128> buffer{};
    char* it = buffer.data();
    it       = ciel::to_chars(it, static_cast<int64_t>(0));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<int64_t>(9999999999));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<int64_t>(std::numeric_limits<uint32_t>::max()));
    *it++    = ',';
    it       = ciel::to_chars(it, std::numeric_limits<int64_t>::max());
    *it++    = ',';
    it       = ciel::to_chars(it, std::numeric_limits<int64_t>::min());

    ASSERT_TRUE(is_string_equal(buffer.data(), "0,9999999999,4294967295,9223372036854775807,-9223372036854775808"));
}

TEST(to_chars, uint32_t) {
    std::array<char, 128> buffer{};
    char* it = buffer.data();
    it       = ciel::to_chars(it, static_cast<uint32_t>(0));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<uint32_t>(12));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<uint32_t>(123));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<uint32_t>(1234));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<uint32_t>(12345));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<uint32_t>(123456));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<uint32_t>(1234567));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<uint32_t>(12345678));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<uint32_t>(123456789));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<uint32_t>(1234567890));
    *it++    = ',';
    it       = ciel::to_chars(it, std::numeric_limits<uint32_t>::max());

    ASSERT_TRUE(
        is_string_equal(buffer.data(), "0,12,123,1234,12345,123456,1234567,12345678,123456789,1234567890,4294967295"));
}

TEST(to_chars, int32_t) {
    std::array<char, 128> buffer{};
    char* it = buffer.data();
    it       = ciel::to_chars(it, static_cast<int32_t>(0));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<int32_t>(12));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<int32_t>(123));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<int32_t>(1234));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<int32_t>(12345));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<int32_t>(123456));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<int32_t>(1234567));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<int32_t>(12345678));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<int32_t>(123456789));
    *it++    = ',';
    it       = ciel::to_chars(it, static_cast<int32_t>(1234567890));
    *it++    = ',';
    it       = ciel::to_chars(it, std::numeric_limits<int32_t>::max());
    *it++    = ',';
    it       = ciel::to_chars(it, std::numeric_limits<int32_t>::min());

    ASSERT_TRUE(is_string_equal(
        buffer.data(), "0,12,123,1234,12345,123456,1234567,12345678,123456789,1234567890,2147483647,-2147483648"));
}

TEST(to_chars, pointer) {
    std::array<char, 128> buffer{};
    char* it = buffer.data();
    it       = ciel::to_chars(it, nullptr);
    *it++    = ',';
    it       = ciel::to_chars(it, reinterpret_cast<void*>(0x0123456789abcdef));
    *it++    = ',';
    it       = ciel::to_chars(it, reinterpret_cast<void*>(0x09ab00ef));

    ASSERT_TRUE(is_string_equal(buffer.data(), "(nullptr),0x0123456789abcdef,0x0000000009ab00ef"));
}

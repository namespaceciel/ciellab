#ifndef CIELLAB_INCLUDE_CIEL_CORE_MESSAGE_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_MESSAGE_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/is_range.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <utility>

#ifdef __cpp_lib_source_location
#  include <source_location>
#  define CIEL_CURRENT_LINE std::source_location::current().line()
#  define CIEL_CURRENT_FILE std::source_location::current().file_name()
#else
#  define CIEL_CURRENT_LINE CIEL_TO_STRING(__LINE__)
#  define CIEL_CURRENT_FILE __FILE__
#endif

NAMESPACE_CIEL_BEGIN

// Inspired by Microsoft snmalloc's implementation.

template<size_t BufferSize>
class message_builder {
private:
    static_assert(BufferSize != 0, "At least one char needed to be \\0");

    std::array<char, BufferSize> buffer_{};
    size_t end_{0};

public:
    template<class... Args>
    message_builder(const char* msg, Args... args) noexcept {
        append(msg, args...);
    }

    message_builder(const message_builder&)            = delete;
    message_builder& operator=(const message_builder&) = delete;

    void append(const char* msg) noexcept {
        for (const char* s = msg; *s != 0; ++s) {
            append(*s);
        }
    }

    template<class Front, class... Rest>
    void append(const char* msg, Front front, Rest... rest) noexcept {
        const char* s = msg;
        for (; *s != 0; ++s) {
            if (s[0] == '{' && s[1] == '}') {
                break;
            }

            append(*s);
        }

        append(front);
        append(s + 2, rest...);
    }

    void append(char c) noexcept {
        if (end_ < buffer_.size() - 1) {
            buffer_[end_++] = c;
        }
    }

    template<class Int, enable_if_t<std::is_integral<Int>::value> = 0>
    void append(Int value) noexcept {
        if (value == 0) {
            append('0');
            return;
        }

        std::array<char, 20> temp{}; // Enough to hold the longest 64 bit decimal number.
        size_t p = 0;

        if (value < 0) {
            append('-');

            while (value != 0) {
                temp[p++] = static_cast<char>(-(value % 10) + '0');
                value /= 10;
            }

        } else {
            while (value != 0) {
                temp[p++] = static_cast<char>(value % 10 + '0');
                value /= 10;
            }
        }

        for (auto it = decltype(temp)::reverse_iterator(temp.begin() + p); it != temp.rend(); ++it) {
            append(*it);
        }
    }

    void append(const void* p) noexcept {
        if (p == nullptr) {
            append(nullptr);
            return;
        }

        append("(0x");

        {
            const char hexdigits[] = "0123456789abcdef";

            uintptr_t s = reinterpret_cast<uintptr_t>(p);

            std::array<char, 14> temp{};

            unsigned int to_insert_divider = 0;
            for (auto it = temp.rbegin(); it != temp.rend(); ++to_insert_divider, ++it) {
                if (to_insert_divider > 0 && to_insert_divider % 4 == 0) {
                    *it = '\'';
                    ++it;
                }

                *it = hexdigits[s & 0xf];
                s >>= 4;
            }

            append(temp);
        }

        append(" | 0b");

        {
            const char bindigits[] = "01";

            uintptr_t s = reinterpret_cast<uintptr_t>(p);

            std::array<char, 59> temp{};

            unsigned int to_insert_divider = 0;
            for (auto it = temp.rbegin(); it != temp.rend(); ++to_insert_divider, ++it) {
                if (to_insert_divider > 0 && to_insert_divider % 4 == 0) {
                    *it = '\'';
                    ++it;
                }

                *it = bindigits[s & 0x1];
                s >>= 1;
            }

            append(temp);
        }

        append(')');
    }

    void append(nullptr_t) noexcept {
        append("(nullptr)");
    }

    template<class R, enable_if_t<is_range<R>::value> = 0>
    void append(R&& r) noexcept {
        for (auto&& e : r) {
            append(e);
        }
    }

    CIEL_NODISCARD const char* get() const noexcept {
        return buffer_.data();
    }

    // '\0' doesn't count.
    CIEL_NODISCARD size_t size() const noexcept {
        return end_;
    }

}; // class message_builder

// print

template<size_t BufferSize = 512, class... Args>
void print(std::FILE* stream, const char* msg, Args... args) noexcept {
    const message_builder<BufferSize> mb(msg, args...);
    CIEL_UNUSED(std::fprintf(stream, "%s", mb.get()));
}

template<size_t BufferSize = 512, class... Args>
void print(const char* msg, Args... args) noexcept {
    print<BufferSize>(stdout, msg, args...);
}

template<size_t BufferSize = 512, class... Args>
void print(char* buffer, const char* msg, Args... args) noexcept {
    const message_builder<BufferSize> mb(msg, args...);
    std::memmove(buffer, mb.get(), mb.size() + 1);
}

template<size_t BufferSize = 512, class... Args>
void println(std::FILE* stream, const char* msg, Args... args) noexcept {
    message_builder<BufferSize> mb(msg, args...);
    mb.append('\n');
    CIEL_UNUSED(std::fprintf(stream, "%s", mb.get()));
}

template<size_t BufferSize = 512, class... Args>
void println(const char* msg, Args... args) noexcept {
    println<BufferSize>(stdout, msg, args...);
}

// fatal

template<class... Args>
[[noreturn]] void fatal(Args&&... args) {
    ciel::println(stderr, std::forward<Args>(args)...);
    CIEL_UNUSED(std::fflush(nullptr));
    std::abort();
}

// exception

#ifdef CIEL_HAS_EXCEPTIONS
#  define CIEL_THROW_EXCEPTION(e) throw e
#else
#  define CIEL_THROW_EXCEPTION(e)                                                                                      \
      do {                                                                                                             \
          ciel::fatal("exception throw: {} in {} on line {}. {}", #e, CIEL_CURRENT_FILE, CIEL_CURRENT_LINE, e.what()); \
      } while (false)
#endif

// assert

#ifdef CIEL_IS_DEBUGGING
#  define CIEL_ASSERT_M(cond, fmt, ...)                                                                             \
      do {                                                                                                          \
          if (!(cond)) {                                                                                            \
              ciel::fatal("assertion fail: {} in {} on line {}. " fmt, #cond, CIEL_CURRENT_FILE, CIEL_CURRENT_LINE, \
                          ##__VA_ARGS__);                                                                           \
          }                                                                                                         \
      } while (false)
#else
#  define CIEL_ASSERT_M(cond, fmt, ...) CIEL_UNUSED(cond)
#endif

#define CIEL_ASSERT(cond) CIEL_ASSERT_M(cond, "")

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_MESSAGE_HPP_

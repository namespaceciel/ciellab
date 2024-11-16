#ifndef CIELLAB_INCLUDE_CIEL_CORE_MESSAGE_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_MESSAGE_HPP_

#include <ciel/core/config.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <utility>

NAMESPACE_CIEL_BEGIN

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

    void append(uintptr_t s) noexcept {
        const char hexdigits[] = "0123456789abcdef";

        append('0');
        append('x');

        std::array<char, 16> temp{};

        for (auto it = temp.rbegin(); it != temp.rend(); ++it) { // NOLINT(modernize-loop-convert)
            *it = hexdigits[s & 0xf];
            s >>= 4;
        }

        for (const char c : temp) {
            append(c);
        }
    }

    void append(const void* p) noexcept {
        append(reinterpret_cast<uintptr_t>(p));
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
#  define CIEL_THROW_EXCEPTION(e)                        \
      do {                                               \
          ciel::println(stderr, "exception throw: " #e); \
          ciel::fatal(e.what());                         \
      } while (false)
#endif

// assert

CIEL_DIAGNOSTIC_PUSH
CIEL_CLANG_DIAGNOSTIC_IGNORED("-Wassume")

#ifdef CIEL_IS_DEBUGGING
#  define CIEL_ASSERT(cond, msg, ...)                          \
      do {                                                     \
          if (!(cond)) {                                       \
              ciel::println(stderr, "assertion fail: " #cond); \
              ciel::fatal(msg, ##__VA_ARGS__);                 \
          }                                                    \
      } while (false)
#else
#  define CIEL_ASSERT(cond, msg, ...) CIEL_ASSUME(cond)
#endif

#define CIEL_PRECONDITION(cond)  CIEL_ASSERT(cond, "")
#define CIEL_POSTCONDITION(cond) CIEL_ASSERT(cond, "")

CIEL_DIAGNOSTIC_POP

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_MESSAGE_HPP_

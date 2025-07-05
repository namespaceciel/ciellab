#ifndef CIELLAB_INCLUDE_CIEL_CORE_MESSAGE_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_MESSAGE_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/is_range.hpp>
#include <ciel/core/to_chars.hpp>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
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

class message_builder {
private:
    std::array<char, 55> buffer_;
    uint8_t end_{0};
    std::FILE* const stream;

public:
    template<class... Args>
    message_builder(std::FILE* s, const char* msg, Args... args) noexcept
        : stream(s) {
        append(msg, args...);
    }

    message_builder(const message_builder&)            = delete;
    message_builder& operator=(const message_builder&) = delete;

    ~message_builder() {
        flush();
    }

    void flush() noexcept {
        buffer_[end_] = '\0';
        CIEL_UNUSED(std::fprintf(stream, "%s", buffer_.data()));
        end_ = 0;
    }

    void append(const char c) noexcept {
        if CIEL_UNLIKELY (end_ == buffer_.size() - 1) {
            flush();
        }

        buffer_[end_++] = c;
    }

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

    template<class T, enable_if_t<std::is_arithmetic<T>::value> = 0>
    void append(T value) noexcept {
        static_assert(!std::is_floating_point<T>::value, "not implemented yet");

        std::array<char, 21> temp{}; // Enough to hold the longest 64 bit decimal number.
        CIEL_UNUSED(ciel::to_chars(temp.data(), value));

        append(temp.data());
    }

    void append(const void* p) noexcept {
        if (p == nullptr) {
            append(nullptr);
            return;
        }

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

        append("0x");
        append(temp);
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

}; // class message_builder

// print

template<class... Args>
void print(std::FILE* stream, const char* msg, Args... args) noexcept {
    message_builder{stream, msg, args...};
}

template<class... Args>
void print(const char* msg, Args... args) noexcept {
    print(stdout, msg, args...);
}

template<class... Args>
void println(std::FILE* stream, const char* msg, Args... args) noexcept {
    message_builder{stream, msg, args...}.append('\n');
}

template<class... Args>
void println(const char* msg, Args... args) noexcept {
    println(stdout, msg, args...);
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
#  define CIEL_THROW_EXCEPTION(e)                                                                              \
      do {                                                                                                     \
          ciel::fatal("exception throw: {} in {}:{}. {}", #e, CIEL_CURRENT_FILE, CIEL_CURRENT_LINE, e.what()); \
      } while (false)
#endif

// assert

#ifdef CIEL_IS_DEBUGGING
#  define CIEL_ASSERT_M(cond, fmt, ...)                                                                     \
      do {                                                                                                  \
          if (!(cond)) {                                                                                    \
              ciel::fatal("assertion fail: {} in {}:{}. " fmt, #cond, CIEL_CURRENT_FILE, CIEL_CURRENT_LINE, \
                          ##__VA_ARGS__);                                                                   \
          }                                                                                                 \
      } while (false)
#else
#  define CIEL_ASSERT_M(cond, fmt, ...)
#endif

#define CIEL_ASSERT(cond) CIEL_ASSERT_M(cond, "")

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_MESSAGE_HPP_

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
    std::array<char, 119> buffer_;
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

    void append(const char* s) noexcept {
        for (; *s != 0; ++s) {
            append(*s);
        }
    }

    template<class Front, class... Rest>
    void append(const char* s, Front&& front, Rest&&... rest) noexcept {
        for (; *s != 0; ++s) {
            if (s[0] == '{' && s[1] == '}') {
                break;
            }

            append(*s);
        }

        append(std::forward<Front>(front));
        append(s + 2, std::forward<Rest>(rest)...);
    }

    template<class T, enable_if_t<to_chars_width<T>::value != -1> = 0>
    void append(const T& value) noexcept {
        std::array<char, to_chars_width<T>::value + 1> temp;
        char* end = ciel::to_chars(temp.data(), value);
        *end      = 0;

        append(temp.data());
    }

}; // class message_builder

// print

template<class... Args>
void print(std::FILE* stream, const char* msg, Args&&... args) noexcept {
    message_builder{stream, msg, std::forward<Args>(args)...};
}

template<class... Args>
void print(const char* msg, Args&&... args) noexcept {
    print(stdout, msg, std::forward<Args>(args)...);
}

template<class... Args>
void println(std::FILE* stream, const char* msg, Args&&... args) noexcept {
    message_builder{stream, msg, std::forward<Args>(args)...}.append('\n');
}

template<class... Args>
void println(const char* msg, Args&&... args) noexcept {
    println(stdout, msg, std::forward<Args>(args)...);
}

// fatal

template<class... Args>
[[noreturn]] void fatal(Args&&... args) noexcept {
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

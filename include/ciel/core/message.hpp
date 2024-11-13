#ifndef CIELLAB_INCLUDE_CIEL_CORE_MESSAGE_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_MESSAGE_HPP_

#include <ciel/core/config.hpp>

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

NAMESPACE_CIEL_BEGIN

template<size_t BufferSize>
class message_builder {
private:
    static_assert(BufferSize != 0, "At least one char needed to be \\0");

    std::array<char, BufferSize> buffer_{};
    size_t end_{0};

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

        CIEL_POSTCONDITION(*s != 0);

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

        for (auto it = std::array<char, 20>::reverse_iterator(temp.begin() + p); it != temp.rend(); ++it) {
            append(*it);
        }
    }

public:
    template<class... Args>
    message_builder(const char* msg, Args... args) noexcept {
        append(msg, args...);

        CIEL_POSTCONDITION(buffer_.back() == '\0');
    }

    message_builder(const message_builder&)            = delete;
    message_builder& operator=(const message_builder&) = delete;

    CIEL_NODISCARD const char* get() const noexcept {
        return buffer_.data();
    }

}; // class message_builder

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_MESSAGE_HPP_

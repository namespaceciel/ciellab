#ifndef CIELLAB_INCLUDE_CIEL_EXCEPTION_GENERATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_EXCEPTION_GENERATOR_HPP_

#include <ciel/config.hpp>
#include <ciel/exchange.hpp>
#include <ciel/is_trivially_relocatable.hpp>

#include <type_traits>

NAMESPACE_CIEL_BEGIN

enum ExceptionValidOn {
    DefaultConstructor = 1,
    CopyConstructor    = 1 << 1,
    MoveConstructor    = 1 << 2,
    CopyAssignment     = 1 << 3,
    MoveAssignment     = 1 << 4
};

#ifdef CIEL_HAS_EXCEPTIONS
// ExceptionGenerator
#define ExceptionGeneratorDefinition(ExceptionGeneratorName)                                                     \
    template<size_t ThrowOn, size_t ValidOn, bool NoexceptMove>                                                  \
    class ExceptionGeneratorName {                                                                               \
        static_assert(ValidOn < (1 << 5), "");                                                                   \
                                                                                                                 \
        static constexpr bool ValidOnDefaultConstructor = (ValidOn & DefaultConstructor) != 0;                   \
        static constexpr bool ValidOnCopyConstructor    = (ValidOn & CopyConstructor) != 0;                      \
        static constexpr bool ValidOnMoveConstructor    = (ValidOn & MoveConstructor) != 0;                      \
        static constexpr bool ValidOnCopyAssignment     = (ValidOn & CopyAssignment) != 0;                       \
        static constexpr bool ValidOnMoveAssignment     = (ValidOn & MoveAssignment) != 0;                       \
                                                                                                                 \
        static_assert(!ValidOnMoveConstructor || !NoexceptMove, "");                                             \
        static_assert(!ValidOnMoveAssignment || !NoexceptMove, "");                                              \
                                                                                                                 \
        static size_t counter;                                                                                   \
                                                                                                                 \
        size_t* ptr{nullptr};                                                                                    \
                                                                                                                 \
    public:                                                                                                      \
        static bool enabled;                                                                                     \
                                                                                                                 \
        static void                                                                                              \
        reset() noexcept {                                                                                       \
            counter = 0;                                                                                         \
        }                                                                                                        \
                                                                                                                 \
        static void                                                                                              \
        throw_exception() {                                                                                      \
            reset();                                                                                             \
            throw 0;                                                                                             \
        }                                                                                                        \
                                                                                                                 \
        ExceptionGeneratorName(size_t i = 0) {                                                                   \
            if (ValidOnDefaultConstructor && enabled) {                                                          \
                if (++counter == ThrowOn) {                                                                      \
                    throw_exception();                                                                           \
                }                                                                                                \
            }                                                                                                    \
                                                                                                                 \
            ptr = new size_t{i};                                                                                 \
        }                                                                                                        \
                                                                                                                 \
        ExceptionGeneratorName(const ExceptionGeneratorName& other) {                                            \
            if (ValidOnCopyConstructor && enabled) {                                                             \
                if (++counter == ThrowOn) {                                                                      \
                    throw_exception();                                                                           \
                }                                                                                                \
            }                                                                                                    \
                                                                                                                 \
            ptr = new size_t{static_cast<size_t>(other)};                                                        \
        }                                                                                                        \
                                                                                                                 \
        ExceptionGeneratorName(ExceptionGeneratorName&& other) noexcept(NoexceptMove) {                          \
            if (ValidOnMoveConstructor && !NoexceptMove && enabled) {                                            \
                if (++counter == ThrowOn) {                                                                      \
                    throw_exception();                                                                           \
                }                                                                                                \
            }                                                                                                    \
                                                                                                                 \
            ptr = ciel::exchange(other.ptr, nullptr);                                                            \
        }                                                                                                        \
                                                                                                                 \
        ExceptionGeneratorName&                                                                                  \
        operator=(const ExceptionGeneratorName& other) {                                                         \
            if (ValidOnCopyAssignment && enabled) {                                                              \
                if (++counter == ThrowOn) {                                                                      \
                    throw_exception();                                                                           \
                }                                                                                                \
            }                                                                                                    \
                                                                                                                 \
            delete ptr;                                                                                          \
            ptr = new size_t{static_cast<size_t>(other)};                                                        \
            return *this;                                                                                        \
        }                                                                                                        \
                                                                                                                 \
        ExceptionGeneratorName&                                                                                  \
        operator=(ExceptionGeneratorName&& other) noexcept(NoexceptMove) {                                       \
            if (ValidOnMoveAssignment && !NoexceptMove && enabled) {                                             \
                if (++counter == ThrowOn) {                                                                      \
                    throw_exception();                                                                           \
                }                                                                                                \
            }                                                                                                    \
                                                                                                                 \
            delete ptr;                                                                                          \
            ptr = ciel::exchange(other.ptr, nullptr);                                                            \
            return *this;                                                                                        \
        }                                                                                                        \
                                                                                                                 \
        ~ExceptionGeneratorName() {                                                                              \
            if (ptr) {                                                                                           \
                *ptr = -1;                                                                                       \
                delete ptr;                                                                                      \
            }                                                                                                    \
        }                                                                                                        \
                                                                                                                 \
        CIEL_NODISCARD explicit                                                                                  \
        operator size_t() const noexcept {                                                                       \
            return ptr ? *ptr : 0;                                                                               \
        }                                                                                                        \
    };                                                                                                           \
                                                                                                                 \
    template<size_t ThrowOn, size_t ValidOn, bool NoexceptMove>                                                  \
    size_t ExceptionGeneratorName<ThrowOn, ValidOn, NoexceptMove>::counter = 0;                                  \
    template<size_t ThrowOn, size_t ValidOn, bool NoexceptMove>                                                  \
    bool ExceptionGeneratorName<ThrowOn, ValidOn, NoexceptMove>::enabled = false;                                \
                                                                                                                 \
    template<size_t ThrowOn, size_t ValidOn, bool NoexceptMove>                                                  \
    CIEL_NODISCARD bool operator==(const ExceptionGeneratorName<ThrowOn, ValidOn, NoexceptMove>& lhs,            \
                                   const ExceptionGeneratorName<ThrowOn, ValidOn, NoexceptMove>& rhs) noexcept { \
        return static_cast<size_t>(lhs) == static_cast<size_t>(rhs);                                             \
    }                                                                                                            \
                                                                                                                 \
    template<size_t ThrowOn, size_t ValidOn, bool NoexceptMove>                                                  \
    CIEL_NODISCARD bool operator==(const ExceptionGeneratorName<ThrowOn, ValidOn, NoexceptMove>& lhs,            \
                                   const size_t rhs) noexcept {                                                  \
        return static_cast<size_t>(lhs) == rhs;                                                                  \
    }

ExceptionGeneratorDefinition(ExceptionGenerator);
ExceptionGeneratorDefinition(ExceptionGeneratorTriviallyRelocatable);

template<size_t ThrowOn, size_t ValidOn, bool NoexceptMove>
struct is_trivially_relocatable<ExceptionGeneratorTriviallyRelocatable<ThrowOn, ValidOn, NoexceptMove>>
    : std::true_type {};

#endif // CIEL_HAS_EXCEPTIONS

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_EXCEPTION_GENERATOR_HPP_

#ifndef CIELLAB_INCLUDE_CIEL_TEST_EXCEPTION_GENERATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_TEST_EXCEPTION_GENERATOR_HPP_

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
template<size_t ThrowOn, size_t ValidOn, bool IsNothrowMovable, bool IsTriviallyRelocatable>
class ExceptionGeneratorName {
    static_assert(ValidOn < (1 << 5), "");

    static constexpr bool ValidOnDefaultConstructor = (ValidOn & DefaultConstructor) != 0;
    static constexpr bool ValidOnCopyConstructor    = (ValidOn & CopyConstructor) != 0;
    static constexpr bool ValidOnMoveConstructor    = (ValidOn & MoveConstructor) != 0;
    static constexpr bool ValidOnCopyAssignment     = (ValidOn & CopyAssignment) != 0;
    static constexpr bool ValidOnMoveAssignment     = (ValidOn & MoveAssignment) != 0;

    static_assert(!ValidOnMoveConstructor || !IsNothrowMovable, "");
    static_assert(!ValidOnMoveAssignment || !IsNothrowMovable, "");

    static size_t counter;

    size_t* ptr{nullptr};

public:
    static bool enabled;

    static void reset() noexcept {
        counter = 0;
    }

    static void throw_exception() {
        reset();
        throw 0;
    }

    ExceptionGeneratorName(size_t i = 0) {
        if (ValidOnDefaultConstructor && enabled) {
            if (++counter == ThrowOn) {
                throw_exception();
            }
        }

        ptr = new size_t{i};
    }

    ExceptionGeneratorName(const ExceptionGeneratorName& other) {
        if (ValidOnCopyConstructor && enabled) {
            if (++counter == ThrowOn) {
                throw_exception();
            }
        }

        ptr = new size_t{static_cast<size_t>(other)};
    }

    ExceptionGeneratorName(ExceptionGeneratorName&& other) noexcept(IsNothrowMovable) {
        if (ValidOnMoveConstructor && !IsNothrowMovable && enabled) {
            if (++counter == ThrowOn) {
                throw_exception();
            }
        }

        ptr = ciel::exchange(other.ptr, nullptr);
    }

    ExceptionGeneratorName& operator=(const ExceptionGeneratorName& other) {
        if (ValidOnCopyAssignment && enabled) {
            if (++counter == ThrowOn) {
                throw_exception();
            }
        }

        delete ptr;
        ptr = new size_t{static_cast<size_t>(other)};
        return *this;
    }

    ExceptionGeneratorName& operator=(ExceptionGeneratorName&& other) noexcept(IsNothrowMovable) {
        if (ValidOnMoveAssignment && !IsNothrowMovable && enabled) {
            if (++counter == ThrowOn) {
                throw_exception();
            }
        }

        delete ptr;
        ptr = ciel::exchange(other.ptr, nullptr);
        return *this;
    }

    ~ExceptionGeneratorName() {
        if (ptr) {
            *ptr = -1;
            delete ptr;
        }
    }

    CIEL_NODISCARD explicit operator size_t() const noexcept {
        return ptr ? *ptr : 0;
    }
};

template<size_t ThrowOn, size_t ValidOn, bool IsNothrowMovable, bool IsTriviallyRelocatable>
size_t ExceptionGeneratorName<ThrowOn, ValidOn, IsNothrowMovable, IsTriviallyRelocatable>::counter = 0;
template<size_t ThrowOn, size_t ValidOn, bool IsNothrowMovable, bool IsTriviallyRelocatable>
bool ExceptionGeneratorName<ThrowOn, ValidOn, IsNothrowMovable, IsTriviallyRelocatable>::enabled = false;

template<size_t ThrowOn, size_t ValidOn, bool IsNothrowMovable, bool IsTriviallyRelocatable>
CIEL_NODISCARD bool
operator==(const ExceptionGeneratorName<ThrowOn, ValidOn, IsNothrowMovable, IsTriviallyRelocatable>& lhs,
           const ExceptionGeneratorName<ThrowOn, ValidOn, IsNothrowMovable, IsTriviallyRelocatable>& rhs) noexcept {
    return static_cast<size_t>(lhs) == static_cast<size_t>(rhs);
}

template<size_t ThrowOn, size_t ValidOn, bool IsNothrowMovable, bool IsTriviallyRelocatable>
CIEL_NODISCARD bool
operator==(const ExceptionGeneratorName<ThrowOn, ValidOn, IsNothrowMovable, IsTriviallyRelocatable>& lhs,
           const size_t rhs) noexcept {
    return static_cast<size_t>(lhs) == rhs;
}

template<size_t ThrowOn, size_t ValidOn, bool IsNothrowMovable>
using ExceptionGenerator = ExceptionGeneratorName<ThrowOn, ValidOn, IsNothrowMovable, false>;
template<size_t ThrowOn, size_t ValidOn, bool IsNothrowMovable>
using ExceptionGeneratorTriviallyRelocatable = ExceptionGeneratorName<ThrowOn, ValidOn, IsNothrowMovable, true>;

template<size_t ThrowOn, size_t ValidOn, bool IsNothrowMovable>
struct is_trivially_relocatable<ExceptionGeneratorTriviallyRelocatable<ThrowOn, ValidOn, IsNothrowMovable>>
    : std::true_type {};

#endif

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TEST_EXCEPTION_GENERATOR_HPP_

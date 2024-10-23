#ifndef CIELLAB_INCLUDE_CIEL_INT_WRAPPER_HPP_
#define CIELLAB_INCLUDE_CIEL_INT_WRAPPER_HPP_

#include <ciel/config.hpp>
#include <ciel/exchange.hpp>
#include <ciel/is_trivially_relocatable.hpp>

#include <cstddef>

NAMESPACE_CIEL_BEGIN

#define IntWrapperDefinition(TypeName, IsNothrowMovable)         \
    struct TypeName {                                            \
    private:                                                     \
        int i_;                                                  \
                                                                 \
    public:                                                      \
        TypeName(const int i = 0) noexcept                       \
            : i_(i) {}                                           \
                                                                 \
        TypeName(const TypeName&) noexcept = default;            \
        TypeName&                                                \
        operator=(const TypeName&) noexcept                      \
            = default;                                           \
                                                                 \
        TypeName(TypeName&& other) noexcept(IsNothrowMovable)    \
            : i_(ciel::exchange(other.i_, -1)) {}                \
                                                                 \
        TypeName&                                                \
        operator=(TypeName&& other) noexcept(IsNothrowMovable) { \
            i_ = ciel::exchange(other.i_, -1);                   \
            return *this;                                        \
        }                                                        \
                                                                 \
        TypeName&                                                \
        operator++() noexcept {                                  \
            ++i_;                                                \
            return *this;                                        \
        }                                                        \
                                                                 \
        CIEL_NODISCARD TypeName                                  \
        operator++(int) noexcept {                               \
            auto res = *this;                                    \
            ++(*this);                                           \
            return res;                                          \
        }                                                        \
                                                                 \
        TypeName&                                                \
        operator--() noexcept {                                  \
            --i_;                                                \
            return *this;                                        \
        }                                                        \
                                                                 \
        CIEL_NODISCARD TypeName                                  \
        operator--(int) noexcept {                               \
            auto res = *this;                                    \
            --(*this);                                           \
            return res;                                          \
        }                                                        \
                                                                 \
        TypeName&                                                \
        operator+=(const TypeName other) noexcept {              \
            i_ += other.i_;                                      \
            return *this;                                        \
        }                                                        \
                                                                 \
        TypeName&                                                \
        operator-=(const TypeName other) noexcept {              \
            return (*this) += (-other);                          \
        }                                                        \
                                                                 \
        CIEL_NODISCARD TypeName                                  \
        operator-() noexcept {                                   \
            TypeName res(-i_);                                   \
            return res;                                          \
        }                                                        \
                                                                 \
        CIEL_NODISCARD                                           \
        operator int() const noexcept {                          \
            return i_;                                           \
        }                                                        \
                                                                 \
        CIEL_NODISCARD friend TypeName                           \
        operator+(TypeName lhs, const TypeName rhs) noexcept {   \
            lhs.i_ += rhs.i_;                                    \
            return lhs;                                          \
        }                                                        \
                                                                 \
        CIEL_NODISCARD friend TypeName                           \
        operator-(TypeName lhs, const TypeName rhs) noexcept {   \
            lhs.i_ -= rhs.i_;                                    \
            return lhs;                                          \
        }                                                        \
    }

IntWrapperDefinition(Int, true);
IntWrapperDefinition(TRInt, true);
IntWrapperDefinition(TMInt, false);

template<>
struct is_trivially_relocatable<Int> : std::false_type {};

template<>
struct is_trivially_relocatable<TMInt> : std::false_type {};

template<>
struct is_trivially_relocatable<TRInt> : std::true_type {};

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_INT_WRAPPER_HPP_

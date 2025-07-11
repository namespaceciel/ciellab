#ifndef CIELLAB_INCLUDE_CIEL_CORE_ARRAY_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_ARRAY_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/message.hpp>

#include <array>
#include <cstddef>

NAMESPACE_CIEL_BEGIN

template<class T, size_t Begin, size_t End>
class array {
private:
    std::array<T, End - Begin> arr_;

public:
    T& operator[](size_t index) noexcept {
        CIEL_ASSERT_M(Begin <= index && index < End, "Call ciel::array<T, {}, {}>::operator[] at index: {}", Begin, End,
                      index);

        return arr_[index - Begin];
    }

    const T& operator[](size_t index) const noexcept {
        CIEL_ASSERT_M(Begin <= index && index < End, "Call ciel::array<T, {}, {}>::operator[] at index: {}", Begin, End,
                      index);

        return arr_[index - Begin];
    }

    T* begin() noexcept {
        return arr_.data();
    }

    const T* begin() const noexcept {
        return arr_.data();
    }

    T* end() noexcept {
        return arr_.data() + arr_.size();
    }

    const T* end() const noexcept {
        return arr_.data() + arr_.size();
    }

}; // class array

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_ARRAY_HPP_

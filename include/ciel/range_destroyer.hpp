#ifndef CIELLAB_INCLUDE_CIEL_RANGE_DESTROYER_HPP_
#define CIELLAB_INCLUDE_CIEL_RANGE_DESTROYER_HPP_

#include <memory>
#include <type_traits>

#include <ciel/compressed_pair.hpp>
#include <ciel/config.hpp>

NAMESPACE_CIEL_BEGIN

// Destroy ranges in destructor for exception handling.
// Note that Allocator can be reference type.
template<class T, class Allocator, class = void>
class range_destroyer {
    static_assert(!std::is_rvalue_reference<Allocator>::value, "");

private:
    using allocator_type = typename std::remove_reference<Allocator>::type;
    using pointer        = typename std::allocator_traits<allocator_type>::pointer;
    using alloc_traits   = std::allocator_traits<allocator_type>;

    static_assert(std::is_same<typename allocator_type::value_type, T>::value, "");

    pointer begin_;
    compressed_pair<pointer, Allocator> end_alloc_;

    CIEL_NODISCARD pointer&
    end_() noexcept {
        return end_alloc_.first();
    }

    allocator_type&
    allocator_() noexcept {
        return end_alloc_.second();
    }

public:
    range_destroyer(pointer begin, pointer end, allocator_type& alloc) noexcept
        : begin_{begin}, end_alloc_{end, alloc} {}

    range_destroyer(pointer begin, pointer end, const allocator_type& alloc) noexcept
        : begin_{begin}, end_alloc_{end, alloc} {}

    range_destroyer(const range_destroyer&) = delete;
    // clang-format off
    range_destroyer& operator=(const range_destroyer&) = delete;
    // clang-format on

    ~range_destroyer() {
        CIEL_PRECONDITION(begin_ <= end_());

        while (end_() != begin_) {
            alloc_traits::destroy(allocator_(), --end_());
        }
    }

    void
    advance_forward() noexcept {
        ++end_();
    }

    void
    advance_backward() noexcept {
        --begin_;
    }

    void
    release() noexcept {
        end_() = begin_;
    }

}; // class range_destroyer

template<class T, class Allocator>
class range_destroyer<T, Allocator,
                      void_t<typename std::enable_if<std::is_trivially_destructible<T>::value, int>::type>> {
    static_assert(!std::is_rvalue_reference<Allocator>::value, "");

private:
    using allocator_type = typename std::remove_reference<Allocator>::type;
    using pointer        = typename std::allocator_traits<allocator_type>::pointer;

    static_assert(std::is_same<typename allocator_type::value_type, T>::value, "");

public:
    range_destroyer(pointer, pointer, const allocator_type&) noexcept {}

    range_destroyer(const range_destroyer&) = delete;
    // clang-format off
    range_destroyer& operator=(const range_destroyer&) = delete;
    // clang-format on

    void
    advance_forward() noexcept {}

    void
    advance_backward() noexcept {}

    void
    release() noexcept {}

}; // class range_destroyer

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_RANGE_DESTROYER_HPP_

#ifndef CIELLAB_INCLUDE_CIEL_RANGE_DESTROYER_HPP_
#define CIELLAB_INCLUDE_CIEL_RANGE_DESTROYER_HPP_

#include <ciel/allocator_traits.hpp>
#include <ciel/core/compressed_pair.hpp>
#include <ciel/core/config.hpp>
#include <ciel/core/message.hpp>
#include <ciel/to_address.hpp>

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

NAMESPACE_CIEL_BEGIN

template<class...>
class range_destroyer;

// Destroy ranges in destructor for exception handling.
// Note that Allocator should be reference type.
template<class T, class Allocator>
class range_destroyer<T, Allocator> {
    static_assert(std::is_lvalue_reference<Allocator>::value, "");

private:
    using allocator_type = remove_reference_t<Allocator>;
    using pointer        = typename std::allocator_traits<allocator_type>::pointer;
    using alloc_traits   = std::allocator_traits<allocator_type>;

    static_assert(std::is_same<typename allocator_type::value_type, T>::value, "");

    pointer begin_;
    pointer end_;
    Allocator allocator_;

public:
    range_destroyer(pointer begin, pointer end, allocator_type& alloc) noexcept
        : begin_{begin}, end_{end}, allocator_{alloc} {}

    range_destroyer(const range_destroyer&)            = delete;
    range_destroyer& operator=(const range_destroyer&) = delete;

    ~range_destroyer() {
        CIEL_ASSERT(begin_ <= end_);

        for (; begin_ != end_; ++begin_) {
            alloc_traits::destroy(allocator_, ciel::to_address(begin_));
        }
    }

    void advance_forward() noexcept {
        ++end_;
    }

    void advance_forward(const ptrdiff_t n) noexcept {
        end_ += n;
    }

    void advance_backward() noexcept {
        --begin_;
    }

    void advance_backward(const ptrdiff_t n) noexcept {
        begin_ -= n;
    }

    void release() noexcept {
        end_ = begin_;
    }

}; // class range_destroyer

template<class T>
class range_destroyer<T> {
private:
    using value_type = T;
    using pointer    = T*;

    pointer begin_;
    pointer end_;

public:
    range_destroyer(pointer begin, pointer end) noexcept
        : begin_{begin}, end_{end} {}

    range_destroyer(const range_destroyer&)            = delete;
    range_destroyer& operator=(const range_destroyer&) = delete;

    ~range_destroyer() {
        CIEL_ASSERT(begin_ <= end_);

        for (; begin_ != end_; ++begin_) {
            begin_->~value_type();
        }
    }

    void advance_forward() noexcept {
        ++end_;
    }

    void advance_forward(const ptrdiff_t n) noexcept {
        end_ += n;
    }

    void advance_backward() noexcept {
        --begin_;
    }

    void advance_backward(const ptrdiff_t n) noexcept {
        begin_ -= n;
    }

    void release() noexcept {
        end_ = begin_;
    }

}; // class range_destroyer

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_RANGE_DESTROYER_HPP_

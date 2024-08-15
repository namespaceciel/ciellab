#ifndef CIELLAB_INCLUDE_CIEL_RANGE_DESTROYER_HPP_
#define CIELLAB_INCLUDE_CIEL_RANGE_DESTROYER_HPP_

#include <memory>
#include <type_traits>

#include <ciel/config.hpp>

NAMESPACE_CIEL_BEGIN

// Destroy ranges in destructor for exception handling.

template<class T, class Allocator, class = void>
class range_destroyer : private Allocator {
    static_assert(std::is_same<typename Allocator::value_type, T>::value, "");

private:
    using allocator_type = Allocator;
    using pointer        = typename std::allocator_traits<allocator_type>::pointer;
    using alloc_traits   = std::allocator_traits<allocator_type>;

    pointer begin_;
    pointer end_;

    allocator_type&
    allocator_() noexcept {
        return static_cast<allocator_type&>(*this);
    }

public:
    range_destroyer(pointer begin, pointer end, const allocator_type& alloc) noexcept
        : allocator_type{alloc}, begin_{begin}, end_{end} {}

    range_destroyer(const range_destroyer&) = delete;
    range_destroyer&
    operator=(const range_destroyer&)
        = delete;

    ~range_destroyer() {
        CIEL_PRECONDITION(begin_ <= end_);

        while (end_ != begin_) {
            alloc_traits::destroy(allocator_(), --end_);
        }
    }

    void
    release() noexcept {
        end_ = begin_;
    }

}; // class range_destroyer

template<class T, class Allocator>
class range_destroyer<T, Allocator,
                      void_t<typename std::enable_if<std::is_trivially_destructible<T>::value, int>::type>> {
public:
    range_destroyer(...) noexcept {}

    range_destroyer(const range_destroyer&) = delete;
    range_destroyer&
    operator=(const range_destroyer&)
        = delete;

    void
    release() noexcept {}

}; // class range_destroyer

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_RANGE_DESTROYER_HPP_

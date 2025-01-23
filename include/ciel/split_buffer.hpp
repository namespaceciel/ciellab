#ifndef CIELLAB_INCLUDE_CIEL_SPLIT_BUFFER_HPP_
#define CIELLAB_INCLUDE_CIEL_SPLIT_BUFFER_HPP_

#include <ciel/copy_n.hpp>
#include <ciel/core/config.hpp>
#include <ciel/core/message.hpp>
#include <ciel/to_address.hpp>

#include <memory>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

// Inspired by LLVM libc++'s implementation.

template<class, class>
class vector;

template<class T, class Allocator>
class split_buffer {
    static_assert(std::is_lvalue_reference<Allocator>::value,
                  "Allocator should be lvalue reference type as being used by vector.");

public:
    using value_type      = T;
    using allocator_type  = remove_reference_t<Allocator>;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = typename std::allocator_traits<allocator_type>::pointer;
    using const_pointer   = typename std::allocator_traits<allocator_type>::const_pointer;

private:
    using alloc_traits = std::allocator_traits<allocator_type>;

    pointer begin_cap_{nullptr};
    pointer begin_{nullptr};
    pointer end_{nullptr};
    pointer end_cap_{nullptr};
    Allocator allocator_;

    friend class vector<value_type, allocator_type>;

    template<class... Args>
    void construct(pointer p, Args&&... args) {
        alloc_traits::construct(allocator_, ciel::to_address(p), std::forward<Args>(args)...);
    }

    void construct_at_end(const size_type n, const value_type& value) {
        CIEL_ASSERT(end_ + n <= end_cap_);

        for (size_type i = 0; i < n; ++i) {
            unchecked_emplace_back(value);
        }
    }

    template<class Iter>
    void construct_at_end(Iter first, Iter last) {
        ciel::uninitialized_copy(allocator_, first, last, end_);
    }

    template<class... Args>
    void unchecked_emplace_front(Args&&... args) {
        CIEL_ASSERT(begin_cap_ < begin_);

        construct(begin_ - 1, std::forward<Args>(args)...);
        --begin_;
    }

    template<class... Args>
    void unchecked_emplace_back(Args&&... args) {
        CIEL_ASSERT(end_ < end_cap_);

        construct(end_, std::forward<Args>(args)...);
        ++end_;
    }

public:
    explicit split_buffer(Allocator alloc, const size_type cap, const size_type offset)
        : allocator_(alloc) {
        CIEL_ASSERT(cap != 0);
        CIEL_ASSERT(cap >= offset);

        begin_cap_ = alloc_traits::allocate(allocator_, cap);
        end_cap_   = begin_cap_ + cap;
        begin_     = begin_cap_ + offset;
        end_       = begin_;
    }

    split_buffer(const split_buffer& other)            = delete;
    split_buffer& operator=(const split_buffer& other) = delete;

    ~split_buffer() {
        if (begin_cap_) {
            clear();
            alloc_traits::deallocate(allocator_, begin_cap_, capacity());
        }
    }

    CIEL_NODISCARD size_type front_spare() const noexcept {
        CIEL_ASSERT(begin_cap_ <= begin_);

        return begin_ - begin_cap_;
    }

    CIEL_NODISCARD size_type back_spare() const noexcept {
        CIEL_ASSERT(end_ <= end_cap_);

        return end_cap_ - end_;
    }

    CIEL_NODISCARD size_type capacity() const noexcept {
        CIEL_ASSERT(begin_cap_ < end_cap_); // capacity should not be zero.

        return end_cap_ - begin_cap_;
    }

    void clear() noexcept {
        CIEL_ASSERT(begin_ <= end_);

        for (; begin_ != end_; ++begin_) {
            alloc_traits::destroy(allocator_, ciel::to_address(begin_));
        }
    }

}; // class split_buffer

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_SPLIT_BUFFER_HPP_

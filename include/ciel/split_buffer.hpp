#ifndef CIELLAB_INCLUDE_CIEL_SPLIT_BUFFER_HPP_
#define CIELLAB_INCLUDE_CIEL_SPLIT_BUFFER_HPP_

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include <ciel/config.hpp>
#include <ciel/type_traits.hpp>

NAMESPACE_CIEL_BEGIN

// This class is used in std::vector implementation and it's like a double-ended vector.
// When std::vector inserts beyond it's capacity, it defines a temp split_buffer to store insertions
// and push vector's elements into two sides, and swap out at last,
// so that it can keep basic exception safety.
// We complete its functionality so that it can be used as a normal container.
// When pushing elements and there is no space this side, we try to shift to other side if there is plenty of space,
// or just expand.
// When it comes to expansion, we try to move old elements to the middle of new space
// and leave some free space at both sides.
// Self assignments like v.emplace_back(v[0]) is not supported so that we can simplify the code.

// TODO: insert is not implemented yet.

template<class, class>
class vector;
template<class, size_t, class>
class small_vector;

template<class T, class Allocator = std::allocator<T>>
class split_buffer : public Allocator {

    static_assert(std::is_same<typename Allocator::value_type, T>::value, "");

public:
    using value_type             = T;
    using allocator_type         = Allocator;
    using size_type              = size_t;
    using difference_type        = ptrdiff_t;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using pointer                = typename std::allocator_traits<allocator_type>::pointer;
    using const_pointer          = typename std::allocator_traits<allocator_type>::const_pointer;

    using iterator               = pointer;
    using const_iterator         = const_pointer;

    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    using alloc_traits           = std::allocator_traits<allocator_type>;

    struct new_allocation {
        owner<pointer> new_begin_cap_;
        size_type new_cap_;

    };  // struct new_allocation

    owner<pointer> begin_cap_;
    pointer begin_;
    pointer end_;
    pointer end_cap_;

    friend class vector<value_type, allocator_type>;

    template<class, size_t, class>
    friend class small_vector;

    auto reserve_cap_and_offset_to(const size_type cap, const size_type offset) -> void {
        begin_cap_ = alloc_traits::allocate(allocator_(), cap);
        end_cap_ = begin_cap_ + cap;
        begin_ = begin_cap_ + offset;
        end_ = begin_;
    }

    auto allocator_() noexcept -> allocator_type& {
        return static_cast<allocator_type&>(*this);
    }

    auto allocator_() const noexcept -> const allocator_type& {
        return static_cast<const allocator_type&>(*this);
    }

    auto recommend_cap(const size_type new_size) const -> size_type {
        CIEL_PRECONDITION(new_size > 0);

        const size_type ms = max_size();

        if (new_size > ms) CIEL_UNLIKELY {
            THROW(std::length_error("ciel::split_buffer reserving size is beyond max_size"));
        }

        const size_type cap = capacity();

        if (cap >= ms / 2) CIEL_UNLIKELY {
            return ms;
        }

        return std::max(2 * cap, new_size);
    }

    template<class... Args>
    auto construct_one_at_end(Args&& ... args) -> void {
        CIEL_PRECONDITION(end_ < end_cap_);

        alloc_traits::construct(allocator_(), end_, std::forward<Args>(args)...);
        ++end_;
    }

    auto construct_at_end(const size_type n) -> void {
        CIEL_PRECONDITION(end_ + n <= end_cap_);

        for (size_type i = 0; i < n; ++i) {
            alloc_traits::construct(allocator_(), end_);
            ++end_;
        }
    }

    auto construct_at_end(const size_type n, const value_type& value) -> void {
        CIEL_PRECONDITION(end_ + n <= end_cap_);

        for (size_type i = 0; i < n; ++i) {
            alloc_traits::construct(allocator_(), end_, value);
            ++end_;
        }
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    auto construct_at_end(Iter first, Iter last) -> void {
        CIEL_PRECONDITION(end_ + std::distance(first, last) <= end_cap_);

        while (first != last) {
            alloc_traits::construct(allocator_(), end_, *first);
            ++first;
            ++end_;
        }
    }

    template<class... Args>
    auto construct_one_at_begin(Args&& ... args) -> void {
        CIEL_PRECONDITION(begin_cap_ < begin_);

        alloc_traits::construct(allocator_(), begin_ - 1, std::forward<Args>(args)...);
        --begin_;
    }

    // std::is_trivially_destructible<value_type> -> std::true_type
    auto alloc_range_destroy(pointer begin, pointer end, std::true_type) noexcept -> pointer {
        CIEL_PRECONDITION(begin <= end);

        return begin;
    }

    // std::is_trivially_destructible<value_type> -> std::false_type
    auto alloc_range_destroy(pointer begin, pointer end, std::false_type) noexcept -> pointer {
        CIEL_PRECONDITION(begin <= end);

        while (end != begin) {
            alloc_traits::destroy(allocator_(), --end);
        }
        return begin;
    }

    // is_trivially_relocatable -> std::true_type
    auto swap_out_buffer(split_buffer& sb, pointer pos, std::true_type) noexcept -> void {
        // If either dest or src is an invalid or null pointer, the behavior is undefined, even if count is zero.
        if (begin_cap_) {
            const size_type front_count = pos - begin_;
            const size_type back_count = end_ - pos;

            CIEL_PRECONDITION(sb.front_spare() >= front_count);
            CIEL_PRECONDITION(sb.back_spare() >= back_count);

            sb.begin_ -= front_count;
            memcpy(sb.begin_, begin_, sizeof(value_type) / sizeof(unsigned char) * front_count);

            memcpy(sb.end_, pos, sizeof(value_type) / sizeof(unsigned char) * back_count);
            sb.end_ += back_count;

            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
        }

        begin_cap_ = sb.begin_cap_;
        begin_ = sb.begin_;
        end_ = sb.end_;
        end_cap_ = sb.end_cap_;

        sb.set_nullptr();
    }

    // is_trivially_relocatable -> std::false_type
    auto swap_out_buffer(split_buffer& sb, pointer pos, std::false_type)
            noexcept(std::is_nothrow_move_constructible<value_type>::value) -> void {
        if (begin_cap_) {
            const size_type front_count = pos - begin_;
            const size_type back_count = end_ - pos;

            CIEL_PRECONDITION(sb.front_spare() >= front_count);
            CIEL_PRECONDITION(sb.back_spare() >= back_count);

            for (pointer p = pos - 1; p >= begin_; --p) {
                sb.emplace_front(std::move(*p));
            }

            for (pointer p = pos; p < end_; ++p) {
                sb.emplace_back(std::move(*p));
            }

            clear();
            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
        }

        begin_cap_ = sb.begin_cap_;
        begin_ = sb.begin_;
        end_ = sb.end_;
        end_cap_ = sb.end_cap_;

        sb.set_nullptr();
    }

    // is_trivially_relocatable -> std::true_type
    auto swap_out_buffer(split_buffer& sb, std::true_type) noexcept -> void {
        CIEL_PRECONDITION(sb.front_spare() >= size());

        // If either dest or src is an invalid or null pointer, the behavior is undefined, even if count is zero.
        if (begin_cap_) {
            sb.begin_ -= size();
            memcpy(sb.begin_, begin_, sizeof(value_type) / sizeof(unsigned char) * size());

            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
        }

        begin_cap_ = sb.begin_cap_;
        begin_ = sb.begin_;
        end_ = sb.end_;
        end_cap_ = sb.end_cap_;

        sb.set_nullptr();
    }

    // is_trivially_relocatable -> std::false_type
    auto swap_out_buffer(split_buffer& sb, std::false_type)
            noexcept(std::is_nothrow_move_constructible<value_type>::value) -> void {
        CIEL_PRECONDITION(sb.front_spare() >= size());

        if (begin_cap_) {
            for (pointer p = end_ - 1; p >= begin_; --p) {
                sb.construct_one_at_begin(std::move(*p));
            }

            clear();
            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
        }

        begin_cap_ = sb.begin_cap_;
        begin_ = sb.begin_;
        end_ = sb.end_;
        end_cap_ = sb.end_cap_;

        sb.set_nullptr();
    }

    auto swap_out_buffer(split_buffer& sb) noexcept -> void {
        do_destroy();

        CIEL_PRECONDITION(sb.begin_cap_ == sb.begin_);  // TODO: maybe useless

        begin_cap_ = sb.begin_cap_;
        begin_ = sb.begin_;
        end_ = sb.end_;
        end_cap_ = sb.end_cap_;

        sb.set_nullptr();
    }

    auto front_spare() const noexcept -> size_type {
        const difference_type res = std::distance(begin_cap_, begin_);

        CIEL_PRECONDITION(res >= 0);

        return res;
    }

    auto back_spare() const noexcept -> size_type {
        const difference_type res = std::distance(end_, end_cap_);

        CIEL_PRECONDITION(res >= 0);

        return res;
    }

    // Note that this will invalidate iterators
    auto left_shift_n(const size_type n) noexcept -> void {
        CIEL_PRECONDITION(front_spare() >= n);

        const size_type old_size = size();

        pointer new_begin = begin_ - n;
        pointer new_end = new_begin;

        if (old_size >= n) {    // n placement new, size - n move assign, n destroy

            //          ----------
            //
            //  ----------
            //  |      | |       |
            // placement new
            //      move assign
            //            destroy

            size_type i = 0;
            for (; i < n; ++i) {
                alloc_traits::construct(allocator_(), new_end, std::move(*(begin_ + i)));
                ++new_end;
            }

            for (; i < old_size; ++i) {
                *new_end = std::move(*(begin_ + i));
                ++new_end;
            }

            end_ = alloc_range_destroy(new_end, end_, std::is_trivially_destructible<value_type>{});
            begin_ = new_begin;

        } else {    // size placement new, size destroy

            //                ----------
            //
            //  ----------
            //  |        |    |        |
            // placement new
            //                 destroy

            for (size_type i = 0; i < old_size; ++i) {
                alloc_traits::construct(allocator_(), new_end, std::move(*(begin_ + i)));
                ++new_end;
            }

            alloc_range_destroy(begin_, end_, std::is_trivially_destructible<value_type>{});
            begin_ = new_begin;
            end_ = new_end;
        }
    }

    // Note that this will invalidate iterators
    auto right_shift_n(const size_type n) noexcept -> void {
        CIEL_PRECONDITION(back_spare() >= n);

        const size_type old_size = size();

        pointer new_end = end_ + n;;
        pointer new_begin = new_end;

        if (old_size >= n) {    // n placement new, size - n move assign, n destroy

            //  ----------
            //
            //          ----------
            //  |       | |      |
            //         placement new
            //      move assign
            //  destroy

            size_type i = 1;
            for (; i <= n; ++i) {
                alloc_traits::construct(allocator_(), --new_begin, std::move(*(end_ - i)));
            }

            for (; i <= old_size; ++i) {
                *(--new_begin) = std::move(*(end_ - i));
            }

            alloc_range_destroy(begin_, new_begin, std::is_trivially_destructible<value_type>{});
            begin_ = new_begin;
            end_ = new_end;

        } else {    // size placement new, size destroy

            //  ----------
            //
            //                ----------
            //  |        |    |        |
            //               placement new
            //   destroy

            for (size_type i = 1; i <= old_size; ++i) {
                alloc_traits::construct(allocator_(), --new_begin, std::move(*(end_ - i)));
            }

            alloc_range_destroy(begin_, end_, std::is_trivially_destructible<value_type>{});
            begin_ = new_begin;
            end_ = new_end;
        }
    }

    auto do_destroy() noexcept -> void {
        if (begin_cap_) {
            clear();
            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
        }
    }

    auto move_range(pointer __from_s, pointer __from_e, pointer __to) -> void {
        pointer old_end_ = end_;
        difference_type __n = old_end_ - __to;
        {
            pointer p = __from_s + __n;

            for (; p < __from_e; ++p) {
                construct_one_at_end(std::move(*p));
            }
        }
        std::move_backward(__from_s, __from_s + __n, old_end_);
    }

    auto set_nullptr() noexcept -> void {
        begin_cap_ = nullptr;
        begin_ = nullptr;
        end_ = nullptr;
        end_cap_ = nullptr;
    }

public:
    split_buffer() noexcept(noexcept(allocator_type()))
        : allocator_type(), begin_cap_(nullptr), begin_(nullptr), end_(nullptr), end_cap_(nullptr) {}

    explicit split_buffer(const allocator_type& alloc) noexcept
        : allocator_type(alloc), begin_cap_(nullptr), begin_(nullptr), end_(nullptr), end_cap_(nullptr) {}

    split_buffer(const size_type count, const value_type& value, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {

        if (count > 0) CIEL_LIKELY {
            begin_cap_ = alloc_traits::allocate(allocator_(), count);
            end_cap_ = begin_cap_ + count;
            begin_ = begin_cap_;
            end_ = begin_;

            CIEL_TRY {
                construct_at_end(count, value);

            } CIEL_CATCH (...) {
                clear();
                alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
                CIEL_THROW;
            }
        }
    }

    explicit split_buffer(const size_type count, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {

        if (count > 0) CIEL_LIKELY {
            begin_cap_ = alloc_traits::allocate(allocator_(), count);
            end_cap_ = begin_cap_ + count;
            begin_ = begin_cap_;
            end_ = begin_;

            CIEL_TRY {
                construct_at_end(count);

            } CIEL_CATCH (...) {
                clear();
                alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
                CIEL_THROW;
            }
        }
    }

    template<class Iter, typename std::enable_if<is_exactly_input_iterator<Iter>::value, int>::type = 0>
    split_buffer(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        CIEL_TRY {
            while (first != last) {
                emplace_back(*first);
                ++first;
            }

        } CIEL_CATCH (...) {
            do_destroy();
            CIEL_THROW;
        }
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    split_buffer(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        const auto count = std::distance(first, last);

        if (count > 0) CIEL_LIKELY {
            begin_cap_ = alloc_traits::allocate(allocator_(), count);
            end_cap_ = begin_cap_ + count;
            begin_ = begin_cap_;
            end_ = begin_;

            CIEL_TRY {
                construct_at_end(first, last);

            } CIEL_CATCH (...) {
                clear();
                alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
                CIEL_THROW;
            }
        }
    }

    split_buffer(const split_buffer& other)
        : split_buffer(other.begin(), other.end(),
                       alloc_traits::select_on_container_copy_construction(other.get_allocator())) {}

    split_buffer(const split_buffer& other, const allocator_type& alloc)
        : split_buffer(other.begin(), other.end(), alloc) {}

    split_buffer(split_buffer&& other) noexcept
        : allocator_type(std::move(other.allocator_())),
          begin_cap_(other.begin_cap_), begin_(other.begin_), end_(other.end_), end_cap_(other.end_cap_) {

        other.set_nullptr();
    }

    split_buffer(split_buffer&& other, const allocator_type& alloc) {
        if (alloc == other.get_allocator()) {
            allocator_() = alloc;
            begin_cap_ = other.begin_cap_;
            begin_ = other.begin_;
            end_ = other.end_;
            end_cap_ = other.end_cap_;

            other.set_nullptr();

        } else {
            split_buffer(other, alloc).swap(*this);
        }
    }

    split_buffer(std::initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
        : split_buffer(init.begin(), init.end(), alloc) {}

    ~split_buffer() {
        do_destroy();
    }

    auto operator=(const split_buffer& other) -> split_buffer& {
        if (this == std::addressof(other)) CIEL_UNLIKELY {
            return *this;
        }

        if (alloc_traits::propagate_on_container_copy_assignment::value) {
            if (allocator_() != other.allocator_()) {
                split_buffer(other.allocator_()).swap(*this);
                assign(other.begin(), other.end());
                return *this;
            }

            allocator_() = other.allocator_();
        }

        assign(other.begin(), other.end());

        CIEL_POSTCONDITION(*this == other);
        return *this;
    }

    auto operator=(split_buffer&& other)
        noexcept(alloc_traits::propagate_on_container_move_assignment::value ||
                 alloc_traits::is_always_equal::value) -> split_buffer& {
        if (this == std::addressof(other)) CIEL_UNLIKELY {
            return *this;
        }

        if (!alloc_traits::propagate_on_container_move_assignment::value && allocator_() != other.allocator_()) {
            assign(other.begin(), other.end());
            return *this;
        }

        if (alloc_traits::propagate_on_container_move_assignment::value) {
            allocator_() = std::move(other.allocator_());
        }

        do_destroy();

        begin_cap_ = other.begin_cap_;
        begin_ = other.begin_;
        end_ = other.end_;
        end_cap_ = other.end_cap_;

        other.set_nullptr();

        return *this;
    }

    auto operator=(std::initializer_list<value_type> ilist) -> split_buffer& {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    auto assign(const size_type count, const value_type& value) -> void {
        if (back_spare() + size() < count) {
            const size_type diff = count - back_spare() - size();

            if (front_spare() >= diff) {
                left_shift_n(diff);

            } else {
                split_buffer sb(allocator_());
                sb.reserve_cap_and_offset_to(count, 0);

                sb.construct_at_end(count, value);

                swap_out_buffer(sb);
                return;
            }

        } else if (size() > count) {
            end_ = alloc_range_destroy(begin_ + count, end_, std::is_trivially_destructible<value_type>{});;
        }

        CIEL_POSTCONDITION(size() <= count);

        std::fill_n(begin_, size(), value);
        // if count > size()
        construct_at_end(count - size(), value);

        CIEL_POSTCONDITION(size() == count);
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    auto assign(Iter first, Iter last) -> void {
        const size_type count = std::distance(first, last);

        if (back_spare() + size() < count) {
            const size_type diff = count - back_spare() - size();
            
            if (front_spare() >= diff) {
                left_shift_n(diff);
                
            } else {
                split_buffer sb(allocator_());
                sb.reserve_cap_and_offset_to(count, 0);

                sb.construct_at_end(first, last);

                swap_out_buffer(sb);
                return;
            }

        } else if (size() > count) {
            end_ = alloc_range_destroy(begin_ + count, end_, std::is_trivially_destructible<value_type>{});;
        }

        CIEL_POSTCONDITION(size() <= count);

        Iter mid = first + size();

        std::copy(first, mid, begin_);
        // if mid < last
        construct_at_end(mid, last);

        CIEL_POSTCONDITION(size() == count);
    }

    template<class Iter, typename std::enable_if<is_exactly_input_iterator<Iter>::value, int>::type = 0>
    auto assign(Iter first, Iter last) -> void {
        clear();
        
        while (first != last) {
            emplace_back(*first);
            ++first;
        }
    }

    auto assign(std::initializer_list<value_type> ilist) -> void {
        assign(ilist.begin(), ilist.end());
    }

    CIEL_NODISCARD auto get_allocator() const noexcept -> allocator_type {
        return allocator_();
    }

    CIEL_NODISCARD auto at(const size_type pos) -> reference {
        if (pos >= size()) CIEL_UNLIKELY {
            THROW(std::out_of_range("pos is not within the range of ciel::split_buffer"));
        }

        return begin_[pos];
    }

    CIEL_NODISCARD auto at(const size_type pos) const -> const_reference {
        if (pos >= size()) CIEL_UNLIKELY {
            THROW(std::out_of_range("pos is not within the range of ciel::split_buffer"));
        }

        return begin_[pos];
    }

    CIEL_NODISCARD auto operator[](const size_type pos) -> reference {
        CIEL_PRECONDITION(pos < size());

        return begin_[pos];
    }

    CIEL_NODISCARD auto operator[](const size_type pos) const -> const_reference {
        CIEL_PRECONDITION(pos < size());

        return begin_[pos];
    }

    CIEL_NODISCARD auto front() -> reference {
        CIEL_PRECONDITION(!empty());

        return begin_[0];
    }

    CIEL_NODISCARD auto front() const -> const_reference {
        CIEL_PRECONDITION(!empty());

        return begin_[0];
    }

    CIEL_NODISCARD auto back() -> reference {
        CIEL_PRECONDITION(!empty());

        return *(end_ - 1);
    }

    CIEL_NODISCARD auto back() const -> const_reference {
        CIEL_PRECONDITION(!empty());

        return *(end_ - 1);
    }

    CIEL_NODISCARD auto data() noexcept -> T* {
        return begin_;
    }

    CIEL_NODISCARD auto data() const noexcept -> const T* {
        return begin_;
    }

    CIEL_NODISCARD auto begin() noexcept -> iterator {
        return iterator(begin_);
    }

    CIEL_NODISCARD auto begin() const noexcept -> const_iterator {
        return const_iterator(begin_);
    }

    CIEL_NODISCARD auto cbegin() const noexcept -> const_iterator {
        return begin();
    }

    CIEL_NODISCARD auto end() noexcept -> iterator {
        return iterator(end_);
    }

    CIEL_NODISCARD auto end() const noexcept -> const_iterator {
        return const_iterator(end_);
    }

    CIEL_NODISCARD auto cend() const noexcept -> const_iterator {
        return end();
    }

    CIEL_NODISCARD auto rbegin() noexcept -> reverse_iterator {
        return reverse_iterator(end());
    }

    CIEL_NODISCARD auto rbegin() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(end());
    }

    CIEL_NODISCARD auto crbegin() const noexcept -> const_reverse_iterator {
        return rbegin();
    }

    CIEL_NODISCARD auto rend() noexcept -> reverse_iterator {
        return reverse_iterator(begin());
    }

    CIEL_NODISCARD auto rend() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(begin());
    }

    CIEL_NODISCARD auto crend() const noexcept -> const_reverse_iterator {
        return rend();
    }

    CIEL_NODISCARD auto empty() const noexcept -> bool {
        return begin_ == end_;
    }

    CIEL_NODISCARD auto size() const noexcept -> size_type {
        return end_ - begin_;
    }

    CIEL_NODISCARD auto max_size() const noexcept -> size_type {
        return alloc_traits::max_size(allocator_());
    }

    auto reserve_front_spare(const size_type new_spare) -> void {
        if (new_spare <= front_spare()) {
            return;
        }

        if (new_spare <= front_spare() + back_spare()) {
            right_shift_n(new_spare - front_spare());

            CIEL_POSTCONDITION(new_spare <= front_spare());
            return;
        }

        split_buffer sb(allocator_());
        sb.reserve_cap_and_offset_to(new_spare + size() + back_spare(), new_spare);

        swap_out_buffer(sb, begin_, is_trivially_relocatable<value_type>{});

        CIEL_POSTCONDITION(new_spare <= front_spare());
    }

    auto reserve_back_spare(const size_type new_spare) -> void {
        if (new_spare <= back_spare()) {
            return;
        }

        if (new_spare <= front_spare() + back_spare()) {
            left_shift_n(new_spare - back_spare());

            CIEL_POSTCONDITION(new_spare <= back_spare());
            return;
        }

        split_buffer sb(allocator_());
        sb.reserve_cap_and_offset_to(new_spare + size() + front_spare(), front_spare());

        swap_out_buffer(sb, begin_, is_trivially_relocatable<value_type>{});

        CIEL_POSTCONDITION(new_spare <= back_spare());
    }

    CIEL_NODISCARD auto capacity() const noexcept -> size_type {
        return end_cap_ - begin_cap_;
    }

    auto shrink_to_fit() -> void {
        if (front_spare() == 0 && back_spare() == 0) CIEL_UNLIKELY {
            return;
        }

        if (size() > 0) {
            split_buffer sb(allocator_());
            sb.reserve_cap_and_offset_to(size(), size());

            swap_out_buffer(sb, is_trivially_relocatable<value_type>{});

        } else if (begin_cap_) {
            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
            set_nullptr();
        }
    }

    auto clear() noexcept -> void {
        end_ = alloc_range_destroy(begin_, end_, std::is_trivially_destructible<value_type>{});
    }

    // auto insert(iterator pos, const value_type& value) -> iterator;
    //
    // auto insert(iterator pos, value_type&& value) -> iterator;
    //
    // auto insert(iterator pos, const size_type count, const value_type& value) -> iterator;
    //
    // // We construct all at the end at first, then rotate them to the right place
    // template<class Iter, typename std::enable_if<is_exactly_input_iterator<Iter>::value, int>::type = 0>
    // auto insert(iterator pos, Iter first, Iter last) -> iterator {
    //     // record these index because it may reallocate
    //     const auto pos_index = pos - begin();
    //     const size_type old_size = size();
    //
    //     CIEL_TRY {
    //         while (first != last) {
    //             emplace_back(*first);
    //             ++first;
    //         }
    //
    //     } CIEL_CATCH (...) {
    //         end_ = alloc_range_destroy(begin_ + old_size, end_, std::is_trivially_destructible<value_type>{});
    //         CIEL_THROW;
    //     }
    //
    //     std::rotate(begin() + pos_index, begin() + old_size, end());
    //     return begin() + pos_index;
    // }
    //
    // template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    // auto insert(iterator pos, Iter first, Iter last) -> iterator {
    //     // TODO
    //
    //     const auto count = std::distance(first, last);
    //     if (count <= 0) CIEL_UNLIKELY {
    //         return pos;
    //     }
    //
    //     if (front_spare() >= static_cast<size_type>(count) &&
    //             (distance(begin(), pos) < distance(pos, end()) ||
    //             back_spare() < static_cast<size_type>(count))) {
    //         // move left half to left
    //
    //         alloc_range_construct(begin_ - count, first, last);
    //
    //         iterator old_begin = begin();
    //         begin_ -= count;
    //
    //         rotate(begin(), old_begin, pos);
    //         return pos -= count;
    //     }
    //
    //     if (back_spare() < static_cast<size_type>(count) &&
    //             front_spare() + back_spare() >= static_cast<size_type>(count)) {
    //         const auto diff = count - back_spare();
    //         left_shift_n(diff);
    //         pos -= diff;
    //     }
    //
    //     if (back_spare() >= static_cast<size_type>(count)) {    // move right half to right
    //         iterator old_end = end();
    //         end_ = alloc_range_construct(end_, first, last);
    //         rotate(pos, old_end, end());
    //         return pos;
    //     }
    //
    //     // When it comes to expansion, we need to construct new elements directly on new space,
    //     // if it throws then has no effect. And move construct two ranges divided by pos on new space
    //
    //     const size_type new_size = size() + count;
    //     size_type new_cap = capacity() ? capacity() * 2 : 1;
    //     while (new_size > new_cap) {
    //         new_cap *= 2;
    //     }
    //
    //     const size_type idx = pos - begin();
    //     pointer new_start = alloc_traits::allocate(allocator_(), new_cap);
    //     pointer new_pos = new_start + idx;
    //
    //     CIEL_TRY {
    //         alloc_range_construct(new_pos, first, last);
    //     } CIEL_CATCH (...) {
    //         alloc_traits::deallocate(allocator_(), new_start, new_cap);
    //         CIEL_THROW;
    //     }
    //
    //     if (begin_cap_) {
    //         alloc_range_move(new_start, begin_, to_address(pos));
    //         alloc_range_move(new_pos + count, to_address(pos), end_);
    //
    //         clear();
    //         alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
    //     }
    //
    //     begin_cap_ = new_start;
    //     begin_ = new_start;
    //     end_ = begin_ + new_size;
    //     end_cap_ = begin_cap_ + new_cap;
    //
    //     return iterator(new_pos);
    // }
    //
    // auto insert(iterator pos, std::initializer_list<value_type> ilist) -> iterator {
    //     return insert(pos, ilist.begin(), ilist.end());
    // }
    //
    // template<class... Args>
    // auto emplace(iterator pos, Args&& ... args) -> iterator {
    //     if (pos == end()) {
    //         emplace_back(std::forward<Args>(args)...);
    //         return iterator(end_ - 1);
    //     }
    //
    //     if (pos == begin()) {
    //         emplace_front(std::forward<Args>(args)...);
    //         return begin();
    //     }
    //
    //     return insert_n(pos, 1, std::forward<Args>(args)...);
    // }

    auto erase(iterator pos) noexcept -> iterator {
        CIEL_PRECONDITION(!empty());

        return erase(pos, pos + 1);
    }

    auto erase(iterator first, iterator last) noexcept -> iterator {
        if (std::distance(first, last) <= 0) CIEL_UNLIKELY {
            return last;
        }

        const difference_type begin_first_distance = std::distance(begin(), first);
        CIEL_PRECONDITION(begin_first_distance >= 0);

        if (begin_first_distance < std::distance(last, end())) {
            pointer old_begin = begin_;

            begin_ = std::move_backward(begin(), first, last);

            alloc_range_destroy(old_begin, begin_, std::is_trivially_destructible<value_type>{});

        } else {
            iterator new_end = std::move(last, end(), first);

            end_ = alloc_range_destroy(new_end, end_, std::is_trivially_destructible<value_type>{});
        }

        return begin() + begin_first_distance;
    }

    auto push_back(const value_type& value) -> void {
        emplace_back(value);
    }

    auto push_back(value_type&& value) -> void {
        emplace_back(std::move(value));
    }

    // Comparing with vector growing factor: get n * 2 memory, move n elements and get n new space,
    // it's terrible if we shift one (move n elements) to get 1 vacant space for emplace,
    // so only if there is plenty of space at other side will we consider shifting.
    // This situation may be seen when it's used as queue's base container.
    template<class... Args>
    auto emplace_back(Args&& ... args) -> reference {
        if (back_spare() == 0) {

            if (front_spare() > size()) CIEL_UNLIKELY {   // move size elements to get more than size / 2 vacant space
                left_shift_n(std::max<size_type>(front_spare() / 2, 1));

                construct_one_at_end(std::forward<Args>(args)...);

            } else {
                split_buffer sb(allocator_());
                // end_ - begin_cap_ == front_spare() + size()
                sb.reserve_cap_and_offset_to(recommend_cap(end_ - begin_cap_ + 1), end_ - begin_cap_);

                sb.construct_one_at_end(std::forward<Args>(args)...);

                swap_out_buffer(sb, is_trivially_relocatable<value_type>{});
            }

        } else {
            construct_one_at_end(std::forward<Args>(args)...);
        }

        return back();
    }

    auto pop_back() noexcept -> void {
        CIEL_PRECONDITION(!empty());

        end_ = alloc_range_destroy(end_ - 1, end_, std::is_trivially_destructible<value_type>{});
    }

    auto push_front(const value_type& value) -> void {
        emplace_front(value);
    }

    auto push_front(value_type&& value) -> void {
        emplace_front(std::move(value));
    }

    template<class... Args>
    auto emplace_front(Args&& ... args) -> reference {
        if (front_spare() == 0) {

            if (back_spare() > size()) CIEL_UNLIKELY {   // move size elements to get more than size / 2 vacant space
                right_shift_n(std::max<size_type>(back_spare() / 2, 1));

                construct_one_at_begin(std::forward<Args>(args)...);

            } else {
                split_buffer sb(allocator_());
                // end_cap_ - begin_ == back_spare() + size()
                const size_type new_cap = recommend_cap(end_cap_ - begin_ + 1);
                sb.reserve_cap_and_offset_to(new_cap, new_cap - (end_cap_ - begin_));

                sb.construct_one_at_begin(std::forward<Args>(args)...);

                swap_out_buffer(sb, begin_,  is_trivially_relocatable<value_type>{});
            }

        } else {
            construct_one_at_begin(std::forward<Args>(args)...);
        }

        return front();
    }

    auto pop_front() noexcept -> void {
        CIEL_PRECONDITION(!empty());

        alloc_range_destroy(begin_, begin_ + 1, std::is_trivially_destructible<value_type>{});
        ++begin_;
    }

    auto resize(const size_type count) -> void {
        if (size() >= count) {
            end_ = alloc_range_destroy(begin_ + count, end_, std::is_trivially_destructible<value_type>{});
            return;
        }

        reserve_back_spare(count - size());

        construct_at_end(count - size());
    }

    auto resize(const size_type count, const value_type& value) -> void {
        if (size() >= count) {
            end_ = alloc_range_destroy(begin_ + count, end_, std::is_trivially_destructible<value_type>{});
            return;
        }

        reserve_back_spare(count - size());

        construct_at_end(count - size(), value);
    }

    auto swap(split_buffer& other) noexcept(alloc_traits::propagate_on_container_swap::value ||
                                            alloc_traits::is_always_equal::value) -> void {
        using std::swap;
        swap(begin_cap_, other.begin_cap_);
        swap(begin_, other.begin_);
        swap(end_, other.end_);
        swap(end_cap_, other.end_cap_);
        swap(allocator_(), other.allocator_());
    }

};  // class split_buffer

template<class T, class Alloc>
CIEL_NODISCARD auto operator==(const split_buffer<T, Alloc>& lhs, const split_buffer<T, Alloc>& rhs) -> bool {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

// So that we can test more efficiently
template<class T, class Alloc>
CIEL_NODISCARD auto operator==(const split_buffer<T, Alloc>& lhs, std::initializer_list<T> rhs) -> bool {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

#if CIEL_STD_VER >= 17

template<class Iter, class Alloc = std::allocator<typename std::iterator_traits<Iter>::value_type>>
split_buffer(Iter, Iter, Alloc = Alloc()) -> split_buffer<typename std::iterator_traits<Iter>::value_type, Alloc>;

#endif

NAMESPACE_CIEL_END

namespace std {

template<class T, class Alloc>
auto swap(ciel::split_buffer<T, Alloc>& lhs,
          ciel::split_buffer<T, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) -> void {
    lhs.swap(rhs);
}

}   // namespace std

#endif // CIELLAB_INCLUDE_CIEL_SPLIT_BUFFER_HPP_
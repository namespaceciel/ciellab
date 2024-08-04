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
#include <ciel/move_proxy.hpp>
#include <ciel/type_traits.hpp>

NAMESPACE_CIEL_BEGIN

// This class is used in std::vector implementation and it's like a double-ended vector.
// When std::vector inserts beyond its capacity, it defines a temp split_buffer to store insertions
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
class split_buffer : private Allocator {
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
    using alloc_traits = std::allocator_traits<allocator_type>;

    struct new_allocation {
        owner<pointer> new_begin_cap_;
        size_type new_cap_;

    }; // struct new_allocation

    owner<pointer> begin_cap_;
    pointer begin_;
    pointer end_;
    pointer end_cap_;

    friend class vector<value_type, allocator_type>;

    template<class, size_t, class>
    friend class small_vector;

    void
    reserve_cap_and_offset_to(const size_type cap, const size_type offset) {
        begin_cap_ = alloc_traits::allocate(allocator_(), cap);
        end_cap_   = begin_cap_ + cap;
        begin_     = begin_cap_ + offset;
        end_       = begin_;
    }

    allocator_type&
    allocator_() noexcept {
        return static_cast<allocator_type&>(*this);
    }

    const allocator_type&
    allocator_() const noexcept {
        return static_cast<const allocator_type&>(*this);
    }

    size_type
    recommend_cap(const size_type new_size) const {
        CIEL_PRECONDITION(new_size > 0);

        const size_type ms = max_size();

        if CIEL_UNLIKELY (new_size > ms) {
            ciel::throw_exception(std::length_error("ciel::split_buffer reserving size is beyond max_size"));
        }

        const size_type cap = capacity();

        if CIEL_UNLIKELY (cap >= ms / 2) {
            return ms;
        }

        return std::max(cap * 2, new_size);
    }

    template<class... Args>
    void
    construct_one_at_end(Args&&... args) {
        CIEL_PRECONDITION(end_ < end_cap_);

        alloc_traits::construct(allocator_(), end_, std::forward<Args>(args)...);
        ++end_;
    }

    void
    construct_at_end(const size_type n) {
        CIEL_PRECONDITION(end_ + n <= end_cap_);

        for (size_type i = 0; i < n; ++i) {
            alloc_traits::construct(allocator_(), end_);
            ++end_;
        }
    }

    void
    construct_at_end(const size_type n, const value_type& value) {
        CIEL_PRECONDITION(end_ + n <= end_cap_);

        for (size_type i = 0; i < n; ++i) {
            alloc_traits::construct(allocator_(), end_, value);
            ++end_;
        }
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    void
    construct_at_end(Iter first, Iter last) {
        CIEL_PRECONDITION(end_ + std::distance(first, last) <= end_cap_);

        while (first != last) {
            alloc_traits::construct(allocator_(), end_, *first);
            ++first;
            ++end_;
        }
    }

    template<class... Args>
    void
    construct_one_at_begin(Args&&... args) {
        CIEL_PRECONDITION(begin_cap_ < begin_);

        alloc_traits::construct(allocator_(), begin_ - 1, std::forward<Args>(args)...);
        --begin_;
    }

    // std::is_trivially_destructible<value_type> -> std::true_type
    pointer
    alloc_range_destroy(pointer begin, pointer end, std::true_type) noexcept {
        CIEL_PRECONDITION(begin <= end);

        return begin;
    }

    // std::is_trivially_destructible<value_type> -> std::false_type
    pointer
    alloc_range_destroy(pointer begin, pointer end, std::false_type) noexcept {
        CIEL_PRECONDITION(begin <= end);

        while (end != begin) {
            alloc_traits::destroy(allocator_(), --end);
        }

        return begin;
    }

    // is_trivially_relocatable -> std::true_type
    void
    swap_out_buffer(split_buffer& sb, pointer pos, std::true_type) noexcept {
        // If either dest or src is an invalid or null pointer, the behavior is undefined, even if count is zero.
        if (begin_cap_) {
            const size_type front_count = pos - begin_;
            const size_type back_count  = end_ - pos;

            CIEL_PRECONDITION(sb.front_spare() >= front_count);
            CIEL_PRECONDITION(sb.back_spare() >= back_count);

            sb.begin_ -= front_count;
            memcpy(sb.begin_, begin_, sizeof(value_type) / sizeof(unsigned char) * front_count);

            memcpy(sb.end_, pos, sizeof(value_type) / sizeof(unsigned char) * back_count);
            sb.end_ += back_count;

            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
        }

        begin_cap_ = sb.begin_cap_;
        begin_     = sb.begin_;
        end_       = sb.end_;
        end_cap_   = sb.end_cap_;

        sb.set_nullptr();
    }

    // is_trivially_relocatable -> std::false_type
    void
    swap_out_buffer(split_buffer& sb, pointer pos,
                    std::false_type) noexcept(std::is_nothrow_move_constructible<value_type>::value) {
        if (begin_cap_) {
            const size_type front_count = pos - begin_;
            const size_type back_count  = end_ - pos;

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
        begin_     = sb.begin_;
        end_       = sb.end_;
        end_cap_   = sb.end_cap_;

        sb.set_nullptr();
    }

    // is_trivially_relocatable -> std::true_type
    void
    swap_out_buffer(split_buffer& sb, std::true_type) noexcept {
        CIEL_PRECONDITION(sb.front_spare() >= size());

        // If either dest or src is an invalid or null pointer, the behavior is undefined, even if count is zero.
        if (begin_cap_) {
            sb.begin_ -= size();
            memcpy(sb.begin_, begin_, sizeof(value_type) / sizeof(unsigned char) * size());

            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
        }

        begin_cap_ = sb.begin_cap_;
        begin_     = sb.begin_;
        end_       = sb.end_;
        end_cap_   = sb.end_cap_;

        sb.set_nullptr();
    }

    // is_trivially_relocatable -> std::false_type
    void
    swap_out_buffer(split_buffer& sb, std::false_type) noexcept(std::is_nothrow_move_constructible<value_type>::value) {
        CIEL_PRECONDITION(sb.front_spare() >= size());

        if (begin_cap_) {
            for (pointer p = end_ - 1; p >= begin_; --p) {
                sb.construct_one_at_begin(std::move(*p));
            }

            clear();
            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
        }

        begin_cap_ = sb.begin_cap_;
        begin_     = sb.begin_;
        end_       = sb.end_;
        end_cap_   = sb.end_cap_;

        sb.set_nullptr();
    }

    void
    swap_out_buffer(split_buffer& sb) noexcept {
        do_destroy();

        CIEL_PRECONDITION(sb.begin_cap_ == sb.begin_); // TODO: maybe useless

        begin_cap_ = sb.begin_cap_;
        begin_     = sb.begin_;
        end_       = sb.end_;
        end_cap_   = sb.end_cap_;

        sb.set_nullptr();
    }

    size_type
    front_spare() const noexcept {
        const difference_type res = std::distance(begin_cap_, begin_);

        CIEL_PRECONDITION(res >= 0);

        return res;
    }

    size_type
    back_spare() const noexcept {
        const difference_type res = std::distance(end_, end_cap_);

        CIEL_PRECONDITION(res >= 0);

        return res;
    }

    // Note that this will invalidate iterators
    void
    left_shift_n(const size_type n) noexcept {
        CIEL_PRECONDITION(front_spare() >= n);

        const size_type old_size = size();

        pointer new_begin = begin_ - n;
        pointer new_end   = new_begin;

        if (old_size >= n) { // n placement new, size - n move assign, n destroy

            // ----------
            //
            // ----------
            // |      | |       |
            // placement new
            // move assign
            //   destroy

            size_type i = 0;
            for (; i < n; ++i) {
                alloc_traits::construct(allocator_(), new_end, std::move(*(begin_ + i)));
                ++new_end;
            }

            for (; i < old_size; ++i) {
                *new_end = std::move(*(begin_ + i));
                ++new_end;
            }

            end_   = alloc_range_destroy(new_end, end_, std::is_trivially_destructible<value_type>{});
            begin_ = new_begin;

        } else { // size placement new, size destroy

            // ----------
            //
            // ----------
            // |        |    |        |
            // placement new
            //  destroy

            for (size_type i = 0; i < old_size; ++i) {
                alloc_traits::construct(allocator_(), new_end, std::move(*(begin_ + i)));
                ++new_end;
            }

            alloc_range_destroy(begin_, end_, std::is_trivially_destructible<value_type>{});
            begin_ = new_begin;
            end_   = new_end;
        }
    }

    // Note that this will invalidate iterators
    void
    right_shift_n(const size_type n) noexcept {
        CIEL_PRECONDITION(back_spare() >= n);

        const size_type old_size = size();

        pointer new_end = end_ + n;
        ;
        pointer new_begin = new_end;

        if (old_size >= n) { // n placement new, size - n move assign, n destroy

            // ----------
            //
            //         ----------
            // |       | |      |
            //        placement new
            //     move assign
            // destroy

            size_type i = 1;
            for (; i <= n; ++i) {
                alloc_traits::construct(allocator_(), --new_begin, std::move(*(end_ - i)));
            }

            for (; i <= old_size; ++i) {
                *(--new_begin) = std::move(*(end_ - i));
            }

            alloc_range_destroy(begin_, new_begin, std::is_trivially_destructible<value_type>{});
            begin_ = new_begin;
            end_   = new_end;

        } else { // size placement new, size destroy

            // ----------
            //
            //               ----------
            // |        |    |        |
            //              placement new
            //  destroy

            for (size_type i = 1; i <= old_size; ++i) {
                alloc_traits::construct(allocator_(), --new_begin, std::move(*(end_ - i)));
            }

            alloc_range_destroy(begin_, end_, std::is_trivially_destructible<value_type>{});
            begin_ = new_begin;
            end_   = new_end;
        }
    }

    void
    do_destroy() noexcept {
        if (begin_cap_) {
            clear();
            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
        }
    }

    void
    move_range(pointer from_s, pointer from_e, pointer to) {
        pointer old_end   = end_;
        difference_type n = old_end - to;

        {
            pointer p = from_s + n;

            for (; p < from_e; ++p) {
                construct_one_at_end(std::move(*p));
            }
        }

        std::move_backward(from_s, from_s + n, old_end);
    }

    void
    set_nullptr() noexcept {
        begin_cap_ = nullptr;
        begin_     = nullptr;
        end_       = nullptr;
        end_cap_   = nullptr;
    }

public:
    split_buffer() noexcept(noexcept(allocator_type()))
        : allocator_type(), begin_cap_(nullptr), begin_(nullptr), end_(nullptr), end_cap_(nullptr) {}

    explicit split_buffer(const allocator_type& alloc) noexcept
        : allocator_type(alloc), begin_cap_(nullptr), begin_(nullptr), end_(nullptr), end_cap_(nullptr) {}

    split_buffer(const size_type count, const value_type& value, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        if CIEL_LIKELY (count > 0) {
            begin_cap_ = alloc_traits::allocate(allocator_(), count);
            end_cap_   = begin_cap_ + count;
            begin_     = begin_cap_;
            end_       = begin_;

            CIEL_TRY {
                construct_at_end(count, value);
            }
            CIEL_CATCH (...) {
                clear();
                alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
                CIEL_THROW;
            }
        }
    }

    explicit split_buffer(const size_type count, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        if CIEL_LIKELY (count > 0) {
            begin_cap_ = alloc_traits::allocate(allocator_(), count);
            end_cap_   = begin_cap_ + count;
            begin_     = begin_cap_;
            end_       = begin_;

            CIEL_TRY {
                construct_at_end(count);
            }
            CIEL_CATCH (...) {
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
        }
        CIEL_CATCH (...) {
            do_destroy();
            CIEL_THROW;
        }
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    split_buffer(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        const auto count = std::distance(first, last);

        if CIEL_LIKELY (count > 0) {
            begin_cap_ = alloc_traits::allocate(allocator_(), count);
            end_cap_   = begin_cap_ + count;
            begin_     = begin_cap_;
            end_       = begin_;

            CIEL_TRY {
                construct_at_end(first, last);
            }
            CIEL_CATCH (...) {
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
          begin_cap_(other.begin_cap_),
          begin_(other.begin_),
          end_(other.end_),
          end_cap_(other.end_cap_) {
        other.set_nullptr();
    }

    split_buffer(split_buffer&& other, const allocator_type& alloc) {
        if (alloc == other.get_allocator()) {
            allocator_() = alloc;
            begin_cap_   = other.begin_cap_;
            begin_       = other.begin_;
            end_         = other.end_;
            end_cap_     = other.end_cap_;

            other.set_nullptr();

        } else {
            split_buffer(other, alloc).swap(*this);
        }
    }

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    split_buffer(InitializerList init, const allocator_type& alloc = allocator_type())
        : split_buffer(init.begin(), init.end(), alloc) {}

    template<class U = value_type, typename std::enable_if<worth_move_constructing<U>::value, int>::type = 0>
    split_buffer(std::initializer_list<move_proxy<value_type>> init, const allocator_type& alloc = allocator_type())
        : split_buffer(init.begin(), init.end(), alloc) {}

    template<class U = value_type, typename std::enable_if<!worth_move_constructing<U>::value, int>::type = 0>
    split_buffer(std::initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
        : split_buffer(init.begin(), init.end(), alloc) {}

    ~split_buffer() {
        do_destroy();
    }

    split_buffer&
    operator=(const split_buffer& other) {
        if CIEL_UNLIKELY (this == std::addressof(other)) {
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

    split_buffer&
    operator=(split_buffer&& other) noexcept(alloc_traits::propagate_on_container_move_assignment::value
                                             || alloc_traits::is_always_equal::value) {
        if CIEL_UNLIKELY (this == std::addressof(other)) {
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
        begin_     = other.begin_;
        end_       = other.end_;
        end_cap_   = other.end_cap_;

        other.set_nullptr();

        return *this;
    }

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    split_buffer&
    operator=(InitializerList ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    template<class U = value_type, typename std::enable_if<ciel::worth_move<U>::value, int>::type = 0>
    split_buffer&
    operator=(std::initializer_list<move_proxy<value_type>> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    template<class U = value_type, typename std::enable_if<!ciel::worth_move<U>::value, int>::type = 0>
    split_buffer&
    operator=(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    void
    assign(const size_type count, const value_type& value) {
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
            end_ = alloc_range_destroy(begin_ + count, end_, std::is_trivially_destructible<value_type>{});
            ;
        }

        CIEL_POSTCONDITION(size() <= count);

        std::fill_n(begin_, size(), value);
        // if count > size()
        construct_at_end(count - size(), value);

        CIEL_POSTCONDITION(size() == count);
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    void
    assign(Iter first, Iter last) {
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
            end_ = alloc_range_destroy(begin_ + count, end_, std::is_trivially_destructible<value_type>{});
        }

        CIEL_POSTCONDITION(size() <= count);

        Iter mid = first + size();

        std::copy(first, mid, begin_);
        // if mid < last
        construct_at_end(mid, last);

        CIEL_POSTCONDITION(size() == count);
    }

    template<class Iter, typename std::enable_if<is_exactly_input_iterator<Iter>::value, int>::type = 0>
    void
    assign(Iter first, Iter last) {
        clear();

        while (first != last) {
            emplace_back(*first);
            ++first;
        }
    }

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    void
    assign(InitializerList ilist) {
        assign(ilist.begin(), ilist.end());
    }

    template<class U = value_type, typename std::enable_if<ciel::worth_move<U>::value, int>::type = 0>
    void
    assign(std::initializer_list<move_proxy<value_type>> ilist) {
        assign(ilist.begin(), ilist.end());
    }

    template<class U = value_type, typename std::enable_if<!ciel::worth_move<U>::value, int>::type = 0>
    void
    assign(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end());
    }

    CIEL_NODISCARD allocator_type
    get_allocator() const noexcept {
        return allocator_();
    }

    CIEL_NODISCARD reference
    at(const size_type pos) {
        if CIEL_UNLIKELY (pos >= size()) {
            ciel::throw_exception(std::out_of_range("pos is not within the range of ciel::split_buffer"));
        }

        return begin_[pos];
    }

    CIEL_NODISCARD const_reference
    at(const size_type pos) const {
        if CIEL_UNLIKELY (pos >= size()) {
            ciel::throw_exception(std::out_of_range("pos is not within the range of ciel::split_buffer"));
        }

        return begin_[pos];
    }

    CIEL_NODISCARD reference
    operator[](const size_type pos) {
        CIEL_PRECONDITION(pos < size());

        return begin_[pos];
    }

    CIEL_NODISCARD const_reference
    operator[](const size_type pos) const {
        CIEL_PRECONDITION(pos < size());

        return begin_[pos];
    }

    CIEL_NODISCARD reference
    front() {
        CIEL_PRECONDITION(!empty());

        return begin_[0];
    }

    CIEL_NODISCARD const_reference
    front() const {
        CIEL_PRECONDITION(!empty());

        return begin_[0];
    }

    CIEL_NODISCARD reference
    back() {
        CIEL_PRECONDITION(!empty());

        return *(end_ - 1);
    }

    CIEL_NODISCARD const_reference
    back() const {
        CIEL_PRECONDITION(!empty());

        return *(end_ - 1);
    }

    CIEL_NODISCARD T*
    data() noexcept {
        return begin_;
    }

    CIEL_NODISCARD const T*
    data() const noexcept {
        return begin_;
    }

    CIEL_NODISCARD iterator
    begin() noexcept {
        return iterator(begin_);
    }

    CIEL_NODISCARD const_iterator
    begin() const noexcept {
        return const_iterator(begin_);
    }

    CIEL_NODISCARD const_iterator
    cbegin() const noexcept {
        return begin();
    }

    CIEL_NODISCARD iterator
    end() noexcept {
        return iterator(end_);
    }

    CIEL_NODISCARD const_iterator
    end() const noexcept {
        return const_iterator(end_);
    }

    CIEL_NODISCARD const_iterator
    cend() const noexcept {
        return end();
    }

    CIEL_NODISCARD reverse_iterator
    rbegin() noexcept {
        return reverse_iterator(end());
    }

    CIEL_NODISCARD const_reverse_iterator
    rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    CIEL_NODISCARD const_reverse_iterator
    crbegin() const noexcept {
        return rbegin();
    }

    CIEL_NODISCARD reverse_iterator
    rend() noexcept {
        return reverse_iterator(begin());
    }

    CIEL_NODISCARD const_reverse_iterator
    rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    CIEL_NODISCARD const_reverse_iterator
    crend() const noexcept {
        return rend();
    }

    CIEL_NODISCARD bool
    empty() const noexcept {
        return begin_ == end_;
    }

    CIEL_NODISCARD size_type
    size() const noexcept {
        return end_ - begin_;
    }

    CIEL_NODISCARD size_type
    max_size() const noexcept {
        return alloc_traits::max_size(allocator_());
    }

    void
    reserve_front_spare(const size_type new_spare) {
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

    void
    reserve_back_spare(const size_type new_spare) {
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

    CIEL_NODISCARD size_type
    capacity() const noexcept {
        return end_cap_ - begin_cap_;
    }

    void
    shrink_to_fit() {
        if CIEL_UNLIKELY (front_spare() == 0 && back_spare() == 0) {
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

    void
    clear() noexcept {
        end_ = alloc_range_destroy(begin_, end_, std::is_trivially_destructible<value_type>{});
    }

    // iterator insert(iterator pos, const value_type& value);
    //
    // iterator insert(iterator pos, value_type&& value);
    //
    // iterator insert(iterator pos, const size_type count, const value_type& value);
    //
    // // We construct all at the end at first, then rotate them to the right place
    // template<class Iter, typename std::enable_if<is_exactly_input_iterator<Iter>::value, int>::type = 0>
    // iterator insert(iterator pos, Iter first, Iter last) {
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
    // iterator insert(iterator pos, Iter first, Iter last) {
    //     // TODO
    //
    //     const auto count = std::distance(first, last);
    //     if CIEL_UNLIKELY (count <= 0) {
    //         return pos;
    //     }
    //
    //     if (front_spare() >= static_cast<size_type>(count) &&
    //             (std::distance(begin(), pos) < std::distance(pos, end()) ||
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
    // iterator insert(iterator pos, std::initializer_list<value_type> ilist) {
    //     return insert(pos, ilist.begin(), ilist.end());
    // }
    //
    // template<class... Args>
    // iterator emplace(iterator pos, Args&& ... args) {
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

    iterator
    erase(iterator pos) noexcept {
        CIEL_PRECONDITION(!empty());

        return erase(pos, pos + 1);
    }

    iterator
    erase(iterator first, iterator last) noexcept {
        if CIEL_UNLIKELY (std::distance(first, last) <= 0) {
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

    void
    push_back(const value_type& value) {
        emplace_back(value);
    }

    void
    push_back(value_type&& value) {
        emplace_back(std::move(value));
    }

    // Comparing with vector growing factor: get n * 2 memory, move n elements and get n new space,
    // it's terrible if we shift one (move n elements) to get 1 vacant space for emplace,
    // so only if there is plenty of space at other side will we consider shifting.
    // This situation may be seen when it's used as queue's base container.
    template<class... Args>
    reference
    emplace_back(Args&&... args) {
        if (back_spare() == 0) {
            if CIEL_UNLIKELY (front_spare() > size()) { // move size elements to get more than size / 2 vacant space
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

    void
    pop_back() noexcept {
        CIEL_PRECONDITION(!empty());

        end_ = alloc_range_destroy(end_ - 1, end_, std::is_trivially_destructible<value_type>{});
    }

    void
    push_front(const value_type& value) {
        emplace_front(value);
    }

    void
    push_front(value_type&& value) {
        emplace_front(std::move(value));
    }

    template<class... Args>
    reference
    emplace_front(Args&&... args) {
        if (front_spare() == 0) {
            if CIEL_UNLIKELY (back_spare() > size()) { // move size elements to get more than size / 2 vacant space
                right_shift_n(std::max<size_type>(back_spare() / 2, 1));

                construct_one_at_begin(std::forward<Args>(args)...);

            } else {
                split_buffer sb(allocator_());
                // end_cap_ - begin_ == back_spare() + size()
                const size_type new_cap = recommend_cap(end_cap_ - begin_ + 1);
                sb.reserve_cap_and_offset_to(new_cap, new_cap - (end_cap_ - begin_));

                sb.construct_one_at_begin(std::forward<Args>(args)...);

                swap_out_buffer(sb, begin_, is_trivially_relocatable<value_type>{});
            }

        } else {
            construct_one_at_begin(std::forward<Args>(args)...);
        }

        return front();
    }

    void
    pop_front() noexcept {
        CIEL_PRECONDITION(!empty());

        alloc_range_destroy(begin_, begin_ + 1, std::is_trivially_destructible<value_type>{});
        ++begin_;
    }

    void
    resize(const size_type count) {
        if (size() >= count) {
            end_ = alloc_range_destroy(begin_ + count, end_, std::is_trivially_destructible<value_type>{});
            return;
        }

        reserve_back_spare(count - size());

        construct_at_end(count - size());
    }

    void
    resize(const size_type count, const value_type& value) {
        if (size() >= count) {
            end_ = alloc_range_destroy(begin_ + count, end_, std::is_trivially_destructible<value_type>{});
            return;
        }

        reserve_back_spare(count - size());

        construct_at_end(count - size(), value);
    }

    void
    swap(split_buffer& other) noexcept(alloc_traits::propagate_on_container_swap::value
                                       || alloc_traits::is_always_equal::value) {
        using std::swap;

        swap(begin_cap_, other.begin_cap_);
        swap(begin_, other.begin_);
        swap(end_, other.end_);
        swap(end_cap_, other.end_cap_);
        swap(allocator_(), other.allocator_());
    }

}; // class split_buffer

template<class T, class Alloc>
CIEL_NODISCARD bool
operator==(const split_buffer<T, Alloc>& lhs, const split_buffer<T, Alloc>& rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

// So that we can test more efficiently
template<class T, class Alloc>
CIEL_NODISCARD bool
operator==(const split_buffer<T, Alloc>& lhs, std::initializer_list<T> rhs) noexcept {
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
void
swap(ciel::split_buffer<T, Alloc>& lhs, ciel::split_buffer<T, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_SPLIT_BUFFER_HPP_

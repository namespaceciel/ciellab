#ifndef CIELLAB_INCLUDE_CIEL_SPLIT_BUFFER_HPP_
#define CIELLAB_INCLUDE_CIEL_SPLIT_BUFFER_HPP_

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include <ciel/compressed_pair.hpp>
#include <ciel/config.hpp>
#include <ciel/copy_n.hpp>
#include <ciel/range_destroyer.hpp>
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

// TODO: insert and erase

template<class, class>
class vector;
template<class, size_t, class>
class small_vector;

// Note that Allocator can be reference type as being used by vector,
// however in this case, the assignment operator of split_buffer may be invalid.
template<class T, class Allocator = std::allocator<T>>
class split_buffer {
    static_assert(!std::is_rvalue_reference<Allocator>::value, "");

public:
    using value_type             = T;
    using allocator_type         = typename std::remove_reference<Allocator>::type;
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

    static_assert(std::is_same<typename allocator_type::value_type, T>::value, "");

private:
    using alloc_traits = std::allocator_traits<allocator_type>;

    pointer begin_cap_{nullptr};
    pointer begin_{nullptr};
    pointer end_{nullptr};
    compressed_pair<pointer, Allocator> end_cap_alloc_{nullptr, default_init};

    template<class, class>
    friend class split_buffer;
    template<class, class>
    friend class vector;
    template<class, size_t, class>
    friend class small_vector;

    void
    reserve_cap_and_offset_to(const size_type cap, const size_type offset) {
        CIEL_PRECONDITION(begin_cap_ == nullptr);
        CIEL_PRECONDITION(cap != 0);
        CIEL_PRECONDITION(cap >= offset);

        begin_cap_ = alloc_traits::allocate(allocator_(), cap);
        end_cap_() = begin_cap_ + cap;
        begin_     = begin_cap_ + offset;
        end_       = begin_;
    }

    CIEL_NODISCARD pointer&
    end_cap_() noexcept {
        return end_cap_alloc_.first();
    }

    CIEL_NODISCARD const pointer&
    end_cap_() const noexcept {
        return end_cap_alloc_.first();
    }

    allocator_type&
    allocator_() noexcept {
        return end_cap_alloc_.second();
    }

    const allocator_type&
    allocator_() const noexcept {
        return end_cap_alloc_.second();
    }

    CIEL_NODISCARD size_type
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

    void
    construct_at_end(const size_type n) {
        CIEL_PRECONDITION(end_ + n <= end_cap_());

        for (size_type i = 0; i < n; ++i) {
            unchecked_emplace_back();
        }
    }

    void
    construct_at_end(const size_type n, const value_type& value) {
        CIEL_PRECONDITION(end_ + n <= end_cap_());

        for (size_type i = 0; i < n; ++i) {
            unchecked_emplace_back(value);
        }
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    void
    construct_at_end(Iter first, Iter last) {
        CIEL_PRECONDITION(end_ + std::distance(first, last) <= end_cap_());

        while (first != last) {
            unchecked_emplace_back(*first);
            ++first;
        }
    }

    template<class U = value_type, typename std::enable_if<std::is_trivially_destructible<U>::value, int>::type = 0>
    pointer
    alloc_range_destroy(pointer begin, pointer end) noexcept {
        CIEL_PRECONDITION(begin <= end);
        CIEL_PRECONDITION(begin_ <= begin);
        CIEL_PRECONDITION(end <= end_);

        return begin;
    }

    template<class U = value_type, typename std::enable_if<!std::is_trivially_destructible<U>::value, int>::type = 0>
    pointer
    alloc_range_destroy(pointer begin, pointer end) noexcept {
        CIEL_PRECONDITION(begin <= end);
        CIEL_PRECONDITION(begin_ <= begin);
        CIEL_PRECONDITION(end <= end_);

        while (end != begin) {
            alloc_traits::destroy(allocator_(), --end);
        }

        return begin;
    }

    template<class U = value_type, typename std::enable_if<is_trivially_relocatable<U>::value, int>::type = 0>
    void
    swap_out_buffer(split_buffer<value_type, allocator_type&>&& sb, pointer pos) noexcept {
        // Used by emplace_front and emplace_back respectively.
        CIEL_PRECONDITION(pos == begin_ || pos == end_);

        // If either dest or src is an invalid or null pointer, memcpy's behavior is undefined, even if count is zero.
        if (begin_cap_) {
            const size_type front_count = pos - begin_;
            const size_type back_count  = end_ - pos;

            CIEL_PRECONDITION(sb.front_spare() >= front_count);
            CIEL_PRECONDITION(sb.back_spare() >= back_count);

            sb.begin_ -= front_count;
            memcpy(sb.begin_, begin_, sizeof(value_type) * front_count);

            memcpy(sb.end_, pos, sizeof(value_type) * back_count);
            sb.end_ += back_count;

            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
        }

        begin_cap_ = sb.begin_cap_;
        begin_     = sb.begin_;
        end_       = sb.end_;
        end_cap_() = sb.end_cap_();

        sb.set_nullptr();
    }

    template<class U = value_type, typename std::enable_if<!is_trivially_relocatable<U>::value, int>::type = 0>
    void
    swap_out_buffer(split_buffer<value_type, allocator_type&>&& sb,
                    pointer pos) noexcept(std::is_nothrow_move_constructible<value_type>::value) {
        // Used by emplace_front and emplace_back respectively.
        CIEL_PRECONDITION(pos == begin_ || pos == end_);

        if (begin_cap_) {
            const size_type front_count = pos - begin_;
            const size_type back_count  = end_ - pos;

            CIEL_PRECONDITION(sb.front_spare() >= front_count);
            CIEL_PRECONDITION(sb.back_spare() >= back_count);

            for (pointer p = pos - 1; p >= begin_; --p) {
#ifdef CIEL_HAS_EXCEPTIONS
                sb.unchecked_emplace_front(std::move_if_noexcept(*p));
#else
                sb.unchecked_emplace_front(std::move(*p));
#endif
            }

            for (pointer p = pos; p < end_; ++p) {
#ifdef CIEL_HAS_EXCEPTIONS
                sb.unchecked_emplace_back(std::move_if_noexcept(*p));
#else
                sb.unchecked_emplace_back(std::move(*p));
#endif
            }

            clear();
            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
        }

        begin_cap_ = sb.begin_cap_;
        begin_     = sb.begin_;
        end_       = sb.end_;
        end_cap_() = sb.end_cap_();

        sb.set_nullptr();
    }

    CIEL_NODISCARD size_type
    front_spare() const noexcept {
        CIEL_PRECONDITION(begin_cap_ <= begin_);

        return begin_ - begin_cap_;
    }

    CIEL_NODISCARD size_type
    back_spare() const noexcept {
        CIEL_PRECONDITION(end_ <= end_cap_());

        return end_cap_() - end_;
    }

    // Note that this will invalidate iterators.
    template<class U = value_type, typename std::enable_if<is_trivially_relocatable<U>::value, int>::type = 0>
    void
    left_shift_n(const size_type n) noexcept {
        CIEL_PRECONDITION(front_spare() >= n);

        std::memmove(begin_ - n, begin_, sizeof(value_type) * size());
        begin_ -= n;
        end_ -= n;
    }

    // Note that this will invalidate iterators.
    template<class U = value_type, typename std::enable_if<!is_trivially_relocatable<U>::value, int>::type = 0>
    void
    left_shift_n(const size_type n) noexcept {
        CIEL_PRECONDITION(front_spare() >= n);

        const size_type old_size = size();

        pointer new_begin = begin_ - n;
        pointer new_end   = new_begin;
        range_destroyer<value_type, allocator_type&> rd{new_begin, new_end, allocator_()};

        if (old_size >= n) { // n placement new, size - n move assign, n destroy
            // clang-format off
            //         ----------
            //
            // ----------
            // |      | |       |
            // placement new
            //    move assign
            //           destroy
            // clang-format on

            size_type i = 0;
            for (; i < n; ++i) {
                alloc_traits::construct(allocator_(), new_end, std::move(*(begin_ + i)));
                ++new_end;
                rd.advance_forward();
            }

            for (; i < old_size; ++i) {
                *new_end = std::move(*(begin_ + i));
                ++new_end;
            }

            end_   = alloc_range_destroy(new_end, end_);
            begin_ = new_begin;

        } else { // size placement new, size destroy
            // clang-format off
            //               ----------
            //
            // ----------
            // |        |    |        |
            // placement new
            //                 destroy
            // clang-format on

            for (size_type i = 0; i < old_size; ++i) {
                alloc_traits::construct(allocator_(), new_end, std::move(*(begin_ + i)));
                ++new_end;
                rd.advance_forward();
            }

            alloc_range_destroy(begin_, end_);
            begin_ = new_begin;
            end_   = new_end;
        }

        rd.release();
    }

    // Note that this will invalidate iterators.
    template<class U = value_type, typename std::enable_if<is_trivially_relocatable<U>::value, int>::type = 0>
    void
    right_shift_n(const size_type n) noexcept {
        CIEL_PRECONDITION(back_spare() >= n);

        std::memmove(begin_ + n, begin_, sizeof(value_type) * size());
        begin_ += n;
        end_ += n;
    }

    // Note that this will invalidate iterators.
    template<class U = value_type, typename std::enable_if<!is_trivially_relocatable<U>::value, int>::type = 0>
    void
    right_shift_n(const size_type n) noexcept {
        CIEL_PRECONDITION(back_spare() >= n);

        const size_type old_size = size();

        pointer new_end   = end_ + n;
        pointer new_begin = new_end;
        range_destroyer<value_type, allocator_type&> rd{new_begin, new_end, allocator_()};

        if (old_size >= n) { // n placement new, size - n move assign, n destroy
            // clang-format off
            // ----------
            //
            //         ----------
            // |       | |      |
            //             placement new
            //     move assign
            //  destroy
            // clang-format on

            size_type i = 1;
            for (; i <= n; ++i) {
                alloc_traits::construct(allocator_(), --new_begin, std::move(*(end_ - i)));
                rd.advance_backward();
            }

            for (; i <= old_size; ++i) {
                *(--new_begin) = std::move(*(end_ - i));
            }

            alloc_range_destroy(begin_, new_begin);
            begin_ = new_begin;
            end_   = new_end;

        } else { // size placement new, size destroy
            // clang-format off
            // ----------
            //
            //               ----------
            // |        |    |        |
            //              placement new
            //  destroy
            // clang-format on

            for (size_type i = 1; i <= old_size; ++i) {
                alloc_traits::construct(allocator_(), --new_begin, std::move(*(end_ - i)));
                rd.advance_backward();
            }

            alloc_range_destroy(begin_, end_);
            begin_ = new_begin;
            end_   = new_end;
        }

        rd.release();
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

        for (pointer p = from_s + n; p < from_e; ++p) {
            unchecked_emplace_back(std::move(*p));
        }

        std::move_backward(from_s, from_s + n, old_end);
    }

    void
    set_nullptr() noexcept {
        begin_cap_ = nullptr;
        begin_     = nullptr;
        end_       = nullptr;
        end_cap_() = nullptr;
    }

    // Comparing with vector growing factor: get n * 2 memory, move n elements and get n new space,
    // it's terrible if we shift one (move n elements) to get 1 vacant space for emplace,
    // so only if there is plenty of space at other side will we consider shifting.
    // This situation may be seen when it's used as queue's base container.
    template<class... Args>
    reference
    emplace_back_aux(Args&&... args) {
        if (back_spare() == 0) {
            if CIEL_UNLIKELY (front_spare() > size()) { // move size elements to get more than size / 2 vacant space
                // To support self reference operations like v.emplace_back(v[0]),
                // we must construct temp object here and move it afterwards.
                value_type tmp(std::forward<Args>(args)...);

                left_shift_n(std::max<size_type>(front_spare() / 2, 1));

                unchecked_emplace_back(std::move(tmp));

            } else {
                split_buffer<value_type, allocator_type&> sb(allocator_());
                // end_ - begin_cap_ == front_spare() + size()
                sb.reserve_cap_and_offset_to(recommend_cap(end_ - begin_cap_ + 1), end_ - begin_cap_);

                sb.unchecked_emplace_back(std::forward<Args>(args)...);

                swap_out_buffer(std::move(sb), end_);
            }

        } else {
            return unchecked_emplace_back(std::forward<Args>(args)...);
        }

        return back();
    }

    // Check out emplace_back_aux for annotations.
    template<class... Args>
    reference
    emplace_front_aux(Args&&... args) {
        if (front_spare() == 0) {
            if CIEL_UNLIKELY (back_spare() > size()) {
                value_type tmp(std::forward<Args>(args)...);

                right_shift_n(std::max<size_type>(back_spare() / 2, 1));

                unchecked_emplace_front(std::move(tmp));

            } else {
                split_buffer<value_type, allocator_type&> sb(allocator_());
                // end_cap_() - begin_ == back_spare() + size()
                const size_type new_cap = recommend_cap(end_cap_() - begin_ + 1);
                sb.reserve_cap_and_offset_to(new_cap, new_cap - (end_cap_() - begin_));

                sb.unchecked_emplace_front(std::forward<Args>(args)...);

                swap_out_buffer(std::move(sb), begin_);
            }

        } else {
            return unchecked_emplace_front(std::forward<Args>(args)...);
        }

        return front();
    }

    template<class... Args>
    reference
    unchecked_emplace_back_aux(Args&&... args) {
        CIEL_PRECONDITION(end_ < end_cap_());

        alloc_traits::construct(allocator_(), end_, std::forward<Args>(args)...);
        ++end_;

        return back();
    }

    template<class... Args>
    reference
    unchecked_emplace_front_aux(Args&&... args) {
        CIEL_PRECONDITION(begin_cap_ < begin_);

        alloc_traits::construct(allocator_(), begin_ - 1, std::forward<Args>(args)...);
        --begin_;

        return front();
    }

    template<class Iter>
    void
    assign(Iter first, Iter last, const size_type count) {
        if (back_spare() + size() < count) {
            const size_type diff = count - back_spare() - size();

            if (front_spare() >= diff) {
                left_shift_n(diff);

            } else {
                split_buffer{first, last, allocator_()}.swap(*this);
                return;
            }

        } else if (size() > count) {
            end_ = alloc_range_destroy(begin_ + count, end_);
        }

        CIEL_POSTCONDITION(size() <= count);

        Iter mid = ciel::copy_n(first, size(), begin_);

        // if mid < last
        construct_at_end(mid, last);

        CIEL_POSTCONDITION(size() == count);
    }

public:
    split_buffer() noexcept(noexcept(allocator_type())) = default;

    explicit split_buffer(allocator_type& alloc) noexcept
        : end_cap_alloc_(nullptr, alloc) {}

    explicit split_buffer(const allocator_type& alloc) noexcept
        : end_cap_alloc_(nullptr, alloc) {}

    split_buffer(const size_type count, const value_type& value, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        if CIEL_LIKELY (count > 0) {
            begin_cap_ = alloc_traits::allocate(allocator_(), count);
            end_cap_() = begin_cap_ + count;
            begin_     = begin_cap_;
            end_       = begin_;

            construct_at_end(count, value);
        }
    }

    explicit split_buffer(const size_type count, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        if CIEL_LIKELY (count > 0) {
            begin_cap_ = alloc_traits::allocate(allocator_(), count);
            end_cap_() = begin_cap_ + count;
            begin_     = begin_cap_;
            end_       = begin_;

            construct_at_end(count);
        }
    }

    template<class Iter, typename std::enable_if<is_exactly_input_iterator<Iter>::value, int>::type = 0>
    split_buffer(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        while (first != last) {
            emplace_back(*first);
            ++first;
        }
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    split_buffer(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        const auto count = std::distance(first, last);

        if CIEL_LIKELY (count > 0) {
            begin_cap_ = alloc_traits::allocate(allocator_(), count);
            end_cap_() = begin_cap_ + count;
            begin_     = begin_cap_;
            end_       = begin_;

            construct_at_end(first, last);
        }
    }

    split_buffer(const split_buffer& other)
        : split_buffer(other.begin(), other.end(),
                       alloc_traits::select_on_container_copy_construction(other.get_allocator())) {}

    split_buffer(const split_buffer& other, const allocator_type& alloc)
        : split_buffer(other.begin(), other.end(), alloc) {}

    split_buffer(split_buffer&& other) noexcept
        : begin_cap_(other.begin_cap_),
          begin_(other.begin_),
          end_(other.end_),
          end_cap_alloc_(other.end_cap_(), std::move(other.allocator_())) {
        other.set_nullptr();
    }

    split_buffer(split_buffer&& other, const allocator_type& alloc) {
        if (alloc == other.get_allocator()) {
            allocator_() = alloc;
            begin_cap_   = other.begin_cap_;
            begin_       = other.begin_;
            end_         = other.end_;
            end_cap_()   = other.end_cap_();

            other.set_nullptr();

        } else {
            split_buffer(other, alloc).swap(*this);
        }
    }

    split_buffer(std::initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
        : split_buffer(init.begin(), init.end(), alloc) {}

    template<class R,
             typename std::enable_if<is_range_without_size<R>::value && std::is_lvalue_reference<R>::value, int>::type
             = 0>
    split_buffer(from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : split_buffer(rg.begin(), rg.end(), alloc) {}

    template<class R,
             typename std::enable_if<is_range_without_size<R>::value && !std::is_lvalue_reference<R>::value, int>::type
             = 0>
    split_buffer(from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : split_buffer(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()), alloc) {}

    template<class R,
             typename std::enable_if<is_range_with_size<R>::value && std::is_lvalue_reference<R>::value, int>::type = 0>
    split_buffer(from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        const auto count = rg.size();

        if CIEL_LIKELY (count > 0) {
            begin_     = alloc_traits::allocate(allocator_(), count);
            end_cap_() = begin_ + count;
            end_       = begin_;

            construct_at_end(rg.begin(), rg.end());
        }
    }

    template<class R,
             typename std::enable_if<is_range_with_size<R>::value && !std::is_lvalue_reference<R>::value, int>::type
             = 0>
    split_buffer(from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        const auto count = rg.size();

        if CIEL_LIKELY (count > 0) {
            begin_     = alloc_traits::allocate(allocator_(), count);
            end_cap_() = begin_ + count;
            end_       = begin_;

            construct_at_end(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()));
        }
    }

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
                split_buffer(other.begin(), other.end(), other.allocator_()).swap(*this);
                return *this;
            }

            allocator_() = other.allocator_();
        }

        assign(other.begin(), other.end());

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
        end_cap_() = other.end_cap_();

        other.set_nullptr();

        return *this;
    }

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
                split_buffer{count, value, allocator_()}.swap(*this);
                return;
            }

        } else if (size() > count) {
            end_ = alloc_range_destroy(begin_ + count, end_);
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

        assign(first, last, count);
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

        split_buffer<value_type, allocator_type&> sb(allocator_());
        sb.reserve_cap_and_offset_to(new_spare + size() + back_spare(), new_spare);

        swap_out_buffer(std::move(sb), begin_);

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

        split_buffer<value_type, allocator_type&> sb(allocator_());
        sb.reserve_cap_and_offset_to(new_spare + size() + front_spare(), front_spare());

        swap_out_buffer(std::move(sb), begin_);

        CIEL_POSTCONDITION(new_spare <= back_spare());
    }

    CIEL_NODISCARD size_type
    capacity() const noexcept {
        return end_cap_() - begin_cap_;
    }

    void
    shrink_to_fit() {
        if CIEL_UNLIKELY (front_spare() == 0 && back_spare() == 0) {
            return;
        }

        if (size() > 0) {
            split_buffer<value_type, allocator_type&> sb(allocator_());

            CIEL_TRY {
                sb.reserve_cap_and_offset_to(size(), 0);

                swap_out_buffer(std::move(sb), begin_);
            }
            CIEL_CATCH (...) {}

        } else if (begin_cap_) {
            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
            set_nullptr();
        }
    }

    void
    clear() noexcept {
        end_ = alloc_range_destroy(begin_, end_);
    }

    void
    push_back(const value_type& value) {
        emplace_back(value);
    }

    void
    push_back(value_type&& value) {
        emplace_back(std::move(value));
    }

    template<class... Args>
    reference
    emplace_back(Args&&... args) {
        return emplace_back_aux(std::forward<Args>(args)...);
    }

    template<class U, class... Args>
    reference
    emplace_back(std::initializer_list<U> il, Args&&... args) {
        return emplace_back_aux(il, std::forward<Args>(args)...);
    }

    void
    pop_back() noexcept {
        CIEL_PRECONDITION(!empty());

        end_ = alloc_range_destroy(end_ - 1, end_);
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
        return emplace_front_aux(std::forward<Args>(args)...);
    }

    template<class U, class... Args>
    reference
    emplace_front(std::initializer_list<U> il, Args&&... args) {
        return emplace_front_aux(il, std::forward<Args>(args)...);
    }

    void
    pop_front() noexcept {
        CIEL_PRECONDITION(!empty());

        alloc_range_destroy(begin_, begin_ + 1);
        ++begin_;
    }

    void
    resize(const size_type count) {
        if (size() >= count) {
            end_ = alloc_range_destroy(begin_ + count, end_);
            return;
        }

        reserve_back_spare(count - size());

        construct_at_end(count - size());
    }

    void
    resize(const size_type count, const value_type& value) {
        if (size() >= count) {
            end_ = alloc_range_destroy(begin_ + count, end_);
            return;
        }

        reserve_back_spare(count - size());

        construct_at_end(count - size(), value);
    }

    template<class A = Allocator, class U = split_buffer,
             typename std::enable_if<!std::is_reference<A>::value && is_trivially_relocatable<U>::value, int>::type = 0>
    void
    swap(split_buffer& other) noexcept {
        ciel::relocatable_swap(*this, other);
    }

    template<class A = Allocator, class U = split_buffer,
             typename std::enable_if<!std::is_reference<A>::value && !is_trivially_relocatable<U>::value, int>::type
             = 0>
    void
    swap(split_buffer& other) noexcept(alloc_traits::propagate_on_container_swap::value
                                       || alloc_traits::is_always_equal::value) {
        using std::swap;

        swap(begin_cap_, other.begin_cap_);
        swap(begin_, other.begin_);
        swap(end_, other.end_);
        swap(end_cap_(), other.end_cap_());
        swap(allocator_(), other.allocator_());
    }

    template<class... Args>
    reference
    unchecked_emplace_back(Args&&... args) {
        return unchecked_emplace_back_aux(std::forward<Args>(args)...);
    }

    template<class U, class... Args>
    reference
    unchecked_emplace_back(std::initializer_list<U> il, Args&&... args) {
        return unchecked_emplace_back_aux(il, std::forward<Args>(args)...);
    }

    template<class... Args>
    reference
    unchecked_emplace_front(Args&&... args) {
        return unchecked_emplace_front_aux(std::forward<Args>(args)...);
    }

    template<class U, class... Args>
    reference
    unchecked_emplace_front(std::initializer_list<U> il, Args&&... args) {
        return unchecked_emplace_front_aux(il, std::forward<Args>(args)...);
    }

    template<class R, typename std::enable_if<is_range<R>::value, int>::type = 0>
    void
    assign_range(R&& rg) {
        if CIEL_CONSTEXPR_SINCE_CXX17 (is_range_with_size<R>::value) {
            if CIEL_CONSTEXPR_SINCE_CXX17 (std::is_lvalue_reference<R>::value) {
                assign(rg.begin(), rg.end(), rg.size());

            } else {
                assign(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()), rg.size());
            }

        } else {
            if CIEL_CONSTEXPR_SINCE_CXX17 (std::is_lvalue_reference<R>::value) {
                assign(rg.begin(), rg.end());

            } else {
                assign(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()));
            }
        }
    }

}; // class split_buffer

template<class T, class Allocator>
struct is_trivially_relocatable<split_buffer<T, Allocator>> : is_trivially_relocatable<Allocator> {};

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

#ifndef CIELLAB_INCLUDE_CIEL_SPLIT_BUFFER_HPP_
#define CIELLAB_INCLUDE_CIEL_SPLIT_BUFFER_HPP_

#include <ciel/allocator_traits.hpp>
#include <ciel/compare.hpp>
#include <ciel/compressed_pair.hpp>
#include <ciel/config.hpp>
#include <ciel/copy_n.hpp>
#include <ciel/cstring.hpp>
#include <ciel/do_if_noexcept.hpp>
#include <ciel/is_range.hpp>
#include <ciel/is_trivially_relocatable.hpp>
#include <ciel/iterator_category.hpp>
#include <ciel/range_destroyer.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>

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

    static constexpr bool should_pass_by_value
        = std::is_trivially_copyable<value_type>::value && sizeof(value_type) <= 16;
    using lvalue = typename std::conditional<should_pass_by_value, value_type, const value_type&>::type;
    using rvalue = typename std::conditional<should_pass_by_value, value_type, value_type&&>::type;

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
            CIEL_THROW_EXCEPTION(std::length_error("ciel::split_buffer reserving size is beyond max_size"));
        }

        const size_type cap = capacity();

        if CIEL_UNLIKELY (cap >= ms / 2) {
            return ms;
        }

        return std::max(cap * 2, new_size);
    }

    CIEL_NODISCARD bool
    internal_value(const value_type& value, pointer begin) const noexcept {
        if (should_pass_by_value) {
            return false;
        }

        if CIEL_UNLIKELY (begin <= std::addressof(value) && std::addressof(value) < end_) {
            return true;
        }

        return false;
    }

    void
    construct_at_end(const size_type n) {
        CIEL_PRECONDITION(end_ + n <= end_cap_());

        for (size_type i = 0; i < n; ++i) {
            unchecked_emplace_back_aux();
        }
    }

    void
    construct_at_end(const size_type n, lvalue value) {
        CIEL_PRECONDITION(end_ + n <= end_cap_());

        for (size_type i = 0; i < n; ++i) {
            unchecked_emplace_back_aux(value);
        }
    }

    template<class Iter>
    void
    construct_at_end(Iter first, Iter last) {
        ciel::uninitialized_copy(allocator_(), first, last, end_);
    }

    template<class... Args>
    void
    construct(pointer p, Args&&... args) {
        if (allocator_has_trivial_construct<allocator_type, pointer, Args...>::value) {
            new (ciel::to_address(p)) value_type(std::forward<Args>(args)...);

        } else {
            alloc_traits::construct(allocator_(), ciel::to_address(p), std::forward<Args>(args)...);
        }
    }

    template<class U                                                                            = value_type,
             typename std::enable_if<std::is_trivially_copy_constructible<U>::value, int>::type = 0>
    void
    construct(pointer p, value_type value) {
        if (allocator_has_trivial_copy_construct<allocator_type>::value) {
            *p = value;

        } else {
            alloc_traits::construct(allocator_(), ciel::to_address(p), value);
        }
    }

    template<class U                                                                             = value_type,
             typename std::enable_if<!std::is_trivially_copy_constructible<U>::value, int>::type = 0>
    void
    construct(pointer p, const value_type& value) {
        if (allocator_has_trivial_copy_construct<allocator_type>::value) {
            new (ciel::to_address(p)) value_type(value);

        } else {
            alloc_traits::construct(allocator_(), ciel::to_address(p), value);
        }
    }

    pointer
    destroy(pointer first, pointer last) noexcept {
        CIEL_PRECONDITION(begin_ <= first);
        CIEL_PRECONDITION(first <= last);
        CIEL_PRECONDITION(last <= end_);

        const pointer res = first;

        if (allocator_has_trivial_destroy<allocator_type>::value) {
            if (!std::is_trivially_destructible<value_type>::value) {
                for (; last - first >= 4; first += 4) {
                    (ciel::to_address(first + 0))->~value_type();
                    (ciel::to_address(first + 1))->~value_type();
                    (ciel::to_address(first + 2))->~value_type();
                    (ciel::to_address(first + 3))->~value_type();
                }

                for (; first != last; ++first) {
                    ciel::to_address(first)->~value_type();
                }
            }

        } else {
            for (; first != last; ++first) {
                alloc_traits::destroy(allocator_(), ciel::to_address(first));
            }
        }

        return res;
    }

    void
    destroy(pointer p) noexcept {
        CIEL_PRECONDITION(begin_ <= p);
        CIEL_PRECONDITION(p <= end_); // called by pop_back

        if (allocator_has_trivial_destroy<allocator_type>::value) {
            if (!std::is_trivially_destructible<value_type>::value) {
                p->~value_type();
            }

        } else {
            alloc_traits::destroy(allocator_(), p);
        }
    }

    void
    swap_out_buffer(split_buffer<value_type, allocator_type&>&& sb,
                    pointer pos) noexcept(is_trivially_relocatable<value_type>::value
                                          || std::is_nothrow_move_constructible<value_type>::value) {
        // Used by emplace_front and emplace_back respectively.
        CIEL_PRECONDITION(pos == begin_ || pos == end_);

        // If either dest or src is an invalid or null pointer, memcpy's behavior is undefined, even if count is zero.
        if (begin_cap_) {
            const size_type front_count = pos - begin_;
            const size_type back_count  = end_ - pos;

            CIEL_PRECONDITION(sb.front_spare() >= front_count);
            CIEL_PRECONDITION(sb.back_spare() >= back_count);

            if (is_trivially_relocatable<value_type>::value) {
                sb.begin_ -= front_count;
                ciel::memcpy(ciel::to_address(sb.begin_), ciel::to_address(begin_), sizeof(value_type) * front_count);

                ciel::memcpy(ciel::to_address(sb.end_), ciel::to_address(pos), sizeof(value_type) * back_count);
                sb.end_ += back_count;

            } else {
                for (pointer p = pos - 1; p >= begin_; --p) {
                    sb.unchecked_emplace_front_aux(ciel::move_if_noexcept(*p));
                }

                for (pointer p = pos; p < end_; ++p) {
                    sb.unchecked_emplace_back_aux(ciel::move_if_noexcept(*p));
                }

                clear();
            }

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

        ciel::memmove(begin_ - n, begin_, sizeof(value_type) * size());
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
                construct(new_end, std::move(*(begin_ + i)));
                ++new_end;
                rd.advance_forward();
            }

            for (; i < old_size; ++i) {
                *new_end = std::move(*(begin_ + i));
                ++new_end;
            }

            end_   = destroy(new_end, end_);
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
                construct(new_end, std::move(*(begin_ + i)));
                ++new_end;
                rd.advance_forward();
            }

            destroy(begin_, end_);
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

        ciel::memmove(begin_ + n, begin_, sizeof(value_type) * size());
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
                construct(--new_begin, std::move(*(end_ - i)));
                rd.advance_backward();
            }

            for (; i <= old_size; ++i) {
                *(--new_begin) = std::move(*(end_ - i));
            }

            destroy(begin_, new_begin);
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
                construct(--new_begin, std::move(*(end_ - i)));
                rd.advance_backward();
            }

            destroy(begin_, end_);
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
    set_nullptr() noexcept {
        begin_cap_ = nullptr;
        begin_     = nullptr;
        end_       = nullptr;
        end_cap_() = nullptr;
    }

    void
    reset(const size_type count) {
        CIEL_PRECONDITION(count != 0);

        do_destroy();
        set_nullptr(); // It's neccessary since allocation would throw.
        init(count);
    }

    void
    reset() noexcept {
        do_destroy();
        set_nullptr();
    }

    void
    init(const size_type count) {
        CIEL_PRECONDITION(count != 0);
        CIEL_PRECONDITION(begin_cap_ == nullptr);
        CIEL_PRECONDITION(begin_ == nullptr);
        CIEL_PRECONDITION(end_ == nullptr);
        CIEL_PRECONDITION(end_cap_() == nullptr);

        begin_cap_ = alloc_traits::allocate(allocator_(), count);
        end_cap_() = begin_cap_ + count;
        begin_     = begin_cap_;
        end_       = begin_;
    }

    void
    reserve_cap_and_offset_to(const size_type cap, const size_type offset) {
        CIEL_PRECONDITION(begin_cap_ == nullptr);
        CIEL_PRECONDITION(begin_ == nullptr);
        CIEL_PRECONDITION(end_ == nullptr);
        CIEL_PRECONDITION(end_cap_() == nullptr);
        CIEL_PRECONDITION(cap != 0);
        CIEL_PRECONDITION(cap >= offset);

        begin_cap_ = alloc_traits::allocate(allocator_(), cap);
        end_cap_() = begin_cap_ + cap;
        begin_     = begin_cap_ + offset;
        end_       = begin_;
    }

    void
    move_range(pointer from_s, pointer from_e, pointer to) {
        pointer old_end   = end_;
        difference_type n = old_end - to;

        for (pointer p = from_s + n; p < from_e; ++p) {
            unchecked_emplace_back_aux(std::move(*p));
        }

        std::move_backward(from_s, from_s + n, old_end);
    }

    // Comparing with vector growing factor: get n * 2 memory, move n elements and get n new space,
    // it's terrible if we shift one (move n elements) to get 1 vacant space for emplace,
    // so only if there is plenty of space at other side will we consider shifting.
    // This situation may be seen when it's used as queue's base container.
    template<class... Args>
    void
    emplace_back_aux(Args&&... args) {
        if (back_spare() == 0) {
            if CIEL_UNLIKELY (front_spare() > size()) { // move size elements to get more than size / 2 vacant space
                // To support self reference operations like v.emplace_back(v[0]),
                // we must construct temp object here and move it afterwards.
                value_type tmp(std::forward<Args>(args)...);
                left_shift_n(std::max<size_type>(front_spare() / 2, 1));
                unchecked_emplace_back_aux(std::move(tmp));

            } else {
                split_buffer<value_type, allocator_type&> sb(allocator_());
                // end_ - begin_cap_ == front_spare() + size()
                sb.reserve_cap_and_offset_to(recommend_cap(end_ - begin_cap_ + 1), end_ - begin_cap_);
                sb.unchecked_emplace_back_aux(std::forward<Args>(args)...);
                swap_out_buffer(std::move(sb), end_);
            }

        } else {
            unchecked_emplace_back_aux(std::forward<Args>(args)...);
        }
    }

    // Check out emplace_back_aux for annotations.
    template<class... Args>
    void
    emplace_front_aux(Args&&... args) {
        if (front_spare() == 0) {
            if CIEL_UNLIKELY (back_spare() > size()) {
                value_type tmp(std::forward<Args>(args)...);
                right_shift_n(std::max<size_type>(back_spare() / 2, 1));
                unchecked_emplace_front_aux(std::move(tmp));

            } else {
                split_buffer<value_type, allocator_type&> sb(allocator_());
                // end_cap_() - begin_ == back_spare() + size()
                const size_type new_cap = recommend_cap(end_cap_() - begin_ + 1);
                sb.reserve_cap_and_offset_to(new_cap, new_cap - (end_cap_() - begin_));
                sb.unchecked_emplace_front_aux(std::forward<Args>(args)...);
                swap_out_buffer(std::move(sb), begin_);
            }

        } else {
            unchecked_emplace_front_aux(std::forward<Args>(args)...);
        }
    }

    template<class... Args>
    void
    unchecked_emplace_back_aux(Args&&... args) {
        CIEL_PRECONDITION(end_ < end_cap_());

        construct(end_, std::forward<Args>(args)...);
        ++end_;
    }

    template<class... Args>
    void
    unchecked_emplace_front_aux(Args&&... args) {
        CIEL_PRECONDITION(begin_cap_ < begin_);

        construct(begin_ - 1, std::forward<Args>(args)...);
        --begin_;
    }

    void
    copy_assign_alloc(const split_buffer& other, std::true_type) {
        if (allocator_() != other.allocator_()) {
            reset();
        }

        allocator_() = other.allocator_();
    }

    void
    copy_assign_alloc(const split_buffer&, std::false_type) const noexcept {}

public:
    split_buffer() noexcept(noexcept(allocator_type())) = default;

    explicit split_buffer(allocator_type& alloc) noexcept
        : end_cap_alloc_(nullptr, alloc) {}

    explicit split_buffer(const allocator_type& alloc) noexcept
        : end_cap_alloc_(nullptr, alloc) {}

    split_buffer(const size_type count, lvalue value, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        if CIEL_LIKELY (count > 0) {
            init(count);
            construct_at_end(count, value);
        }
    }

    explicit split_buffer(const size_type count, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        if CIEL_LIKELY (count > 0) {
            init(count);
            construct_at_end(count);
        }
    }

    template<class Iter, typename std::enable_if<is_exactly_input_iterator<Iter>::value, int>::type = 0>
    split_buffer(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        for (; first != last; ++first) {
            emplace_back(*first);
        }
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    split_buffer(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        const auto count = std::distance(first, last);

        if CIEL_LIKELY (count > 0) {
            init(count);
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

    split_buffer(split_buffer&& other, const allocator_type& alloc)
        : split_buffer(alloc) {
        if (allocator_() == other.get_allocator()) {
            begin_cap_ = other.begin_cap_;
            begin_     = other.begin_;
            end_       = other.end_;
            end_cap_() = other.end_cap_();

            other.set_nullptr();

        } else if (other.size() > 0) {
            init(other.size());
            construct_at_end(other.begin(), other.end());
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
            init(count);
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
            init(count);
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

        copy_assign_alloc(other, typename alloc_traits::propagate_on_container_copy_assignment{});
        assign(other.begin(), other.end());

        return *this;
    }

    split_buffer&
    operator=(split_buffer&& other) noexcept(alloc_traits::propagate_on_container_move_assignment::value
                                             || alloc_traits::is_always_equal::value) {
        if CIEL_UNLIKELY (this == std::addressof(other)) {
            return *this;
        }

        if (alloc_traits::propagate_on_container_move_assignment::value || allocator_() == other.allocator_()) {
            swap(other);

        } else {
            assign(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
        }

        return *this;
    }

    split_buffer&
    operator=(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    void
    assign(const size_type count, lvalue value) {
        if (back_spare() + size() < count) {
            if (internal_value(value, begin_)) {
                value_type copy = std::move(*(begin_ + (std::addressof(value) - begin_)));
                reset(count);
                construct_at_end(count, copy);

            } else {
                reset(count);
                construct_at_end(count, value);
            }

            return;
        }

        if (count >= size()) {
            std::fill_n(begin_, size(), value);
            construct_at_end(count - size(), value);

        } else {
            std::fill_n(begin_, count, value);
            end_ = destroy(begin_ + count, end_);
        }

        CIEL_POSTCONDITION(size() == count);
    }

private:
    template<class Iter>
    void
    assign(Iter first, Iter last, const size_type count) {
        if (back_spare() + size() < count) {
            const size_type diff = count - back_spare() - size();

            if (front_spare() >= diff) {
                left_shift_n(diff);

            } else {
                reset(count);
                construct_at_end(first, last);
                return;
            }
        }

        if (size() > count) {
            end_ = destroy(begin_ + count, end_);
            ciel::copy_n(first, count, begin_); // count == size()

        } else {
            Iter mid = ciel::copy_n(first, size(), begin_);
            construct_at_end(mid, last);
        }

        CIEL_POSTCONDITION(size() == count);
    }

public:
    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    void
    assign(Iter first, Iter last) {
        const size_type count = std::distance(first, last);

        assign(first, last, count);
    }

    template<class Iter, typename std::enable_if<is_exactly_input_iterator<Iter>::value, int>::type = 0>
    void
    assign(Iter first, Iter last) {
        pointer p = begin_;
        for (; first != last && p != end_; ++first, ++p) {
            *p = *first;
        }

        if (p != end_) {
            end_ = destroy(p, end_);

        } else {
            for (; first != last; ++first) {
                emplace_back_aux(*first);
            }
        }
    }

    void
    assign(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end());
    }

    allocator_type
    get_allocator() const noexcept {
        return allocator_();
    }

    CIEL_NODISCARD reference
    at(const size_type pos) {
        if CIEL_UNLIKELY (pos >= size()) {
            CIEL_THROW_EXCEPTION(std::out_of_range("pos is not within the range of ciel::split_buffer"));
        }

        return begin_[pos];
    }

    CIEL_NODISCARD const_reference
    at(const size_type pos) const {
        if CIEL_UNLIKELY (pos >= size()) {
            CIEL_THROW_EXCEPTION(std::out_of_range("pos is not within the range of ciel::split_buffer"));
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
        return ciel::to_address(begin_);
    }

    CIEL_NODISCARD const T*
    data() const noexcept {
        return ciel::to_address(begin_);
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
        end_ = destroy(begin_, end_);
    }

    void
    push_back(lvalue value) {
        emplace_back(value);
    }

    template<bool Valid = !should_pass_by_value, typename std::enable_if<Valid, int>::type = 0>
    void
    push_back(rvalue value) {
        emplace_back(std::move(value));
    }

    template<class... Args>
    reference
    emplace_back(Args&&... args) {
        emplace_back_aux(std::forward<Args>(args)...);

        return back();
    }

    template<class U, class... Args>
    reference
    emplace_back(std::initializer_list<U> il, Args&&... args) {
        emplace_back_aux(il, std::forward<Args>(args)...);

        return back();
    }

    void
    pop_back() noexcept {
        CIEL_PRECONDITION(!empty());

        --end_;
        destroy(end_);
    }

    void
    push_front(lvalue value) {
        emplace_front(value);
    }

    template<bool Valid = !should_pass_by_value, typename std::enable_if<Valid, int>::type = 0>
    void
    push_front(rvalue value) {
        emplace_front(std::move(value));
    }

    template<class... Args>
    reference
    emplace_front(Args&&... args) {
        emplace_front_aux(std::forward<Args>(args)...);

        return front();
    }

    template<class U, class... Args>
    reference
    emplace_front(std::initializer_list<U> il, Args&&... args) {
        emplace_front_aux(il, std::forward<Args>(args)...);

        return front();
    }

    void
    pop_front() noexcept {
        CIEL_PRECONDITION(!empty());

        destroy(begin_);
        ++begin_;
    }

    void
    resize(const size_type count) {
        if (size() >= count) {
            end_ = destroy(begin_ + count, end_);

        } else {
            reserve_back_spare(count - size());
            construct_at_end(count - size());
        }
    }

    void
    resize(const size_type count, lvalue value) {
        if (size() >= count) {
            end_ = destroy(begin_ + count, end_);

        } else if (count > size() + back_spare()) {
            split_buffer<value_type, allocator_type&> sb(allocator_());
            sb.reserve_cap_and_offset_to(front_spare() + count, front_spare() + size());
            sb.construct_at_end(count - size(), value);
            swap_out_buffer(std::move(sb), end_);

        } else {
            construct_at_end(count - size(), value);
        }
    }

    void
    swap(split_buffer& other) noexcept(alloc_traits::propagate_on_container_swap::value
                                       || alloc_traits::is_always_equal::value) {
        using std::swap;

        swap(begin_cap_, other.begin_cap_);
        swap(begin_, other.begin_);
        swap(end_, other.end_);
        swap(end_cap_(), other.end_cap_());

        if (alloc_traits::propagate_on_container_swap::value) {
            swap(allocator_(), other.allocator_());
        }
    }

    template<class... Args>
    reference
    unchecked_emplace_back(Args&&... args) {
        unchecked_emplace_back_aux(std::forward<Args>(args)...);

        return back();
    }

    template<class U, class... Args>
    reference
    unchecked_emplace_back(std::initializer_list<U> il, Args&&... args) {
        unchecked_emplace_back_aux(il, std::forward<Args>(args)...);

        return back();
    }

    template<class... Args>
    reference
    unchecked_emplace_front(Args&&... args) {
        unchecked_emplace_front_aux(std::forward<Args>(args)...);

        return front();
    }

    template<class U, class... Args>
    reference
    unchecked_emplace_front(std::initializer_list<U> il, Args&&... args) {
        unchecked_emplace_front_aux(il, std::forward<Args>(args)...);

        return front();
    }

    template<class R, typename std::enable_if<is_range<R>::value, int>::type = 0>
    void
    assign_range(R&& rg) {
        if (is_range_with_size<R>::value) {
            if (std::is_lvalue_reference<R>::value) {
                assign(rg.begin(), rg.end(), rg.size());

            } else {
                assign(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()), rg.size());
            }

        } else {
            if (std::is_lvalue_reference<R>::value) {
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

#ifndef CIELLAB_INCLUDE_CIEL_VECTOR_HPP_
#define CIELLAB_INCLUDE_CIEL_VECTOR_HPP_

#include <ciel/alignment.hpp>
#include <ciel/allocator_traits.hpp>
#include <ciel/compare.hpp>
#include <ciel/compressed_pair.hpp>
#include <ciel/config.hpp>
#include <ciel/copy_n.hpp>
#include <ciel/is_range.hpp>
#include <ciel/is_trivially_relocatable.hpp>
#include <ciel/iterator_category.hpp>
#include <ciel/range_destroyer.hpp>
#include <ciel/split_buffer.hpp>
#include <ciel/swap.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

template<class T, class Allocator = std::allocator<T>>
class vector {
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

    pointer begin_{nullptr};
    pointer end_{nullptr};
    ciel::compressed_pair<pointer, allocator_type> end_cap_alloc_{nullptr, ciel::value_init};

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
            ciel::throw_exception(std::length_error("ciel::vector reserving size is beyond max_size"));
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
            unchecked_emplace_back_aux();
        }
    }

    void
    construct_at_end(const size_type n, const value_type& value) {
        CIEL_PRECONDITION(end_ + n <= end_cap_());

        for (size_type i = 0; i < n; ++i) {
            unchecked_emplace_back_aux(value);
        }
    }

    template<class Iter, typename std::enable_if<ciel::is_forward_iterator<Iter>::value, int>::type = 0>
    void
    construct_at_end(Iter first, Iter last) {
        CIEL_PRECONDITION(end_ + std::distance(first, last) <= end_cap_());

        while (first != last) {
            unchecked_emplace_back_aux(*first);
            ++first;
        }
    }

    template<class... Args>
    void
    construct(pointer p, Args&&... args) {
        if (ciel::allocator_has_trivial_construct<allocator_type, pointer, Args...>::value) {
            new (p) value_type(std::forward<Args>(args)...);

        } else {
            alloc_traits::construct(allocator_(), p, std::forward<Args>(args)...);
        }
    }

    template<class U = value_type, typename std::enable_if<std::is_trivial<U>::value, int>::type = 0>
    void
    construct(pointer p, value_type value) {
        if (ciel::allocator_has_trivial_copy_construct<allocator_type>::value) {
            *p = value;

        } else {
            alloc_traits::construct(allocator_(), p, value);
        }
    }

    template<class U = value_type, typename std::enable_if<!std::is_trivial<U>::value, int>::type = 0>
    void
    construct(pointer p, const value_type& value) {
        if (ciel::allocator_has_trivial_copy_construct<allocator_type>::value) {
            new (p) value_type(value);

        } else {
            alloc_traits::construct(allocator_(), p, value);
        }
    }

    pointer
    destroy(pointer first, pointer last) noexcept {
        CIEL_PRECONDITION(begin_ <= first);
        CIEL_PRECONDITION(first <= last);
        CIEL_PRECONDITION(last <= end_);

        const pointer res = first;

        if (ciel::allocator_has_trivial_destroy<allocator_type>::value) {
            if (!std::is_trivially_destructible<value_type>::value) {
                for (; last - first >= 4; first += 4) {
                    (first + 0)->~value_type();
                    (first + 1)->~value_type();
                    (first + 2)->~value_type();
                    (first + 3)->~value_type();
                }

                for (; first != last; ++first) {
                    first->~value_type();
                }
            }

        } else {
            for (; first != last; ++first) {
                alloc_traits::destroy(allocator_(), first);
            }
        }

        return res;
    }

    void
    destroy(pointer p) noexcept {
        CIEL_PRECONDITION(begin_ <= p);
        CIEL_PRECONDITION(p <= end_); // called by pop_back

        if (ciel::allocator_has_trivial_destroy<allocator_type>::value) {
            if (!std::is_trivially_destructible<value_type>::value) {
                p->~value_type();
            }

        } else {
            alloc_traits::destroy(allocator_(), p);
        }
    }

    template<class U = value_type, typename std::enable_if<ciel::is_trivially_relocatable<U>::value, int>::type = 0>
    void
    swap_out_buffer(ciel::split_buffer<value_type, allocator_type&>&& sb, pointer pos) noexcept {
        // If either dest or src is an invalid or null pointer, memcpy's behavior is undefined, even if count is zero.
        if (begin_) {
            const size_type front_count = pos - begin_;
            const size_type back_count  = end_ - pos;

            CIEL_PRECONDITION(sb.front_spare() == front_count);
            CIEL_PRECONDITION(sb.back_spare() >= back_count);

            std::memcpy(sb.begin_cap_, begin_, sizeof(value_type) * front_count);
            // sb.begin_ = sb.begin_cap_;

            std::memcpy(sb.end_, pos, sizeof(value_type) * back_count);
            sb.end_ += back_count;

            alloc_traits::deallocate(allocator_(), begin_, capacity());
        }

        begin_     = sb.begin_cap_;
        end_       = sb.end_;
        end_cap_() = sb.end_cap_();

        sb.set_nullptr();
    }

    template<class U = value_type, typename std::enable_if<!ciel::is_trivially_relocatable<U>::value, int>::type = 0>
    void
    swap_out_buffer(ciel::split_buffer<value_type, allocator_type&>&& sb,
                    pointer pos) noexcept(std::is_nothrow_move_constructible<value_type>::value) {
        if (begin_) {
            const size_type front_count = pos - begin_;
            const size_type back_count  = end_ - pos;

            CIEL_PRECONDITION(sb.front_spare() == front_count);
            CIEL_PRECONDITION(sb.back_spare() >= back_count);

            for (pointer p = pos - 1; p >= begin_; --p) {
#ifdef CIEL_HAS_EXCEPTIONS
                sb.unchecked_emplace_front_aux(std::move_if_noexcept(*p));
#else
                sb.unchecked_emplace_front_aux(std::move(*p));
#endif
            }

            for (pointer p = pos; p < end_; ++p) {
#ifdef CIEL_HAS_EXCEPTIONS
                sb.unchecked_emplace_back_aux(std::move_if_noexcept(*p));
#else
                sb.unchecked_emplace_back_aux(std::move(*p));
#endif
            }

            clear();
            alloc_traits::deallocate(allocator_(), begin_, capacity());
        }

        CIEL_POSTCONDITION(sb.begin_ == sb.begin_cap_);

        begin_     = sb.begin_cap_;
        end_       = sb.end_;
        end_cap_() = sb.end_cap_();

        sb.set_nullptr();
    }

    template<class U = value_type, typename std::enable_if<ciel::is_trivially_relocatable<U>::value, int>::type = 0>
    void
    swap_out_buffer(ciel::split_buffer<value_type, allocator_type&>&& sb) noexcept {
        CIEL_PRECONDITION(sb.front_spare() == size());

        // If either dest or src is an invalid or null pointer, memcpy's behavior is undefined, even if count is zero.
        if (begin_) {
            std::memcpy(sb.begin_cap_, begin_, sizeof(value_type) * size());
            // sb.begin_ = sb.begin_cap_;

            alloc_traits::deallocate(allocator_(), begin_, capacity());
        }

        begin_     = sb.begin_cap_;
        end_       = sb.end_;
        end_cap_() = sb.end_cap_();

        sb.set_nullptr();
    }

    template<class U = value_type, typename std::enable_if<!ciel::is_trivially_relocatable<U>::value, int>::type = 0>
    void
    swap_out_buffer(ciel::split_buffer<value_type, allocator_type&>&& sb) noexcept(
        std::is_nothrow_move_constructible<value_type>::value) {
        CIEL_PRECONDITION(sb.front_spare() == size());

        if (begin_) {
            for (pointer p = end_ - 1; p >= begin_; --p) {
#ifdef CIEL_HAS_EXCEPTIONS
                sb.unchecked_emplace_front_aux(std::move_if_noexcept(*p));
#else
                sb.unchecked_emplace_front_aux(std::move(*p));
#endif
            }

            clear();
            alloc_traits::deallocate(allocator_(), begin_, capacity());
        }

        CIEL_POSTCONDITION(sb.begin_ == sb.begin_cap_);

        begin_     = sb.begin_cap_;
        end_       = sb.end_;
        end_cap_() = sb.end_cap_();

        sb.set_nullptr();
    }

    void
    do_destroy() noexcept {
        if (begin_) {
            clear();
            alloc_traits::deallocate(allocator_(), begin_, capacity());
        }
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

    void
    set_nullptr() noexcept {
        begin_     = nullptr;
        end_       = nullptr;
        end_cap_() = nullptr;
    }

    template<class U = value_type, typename std::enable_if<ciel::is_trivially_relocatable<U>::value, int>::type = 0>
    iterator
    erase_impl(iterator first, iterator last, const size_type distance) noexcept {
        CIEL_PRECONDITION(first < last);
        CIEL_PRECONDITION(distance != 0);

        const auto index      = first - begin();
        const auto back_count = end() - last;

        destroy(first, last);
        std::memmove(first, last, sizeof(value_type) * back_count);
        end_ -= distance;

        return begin() + index;
    }

    template<class U = value_type, typename std::enable_if<!ciel::is_trivially_relocatable<U>::value, int>::type = 0>
    iterator
    erase_impl(iterator first, iterator last, const size_type) {
        CIEL_PRECONDITION(first < last);

        const auto index = first - begin();

        iterator new_end = std::move(last, end(), first);
        end_             = destroy(new_end, end_);

        return begin() + index;
    }

    template<class T1, class T2, class U = value_type,
             typename std::enable_if<ciel::is_trivially_relocatable<U>::value, int>::type = 0>
    void
    insert_impl(iterator pos, T1&& t1, T2&& t2, const size_type count) {
        // ------------------------------------
        // begin                  pos       end
        //                       ----------
        //                       first last
        //                       |  count |
        // relocate [pos, end) count units later
        std::memmove(pos + count, pos, sizeof(value_type) * (end_ - pos));
        // ----------------------          --------------
        // begin             new_end       pos    |   end
        //                       ----------       |
        //                       first last      range_destroyer in case of exceptions
        //                       |  count |
        ciel::range_destroyer<value_type, allocator_type&> rd{pos + count, end_ + count, allocator_()};
        const pointer old_end = end_;
        end_                  = pos;
        // ----------------------------------------------
        // begin             first        last        end
        //                                 pos
        //                               new_end
        construct_at_end(std::forward<T1>(t1), std::forward<T2>(t2));
        // new_end
        end_ = old_end + count;
        rd.release();
    }

    template<class Iter, class U = value_type,
             typename std::enable_if<
                 ciel::is_forward_iterator<Iter>::value && !ciel::is_trivially_relocatable<U>::value, int>::type
             = 0>
    void
    insert_impl(iterator pos, Iter first, Iter last, size_type count) {
        CIEL_PRECONDITION(count != 0);

        const size_type old_count        = count;
        pointer old_end                  = end_;
        auto mid                         = std::next(first, count);
        const size_type pos_end_distance = end_ - pos;

        if (count > pos_end_distance) {
            mid = std::next(first, pos_end_distance);
            construct_at_end(mid, last);

            count = pos_end_distance;
        }

        if (count > 0) {
            move_range(pos, old_end, pos + old_count);

            std::copy(first, mid, pos);
        }
    }

    template<class U = value_type, typename std::enable_if<!ciel::is_trivially_relocatable<U>::value, int>::type = 0>
    void
    insert_impl(iterator pos, size_type count, const value_type& value, const size_type old_count) {
        pointer old_end = end_;

        const size_type pos_end_distance = end_ - pos;

        if (count > pos_end_distance) {
            const size_type n = count - pos_end_distance;
            construct_at_end(n, value);

            count = pos_end_distance;
        }

        if (count > 0) {
            move_range(pos, old_end, pos + old_count);

            std::fill_n(pos, count, value);
        }
    }

    class insert_impl_callback {
    private:
        vector* const this_;

    public:
        insert_impl_callback(vector* const t) noexcept
            : this_{t} {}

        template<class... Args, class U = value_type,
                 typename std::enable_if<ciel::is_trivially_relocatable<U>::value, int>::type = 0>
        void
        operator()(iterator pos, Args&&... args) const {
            constexpr size_type count = 1;
            std::memmove(pos + count, pos, sizeof(value_type) * (this_->end_ - pos));

            ciel::range_destroyer<value_type, allocator_type&> rd{pos + count, this_->end_ + count,
                                                                  this_->allocator_()};
            const pointer old_end = this_->end_;
            this_->end_           = pos;

            this_->unchecked_emplace_back_aux(std::forward<Args>(args)...);

            this_->end_ = old_end + count;
            rd.release();
        }

        template<class... Args, class U = value_type,
                 typename std::enable_if<!ciel::is_trivially_relocatable<U>::value, int>::type = 0>
        void
        operator()(iterator pos, Args&&... args) const {
            this_->move_range(pos, this_->end_, pos + 1);
            *pos = value_type{std::forward<Args>(args)...};
        }

        template<class U, class URaw = typename std::decay<U>::type,
                 typename std::enable_if<
                     std::is_same<value_type, URaw>::value && !ciel::is_trivially_relocatable<URaw>::value, int>::type
                 = 0>
        void
        operator()(iterator pos, U&& value) const {
            this_->move_range(pos, this_->end_, pos + 1);
            *pos = std::forward<U>(value);
        }
    };

    template<class... Args>
    iterator
    emplace_impl(const insert_impl_callback cb, iterator pos, Args&&... args) {
        CIEL_PRECONDITION(begin() <= pos);
        CIEL_PRECONDITION(pos <= end());

        const size_type pos_index = pos - begin();

        if (end_ == end_cap_()) { // expansion
            ciel::split_buffer<value_type, allocator_type&> sb(allocator_());
            sb.reserve_cap_and_offset_to(recommend_cap(size() + 1), pos_index);

            sb.unchecked_emplace_back_aux(std::forward<Args>(args)...);

            swap_out_buffer(std::move(sb), pos);

        } else if (pos == end_) { // equal to emplace_back
            unchecked_emplace_back_aux(std::forward<Args>(args)...);

        } else {
            cb(pos, std::forward<Args>(args)...);
        }

        return begin() + pos_index;
    }

    template<class... Args>
    void
    emplace_back_aux(Args&&... args) {
        if (end_ == end_cap_()) {
            ciel::split_buffer<value_type, allocator_type&> sb(allocator_());
            sb.reserve_cap_and_offset_to(recommend_cap(size() + 1), size());

            sb.unchecked_emplace_back_aux(std::forward<Args>(args)...);

            swap_out_buffer(std::move(sb));

        } else {
            return unchecked_emplace_back_aux(std::forward<Args>(args)...);
        }
    }

    template<class... Args>
    void
    unchecked_emplace_back_aux(Args&&... args) {
        CIEL_PRECONDITION(end_ < end_cap_());

        construct(end_, std::forward<Args>(args)...);
        ++end_;
    }

    template<class Iter>
    void
    assign(Iter first, Iter last, const size_type count) {
        if (capacity() < count) {
            vector{first, last, allocator_()}.swap(*this);
            return;
        }

        if (size() > count) {
            end_ = destroy(begin_ + count, end_);
        }

        CIEL_POSTCONDITION(size() <= count);

        Iter mid = ciel::copy_n(first, size(), begin_);

        // if mid < last
        construct_at_end(mid, last);

        CIEL_POSTCONDITION(size() == count);
    }

    template<class Iter>
    iterator
    insert(const_iterator p, Iter first, Iter last, size_type count) {
        iterator pos = begin() + (p - begin());
        CIEL_PRECONDITION(begin() <= pos);
        CIEL_PRECONDITION(pos <= end());

        if CIEL_UNLIKELY (count == 0) {
            return pos;
        }

        const size_type pos_index = pos - begin();

        if (count + size() > capacity()) { // expansion
            ciel::split_buffer<value_type, allocator_type&> sb(allocator_());
            sb.reserve_cap_and_offset_to(recommend_cap(count + size()), pos_index);

            sb.construct_at_end(first, last);

            swap_out_buffer(std::move(sb), pos);

        } else { // enough back space
            insert_impl(pos, first, last, count);
        }

        return begin() + pos_index;
    }

#if defined(_LIBCPP_VECTOR) || defined(_GLIBCXX_VECTOR)
    struct std_vector_rob {
        static_assert(!std::is_same<value_type, bool>::value, "");

        static constexpr bool is_ebo_optimized
            = std::is_empty<allocator_type>::value && !ciel::is_final<allocator_type>::value;

        pointer* const begin_ptr;
        pointer* const end_ptr;
        pointer* const end_cap_ptr;
        allocator_type* const alloc_ptr;

        std_vector_rob(std::vector<value_type, allocator_type>&& other) noexcept
#if defined(_LIBCPP_VECTOR)
            : begin_ptr((pointer*)(&other)),
              end_ptr(begin_ptr + 1),
              end_cap_ptr(end_ptr + 1),
              alloc_ptr(is_ebo_optimized
                            ? (allocator_type*)end_cap_ptr
                            : (allocator_type*)ciel::align_up((uintptr_t)(end_cap_ptr + 1), alignof(allocator_type)))
#elif defined(_GLIBCXX_VECTOR)
            : begin_ptr(is_ebo_optimized
                            ? (pointer*)(&other)
                            : (pointer*)ciel::align_up((uintptr_t)(&other) + ciel::datasizeof<allocator_type>::value,
                                                       alignof(pointer))),
              end_ptr(begin_ptr + 1),
              end_cap_ptr(end_ptr + 1),
              alloc_ptr((allocator_type*)(&other))
#endif
        {
            CIEL_PRECONDITION(other.data() == *begin_ptr);
            CIEL_PRECONDITION(other.size() == static_cast<size_t>(*end_ptr - *begin_ptr));
            CIEL_PRECONDITION(other.capacity() == static_cast<size_t>(*end_cap_ptr - *begin_ptr));
        }

    }; // struct std_vector_rob
#endif

public:
    vector() noexcept(noexcept(allocator_type())) = default;

    explicit vector(const allocator_type& alloc) noexcept
        : end_cap_alloc_(nullptr, alloc) {}

    vector(const size_type count, const value_type& value, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        if CIEL_LIKELY (count > 0) {
            begin_     = alloc_traits::allocate(allocator_(), count);
            end_cap_() = begin_ + count;
            end_       = begin_;

            construct_at_end(count, value);
        }
    }

    explicit vector(const size_type count, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        if CIEL_LIKELY (count > 0) {
            begin_     = alloc_traits::allocate(allocator_(), count);
            end_cap_() = begin_ + count;
            end_       = begin_;

            construct_at_end(count);
        }
    }

    template<class Iter, typename std::enable_if<ciel::is_exactly_input_iterator<Iter>::value, int>::type = 0>
    vector(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        while (first != last) {
            emplace_back_aux(*first);
            ++first;
        }
    }

    template<class Iter, typename std::enable_if<ciel::is_forward_iterator<Iter>::value, int>::type = 0>
    vector(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        const auto count = std::distance(first, last);

        if CIEL_LIKELY (count > 0) {
            begin_     = alloc_traits::allocate(allocator_(), count);
            end_cap_() = begin_ + count;
            end_       = begin_;

            construct_at_end(first, last);
        }
    }

    vector(const vector& other)
        : vector(other.begin(), other.end(),
                 alloc_traits::select_on_container_copy_construction(other.get_allocator())) {}

    vector(const vector& other, const allocator_type& alloc)
        : vector(other.begin(), other.end(), alloc) {}

    vector(vector&& other) noexcept
        : begin_(other.begin_), end_(other.end_), end_cap_alloc_(other.end_cap_(), std::move(other.allocator_())) {
        other.set_nullptr();
    }

    vector(vector&& other, const allocator_type& alloc) {
        if (alloc == other.get_allocator()) {
            allocator_() = alloc;
            begin_       = other.begin_;
            end_         = other.end_;
            end_cap_()   = other.end_cap_();

            other.set_nullptr();

        } else {
            vector(other, alloc).swap(*this);
        }
    }

    vector(std::initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
        : vector(init.begin(), init.end(), alloc) {}

    template<class R, typename std::enable_if<
                          ciel::is_range_without_size<R>::value && std::is_lvalue_reference<R>::value, int>::type
                      = 0>
    vector(ciel::from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : vector(rg.begin(), rg.end(), alloc) {}

    template<class R, typename std::enable_if<
                          ciel::is_range_without_size<R>::value && !std::is_lvalue_reference<R>::value, int>::type
                      = 0>
    vector(ciel::from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : vector(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()), alloc) {}

    template<class R, typename std::enable_if<ciel::is_range_with_size<R>::value && std::is_lvalue_reference<R>::value,
                                              int>::type
                      = 0>
    vector(ciel::from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        const auto count = rg.size();

        if CIEL_LIKELY (count > 0) {
            begin_     = alloc_traits::allocate(allocator_(), count);
            end_cap_() = begin_ + count;
            end_       = begin_;

            construct_at_end(rg.begin(), rg.end());
        }
    }

    template<class R, typename std::enable_if<ciel::is_range_with_size<R>::value && !std::is_lvalue_reference<R>::value,
                                              int>::type
                      = 0>
    vector(ciel::from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        const auto count = rg.size();

        if CIEL_LIKELY (count > 0) {
            begin_     = alloc_traits::allocate(allocator_(), count);
            end_cap_() = begin_ + count;
            end_       = begin_;

            construct_at_end(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()));
        }
    }

#if defined(_LIBCPP_VECTOR) || defined(_GLIBCXX_VECTOR)
    template<class U = value_type, typename std::enable_if<!std::is_same<U, bool>::value, int>::type = 0>
    vector(std::vector<value_type, allocator_type>&& other) noexcept {
        std_vector_rob svr(std::move(other));

        allocator_()     = std::move(*svr.alloc_ptr);
        begin_           = *svr.begin_ptr;
        end_             = *svr.end_ptr;
        end_cap_()       = *svr.end_cap_ptr;
        *svr.begin_ptr   = nullptr;
        *svr.end_ptr     = nullptr;
        *svr.end_cap_ptr = nullptr;

        CIEL_POSTCONDITION(other.size() == 0);
        CIEL_POSTCONDITION(other.capacity() == 0);
    }
#endif

    ~vector() {
        do_destroy();
    }

    vector&
    operator=(const vector& other) {
        if CIEL_UNLIKELY (this == std::addressof(other)) {
            return *this;
        }

        if (alloc_traits::propagate_on_container_copy_assignment::value) {
            if (allocator_() != other.allocator_()) {
                vector(other.begin(), other.end(), other.allocator_()).swap(*this);
                return *this;
            }

            allocator_() = other.allocator_();
        }

        assign(other.begin(), other.end());

        return *this;
    }

    vector&
    operator=(vector&& other) noexcept(alloc_traits::propagate_on_container_move_assignment::value
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

        begin_     = other.begin_;
        end_       = other.end_;
        end_cap_() = other.end_cap_();

        other.set_nullptr();

        return *this;
    }

    vector&
    operator=(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    void
    assign(const size_type count, const value_type& value) {
        if (capacity() < count) {
            vector{count, value, allocator_()}.swap(*this);
            return;
        }

        if (size() > count) {
            end_ = destroy(begin_ + count, end_);
        }

        CIEL_POSTCONDITION(size() <= count);

        std::fill_n(begin_, size(), value);
        // if count > size()
        construct_at_end(count - size(), value);

        CIEL_POSTCONDITION(size() == count);
    }

    template<class Iter, typename std::enable_if<ciel::is_forward_iterator<Iter>::value, int>::type = 0>
    void
    assign(Iter first, Iter last) {
        const size_type count = std::distance(first, last);

        assign(first, last, count);
    }

    template<class Iter, typename std::enable_if<ciel::is_exactly_input_iterator<Iter>::value, int>::type = 0>
    void
    assign(Iter first, Iter last) {
        clear();

        while (first != last) {
            emplace_back_aux(*first);
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
            ciel::throw_exception(std::out_of_range("pos is not within the range of ciel::vector"));
        }

        return begin_[pos];
    }

    CIEL_NODISCARD const_reference
    at(const size_type pos) const {
        if CIEL_UNLIKELY (pos >= size()) {
            ciel::throw_exception(std::out_of_range("pos is not within the range of ciel::vector"));
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
    reserve(const size_type new_cap) {
        if (new_cap <= capacity()) {
            return;
        }

        ciel::split_buffer<value_type, allocator_type&> sb(allocator_());
        sb.reserve_cap_and_offset_to(new_cap, size());

        swap_out_buffer(std::move(sb));
    }

    CIEL_NODISCARD size_type
    capacity() const noexcept {
        return end_cap_() - begin_;
    }

    void
    shrink_to_fit() {
        if CIEL_UNLIKELY (size() == capacity()) {
            return;
        }

        if (size() > 0) {
            ciel::split_buffer<value_type, allocator_type&> sb(allocator_());

            CIEL_TRY {
                sb.reserve_cap_and_offset_to(size(), size());

                swap_out_buffer(std::move(sb));
            }
            CIEL_CATCH (...) {}

        } else {
            alloc_traits::deallocate(allocator_(), begin_, capacity());
            set_nullptr();
        }
    }

    void
    clear() noexcept {
        end_ = destroy(begin_, end_);
    }

    iterator
    insert(const_iterator p, const value_type& value) {
        iterator pos = begin() + (p - begin());
        return emplace_impl(insert_impl_callback{this}, pos, value);
    }

    iterator
    insert(const_iterator p, value_type&& value) {
        iterator pos = begin() + (p - begin());
        return emplace_impl(insert_impl_callback{this}, pos, std::move(value));
    }

    iterator
    insert(const_iterator p, size_type count, const value_type& value) {
        iterator pos = begin() + (p - begin());
        CIEL_PRECONDITION(begin() <= pos);
        CIEL_PRECONDITION(pos <= end());

        const size_type pos_index = pos - begin();

        if (count + size() > capacity()) { // expansion
            ciel::split_buffer<value_type, allocator_type&> sb(allocator_());
            sb.reserve_cap_and_offset_to(recommend_cap(size() + count), pos_index);

            sb.construct_at_end(count, value);

            swap_out_buffer(std::move(sb), pos);

        } else { // enough back space
            insert_impl(pos, count, value, count);
        }

        return begin() + pos_index;
    }

    // We construct all at the end at first, then rotate them to the right place.
    template<class Iter, typename std::enable_if<ciel::is_exactly_input_iterator<Iter>::value, int>::type = 0>
    iterator
    insert(const_iterator p, Iter first, Iter last) {
        iterator pos = begin() + (p - begin());
        // record these index because it may reallocate
        const auto pos_index     = pos - begin();
        const size_type old_size = size();

        while (first != last) {
            emplace_back_aux(*first);
            ++first;
        }

        std::rotate(begin() + pos_index, begin() + old_size, end());
        return begin() + pos_index;
    }

    template<class Iter, typename std::enable_if<ciel::is_forward_iterator<Iter>::value, int>::type = 0>
    iterator
    insert(const_iterator pos, Iter first, Iter last) {
        const size_type count = std::distance(first, last);

        return insert(pos, first, last, count);
    }

    iterator
    insert(const_iterator pos, std::initializer_list<value_type> ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }

    // Note that emplace is not a superset of insert when pos is not at the end.
    template<class... Args>
    iterator
    emplace(const_iterator p, Args&&... args) {
        iterator pos = begin() + (p - begin());
        return emplace_impl(insert_impl_callback{this}, pos, std::forward<Args>(args)...);
    }

    template<class U, class... Args>
    iterator
    emplace(const_iterator p, std::initializer_list<U> il, Args&&... args) {
        iterator pos = begin() + (p - begin());
        return emplace_impl(insert_impl_callback{this}, pos, il, std::forward<Args>(args)...);
    }

    iterator
    erase(const_iterator p) {
        iterator pos = begin() + (p - begin());
        CIEL_PRECONDITION(begin() <= pos);
        CIEL_PRECONDITION(pos < end());

        return erase_impl(pos, pos + 1, 1);
    }

    iterator
    erase(const_iterator f, const_iterator l) {
        iterator first = begin() + (f - begin());
        iterator last  = begin() + (l - begin());
        CIEL_PRECONDITION(begin() <= first);
        CIEL_PRECONDITION(last <= end());

        const auto distance = std::distance(first, last);

        if CIEL_UNLIKELY (distance <= 0) {
            return last;
        }

        return erase_impl(first, last, distance);
    }

    void
    push_back(const value_type& value) {
        emplace_back_aux(value);
    }

    void
    push_back(value_type&& value) {
        emplace_back_aux(std::move(value));
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
    resize(const size_type count) {
        if (size() >= count) {
            end_ = destroy(begin_ + count, end_);
            return;
        }

        reserve(count);

        construct_at_end(count - size());
    }

    void
    resize(const size_type count, const value_type& value) {
        if (size() >= count) {
            end_ = destroy(begin_ + count, end_);
            return;
        }

        reserve(count);

        construct_at_end(count - size(), value);
    }

    void
    swap(vector& other) noexcept(alloc_traits::propagate_on_container_swap::value
                                 || alloc_traits::is_always_equal::value) {
        using std::swap;

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

    template<class R, typename std::enable_if<ciel::is_range<R>::value, int>::type = 0>
    void
    assign_range(R&& rg) {
        if CIEL_CONSTEXPR_SINCE_CXX17 (ciel::is_range_with_size<R>::value) {
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

    template<class R, typename std::enable_if<ciel::is_range<R>::value, int>::type = 0>
    void
    append_range(R&& rg) {
        insert_range(end(), std::forward<R>(rg));
    }

    template<class R, typename std::enable_if<ciel::is_range<R>::value, int>::type = 0>
    iterator
    insert_range(const_iterator pos, R&& rg) {
        if CIEL_CONSTEXPR_SINCE_CXX17 (ciel::is_range_with_size<R>::value
                                       && ciel::is_forward_iterator<decltype(rg.begin())>::value) {
            if CIEL_CONSTEXPR_SINCE_CXX17 (std::is_lvalue_reference<R>::value) {
                return insert(pos, rg.begin(), rg.end(), rg.size());

            } else {
                return insert(pos, std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()), rg.size());
            }

        } else {
            if CIEL_CONSTEXPR_SINCE_CXX17 (std::is_lvalue_reference<R>::value) {
                return insert(pos, rg.begin(), rg.end());

            } else {
                return insert(pos, std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()));
            }
        }
    }

#if defined(_LIBCPP_VECTOR) || defined(_GLIBCXX_VECTOR)
    template<class U = value_type, typename std::enable_if<!std::is_same<U, bool>::value, int>::type = 0>
    operator std::vector<value_type, allocator_type>() && noexcept {
        std::vector<value_type, allocator_type> res;
        std_vector_rob svr(std::move(res));

        *svr.alloc_ptr   = std::move(allocator_());
        *svr.begin_ptr   = begin_;
        *svr.end_ptr     = end_;
        *svr.end_cap_ptr = end_cap_();
        begin_           = nullptr;
        end_             = nullptr;
        end_cap_()       = nullptr;

        return res;
    }
#endif

}; // class vector

template<class T, class Allocator>
struct is_trivially_relocatable<vector<T, Allocator>> : is_trivially_relocatable<Allocator> {};

template<class T, class Alloc, class U>
typename vector<T, Alloc>::size_type
erase(vector<T, Alloc>& c, const U& value) {
    auto it        = std::remove(c.begin(), c.end(), value);
    const auto res = std::distance(it, c.end());
    c.erase(it, c.end());
    return res;
}

template<class T, class Alloc, class Pred>
typename vector<T, Alloc>::size_type
erase_if(vector<T, Alloc>& c, Pred pred) {
    auto it        = std::remove_if(c.begin(), c.end(), pred);
    const auto res = std::distance(it, c.end());
    c.erase(it, c.end());
    return res;
}

#if CIEL_STD_VER >= 17

template<class Iter, class Alloc = std::allocator<typename std::iterator_traits<Iter>::value_type>>
vector(Iter, Iter, Alloc = Alloc()) -> vector<typename std::iterator_traits<Iter>::value_type, Alloc>;

#endif

NAMESPACE_CIEL_END

namespace std {

template<class T, class Alloc>
void
swap(ciel::vector<T, Alloc>& lhs, ciel::vector<T, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_VECTOR_HPP_

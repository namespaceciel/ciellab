#ifndef CIELLAB_INCLUDE_CIEL_SPLIT_BUFFER_HPP_
#define CIELLAB_INCLUDE_CIEL_SPLIT_BUFFER_HPP_

#include <ciel/allocator_traits.hpp>
#include <ciel/compare.hpp>
#include <ciel/compressed_pair.hpp>
#include <ciel/config.hpp>
#include <ciel/copy_n.hpp>
#include <ciel/cstring.hpp>
#include <ciel/demangle.hpp>
#include <ciel/do_if_noexcept.hpp>
#include <ciel/is_range.hpp>
#include <ciel/is_trivially_relocatable.hpp>
#include <ciel/iterator_category.hpp>
#include <ciel/range_destroyer.hpp>
#include <ciel/sbv_crtp_base.hpp>
#include <ciel/to_address.hpp>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

template<class, class>
class vector;

// Note that Allocator can be reference type as being used by vector,
// however in this case, the assignment operator of split_buffer may be invalid.
template<class T, class Allocator = std::allocator<T>>
class split_buffer : private sbv_crtp_base<T, Allocator, split_buffer<T, Allocator>> {
    static_assert(!std::is_rvalue_reference<Allocator>::value, "");

    using base_type = sbv_crtp_base<T, Allocator, split_buffer<T, Allocator>>;

public: // alias
    using typename base_type::allocator_type;
    using typename base_type::const_iterator;
    using typename base_type::const_pointer;
    using typename base_type::const_reference;
    using typename base_type::const_reverse_iterator;
    using typename base_type::difference_type;
    using typename base_type::iterator;
    using typename base_type::pointer;
    using typename base_type::reference;
    using typename base_type::reverse_iterator;
    using typename base_type::size_type;
    using typename base_type::value_type;

private:
    using base_type::should_pass_by_value;
    using typename base_type::alloc_traits;
    using typename base_type::lvalue;
    using typename base_type::rvalue;

private: // members
    pointer begin_cap_{nullptr};
    pointer begin_{nullptr};
    pointer end_{nullptr};
    compressed_pair<pointer, Allocator> end_cap_alloc_{nullptr, default_init};

private: // friends
    template<class, class>
    friend class split_buffer;
    template<class, class>
    friend class vector;
    friend base_type;

private: // private functions
    using base_type::allocator_;
    using base_type::construct;
    using base_type::construct_at_end;
    using base_type::destroy;
    using base_type::end_cap_;
    using base_type::internal_value;
    using base_type::recommend_cap;
    using base_type::reset;
    using base_type::swap_alloc;

public: // public functions
    // constructor
    // destructor
    // operator=
    using base_type::operator=;
    // assign
    using base_type::assign;
    using base_type::assign_range;
    using base_type::at;
    using base_type::get_allocator;
    using base_type::operator[];
    using base_type::back;
    using base_type::begin;
    using base_type::cbegin;
    using base_type::cend;
    using base_type::crbegin;
    using base_type::crend;
    using base_type::data;
    using base_type::empty;
    using base_type::end;
    using base_type::front;
    using base_type::max_size;
    using base_type::rbegin;
    using base_type::rend;
    using base_type::size;
    // reserve
    // capacity
    // shrink_to_fit
    using base_type::clear;
    // insert
    // insert_range
    // emplace
    using base_type::emplace_back;
    using base_type::erase;
    using base_type::push_back;
    using base_type::unchecked_emplace_back;
    // emplace_front
    // push_front
    // unchecked_emplace_front
    // append_range
    using base_type::pop_back;
    // pop_front
    // resize
    // swap

private:
    void swap_out_buffer(split_buffer<value_type, allocator_type&>&& sb,
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
                    sb.unchecked_emplace_front(ciel::move_if_noexcept(*p));
                }

                for (pointer p = pos; p < end_; ++p) {
                    sb.unchecked_emplace_back(ciel::move_if_noexcept(*p));
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

    // Note that this will invalidate iterators.
    template<class U = value_type, enable_if_t<is_trivially_relocatable<U>::value> = 0>
    void left_shift_n(const size_type n) noexcept {
        CIEL_PRECONDITION(front_spare() >= n);

        ciel::memmove(ciel::to_address(begin_ - n), ciel::to_address(begin_), sizeof(value_type) * size());
        begin_ -= n;
        end_ -= n;
    }

    // Note that this will invalidate iterators.
    template<class U = value_type, enable_if_t<!is_trivially_relocatable<U>::value> = 0>
    void left_shift_n(const size_type n) noexcept {
        CIEL_PRECONDITION(front_spare() >= n);

        const size_type old_size = size();

        const pointer new_begin = begin_ - n;
        pointer new_end         = new_begin;
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
    template<class U = value_type, enable_if_t<is_trivially_relocatable<U>::value> = 0>
    void right_shift_n(const size_type n) noexcept {
        CIEL_PRECONDITION(back_spare() >= n);

        ciel::memmove(ciel::to_address(begin_ + n), ciel::to_address(begin_), sizeof(value_type) * size());
        begin_ += n;
        end_ += n;
    }

    // Note that this will invalidate iterators.
    template<class U = value_type, enable_if_t<!is_trivially_relocatable<U>::value> = 0>
    void right_shift_n(const size_type n) noexcept {
        CIEL_PRECONDITION(back_spare() >= n);

        const size_type old_size = size();

        const pointer new_end = end_ + n;
        pointer new_begin     = new_end;
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

    void do_destroy() noexcept {
        if (begin_cap_) {
            clear();
            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
        }
    }

    void set_nullptr() noexcept {
        begin_cap_ = nullptr;
        begin_     = nullptr;
        end_       = nullptr;
        end_cap_() = nullptr;
    }

    void init(const size_type count) {
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

    void reserve_cap_and_offset_to(const size_type cap, const size_type offset) {
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

    template<class... Args>
    void emplace_back_aux(Args&&... args) {
        if (back_spare() == 0) {
            if CIEL_UNLIKELY (front_spare() > size()) {
                value_type tmp(std::forward<Args>(args)...); // NOLINT(misc-const-correctness)
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
            unchecked_emplace_back(std::forward<Args>(args)...);
        }
    }

    template<class... Args>
    void emplace_front_aux(Args&&... args) {
        if (front_spare() == 0) {
            if CIEL_UNLIKELY (back_spare() > size()) {
                value_type tmp(std::forward<Args>(args)...); // NOLINT(misc-const-correctness)
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
            unchecked_emplace_front(std::forward<Args>(args)...);
        }
    }

    template<class... Args>
    void unchecked_emplace_front_aux(Args&&... args) {
        CIEL_PRECONDITION(begin_cap_ < begin_);

        construct(begin_ - 1, std::forward<Args>(args)...);
        --begin_;
    }

public:
    split_buffer() = default;

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

    template<class Iter, enable_if_t<is_exactly_input_iterator<Iter>::value> = 0>
    split_buffer(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        for (; first != last; ++first) {
            emplace_back(*first);
        }
    }

    template<class Iter, enable_if_t<is_forward_iterator<Iter>::value> = 0>
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
            construct_at_end(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
        }
    }

    split_buffer(std::initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
        : split_buffer(init.begin(), init.end(), alloc) {}

    template<class R, enable_if_t<is_range_without_size<R>::value && std::is_lvalue_reference<R>::value> = 0>
    split_buffer(from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : split_buffer(rg.begin(), rg.end(), alloc) {}

    template<class R, enable_if_t<is_range_without_size<R>::value && !std::is_lvalue_reference<R>::value> = 0>
    split_buffer(from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : split_buffer(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()), alloc) {}

    template<class R, enable_if_t<is_range_with_size<R>::value && std::is_lvalue_reference<R>::value> = 0>
    split_buffer(from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : split_buffer(alloc) {
        const auto count = rg.size();

        if CIEL_LIKELY (count > 0) {
            init(count);
            construct_at_end(rg.begin(), rg.end());
        }
    }

    template<class R, enable_if_t<is_range_with_size<R>::value && !std::is_lvalue_reference<R>::value> = 0>
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

    split_buffer& operator=(const split_buffer& other) {
        return static_cast<base_type&>(*this) = other; // NOLINT(misc-unconventional-assign-operator)
    }

    split_buffer& operator=(split_buffer&& other) noexcept(alloc_traits::propagate_on_container_move_assignment::value
                                                           || alloc_traits::is_always_equal::value) {
        return static_cast<base_type&>(*this) = std::move(other); // NOLINT(misc-unconventional-assign-operator)
    }

    void assign(const size_type count, lvalue value) {
        if (back_spare() + size() < count) {
            const auto callback = [&] {
                if (capacity() >= count) {
                    clear();
                    begin_ = begin_cap_;
                    end_   = begin_;

                } else {
                    reset(count);
                }
            };

            if (internal_value(value, begin_)) {
                const value_type copy = std::move(*(begin_ + (std::addressof(value) - ciel::to_address(begin_))));
                callback();
                construct_at_end(count, copy);

            } else {
                callback();
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
    }

private:
    template<class Iter>
    void assign(Iter first, Iter last, const size_type count) {
        if (back_spare() + size() < count) {
            if (capacity() >= count) {
                clear();
                begin_ = begin_cap_;
                end_   = begin_;

            } else {
                reset(count);
            }

            construct_at_end(first, last);
            return;
        }

        if (size() > count) {
            end_ = destroy(begin_ + count, end_);
            ciel::copy_n(first, count, begin_); // count == size()

        } else {
            Iter mid = ciel::copy_n(first, size(), begin_);
            construct_at_end(mid, last);
        }
    }

public:
    CIEL_NODISCARD size_type front_spare() const noexcept {
        CIEL_PRECONDITION(begin_cap_ <= begin_);

        return begin_ - begin_cap_;
    }

    CIEL_NODISCARD size_type back_spare() const noexcept {
        CIEL_PRECONDITION(end_ <= end_cap_());

        return end_cap_() - end_;
    }

    void reserve_front_spare(const size_type new_spare) {
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

    void reserve_back_spare(const size_type new_spare) {
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

    CIEL_NODISCARD size_type capacity() const noexcept {
        return end_cap_() - begin_cap_;
    }

    void shrink_to_fit() {
        if CIEL_UNLIKELY (size() == capacity()) {
            return;
        }

        if (size() > 0) {
            split_buffer<value_type, allocator_type&> sb(allocator_());

            CIEL_TRY {
                sb.reserve_cap_and_offset_to(size(), 0);
                swap_out_buffer(std::move(sb), begin_);
            }
            CIEL_CATCH (...) {}

        } else {
            alloc_traits::deallocate(allocator_(), begin_cap_, capacity());
            set_nullptr();
        }
    }

private:
    iterator erase_impl(pointer first, pointer last,
                        const difference_type count) noexcept(is_trivially_relocatable<value_type>::value
                                                              || std::is_nothrow_move_assignable<value_type>::value) {
        CIEL_PRECONDITION(last - first == count);
        CIEL_PRECONDITION(count != 0);

        const auto index      = first - begin_;
        const auto back_count = end_ - last;

        if (back_count == 0) {
            end_ = destroy(first, end_);

        } else if (index == 0) {
            destroy(begin_, last);
            begin_ = last;

        } else if (back_count < index) { // move backward last half
            if (is_trivially_relocatable<value_type>::value) {
                destroy(first, last);
                end_ -= count;

                if (count >= back_count) {
                    ciel::memcpy(ciel::to_address(first), ciel::to_address(last), sizeof(value_type) * back_count);

                } else {
                    ciel::memmove(ciel::to_address(first), ciel::to_address(last), sizeof(value_type) * back_count);
                }

            } else {
                pointer new_end = std::move(last, end_, first);
                end_            = destroy(new_end, end_);
            }

        } else { // move forward first half
            if (is_trivially_relocatable<value_type>::value) {
                destroy(first, last);

                if (count >= index) {
                    ciel::memcpy(ciel::to_address(begin_ + count), ciel::to_address(begin_),
                                 sizeof(value_type) * index);

                } else {
                    ciel::memmove(ciel::to_address(begin_ + count), ciel::to_address(begin_),
                                  sizeof(value_type) * index);
                }

                begin_ += count;

            } else {
                pointer new_begin = std::move_backward(begin_, first, last);
                destroy(begin_, new_begin);
                begin_ = new_begin;
            }
        }

        return begin() + index;
    }

public:
    template<class... Args>
    reference emplace_front(Args&&... args) {
        emplace_front_aux(std::forward<Args>(args)...);

        return front();
    }

    template<class U, class... Args>
    reference emplace_front(std::initializer_list<U> il, Args&&... args) {
        emplace_front_aux(il, std::forward<Args>(args)...);

        return front();
    }

    void push_front(lvalue value) {
        emplace_front(value);
    }

    template<bool Valid = !should_pass_by_value, enable_if_t<Valid> = 0>
    void push_front(rvalue value) {
        emplace_front(std::move(value));
    }

    template<class... Args>
    reference unchecked_emplace_front(Args&&... args) {
        unchecked_emplace_front_aux(std::forward<Args>(args)...);

        return front();
    }

    template<class U, class... Args>
    reference unchecked_emplace_front(std::initializer_list<U> il, Args&&... args) {
        unchecked_emplace_front_aux(il, std::forward<Args>(args)...);

        return front();
    }

    void pop_front() noexcept {
        CIEL_PRECONDITION(!empty());

        destroy(begin_);
        ++begin_;
    }

    void resize(const size_type count) {
        if (size() >= count) {
            end_ = destroy(begin_ + count, end_);

        } else {
            reserve_back_spare(count - size());
            construct_at_end(count - size());
        }
    }

    void resize(const size_type count, lvalue value) {
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

    void swap(split_buffer& other) noexcept {
        using std::swap;

        swap(begin_cap_, other.begin_cap_);
        swap(begin_, other.begin_);
        swap(end_, other.end_);
        swap(end_cap_(), other.end_cap_());

        swap_alloc(other, typename alloc_traits::propagate_on_container_swap{});
    }

}; // class split_buffer

template<class T, class Allocator>
std::ostream& operator<<(std::ostream& out, const split_buffer<T, Allocator>& sb) {
#ifdef CIEL_HAS_RTTI
    out << ciel::demangle(typeid(sb).name()) << ": ";
#endif
    out << "[ __" << sb.front_spare() << "__, ";

    for (auto it = sb.begin(); it != sb.end(); ++it) {
        out << *it << ", ";
    }

    out << "__" << sb.back_spare() << "__ ]";

    return out;
}

template<class T, class Allocator>
struct is_trivially_relocatable<split_buffer<T, Allocator>>
    : conjunction<is_trivially_relocatable<Allocator>,
                  is_trivially_relocatable<typename std::allocator_traits<remove_reference_t<Allocator>>::pointer>> {};

template<class T, class Alloc, class U>
typename split_buffer<T, Alloc>::size_type erase(split_buffer<T, Alloc>& c, const U& value) {
    auto it        = std::remove(c.begin(), c.end(), value);
    const auto res = std::distance(it, c.end());
    c.erase(it, c.end());
    return res;
}

template<class T, class Alloc, class Pred>
typename split_buffer<T, Alloc>::size_type erase_if(split_buffer<T, Alloc>& c, Pred pred) {
    auto it        = std::remove_if(c.begin(), c.end(), pred);
    const auto res = std::distance(it, c.end());
    c.erase(it, c.end());
    return res;
}

#if CIEL_STD_VER >= 17

template<class Iter, class Alloc = std::allocator<typename std::iterator_traits<Iter>::value_type>>
split_buffer(Iter, Iter, Alloc = Alloc()) -> split_buffer<typename std::iterator_traits<Iter>::value_type, Alloc>;

#endif

NAMESPACE_CIEL_END

namespace std {

template<class T, class Alloc>
void swap(ciel::split_buffer<T, Alloc>& lhs, ciel::split_buffer<T, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_SPLIT_BUFFER_HPP_

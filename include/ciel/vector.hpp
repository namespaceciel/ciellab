#ifndef CIELLAB_INCLUDE_CIEL_VECTOR_HPP_
#define CIELLAB_INCLUDE_CIEL_VECTOR_HPP_

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
#include <ciel/sbv_crtp_base.hpp>
#include <ciel/split_buffer.hpp>
#include <ciel/to_address.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

template<class T, class Allocator = std::allocator<T>>
class vector : private sbv_crtp_base<T, Allocator, vector<T, Allocator>> {
    static_assert(!std::is_reference<Allocator>::value, "");

    using base_type = sbv_crtp_base<T, Allocator, vector<T, Allocator>>;

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
    pointer begin_{nullptr};
    pointer end_{nullptr};
    compressed_pair<pointer, Allocator> end_cap_alloc_{nullptr, default_init};

private: // friends
    friend base_type;

private: // private functions
    using base_type::allocator_;
    using base_type::construct;
    using base_type::construct_at_end;
    using base_type::copy_assign_alloc;
    using base_type::destroy;
    using base_type::end_cap_;
    using base_type::internal_value;
    using base_type::move_range;
    using base_type::recommend_cap;
    using base_type::reset;
    using base_type::swap_alloc;
    using base_type::unchecked_emplace_back_aux;

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
    // erase
    using base_type::emplace_back;
    using base_type::push_back;
    using base_type::unchecked_emplace_back;
    // append_range
    using base_type::pop_back;
    // resize
    // swap

private:
    void
    swap_out_buffer(split_buffer<value_type, allocator_type&>&& sb) noexcept(
        is_trivially_relocatable<value_type>::value || std::is_nothrow_move_constructible<value_type>::value) {
        CIEL_PRECONDITION(sb.front_spare() == size());

        // If either dest or src is an invalid or null pointer, memcpy's behavior is undefined, even if count is zero.
        if (begin_) {
            if (is_trivially_relocatable<value_type>::value) {
                ciel::memcpy(ciel::to_address(sb.begin_cap_), ciel::to_address(begin_), sizeof(value_type) * size());
                // sb.begin_ = sb.begin_cap_;

            } else {
                for (pointer p = end_ - 1; p >= begin_; --p) {
                    sb.unchecked_emplace_front_aux(ciel::move_if_noexcept(*p));
                }

                clear();
            }

            alloc_traits::deallocate(allocator_(), begin_, capacity());
        }

        begin_     = sb.begin_cap_;
        end_       = sb.end_;
        end_cap_() = sb.end_cap_();

        sb.set_nullptr();
    }

    void
    swap_out_buffer(split_buffer<value_type, allocator_type&>&& sb,
                    pointer pos) noexcept(is_trivially_relocatable<value_type>::value
                                          || std::is_nothrow_move_constructible<value_type>::value) {
        // If either dest or src is an invalid or null pointer, memcpy's behavior is undefined, even if count is zero.
        if (begin_) {
            const size_type front_count = pos - begin_;
            const size_type back_count  = end_ - pos;

            CIEL_PRECONDITION(sb.front_spare() == front_count);
            CIEL_PRECONDITION(sb.back_spare() >= back_count);

            if (is_trivially_relocatable<value_type>::value) {
                ciel::memcpy(ciel::to_address(sb.begin_cap_), ciel::to_address(begin_),
                             sizeof(value_type) * front_count);
                // sb.begin_ = sb.begin_cap_;

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

            alloc_traits::deallocate(allocator_(), begin_, capacity());
        }

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
    set_nullptr() noexcept {
        begin_     = nullptr;
        end_       = nullptr;
        end_cap_() = nullptr;
    }

    void
    init(const size_type count) {
        CIEL_PRECONDITION(count != 0);
        CIEL_PRECONDITION(begin_ == nullptr);
        CIEL_PRECONDITION(end_ == nullptr);
        CIEL_PRECONDITION(end_cap_() == nullptr);

        begin_     = alloc_traits::allocate(allocator_(), count);
        end_cap_() = begin_ + count;
        end_       = begin_;
    }

    template<class... Args>
    void
    emplace_back_aux(Args&&... args) {
        if (end_ == end_cap_()) {
            split_buffer<value_type, allocator_type&> sb(allocator_());
            sb.reserve_cap_and_offset_to(recommend_cap(size() + 1), size());
            sb.unchecked_emplace_back_aux(std::forward<Args>(args)...);
            swap_out_buffer(std::move(sb));

        } else {
            return unchecked_emplace_back_aux(std::forward<Args>(args)...);
        }
    }

public:
    vector() noexcept(noexcept(allocator_type())) = default;

    explicit vector(const allocator_type& alloc) noexcept
        : end_cap_alloc_(nullptr, alloc) {}

    vector(const size_type count, lvalue value, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        if CIEL_LIKELY (count > 0) {
            init(count);
            construct_at_end(count, value);
        }
    }

    explicit vector(const size_type count, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        if CIEL_LIKELY (count > 0) {
            init(count);
            construct_at_end(count);
        }
    }

    template<class Iter, enable_if_t<is_exactly_input_iterator<Iter>::value, int> = 0>
    vector(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        for (; first != last; ++first) {
            emplace_back_aux(*first);
        }
    }

    template<class Iter, enable_if_t<is_forward_iterator<Iter>::value, int> = 0>
    vector(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        const auto count = std::distance(first, last);

        if CIEL_LIKELY (count > 0) {
            init(count);
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

    vector(vector&& other, const allocator_type& alloc)
        : vector(alloc) {
        if (allocator_() == other.get_allocator()) {
            begin_     = other.begin_;
            end_       = other.end_;
            end_cap_() = other.end_cap_();

            other.set_nullptr();

        } else if (other.size() > 0) {
            init(other.size());
            construct_at_end(other.begin(), other.end());
        }
    }

    vector(std::initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
        : vector(init.begin(), init.end(), alloc) {}

    template<class R, enable_if_t<is_range_without_size<R>::value && std::is_lvalue_reference<R>::value, int> = 0>
    vector(from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : vector(rg.begin(), rg.end(), alloc) {}

    template<class R, enable_if_t<is_range_without_size<R>::value && !std::is_lvalue_reference<R>::value, int> = 0>
    vector(from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : vector(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()), alloc) {}

    template<class R, enable_if_t<is_range_with_size<R>::value && std::is_lvalue_reference<R>::value, int> = 0>
    vector(from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        const auto count = rg.size();

        if CIEL_LIKELY (count > 0) {
            init(count);
            construct_at_end(rg.begin(), rg.end());
        }
    }

    template<class R, enable_if_t<is_range_with_size<R>::value && !std::is_lvalue_reference<R>::value, int> = 0>
    vector(from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        const auto count = rg.size();

        if CIEL_LIKELY (count > 0) {
            init(count);
            construct_at_end(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()));
        }
    }

    ~vector() {
        do_destroy();
    }

    vector&
    operator=(const vector& other) {
        return static_cast<base_type&>(*this) = other;
    }

    vector&
    operator=(vector&& other) noexcept(alloc_traits::propagate_on_container_move_assignment::value
                                       || alloc_traits::is_always_equal::value) {
        return static_cast<base_type&>(*this) = std::move(other);
    }

    void
    assign(const size_type count, lvalue value) {
        if (capacity() < count) {
            if (internal_value(value, begin_)) {
                value_type copy = std::move(*(begin_ + (std::addressof(value) - ciel::to_address(begin_))));
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
        if (capacity() < count) {
            reset(count);
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

        CIEL_POSTCONDITION(size() == count);
    }

public:
    void
    reserve(const size_type new_cap) {
        if (new_cap <= capacity()) {
            return;

        } else if CIEL_UNLIKELY (new_cap > max_size()) {
            CIEL_THROW_EXCEPTION(std::length_error{"ciel::vector reserve capacity beyond max_size"});
        }

        split_buffer<value_type, allocator_type&> sb(allocator_());
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
            split_buffer<value_type, allocator_type&> sb(allocator_());

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

private:
    struct emplace_impl_callback {
        vector* const this_;

        emplace_impl_callback(vector* const t) noexcept
            : this_{t} {}

        template<class... Args, class U = value_type, enable_if_t<is_trivially_relocatable<U>::value, int> = 0>
        void
        operator()(pointer pos, Args&&... args) const {
            ciel::memmove(pos + 1, pos, sizeof(value_type) * (this_->end_ - pos));

            range_destroyer<value_type, allocator_type&> rd{pos + 1, this_->end_ + 1, this_->allocator_()};
            const pointer old_end = this_->end_;
            this_->end_           = pos;

            this_->unchecked_emplace_back_aux(std::forward<Args>(args)...);

            this_->end_ = old_end + 1;
            rd.release();
        }

        template<class... Args, class U = value_type, enable_if_t<!is_trivially_relocatable<U>::value, int> = 0>
        void
        operator()(pointer pos, Args&&... args) const {
            this_->move_range(pos, this_->end_, pos + 1);
            *pos = value_type{std::forward<Args>(args)...};
        }

        template<class U, class URaw = decay_t<U>,
                 enable_if_t<std::is_same<value_type, URaw>::value && !is_trivially_relocatable<URaw>::value, int> = 0>
        void
        operator()(pointer pos, U&& value) const {
            this_->move_range(pos, this_->end_, pos + 1);
            *pos = std::forward<U>(value);
        }

    }; // struct emplace_impl_callback

    template<class... Args>
    iterator
    emplace_impl(pointer pos, Args&&... args) {
        CIEL_PRECONDITION(begin_ <= pos);
        CIEL_PRECONDITION(pos <= end_);

        const size_type pos_index = pos - begin_;

        if (end_ == end_cap_()) { // expansion
            split_buffer<value_type, allocator_type&> sb(allocator_());
            sb.reserve_cap_and_offset_to(recommend_cap(size() + 1), pos_index);
            sb.unchecked_emplace_back_aux(std::forward<Args>(args)...);
            swap_out_buffer(std::move(sb), pos);

        } else if (pos == end_) { // equal to emplace_back
            unchecked_emplace_back_aux(std::forward<Args>(args)...);

        } else {
            emplace_impl_callback{this}(pos, std::forward<Args>(args)...);
        }

        return begin() + pos_index;
    }

public:
    template<class... Args>
    iterator
    emplace(const_iterator p, Args&&... args) {
        pointer pos = begin_ + (p - begin());

        return emplace_impl(pos, std::forward<Args>(args)...);
    }

    template<class U, class... Args>
    iterator
    emplace(const_iterator p, std::initializer_list<U> il, Args&&... args) {
        pointer pos = begin_ + (p - begin());

        return emplace_impl(pos, il, std::forward<Args>(args)...);
    }

    iterator
    insert(const_iterator p, lvalue value) {
        pointer pos = begin_ + (p - begin());

        if (internal_value(value, pos)) {
            value_type copy = value;
            return emplace_impl(pos, std::move(copy));

        } else {
            return emplace_impl(pos, value);
        }
    }

    template<bool Valid = !should_pass_by_value, enable_if_t<Valid, int> = 0>
    iterator
    insert(const_iterator p, rvalue value) {
        pointer pos = begin_ + (p - begin());

        if (internal_value(value, pos)) {
            value_type copy = std::move(value);
            return emplace_impl(pos, std::move(copy));

        } else {
            return emplace_impl(pos, std::move(value));
        }
    }

private:
    template<class T1, class T2, class U = value_type, enable_if_t<is_trivially_relocatable<U>::value, int> = 0>
    void
    insert_impl(pointer pos, T1&& t1, T2&& t2, const size_type count) {
        // ------------------------------------
        // begin                  pos       end
        //                       ----------
        //                       first last
        //                       |  count |
        // relocate [pos, end) count units later
        ciel::memmove(pos + count, pos, sizeof(value_type) * (end_ - pos));
        // ----------------------          --------------
        // begin             new_end       pos    |   end
        //                       ----------       |
        //                       first last      range_destroyer in case of exceptions
        //                       |  count |
        range_destroyer<value_type, allocator_type&> rd{pos + count, end_ + count, allocator_()};
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
             enable_if_t<is_forward_iterator<Iter>::value && !is_trivially_relocatable<U>::value, int> = 0>
    void
    insert_impl(pointer pos, Iter first, Iter last, size_type count) {
        CIEL_PRECONDITION(count != 0);

        const size_type old_count  = count;
        pointer old_end            = end_;
        auto mid                   = std::next(first, count);
        const size_type back_count = end_ - pos;

        if (count > back_count) {
            mid = std::next(first, back_count);
            construct_at_end(mid, last);
            count = back_count;
        }

        if (count > 0) {
            move_range(pos, old_end, pos + old_count);
            std::copy(first, mid, pos);
        }
    }

    template<class U = value_type, enable_if_t<!is_trivially_relocatable<U>::value, int> = 0>
    void
    insert_impl(pointer pos, size_type count, const value_type& value, const size_type old_count) {
        pointer old_end = end_;

        const size_type back_count = end_ - pos;

        if (count > back_count) {
            const size_type n = count - back_count;
            construct_at_end(n, value);
            count = back_count;
        }

        if (count > 0) {
            move_range(pos, old_end, pos + old_count);
            std::fill_n(pos, count, value);
        }
    }

    template<class Iter>
    iterator
    insert(const_iterator p, Iter first, Iter last, size_type count) {
        pointer pos = begin_ + (p - begin());
        CIEL_PRECONDITION(begin() <= pos);
        CIEL_PRECONDITION(pos <= end());

        if CIEL_UNLIKELY (count == 0) {
            return pos;
        }

        const size_type pos_index = pos - begin();

        if (count + size() > capacity()) { // expansion
            split_buffer<value_type, allocator_type&> sb(allocator_());
            sb.reserve_cap_and_offset_to(recommend_cap(count + size()), pos_index);
            sb.construct_at_end(first, last);
            swap_out_buffer(std::move(sb), pos);

        } else { // enough back space
            insert_impl(pos, first, last, count);
        }

        return begin() + pos_index;
    }

public:
    iterator
    insert(const_iterator p, size_type count, lvalue value) {
        pointer pos = begin_ + (p - begin());
        CIEL_PRECONDITION(begin() <= pos);
        CIEL_PRECONDITION(pos <= end());

        const size_type pos_index = pos - begin();

        if (count + size() > capacity()) { // expansion
            split_buffer<value_type, allocator_type&> sb(allocator_());
            sb.reserve_cap_and_offset_to(recommend_cap(size() + count), pos_index);
            sb.construct_at_end(count, value);
            swap_out_buffer(std::move(sb), pos);

        } else { // enough back space
            if (internal_value(value, pos)) {
                value_type copy = value;
                insert_impl(pos, count, copy, count);

            } else {
                insert_impl(pos, count, value, count);
            }
        }

        return begin() + pos_index;
    }

    // Construct them all at the end at first, then rotate them to the right place.
    template<class Iter, enable_if_t<is_exactly_input_iterator<Iter>::value, int> = 0>
    iterator
    insert(const_iterator p, Iter first, Iter last) {
        pointer pos = begin_ + (p - begin());
        // record these index because it may reallocate
        const auto pos_index     = pos - begin();
        const size_type old_size = size();

        for (; first != last; ++first) {
            emplace_back_aux(*first);
        }

        std::rotate(begin() + pos_index, begin() + old_size, end());
        return begin() + pos_index;
    }

    template<class Iter, enable_if_t<is_forward_iterator<Iter>::value, int> = 0>
    iterator
    insert(const_iterator pos, Iter first, Iter last) {
        const size_type count = std::distance(first, last);

        return insert(pos, first, last, count);
    }

    iterator
    insert(const_iterator pos, std::initializer_list<value_type> ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }

private:
    iterator
    erase_impl(pointer first, pointer last,
               const difference_type count) noexcept(is_trivially_relocatable<value_type>::value
                                                     || std::is_nothrow_move_assignable<value_type>::value) {
        CIEL_PRECONDITION(last - first == count);
        CIEL_PRECONDITION(count != 0);

        const auto index      = first - begin_;
        const auto back_count = end_ - last;

        if (back_count == 0) {
            end_ = destroy(first, end_);

        } else if (is_trivially_relocatable<value_type>::value) {
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

        return begin() + index;
    }

public:
    iterator
    erase(const_iterator p) {
        const pointer pos = begin_ + (p - begin());
        CIEL_PRECONDITION(begin_ <= pos);
        CIEL_PRECONDITION(pos < end_);

        return erase_impl(pos, pos + 1, 1);
    }

    iterator
    erase(const_iterator f, const_iterator l) {
        const pointer first = begin_ + (f - begin());
        const pointer last  = begin_ + (l - begin());
        CIEL_PRECONDITION(begin_ <= first);
        CIEL_PRECONDITION(last <= end_);

        const auto count = last - first;

        if CIEL_UNLIKELY (count <= 0) {
            return last;
        }

        return erase_impl(first, last, count);
    }

    void
    resize(const size_type count) {
        if (size() >= count) {
            end_ = destroy(begin_ + count, end_);

        } else {
            reserve(count);
            construct_at_end(count - size());
        }
    }

    void
    resize(const size_type count, lvalue value) {
        if (size() >= count) {
            end_ = destroy(begin_ + count, end_);

        } else if (count > capacity()) {
            split_buffer<value_type, allocator_type&> sb(allocator_());
            sb.reserve_cap_and_offset_to(count, size());
            sb.construct_at_end(count - size(), value);
            swap_out_buffer(std::move(sb));

        } else {
            construct_at_end(count - size(), value);
        }
    }

    void
    swap(vector& other) noexcept {
        using std::swap;

        swap(begin_, other.begin_);
        swap(end_, other.end_);
        swap(end_cap_(), other.end_cap_());

        swap_alloc(other, typename alloc_traits::propagate_on_container_swap{});
    }

    template<class R, enable_if_t<is_range<R>::value, int> = 0>
    void
    append_range(R&& rg) {
        insert_range(end(), std::forward<R>(rg));
    }

    template<class R, enable_if_t<is_range<R>::value, int> = 0>
    iterator
    insert_range(const_iterator pos, R&& rg) {
        if (is_range_with_size<R>::value && is_forward_iterator<decltype(rg.begin())>::value) {
            if (std::is_lvalue_reference<R>::value) {
                return insert(pos, rg.begin(), rg.end(), rg.size());

            } else {
                return insert(pos, std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()), rg.size());
            }

        } else {
            if (std::is_lvalue_reference<R>::value) {
                return insert(pos, rg.begin(), rg.end());

            } else {
                return insert(pos, std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()));
            }
        }
    }

}; // class vector

template<class T, class Allocator>
struct is_trivially_relocatable<vector<T, Allocator>>
    : conjunction<is_trivially_relocatable<Allocator>,
                  is_trivially_relocatable<typename std::allocator_traits<remove_reference_t<Allocator>>::pointer>> {};

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

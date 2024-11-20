#ifndef CIELLAB_INCLUDE_CIEL_VECTOR_HPP_
#define CIELLAB_INCLUDE_CIEL_VECTOR_HPP_

#include <ciel/allocator_traits.hpp>
#include <ciel/compare.hpp>
#include <ciel/copy_n.hpp>
#include <ciel/core/compressed_pair.hpp>
#include <ciel/core/config.hpp>
#include <ciel/core/cstring.hpp>
#include <ciel/core/do_if_noexcept.hpp>
#include <ciel/core/exchange.hpp>
#include <ciel/core/is_range.hpp>
#include <ciel/core/is_trivially_relocatable.hpp>
#include <ciel/core/iterator_category.hpp>
#include <ciel/core/message.hpp>
#include <ciel/demangle.hpp>
#include <ciel/range_destroyer.hpp>
#include <ciel/split_buffer.hpp>
#include <ciel/to_address.hpp>

#include <algorithm>
#include <iostream>
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

    template<class... Args>
    using via_trivial_construct =
        allocator_has_trivial_construct<allocator_type, decltype(ciel::to_address(std::declval<pointer>())), Args...>;
    using via_trivial_destroy =
        allocator_has_trivial_destroy<allocator_type, decltype(ciel::to_address(std::declval<pointer>()))>;

    static constexpr bool expand_via_memcpy =
        is_trivially_relocatable<value_type>::value
        && via_trivial_construct<decltype(ciel::move_if_noexcept(*std::declval<pointer>()))>::value
        && via_trivial_destroy::value;

    static constexpr bool move_via_memmove = is_trivially_relocatable<value_type>::value
                                          && via_trivial_construct<decltype(std::move(*std::declval<pointer>()))>::value
                                          && via_trivial_destroy::value;

    pointer begin_{nullptr};
    pointer end_{nullptr};
    compressed_pair<pointer, allocator_type> end_cap_alloc_{nullptr, default_init};

    CIEL_NODISCARD pointer& end_cap_() noexcept {
        return end_cap_alloc_.first();
    }

    CIEL_NODISCARD const pointer& end_cap_() const noexcept {
        return end_cap_alloc_.first();
    }

    allocator_type& allocator_() noexcept { // NOLINT(modernize-use-nodiscard)
        return end_cap_alloc_.second();
    }

    const allocator_type& allocator_() const noexcept { // NOLINT(modernize-use-nodiscard)
        return end_cap_alloc_.second();
    }

    static constexpr bool should_pass_by_value =
        std::is_trivially_copyable<value_type>::value && sizeof(value_type) <= 16;
    using lvalue = conditional_t<should_pass_by_value, value_type, const value_type&>;
    using rvalue = conditional_t<should_pass_by_value, value_type, value_type&&>;

    CIEL_NODISCARD bool internal_value(const value_type& value, pointer begin) const noexcept {
        if (should_pass_by_value) {
            return false;
        }

        if CIEL_UNLIKELY (ciel::to_address(begin) <= std::addressof(value)
                          && std::addressof(value) < ciel::to_address(end_)) {
            return true;
        }

        return false;
    }

    CIEL_NODISCARD size_type recommend_cap(const size_type new_size) const {
        CIEL_PRECONDITION(new_size > 0);

        const size_type ms = max_size();

        if CIEL_UNLIKELY (new_size > ms) {
            CIEL_THROW_EXCEPTION(std::length_error("ciel::vector expanding size is beyond max_size"));
        }

        const size_type cap = capacity();

        if CIEL_UNLIKELY (cap >= ms / 2) {
            return ms;
        }

        return std::max(cap * 2, new_size);
    }

    template<class... Args>
    void construct(pointer p, Args&&... args) {
        alloc_traits::construct(allocator_(), ciel::to_address(p), std::forward<Args>(args)...);
    }

    void destroy(pointer p) noexcept {
        CIEL_PRECONDITION(begin_ <= p);
        CIEL_PRECONDITION(p < end_);

        alloc_traits::destroy(allocator_(), ciel::to_address(p));
    }

    pointer destroy(pointer first, pointer last) noexcept {
        CIEL_PRECONDITION(begin_ <= first);
        CIEL_PRECONDITION(first <= last);
        CIEL_PRECONDITION(last <= end_);

        const pointer res = first;

        for (; first != last; ++first) {
            alloc_traits::destroy(allocator_(), ciel::to_address(first));
        }

        return res;
    }

    void construct_at_end(const size_type n) {
        CIEL_PRECONDITION(end_ + n <= end_cap_());

        for (size_type i = 0; i < n; ++i) {
            unchecked_emplace_back();
        }
    }

    void construct_at_end(const size_type n, lvalue value) {
        CIEL_PRECONDITION(end_ + n <= end_cap_());

        for (size_type i = 0; i < n; ++i) {
            unchecked_emplace_back(value);
        }
    }

    template<class Iter>
    void construct_at_end(Iter first, Iter last) {
        ciel::uninitialized_copy(allocator_(), first, last, end_);
    }

    void set_nullptr() noexcept {
        begin_     = nullptr;
        end_       = nullptr;
        end_cap_() = nullptr;
    }

    void init(const size_type count) {
        CIEL_PRECONDITION(count != 0);
        CIEL_PRECONDITION(begin_ == nullptr);
        CIEL_PRECONDITION(end_ == nullptr);
        CIEL_PRECONDITION(end_cap_() == nullptr);

        begin_     = alloc_traits::allocate(allocator_(), count);
        end_cap_() = begin_ + count;
        end_       = begin_;
    }

    void reset() noexcept {
        do_destroy();
        set_nullptr();
    }

    void reset(const size_type count) {
        CIEL_PRECONDITION(count != 0);

        do_destroy();
        set_nullptr(); // It's neccessary since allocation would throw.
        init(count);
    }

    void swap_out_buffer(split_buffer<value_type, allocator_type&>&& sb) noexcept(
        expand_via_memcpy || std::is_nothrow_move_constructible<value_type>::value) {
        CIEL_PRECONDITION(sb.front_spare() == size());

        // If either dest or src is an invalid or null pointer, memcpy's behavior is undefined, even if count is zero.
        if (begin_) {
            if (expand_via_memcpy) {
                ciel::memcpy(ciel::to_address(sb.begin_cap_), ciel::to_address(begin_), sizeof(value_type) * size());
                // sb.begin_ = sb.begin_cap_;

            } else {
                for (pointer p = end_ - 1; p >= begin_; --p) {
                    sb.unchecked_emplace_front(ciel::move_if_noexcept(*p));
                }

                clear();
            }

            alloc_traits::deallocate(allocator_(), begin_, capacity());
        }

        begin_     = sb.begin_cap_;
        end_       = sb.end_;
        end_cap_() = sb.end_cap_;

        sb.begin_cap_ = nullptr; // enough for split_buffer's destructor
    }

    void swap_out_buffer(split_buffer<value_type, allocator_type&>&& sb,
                         pointer pos) noexcept(expand_via_memcpy
                                               || std::is_nothrow_move_constructible<value_type>::value) {
        // If either dest or src is an invalid or null pointer, memcpy's behavior is undefined, even if count is zero.
        if (begin_) {
            const size_type front_count = pos - begin_;
            const size_type back_count  = end_ - pos;

            CIEL_PRECONDITION(sb.front_spare() == front_count);
            CIEL_PRECONDITION(sb.back_spare() >= back_count);

            if (expand_via_memcpy) {
                ciel::memcpy(ciel::to_address(sb.begin_cap_), ciel::to_address(begin_),
                             sizeof(value_type) * front_count);
                // sb.begin_ = sb.begin_cap_;

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

            alloc_traits::deallocate(allocator_(), begin_, capacity());
        }

        begin_     = sb.begin_cap_;
        end_       = sb.end_;
        end_cap_() = sb.end_cap_;

        sb.begin_cap_ = nullptr; // enough for split_buffer's destructor
    }

    template<class... Args>
    void emplace_back_aux(Args&&... args) {
        if (end_ == end_cap_()) {
            split_buffer<value_type, allocator_type&> sb(allocator_(), recommend_cap(size() + 1), size());
            sb.unchecked_emplace_back(std::forward<Args>(args)...);
            swap_out_buffer(std::move(sb));

        } else {
            unchecked_emplace_back(std::forward<Args>(args)...);
        }
    }

    template<class... Args>
    void unchecked_emplace_back_aux(Args&&... args) {
        CIEL_PRECONDITION(end_ < end_cap_());

        construct(end_, std::forward<Args>(args)...);
        ++end_;
    }

    void copy_assign_alloc(const vector& other, std::true_type) {
        if (allocator_() != other.allocator_()) {
            reset();
        }

        allocator_() = other.allocator_();
    }

    void copy_assign_alloc(const vector&, std::false_type) noexcept {}

    void swap_alloc(vector& other, std::true_type) noexcept {
        using std::swap;
        swap(allocator_(), other.allocator_());
    }

    void swap_alloc(vector&, std::false_type) noexcept {}

    void do_destroy() noexcept {
        if (begin_) {
            clear();
            alloc_traits::deallocate(allocator_(), begin_, capacity());
        }
    }

public:
    vector() = default;

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

    template<class Iter, enable_if_t<is_exactly_input_iterator<Iter>::value> = 0>
    vector(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        for (; first != last; ++first) {
            emplace_back(*first);
        }
    }

    template<class Iter, enable_if_t<is_forward_iterator<Iter>::value> = 0>
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
        : begin_(ciel::exchange(other.begin_, nullptr)),
          end_(ciel::exchange(other.end_, nullptr)),
          end_cap_alloc_(ciel::exchange(other.end_cap_(), nullptr), std::move(other.allocator_())) {}

    vector(vector&& other, const allocator_type& alloc)
        : vector(alloc) {
        if (allocator_() == other.get_allocator()) {
            begin_     = ciel::exchange(other.begin_, nullptr);
            end_       = ciel::exchange(other.end_, nullptr);
            end_cap_() = ciel::exchange(other.end_cap_(), nullptr);

        } else if (other.size() > 0) {
            init(other.size());
            construct_at_end(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
        }
    }

    vector(std::initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
        : vector(init.begin(), init.end(), alloc) {}

    template<class R, enable_if_t<is_range_without_size<R>::value && std::is_lvalue_reference<R>::value> = 0>
    vector(from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : vector(rg.begin(), rg.end(), alloc) {}

    template<class R, enable_if_t<is_range_without_size<R>::value && !std::is_lvalue_reference<R>::value> = 0>
    vector(from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : vector(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()), alloc) {}

    template<class R, enable_if_t<is_range_with_size<R>::value && std::is_lvalue_reference<R>::value> = 0>
    vector(from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        const auto count = rg.size();

        if CIEL_LIKELY (count > 0) {
            init(count);
            construct_at_end(rg.begin(), rg.end());
        }
    }

    template<class R, enable_if_t<is_range_with_size<R>::value && !std::is_lvalue_reference<R>::value> = 0>
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

    vector& operator=(const vector& other) {
        if CIEL_UNLIKELY (this == std::addressof(other)) {
            return *this;
        }

        copy_assign_alloc(other, typename alloc_traits::propagate_on_container_copy_assignment{});
        assign(other.begin(), other.end());

        return *this;
    }

    vector& operator=(vector&& other) noexcept(alloc_traits::propagate_on_container_move_assignment::value
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

    void assign(const size_type count, lvalue value) {
        if (capacity() < count) {
            if (internal_value(value, begin_)) {
                const value_type copy = std::move(*(begin_ + (std::addressof(value) - ciel::to_address(begin_))));
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
    }

private:
    template<class Iter>
    void assign(Iter first, Iter last, const size_type count) {
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
    }

public:
    vector& operator=(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end(), ilist.size());
        return *this;
    }

    template<class Iter, enable_if_t<is_forward_iterator<Iter>::value> = 0>
    void assign(Iter first, Iter last) {
        const size_type count = std::distance(first, last);

        assign(first, last, count);
    }

    template<class Iter, enable_if_t<is_exactly_input_iterator<Iter>::value> = 0>
    void assign(Iter first, Iter last) {
        pointer p = begin_;
        for (; first != last && p != end_; ++first) {
            *p = *first;
            ++p;
        }

        if (p != end_) {
            end_ = destroy(p, end_);

        } else {
            for (; first != last; ++first) {
                emplace_back(*first);
            }
        }
    }

    void assign(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end(), ilist.size());
    }

    template<class R, enable_if_t<is_range<R>::value> = 0>
    void assign_range(R&& rg) {
        if (is_range_with_size<R>::value) {
            if (std::is_lvalue_reference<R>::value) {
                assign(rg.begin(), rg.end(), ciel::distance(rg));

            } else {
                assign(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()), ciel::distance(rg));
            }

        } else {
            if (std::is_lvalue_reference<R>::value) {
                assign(rg.begin(), rg.end());

            } else {
                assign(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()));
            }
        }
    }

    allocator_type get_allocator() const noexcept { // NOLINT(modernize-use-nodiscard)
        return allocator_();
    }

    CIEL_NODISCARD reference at(const size_type pos) {
        if CIEL_UNLIKELY (pos >= size()) {
            CIEL_THROW_EXCEPTION(std::out_of_range("ciel::vector::at pos is not within the range"));
        }

        return begin_[pos];
    }

    CIEL_NODISCARD const_reference at(const size_type pos) const {
        if CIEL_UNLIKELY (pos >= size()) {
            CIEL_THROW_EXCEPTION(std::out_of_range("ciel::vector::at pos is not within the range"));
        }

        return begin_[pos];
    }

    CIEL_NODISCARD reference operator[](const size_type pos) {
        CIEL_PRECONDITION(pos < size());

        return begin_[pos];
    }

    CIEL_NODISCARD const_reference operator[](const size_type pos) const {
        CIEL_PRECONDITION(pos < size());

        return begin_[pos];
    }

    CIEL_NODISCARD reference front() {
        CIEL_PRECONDITION(!empty());

        return begin_[0];
    }

    CIEL_NODISCARD const_reference front() const {
        CIEL_PRECONDITION(!empty());

        return begin_[0];
    }

    CIEL_NODISCARD reference back() {
        CIEL_PRECONDITION(!empty());

        return *(end_ - 1);
    }

    CIEL_NODISCARD const_reference back() const {
        CIEL_PRECONDITION(!empty());

        return *(end_ - 1);
    }

    CIEL_NODISCARD T* data() noexcept {
        return ciel::to_address(begin_);
    }

    CIEL_NODISCARD const T* data() const noexcept {
        return ciel::to_address(begin_);
    }

    CIEL_NODISCARD iterator begin() noexcept {
        return iterator(begin_);
    }

    CIEL_NODISCARD const_iterator begin() const noexcept {
        return const_iterator(begin_);
    }

    CIEL_NODISCARD const_iterator cbegin() const noexcept {
        return begin();
    }

    CIEL_NODISCARD iterator end() noexcept {
        return iterator(end_);
    }

    CIEL_NODISCARD const_iterator end() const noexcept {
        return const_iterator(end_);
    }

    CIEL_NODISCARD const_iterator cend() const noexcept {
        return end();
    }

    CIEL_NODISCARD reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    CIEL_NODISCARD const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    CIEL_NODISCARD const_reverse_iterator crbegin() const noexcept {
        return rbegin();
    }

    CIEL_NODISCARD reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    CIEL_NODISCARD const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    CIEL_NODISCARD const_reverse_iterator crend() const noexcept {
        return rend();
    }

    CIEL_NODISCARD bool empty() const noexcept {
        return begin_ == end_;
    }

    CIEL_NODISCARD size_type size() const noexcept {
        return end_ - begin_;
    }

    CIEL_NODISCARD size_type max_size() const noexcept {
        return std::min<size_type>(std::numeric_limits<difference_type>::max(), alloc_traits::max_size(allocator_()));
    }

    void reserve(const size_type new_cap) {
        if (new_cap <= capacity()) {
            return;
        }

        if CIEL_UNLIKELY (new_cap > max_size()) {
            CIEL_THROW_EXCEPTION(std::length_error{"ciel::vector::reserve capacity beyond max_size"});
        }

        split_buffer<value_type, allocator_type&> sb(allocator_(), new_cap, size());
        swap_out_buffer(std::move(sb));
    }

    CIEL_NODISCARD size_type capacity() const noexcept {
        return end_cap_() - begin_;
    }

    void shrink_to_fit() {
        if CIEL_UNLIKELY (size() == capacity()) {
            return;
        }

        if (size() > 0) {
            CIEL_TRY {
                split_buffer<value_type, allocator_type&> sb(allocator_(), size(), size());
                swap_out_buffer(std::move(sb));
            }
            CIEL_CATCH (...) {}

        } else {
            alloc_traits::deallocate(allocator_(), begin_, capacity());
            set_nullptr();
        }
    }

    void clear() noexcept {
        end_ = destroy(begin_, end_);
    }

private:
    template<class ExpansionCallback, class AppendCallback, class InsertCallback, class IsInternalValueCallback>
    iterator insert_impl(pointer pos, const size_type count, ExpansionCallback&& expansion_callback,
                         AppendCallback&& append_callback, InsertCallback&& insert_callback,
                         IsInternalValueCallback&& is_internal_value_callback) {
        CIEL_PRECONDITION(begin_ <= pos);
        CIEL_PRECONDITION(pos <= end_);
        CIEL_PRECONDITION(count != 0);

        const size_type pos_index = pos - begin_;

        if (size() + count > capacity()) { // expansion
            split_buffer<value_type, allocator_type&> sb(allocator_(), recommend_cap(size() + count), pos_index);
            expansion_callback(sb);
            swap_out_buffer(std::move(sb), pos);

        } else if (pos == end_) { // equal to emplace_back
            append_callback();

        } else {
            const bool is_internal_value = is_internal_value_callback();
            const pointer old_end        = end_;

            range_destroyer<value_type, allocator_type&> rd{end_ + count, end_ + count, allocator_()};
            // ------------------------------------
            // begin                  pos       end
            //                       ----------
            //                       first last
            //                       |  count |
            // relocate [pos, end) count units later
            // ----------------------          --------------
            // begin             new_end       pos    |   end
            //                       ----------       |
            //                       first last      range_destroyer in case of exceptions
            //                       |  count |
            if (move_via_memmove) {
                const size_type pos_end_dis = end_ - pos;
                ciel::memmove(ciel::to_address(pos + count), ciel::to_address(pos), sizeof(value_type) * pos_end_dis);
                end_ = pos;
                rd.advance_backward(pos_end_dis);

            } else {
                for (pointer p = end_ - 1; p >= pos; --p) {
                    construct(p + count, std::move(*p));
                    destroy(p);
                    --end_;
                    rd.advance_backward();
                }
            }
            // ----------------------------------------------
            // begin             first        last        end
            //                                 pos
            //                               new_end
            if (is_internal_value) {
                insert_callback();

            } else {
                append_callback();
            }
            // new_end
            end_ = old_end + count;
            rd.release();
        }

        return begin() + pos_index;
    }

public:
    iterator insert(const_iterator p, lvalue value) {
        const pointer pos = begin_ + (p - begin());

        return insert_impl(
            pos, 1,
            [&](split_buffer<value_type, allocator_type&>& sb) {
                sb.unchecked_emplace_back(value);
            },
            [&] {
                unchecked_emplace_back(value);
            },
            [&] {
                unchecked_emplace_back(*(std::addressof(value) + 1));
            },
            [&] {
                return internal_value(value, pos);
            });
    }

    template<bool Valid = !should_pass_by_value, enable_if_t<Valid> = 0>
    iterator insert(const_iterator p, rvalue value) {
        const pointer pos = begin_ + (p - begin());

        return insert_impl(
            pos, 1,
            [&](split_buffer<value_type, allocator_type&>& sb) {
                sb.unchecked_emplace_back(std::move(value));
            },
            [&] {
                unchecked_emplace_back(std::move(value));
            },
            [&] {
                unchecked_emplace_back(std::move(*(std::addressof(value) + 1)));
            },
            [&] {
                return internal_value(value, pos);
            });
    }

    iterator insert(const_iterator p, size_type count, lvalue value) {
        const pointer pos = begin_ + (p - begin());

        if CIEL_UNLIKELY (count == 0) {
            return iterator(pos);
        }

        return insert_impl(
            pos, count,
            [&](split_buffer<value_type, allocator_type&>& sb) {
                sb.construct_at_end(count, value);
            },
            [&] {
                construct_at_end(count, value);
            },
            [&] {
                construct_at_end(count, *(std::addressof(value) + count));
            },
            [&] {
                return internal_value(value, pos);
            });
    }

private:
    template<class Iter>
    iterator insert(const_iterator p, Iter first, Iter last, size_type count) {
        const pointer pos = begin_ + (p - begin());

        if CIEL_UNLIKELY (count == 0) {
            return iterator(pos);
        }

        return insert_impl(
            pos, count,
            [&](split_buffer<value_type, allocator_type&>& sb) {
                sb.construct_at_end(first, last);
            },
            [&] {
                construct_at_end(first, last);
            },
            [&] {
                unreachable();
            },
            [&] {
                return false;
            });
    }

public:
    template<class Iter, enable_if_t<is_forward_iterator<Iter>::value> = 0>
    iterator insert(const_iterator pos, Iter first, Iter last) {
        return insert(pos, first, last, std::distance(first, last));
    }

    // Construct them all at the end at first, then rotate them to the right place.
    template<class Iter, enable_if_t<is_exactly_input_iterator<Iter>::value> = 0>
    iterator insert(const_iterator p, Iter first, Iter last) {
        const pointer pos = begin_ + (p - begin());

        const auto pos_index     = pos - begin();
        const size_type old_size = size();

        for (; first != last; ++first) {
            emplace_back(*first);
        }

        std::rotate(begin() + pos_index, begin() + old_size, end());
        return begin() + pos_index;
    }

    iterator insert(const_iterator pos, std::initializer_list<value_type> ilist) {
        return insert(pos, ilist.begin(), ilist.end(), ilist.size());
    }

    template<class R, enable_if_t<is_range<R>::value> = 0>
    iterator insert_range(const_iterator pos, R&& rg) {
        if (is_range_with_size<R>::value) {
            if (std::is_lvalue_reference<R>::value) {
                return insert(pos, rg.begin(), rg.end(), ciel::distance(rg));
            }

            return insert(pos, std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()),
                          ciel::distance(rg));
        }

        if (std::is_lvalue_reference<R>::value) {
            return insert(pos, rg.begin(), rg.end());
        }

        return insert(pos, std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()));
    }

    template<class... Args>
    iterator emplace(const_iterator p, Args&&... args) {
        const pointer pos = begin_ + (p - begin());

        return insert_impl(
            pos, 1,
            [&](split_buffer<value_type, allocator_type&>& sb) {
                sb.unchecked_emplace_back(std::forward<Args>(args)...);
            },
            [&] {
                unchecked_emplace_back(std::forward<Args>(args)...);
            },
            [&] {
                unreachable();
            },
            [&] {
                return false;
            });
    }

    template<class U, class... Args>
    iterator emplace(const_iterator p, std::initializer_list<U> il, Args&&... args) {
        const pointer pos = begin_ + (p - begin());

        return insert_impl(
            pos, 1,
            [&](split_buffer<value_type, allocator_type&>& sb) {
                sb.unchecked_emplace_back(il, std::forward<Args>(args)...);
            },
            [&] {
                unchecked_emplace_back(il, std::forward<Args>(args)...);
            },
            [&] {
                unreachable();
            },
            [&] {
                return false;
            });
    }

    template<class U, enable_if_t<std::is_same<remove_cvref_t<U>, value_type>::value> = 0>
    iterator emplace(const_iterator p, U&& value) {
        return insert(p, std::forward<U>(value));
    }

private:
    iterator erase_impl(pointer first, pointer last,
                        const difference_type count) noexcept(move_via_memmove
                                                              || std::is_nothrow_move_assignable<value_type>::value) {
        CIEL_PRECONDITION(last - first == count);
        CIEL_PRECONDITION(count != 0);

        const auto index      = first - begin_;
        const auto back_count = end_ - last;

        if (back_count == 0) {
            end_ = destroy(first, end_);

        } else if (move_via_memmove) {
            destroy(first, last);
            end_ -= count;

            if (count >= back_count) {
                ciel::memcpy(ciel::to_address(first), ciel::to_address(last), sizeof(value_type) * back_count);

            } else {
                ciel::memmove(ciel::to_address(first), ciel::to_address(last), sizeof(value_type) * back_count);
            }

        } else {
            const pointer new_end = std::move(last, end_, first);
            end_                  = destroy(new_end, end_);
        }

        return begin() + index;
    }

public:
    iterator erase(const_iterator p) {
        const pointer pos = begin_ + (p - begin());
        CIEL_PRECONDITION(begin_ <= pos);
        CIEL_PRECONDITION(pos < end_);

        return erase_impl(pos, pos + 1, 1);
    }

    iterator erase(const_iterator f, const_iterator l) {
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

    void push_back(lvalue value) {
        emplace_back(value);
    }

    template<bool Valid = !should_pass_by_value, enable_if_t<Valid> = 0>
    void push_back(rvalue value) {
        emplace_back(std::move(value));
    }

    template<class... Args>
    reference emplace_back(Args&&... args) {
        emplace_back_aux(std::forward<Args>(args)...);

        return back();
    }

    template<class U, class... Args>
    reference emplace_back(std::initializer_list<U> il, Args&&... args) {
        emplace_back_aux(il, std::forward<Args>(args)...);

        return back();
    }

    template<class... Args>
    reference unchecked_emplace_back(Args&&... args) {
        unchecked_emplace_back_aux(std::forward<Args>(args)...);

        return back();
    }

    template<class U, class... Args>
    reference unchecked_emplace_back(std::initializer_list<U> il, Args&&... args) {
        unchecked_emplace_back_aux(il, std::forward<Args>(args)...);

        return back();
    }

    template<class R, enable_if_t<is_range<R>::value> = 0>
    void append_range(R&& rg) {
        insert_range(end(), std::forward<R>(rg));
    }

    void pop_back() noexcept {
        CIEL_PRECONDITION(!empty());

        destroy(end_ - 1);
        --end_;
    }

    void resize(const size_type count) {
        if (size() >= count) {
            end_ = destroy(begin_ + count, end_);

        } else {
            reserve(count);
            construct_at_end(count - size());
        }
    }

    void resize(const size_type count, lvalue value) {
        if (size() >= count) {
            end_ = destroy(begin_ + count, end_);

        } else if (count > capacity()) {
            split_buffer<value_type, allocator_type&> sb(allocator_(), count, size());
            sb.construct_at_end(count - size(), value);
            swap_out_buffer(std::move(sb));

        } else {
            construct_at_end(count - size(), value);
        }
    }

    void swap(vector& other) noexcept {
        using std::swap;

        swap(begin_, other.begin_);
        swap(end_, other.end_);
        swap(end_cap_(), other.end_cap_());

        swap_alloc(other, typename alloc_traits::propagate_on_container_swap{});
    }

}; // class vector

template<class T, class Allocator>
std::ostream& operator<<(std::ostream& out, const vector<T, Allocator>& v) {
#ifdef CIEL_HAS_RTTI
    out << ciel::demangle(typeid(v).name()) << ": ";
#endif
    out << "[ ";

    for (auto it = v.begin(); it != v.end(); ++it) {
        out << *it << ", ";
    }

    out << "__" << v.capacity() - v.size() << "__ ]";
    return out;
}

template<class T, class Allocator>
struct is_trivially_relocatable<vector<T, Allocator>>
    : conjunction<is_trivially_relocatable<Allocator>,
                  is_trivially_relocatable<typename std::allocator_traits<remove_reference_t<Allocator>>::pointer>> {};

template<class T, class Alloc, class U>
typename vector<T, Alloc>::size_type erase(vector<T, Alloc>& c, const U& value) {
    auto it        = std::remove(c.begin(), c.end(), value);
    const auto res = std::distance(it, c.end());
    c.erase(it, c.end());
    return res;
}

template<class T, class Alloc, class Pred>
typename vector<T, Alloc>::size_type erase_if(vector<T, Alloc>& c, Pred pred) {
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
void swap(ciel::vector<T, Alloc>& lhs, ciel::vector<T, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_VECTOR_HPP_

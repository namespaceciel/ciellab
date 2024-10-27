#ifndef CIELLAB_INCLUDE_CIEL_VECTOR_HPP_
#define CIELLAB_INCLUDE_CIEL_VECTOR_HPP_

#include <ciel/allocator_traits.hpp>
#include <ciel/compare.hpp>
#include <ciel/compressed_pair.hpp>
#include <ciel/config.hpp>
#include <ciel/copy_n.hpp>
#include <ciel/cstring.hpp>
#include <ciel/is_range.hpp>
#include <ciel/is_trivially_relocatable.hpp>
#include <ciel/iterator_category.hpp>
#include <ciel/range_destroyer.hpp>
#include <ciel/split_buffer.hpp>

#include <algorithm>
#include <cstddef>
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
    compressed_pair<pointer, allocator_type> end_cap_alloc_{nullptr, value_init};

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
            CIEL_THROW_EXCEPTION(std::length_error("ciel::vector reserving size is beyond max_size"));
        }

        const size_type cap = capacity();

        if CIEL_UNLIKELY (cap >= ms / 2) {
            return ms;
        }

        return std::max(cap * 2, new_size);
    }

    CIEL_NODISCARD bool
    internal_value(const value_type& value) const noexcept {
        if (should_pass_by_value) {
            return false;
        }

        if CIEL_UNLIKELY (begin_ <= std::addressof(value) && std::addressof(value) < end_) {
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
                (ciel::to_address(p))->~value_type();
            }

        } else {
            alloc_traits::destroy(allocator_(), ciel::to_address(p));
        }
    }

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
#ifdef CIEL_HAS_EXCEPTIONS
                    sb.unchecked_emplace_front_aux(std::move_if_noexcept(*p));
#else
                    sb.unchecked_emplace_front_aux(std::move(*p));
#endif
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
        CIEL_PRECONDITION(begin_ == nullptr);
        CIEL_PRECONDITION(end_ == nullptr);
        CIEL_PRECONDITION(end_cap_() == nullptr);

        begin_     = alloc_traits::allocate(allocator_(), count);
        end_cap_() = begin_ + count;
        end_       = begin_;
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

    template<class Iter, typename std::enable_if<is_exactly_input_iterator<Iter>::value, int>::type = 0>
    vector(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        for (; first != last; ++first) {
            emplace_back_aux(*first);
        }
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    vector(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        const auto count = std::distance(first, last);

        if CIEL_LIKELY (count > 0) {
            init(count);
            construct_at_end(first, last);
        }
    }

    CIEL_DIAGNOSTIC_PUSH
    CIEL_GCC_DIAGNOSTIC_IGNORED("-Wunused-result")

    vector(const vector& other)
        : vector(other.begin(), other.end(),
                 alloc_traits::select_on_container_copy_construction(other.get_allocator())) {}

    CIEL_DIAGNOSTIC_POP

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

    // non-standard extension
    template<class R,
             typename std::enable_if<is_range_without_size<R>::value && std::is_lvalue_reference<R>::value, int>::type
             = 0>
    vector(from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : vector(rg.begin(), rg.end(), alloc) {}

    // non-standard extension
    template<class R,
             typename std::enable_if<is_range_without_size<R>::value && !std::is_lvalue_reference<R>::value, int>::type
             = 0>
    vector(from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : vector(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()), alloc) {}

    // non-standard extension
    template<class R,
             typename std::enable_if<is_range_with_size<R>::value && std::is_lvalue_reference<R>::value, int>::type = 0>
    vector(from_range_t, R&& rg, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        const auto count = rg.size();

        if CIEL_LIKELY (count > 0) {
            init(count);
            construct_at_end(rg.begin(), rg.end());
        }
    }

    // non-standard extension
    template<class R,
             typename std::enable_if<is_range_with_size<R>::value && !std::is_lvalue_reference<R>::value, int>::type
             = 0>
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
        if CIEL_UNLIKELY (this == std::addressof(other)) {
            return *this;
        }

        if (alloc_traits::propagate_on_container_copy_assignment::value) {
            if (allocator_() != other.allocator_()) {
                reset();
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

        if (alloc_traits::propagate_on_container_move_assignment::value) {
            swap(other);
            // other.clear(); // It's not neccessary but...

        } else {
            reset(other.size());
            construct_at_end(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
        }

        return *this;
    }

    vector&
    operator=(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    void
    assign(const size_type count, lvalue value) {
        if (capacity() < count) {
            if (internal_value(value)) {
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

    CIEL_NODISCARD allocator_type
    get_allocator() const noexcept {
        return allocator_();
    }

    CIEL_NODISCARD reference
    at(const size_type pos) {
        if CIEL_UNLIKELY (pos >= size()) {
            CIEL_THROW_EXCEPTION(std::out_of_range("pos is not within the range of ciel::vector"));
        }

        return begin_[pos];
    }

    CIEL_NODISCARD const_reference
    at(const size_type pos) const {
        if CIEL_UNLIKELY (pos >= size()) {
            CIEL_THROW_EXCEPTION(std::out_of_range("pos is not within the range of ciel::vector"));
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

    void
    clear() noexcept {
        end_ = destroy(begin_, end_);
    }

private:
    class emplace_impl_callback {
    private:
        vector* const this_;

    public:
        emplace_impl_callback(vector* const t) noexcept
            : this_{t} {}

        template<class... Args, class U = value_type,
                 typename std::enable_if<is_trivially_relocatable<U>::value, int>::type = 0>
        void
        operator()(pointer pos, Args&&... args) const {
            constexpr size_type count = 1;
            ciel::memmove(pos + count, pos, sizeof(value_type) * (this_->end_ - pos));

            range_destroyer<value_type, allocator_type&> rd{pos + count, this_->end_ + count, this_->allocator_()};
            const pointer old_end = this_->end_;
            this_->end_           = pos;

            this_->unchecked_emplace_back_aux(std::forward<Args>(args)...);

            this_->end_ = old_end + count;
            rd.release();
        }

        template<class... Args, class U = value_type,
                 typename std::enable_if<!is_trivially_relocatable<U>::value, int>::type = 0>
        void
        operator()(pointer pos, Args&&... args) const {
            this_->move_range(pos, this_->end_, pos + 1);
            *pos = value_type{std::forward<Args>(args)...};
        }

        template<class U, class URaw = typename std::decay<U>::type,
                 typename std::enable_if<
                     std::is_same<value_type, URaw>::value && !is_trivially_relocatable<URaw>::value, int>::type
                 = 0>
        void
        operator()(pointer pos, U&& value) const {
            this_->move_range(pos, this_->end_, pos + 1);
            *pos = std::forward<U>(value);
        }
    };

    template<class... Args>
    iterator
    emplace_impl(const emplace_impl_callback cb, pointer pos, Args&&... args) {
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
            cb(pos, std::forward<Args>(args)...);
        }

        return begin() + pos_index;
    }

public:
    // Note that emplace is not a superset of insert when pos is not at the end.
    template<class... Args>
    iterator
    emplace(const_iterator p, Args&&... args) {
        pointer pos = begin_ + (p - begin());

        return emplace_impl(emplace_impl_callback{this}, pos, std::forward<Args>(args)...);
    }

    // non-standard extension
    template<class U, class... Args>
    iterator
    emplace(const_iterator p, std::initializer_list<U> il, Args&&... args) {
        pointer pos = begin_ + (p - begin());

        return emplace_impl(emplace_impl_callback{this}, pos, il, std::forward<Args>(args)...);
    }

    iterator
    insert(const_iterator p, lvalue value) {
        pointer pos = begin_ + (p - begin());

        if (internal_value(value)) {
            value_type copy = value;
            return emplace_impl(emplace_impl_callback{this}, pos, std::move(copy));

        } else {
            return emplace_impl(emplace_impl_callback{this}, pos, value);
        }
    }

    template<bool Valid = !should_pass_by_value, typename std::enable_if<Valid, int>::type = 0>
    iterator
    insert(const_iterator p, rvalue value) {
        pointer pos = begin_ + (p - begin());

        if (internal_value(value)) {
            value_type copy = std::move(value);
            return emplace_impl(emplace_impl_callback{this}, pos, std::move(copy));

        } else {
            return emplace_impl(emplace_impl_callback{this}, pos, std::move(value));
        }
    }

private:
    void
    move_range(pointer from_s, pointer from_e, pointer to) {
        pointer old_end   = end_;
        difference_type n = old_end - to;

        for (pointer p = from_s + n; p < from_e; ++p) {
            unchecked_emplace_back_aux(std::move(*p));
        }

        std::move_backward(from_s, from_s + n, old_end);
    }

    template<class T1, class T2, class U = value_type,
             typename std::enable_if<is_trivially_relocatable<U>::value, int>::type = 0>
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
             typename std::enable_if<is_forward_iterator<Iter>::value && !is_trivially_relocatable<U>::value, int>::type
             = 0>
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

    template<class U = value_type, typename std::enable_if<!is_trivially_relocatable<U>::value, int>::type = 0>
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
            if (internal_value(value)) {
                value_type copy = value;
                insert_impl(pos, count, copy, count);

            } else {
                insert_impl(pos, count, value, count);
            }
        }

        return begin() + pos_index;
    }

    // Construct them all at the end at first, then rotate them to the right place.
    template<class Iter, typename std::enable_if<is_exactly_input_iterator<Iter>::value, int>::type = 0>
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

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
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
                ciel::memcpy(first, last, sizeof(value_type) * back_count);

            } else {
                ciel::memmove(first, last, sizeof(value_type) * back_count);
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
        const iterator pos = begin() + (p - begin());
        CIEL_PRECONDITION(begin() <= pos);
        CIEL_PRECONDITION(pos < end());

        return erase_impl(ciel::to_address(pos), ciel::to_address(pos + 1), 1);
    }

    iterator
    erase(const_iterator f, const_iterator l) {
        const iterator first = begin() + (f - begin());
        const iterator last  = begin() + (l - begin());
        CIEL_PRECONDITION(begin() <= first);
        CIEL_PRECONDITION(last <= end());

        const auto count = last - first;

        if CIEL_UNLIKELY (count <= 0) {
            return last;
        }

        return erase_impl(ciel::to_address(first), ciel::to_address(last), count);
    }

private:
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

    template<class... Args>
    void
    unchecked_emplace_back_aux(Args&&... args) {
        CIEL_PRECONDITION(end_ < end_cap_());

        construct(end_, std::forward<Args>(args)...);
        ++end_;
    }

public:
    void
    push_back(lvalue value) {
        emplace_back_aux(value);
    }

    template<bool Valid = !should_pass_by_value, typename std::enable_if<Valid, int>::type = 0>
    void
    push_back(rvalue value) {
        emplace_back_aux(std::move(value));
    }

    template<class... Args>
    reference
    emplace_back(Args&&... args) {
        emplace_back_aux(std::forward<Args>(args)...);

        return back();
    }

    // non-standard extension
    template<class U, class... Args>
    reference
    emplace_back(std::initializer_list<U> il, Args&&... args) {
        emplace_back_aux(il, std::forward<Args>(args)...);

        return back();
    }

    // non-standard extension
    template<class... Args>
    reference
    unchecked_emplace_back(Args&&... args) {
        unchecked_emplace_back_aux(std::forward<Args>(args)...);

        return back();
    }

    // non-standard extension
    template<class U, class... Args>
    reference
    unchecked_emplace_back(std::initializer_list<U> il, Args&&... args) {
        unchecked_emplace_back_aux(il, std::forward<Args>(args)...);

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

    // non-standard extension
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

    // non-standard extension
    template<class R, typename std::enable_if<is_range<R>::value, int>::type = 0>
    void
    append_range(R&& rg) {
        insert_range(end(), std::forward<R>(rg));
    }

    // non-standard extension
    template<class R, typename std::enable_if<is_range<R>::value, int>::type = 0>
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

template<class T>
struct is_trivially_relocatable<std::allocator<T>> : std::true_type {};

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

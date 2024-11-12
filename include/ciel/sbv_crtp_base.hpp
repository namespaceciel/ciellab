#ifndef CIELLAB_INCLUDE_CIEL_SBV_CRTP_BASE_HPP_
#define CIELLAB_INCLUDE_CIEL_SBV_CRTP_BASE_HPP_

#include <ciel/allocator_traits.hpp>
#include <ciel/config.hpp>
#include <ciel/copy_n.hpp>
#include <ciel/is_range.hpp>
#include <ciel/to_address.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

template<class T, class Allocator, class Derived>
struct sbv_crtp_base {
    using value_type             = T;
    using allocator_type         = remove_reference_t<Allocator>;
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

    using alloc_traits = std::allocator_traits<allocator_type>;

    static constexpr bool should_pass_by_value =
        std::is_trivially_copyable<value_type>::value && sizeof(value_type) <= 16;
    using lvalue = conditional_t<should_pass_by_value, value_type, const value_type&>;
    using rvalue = conditional_t<should_pass_by_value, value_type, value_type&&>;

    CIEL_NODISCARD Derived* this_() noexcept {
        return static_cast<Derived*>(this);
    }

    CIEL_NODISCARD const Derived* this_() const noexcept {
        return static_cast<const Derived*>(this);
    }

    CIEL_NODISCARD pointer& end_cap_() noexcept {
        return this_()->end_cap_alloc_.first();
    }

    CIEL_NODISCARD const pointer& end_cap_() const noexcept {
        return this_()->end_cap_alloc_.first();
    }

    allocator_type& allocator_() noexcept { // NOLINT(modernize-use-nodiscard)
        return this_()->end_cap_alloc_.second();
    }

    const allocator_type& allocator_() const noexcept { // NOLINT(modernize-use-nodiscard)
        return this_()->end_cap_alloc_.second();
    }

    CIEL_NODISCARD size_type recommend_cap(const size_type new_size) const {
        CIEL_PRECONDITION(new_size > 0);

        const size_type ms = max_size();

        if CIEL_UNLIKELY (new_size > ms) {
            CIEL_THROW_EXCEPTION(std::length_error("reserving size is beyond max_size"));
        }

        const size_type cap = this_()->capacity();

        if CIEL_UNLIKELY (cap >= ms / 2) {
            return ms;
        }

        return std::max(cap * 2, new_size);
    }

    CIEL_NODISCARD bool internal_value(const value_type& value, pointer begin) const noexcept {
        if (should_pass_by_value) {
            return false;
        }

        if CIEL_UNLIKELY (ciel::to_address(begin) <= std::addressof(value)
                          && std::addressof(value) < ciel::to_address(this_()->end_)) {
            return true;
        }

        return false;
    }

    template<class... Args>
    void construct(pointer p, Args&&... args) {
        alloc_traits::construct(allocator_(), ciel::to_address(p), std::forward<Args>(args)...);
    }

    void destroy(pointer p) noexcept {
        CIEL_PRECONDITION(this_()->begin_ <= p);
        CIEL_PRECONDITION(p < this_()->end_);

        alloc_traits::destroy(allocator_(), ciel::to_address(p));
    }

    pointer destroy(pointer first, pointer last) noexcept {
        CIEL_PRECONDITION(this_()->begin_ <= first);
        CIEL_PRECONDITION(first <= last);
        CIEL_PRECONDITION(last <= this_()->end_);

        const pointer res = first;

        for (; first != last; ++first) {
            alloc_traits::destroy(allocator_(), ciel::to_address(first));
        }

        return res;
    }

    void construct_at_end(const size_type n) {
        CIEL_PRECONDITION(this_()->end_ + n <= end_cap_());

        for (size_type i = 0; i < n; ++i) {
            unchecked_emplace_back();
        }
    }

    void construct_at_end(const size_type n, lvalue value) {
        CIEL_PRECONDITION(this_()->end_ + n <= end_cap_());

        for (size_type i = 0; i < n; ++i) {
            unchecked_emplace_back(value);
        }
    }

    template<class Iter>
    void construct_at_end(Iter first, Iter last) {
        ciel::uninitialized_copy(allocator_(), first, last, this_()->end_);
    }

    void reset() noexcept {
        this_()->do_destroy();
        this_()->set_nullptr();
    }

    void reset(const size_type count) {
        CIEL_PRECONDITION(count != 0);

        this_()->do_destroy();
        this_()->set_nullptr(); // It's neccessary since allocation would throw.
        this_()->init(count);
    }

    void copy_assign_alloc(const Derived& other, std::true_type) {
        if (allocator_() != other.allocator_()) {
            reset();
        }

        allocator_() = other.allocator_();
    }

    void copy_assign_alloc(const Derived&, std::false_type) noexcept {}

    void swap_alloc(Derived& other, std::true_type) noexcept {
        using std::swap;
        swap(allocator_(), other.allocator_());
    }

    void swap_alloc(Derived&, std::false_type) noexcept {}

    template<class... Args>
    void unchecked_emplace_back_aux(Args&&... args) {
        CIEL_PRECONDITION(this_()->end_ < end_cap_());

        construct(this_()->end_, std::forward<Args>(args)...);
        ++this_()->end_;
    }

    // NOLINTNEXTLINE(misc-unconventional-assign-operator)
    Derived& operator=(const Derived& other) {
        if CIEL_UNLIKELY (this == std::addressof(other)) {
            return *(this_());
        }

        copy_assign_alloc(other, typename alloc_traits::propagate_on_container_copy_assignment{});
        assign(other.begin(), other.end());

        return *(this_());
    }

    // NOLINTNEXTLINE(misc-unconventional-assign-operator)
    Derived& operator=(Derived&& other) noexcept(alloc_traits::propagate_on_container_move_assignment::value
                                                 || alloc_traits::is_always_equal::value) {
        if CIEL_UNLIKELY (this == std::addressof(other)) {
            return *(this_());
        }

        if (alloc_traits::propagate_on_container_move_assignment::value || allocator_() == other.allocator_()) {
            this_()->swap(other);

        } else {
            assign(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
        }

        return *(this_());
    }

    // NOLINTNEXTLINE(misc-unconventional-assign-operator)
    Derived& operator=(std::initializer_list<value_type> ilist) {
        this_()->assign(ilist.begin(), ilist.end(), ilist.size());
        return *(this_());
    }

    template<class Iter, enable_if_t<is_forward_iterator<Iter>::value> = 0>
    void assign(Iter first, Iter last) {
        const size_type count = std::distance(first, last);

        this_()->assign(first, last, count);
    }

    template<class Iter, enable_if_t<is_exactly_input_iterator<Iter>::value> = 0>
    void assign(Iter first, Iter last) {
        pointer p = this_()->begin_;
        for (; first != last && p != this_()->end_; ++first) {
            *p = *first;
            ++p;
        }

        if (p != this_()->end_) {
            this_()->end_ = destroy(p, this_()->end_);

        } else {
            for (; first != last; ++first) {
                this_()->emplace_back(*first);
            }
        }
    }

    void assign(std::initializer_list<value_type> ilist) {
        this_()->assign(ilist.begin(), ilist.end(), ilist.size());
    }

    template<class R, enable_if_t<is_range<R>::value> = 0>
    void assign_range(R&& rg) {
        if (is_range_with_size<R>::value) {
            if (std::is_lvalue_reference<R>::value) {
                this_()->assign(rg.begin(), rg.end(), rg.size());

            } else {
                this_()->assign(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()), rg.size());
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
            CIEL_THROW_EXCEPTION(std::out_of_range("pos is not within the range"));
        }

        return this_()->begin_[pos];
    }

    CIEL_NODISCARD const_reference at(const size_type pos) const {
        if CIEL_UNLIKELY (pos >= size()) {
            CIEL_THROW_EXCEPTION(std::out_of_range("pos is not within the range"));
        }

        return this_()->begin_[pos];
    }

    CIEL_NODISCARD reference operator[](const size_type pos) {
        CIEL_PRECONDITION(pos < size());

        return this_()->begin_[pos];
    }

    CIEL_NODISCARD const_reference operator[](const size_type pos) const {
        CIEL_PRECONDITION(pos < size());

        return this_()->begin_[pos];
    }

    CIEL_NODISCARD reference front() {
        CIEL_PRECONDITION(!empty());

        return this_()->begin_[0];
    }

    CIEL_NODISCARD const_reference front() const {
        CIEL_PRECONDITION(!empty());

        return this_()->begin_[0];
    }

    CIEL_NODISCARD reference back() {
        CIEL_PRECONDITION(!empty());

        return *(this_()->end_ - 1);
    }

    CIEL_NODISCARD const_reference back() const {
        CIEL_PRECONDITION(!empty());

        return *(this_()->end_ - 1);
    }

    CIEL_NODISCARD T* data() noexcept {
        return ciel::to_address(this_()->begin_);
    }

    CIEL_NODISCARD const T* data() const noexcept {
        return ciel::to_address(this_()->begin_);
    }

    CIEL_NODISCARD iterator begin() noexcept {
        return iterator(this_()->begin_);
    }

    CIEL_NODISCARD const_iterator begin() const noexcept {
        return const_iterator(this_()->begin_);
    }

    CIEL_NODISCARD const_iterator cbegin() const noexcept {
        return begin();
    }

    CIEL_NODISCARD iterator end() noexcept {
        return iterator(this_()->end_);
    }

    CIEL_NODISCARD const_iterator end() const noexcept {
        return const_iterator(this_()->end_);
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
        return this_()->begin_ == this_()->end_;
    }

    CIEL_NODISCARD size_type size() const noexcept {
        return this_()->end_ - this_()->begin_;
    }

    CIEL_NODISCARD size_type max_size() const noexcept {
        return std::min<size_type>(std::numeric_limits<difference_type>::max(), alloc_traits::max_size(allocator_()));
    }

    void clear() noexcept {
        this_()->end_ = destroy(this_()->begin_, this_()->end_);
    }

    iterator erase(const_iterator p) {
        const pointer pos = this_()->begin_ + (p - begin());
        CIEL_PRECONDITION(this_()->begin_ <= pos);
        CIEL_PRECONDITION(pos < this_()->end_);

        return this_()->erase_impl(pos, pos + 1, 1);
    }

    iterator erase(const_iterator f, const_iterator l) {
        const pointer first = this_()->begin_ + (f - begin());
        const pointer last  = this_()->begin_ + (l - begin());
        CIEL_PRECONDITION(this_()->begin_ <= first);
        CIEL_PRECONDITION(last <= this_()->end_);

        const auto count = last - first;

        if CIEL_UNLIKELY (count <= 0) {
            return last;
        }

        return this_()->erase_impl(first, last, count);
    }

    void push_back(lvalue value) {
        this_()->emplace_back(value);
    }

    template<bool Valid = !should_pass_by_value, enable_if_t<Valid> = 0>
    void push_back(rvalue value) {
        this_()->emplace_back(std::move(value));
    }

    template<class... Args>
    reference emplace_back(Args&&... args) {
        this_()->emplace_back_aux(std::forward<Args>(args)...);

        return back();
    }

    template<class U, class... Args>
    reference emplace_back(std::initializer_list<U> il, Args&&... args) {
        this_()->emplace_back_aux(il, std::forward<Args>(args)...);

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

    void pop_back() noexcept {
        CIEL_PRECONDITION(!empty());

        destroy(this_()->end_ - 1);
        --this_()->end_;
    }

}; // struct sbv_crtp_base

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_SBV_CRTP_BASE_HPP_

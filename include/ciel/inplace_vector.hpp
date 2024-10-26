#ifndef CIELLAB_INCLUDE_CIEL_INPLACE_VECTOR_HPP_
#define CIELLAB_INCLUDE_CIEL_INPLACE_VECTOR_HPP_

#include <ciel/compare.hpp>
#include <ciel/config.hpp>
#include <ciel/copy_n.hpp>
#include <ciel/cstring.hpp>
#include <ciel/is_range.hpp>
#include <ciel/is_trivially_relocatable.hpp>
#include <ciel/iterator_category.hpp>
#include <ciel/range_destroyer.hpp>
#include <ciel/swap.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>

NAMESPACE_CIEL_BEGIN

namespace detail {

// By employing the Curiously Recurring Template Pattern (CRTP),
// we can conditionally provide trivial copy constructor based on
// whether the template type T has a trivial copy constructor.
// This approach allows the compiler to generate more efficient code,
// such as using memcpy for types that support trivial copying.
// Similarly, it enhances the handling of move constructors and destructors for other types.

// maybe_has_trivial_copy_constructor
struct has_trivial_copy_constructor {};

template<class T, class D>
struct has_non_trivial_copy_constructor {
    has_non_trivial_copy_constructor(const has_non_trivial_copy_constructor& o) noexcept(
        std::is_nothrow_copy_constructible<T>::value) {
        D& self        = static_cast<D&>(*this);
        const D& other = static_cast<const D&>(o);
        self.size_     = 0;
        self.construct_at_end(other.begin(), other.end());
    }
};

// clang-format off
template<class T, class D>
using maybe_has_trivial_copy_constructor =
    typename std::conditional<std::is_trivially_copy_constructible<T>::value,
                              has_trivial_copy_constructor,
                              has_non_trivial_copy_constructor<T, D>
    >::type;
// clang-format on

// maybe_has_trivial_move_constructor
struct has_trivial_move_constructor {};

template<class T, class D>
struct has_non_trivial_move_constructor {
    has_non_trivial_move_constructor(has_non_trivial_move_constructor&& o) noexcept(
        std::is_nothrow_move_constructible<T>::value) {
        D& self    = static_cast<D&>(*this);
        D&& other  = static_cast<D&&>(o);
        self.size_ = 0;
        self.construct_at_end(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
    }
};

template<class D>
struct has_trivially_relocatable_move_constructor {
    has_trivially_relocatable_move_constructor(has_trivially_relocatable_move_constructor&& o) noexcept {
        ciel::memcpy(this, &o, sizeof(D));
        static_cast<D&&>(o).size_ = 0;
    }
};

// clang-format off
template<class T, class D>
using maybe_has_trivial_move_constructor =
    typename std::conditional<std::is_trivially_move_constructible<T>::value,
                              has_trivial_move_constructor,
                              typename std::conditional<is_trivially_relocatable<T>::value,
                                                        has_trivially_relocatable_move_constructor<D>,
                                                        has_non_trivial_move_constructor<T, D>
                              >::type
    >::type;
// clang-format on

// maybe_has_trivial_copy_assignment
struct has_trivial_copy_assignment {};

template<class T, class D>
struct has_non_trivial_copy_assignment {
    has_non_trivial_copy_assignment&
    operator=(const has_non_trivial_copy_assignment& o) noexcept(std::is_nothrow_copy_assignable<T>::value) {
        if CIEL_UNLIKELY (this == std::addressof(o)) {
            return *this;
        }

        D& self        = static_cast<D&>(*this);
        const D& other = static_cast<const D&>(o);
        self.assign(other.begin(), other.end(), other.size());

        return *this;
    }
};

// clang-format off
template<class T, class D>
using maybe_has_trivial_copy_assignment =
    typename std::conditional<std::is_trivially_copy_constructible<T>::value
                                  && std::is_trivially_copy_assignable<T>::value
                                  && std::is_trivially_destructible<T>::value,
                              has_trivial_copy_assignment,
                              has_non_trivial_copy_assignment<T, D>
    >::type;
// clang-format on

// maybe_has_trivial_move_assignment
struct has_trivial_move_assignment {};

template<class T, class D>
struct has_non_trivial_move_assignment {
    has_non_trivial_move_assignment&
    operator=(has_non_trivial_move_assignment&& o) noexcept(std::is_nothrow_move_assignable<T>::value) {
        if CIEL_UNLIKELY (this == std::addressof(o)) {
            return *this;
        }

        D& self   = static_cast<D&>(*this);
        D&& other = static_cast<D&&>(o);
        self.assign(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()), other.size());

        return *this;
    }
};

template<class D>
struct has_trivially_relocatable_move_assignment {
    has_trivially_relocatable_move_assignment&
    operator=(has_trivially_relocatable_move_assignment&& o) noexcept {
        if CIEL_UNLIKELY (this == std::addressof(o)) {
            return *this;
        }

        ciel::memcpy(this, &o, sizeof(D));
        static_cast<D&&>(o).size_ = 0;

        return *this;
    }
};

// clang-format off
template<class T, class D>
using maybe_has_trivial_move_assignment =
    typename std::conditional<std::is_trivially_move_constructible<T>::value
                                  && std::is_trivially_move_assignable<T>::value
                                  && std::is_trivially_destructible<T>::value,
                              has_trivial_move_assignment,
                              typename std::conditional<is_trivially_relocatable<T>::value,
                                                        has_trivially_relocatable_move_assignment<D>,
                                                        has_non_trivial_move_assignment<T, D>
                              >::type
    >::type;
// clang-format on

// maybe_has_trivial_destructor
struct has_trivial_destructor {};

template<class D>
struct has_non_trivial_destructor {
    ~has_non_trivial_destructor() {
        D& self = static_cast<D&>(*this);
        self.clear();
    }
};

// clang-format off
template<class T, class D>
using maybe_has_trivial_destructor =
    typename std::conditional<std::is_trivially_destructible<T>::value,
                              has_trivial_destructor,
                              has_non_trivial_destructor<D>
    >::type;
// clang-format on

} // namespace detail

template<class T, uint64_t Capacity>
class inplace_vector : detail::maybe_has_trivial_copy_constructor<T, inplace_vector<T, Capacity>>,
                       detail::maybe_has_trivial_move_constructor<T, inplace_vector<T, Capacity>>,
                       detail::maybe_has_trivial_copy_assignment<T, inplace_vector<T, Capacity>>,
                       detail::maybe_has_trivial_move_assignment<T, inplace_vector<T, Capacity>>,
                       detail::maybe_has_trivial_destructor<T, inplace_vector<T, Capacity>> {
    static_assert(Capacity > 0, "");

    // clang-format off
    using inplace_vector_size_type =
        typename std::conditional<Capacity <= std::numeric_limits<uint8_t>::max(), uint8_t,
        typename std::conditional<Capacity <= std::numeric_limits<uint16_t>::max(), uint16_t,
        typename std::conditional<Capacity <= std::numeric_limits<uint32_t>::max(), uint32_t, uint64_t>::type>::type>::type;
    // clang-format on

public:
    using value_type             = T;
    using size_type              = size_t;
    using difference_type        = ptrdiff_t;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    inplace_vector_size_type size_;

    union {
        value_type data_[Capacity];
    };

    CIEL_NODISCARD pointer
    begin_() noexcept {
        return data_;
    }

    CIEL_NODISCARD const_pointer
    begin_() const noexcept {
        return data_;
    }

    CIEL_NODISCARD pointer
    end_() noexcept {
        return begin_() + size();
    }

    CIEL_NODISCARD const_pointer
    end_() const noexcept {
        return begin_() + size();
    }

    CIEL_NODISCARD pointer
    end_cap_() noexcept {
        return begin_() + capacity();
    }

    CIEL_NODISCARD const_pointer
    end_cap_() const noexcept {
        return begin_() + capacity();
    }

    void
    construct_at_end(const size_type n) {
        CIEL_PRECONDITION(size() + n <= capacity());

        for (size_type i = 0; i < n; ++i) {
            ::new (end_()) value_type{};
            ++size_;
        }
    }

    void
    construct_at_end(const size_type n, const value_type& value) {
        CIEL_PRECONDITION(size() + n <= capacity());

        for (size_type i = 0; i < n; ++i) {
            ::new (end_()) value_type{value};
            ++size_;
        }
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    void
    construct_at_end(Iter first, Iter last) {
        CIEL_PRECONDITION(size() + std::distance(first, last) <= capacity());

        while (first != last) {
            ::new (end_()) value_type{*first};
            ++first;
            ++size_;
        }
    }

    template<class U = value_type, typename std::enable_if<std::is_trivially_destructible<U>::value, int>::type = 0>
    void
    range_destroy(pointer begin, pointer end) noexcept {
        CIEL_PRECONDITION(begin <= end);
        CIEL_PRECONDITION(begin_() <= begin);
        CIEL_PRECONDITION(end <= end_());

        size_ -= std::distance(begin, end);
    }

    template<class U = value_type, typename std::enable_if<!std::is_trivially_destructible<U>::value, int>::type = 0>
    void
    range_destroy(pointer begin, pointer end) noexcept {
        CIEL_PRECONDITION(begin <= end);
        CIEL_PRECONDITION(begin_() <= begin);
        CIEL_PRECONDITION(end <= end_());

        while (end != begin) {
            --size_;
            --end;
            end->~value_type();
        }
    }

    template<class... Args>
    reference
    emplace_back_aux(Args&&... args) {
        if (size() == capacity()) {
            ciel::throw_exception(std::bad_alloc{});

        } else {
            return unchecked_emplace_back(std::forward<Args>(args)...);
        }

        return back();
    }

    template<class... Args>
    reference
    unchecked_emplace_back_aux(Args&&... args) {
        CIEL_PRECONDITION(size() < capacity());

        ::new (end_()) value_type{std::forward<Args>(args)...};
        ++size_;

        return back();
    }

    template<class Iter>
    void
    assign(Iter first, Iter last, const size_type count) {
        if (capacity() < count) {
            ciel::throw_exception(std::bad_alloc{});
        }

        if (size() > count) {
            range_destroy(begin_() + count, end_());
        }

        CIEL_POSTCONDITION(size() <= count);

        Iter mid = ciel::copy_n(first, size(), begin_());

        // if mid < last
        construct_at_end(mid, last);

        CIEL_POSTCONDITION(size() == count);
    }

public:
    inplace_vector() noexcept
        : size_(0) {}

    inplace_vector(const size_type count, const value_type& value)
        : inplace_vector() {
        if CIEL_UNLIKELY (count > capacity()) {
            ciel::throw_exception(std::bad_alloc{});
        }

        construct_at_end(count, value);
    }

    explicit inplace_vector(const size_type count)
        : inplace_vector() {
        if CIEL_UNLIKELY (count > capacity()) {
            ciel::throw_exception(std::bad_alloc{});
        }

        construct_at_end(count);
    }

    template<class Iter, typename std::enable_if<is_exactly_input_iterator<Iter>::value, int>::type = 0>
    inplace_vector(Iter first, Iter last)
        : inplace_vector() {
        while (first != last) {
            emplace_back(*first);
            ++first;
        }
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    inplace_vector(Iter first, Iter last)
        : inplace_vector() {
        const size_type count = std::distance(first, last);
        if CIEL_UNLIKELY (count > capacity()) {
            ciel::throw_exception(std::bad_alloc{});
        }

        construct_at_end(first, last);
    }

    inplace_vector(const inplace_vector& other) noexcept(std::is_nothrow_copy_constructible<T>::value) = default;
    inplace_vector(inplace_vector&& other) noexcept(std::is_nothrow_move_constructible<T>::value)      = default;

    template<class R,
             typename std::enable_if<is_range_without_size<R>::value && std::is_lvalue_reference<R>::value, int>::type
             = 0>
    inplace_vector(from_range_t, R&& rg)
        : inplace_vector(rg.begin(), rg.end()) {}

    template<class R,
             typename std::enable_if<is_range_without_size<R>::value && !std::is_lvalue_reference<R>::value, int>::type
             = 0>
    inplace_vector(from_range_t, R&& rg)
        : inplace_vector(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end())) {}

    template<class R,
             typename std::enable_if<is_range_with_size<R>::value && std::is_lvalue_reference<R>::value, int>::type = 0>
    inplace_vector(from_range_t, R&& rg)
        : inplace_vector() {
        const size_type count = rg.size();
        if CIEL_UNLIKELY (count > capacity()) {
            ciel::throw_exception(std::bad_alloc{});
        }

        construct_at_end(rg.begin(), rg.end());
    }

    template<class R,
             typename std::enable_if<is_range_with_size<R>::value && !std::is_lvalue_reference<R>::value, int>::type
             = 0>
    inplace_vector(from_range_t, R&& rg)
        : inplace_vector() {
        const size_type count = rg.size();
        if CIEL_UNLIKELY (count > capacity()) {
            ciel::throw_exception(std::bad_alloc{});
        }

        construct_at_end(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()));
    }

    inplace_vector(std::initializer_list<value_type> init)
        : inplace_vector(init.begin(), init.end()) {}

    ~inplace_vector() = default;

    // clang-format off
    inplace_vector& operator=(const inplace_vector& other) noexcept(std::is_nothrow_copy_assignable<value_type>::value) = default;
    inplace_vector& operator=(inplace_vector&& other)
        noexcept(std::is_nothrow_move_assignable<value_type>::value || is_trivially_relocatable<value_type>::value) = default;
    // clang-format on

    inplace_vector&
    operator=(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    void
    assign(const size_type count, const value_type& value) {
        if (capacity() < count) {
            ciel::throw_exception(std::bad_alloc{});
        }

        if (size() > count) {
            range_destroy(begin_() + count, end_());
        }

        CIEL_POSTCONDITION(size() <= count);

        std::fill_n(begin_(), size(), value);
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

    CIEL_NODISCARD reference
    at(const size_type pos) {
        if CIEL_UNLIKELY (pos >= size()) {
            ciel::throw_exception(std::out_of_range("pos is not within the range of ciel::inplace_vector"));
        }

        return begin_()[pos];
    }

    CIEL_NODISCARD const_reference
    at(const size_type pos) const {
        if CIEL_UNLIKELY (pos >= size()) {
            ciel::throw_exception(std::out_of_range("pos is not within the range of ciel::inplace_vector"));
        }

        return begin_()[pos];
    }

    CIEL_NODISCARD reference
    operator[](const size_type pos) {
        CIEL_PRECONDITION(pos < size());

        return begin_()[pos];
    }

    CIEL_NODISCARD const_reference
    operator[](const size_type pos) const {
        CIEL_PRECONDITION(pos < size());

        return begin_()[pos];
    }

    CIEL_NODISCARD reference
    front() {
        CIEL_PRECONDITION(!empty());

        return begin_()[0];
    }

    CIEL_NODISCARD const_reference
    front() const {
        CIEL_PRECONDITION(!empty());

        return begin_()[0];
    }

    CIEL_NODISCARD reference
    back() {
        CIEL_PRECONDITION(!empty());

        return *(end_() - 1);
    }

    CIEL_NODISCARD const_reference
    back() const {
        CIEL_PRECONDITION(!empty());

        return *(end_() - 1);
    }

    CIEL_NODISCARD T*
    data() noexcept {
        return begin_();
    }

    CIEL_NODISCARD const T*
    data() const noexcept {
        return begin_();
    }

    CIEL_NODISCARD iterator
    begin() noexcept {
        return iterator(begin_());
    }

    CIEL_NODISCARD const_iterator
    begin() const noexcept {
        return const_iterator(begin_());
    }

    CIEL_NODISCARD const_iterator
    cbegin() const noexcept {
        return begin();
    }

    CIEL_NODISCARD iterator
    end() noexcept {
        return iterator(end_());
    }

    CIEL_NODISCARD const_iterator
    end() const noexcept {
        return const_iterator(end_());
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
        return size() == 0;
    }

    CIEL_NODISCARD size_type
    size() const noexcept {
        return size_;
    }

    CIEL_NODISCARD static constexpr size_type
    max_size() noexcept {
        return Capacity;
    }

    CIEL_NODISCARD static constexpr size_type
    capacity() noexcept {
        return Capacity;
    }

    void
    resize(const size_type count) {
        if CIEL_UNLIKELY (count > capacity()) {
            ciel::throw_exception(std::bad_alloc{});
        }

        if (size() >= count) {
            range_destroy(begin_() + count, end_());
            return;
        }

        construct_at_end(count - size());
    }

    void
    resize(const size_type count, const value_type& value) {
        if CIEL_UNLIKELY (count > capacity()) {
            ciel::throw_exception(std::bad_alloc{});
        }

        if (size() >= count) {
            range_destroy(begin_() + count, end_());
            return;
        }

        construct_at_end(count - size(), value);
    }

    static CIEL_CONSTEXPR_SINCE_CXX14 void
    reserve(const size_type new_cap) {
        if (new_cap > capacity()) {
            ciel::throw_exception(std::bad_alloc{});
        }
    }

    static CIEL_CONSTEXPR_SINCE_CXX14 void
    shrink_to_fit() noexcept {}

    // TODO: insert, insert_range, emplace

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

    void
    push_back(const value_type& value) {
        emplace_back(value);
    }

    void
    push_back(value_type&& value) {
        emplace_back(std::move(value));
    }

    pointer
    try_push_back(const value_type& value) {
        if (size() == capacity()) {
            return nullptr;
        }

        return std::addressof(unchecked_emplace_back(value));
    }

    pointer
    try_push_back(value_type&& value) {
        if (size() == capacity()) {
            return nullptr;
        }

        return std::addressof(unchecked_emplace_back(std::move(value)));
    }

    reference
    unchecked_push_back(const value_type& value) {
        return unchecked_emplace_back(value);
    }

    reference
    unchecked_push_back(value_type&& value) {
        return unchecked_emplace_back(std::move(value));
    }

    void
    pop_back() noexcept {
        CIEL_PRECONDITION(!empty());

        range_destroy(end_() - 1, end_());
    }

    // TODO: append_range, try_append_range

    void
    clear() noexcept {
        range_destroy(begin_(), end_());
    }

    // TODO: erase

    template<class U = inplace_vector, typename std::enable_if<is_trivially_relocatable<U>::value, int>::type = 0>
    void
    swap(inplace_vector& other) noexcept {
        ciel::relocatable_swap(*this, other);
    }

    template<class U = inplace_vector, typename std::enable_if<!is_trivially_relocatable<U>::value, int>::type = 0>
    void
    swap(inplace_vector& other) noexcept(std::is_nothrow_move_constructible<T>::value
                                         && std::is_nothrow_move_assignable<T>::value) {
        inplace_vector* smaller               = this;
        inplace_vector* bigger                = std::addressof(other);
        inplace_vector_size_type smaller_size = smaller->size_;
        inplace_vector_size_type bigger_size  = bigger->size_;

        if (smaller_size > bigger_size) {
            std::swap(smaller, bigger);
            std::swap(smaller_size, bigger_size);
        }

        std::swap_ranges(smaller->begin_(), smaller->begin_() + smaller_size, bigger->begin_());

        for (inplace_vector_size_type i = smaller_size; i != bigger_size; ++i) {
            smaller->unchecked_emplace_back(std::move(bigger->operator[](i)));
        }

        bigger->range_destroy(bigger->begin_() + smaller_size, bigger->end_());
    }

}; // class inplace_vector

template<class T, size_t Capacity>
struct is_trivially_relocatable<inplace_vector<T, Capacity>> : is_trivially_relocatable<T> {};

/*
template<class T, size_t N, class U = T>
typename inplace_vector<T, N>::size_type
erase(inplace_vector<T, N>& c, const U& value) {
    auto it = std::remove(c.begin(), c.end(), value);
    auto r = std::distance(it, c.end());
    c.erase(it, c.end());
    return r;
}

template<class T, size_t N, class Pred>
typename inplace_vector<T, N>::size_type
erase_if(inplace_vector<T, N>& c, Pred pred) {
    auto it = std::remove_if(c.begin(), c.end(), pred);
    auto r = std::distance(it, c.end());
    c.erase(it, c.end());
    return r;
}
*/

NAMESPACE_CIEL_END

namespace std {

template<class T, size_t Capacity>
void
swap(ciel::inplace_vector<T, Capacity>& lhs, ciel::inplace_vector<T, Capacity>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_INPLACE_VECTOR_HPP_

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

template<class, uint64_t>
class inplace_vector;

namespace detail {

// By employing the Curiously Recurring Template Pattern (CRTP),
// we can conditionally provide trivial copy constructor based on
// whether the template type T has a trivial copy constructor.
// This approach allows the compiler to generate more efficient code,
// such as using memcpy for types that support trivial copying.
// Similarly, it enhances the handling of move constructors and destructors for other types.

// inplace_vector_storage

template<class T, uint64_t Capacity, class D>
struct inplace_vector_storage {
    // clang-format off
    using inplace_vector_size_type =
        conditional_t<Capacity <= std::numeric_limits<uint8_t>::max(), uint8_t,
        conditional_t<Capacity <= std::numeric_limits<uint16_t>::max(), uint16_t,
        conditional_t<Capacity <= std::numeric_limits<uint32_t>::max(), uint32_t, uint64_t>>>;
    // clang-format on

    inplace_vector_size_type size_;

    union {
        T data_[Capacity];
        unsigned char null_state_;
    };

    inplace_vector_storage() noexcept
        : size_(0), null_state_() {}

    inplace_vector_storage(const inplace_vector_storage&) = default;
    inplace_vector_storage(inplace_vector_storage&&)      = default;
    // clang-format off
    inplace_vector_storage& operator=(const inplace_vector_storage&) = default;
    inplace_vector_storage& operator=(inplace_vector_storage&&) = default;
    // clang-format on

}; // struct inplace_vector_storage

// maybe_has_trivial_copy_constructor

template<class T, uint64_t Capacity, class D, bool = std::is_trivially_copy_constructible<T>::value>
struct maybe_has_trivial_copy_constructor : inplace_vector_storage<T, Capacity, D> {
    using inplace_vector_storage<T, Capacity, D>::inplace_vector_storage;

    maybe_has_trivial_copy_constructor(const maybe_has_trivial_copy_constructor& o) noexcept(
        std::is_nothrow_copy_constructible<T>::value) {
        D& self        = static_cast<D&>(*this);
        const D& other = static_cast<const D&>(o);
        self.construct_at_end(other.begin_(), other.end_());
    }

    maybe_has_trivial_copy_constructor()                                     = default;
    maybe_has_trivial_copy_constructor(maybe_has_trivial_copy_constructor&&) = default;
    // clang-format off
    maybe_has_trivial_copy_constructor& operator=(const maybe_has_trivial_copy_constructor&) = default;
    maybe_has_trivial_copy_constructor& operator=(maybe_has_trivial_copy_constructor&&) = default;
    // clang-format on

}; // struct maybe_has_trivial_copy_constructor

template<class T, uint64_t Capacity, class D>
struct maybe_has_trivial_copy_constructor<T, Capacity, D, true> : inplace_vector_storage<T, Capacity, D> {
    using inplace_vector_storage<T, Capacity, D>::inplace_vector_storage;

}; // struct maybe_has_trivial_copy_constructor<T, Capacity, D, true>

// maybe_has_trivial_move_constructor

template<class T, uint64_t Capacity, class D, bool = is_trivially_relocatable<T>::value,
         bool = std::is_trivially_move_constructible<T>::value>
struct maybe_has_trivial_move_constructor : maybe_has_trivial_copy_constructor<T, Capacity, D> {
    using maybe_has_trivial_copy_constructor<T, Capacity, D>::maybe_has_trivial_copy_constructor;

    maybe_has_trivial_move_constructor(maybe_has_trivial_move_constructor&& o) noexcept(
        std::is_nothrow_move_constructible<T>::value) {
        D& self   = static_cast<D&>(*this);
        D&& other = static_cast<D&&>(o);
        self.construct_at_end(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
    }

    maybe_has_trivial_move_constructor()                                          = default;
    maybe_has_trivial_move_constructor(const maybe_has_trivial_move_constructor&) = default;
    // clang-format off
    maybe_has_trivial_move_constructor& operator=(const maybe_has_trivial_move_constructor&) = default;
    maybe_has_trivial_move_constructor& operator=(maybe_has_trivial_move_constructor&&) = default;
    // clang-format on

}; // struct maybe_has_trivial_move_constructor

template<class T, uint64_t Capacity, class D, bool B>
struct maybe_has_trivial_move_constructor<T, Capacity, D, B, true>
    : maybe_has_trivial_copy_constructor<T, Capacity, D> {
    using maybe_has_trivial_copy_constructor<T, Capacity, D>::maybe_has_trivial_copy_constructor;

}; // struct maybe_has_trivial_move_constructor<T, Capacity, D, B, true>

template<class T, uint64_t Capacity, class D>
struct maybe_has_trivial_move_constructor<T, Capacity, D, true, false>
    : maybe_has_trivial_copy_constructor<T, Capacity, D> {
    using maybe_has_trivial_copy_constructor<T, Capacity, D>::maybe_has_trivial_copy_constructor;

    maybe_has_trivial_move_constructor(maybe_has_trivial_move_constructor&& o) noexcept {
        ciel::memcpy(this, std::addressof(o), sizeof(D));
        static_cast<D&&>(o).size_ = 0;
    }

    maybe_has_trivial_move_constructor()                                          = default;
    maybe_has_trivial_move_constructor(const maybe_has_trivial_move_constructor&) = default;
    // clang-format off
    maybe_has_trivial_move_constructor& operator=(const maybe_has_trivial_move_constructor&) = default;
    maybe_has_trivial_move_constructor& operator=(maybe_has_trivial_move_constructor&&) = default;
    // clang-format on

}; // struct maybe_has_trivial_move_constructor<T, Capacity, D, true, false>

// maybe_has_trivial_copy_assignment

template<class T, uint64_t Capacity, class D,
         bool = std::is_trivially_copy_constructible<T>::value && std::is_trivially_copy_assignable<T>::value
             && std::is_trivially_destructible<T>::value>
struct maybe_has_trivial_copy_assignment : maybe_has_trivial_move_constructor<T, Capacity, D> {
    using maybe_has_trivial_move_constructor<T, Capacity, D>::maybe_has_trivial_move_constructor;

    maybe_has_trivial_copy_assignment&
    operator=(const maybe_has_trivial_copy_assignment& o) noexcept(std::is_nothrow_copy_assignable<T>::value) {
        if CIEL_UNLIKELY (this == std::addressof(o)) {
            return *this;
        }

        D& self        = static_cast<D&>(*this);
        const D& other = static_cast<const D&>(o);
        self.assign_range(other);

        return *this;
    }

    maybe_has_trivial_copy_assignment()                                         = default;
    maybe_has_trivial_copy_assignment(const maybe_has_trivial_copy_assignment&) = default;
    maybe_has_trivial_copy_assignment(maybe_has_trivial_copy_assignment&&)      = default;
    // clang-format off
    maybe_has_trivial_copy_assignment& operator=(maybe_has_trivial_copy_assignment&&) = default;
    // clang-format on

}; // struct maybe_has_trivial_copy_assignment

template<class T, uint64_t Capacity, class D>
struct maybe_has_trivial_copy_assignment<T, Capacity, D, true> : maybe_has_trivial_move_constructor<T, Capacity, D> {
    using maybe_has_trivial_move_constructor<T, Capacity, D>::maybe_has_trivial_move_constructor;

}; // struct maybe_has_trivial_copy_assignment<T, Capacity, D, true>

// maybe_has_trivial_move_assignment

template<class T, uint64_t Capacity, class D, bool = is_trivially_relocatable<T>::value,
         bool = std::is_trivially_move_constructible<T>::value && std::is_trivially_move_assignable<T>::value
             && std::is_trivially_destructible<T>::value>
struct maybe_has_trivial_move_assignment : maybe_has_trivial_copy_assignment<T, Capacity, D> {
    using maybe_has_trivial_copy_assignment<T, Capacity, D>::maybe_has_trivial_copy_assignment;

    maybe_has_trivial_move_assignment&
    operator=(maybe_has_trivial_move_assignment&& o) noexcept(std::is_nothrow_move_assignable<T>::value) {
        if CIEL_UNLIKELY (this == std::addressof(o)) {
            return *this;
        }

        D& self   = static_cast<D&>(*this);
        D&& other = static_cast<D&&>(o);
        self.assign_range(std::move(other));

        return *this;
    }

    maybe_has_trivial_move_assignment()                                         = default;
    maybe_has_trivial_move_assignment(const maybe_has_trivial_move_assignment&) = default;
    maybe_has_trivial_move_assignment(maybe_has_trivial_move_assignment&&)      = default;
    // clang-format off
    maybe_has_trivial_move_assignment& operator=(const maybe_has_trivial_move_assignment&) = default;
    // clang-format on

}; // struct maybe_has_trivial_move_assignment

template<class T, uint64_t Capacity, class D, bool B>
struct maybe_has_trivial_move_assignment<T, Capacity, D, B, true> : maybe_has_trivial_copy_assignment<T, Capacity, D> {
    using maybe_has_trivial_copy_assignment<T, Capacity, D>::maybe_has_trivial_copy_assignment;

}; // struct maybe_has_trivial_move_assignment<T, Capacity, D, B, true>

template<class T, uint64_t Capacity, class D>
struct maybe_has_trivial_move_assignment<T, Capacity, D, true, false>
    : maybe_has_trivial_copy_assignment<T, Capacity, D> {
    using maybe_has_trivial_copy_assignment<T, Capacity, D>::maybe_has_trivial_copy_assignment;

    maybe_has_trivial_move_assignment&
    operator=(maybe_has_trivial_move_assignment&& o) noexcept {
        if CIEL_UNLIKELY (this == std::addressof(o)) {
            return *this;
        }

        ciel::memcpy(this, std::addressof(o), sizeof(D));
        static_cast<D&&>(o).size_ = 0;

        return *this;
    }

    maybe_has_trivial_move_assignment()                                         = default;
    maybe_has_trivial_move_assignment(const maybe_has_trivial_move_assignment&) = default;
    maybe_has_trivial_move_assignment(maybe_has_trivial_move_assignment&&)      = default;
    // clang-format off
    maybe_has_trivial_move_assignment& operator=(const maybe_has_trivial_move_assignment&) = default;
    // clang-format on

}; // struct maybe_has_trivial_move_assignment<T, Capacity, D, true, false>

// maybe_has_trivial_destructor

template<class T, uint64_t Capacity, class D, bool = std::is_trivially_destructible<T>::value>
struct maybe_has_trivial_destructor : maybe_has_trivial_move_assignment<T, Capacity, D> {
    using maybe_has_trivial_move_assignment<T, Capacity, D>::maybe_has_trivial_move_assignment;

    ~maybe_has_trivial_destructor() {
        D& self = static_cast<D&>(*this);
        self.clear();
    }

}; // struct maybe_has_trivial_destructor

template<class T, uint64_t Capacity, class D>
struct maybe_has_trivial_destructor<T, Capacity, D, true> : maybe_has_trivial_move_assignment<T, Capacity, D> {
    using maybe_has_trivial_move_assignment<T, Capacity, D>::maybe_has_trivial_move_assignment;

}; // struct maybe_has_trivial_destructor<T, Capacity, D, true>

} // namespace detail

template<class T, uint64_t Capacity>
class inplace_vector : detail::maybe_has_trivial_destructor<T, Capacity, inplace_vector<T, Capacity>> {
public:
    using detail::maybe_has_trivial_destructor<T, Capacity, inplace_vector<T, Capacity>>::maybe_has_trivial_destructor;

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
    CIEL_NODISCARD pointer
    begin_() noexcept {
        return this->data_;
    }

    CIEL_NODISCARD const_pointer
    begin_() const noexcept {
        return this->data_;
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
            ::new (end_()) value_type();
            ++this->size_;
        }
    }

    void
    construct_at_end(const size_type n, const value_type& value) {
        CIEL_PRECONDITION(size() + n <= capacity());

        for (size_type i = 0; i < n; ++i) {
            ::new (end_()) value_type(value);
            ++this->size_;
        }
    }

    template<class Iter, enable_if_t<is_forward_iterator<Iter>::value> = 0>
    void
    construct_at_end(Iter first, Iter last) {
        CIEL_PRECONDITION(size() + std::distance(first, last) <= capacity());

        using U = typename std::iterator_traits<Iter>::value_type;

        if (is_contiguous_iterator<Iter>::value && std::is_same<value_type, U>::value
            && std::is_trivially_copy_constructible<value_type>::value) {
            const size_t count = std::distance(first, last);
            if (count != 0) {
                ciel::memcpy(end(), ciel::to_address(first), count * sizeof(value_type));
                this->size_ += count;
            }

        } else {
            for (; first != last; ++first) {
                ::new (end_()) value_type(*first);
                ++this->size_;
            }
        }
    }

    void
    range_destroy(pointer begin, pointer end) noexcept {
        CIEL_PRECONDITION(begin <= end);
        CIEL_PRECONDITION(begin_() <= begin);
        CIEL_PRECONDITION(end <= end_());

        for (; begin != end; ++begin) {
            begin->~value_type();
            --this->size_;
        }
    }

    template<class... Args>
    void
    emplace_back_aux(Args&&... args) {
        if (size() == capacity()) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }

        unchecked_emplace_back(std::forward<Args>(args)...);
    }

    template<class... Args>
    void
    unchecked_emplace_back_aux(Args&&... args) {
        CIEL_PRECONDITION(size() < capacity());

        ::new (end_()) value_type(std::forward<Args>(args)...);
        ++this->size_;
    }

    template<class Iter>
    void
    assign(Iter first, Iter last, const size_type count) {
        if (capacity() < count) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }

        if (size() > count) {
            range_destroy(begin_() + count, end_());
        }

        CIEL_POSTCONDITION(size() <= count);

        Iter mid = ciel::copy_n(first, size(), begin_());

        // if mid < last
        construct_at_end(mid, last);
    }

public:
    inplace_vector() noexcept = default;

    inplace_vector(const size_type count, const value_type& value)
        : inplace_vector() {
        if CIEL_UNLIKELY (count > capacity()) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }

        construct_at_end(count, value);
    }

    explicit inplace_vector(const size_type count)
        : inplace_vector() {
        if CIEL_UNLIKELY (count > capacity()) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }

        construct_at_end(count);
    }

    template<class Iter, enable_if_t<is_exactly_input_iterator<Iter>::value> = 0>
    inplace_vector(Iter first, Iter last)
        : inplace_vector() {
        for (; first != last; ++first) {
            emplace_back(*first);
        }
    }

    template<class Iter, enable_if_t<is_forward_iterator<Iter>::value> = 0>
    inplace_vector(Iter first, Iter last)
        : inplace_vector() {
        const size_type count = std::distance(first, last);
        if CIEL_UNLIKELY (count > capacity()) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }

        construct_at_end(first, last);
    }

    inplace_vector(const inplace_vector& other) noexcept(std::is_nothrow_copy_constructible<T>::value) = default;
    inplace_vector(inplace_vector&& other) noexcept(std::is_nothrow_move_constructible<T>::value)      = default;

    template<class R, enable_if_t<is_range_without_size<R>::value && std::is_lvalue_reference<R>::value> = 0>
    inplace_vector(from_range_t, R&& rg)
        : inplace_vector(rg.begin(), rg.end()) {}

    template<class R, enable_if_t<is_range_without_size<R>::value && !std::is_lvalue_reference<R>::value> = 0>
    inplace_vector(from_range_t, R&& rg)
        : inplace_vector(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end())) {}

    template<class R, enable_if_t<is_range_with_size<R>::value && std::is_lvalue_reference<R>::value> = 0>
    inplace_vector(from_range_t, R&& rg)
        : inplace_vector() {
        const size_type count = rg.size();
        if CIEL_UNLIKELY (count > capacity()) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }

        construct_at_end(rg.begin(), rg.end());
    }

    template<class R, enable_if_t<is_range_with_size<R>::value && !std::is_lvalue_reference<R>::value> = 0>
    inplace_vector(from_range_t, R&& rg)
        : inplace_vector() {
        const size_type count = rg.size();
        if CIEL_UNLIKELY (count > capacity()) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
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
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }

        if (size() > count) {
            range_destroy(begin_() + count, end_());
        }

        CIEL_POSTCONDITION(size() <= count);

        std::fill_n(begin_(), size(), value);
        // if count > size()
        construct_at_end(count - size(), value);
    }

    template<class Iter, enable_if_t<is_forward_iterator<Iter>::value> = 0>
    void
    assign(Iter first, Iter last) {
        const size_type count = std::distance(first, last);

        assign(first, last, count);
    }

    template<class Iter, enable_if_t<is_exactly_input_iterator<Iter>::value> = 0>
    void
    assign(Iter first, Iter last) {
        clear();

        for (; first != last; ++first) {
            emplace_back(*first);
        }
    }

    void
    assign(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end());
    }

    template<class R, enable_if_t<is_range<R>::value> = 0>
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
            CIEL_THROW_EXCEPTION(std::out_of_range("pos is not within the range of ciel::inplace_vector"));
        }

        return begin_()[pos];
    }

    CIEL_NODISCARD const_reference
    at(const size_type pos) const {
        if CIEL_UNLIKELY (pos >= size()) {
            CIEL_THROW_EXCEPTION(std::out_of_range("pos is not within the range of ciel::inplace_vector"));
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
        return this->size_;
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
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
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
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
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
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }
    }

    static CIEL_CONSTEXPR_SINCE_CXX14 void
    shrink_to_fit() noexcept {}

    // TODO: insert, insert_range, emplace

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

    template<class U = inplace_vector, enable_if_t<is_trivially_relocatable<U>::value> = 0>
    void
    swap(inplace_vector& other) noexcept {
        ciel::relocatable_swap(*this, other);
    }

    template<class U = inplace_vector, enable_if_t<!is_trivially_relocatable<U>::value> = 0>
    void
    swap(inplace_vector& other) noexcept(std::is_nothrow_move_constructible<T>::value
                                         && std::is_nothrow_move_assignable<T>::value) {
        inplace_vector* smaller = this;
        inplace_vector* bigger  = std::addressof(other);
        auto smaller_size       = smaller->size_;
        auto bigger_size        = bigger->size_;

        if (smaller_size > bigger_size) {
            std::swap(smaller, bigger);
            std::swap(smaller_size, bigger_size);
        }

        std::swap_ranges(smaller->begin_(), smaller->begin_() + smaller_size, bigger->begin_());

        for (auto i = smaller_size; i != bigger_size; ++i) {
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

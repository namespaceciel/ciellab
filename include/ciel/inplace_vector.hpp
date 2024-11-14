#ifndef CIELLAB_INCLUDE_CIEL_INPLACE_VECTOR_HPP_
#define CIELLAB_INCLUDE_CIEL_INPLACE_VECTOR_HPP_

#include <ciel/compare.hpp>
#include <ciel/copy_n.hpp>
#include <ciel/core/config.hpp>
#include <ciel/core/cstring.hpp>
#include <ciel/core/is_range.hpp>
#include <ciel/core/is_trivially_relocatable.hpp>
#include <ciel/core/iterator_category.hpp>
#include <ciel/core/message.hpp>
#include <ciel/range_destroyer.hpp>
#include <ciel/swap.hpp>
#include <ciel/to_address.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

NAMESPACE_CIEL_BEGIN

template<class, size_t>
class inplace_vector;

namespace detail {

// By employing the Curiously Recurring Template Pattern (CRTP),
// we can conditionally provide trivial copy constructor based on
// whether the template type T has a trivial copy constructor.
// This approach allows the compiler to generate more efficient code,
// such as using memcpy for types that support trivial copying.
// Similarly, it enhances the handling of move constructors and destructors for other types.

// inplace_vector_size

template<class T, size_t Capacity>
struct inplace_vector_size {
    // clang-format off
    using inplace_vector_size_type =
        conditional_t<Capacity <= std::numeric_limits<uint8_t>::max(), uint8_t,
        conditional_t<Capacity <= std::numeric_limits<uint16_t>::max(), uint16_t,
        conditional_t<Capacity <= std::numeric_limits<uint32_t>::max(), uint32_t, size_t>>>;
    // clang-format on

    inplace_vector_size_type size_{0};

}; // struct inplace_vector_size

// inplace_vector_storage

template<class T, size_t Capacity, class D, bool = std::is_trivially_destructible<T>::value>
struct inplace_vector_storage : inplace_vector_size<T, Capacity> {
    union {
        T data_[Capacity];
        unsigned char null_state_;
    };

    inplace_vector_storage() noexcept
        : null_state_() {}

    ~inplace_vector_storage() {
        D& self = static_cast<D&>(*this);
        self.clear();
    }

}; // struct inplace_vector_storage

template<class T, size_t Capacity, class D>
struct inplace_vector_storage<T, Capacity, D, true> : inplace_vector_size<T, Capacity> {
    union {
        T data_[Capacity];
        unsigned char null_state_;
    };

    inplace_vector_storage() noexcept
        : null_state_() {}

}; // struct inplace_vector_storage<T, Capacity, D, true>

// maybe_has_trivial_copy_constructor

template<class T, size_t Capacity, class D, bool = std::is_trivially_copy_constructible<T>::value>
struct maybe_has_trivial_copy_constructor : inplace_vector_storage<T, Capacity, D> {
    using inplace_vector_storage<T, Capacity, D>::inplace_vector_storage;

    maybe_has_trivial_copy_constructor(const maybe_has_trivial_copy_constructor& o) noexcept(
        std::is_nothrow_copy_constructible<T>::value) {
        D& self        = static_cast<D&>(*this);
        const D& other = static_cast<const D&>(o);
        self.construct_at_end(other.begin(), other.end());
    }

    maybe_has_trivial_copy_constructor()                                                     = default;
    maybe_has_trivial_copy_constructor(maybe_has_trivial_copy_constructor&&)                 = default;
    maybe_has_trivial_copy_constructor& operator=(const maybe_has_trivial_copy_constructor&) = default;
    maybe_has_trivial_copy_constructor& operator=(maybe_has_trivial_copy_constructor&&)      = default;

}; // struct maybe_has_trivial_copy_constructor

template<class T, size_t Capacity, class D>
struct maybe_has_trivial_copy_constructor<T, Capacity, D, true> : inplace_vector_storage<T, Capacity, D> {
    using inplace_vector_storage<T, Capacity, D>::inplace_vector_storage;

}; // struct maybe_has_trivial_copy_constructor<T, Capacity, D, true>

// maybe_has_trivial_move_constructor

template<class T, size_t Capacity, class D, bool = is_trivially_relocatable<T>::value,
         bool = std::is_trivially_move_constructible<T>::value>
struct maybe_has_trivial_move_constructor : maybe_has_trivial_copy_constructor<T, Capacity, D> {
    using maybe_has_trivial_copy_constructor<T, Capacity, D>::maybe_has_trivial_copy_constructor;

    maybe_has_trivial_move_constructor(maybe_has_trivial_move_constructor&& o) noexcept(
        std::is_nothrow_move_constructible<T>::value) {
        D& self   = static_cast<D&>(*this);
        D&& other = static_cast<D&&>(o);
        self.construct_at_end(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
    }

    maybe_has_trivial_move_constructor()                                                     = default;
    maybe_has_trivial_move_constructor(const maybe_has_trivial_move_constructor&)            = default;
    maybe_has_trivial_move_constructor& operator=(const maybe_has_trivial_move_constructor&) = default;
    maybe_has_trivial_move_constructor& operator=(maybe_has_trivial_move_constructor&&)      = default;

}; // struct maybe_has_trivial_move_constructor

template<class T, size_t Capacity, class D, bool B>
struct maybe_has_trivial_move_constructor<T, Capacity, D, B, true>
    : maybe_has_trivial_copy_constructor<T, Capacity, D> {
    using maybe_has_trivial_copy_constructor<T, Capacity, D>::maybe_has_trivial_copy_constructor;

}; // struct maybe_has_trivial_move_constructor<T, Capacity, D, B, true>

template<class T, size_t Capacity, class D>
struct maybe_has_trivial_move_constructor<T, Capacity, D, true, false>
    : maybe_has_trivial_copy_constructor<T, Capacity, D> {
    using maybe_has_trivial_copy_constructor<T, Capacity, D>::maybe_has_trivial_copy_constructor;

    maybe_has_trivial_move_constructor(maybe_has_trivial_move_constructor&& o) noexcept {
        ciel::memcpy(this, std::addressof(o), sizeof(D));
        static_cast<D&&>(o).size_ = 0;
    }

    maybe_has_trivial_move_constructor()                                                     = default;
    maybe_has_trivial_move_constructor(const maybe_has_trivial_move_constructor&)            = default;
    maybe_has_trivial_move_constructor& operator=(const maybe_has_trivial_move_constructor&) = default;
    maybe_has_trivial_move_constructor& operator=(maybe_has_trivial_move_constructor&&)      = default;

}; // struct maybe_has_trivial_move_constructor<T, Capacity, D, true, false>

// maybe_has_trivial_copy_assignment

template<class T, size_t Capacity, class D,
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

    maybe_has_trivial_copy_assignment()                                               = default;
    maybe_has_trivial_copy_assignment(const maybe_has_trivial_copy_assignment&)       = default;
    maybe_has_trivial_copy_assignment(maybe_has_trivial_copy_assignment&&)            = default;
    maybe_has_trivial_copy_assignment& operator=(maybe_has_trivial_copy_assignment&&) = default;

}; // struct maybe_has_trivial_copy_assignment

template<class T, size_t Capacity, class D>
struct maybe_has_trivial_copy_assignment<T, Capacity, D, true> : maybe_has_trivial_move_constructor<T, Capacity, D> {
    using maybe_has_trivial_move_constructor<T, Capacity, D>::maybe_has_trivial_move_constructor;

}; // struct maybe_has_trivial_copy_assignment<T, Capacity, D, true>

// maybe_has_trivial_move_assignment

template<class T, size_t Capacity, class D, bool = is_trivially_relocatable<T>::value,
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

    maybe_has_trivial_move_assignment()                                                    = default;
    maybe_has_trivial_move_assignment(const maybe_has_trivial_move_assignment&)            = default;
    maybe_has_trivial_move_assignment(maybe_has_trivial_move_assignment&&)                 = default;
    maybe_has_trivial_move_assignment& operator=(const maybe_has_trivial_move_assignment&) = default;

}; // struct maybe_has_trivial_move_assignment

template<class T, size_t Capacity, class D, bool B>
struct maybe_has_trivial_move_assignment<T, Capacity, D, B, true> : maybe_has_trivial_copy_assignment<T, Capacity, D> {
    using maybe_has_trivial_copy_assignment<T, Capacity, D>::maybe_has_trivial_copy_assignment;

}; // struct maybe_has_trivial_move_assignment<T, Capacity, D, B, true>

template<class T, size_t Capacity, class D>
struct maybe_has_trivial_move_assignment<T, Capacity, D, true, false>
    : maybe_has_trivial_copy_assignment<T, Capacity, D> {
    using maybe_has_trivial_copy_assignment<T, Capacity, D>::maybe_has_trivial_copy_assignment;

    maybe_has_trivial_move_assignment& operator=(maybe_has_trivial_move_assignment&& o) noexcept {
        if CIEL_UNLIKELY (this == std::addressof(o)) {
            return *this;
        }

        ciel::memcpy(this, std::addressof(o), sizeof(D));
        static_cast<D&&>(o).size_ = 0;

        return *this;
    }

    maybe_has_trivial_move_assignment()                                                    = default;
    maybe_has_trivial_move_assignment(const maybe_has_trivial_move_assignment&)            = default;
    maybe_has_trivial_move_assignment(maybe_has_trivial_move_assignment&&)                 = default;
    maybe_has_trivial_move_assignment& operator=(const maybe_has_trivial_move_assignment&) = default;

}; // struct maybe_has_trivial_move_assignment<T, Capacity, D, true, false>

} // namespace detail

template<class T, size_t Capacity>
class inplace_vector : private detail::maybe_has_trivial_move_assignment<T, Capacity, inplace_vector<T, Capacity>> {
    using base_type = detail::maybe_has_trivial_move_assignment<T, Capacity, inplace_vector<T, Capacity>>;

    template<class, size_t>
    friend struct detail::inplace_vector_size;
    template<class, size_t, class, bool>
    friend struct detail::inplace_vector_storage;
    template<class, size_t, class, bool>
    friend struct detail::maybe_has_trivial_copy_constructor;
    template<class, size_t, class, bool, bool>
    friend struct detail::maybe_has_trivial_move_constructor;
    template<class, size_t, class, bool>
    friend struct detail::maybe_has_trivial_copy_assignment;
    template<class, size_t, class, bool, bool>
    friend struct detail::maybe_has_trivial_move_assignment;

public:
    using base_type::base_type;

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
    static constexpr bool should_pass_by_value =
        std::is_trivially_copyable<value_type>::value && sizeof(value_type) <= 16;
    using lvalue = conditional_t<should_pass_by_value, value_type, const value_type&>;
    using rvalue = conditional_t<should_pass_by_value, value_type, value_type&&>;

    CIEL_NODISCARD bool internal_value(const value_type& value, pointer begin) const noexcept {
        if (should_pass_by_value) {
            return false;
        }

        if CIEL_UNLIKELY (begin <= std::addressof(value) && std::addressof(value) < end_()) {
            return true;
        }

        return false;
    }

    CIEL_NODISCARD pointer begin_() noexcept {
        return this->data_;
    }

    CIEL_NODISCARD const_pointer begin_() const noexcept {
        return this->data_;
    }

    CIEL_NODISCARD pointer end_() noexcept {
        return begin_() + size();
    }

    CIEL_NODISCARD const_pointer end_() const noexcept {
        return begin_() + size();
    }

    CIEL_NODISCARD pointer end_cap_() noexcept {
        return begin_() + capacity();
    }

    CIEL_NODISCARD const_pointer end_cap_() const noexcept {
        return begin_() + capacity();
    }

    void construct_at_end(const size_type n) {
        CIEL_PRECONDITION(size() + n <= capacity());

        for (size_type i = 0; i < n; ++i) {
            ::new (end_()) value_type();
            ++this->size_;
        }
    }

    void construct_at_end(const size_type n, const value_type& value) {
        CIEL_PRECONDITION(size() + n <= capacity());

        for (size_type i = 0; i < n; ++i) {
            ::new (end_()) value_type(value);
            ++this->size_;
        }
    }

    template<class Iter, enable_if_t<is_forward_iterator<Iter>::value> = 0>
    void construct_at_end(Iter first, Iter last) {
        CIEL_PRECONDITION(size() + std::distance(first, last) <= capacity());

        using U = typename std::iterator_traits<Iter>::value_type;

        if (is_contiguous_iterator<Iter>::value && std::is_same<value_type, U>::value
            && std::is_trivially_copy_constructible<value_type>::value) {
            const size_type count = std::distance(first, last);
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

    void destroy(pointer p) noexcept {
        CIEL_PRECONDITION(begin_() <= p);
        CIEL_PRECONDITION(p < end_());

        p->~value_type();
        --this->size_;
    }

    void destroy(pointer begin, pointer end) noexcept {
        CIEL_PRECONDITION(begin <= end);
        CIEL_PRECONDITION(begin_() <= begin);
        CIEL_PRECONDITION(end <= end_());

        for (; begin != end; ++begin) {
            begin->~value_type();
            --this->size_;
        }
    }

    template<class... Args>
    void emplace_back_aux(Args&&... args) {
        if (size() == capacity()) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }

        unchecked_emplace_back(std::forward<Args>(args)...);
    }

    template<class... Args>
    void unchecked_emplace_back_aux(Args&&... args) {
        CIEL_PRECONDITION(size() < capacity());

        ::new (end_()) value_type(std::forward<Args>(args)...);
        ++this->size_;
    }

    template<class Iter>
    void assign(Iter first, Iter last, const size_type count) {
        if (capacity() < count) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }

        if (size() > count) {
            destroy(begin_() + count, end_());
        }

        CIEL_POSTCONDITION(size() <= count);

        Iter mid = ciel::copy_n(first, size(), begin_());

        // if mid < last
        construct_at_end(mid, last);
    }

public:
    inplace_vector() = default;

    inplace_vector(const size_type count, const value_type& value) {
        if CIEL_UNLIKELY (count > capacity()) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }

        construct_at_end(count, value);
    }

    explicit inplace_vector(const size_type count) {
        if CIEL_UNLIKELY (count > capacity()) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }

        construct_at_end(count);
    }

    template<class Iter, enable_if_t<is_exactly_input_iterator<Iter>::value> = 0>
    inplace_vector(Iter first, Iter last) {
        for (; first != last; ++first) {
            emplace_back(*first);
        }
    }

    template<class Iter, enable_if_t<is_forward_iterator<Iter>::value> = 0>
    inplace_vector(Iter first, Iter last) {
        const size_type count = std::distance(first, last);
        if CIEL_UNLIKELY (count > capacity()) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }

        construct_at_end(first, last);
    }

    inplace_vector(const inplace_vector& other) = default;
    inplace_vector(inplace_vector&& other)      = default;

    template<class R, enable_if_t<is_range_without_size<R>::value && std::is_lvalue_reference<R>::value> = 0>
    inplace_vector(from_range_t, R&& rg)
        : inplace_vector(rg.begin(), rg.end()) {}

    template<class R, enable_if_t<is_range_without_size<R>::value && !std::is_lvalue_reference<R>::value> = 0>
    inplace_vector(from_range_t, R&& rg)
        : inplace_vector(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end())) {}

    template<class R, enable_if_t<is_range_with_size<R>::value && std::is_lvalue_reference<R>::value> = 0>
    inplace_vector(from_range_t, R&& rg) {
        const size_type count = rg.size();
        if CIEL_UNLIKELY (count > capacity()) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }

        construct_at_end(rg.begin(), rg.end());
    }

    template<class R, enable_if_t<is_range_with_size<R>::value && !std::is_lvalue_reference<R>::value> = 0>
    inplace_vector(from_range_t, R&& rg) {
        const size_type count = rg.size();
        if CIEL_UNLIKELY (count > capacity()) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }

        construct_at_end(std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()));
    }

    inplace_vector(std::initializer_list<value_type> init)
        : inplace_vector(init.begin(), init.end()) {}

    inplace_vector& operator=(const inplace_vector& other) = default;
    inplace_vector& operator=(inplace_vector&& other)      = default;

    inplace_vector& operator=(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    void assign(const size_type count, const value_type& value) {
        if (capacity() < count) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }

        if (size() > count) {
            destroy(begin_() + count, end_());
        }

        CIEL_POSTCONDITION(size() <= count);

        std::fill_n(begin_(), size(), value);
        // if count > size()
        construct_at_end(count - size(), value);
    }

    template<class Iter, enable_if_t<is_forward_iterator<Iter>::value> = 0>
    void assign(Iter first, Iter last) {
        const size_type count = std::distance(first, last);

        assign(first, last, count);
    }

    template<class Iter, enable_if_t<is_exactly_input_iterator<Iter>::value> = 0>
    void assign(Iter first, Iter last) {
        clear();

        for (; first != last; ++first) {
            emplace_back(*first);
        }
    }

    void assign(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end(), ilist.size());
    }

    template<class R, enable_if_t<is_range<R>::value> = 0>
    void assign_range(R&& rg) {
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

    CIEL_NODISCARD reference at(const size_type pos) {
        if CIEL_UNLIKELY (pos >= size()) {
            CIEL_THROW_EXCEPTION(std::out_of_range("pos is not within the range of ciel::inplace_vector"));
        }

        return begin_()[pos];
    }

    CIEL_NODISCARD const_reference at(const size_type pos) const {
        if CIEL_UNLIKELY (pos >= size()) {
            CIEL_THROW_EXCEPTION(std::out_of_range("pos is not within the range of ciel::inplace_vector"));
        }

        return begin_()[pos];
    }

    CIEL_NODISCARD reference operator[](const size_type pos) {
        CIEL_PRECONDITION(pos < size());

        return begin_()[pos];
    }

    CIEL_NODISCARD const_reference operator[](const size_type pos) const {
        CIEL_PRECONDITION(pos < size());

        return begin_()[pos];
    }

    CIEL_NODISCARD reference front() {
        CIEL_PRECONDITION(!empty());

        return begin_()[0];
    }

    CIEL_NODISCARD const_reference front() const {
        CIEL_PRECONDITION(!empty());

        return begin_()[0];
    }

    CIEL_NODISCARD reference back() {
        CIEL_PRECONDITION(!empty());

        return *(end_() - 1);
    }

    CIEL_NODISCARD const_reference back() const {
        CIEL_PRECONDITION(!empty());

        return *(end_() - 1);
    }

    CIEL_NODISCARD T* data() noexcept {
        return begin_();
    }

    CIEL_NODISCARD const T* data() const noexcept {
        return begin_();
    }

    CIEL_NODISCARD iterator begin() noexcept {
        return iterator(begin_());
    }

    CIEL_NODISCARD const_iterator begin() const noexcept {
        return const_iterator(begin_());
    }

    CIEL_NODISCARD const_iterator cbegin() const noexcept {
        return begin();
    }

    CIEL_NODISCARD iterator end() noexcept {
        return iterator(end_());
    }

    CIEL_NODISCARD const_iterator end() const noexcept {
        return const_iterator(end_());
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
        return size() == 0;
    }

    CIEL_NODISCARD size_type size() const noexcept {
        return this->size_;
    }

    CIEL_NODISCARD static constexpr size_type max_size() noexcept {
        return Capacity;
    }

    CIEL_NODISCARD static constexpr size_type capacity() noexcept {
        return Capacity;
    }

    void resize(const size_type count) {
        if CIEL_UNLIKELY (count > capacity()) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }

        if (size() >= count) {
            destroy(begin_() + count, end_());
            return;
        }

        construct_at_end(count - size());
    }

    void resize(const size_type count, const value_type& value) {
        if CIEL_UNLIKELY (count > capacity()) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }

        if (size() >= count) {
            destroy(begin_() + count, end_());
            return;
        }

        construct_at_end(count - size(), value);
    }

    static void reserve(const size_type new_cap) {
        if (new_cap > capacity()) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});
        }
    }

    static void shrink_to_fit() noexcept {}

private:
    template<class AppendCallback, class InsertCallback, class IsInternalValueCallback>
    iterator insert_impl(pointer pos, const size_type count, AppendCallback&& append_callback,
                         InsertCallback&& insert_callback, IsInternalValueCallback&& is_internal_value_callback) {
        CIEL_PRECONDITION(begin_() <= pos);
        CIEL_PRECONDITION(pos <= end_());
        CIEL_PRECONDITION(count != 0);

        const size_type pos_index = pos - begin_();

        if (size() + count > capacity()) {
            CIEL_THROW_EXCEPTION(std::bad_alloc{});

        } else if (pos == end_()) { // equal to emplace_back
            append_callback();

        } else {
            const bool is_internal_value = is_internal_value_callback();

            range_destroyer<value_type> rd{end_() + count, end_() + count};
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
            const size_type pos_end_dis = end_() - pos;

            if (is_trivially_relocatable<value_type>::value) {
                ciel::memmove(pos + count, pos, sizeof(value_type) * pos_end_dis);
                this->size_ -= pos_end_dis;
                rd.advance_backward(pos_end_dis);

            } else {
                for (pointer p = end_() - 1; p >= pos; --p) {
                    ::new (p + count) value_type(std::move(*p));
                    destroy(p);
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

            this->size_ += pos_end_dis;
            rd.release();
        }

        return begin() + pos_index;
    }

public:
    template<class... Args>
    iterator emplace(const_iterator p, Args&&... args) {
        const pointer pos = begin_() + (p - begin());

        return insert_impl(
            pos, 1,
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
        const pointer pos = begin_() + (p - begin());

        return insert_impl(
            pos, 1,
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

    iterator insert(const_iterator p, lvalue value) {
        const pointer pos = begin_() + (p - begin());

        return insert_impl(
            pos, 1,
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
        const pointer pos = begin_() + (p - begin());

        return insert_impl(
            pos, 1,
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
        const pointer pos = begin_() + (p - begin());

        if CIEL_UNLIKELY (count == 0) {
            return iterator(pos);
        }

        return insert_impl(
            pos, count,
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
        const pointer pos = begin_() + (p - begin());

        if CIEL_UNLIKELY (count == 0) {
            return iterator(pos);
        }

        return insert_impl(
            pos, count,
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

    iterator insert(const_iterator pos, std::initializer_list<value_type> ilist) {
        return insert(pos, ilist.begin(), ilist.end(), ilist.size());
    }

    // Construct them all at the end at first, then rotate them to the right place.
    template<class Iter, enable_if_t<is_exactly_input_iterator<Iter>::value> = 0>
    iterator insert(const_iterator p, Iter first, Iter last) {
        const pointer pos = begin_() + (p - begin());

        const auto pos_index     = pos - begin();
        const size_type old_size = size();

        for (; first != last; ++first) {
            emplace_back(*first);
        }

        std::rotate(begin() + pos_index, begin() + old_size, end());
        return begin() + pos_index;
    }

    template<class R, enable_if_t<is_range<R>::value> = 0>
    iterator insert_range(const_iterator pos, R&& rg) {
        if (is_range_with_size<R>::value) {
            if (std::is_lvalue_reference<R>::value) {
                return insert(pos, rg.begin(), rg.end(), rg.size());
            }

            return insert(pos, std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()), rg.size());
        }

        if (std::is_lvalue_reference<R>::value) {
            return insert(pos, rg.begin(), rg.end());
        }

        return insert(pos, std::make_move_iterator(rg.begin()), std::make_move_iterator(rg.end()));
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

    void push_back(const value_type& value) {
        emplace_back(value);
    }

    void push_back(value_type&& value) {
        emplace_back(std::move(value));
    }

    pointer try_push_back(const value_type& value) {
        if (size() == capacity()) {
            return nullptr;
        }

        return std::addressof(unchecked_emplace_back(value));
    }

    pointer try_push_back(value_type&& value) {
        if (size() == capacity()) {
            return nullptr;
        }

        return std::addressof(unchecked_emplace_back(std::move(value)));
    }

    reference unchecked_push_back(const value_type& value) {
        return unchecked_emplace_back(value);
    }

    reference unchecked_push_back(value_type&& value) {
        return unchecked_emplace_back(std::move(value));
    }

    void pop_back() noexcept {
        CIEL_PRECONDITION(!empty());

        destroy(end_() - 1);
    }

    template<class R, enable_if_t<is_range<R>::value> = 0>
    void append_range(R&& rg) {
        insert_range(end(), std::forward<R>(rg));
    }

    template<class R, enable_if_t<is_range<R>::value> = 0, class Iter = decltype(std::declval<R>().begin())>
    Iter try_append_range(R&& rg) {
        using U = typename std::iterator_traits<Iter>::value_type;

        if (is_contiguous_iterator<Iter>::value && std::is_same<value_type, U>::value
            && std::is_trivially_copy_constructible<value_type>::value) {
            const size_type count = std::min<size_type>(ciel::distance(std::forward<R>(rg)), capacity() - size());
            if (count != 0) {
                ciel::memcpy(end(), ciel::to_address(rg.begin()), count * sizeof(value_type));
                this->size_ += count;
            }

            return std::next(rg.begin(), count);
        }

        Iter first      = rg.begin();
        const Iter last = rg.end();

        for (; first != last && size() != capacity(); ++first) {
            if (std::is_lvalue_reference<R>::value) {
                unchecked_emplace_back(*first);

            } else {
                unchecked_emplace_back(std::move(*first));
            }
        }

        return first;
    }

    void clear() noexcept {
        destroy(begin_(), end_());
    }

private:
    iterator erase_impl(pointer first, pointer last,
                        const difference_type count) noexcept(is_trivially_relocatable<value_type>::value
                                                              || std::is_nothrow_move_assignable<value_type>::value) {
        CIEL_PRECONDITION(last - first == count);
        CIEL_PRECONDITION(count != 0);

        const auto index      = first - begin_();
        const auto back_count = end_() - last;

        if (back_count == 0) {
            destroy(first, end_());

        } else if (is_trivially_relocatable<value_type>::value) {
            destroy(first, last);

            if (count >= back_count) {
                ciel::memcpy(first, last, sizeof(value_type) * back_count);

            } else {
                ciel::memmove(first, last, sizeof(value_type) * back_count);
            }

        } else {
            pointer new_end = std::move(last, end_(), first);
            destroy(new_end, end_());
        }

        return begin() + index;
    }

public:
    iterator erase(const_iterator p) {
        const pointer pos = begin_() + (p - begin());
        CIEL_PRECONDITION(begin_() <= pos);
        CIEL_PRECONDITION(pos < end_());

        return erase_impl(pos, pos + 1, 1);
    }

    iterator erase(const_iterator f, const_iterator l) {
        const pointer first = begin_() + (f - begin());
        const pointer last  = begin_() + (l - begin());
        CIEL_PRECONDITION(begin_() <= first);
        CIEL_PRECONDITION(last <= end_());

        const auto count = last - first;

        if CIEL_UNLIKELY (count <= 0) {
            return last;
        }

        return erase_impl(first, last, count);
    }

    template<class U = inplace_vector, enable_if_t<is_trivially_relocatable<U>::value> = 0>
    void swap(inplace_vector& other) noexcept {
        ciel::relocatable_swap(*this, other);
    }

    template<class U = inplace_vector, enable_if_t<!is_trivially_relocatable<U>::value> = 0>
    void swap(inplace_vector& other) noexcept(std::is_nothrow_move_constructible<T>::value
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

        bigger->destroy(bigger->begin_() + smaller_size, bigger->end_());
    }

}; // class inplace_vector

template<class T, size_t Capacity>
struct is_trivially_relocatable<inplace_vector<T, Capacity>> : is_trivially_relocatable<T> {};

template<class T, size_t N, class U = T>
typename inplace_vector<T, N>::size_type erase(inplace_vector<T, N>& c, const U& value) {
    auto it = std::remove(c.begin(), c.end(), value);
    auto r  = std::distance(it, c.end());
    c.erase(it, c.end());
    return r;
}

template<class T, size_t N, class Pred>
typename inplace_vector<T, N>::size_type erase_if(inplace_vector<T, N>& c, Pred pred) {
    auto it = std::remove_if(c.begin(), c.end(), pred);
    auto r  = std::distance(it, c.end());
    c.erase(it, c.end());
    return r;
}

NAMESPACE_CIEL_END

namespace std {

template<class T, size_t Capacity>
void swap(ciel::inplace_vector<T, Capacity>& lhs,
          ciel::inplace_vector<T, Capacity>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_INPLACE_VECTOR_HPP_

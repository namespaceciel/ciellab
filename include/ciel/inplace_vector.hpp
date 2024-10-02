#ifndef CIELLAB_INCLUDE_CIEL_INPLACE_VECTOR_HPP_
#define CIELLAB_INCLUDE_CIEL_INPLACE_VECTOR_HPP_

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include <ciel/config.hpp>
#include <ciel/move_proxy.hpp>
#include <ciel/range_destroyer.hpp>
#include <ciel/type_traits.hpp>

NAMESPACE_CIEL_BEGIN

template<class T, size_t Capacity, bool Valid = is_trivially_relocatable<T>::value>
class inplace_vector {
    static_assert(Valid, "T must be trivially relocatable, you can explicitly assume it.");
    static_assert(Capacity > 0, "");

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
    // It's not appropriate to optimize the type of the size_ variable, as it would damage its relocatable property.
    size_type size_{0};
    typename aligned_storage<sizeof(T), alignof(T)>::type buffer_[Capacity];

    pointer
    begin_() noexcept {
        return buffer_cast<pointer>(&buffer_);
    }

    const_pointer
    begin_() const noexcept {
        return buffer_cast<const_pointer>(&buffer_);
    }

    pointer
    end_() noexcept {
        return begin_() + size();
    }

    const_pointer
    end_() const noexcept {
        return begin_() + size();
    }

    pointer
    end_cap_() noexcept {
        return begin_() + capacity();
    }

    const_pointer
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
    alloc_range_destroy(pointer begin, pointer end) noexcept {
        CIEL_PRECONDITION(begin <= end);

        size_ -= std::distance(begin, end);
    }

    template<class U = value_type, typename std::enable_if<!std::is_trivially_destructible<U>::value, int>::type = 0>
    void
    alloc_range_destroy(pointer begin, pointer end) noexcept {
        CIEL_PRECONDITION(begin <= end);

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
            unchecked_emplace_back(std::forward<Args>(args)...);
        }

        return back();
    }

    template<class... Args>
    void
    unchecked_emplace_back_aux(Args&&... args) {
        CIEL_PRECONDITION(size() < capacity());

        ::new (end_()) value_type{std::forward<Args>(args)...};
        ++size_;
    }

public:
    inplace_vector() noexcept = default;

    inplace_vector(const size_type count, const value_type& value) {
        if CIEL_UNLIKELY (count > capacity()) {
            ciel::throw_exception(std::bad_alloc{});
        }

        construct_at_end(count, value);
    }

    explicit inplace_vector(const size_type count) {
        if CIEL_UNLIKELY (count > capacity()) {
            ciel::throw_exception(std::bad_alloc{});
        }

        construct_at_end(count);
    }

    template<class Iter, typename std::enable_if<is_exactly_input_iterator<Iter>::value, int>::type = 0>
    inplace_vector(Iter first, Iter last) {
        while (first != last) {
            emplace_back(*first);
            ++first;
        }
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    inplace_vector(Iter first, Iter last) {
        const size_type count = std::distance(first, last);
        if CIEL_UNLIKELY (count > capacity()) {
            ciel::throw_exception(std::bad_alloc{});
        }

        construct_at_end(first, last);
    }

    inplace_vector(const inplace_vector& other)
        : inplace_vector(other.begin(), other.end()) {}

    template<size_t OtherCapacity, typename std::enable_if<Capacity != OtherCapacity, int>::type = 0>
    inplace_vector(const inplace_vector<value_type, OtherCapacity>& other)
        : inplace_vector(other.begin(), other.end()) {}

    inplace_vector(inplace_vector&& other) noexcept {
        std::memcpy(this, &other, sizeof(other));
        other.size_ = 0;
    }

    // clang-format off
    template<size_t OtherCapacity, typename std::enable_if<OtherCapacity < Capacity, int>::type = 0>
    inplace_vector(inplace_vector<value_type, OtherCapacity>&& other) noexcept {
        std::memcpy(this, &other, sizeof(other));
        other.size_ = 0;
    }
    // clang-format on

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    inplace_vector(InitializerList init)
        : inplace_vector(init.begin(), init.end()) {}

    template<class U = value_type, typename std::enable_if<worth_move_constructing<U>::value, int>::type = 0>
    inplace_vector(std::initializer_list<move_proxy<value_type>> init)
        : inplace_vector(init.begin(), init.end()) {}

    template<class U = value_type, typename std::enable_if<!worth_move_constructing<U>::value, int>::type = 0>
    inplace_vector(std::initializer_list<value_type> init)
        : inplace_vector(init.begin(), init.end()) {}

    ~inplace_vector() {
        clear();
    }

    inplace_vector&
    operator=(const inplace_vector& other) {
        if CIEL_UNLIKELY (this == std::addressof(other)) {
            return *this;
        }

        assign(other.begin(), other.end());

        return *this;
    }

    template<size_t OtherCapacity, typename std::enable_if<Capacity != OtherCapacity, int>::type = 0>
    inplace_vector&
    operator=(const inplace_vector<value_type, OtherCapacity>& other) {
        assign(other.begin(), other.end());

        return *this;
    }

    inplace_vector&
    operator=(inplace_vector&& other) noexcept {
        if CIEL_UNLIKELY (this == std::addressof(other)) {
            return *this;
        }

        clear();
        std::memcpy(this, &other, sizeof(other));
        other.size_ = 0;

        return *this;
    }

    // clang-format off
    template<size_t OtherCapacity, typename std::enable_if<OtherCapacity < Capacity, int>::type = 0>
    inplace_vector& operator=(inplace_vector<value_type, OtherCapacity>&& other) noexcept {
        clear();
        std::memcpy(this, &other, sizeof(other));
        other.size_ = 0;

        return *this;
    }
    // clang-format on

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    inplace_vector&
    operator=(InitializerList ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    template<class U = value_type, typename std::enable_if<ciel::worth_move<U>::value, int>::type = 0>
    inplace_vector&
    operator=(std::initializer_list<move_proxy<value_type>> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    template<class U = value_type, typename std::enable_if<!ciel::worth_move<U>::value, int>::type = 0>
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
            alloc_range_destroy(begin_() + count, end_());
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

        if (capacity() < count) {
            ciel::throw_exception(std::bad_alloc{});
        }

        if (size() > count) {
            alloc_range_destroy(begin_() + count, end_());
        }

        CIEL_POSTCONDITION(size() <= count);

        Iter mid = std::next(first, size());

        std::copy(first, mid, begin_());
        // if mid < last
        construct_at_end(mid, last);

        CIEL_POSTCONDITION(size() == count);
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

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    void
    assign(InitializerList ilist) {
        assign(ilist.begin(), ilist.end());
    }

    template<class U = value_type, typename std::enable_if<ciel::worth_move<U>::value, int>::type = 0>
    void
    assign(std::initializer_list<move_proxy<value_type>> ilist) {
        assign(ilist.begin(), ilist.end());
    }

    template<class U = value_type, typename std::enable_if<!ciel::worth_move<U>::value, int>::type = 0>
    void
    assign(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end());
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

    CIEL_NODISCARD constexpr size_type
    max_size() const noexcept {
        return Capacity;
    }

    CIEL_NODISCARD constexpr size_type
    capacity() const noexcept {
        return Capacity;
    }

    void
    clear() noexcept {
        alloc_range_destroy(begin_(), end_());
    }

    void
    push_back(const value_type& value) {
        emplace_back(value);
    }

    void
    push_back(value_type&& value) {
        emplace_back(std::move(value));
    }

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

    void
    pop_back() noexcept {
        CIEL_PRECONDITION(!empty());

        alloc_range_destroy(end_() - 1, end_());
    }

    void
    resize(const size_type count) {
        if CIEL_UNLIKELY (count > capacity()) {
            ciel::throw_exception(std::bad_alloc{});
        }

        if (size() >= count) {
            alloc_range_destroy(begin_() + count, end_());
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
            alloc_range_destroy(begin_() + count, end_());
            return;
        }

        construct_at_end(count - size(), value);
    }

    void
    swap(inplace_vector& other) noexcept {
        ciel::relocatable_swap(*this, other);
    }

    template<class... Args>
    void
    unchecked_emplace_back(Args&&... args) {
        unchecked_emplace_back_aux(std::forward<Args>(args)...);
    }

    template<class U, class... Args>
    void
    unchecked_emplace_back(std::initializer_list<U> il, Args&&... args) {
        unchecked_emplace_back_aux(il, std::forward<Args>(args)...);
    }

}; // class inplace_vector

template<class T, size_t Capacity>
struct is_trivially_relocatable<inplace_vector<T, Capacity, true>> : std::true_type {};

NAMESPACE_CIEL_END

namespace std {

template<class T, size_t Capacity>
void
swap(ciel::inplace_vector<T, Capacity>& lhs, ciel::inplace_vector<T, Capacity>& rhs) noexcept {
    lhs.swap(rhs);
}

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_INPLACE_VECTOR_HPP_

#ifndef CIELLAB_INCLUDE_CIEL_VECTOR_HPP_
#define CIELLAB_INCLUDE_CIEL_VECTOR_HPP_

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include <ciel/config.hpp>
#include <ciel/move_proxy.hpp>
#include <ciel/split_buffer.hpp>
#include <ciel/type_traits.hpp>

NAMESPACE_CIEL_BEGIN

// Differences between std::vector and this class:
// 1. We don't provide specialization of vector for bool.
// 2. We don't do trivial destructions.
// 3. Inspired by Folly's FBVector, we have a is_trivially_relocatable trait,
//    which is defaultly equal to std::is_trivially_copyable, you can partially specialize it with certain classes.
//    We will memcpy trivially relocatable objects in expansions.
// 4. Elements shall be moved from std::initializer_list<move_proxy<T>>.

template<class T, class Allocator = std::allocator<T>>
class vector : private Allocator {
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

    owner<pointer> begin_;
    pointer end_;
    pointer end_cap_;

    allocator_type&
    allocator_() noexcept {
        return static_cast<allocator_type&>(*this);
    }

    const allocator_type&
    allocator_() const noexcept {
        return static_cast<const allocator_type&>(*this);
    }

    size_type
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

    template<class... Args>
    void
    construct_one_at_end(Args&&... args) {
        CIEL_PRECONDITION(end_ < end_cap_);

        alloc_traits::construct(allocator_(), end_, std::forward<Args>(args)...);
        ++end_;
    }

    void
    construct_at_end(const size_type n) {
        CIEL_PRECONDITION(end_ + n <= end_cap_);

        for (size_type i = 0; i < n; ++i) {
            alloc_traits::construct(allocator_(), end_);
            ++end_;
        }
    }

    void
    construct_at_end(const size_type n, const value_type& value) {
        CIEL_PRECONDITION(end_ + n <= end_cap_);

        for (size_type i = 0; i < n; ++i) {
            alloc_traits::construct(allocator_(), end_, value);
            ++end_;
        }
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    void
    construct_at_end(Iter first, Iter last) {
        CIEL_PRECONDITION(end_ + std::distance(first, last) <= end_cap_);

        while (first != last) {
            alloc_traits::construct(allocator_(), end_, *first);
            ++first;
            ++end_;
        }
    }

    // std::is_trivially_destructible<value_type> -> std::true_type
    pointer
    alloc_range_destroy(pointer begin, pointer end, std::true_type) noexcept {
        CIEL_PRECONDITION(begin <= end);

        return begin;
    }

    // std::is_trivially_destructible<value_type> -> std::false_type
    pointer
    alloc_range_destroy(pointer begin, pointer end, std::false_type) noexcept {
        CIEL_PRECONDITION(begin <= end);

        while (end != begin) {
            alloc_traits::destroy(allocator_(), --end);
        }

        return begin;
    }

    // is_trivially_relocatable -> std::true_type
    void
    swap_out_buffer(split_buffer<value_type, allocator_type>& sb, pointer pos, std::true_type) noexcept {
        // If either dest or src is an invalid or null pointer, the behavior is undefined, even if count is zero.
        if (begin_) {
            const size_type front_count = pos - begin_;
            const size_type back_count  = end_ - pos;

            CIEL_PRECONDITION(sb.front_spare() == front_count);
            CIEL_PRECONDITION(sb.back_spare() >= back_count);

            memcpy(sb.begin_cap_, begin_, sizeof(value_type) / sizeof(unsigned char) * front_count);
            // sb.begin_ = sb.begin_cap_;

            memcpy(sb.end_, pos, sizeof(value_type) / sizeof(unsigned char) * back_count);
            sb.end_ += back_count;

            alloc_traits::deallocate(allocator_(), begin_, capacity());
        }

        begin_   = sb.begin_cap_;
        end_     = sb.end_;
        end_cap_ = sb.end_cap_;

        sb.set_nullptr();
    }

    // is_trivially_relocatable -> std::false_type
    void
    swap_out_buffer(split_buffer<value_type, allocator_type>& sb, pointer pos,
                    std::false_type) noexcept(std::is_nothrow_move_constructible<value_type>::value) {
        if (begin_) {
            const size_type front_count = pos - begin_;
            const size_type back_count  = end_ - pos;

            CIEL_PRECONDITION(sb.front_spare() == front_count);
            CIEL_PRECONDITION(sb.back_spare() >= back_count);

            for (pointer p = pos - 1; p >= begin_; --p) {
                sb.emplace_front(std::move(*p));
            }

            for (pointer p = pos; p < end_; ++p) {
                sb.emplace_back(std::move(*p));
            }

            clear();
            alloc_traits::deallocate(allocator_(), begin_, capacity());
        }

        begin_   = sb.begin_;
        end_     = sb.end_;
        end_cap_ = sb.end_cap_;

        sb.set_nullptr();
    }

    // is_trivially_relocatable -> std::true_type
    void
    swap_out_buffer(split_buffer<value_type, allocator_type>& sb, std::true_type) noexcept {
        CIEL_PRECONDITION(sb.front_spare() == size());

        // If either dest or src is an invalid or null pointer, the behavior is undefined, even if count is zero.
        if (begin_) {
            memcpy(sb.begin_cap_, begin_, sizeof(value_type) / sizeof(unsigned char) * size());
            // sb.begin_ = sb.begin_cap_;

            alloc_traits::deallocate(allocator_(), begin_, capacity());
        }

        begin_   = sb.begin_cap_;
        end_     = sb.end_;
        end_cap_ = sb.end_cap_;

        sb.set_nullptr();
    }

    // is_trivially_relocatable -> std::false_type
    void
    swap_out_buffer(split_buffer<value_type, allocator_type>& sb,
                    std::false_type) noexcept(std::is_nothrow_move_constructible<value_type>::value) {
        CIEL_PRECONDITION(sb.front_spare() == size());

        if (begin_) {
            for (pointer p = end_ - 1; p >= begin_; --p) {
                sb.construct_one_at_begin(std::move(*p));
            }

            clear();
            alloc_traits::deallocate(allocator_(), begin_, capacity());
        }

        begin_   = sb.begin_cap_;
        end_     = sb.end_;
        end_cap_ = sb.end_cap_;

        sb.set_nullptr();
    }

    void
    swap_out_buffer(split_buffer<value_type, allocator_type>& sb) noexcept {
        do_destroy();

        CIEL_PRECONDITION(sb.begin_cap_ == sb.begin_);

        begin_   = sb.begin_cap_;
        end_     = sb.end_;
        end_cap_ = sb.end_cap_;

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

        {
            pointer p = from_s + n;

            for (; p < from_e; ++p) {
                construct_one_at_end(std::move(*p));
            }
        }

        std::move_backward(from_s, from_s + n, old_end);
    }

    void
    set_nullptr() noexcept {
        begin_   = nullptr;
        end_     = nullptr;
        end_cap_ = nullptr;
    }

public:
    vector() noexcept(noexcept(allocator_type()))
        : allocator_type(), begin_(nullptr), end_(nullptr), end_cap_(nullptr) {}

    explicit vector(const allocator_type& alloc) noexcept
        : allocator_type(alloc), begin_(nullptr), end_(nullptr), end_cap_(nullptr) {}

    vector(const size_type count, const value_type& value, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        if CIEL_LIKELY (count > 0) {
            begin_   = alloc_traits::allocate(allocator_(), count);
            end_cap_ = begin_ + count;
            end_     = begin_;

            CIEL_TRY {
                construct_at_end(count, value);
            }
            CIEL_CATCH (...) {
                clear();
                alloc_traits::deallocate(allocator_(), begin_, capacity());
                CIEL_THROW;
            }
        }
    }

    explicit vector(const size_type count, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        if CIEL_LIKELY (count > 0) {
            begin_   = alloc_traits::allocate(allocator_(), count);
            end_cap_ = begin_ + count;
            end_     = begin_;

            CIEL_TRY {
                construct_at_end(count);
            }
            CIEL_CATCH (...) {
                clear();
                alloc_traits::deallocate(allocator_(), begin_, capacity());
                CIEL_THROW;
            }
        }
    }

    template<class Iter, typename std::enable_if<is_exactly_input_iterator<Iter>::value, int>::type = 0>
    vector(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        CIEL_TRY {
            while (first != last) {
                emplace_back(*first);
                ++first;
            }
        }
        CIEL_CATCH (...) {
            do_destroy();
            CIEL_THROW;
        }
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    vector(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : vector(alloc) {
        const auto count = std::distance(first, last);

        if CIEL_LIKELY (count > 0) {
            begin_   = alloc_traits::allocate(allocator_(), count);
            end_cap_ = begin_ + count;
            end_     = begin_;

            CIEL_TRY {
                construct_at_end(first, last);
            }
            CIEL_CATCH (...) {
                clear();
                alloc_traits::deallocate(allocator_(), begin_, capacity());
                CIEL_THROW;
            }
        }
    }

    vector(const vector& other)
        : vector(other.begin(), other.end(),
                 alloc_traits::select_on_container_copy_construction(other.get_allocator())) {}

    vector(const vector& other, const allocator_type& alloc)
        : vector(other.begin(), other.end(), alloc) {}

    vector(vector&& other) noexcept
        : allocator_type(std::move(other.allocator_())),
          begin_(other.begin_),
          end_(other.end_),
          end_cap_(other.end_cap_) {
        other.set_nullptr();
    }

    vector(vector&& other, const allocator_type& alloc) {
        if (alloc == other.get_allocator()) {
            allocator_() = alloc;
            begin_       = other.begin_;
            end_         = other.end_;
            end_cap_     = other.end_cap_;

            other.set_nullptr();

        } else {
            vector(other, alloc).swap(*this);
        }
    }

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    vector(InitializerList init, const allocator_type& alloc = allocator_type())
        : vector(init.begin(), init.end(), alloc) {}

    template<class U = value_type, typename std::enable_if<worth_move_constructing<U>::value, int>::type = 0>
    vector(std::initializer_list<move_proxy<value_type>> init, const allocator_type& alloc = allocator_type())
        : vector(init.begin(), init.end(), alloc) {}

    template<class U = value_type, typename std::enable_if<!worth_move_constructing<U>::value, int>::type = 0>
    vector(std::initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
        : vector(init.begin(), init.end(), alloc) {}

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
                vector(other.allocator_()).swap(*this);
                assign(other.begin(), other.end());
                return *this;
            }

            allocator_() = other.allocator_();
        }

        assign(other.begin(), other.end());

        CIEL_POSTCONDITION(*this == other);
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

        begin_   = other.begin_;
        end_     = other.end_;
        end_cap_ = other.end_cap_;

        other.set_nullptr();

        return *this;
    }

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    vector&
    operator=(InitializerList ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    template<class U = value_type, typename std::enable_if<ciel::worth_move<U>::value, int>::type = 0>
    vector&
    operator=(std::initializer_list<move_proxy<value_type>> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    template<class U = value_type, typename std::enable_if<!ciel::worth_move<U>::value, int>::type = 0>
    vector&
    operator=(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    void
    assign(const size_type count, const value_type& value) {
        if (capacity() < count) {
            split_buffer<value_type, allocator_type> sb(allocator_());
            sb.reserve_cap_and_offset_to(count, 0);

            sb.construct_at_end(count, value);

            swap_out_buffer(sb);
            return;
        }

        if (size() > count) {
            end_ = alloc_range_destroy(begin_ + count, end_, std::is_trivially_destructible<value_type>{});
        }

        CIEL_POSTCONDITION(size() <= count);

        std::fill_n(begin_, size(), value);
        // if count > size()
        construct_at_end(count - size(), value);

        CIEL_POSTCONDITION(size() == count);
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    void
    assign(Iter first, Iter last) {
        const size_type count = std::distance(first, last);

        if (capacity() < count) {
            split_buffer<value_type, allocator_type> sb(allocator_());
            sb.reserve_cap_and_offset_to(count, 0);

            sb.construct_at_end(first, last);

            swap_out_buffer(sb);
            return;
        }

        if (size() > count) {
            end_ = alloc_range_destroy(begin_ + count, end_, std::is_trivially_destructible<value_type>{});
        }

        CIEL_POSTCONDITION(size() <= count);

        Iter mid = first + size();

        std::copy(first, mid, begin_);
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

        split_buffer<value_type, allocator_type> sb(allocator_());
        sb.reserve_cap_and_offset_to(new_cap, size());

        swap_out_buffer(sb, is_trivially_relocatable<value_type>{});
    }

    CIEL_NODISCARD size_type
    capacity() const noexcept {
        return end_cap_ - begin_;
    }

    void
    shrink_to_fit() {
        if CIEL_UNLIKELY (size() == capacity()) {
            return;
        }

        if (size() > 0) {
            split_buffer<value_type, allocator_type> sb(allocator_());
            sb.reserve_cap_and_offset_to(size(), size());

            swap_out_buffer(sb, is_trivially_relocatable<value_type>{});

        } else {
            alloc_traits::deallocate(allocator_(), begin_, capacity());
            set_nullptr();
        }
    }

    void
    clear() noexcept {
        end_ = alloc_range_destroy(begin_, end_, std::is_trivially_destructible<value_type>{});
    }

    iterator
    insert(iterator pos, const value_type& value) {
        CIEL_PRECONDITION(begin() <= pos);
        CIEL_PRECONDITION(pos <= end());

        const size_type pos_index = pos - begin();
        pointer pos_pointer       = begin_ + pos_index;

        if (end_ == end_cap_) { // expansion
            split_buffer<value_type, allocator_type> sb(allocator_());
            sb.reserve_cap_and_offset_to(recommend_cap(size() + 1), pos_index);

            sb.construct_one_at_end(value);

            swap_out_buffer(sb, pos_pointer, is_trivially_relocatable<value_type>{});

        } else if (pos_pointer == end_) { // equal to emplace_back
            construct_one_at_end(value);

        } else {
            move_range(pos_pointer, end_, pos_pointer + 1);
            *pos_pointer = value;
        }

        return begin() + pos_index;
    }

    iterator
    insert(iterator pos, value_type&& value) {
        CIEL_PRECONDITION(begin() <= pos);
        CIEL_PRECONDITION(pos <= end());

        const size_type pos_index = pos - begin();
        pointer pos_pointer       = begin_ + pos_index;

        if (end_ == end_cap_) { // expansion
            split_buffer<value_type, allocator_type> sb(allocator_());
            sb.reserve_cap_and_offset_to(recommend_cap(size() + 1), pos_index);

            sb.construct_one_at_end(std::move(value));

            swap_out_buffer(sb, pos_pointer, is_trivially_relocatable<value_type>{});

        } else if (pos_pointer == end_) { // equal to emplace_back
            construct_one_at_end(std::move(value));

        } else {
            move_range(pos_pointer, end_, pos_pointer + 1);
            *pos_pointer = std::move(value);
        }

        return begin() + pos_index;
    }

    iterator
    insert(iterator pos, size_type count, const value_type& value) {
        CIEL_PRECONDITION(begin() <= pos);
        CIEL_PRECONDITION(pos <= end());

        const size_type pos_index = pos - begin();
        pointer pos_pointer       = begin_ + pos_index;

        if (count + size() > capacity()) { // expansion
            split_buffer<value_type, allocator_type> sb(allocator_());
            sb.reserve_cap_and_offset_to(recommend_cap(size() + count), pos_index);

            sb.construct_at_end(count, value);

            swap_out_buffer(sb, pos_pointer, is_trivially_relocatable<value_type>{});

        } else { // enough back space
            const size_type old_count = count;
            pointer old_end           = end_;

            const size_type pos_end_distance = std::distance(pos, end());

            if (count > pos_end_distance) {
                const size_type n = count - pos_end_distance;
                construct_at_end(n, value);

                count -= n; // count == pos_end_distance
            }

            if (count > 0) {
                move_range(pos_pointer, old_end, pos_pointer + old_count);

                std::fill_n(pos_pointer, count, value);
            }
        }

        return begin() + pos_index;
    }

    // We construct all at the end at first, then rotate them to the right place
    template<class Iter, typename std::enable_if<is_exactly_input_iterator<Iter>::value, int>::type = 0>
    iterator
    insert(iterator pos, Iter first, Iter last) {
        // record these index because it may reallocate
        const auto pos_index     = pos - begin();
        const size_type old_size = size();

        while (first != last) {
            emplace_back(*first);
            ++first;
        }

        std::rotate(begin() + pos_index, begin() + old_size, end());
        return begin() + pos_index;
    }

    template<class Iter, typename std::enable_if<is_forward_iterator<Iter>::value, int>::type = 0>
    iterator
    insert(iterator pos, Iter first, Iter last) {
        CIEL_PRECONDITION(begin() <= pos);
        CIEL_PRECONDITION(pos <= end());

        difference_type count = std::distance(first, last);

        const size_type pos_index = pos - begin();

        if (count + size() > capacity()) { // expansion
            split_buffer<value_type, allocator_type> sb(allocator_());
            sb.reserve_cap_and_offset_to(recommend_cap(count + size()), pos_index);

            sb.construct_at_end(first, last);

            swap_out_buffer(sb, begin_ + pos_index, is_trivially_relocatable<value_type>{});

        } else { // enough back space
            const size_type old_count = count;
            pointer old_last          = end_;
            auto m                    = std::next(first, count);
            difference_type dx        = end_ - (begin_ + pos_index);

            if (count > dx) {
                m = first;
                std::advance(m, dx);
                construct_at_end(m, last);
                count = dx;
            }

            if (count > 0) {
                move_range(begin_ + pos_index, old_last, begin_ + pos_index + old_count);

                std::copy(first, m, begin_ + pos_index);
            }
        }

        return begin() + pos_index;
    }

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    iterator
    insert(iterator pos, InitializerList ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }

    template<class U = value_type, typename std::enable_if<ciel::worth_move<U>::value, int>::type = 0>
    iterator
    insert(iterator pos, std::initializer_list<move_proxy<value_type>> ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }

    template<class U = value_type, typename std::enable_if<!ciel::worth_move<U>::value, int>::type = 0>
    iterator
    insert(iterator pos, std::initializer_list<value_type> ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }

    template<class... Args>
    iterator
    emplace(iterator pos, Args&&... args) {
        CIEL_PRECONDITION(begin() <= pos);
        CIEL_PRECONDITION(pos <= end());

        const size_type pos_index = pos - begin();
        pointer pos_pointer       = begin_ + pos_index;

        if (end_ == end_cap_) { // expansion
            split_buffer<value_type, allocator_type> sb(allocator_());
            sb.reserve_cap_and_offset_to(recommend_cap(size() + 1), pos_index);

            sb.construct_one_at_end(std::forward<Args>(args)...);

            swap_out_buffer(sb, pos_pointer, is_trivially_relocatable<value_type>{});

        } else if (pos_pointer == end_) { // equal to emplace_back
            construct_one_at_end(std::forward<Args>(args)...);

        } else {
            move_range(pos_pointer, end_, pos_pointer + 1);
            *pos_pointer = value_type{std::forward<Args>(args)...};
        }

        return begin() + pos_index;
    }

    iterator
    erase(iterator pos) {
        CIEL_PRECONDITION(!empty());

        return erase(pos, pos + 1);
    }

    iterator
    erase(iterator first, iterator last) {
        const auto distance = std::distance(first, last);

        if CIEL_UNLIKELY (distance <= 0) {
            return last;
        }

        const auto index = first - begin();

        iterator new_end = std::move(last, end(), first);
        end_             = alloc_range_destroy(new_end, end_, std::is_trivially_destructible<value_type>{});

        return begin() + index;
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
        if (end_ == end_cap_) {
            split_buffer<value_type, allocator_type> sb(allocator_());
            sb.reserve_cap_and_offset_to(recommend_cap(size() + 1), size());

            sb.construct_one_at_end(std::forward<Args>(args)...);

            swap_out_buffer(sb, is_trivially_relocatable<value_type>{});

        } else {
            construct_one_at_end(std::forward<Args>(args)...);
        }

        return back();
    }

    void
    pop_back() noexcept {
        CIEL_PRECONDITION(!empty());

        end_ = alloc_range_destroy(end_ - 1, end_, std::is_trivially_destructible<value_type>{});
    }

    void
    resize(const size_type count) {
        if (size() >= count) {
            end_ = alloc_range_destroy(begin_ + count, end_, std::is_trivially_destructible<value_type>{});
            return;
        }

        reserve(count);

        construct_at_end(count - size());
    }

    void
    resize(const size_type count, const value_type& value) {
        if (size() >= count) {
            end_ = alloc_range_destroy(begin_ + count, end_, std::is_trivially_destructible<value_type>{});
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
        swap(end_cap_, other.end_cap_);
        swap(allocator_(), other.allocator_());
    }

}; // class vector

template<class T, class Alloc>
CIEL_NODISCARD bool
operator==(const vector<T, Alloc>& lhs, const vector<T, Alloc>& rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

// So that we can test more efficiently
template<class T, class Alloc>
CIEL_NODISCARD bool
operator==(const vector<T, Alloc>& lhs, std::initializer_list<T> rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

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

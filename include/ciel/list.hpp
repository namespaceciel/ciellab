#ifndef CIELLAB_INCLUDE_CIEL_LIST_HPP_
#define CIELLAB_INCLUDE_CIEL_LIST_HPP_

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include <ciel/compressed_pair.hpp>
#include <ciel/config.hpp>
#include <ciel/type_traits.hpp>

NAMESPACE_CIEL_BEGIN

// Differences between std::list and this class:
// 1. It keeps hold of allocations to avoid repeated heap allocations and frees
//    when frequently inserting and removing elements.

struct list_node_base {
    list_node_base* prev_;
    list_node_base* next_;

    list_node_base() noexcept : prev_(this), next_(this) {}

    list_node_base(list_node_base* p, list_node_base* n) noexcept : prev_(p), next_(n) {}

    auto clear() noexcept -> void {
        prev_ = this;
        next_ = this;
    }

};    // struct list_node_base

template<class T>
struct list_node : list_node_base {
    T value_;

    template<class... Args>
    list_node(list_node_base* p, list_node_base* n, Args&& ... args)
        : list_node_base(p, n), value_(std::forward<Args>(args)...) {}

};    // struct list_node

template<class T, class Pointer, class Reference>
class list_iterator {
public:
    using difference_type   = ptrdiff_t;
    using value_type        = T;
    using pointer           = Pointer;
    using reference         = Reference;
    using iterator_category = std::bidirectional_iterator_tag;
    using iterator_concept  = std::bidirectional_iterator_tag;

private:
    using base_node_type    = list_node_base;
    using node_type         = list_node<value_type>;

    base_node_type* it_;

public:
    list_iterator() noexcept: it_(nullptr) {}

    explicit list_iterator(const base_node_type* p) noexcept: it_(const_cast<base_node_type*>(p)) {}

    list_iterator(const list_iterator&) noexcept = default;
    list_iterator(list_iterator&&) noexcept = default;

    template<class P, class R>
    list_iterator(const list_iterator<T, P, R>& other) noexcept : it_(const_cast<base_node_type*>(other.base())) {}

    ~list_iterator() = default;

    auto operator=(const list_iterator&) noexcept -> list_iterator& = default;
    auto operator=(list_iterator&&) noexcept -> list_iterator& = default;

    CIEL_NODISCARD auto next() const noexcept -> list_iterator {
        return list_iterator(it_->next_);
    }

    CIEL_NODISCARD auto prev() const noexcept -> list_iterator {
        return list_iterator(it_->prev_);
    }

    CIEL_NODISCARD auto operator*() const noexcept -> reference {
        return static_cast<node_type*>(it_)->value_;
    }

    CIEL_NODISCARD auto operator->() const noexcept -> pointer {
        return &static_cast<node_type*>(it_)->value_;
    }

    auto operator++() noexcept -> list_iterator& {
        it_ = it_->next_;
        return *this;
    }

    CIEL_NODISCARD auto operator++(int) noexcept -> list_iterator {
        list_iterator res(it_);
        ++(*this);
        return res;
    }

    auto operator--() noexcept -> list_iterator& {
        it_ = it_->prev_;
        return *this;
    }

    CIEL_NODISCARD auto operator--(int) noexcept -> list_iterator {
        list_iterator res(it_);
        --(*this);
        return res;
    }

    CIEL_NODISCARD auto base() const noexcept -> base_node_type* {
        return it_;
    }

    CIEL_NODISCARD explicit operator bool() const noexcept {
        return it_ != nullptr;
    }

};    // class list_iterator

template<class T, class Pointer1, class Pointer2, class Reference1, class Reference2>
CIEL_NODISCARD auto operator==(const list_iterator<T, Pointer1, Reference1>& lhs,
                              const list_iterator<T, Pointer2, Reference2>& rhs) noexcept -> bool {
    return lhs.base() == rhs.base();
}

template<class T, class Pointer1, class Pointer2, class Reference1, class Reference2>
CIEL_NODISCARD auto operator!=(const list_iterator<T, Pointer1, Reference1>& lhs,
                              const list_iterator<T, Pointer2, Reference2>& rhs) noexcept -> bool {
    return !(lhs == rhs);
}

template<class T, class Allocator = std::allocator<T>>
class list {
public:
    using value_type             = T;
    using allocator_type         = Allocator;
    using size_type              = size_t;
    using difference_type        = ptrdiff_t;
    using reference              = value_type&;
    using const_reference        = const value_type&;

    using pointer                = typename std::allocator_traits<allocator_type>::pointer;
    using const_pointer          = typename std::allocator_traits<allocator_type>::const_pointer;

    using iterator               = list_iterator<value_type, pointer, reference>;
    using const_iterator         = list_iterator<value_type, const_pointer, const_reference>;

    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;;

private:
    using base_node_type         = list_node_base;
    using node_type              = list_node<value_type>;

    using alloc_traits           = std::allocator_traits<allocator_type>;
    using node_allocator         = typename alloc_traits::template rebind_alloc<node_type>;
    using node_alloc_traits      = typename alloc_traits::template rebind_traits<node_type>;

    base_node_type end_node_;
    node_type* free_node_;
    compressed_pair<size_type, node_allocator> size_node_allocator_;

    auto do_destroy() noexcept -> void {
        iterator it = begin();
        iterator e = end();

        while (it != e) {
            auto* to_be_destroyed = static_cast<node_type*>(it.base());
            ++it;
            node_alloc_traits::destroy(allocator_(), to_be_destroyed);
            node_alloc_traits::deallocate(allocator_(), to_be_destroyed, 1);
        }

        while (free_node_) {
            auto next = static_cast<node_type*>(free_node_->next_);
            node_alloc_traits::deallocate(allocator_(), free_node_, 1);
            free_node_ = next;
        }
    }

    auto get_one_free_node() -> node_type* {
        if (free_node_) {
            node_type* res = free_node_;
            free_node_ = static_cast<node_type*>(free_node_->next_);

            return res;
        }

        return node_alloc_traits::allocate(allocator_(), 1);
    }

    auto store_one_free_node(node_type* free) noexcept -> void {
        free->next_ = free_node_;
        free_node_ = free;
    }

    auto size_() noexcept -> size_type& {
        return size_node_allocator_.first();
    }

    auto size_() const noexcept -> const size_type& {
        return size_node_allocator_.first();
    }

    auto allocator_() noexcept -> node_allocator& {
        return size_node_allocator_.second();
    }

    auto allocator_() const noexcept -> const node_allocator& {
        return size_node_allocator_.second();
    }

    auto alloc_range_destroy(iterator begin, iterator end) noexcept -> iterator {
        iterator loop = begin;
        iterator before_begin = begin.prev();

        while (loop != end) {
            auto* to_be_destroyed = static_cast<node_type*>(loop.base());
            ++loop;

            node_alloc_traits::destroy(allocator_(), to_be_destroyed);
            --size_();
            store_one_free_node(to_be_destroyed);
        }
        
        before_begin.base()->next_ = end.base();
        end.base()->prev_ = before_begin.base();
        
        return end;
    }

    // insert before begin
    template<class... Arg>
    auto alloc_range_construct_n(iterator begin, const size_type n, Arg&& ... arg) -> iterator {
        iterator before_begin = begin.prev();
        iterator original_before_begin = before_begin;

        CIEL_TRY {
            for (size_type i = 0; i < n; ++i) {
                node_type* construct_place = get_one_free_node();
                
                CIEL_TRY {
                    node_alloc_traits::construct(allocator_(), construct_place, before_begin.base(), begin.base(),
                                                 std::forward<Arg>(arg)...);
                    ++size_();

                    before_begin.base()->next_ = construct_place;
                    begin.base()->prev_ = construct_place;
                    ++before_begin;

                } CIEL_CATCH (...) {
                    store_one_free_node(construct_place);
                    CIEL_THROW;
                }
            }
            return original_before_begin.next();

        } CIEL_CATCH (...) {
            alloc_range_destroy(original_before_begin.next(), begin);
            CIEL_THROW;
        }
    }

    template<class Iter, typename std::enable_if<is_input_iterator<Iter>::value, int>::type = 0>
    auto alloc_range_construct(iterator begin, Iter first, Iter last) -> iterator {
        iterator before_begin = begin.prev();
        iterator original_before_begin = before_begin;

        CIEL_TRY {
            while (first != last) {
                node_type* construct_place = get_one_free_node();

                CIEL_TRY {
                    node_alloc_traits::construct(allocator_(), construct_place, before_begin.base(), begin.base(),
                                                 *first);
                    ++size_();
                    ++first;

                    before_begin.base()->next_ = construct_place;
                    begin.base()->prev_ = construct_place;
                    ++before_begin;

                } CIEL_CATCH (...) {
                    store_one_free_node(construct_place);
                    CIEL_THROW;
                }
            }
            return original_before_begin.next();

        } CIEL_CATCH (...) {
            alloc_range_destroy(original_before_begin.next(), begin);
            CIEL_THROW;
        }
    }

public:
    list()
        : free_node_(nullptr), size_node_allocator_(0, default_init_tag{}) {}

    explicit list(const allocator_type& alloc)
        : free_node_(nullptr), size_node_allocator_(0, alloc) {}

    list(const size_type count, const T& value, const allocator_type& alloc = allocator_type())
        : list(alloc) {
        
        CIEL_TRY {
            alloc_range_construct_n(end(), count, value);
            
        } CIEL_CATCH (...) {
            do_destroy();
            CIEL_THROW;
        }
    }

    explicit list(const size_type count, const allocator_type& alloc = allocator_type())
        : list(alloc) {

        CIEL_TRY {
            alloc_range_construct_n(end(), count);

        } CIEL_CATCH (...) {
            do_destroy();
            CIEL_THROW;
        }
    }

    template<class Iter, typename std::enable_if<is_input_iterator<Iter>::value, int>::type = 0>
    list(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : list(alloc) {

        CIEL_TRY {
            alloc_range_construct(end(), first, last);

        } CIEL_CATCH (...) {
            do_destroy();
            CIEL_THROW;
        }
    }

    list(const list& other)
        : list(other.begin(), other.end(),
               alloc_traits::select_on_container_copy_construction(other.get_allocator())) {}

    list(const list& other, const allocator_type& alloc)
        : list(other.begin(), other.end(), alloc) {}

    list(list&& other) noexcept
        : end_node_(other.end_node_),
          free_node_(other.free_node_),
          size_node_allocator_(other.size_(), std::move(other.allocator_())) {
        
        end_node_.next_->prev_ = &end_node_;
        end_node_.prev_->next_ = &end_node_;
        
        other.end_node_.clear();
        other.free_node_ = nullptr;
        other.size_() = 0;
    }

    list(list&& other, const allocator_type& alloc) {
        if (alloc == other.get_allocator()) {
            end_node_ = other.end_node_;
            free_node_ = other.free_node_;
            size_() = other.size_();
            allocator_() = alloc;

            end_node_.next_->prev_ = &end_node_;
            end_node_.prev_->next_ = &end_node_;
            
            other.end_node_.clear();
            other.free_node_ = nullptr;
            other.size_() = 0;

        } else {
            list(other, alloc).swap(*this);
        }
    }

    list(std::initializer_list<T> init, const allocator_type& alloc = allocator_type())
        : list(init.begin(), init.end(), alloc) {}

    ~list() {
        do_destroy();
    }

    auto operator=(const list& other) -> list& {
        if CIEL_UNLIKELY (this == addressof(other)) {
            return *this;
        }

        if (alloc_traits::propagate_on_container_copy_assignment::value) {
            if (allocator_() != other.allocator_()) {
                list(other.allocator_()).swap(*this);
                assign(other.begin(), other.end());
                return *this;
            }

            allocator_() = other.allocator_();
        }

        assign(other.begin(), other.end());

        CIEL_POSTCONDITION(*this == other);
        return *this;
    }

    auto operator=(list&& other) noexcept(alloc_traits::is_always_equal::value) -> list& {
        if CIEL_UNLIKELY (this == addressof(other)) {
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
        
        end_node_ = other.end_node_;
        free_node_ = other.free_node_;
        size_() = other.size_();

        end_node_.next_->prev_ = &end_node_;
        end_node_.prev_->next_ = &end_node_;
        
        other.end_node_.clear();
        other.free_node_ = nullptr;
        other.size_() = 0;
        
        return *this;
    }

    auto operator=(std::initializer_list<T> ilist) -> list& {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    auto assign(size_type count, const T& value) -> void {
        iterator it = begin();
        iterator e = end();

        for (; count > 0 && it != e; --count, ++it) {
            *it = value;
        }

        if (it == e) {
            insert(e, count, value);

        } else {
            erase(it, e);
        }
    }

    template<class Iter, typename std::enable_if<is_input_iterator<Iter>::value, int>::type = 0>
    auto assign(Iter first, Iter last) -> void {
        iterator it = begin();
        iterator e = end();

        for (; first != last && it != e; ++first, ++it) {
            *it = *first;
        }

        if (it == e) {
            insert(e, std::move(first), std::move(last));

        } else {
            erase(it, e);
        }
    }

    auto assign(std::initializer_list<T> ilist) -> void {
        assign(ilist.begin(), ilist.end());
    }

    CIEL_NODISCARD auto get_allocator() const noexcept -> allocator_type {
        return allocator_();
    }

    CIEL_NODISCARD auto front() -> reference {
        CIEL_PRECONDITION(!empty());

        return *begin();
    }

    CIEL_NODISCARD auto front() const -> const_reference {
        CIEL_PRECONDITION(!empty());

        return *begin();
    }

    CIEL_NODISCARD auto back() -> reference {
        CIEL_PRECONDITION(!empty());

        return *(--end());
    }

    CIEL_NODISCARD auto back() const -> const_reference {
        CIEL_PRECONDITION(!empty());

        return *(--end());
    }

    CIEL_NODISCARD auto begin() noexcept -> iterator {
        return iterator(end_node_.next_);
    }

    CIEL_NODISCARD auto begin() const noexcept -> const_iterator {
        return const_iterator(end_node_.next_);
    }

    CIEL_NODISCARD auto cbegin() const noexcept -> const_iterator {
        return begin();
    }

    CIEL_NODISCARD auto end() noexcept -> iterator {
        return iterator(&end_node_);
    }

    CIEL_NODISCARD auto end() const noexcept -> const_iterator {
        return const_iterator(&end_node_);
    }

    CIEL_NODISCARD auto cend() const noexcept -> const_iterator {
        return end();
    }

    CIEL_NODISCARD auto rbegin() noexcept -> reverse_iterator {
        return reverse_iterator(end());
    }

    CIEL_NODISCARD auto rbegin() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(end());
    }

    CIEL_NODISCARD auto crbegin() const noexcept -> const_reverse_iterator {
        return rbegin();
    }

    CIEL_NODISCARD auto rend() noexcept -> reverse_iterator {
        return reverse_iterator(begin());
    }

    CIEL_NODISCARD auto rend() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(begin());
    }

    CIEL_NODISCARD auto crend() const noexcept -> const_reverse_iterator {
        return rend();
    }

    CIEL_NODISCARD auto empty() const noexcept -> bool {
        return size_() == 0;
    }

    CIEL_NODISCARD auto size() const noexcept -> size_type {
        return size_();
    }

    CIEL_NODISCARD auto max_size() const noexcept -> size_type {
        return node_alloc_traits::max_size(allocator_());
    }

    auto clear() noexcept -> void {
        alloc_range_destroy(begin(), end());
    }

    auto insert(iterator pos, const T& value) -> iterator {
        return alloc_range_construct_n(pos, 1, value);
    }

    auto insert(iterator pos, T&& value) -> iterator {
        return alloc_range_construct_n(pos, 1, std::move(value));
    }

    auto insert(iterator pos, const size_type count, const T& value) -> iterator {
        return alloc_range_construct_n(pos, count, value);
    }

    template<class Iter, typename std::enable_if<is_input_iterator<Iter>::value, int>::type = 0>
    auto insert(iterator pos, Iter first, Iter last) -> iterator {
        return alloc_range_construct(pos, first, last);
    }

    auto insert(iterator pos, std::initializer_list<T> ilist) -> iterator {
        return alloc_range_construct(pos, ilist.begin(), ilist.end());
    }

    template<class... Args>
    auto emplace(iterator pos, Args&& ... args) -> iterator {
        return alloc_range_construct_n(pos, 1, std::forward<Args>(args)...);
    }

    auto erase(iterator pos) -> iterator {
        return alloc_range_destroy(pos, pos.next());
    }

    auto erase(iterator first, iterator last) -> iterator {
        return alloc_range_destroy(first, last);
    }

    auto push_back(const T& value) -> void {
        emplace_back(value);
    }

    auto push_back(T&& value) -> void {
        emplace_back(std::move(value));
    }

    template<class... Args>
    auto emplace_back(Args&& ... args) -> reference {
        return *alloc_range_construct_n(end(), 1, std::forward<Args>(args)...);
    }

    auto pop_back() noexcept -> void {
        CIEL_PRECONDITION(!empty());

        alloc_range_destroy(end().prev(), end());
    }

    auto push_front(const T& value) -> void {
        emplace_front(value);
    }

    auto push_front(T&& value) -> void {
        emplace_front(std::move(value));
    }

    template<class... Args>
    auto emplace_front(Args&& ... args) -> reference {
        return *alloc_range_construct_n(begin(), 1, std::forward<Args>(args)...);
    }

    auto pop_front() noexcept -> void {
        CIEL_PRECONDITION(!empty());

        alloc_range_destroy(begin(), begin().next());
    }

    auto resize(const size_type count) -> void {
        if (size() >= count) {
            iterator tmp = std::prev(end(), size() - count);
            alloc_range_destroy(tmp, end());

        } else {
            alloc_range_construct_n(end(), count - size());
        }
    }

    auto resize(const size_type count, const value_type& value) -> void {
        if (size() >= count) {
            iterator tmp = std::prev(end(), size() - count);
            alloc_range_destroy(tmp, end());

        } else {
            alloc_range_construct_n(end(), count - size(), value);
        }
    }

    auto swap(list& other) noexcept(alloc_traits::is_always_equal::value) -> void {
        using std::swap;

        swap(end_node_, other.end_node_);

        end_node_.next_->prev_ = &end_node_;
        end_node_.prev_->next_ = &end_node_;

        other.end_node_.next_->prev_ = &other.end_node_;
        other.end_node_.prev_->next_ = &other.end_node_;

        swap(free_node_, other.free_node_);
        swap(size_node_allocator_, other.size_node_allocator_);
    }

};    // class list

template<class T, class Alloc>
CIEL_NODISCARD auto operator==(const list<T, Alloc>& lhs, const list<T, Alloc>& rhs) noexcept -> bool {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

// So that we can test more efficiently
template<class T, class Alloc>
CIEL_NODISCARD auto operator==(const list<T, Alloc>& lhs, std::initializer_list<T> rhs) noexcept -> bool {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

#if CIEL_STD_VER >= 17

template<class Iter, class Alloc = std::allocator<typename std::iterator_traits<Iter>::value_type>>
list(Iter, Iter, Alloc = Alloc()) -> list<typename std::iterator_traits<Iter>::value_type, Alloc>;

#endif

NAMESPACE_CIEL_END

namespace std {

template<class T, class Alloc>
auto swap(ciel::list<T, Alloc>& lhs, ciel::list<T, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) -> void {
    lhs.swap(rhs);
}

}   // namespace std

#endif // CIELLAB_INCLUDE_CIEL_LIST_HPP_
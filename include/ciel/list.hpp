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
#include <ciel/move_proxy.hpp>
#include <ciel/type_traits.hpp>

NAMESPACE_CIEL_BEGIN

// Differences between std::list and this class:
// 1. It keeps hold of allocations to avoid repeated heap allocations and frees
//    when frequently inserting and removing elements.

struct list_node_base {
    list_node_base* prev_;
    list_node_base* next_;

    list_node_base() noexcept
        : prev_(this), next_(this) {}

    list_node_base(list_node_base* p, list_node_base* n) noexcept
        : prev_(p), next_(n) {}

    void
    clear() noexcept {
        prev_ = this;
        next_ = this;
    }

}; // struct list_node_base

template<class T>
struct list_node : list_node_base {
    T value_;

    template<class... Args>
    list_node(list_node_base* p, list_node_base* n, Args&&... args)
        : list_node_base(p, n), value_(std::forward<Args>(args)...) {}

}; // struct list_node

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
    using base_node_type = list_node_base;
    using node_type      = list_node<value_type>;

    base_node_type* it_;

public:
    list_iterator() noexcept
        : it_(nullptr) {}

    explicit list_iterator(const base_node_type* p) noexcept
        : it_(const_cast<base_node_type*>(p)) {}

    list_iterator(const list_iterator&) noexcept = default;
    list_iterator(list_iterator&&) noexcept      = default;

    template<class P, class R>
    list_iterator(const list_iterator<T, P, R>& other) noexcept
        : it_(const_cast<base_node_type*>(other.base())) {}

    ~list_iterator() = default;

    list_iterator&
    operator=(const list_iterator&) noexcept
        = default;
    list_iterator&
    operator=(list_iterator&&) noexcept
        = default;

    CIEL_NODISCARD list_iterator
    next() const noexcept {
        return list_iterator(it_->next_);
    }

    CIEL_NODISCARD list_iterator
    prev() const noexcept {
        return list_iterator(it_->prev_);
    }

    CIEL_NODISCARD reference
    operator*() const noexcept {
        return static_cast<node_type*>(it_)->value_;
    }

    CIEL_NODISCARD pointer
    operator->() const noexcept {
        return &static_cast<node_type*>(it_)->value_;
    }

    list_iterator&
    operator++() noexcept {
        it_ = it_->next_;
        return *this;
    }

    CIEL_NODISCARD list_iterator
    operator++(int) noexcept {
        list_iterator res(it_);
        ++(*this);
        return res;
    }

    list_iterator&
    operator--() noexcept {
        it_ = it_->prev_;
        return *this;
    }

    CIEL_NODISCARD list_iterator
    operator--(int) noexcept {
        list_iterator res(it_);
        --(*this);
        return res;
    }

    CIEL_NODISCARD base_node_type*
    base() const noexcept {
        return it_;
    }

    CIEL_NODISCARD explicit
    operator bool() const noexcept {
        return it_ != nullptr;
    }

}; // class list_iterator

template<class T, class Pointer1, class Pointer2, class Reference1, class Reference2>
CIEL_NODISCARD bool
operator==(const list_iterator<T, Pointer1, Reference1>& lhs,
           const list_iterator<T, Pointer2, Reference2>& rhs) noexcept {
    return lhs.base() == rhs.base();
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
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    using base_node_type    = list_node_base;
    using node_type         = list_node<value_type>;
    using alloc_traits      = std::allocator_traits<allocator_type>;
    using node_allocator    = typename alloc_traits::template rebind_alloc<node_type>;
    using node_alloc_traits = typename alloc_traits::template rebind_traits<node_type>;

    base_node_type end_node_;
    node_type* free_node_;
    compressed_pair<size_type, node_allocator> size_node_allocator_;

    void
    do_destroy() noexcept {
        iterator it = begin();
        iterator e  = end();

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

    node_type*
    get_one_free_node() {
        if (free_node_) {
            node_type* res = free_node_;
            free_node_     = static_cast<node_type*>(free_node_->next_);

            return res;
        }

        return node_alloc_traits::allocate(allocator_(), 1);
    }

    void
    store_one_free_node(node_type* free) noexcept {
        free->next_ = free_node_;
        free_node_  = free;
    }

    size_type&
    size_() noexcept {
        return size_node_allocator_.first();
    }

    const size_type&
    size_() const noexcept {
        return size_node_allocator_.first();
    }

    node_allocator&
    allocator_() noexcept {
        return size_node_allocator_.second();
    }

    const node_allocator&
    allocator_() const noexcept {
        return size_node_allocator_.second();
    }

    iterator
    alloc_range_destroy(iterator begin, iterator end) noexcept {
        iterator loop         = begin;
        iterator before_begin = begin.prev();

        while (loop != end) {
            auto* to_be_destroyed = static_cast<node_type*>(loop.base());
            ++loop;

            node_alloc_traits::destroy(allocator_(), to_be_destroyed);
            --size_();
            store_one_free_node(to_be_destroyed);
        }

        before_begin.base()->next_ = end.base();
        end.base()->prev_          = before_begin.base();

        return end;
    }

    // insert before begin
    template<class... Arg>
    iterator
    alloc_range_construct_n(iterator begin, const size_type n, Arg&&... arg) {
        iterator before_begin          = begin.prev();
        iterator original_before_begin = before_begin;

        CIEL_TRY {
            for (size_type i = 0; i < n; ++i) {
                node_type* construct_place = get_one_free_node();

                CIEL_TRY {
                    node_alloc_traits::construct(allocator_(), construct_place, before_begin.base(), begin.base(),
                                                 std::forward<Arg>(arg)...);
                    ++size_();

                    before_begin.base()->next_ = construct_place;
                    begin.base()->prev_        = construct_place;
                    ++before_begin;
                }
                CIEL_CATCH (...) {
                    store_one_free_node(construct_place);
                    CIEL_THROW;
                }
            }
            return original_before_begin.next();
        }
        CIEL_CATCH (...) {
            alloc_range_destroy(original_before_begin.next(), begin);
            CIEL_THROW;
        }
    }

    template<class Iter, typename std::enable_if<is_input_iterator<Iter>::value, int>::type = 0>
    iterator
    alloc_range_construct(iterator begin, Iter first, Iter last) {
        iterator before_begin          = begin.prev();
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
                    begin.base()->prev_        = construct_place;
                    ++before_begin;
                }
                CIEL_CATCH (...) {
                    store_one_free_node(construct_place);
                    CIEL_THROW;
                }
            }
            return original_before_begin.next();
        }
        CIEL_CATCH (...) {
            alloc_range_destroy(original_before_begin.next(), begin);
            CIEL_THROW;
        }
    }

public:
    list()
        : free_node_(nullptr), size_node_allocator_(0, default_init_tag) {}

    explicit list(const allocator_type& alloc)
        : free_node_(nullptr), size_node_allocator_(0, alloc) {}

    list(const size_type count, const value_type& value, const allocator_type& alloc = allocator_type())
        : list(alloc) {
        alloc_range_construct_n(end(), count, value);
    }

    explicit list(const size_type count, const allocator_type& alloc = allocator_type())
        : list(alloc) {
        alloc_range_construct_n(end(), count);
    }

    template<class Iter, typename std::enable_if<is_input_iterator<Iter>::value, int>::type = 0>
    list(Iter first, Iter last, const allocator_type& alloc = allocator_type())
        : list(alloc) {
        alloc_range_construct(end(), first, last);
    }

    list(const list& other)
        : list(other.begin(), other.end(), alloc_traits::select_on_container_copy_construction(other.get_allocator())) {
    }

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
        other.size_()    = 0;
    }

    list(list&& other, const allocator_type& alloc)
        : list() {
        if (alloc == other.get_allocator()) {
            end_node_    = other.end_node_;
            free_node_   = other.free_node_;
            size_()      = other.size_();
            allocator_() = alloc;

            end_node_.next_->prev_ = &end_node_;
            end_node_.prev_->next_ = &end_node_;

            other.end_node_.clear();
            other.free_node_ = nullptr;
            other.size_()    = 0;

        } else {
            list(other, alloc).swap(*this);
        }
    }

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    list(InitializerList init, const allocator_type& alloc = allocator_type())
        : list(init.begin(), init.end(), alloc) {}

    template<class U = value_type, typename std::enable_if<worth_move_constructing<U>::value, int>::type = 0>
    list(std::initializer_list<move_proxy<value_type>> init, const allocator_type& alloc = allocator_type())
        : list(init.begin(), init.end(), alloc) {}

    template<class U = value_type, typename std::enable_if<!worth_move_constructing<U>::value, int>::type = 0>
    list(std::initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
        : list(init.begin(), init.end(), alloc) {}

    ~list() {
        do_destroy();
    }

    list&
    operator=(const list& other) {
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

        return *this;
    }

    list&
    operator=(list&& other) noexcept(alloc_traits::is_always_equal::value) {
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

        end_node_  = other.end_node_;
        free_node_ = other.free_node_;
        size_()    = other.size_();

        end_node_.next_->prev_ = &end_node_;
        end_node_.prev_->next_ = &end_node_;

        other.end_node_.clear();
        other.free_node_ = nullptr;
        other.size_()    = 0;

        return *this;
    }

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    list&
    operator=(InitializerList ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    template<class U = value_type, typename std::enable_if<ciel::worth_move<U>::value, int>::type = 0>
    list&
    operator=(std::initializer_list<move_proxy<value_type>> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    template<class U = value_type, typename std::enable_if<!ciel::worth_move<U>::value, int>::type = 0>
    list&
    operator=(std::initializer_list<value_type> ilist) {
        assign(ilist.begin(), ilist.end());
        return *this;
    }

    void
    assign(size_type count, const value_type& value) {
        iterator it = begin();
        iterator e  = end();

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
    void
    assign(Iter first, Iter last) {
        iterator it = begin();
        iterator e  = end();

        for (; first != last && it != e; ++first, ++it) {
            *it = *first;
        }

        if (it == e) {
            insert(e, std::move(first), std::move(last));

        } else {
            erase(it, e);
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
    front() {
        CIEL_PRECONDITION(!empty());

        return *begin();
    }

    CIEL_NODISCARD const_reference
    front() const {
        CIEL_PRECONDITION(!empty());

        return *begin();
    }

    CIEL_NODISCARD reference
    back() {
        CIEL_PRECONDITION(!empty());

        return *(--end());
    }

    CIEL_NODISCARD const_reference
    back() const {
        CIEL_PRECONDITION(!empty());

        return *(--end());
    }

    CIEL_NODISCARD iterator
    begin() noexcept {
        return iterator(end_node_.next_);
    }

    CIEL_NODISCARD const_iterator
    begin() const noexcept {
        return const_iterator(end_node_.next_);
    }

    CIEL_NODISCARD const_iterator
    cbegin() const noexcept {
        return begin();
    }

    CIEL_NODISCARD iterator
    end() noexcept {
        return iterator(&end_node_);
    }

    CIEL_NODISCARD const_iterator
    end() const noexcept {
        return const_iterator(&end_node_);
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
        return size_() == 0;
    }

    CIEL_NODISCARD size_type
    size() const noexcept {
        return size_();
    }

    CIEL_NODISCARD size_type
    max_size() const noexcept {
        return node_alloc_traits::max_size(allocator_());
    }

    void
    clear() noexcept {
        alloc_range_destroy(begin(), end());
    }

    iterator
    insert(iterator pos, const T& value) {
        return alloc_range_construct_n(pos, 1, value);
    }

    iterator
    insert(iterator pos, T&& value) {
        return alloc_range_construct_n(pos, 1, std::move(value));
    }

    iterator
    insert(iterator pos, const size_type count, const T& value) {
        return alloc_range_construct_n(pos, count, value);
    }

    template<class Iter, typename std::enable_if<is_input_iterator<Iter>::value, int>::type = 0>
    iterator
    insert(iterator pos, Iter first, Iter last) {
        return alloc_range_construct(pos, first, last);
    }

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    iterator
    insert(iterator pos, InitializerList ilist) {
        return alloc_range_construct(pos, ilist.begin(), ilist.end());
    }

    template<class U = value_type, typename std::enable_if<ciel::worth_move_constructing<U>::value, int>::type = 0>
    iterator
    insert(iterator pos, std::initializer_list<move_proxy<value_type>> ilist) {
        return alloc_range_construct(pos, ilist.begin(), ilist.end());
    }

    template<class U = value_type, typename std::enable_if<!ciel::worth_move_constructing<U>::value, int>::type = 0>
    iterator
    insert(iterator pos, std::initializer_list<value_type> ilist) {
        return alloc_range_construct(pos, ilist.begin(), ilist.end());
    }

    template<class... Args>
    iterator
    emplace(iterator pos, Args&&... args) {
        return alloc_range_construct_n(pos, 1, std::forward<Args>(args)...);
    }

    iterator
    erase(iterator pos) {
        return alloc_range_destroy(pos, pos.next());
    }

    iterator
    erase(iterator first, iterator last) {
        return alloc_range_destroy(first, last);
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
        return *alloc_range_construct_n(end(), 1, std::forward<Args>(args)...);
    }

    void
    pop_back() noexcept {
        CIEL_PRECONDITION(!empty());

        alloc_range_destroy(end().prev(), end());
    }

    void
    push_front(const value_type& value) {
        emplace_front(value);
    }

    void
    push_front(value_type&& value) {
        emplace_front(std::move(value));
    }

    template<class... Args>
    reference
    emplace_front(Args&&... args) {
        return *alloc_range_construct_n(begin(), 1, std::forward<Args>(args)...);
    }

    void
    pop_front() noexcept {
        CIEL_PRECONDITION(!empty());

        alloc_range_destroy(begin(), begin().next());
    }

    void
    resize(const size_type count) {
        if (size() >= count) {
            iterator tmp = std::prev(end(), size() - count);
            alloc_range_destroy(tmp, end());

        } else {
            alloc_range_construct_n(end(), count - size());
        }
    }

    void
    resize(const size_type count, const value_type& value) {
        if (size() >= count) {
            iterator tmp = std::prev(end(), size() - count);
            alloc_range_destroy(tmp, end());

        } else {
            alloc_range_construct_n(end(), count - size(), value);
        }
    }

    void
    swap(list& other) noexcept(alloc_traits::is_always_equal::value) {
        using std::swap;

        swap(end_node_, other.end_node_);

        end_node_.next_->prev_ = &end_node_;
        end_node_.prev_->next_ = &end_node_;

        other.end_node_.next_->prev_ = &other.end_node_;
        other.end_node_.prev_->next_ = &other.end_node_;

        swap(free_node_, other.free_node_);
        swap(size_node_allocator_, other.size_node_allocator_);
    }

}; // class list

template<class T, class Allocator>
struct is_trivially_relocatable<list<T, Allocator>> : std::false_type {};

#if CIEL_STD_VER >= 17

template<class Iter, class Alloc = std::allocator<typename std::iterator_traits<Iter>::value_type>>
list(Iter, Iter, Alloc = Alloc()) -> list<typename std::iterator_traits<Iter>::value_type, Alloc>;

#endif

NAMESPACE_CIEL_END

namespace std {

template<class T, class Alloc>
void
swap(ciel::list<T, Alloc>& lhs, ciel::list<T, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_LIST_HPP_

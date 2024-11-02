#ifndef CIELLAB_INCLUDE_CIEL_TEST_ALLOCATOR_HPP_
#define CIELLAB_INCLUDE_CIEL_TEST_ALLOCATOR_HPP_

#include <ciel/config.hpp>

#include <cstddef>
#include <limits>
#include <memory>

NAMESPACE_CIEL_BEGIN

struct test_allocator_statistics {
    int time_to_throw   = 0;
    int throw_after     = std::numeric_limits<int>::max();
    int count           = 0; // the number of active instances
    int alloc_count     = 0; // the number of allocations not deallocating
    int allocated_size  = 0; // the size of allocated elements
    int construct_count = 0; // the number of times that ::construct was called
    int destroy_count   = 0; // the number of times that ::destroy was called
    int copied          = 0;
    int moved           = 0;
    int converted       = 0;

    void
    clear() noexcept {
        CIEL_PRECONDITION(count == 0);

        count           = 0;
        time_to_throw   = 0;
        alloc_count     = 0;
        allocated_size  = 0;
        construct_count = 0;
        destroy_count   = 0;
        throw_after     = std::numeric_limits<int>::max();
        clear_ctor_counters();
    }

    void
    clear_ctor_counters() noexcept {
        copied    = 0;
        moved     = 0;
        converted = 0;
    }

}; // struct test_allocator_statistics

struct test_alloc_base {
    static constexpr int destructed_value = -1;
    static constexpr int moved_value      = std::numeric_limits<int>::max();

}; // struct test_alloc_base

template<class T>
class test_allocator {
    int data_                         = 0; // participates in equality
    int id_                           = 0; // unique identifier, doesn't participate in equality
    test_allocator_statistics* stats_ = nullptr;

    template<class U>
    friend class test_allocator;

public:
    using size_type       = unsigned;
    using difference_type = int;
    using value_type      = T;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;
    using reference       = typename std::add_lvalue_reference<value_type>::type;
    using const_reference = typename std::add_lvalue_reference<const value_type>::type;

    template<class U>
    struct rebind {
        using other = test_allocator<U>;
    };

    test_allocator() noexcept = default;

    explicit test_allocator(test_allocator_statistics* stats) noexcept
        : stats_(stats) {
        if (stats_ != nullptr) {
            ++stats_->count;
        }
    }

    explicit test_allocator(int data) noexcept
        : data_(data) {}

    test_allocator(int data, test_allocator_statistics* stats) noexcept
        : data_(data), stats_(stats) {
        if (stats != nullptr) {
            ++stats_->count;
        }
    }

    test_allocator(int data, int id) noexcept
        : data_(data), id_(id) {}

    test_allocator(int data, int id, test_allocator_statistics* stats) noexcept
        : data_(data), id_(id), stats_(stats) {
        if (stats_ != nullptr) {
            ++stats_->count;
        }
    }

    test_allocator(const test_allocator& a) noexcept
        : data_(a.data_), id_(a.id_), stats_(a.stats_) {
        CIEL_PRECONDITION(a.data_ != test_alloc_base::destructed_value && a.id_ != test_alloc_base::destructed_value);

        if (stats_ != nullptr) {
            ++stats_->count;
            ++stats_->copied;
        }
    }

    test_allocator(test_allocator&& a) noexcept
        : data_(a.data_), id_(a.id_), stats_(a.stats_) {
        if (stats_ != nullptr) {
            ++stats_->count;
            ++stats_->moved;
        }

        CIEL_PRECONDITION(a.data_ != test_alloc_base::destructed_value && a.id_ != test_alloc_base::destructed_value);

        a.id_ = test_alloc_base::moved_value;
    }

    template<class U>
    test_allocator(const test_allocator<U>& a) noexcept
        : data_(a.data_), id_(a.id_), stats_(a.stats_) {
        if (stats_ != nullptr) {
            ++stats_->count;
            ++stats_->converted;
        }
    }

    ~test_allocator() {
        CIEL_PRECONDITION(data_ != test_alloc_base::destructed_value);
        CIEL_PRECONDITION(id_ != test_alloc_base::destructed_value);

        if (stats_ != nullptr) {
            --stats_->count;
        }

        data_ = test_alloc_base::destructed_value;
        id_   = test_alloc_base::destructed_value;
    }

    CIEL_NODISCARD pointer
    address(reference x) const noexcept {
        return &x;
    }

    CIEL_NODISCARD const_pointer
    address(const_reference x) const noexcept {
        return &x;
    }

    CIEL_NODISCARD pointer
    allocate(size_type n, const void* = nullptr) {
        CIEL_PRECONDITION(data_ != test_alloc_base::destructed_value);

        if (stats_ != nullptr) {
            if (stats_->time_to_throw >= stats_->throw_after) {
                CIEL_THROW_EXCEPTION(std::bad_alloc{});
            }

            ++stats_->time_to_throw;
            ++stats_->alloc_count;
            stats_->allocated_size += n;
        }

        return std::allocator<value_type>().allocate(n);
    }

    void
    deallocate(pointer p, size_type s) noexcept {
        CIEL_PRECONDITION(data_ != test_alloc_base::destructed_value);

        if (stats_ != nullptr) {
            --stats_->alloc_count;
            stats_->allocated_size -= s;
        }

        std::allocator<value_type>().deallocate(p, s);
    }

    CIEL_NODISCARD size_type
    max_size() const noexcept {
        return std::numeric_limits<unsigned int>::max() / sizeof(T);
    }

    template<class U>
    void
    construct(pointer p, U&& val) {
        if (stats_ != nullptr) {
            ++stats_->construct_count;
        }

        ::new (static_cast<void*>(p)) T(std::forward<U>(val));
    }

    void
    destroy(pointer p) noexcept {
        if (stats_ != nullptr) {
            ++stats_->destroy_count;
        }

        p->~T();
    }

    CIEL_NODISCARD int
    get_data() const noexcept {
        return data_;
    }

    CIEL_NODISCARD int
    get_id() const noexcept {
        return id_;
    }

    CIEL_NODISCARD friend bool
    operator==(const test_allocator& x, const test_allocator& y) noexcept {
        return x.data_ == y.data_;
    }

}; // class test_allocator

template<>
class test_allocator<void> {
    int data_                         = 0;
    int id_                           = 0;
    test_allocator_statistics* stats_ = nullptr;

    template<class U>
    friend class test_allocator;

public:
    using size_type       = unsigned;
    using difference_type = int;
    using value_type      = void;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;

    template<class U>
    struct rebind {
        using other = test_allocator<U>;
    };

    test_allocator() noexcept = default;

    explicit test_allocator(test_allocator_statistics* stats) noexcept
        : stats_(stats) {}

    explicit test_allocator(int data) noexcept
        : data_(data) {}

    test_allocator(int data, test_allocator_statistics* stats) noexcept
        : data_(data), stats_(stats) {}

    test_allocator(int data, int id) noexcept
        : data_(data), id_(id) {}

    test_allocator(int data, int id, test_allocator_statistics* stats) noexcept
        : data_(data), id_(id), stats_(stats) {}

    test_allocator(const test_allocator& a) noexcept
        : data_(a.data_), id_(a.id_), stats_(a.stats_) {}

    template<class U>
    test_allocator(const test_allocator<U>& a) noexcept
        : data_(a.data_), id_(a.id_), stats_(a.stats_) {}

    ~test_allocator() {
        data_ = test_alloc_base::destructed_value;
        id_   = test_alloc_base::destructed_value;
    }

    CIEL_NODISCARD int
    get_id() const noexcept {
        return id_;
    }

    CIEL_NODISCARD int
    get_data() const noexcept {
        return data_;
    }

    CIEL_NODISCARD friend bool
    operator==(const test_allocator& x, const test_allocator& y) noexcept {
        return x.data_ == y.data_;
    }

}; // class test_allocator<void>

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TEST_ALLOCATOR_HPP_

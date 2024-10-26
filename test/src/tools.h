#ifndef CIELLAB_TEST_TOOLS_H_
#define CIELLAB_TEST_TOOLS_H_

#include <ciel/alignment.hpp>
#include <ciel/compare.hpp>
#include <ciel/config.hpp>
#include <ciel/exchange.hpp>
#include <ciel/is_trivially_relocatable.hpp>
#include <ciel/iterator_base.hpp>
#include <ciel/move_proxy.hpp>
#include <ciel/worth_move.hpp>

#include <condition_variable>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>

struct ConstructAndAssignCounter {
    // To not be considered as trivially_relocatable.
    char padding_{};

    static size_t copy_;
    static size_t move_;

    static void
    reset() noexcept;
    static size_t
    copy() noexcept;
    static size_t
    move() noexcept;

    ConstructAndAssignCounter() noexcept = default;

    ConstructAndAssignCounter(const ConstructAndAssignCounter&) noexcept {
        ++copy_;
    }

    ConstructAndAssignCounter(ConstructAndAssignCounter&&) noexcept {
        ++move_;
    }

    ConstructAndAssignCounter&
    operator=(const ConstructAndAssignCounter&) noexcept {
        ++copy_;
        return *this;
    }

    ConstructAndAssignCounter&
    operator=(ConstructAndAssignCounter&&) noexcept {
        ++move_;
        return *this;
    }

}; // struct ConstructAndAssignCounter

static_assert(not ciel::is_trivially_relocatable<ConstructAndAssignCounter>::value, "");

struct MoveProxyTestClass {
    using value_type = ConstructAndAssignCounter;

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    MoveProxyTestClass&
    operator=(InitializerList il) noexcept {
        for (auto&& t : il) {
            CIEL_UNUSED(value_type{std::forward<decltype(t)>(t)});
        }

        return *this;
    }

    template<class U = value_type, typename std::enable_if<ciel::worth_move<U>::value, int>::type = 0>
    MoveProxyTestClass&
    operator=(std::initializer_list<ciel::move_proxy<value_type>> il) noexcept {
        for (auto&& t : il) {
            CIEL_UNUSED(value_type{std::forward<decltype(t)>(t)});
        }

        return *this;
    }

    template<class U = value_type, typename std::enable_if<!ciel::worth_move<U>::value, int>::type = 0>
    MoveProxyTestClass&
    operator=(std::initializer_list<value_type> il) noexcept {
        for (auto&& t : il) {
            CIEL_UNUSED(value_type{std::forward<decltype(t)>(t)});
        }

        return *this;
    }

}; // struct MoveProxyTestClass

template<class T, size_t Size, size_t Alignment>
struct AlignedAllocator {
    using value_type                             = T;
    using size_type                              = size_t;
    using difference_type                        = ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;

    static_assert(alignof(value_type) <= ciel::max_align, "");

    alignas(Alignment) unsigned char buf[Size]{};

    AlignedAllocator() noexcept = default;

    AlignedAllocator(const AlignedAllocator&) noexcept = default;

    AlignedAllocator(AlignedAllocator&& other) noexcept {
        other.buf[0] = 'x';
    }

    // clang-format off
    AlignedAllocator& operator=(const AlignedAllocator&) noexcept = default;
    // clang-format on

    AlignedAllocator&
    operator=(AlignedAllocator&& other) noexcept {
        other.buf[0] = 'x';
        return *this;
    }

    template<class U>
    struct rebind {
        using other = AlignedAllocator<U, Size, Alignment>;
    };

    value_type*
    allocate(const size_t n) {
        return static_cast<value_type*>(::operator new(n * sizeof(value_type)));
    }

    void
    deallocate(value_type* p, size_t) noexcept {
        ::operator delete(p);
    }

}; // struct SimpleAllocator

#endif // CIELLAB_TEST_TOOLS_H_

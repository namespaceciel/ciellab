#ifndef CIELLAB_TEST_TOOLS_H_
#define CIELLAB_TEST_TOOLS_H_

#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>

#include <ciel/config.hpp>
#include <ciel/move_proxy.hpp>

struct ConstructAndAssignCounter {
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
};

struct MoveProxyTestClass {
    template<class InitializerList,
             typename std::enable_if<
                 std::is_same<typename std::remove_cv<typename std::remove_reference<InitializerList>::type>::type,
                              std::initializer_list<ConstructAndAssignCounter>>::value,
                 int>::type
             = 0>
    MoveProxyTestClass&
    operator=(InitializerList&& il) noexcept {
        for (auto&& t : il) {
            CIEL_UNUSED(ConstructAndAssignCounter{std::forward<decltype(t)>(t)});
        }

        return *this;
    }

    MoveProxyTestClass&
    operator=(std::initializer_list<ciel::move_proxy<ConstructAndAssignCounter>> il) noexcept {
        for (auto&& t : il) {
            CIEL_UNUSED(ConstructAndAssignCounter{std::forward<decltype(t)>(t)});
        }

        return *this;
    }
};

#endif // CIELLAB_TEST_TOOLS_H_

#ifndef CIELLAB_TEST_TOOLS_H_
#define CIELLAB_TEST_TOOLS_H_

#include <condition_variable>
#include <cstddef>
#include <initializer_list>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>

#include <ciel/config.hpp>
#include <ciel/move_proxy.hpp>
#include <ciel/type_traits.hpp>

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

class SimpleLatch {
public:
    SimpleLatch(const size_t count_down) noexcept
        : count_down_(count_down) {}

    void
    arrive_and_wait() noexcept {
        std::unique_lock<std::mutex> lock(mutex_);

        if (--count_down_ == 0) {
            cv_.notify_all();

        } else {
            cv_.wait(lock);
        }
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    size_t count_down_;

}; // class SimpleLatch

#endif // CIELLAB_TEST_TOOLS_H_

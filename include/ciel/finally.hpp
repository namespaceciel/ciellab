#ifndef CIELLAB_INCLUDE_CIEL_FINALLY_HPP_
#define CIELLAB_INCLUDE_CIEL_FINALLY_HPP_

#include <utility>

#include <ciel/config.hpp>

NAMESPACE_CIEL_BEGIN

template<class F>
class finally {
public:
    explicit finally(const F& f) : f_(f), valid_(true) {}

    explicit finally(F&& f) : f_(std::move(f)), valid_(true) {}

    finally(finally&& other) noexcept : f_(std::move(other.f_)), valid_(other.valid_) {
        other.valid_ = false;
    }

    ~finally() {
        if (valid_) {
            f_();
        }
    }

    finally(const finally&) = delete;
    auto operator=(const finally&) -> finally& = delete;
    auto operator=(finally&&) noexcept -> finally& = delete;

private:
    F f_;
    bool valid_;

};    // class finally

template<class F>
auto make_finally(F&& f) -> finally<F> {
    return finally<F>(std::forward<F>(f));
}

NAMESPACE_CIEL_END

#define CIEL_CONCAT(a, b) a##b

#define CIEL_DEFER(x) \
    auto CIEL_CONCAT(defer_, __LINE__) = ciel::make_finally([&] { x; })

#endif // CIELLAB_INCLUDE_CIEL_FINALLY_HPP_
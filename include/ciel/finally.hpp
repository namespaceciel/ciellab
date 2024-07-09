#ifndef CIELLAB_INCLUDE_CIEL_FINALLY_HPP_
#define CIELLAB_INCLUDE_CIEL_FINALLY_HPP_

#include <utility>

#include <ciel/config.hpp>

NAMESPACE_CIEL_BEGIN

template<class F>
class finally {
public:
    explicit finally(const F& f)
        : f_(f), valid_(true) {}

    explicit finally(F&& f)
        : f_(std::move(f)), valid_(true) {}

    finally(finally&& other) noexcept
        : f_(std::move(other.f_)), valid_(other.valid_) {
        other.valid_ = false;
    }

    ~finally() {
        if (valid_) {
            f_();
        }
    }

    finally(const finally&) = delete;
    finally&
    operator=(const finally&)
        = delete;
    finally&
    operator=(finally&&) noexcept
        = delete;

private:
    F f_;
    bool valid_;

}; // class finally

template<class F>
finally<F>
make_finally(F&& f) {
    return finally<F>(std::forward<F>(f));
}

NAMESPACE_CIEL_END

// It will be "ab" without forwarding, so all variables' names will be defer___LINE__.
// Forwarding get the real line number like defer_12.
#define CIEL_CONCAT_(a, b) a##b
#define CIEL_CONCAT(a, b)  CIEL_CONCAT_(a, b)

#define CIEL_DEFER(x) auto CIEL_CONCAT(defer_, __LINE__) = ciel::make_finally([&] x)

#endif // CIELLAB_INCLUDE_CIEL_FINALLY_HPP_

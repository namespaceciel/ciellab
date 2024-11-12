#ifndef CIELLAB_INCLUDE_CIEL_FINALLY_HPP_
#define CIELLAB_INCLUDE_CIEL_FINALLY_HPP_

#include <ciel/config.hpp>
#include <ciel/exchange.hpp>

#include <utility>

NAMESPACE_CIEL_BEGIN

template<class F>
class finally {
public:
    explicit finally(const F& f)
        : f_(f), valid_(true) {}

    explicit finally(F&& f)
        : f_(std::move(f)), valid_(true) {}

    finally(finally&& other) noexcept
        : f_(std::move(other.f_)), valid_(ciel::exchange(other.valid_, false)) {}

    ~finally() {
        if (valid_) {
            f_();
        }
    }

    void release() noexcept {
        valid_ = false;
    }

    finally(const finally&)            = delete;
    finally& operator=(const finally&) = delete;
    finally& operator=(finally&&)      = delete;

private:
    F f_;
    bool valid_;

}; // class finally

template<class F, class DecayF = decay_t<F>>
CIEL_NODISCARD finally<DecayF> make_finally(F&& f) {
    return finally<DecayF>(std::forward<F>(f));
}

NAMESPACE_CIEL_END

// It will be "ab" without forwarding, so all variables' names will be defer___LINE__.
// Forwarding get the real line number like defer_12.
#define CIEL_CONCAT_(a, b) a##b
#define CIEL_CONCAT(a, b)  CIEL_CONCAT_(a, b)
// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define CIEL_DEFER(x) auto CIEL_CONCAT(defer_, __LINE__) = ciel::make_finally([&] x)

#endif // CIELLAB_INCLUDE_CIEL_FINALLY_HPP_

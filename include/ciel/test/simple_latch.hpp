#ifndef CIELLAB_INCLUDE_CIEL_TEST_SIMPLE_LATCH_HPP_
#define CIELLAB_INCLUDE_CIEL_TEST_SIMPLE_LATCH_HPP_

#include <ciel/core/config.hpp>

#include <condition_variable>
#include <cstddef>
#include <mutex>

NAMESPACE_CIEL_BEGIN

class SimpleLatch {
public:
    SimpleLatch(const size_t count_down) noexcept
        : count_down_(count_down) {}

    void arrive_and_wait() noexcept {
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

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_TEST_SIMPLE_LATCH_HPP_

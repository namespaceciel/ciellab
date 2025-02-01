#ifndef CIELLAB_INCLUDE_CIEL_CORE_SINGLETON_HPP_
#define CIELLAB_INCLUDE_CIEL_CORE_SINGLETON_HPP_

#include <ciel/core/config.hpp>
#include <ciel/core/finally.hpp>
#include <ciel/core/message.hpp>

#include <atomic>
#include <thread>

NAMESPACE_CIEL_BEGIN

// Inspired by Microsoft snmalloc's implementation.

// Derived must be defaultly constructible.
template<class Derived>
class singleton {
protected:
    singleton() = default;

    enum struct State {
        Uninitialised = 0,
        Initialising,
        Initialised

    }; // enum struct State

public:
    singleton(const singleton&)            = delete;
    singleton& operator=(const singleton&) = delete;

    CIEL_NODISCARD static Derived& get() {
        static std::atomic<State> state;
        alignas(Derived) static unsigned char buffer[sizeof(Derived)];
        static Derived* res;

        State s = state.load(std::memory_order_acquire);

        while (s != State::Initialised) {
            // s is either Uninitialised or Initialising. If s is Uninitialised,
            // it will either win CAS and perform construction of Derived
            // or lose CAS and load as Initialising or Initialised.
            if (s == State::Uninitialised
                && state.compare_exchange_strong(s, State::Initialising, std::memory_order_relaxed)) {
                CIEL_TRY {
                    res = ::new (&buffer) Derived;
                }
                CIEL_CATCH (...) {
                    state.store(State::Uninitialised, std::memory_order_release);
                    CIEL_THROW;
                }

                state.store(State::Initialised, std::memory_order_release);

            } else {
                while (s == State::Initialising) {
                    std::this_thread::yield();
                    s = state.load(std::memory_order_acquire);
                }
            }
        }

        return *res;
    }

}; // class singleton

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_CORE_SINGLETON_HPP_

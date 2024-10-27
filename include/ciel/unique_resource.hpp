#ifndef CIELLAB_INCLUDE_CIEL_UNIQUE_RESOURCE_HPP_
#define CIELLAB_INCLUDE_CIEL_UNIQUE_RESOURCE_HPP_

#include <ciel/compressed_pair.hpp>
#include <ciel/config.hpp>
#include <ciel/exchange.hpp>
#include <ciel/finally.hpp>
#include <ciel/is_nullable.hpp>
#include <ciel/do_if_noexcept.hpp>

#include <functional>
#include <type_traits>
#include <utility>

NAMESPACE_CIEL_BEGIN

namespace unique_resource_detail {

template<class T>
struct destroy_impl {
    void
    operator()(T& value) noexcept {
        value.~T();
    }

}; // struct destroy_impl

} // namespace unique_resource_detail

struct no_op {
    void
    operator()(...) noexcept {}
};

template<class T>
using destroyer = typename std::conditional<std::is_trivially_destructible<T>::value, no_op,
                                            unique_resource_detail::destroy_impl<T>>::type;

template<class T>
struct invoker {
    void
    operator()(T& value) noexcept(noexcept(value())) {
        CIEL_DEFER({ destroyer<T>{}(value); });
        value();
    }

}; // struct invoke

// If T is not an object type, it will be a reference to a function or anything else.
// If it is a function reference, it should be decayed to a function pointer.
template<class T, class Deleter = destroyer<T>,
         class RT = typename std::conditional<
             std::is_object<T>::value, T,
             typename std::conditional<std::is_function<typename std::remove_reference<T>::type>::value,
                                       typename std::decay<T>::type,
                                       std::reference_wrapper<typename std::remove_reference<T>::type>>::type>::type,
         bool = is_nullable<RT>::value>
class unique_resource {
public:
    using value_type   = RT;
    using deleter_type = Deleter;

    static_assert(std::is_nothrow_move_constructible<value_type>::value
                      || std::is_copy_constructible<value_type>::value,
                  "");
    static_assert(std::is_nothrow_move_constructible<deleter_type>::value
                      || std::is_copy_constructible<deleter_type>::value,
                  "");

private:
    union {
        value_type value_;
    };

    union {
        compressed_pair<bool, deleter_type> pair_;
    };

    CIEL_NODISCARD bool&
    has_value_() noexcept {
        return pair_.first();
    }

    CIEL_NODISCARD const bool&
    has_value_() const noexcept {
        return pair_.first();
    }

    CIEL_NODISCARD deleter_type&
    deleter_() noexcept {
        return pair_.second();
    }

    CIEL_NODISCARD const deleter_type&
    deleter_() const noexcept {
        return pair_.second();
    }

public:
    template<class U = value_type, class D = Deleter,
             typename std::enable_if<std::is_default_constructible<U>::value && std::is_default_constructible<D>::value,
                                     int>::type
             = 0>
    unique_resource() noexcept(noexcept(deleter_type()))
        : pair_(false, value_init) {}

    template<class RR, class DD>
    unique_resource(RR&& r, DD&& d) noexcept((std::is_nothrow_constructible<value_type, RR>::value
                                              || std::is_nothrow_constructible<value_type, RR&>::value)
                                             && (std::is_nothrow_constructible<deleter_type, DD>::value
                                                 || std::is_nothrow_constructible<deleter_type, DD&>::value)) {
        CIEL_TRY {
            new (static_cast<void*>(&value_)) value_type(ciel::forward_if_noexcept<RR>(r));
        }
        CIEL_CATCH (...) {
            d(r);
            CIEL_THROW;
        }

        CIEL_TRY {
            if (std::is_nothrow_constructible<deleter_type, DD>::value) {
                new (static_cast<void*>(&pair_)) compressed_pair<bool, deleter_type>(true, std::forward<DD>(d));

            } else {
                new (static_cast<void*>(&pair_)) compressed_pair<bool, deleter_type>(true, d);
            }
        }
        CIEL_CATCH (...) {
            d(value_);
            CIEL_THROW;
        }
    }

    unique_resource(unique_resource&& other) noexcept(std::is_nothrow_move_constructible<value_type>::value
                                                      && std::is_nothrow_move_constructible<deleter_type>::value) {
        if (std::is_nothrow_move_constructible<value_type>::value) {
            new (static_cast<void*>(&value_)) value_type(std::move(other.value_));

        } else {
            new (static_cast<void*>(&value_)) value_type(other.value_);
        }

        CIEL_DEFER(other.reset());

        CIEL_TRY {
            if (std::is_nothrow_move_constructible<deleter_type>::value) {
                new (static_cast<void*>(&pair_)) compressed_pair<bool, deleter_type>(std::move(other.pair_));

            } else {
                new (static_cast<void*>(&pair_)) compressed_pair<bool, deleter_type>(other.pair_);
            }
        }
        CIEL_CATCH (...) {
            other.deleter_()(value_);
            CIEL_THROW;
        }
    }

    ~unique_resource() {
        if (has_value_()) {
            deleter_()(value_);
        }
    }

    CIEL_NODISCARD value_type&
    get() noexcept {
        return value_;
    }

    CIEL_NODISCARD const value_type&
    get() const noexcept {
        return value_;
    }

}; // class unique_resource

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_UNIQUE_RESOURCE_HPP_

#ifndef CIELLAB_INCLUDE_CIEL_NOT_CONSTRUCTIBLE_HPP_
#define CIELLAB_INCLUDE_CIEL_NOT_CONSTRUCTIBLE_HPP_

#include <ciel/config.hpp>

#include <cstddef>
#include <functional>

NAMESPACE_CIEL_BEGIN

class NotConstructible {
private:
    NotConstructible(const NotConstructible&);
    NotConstructible&
    operator=(const NotConstructible&);

public:
    CIEL_NODISCARD friend bool
    operator==(const NotConstructible&, const NotConstructible&) noexcept {
        return true;
    }
}; // class NotConstructible

NAMESPACE_CIEL_END

namespace std {

template<>
struct hash<ciel::NotConstructible> {
    CIEL_NODISCARD size_t
    operator()(const ciel::NotConstructible&) const noexcept {
        return 0;
    }
};

} // namespace std

#endif // CIELLAB_INCLUDE_CIEL_NOT_CONSTRUCTIBLE_HPP_

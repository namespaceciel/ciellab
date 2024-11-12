#ifndef CIELLAB_INCLUDE_CIEL_DEMANGLE_HPP_
#define CIELLAB_INCLUDE_CIEL_DEMANGLE_HPP_

#include <ciel/config.hpp>

#include <cstddef>
#include <cstdlib>
#include <string>

#if defined(__has_include) && __has_include(<cxxabi.h>)
#  define CIEL_HAS_CXXABI_H
#  include <cxxabi.h>
#endif

NAMESPACE_CIEL_BEGIN

#ifdef CIEL_HAS_CXXABI_H
inline const char* demangle_alloc(const char* name) noexcept {
    size_t size = 0;
    int status  = 0;
    return abi::__cxa_demangle(name, nullptr, &size, &status);
}

inline void demangle_free(const char* name) noexcept {
    std::free(const_cast<char*>(name)); // NOLINT(cppcoreguidelines-no-malloc)
}
#else
inline const char* demangle_alloc(const char* name) noexcept {
    return name;
}

inline void demangle_free(const char*) noexcept {}
#endif

class scoped_demangled_name {
private:
    const char* m_p;

public:
    explicit scoped_demangled_name(const char* name) noexcept
        : m_p(demangle_alloc(name)) {}

    scoped_demangled_name(const scoped_demangled_name&)            = delete;
    scoped_demangled_name& operator=(const scoped_demangled_name&) = delete;

    ~scoped_demangled_name() noexcept {
        demangle_free(m_p);
    }

    const char* get() const noexcept {
        return m_p;
    }

}; // class scoped_demangled_name

#ifdef CIEL_HAS_CXXABI_H
inline std::string demangle(const char* name) {
    const scoped_demangled_name demangled_name(name);
    const char* p = demangled_name.get();
    if (!p) {
        p = name;
    }
    return p;
}
#else
inline std::string demangle(const char* name) {
    return name;
}
#endif

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_DEMANGLE_HPP_

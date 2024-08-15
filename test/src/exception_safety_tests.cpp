#include <gtest/gtest.h>

#include <ciel/list.hpp>
#include <ciel/small_vector.hpp>
#include <ciel/split_buffer.hpp>
#include <ciel/vector.hpp>
#include <random>

#ifdef CIEL_HAS_EXCEPTIONS

// We use random number generator to throw some exceptions, use valgrind to test if there are any memory leaks.

namespace {

std::random_device rd;
std::mt19937_64 g(rd());
bool can_throw; // set this false to renew state_holder

void
may_throw() {
    if (can_throw && g() % 5 == 0) {
        ciel::throw_exception(std::exception{});
    }
}

struct NothrowMoveStruct {
    size_t* ptr{nullptr};

    NothrowMoveStruct() noexcept = default;

    NothrowMoveStruct(size_t i) {
        may_throw();
        ptr = new size_t{i};
    }

    NothrowMoveStruct(const NothrowMoveStruct& other) {
        may_throw();
        if (other.ptr) {
            ptr = new size_t{*other.ptr};
        }
    }

    NothrowMoveStruct(NothrowMoveStruct&& other) noexcept {
        ptr       = other.ptr;
        other.ptr = nullptr;
    }

    ~NothrowMoveStruct() {
        if (ptr) {
            delete ptr;
        }
    }

    NothrowMoveStruct&
    operator=(const NothrowMoveStruct& other) {
        if (this == std::addressof(other)) {
            return *this;
        }
        may_throw();
        if (ptr) {
            if (other.ptr) {
                *ptr = *other.ptr;
            } else {
                delete ptr;
                ptr = nullptr;
            }
        } else if (other.ptr) {
            ptr = new size_t{*other.ptr};
        }
        return *this;
    }

    NothrowMoveStruct&
    operator=(NothrowMoveStruct&& other) noexcept {
        if (this == std::addressof(other)) {
            return *this;
        }
        if (ptr) {
            delete ptr;
        }
        ptr       = other.ptr;
        other.ptr = nullptr;
        return *this;
    }

    explicit
    operator size_t() const noexcept {
        return ptr ? *ptr : 1234;
    }

}; // struct NothrowMoveStruct

[[maybe_unused]] bool
operator==(const NothrowMoveStruct& lhs, const NothrowMoveStruct& rhs) {
    if (lhs.ptr) {
        if (rhs.ptr) {
            return *lhs.ptr == *rhs.ptr;
        }

        return false;
    }

    if (rhs.ptr) {
        return false;
    }

    return true;
}

const std::initializer_list<NothrowMoveStruct> il{{11}, {12}, {13}, {}, {14}, {15}, {},  {16},
                                                  {},   {17}, {18}, {}, {19}, {},   {20}};

} // namespace

#define STRONG_TEST_CASE(X)         \
    can_throw    = false;           \
    state_holder = v;               \
    CIEL_TRY {                      \
        can_throw = true;           \
                                    \
        X;                          \
    }                               \
    CIEL_CATCH (...) {              \
        ASSERT_EQ(v, state_holder); \
    }

#define BASIC_TEST_CASE(X) \
    CIEL_TRY {             \
        X;                 \
    }                      \
    CIEL_CATCH (...) {}

TEST(exception_safety_tests, vector_strong) {
    // These vector functions provide strong exception safety:
    // emplace_back, push_back, reserve, shrink_to_fit
    // When these functions throw, they have no effects, so we test if v changes its state in catch block

    ciel::vector<NothrowMoveStruct> v;
    ciel::vector<NothrowMoveStruct> state_holder;

    for (size_t i = 0; i < 200; ++i) {
        STRONG_TEST_CASE(v.shrink_to_fit());

        STRONG_TEST_CASE(v.emplace_back(2));

        STRONG_TEST_CASE(v.reserve(g() % 4000));
    }
}

TEST(exception_safety_tests, vector_basic) {
    // Throw lots of exceptions and use valgrind checking for memory leaks

    ciel::vector<NothrowMoveStruct> v;
    can_throw = true;

    for (size_t i = 0; i < 1000; ++i) {
        // Use random numbers to insert or erase at any position in v: v.begin() + g() % ciel::max<size_t>(v.size(), 1)

        BASIC_TEST_CASE(v.emplace_back());

        BASIC_TEST_CASE(v.emplace(v.begin() + g() % std::max<size_t>(v.size(), 1), 10ULL));

        BASIC_TEST_CASE(v.assign(il));

        BASIC_TEST_CASE(v.resize(g() % (v.size() * 2 + 1), 5));

        BASIC_TEST_CASE(v.insert(v.begin() + g() % std::max<size_t>(v.size(), 1), 10, 20));

        BASIC_TEST_CASE(v.assign(10, 20));

        BASIC_TEST_CASE(v.emplace_back(1));

        BASIC_TEST_CASE(
            v.erase(v.begin() + g() % std::max<size_t>(v.size(), 1), v.begin() + g() % std::max<size_t>(v.size(), 1)));

        BASIC_TEST_CASE(v.insert(v.begin() + g() % std::max<size_t>(v.size(), 1), il));
    }
}

TEST(exception_safety_tests, small_vector_strong) {
    // These vector functions provide strong exception safety:
    // emplace_back, push_back, reserve
    // When these functions throw, they have no effects, so we test if v changes its state in catch block

    ciel::small_vector<NothrowMoveStruct> v;
    ciel::small_vector<NothrowMoveStruct> state_holder;

    for (size_t i = 0; i < 200; ++i) {
        STRONG_TEST_CASE(v.emplace_back(2));

        STRONG_TEST_CASE(v.reserve(g() % 4000));
    }
}

TEST(exception_safety_tests, small_vector_basic) {
    // Throw lots of exceptions and use valgrind checking for memory leaks

    ciel::small_vector<NothrowMoveStruct> v;
    can_throw = true;

    for (size_t i = 0; i < 1000; ++i) {
        // Use random numbers to insert or erase at any position in v: v.begin() + g() % ciel::max<size_t>(v.size(), 1)

        BASIC_TEST_CASE(v.emplace_back());

        BASIC_TEST_CASE(v.assign(il));

        BASIC_TEST_CASE(v.resize(g() % (v.size() * 2 + 1), 5));

        BASIC_TEST_CASE(v.insert(v.begin() + g() % std::max<size_t>(v.size(), 1), 10, 20));

        BASIC_TEST_CASE(v.assign(10, 20));

        BASIC_TEST_CASE(v.emplace_back(1));

        BASIC_TEST_CASE(
            v.erase(v.begin() + g() % std::max<size_t>(v.size(), 1), v.begin() + g() % std::max<size_t>(v.size(), 1)));

        BASIC_TEST_CASE(v.insert(v.begin() + g() % std::max<size_t>(v.size(), 1), il));
    }
}

TEST(exception_safety_tests, split_buffer_strong) {
    // These split_buffer functions provide strong exception safety:
    // emplace_front/back, push_front/back, shrink_to_fit

    ciel::split_buffer<NothrowMoveStruct> v;
    ciel::split_buffer<NothrowMoveStruct> state_holder;

    for (size_t i = 0; i < 200; ++i) {
        STRONG_TEST_CASE(v.emplace_back(2));

        STRONG_TEST_CASE(v.shrink_to_fit());

        STRONG_TEST_CASE(v.emplace_front(4));
    }
}

TEST(exception_safety_tests, split_buffer_basic) {
    // Throw lots of exceptions and use valgrind checking for memory leaks

    ciel::split_buffer<NothrowMoveStruct> v;
    can_throw = true;

    for (size_t i = 0; i < 1000; ++i) {
        // Use random numbers to insert or erase at any position in v: v.begin() + g() % ciel::max<size_t>(v.size(), 1)

        BASIC_TEST_CASE(v.emplace_back(1));

        BASIC_TEST_CASE(v.assign(il));

        BASIC_TEST_CASE(v.resize(g() % (v.size() * 2 + 1), 5));

        BASIC_TEST_CASE(v.assign(10, 20));

        BASIC_TEST_CASE(v.emplace_front(2));

        BASIC_TEST_CASE(
            v.erase(v.begin() + g() % std::max<size_t>(v.size(), 1), v.begin() + g() % std::max<size_t>(v.size(), 1)));
    }
}

TEST(exception_safety_tests, list_strong) {
    // These list functions provide strong exception safety:
    // insert, emplace, push_back, emplace_back, push_front, emplace_front

    ciel::list<NothrowMoveStruct> v;
    ciel::list<NothrowMoveStruct> state_holder;

    for (size_t i = 0; i < 200; ++i) {
        STRONG_TEST_CASE(v.emplace_front(1));

        STRONG_TEST_CASE(v.emplace_back(2));

        STRONG_TEST_CASE(v.insert(v.end(), 10, 20));

        STRONG_TEST_CASE(v.insert(v.begin(), il));

        STRONG_TEST_CASE(v.emplace(v.end(), 3));
    }
}

TEST(exception_safety_tests, list_basic) {
    // Throw lots of exceptions and use valgrind checking for memory leaks

    ciel::list<NothrowMoveStruct> v;
    can_throw = true;

    for (size_t i = 0; i < 1000; ++i) {
        BASIC_TEST_CASE(v.assign(10, 20));

        BASIC_TEST_CASE(v.assign(il));

        BASIC_TEST_CASE(v.resize(g() % (v.size() * 2 + 1), 5));
    }
}

#undef STRONG_TEST_CASE
#undef BASIC_TEST_CASE

#endif // CIEL_HAS_EXCEPTIONS

#include "tools.h"
#include <gtest/gtest.h>

#include <ciel/shared_ptr.hpp>

#include <thread>
#include <vector>

TEST(shared_ptr_tests, default_constuctor) {
    const ciel::shared_ptr<int> s;
}

TEST(shared_ptr_tests, move_constructor) {
    ciel::shared_ptr<int> src(new int(1729));

    ASSERT_TRUE(src);
    ASSERT_EQ(*src, 1729);

    const ciel::shared_ptr<int> dest(std::move(src));

    ASSERT_FALSE(src);
    ASSERT_TRUE(dest);
    ASSERT_EQ(*dest, 1729);
}

TEST(shared_ptr_tests, move_assign) {
    ciel::shared_ptr<int> src(new int(123));
    ciel::shared_ptr<int> dest(new int(888));

    ASSERT_TRUE(src);
    ASSERT_EQ(*src, 123);

    ASSERT_TRUE(dest);
    ASSERT_EQ(*dest, 888);

    dest = std::move(src);

    ASSERT_FALSE(src);
    ASSERT_TRUE(dest);
    ASSERT_EQ(*dest, 123);
}

TEST(shared_ptr_tests, alias_move_constructor) {
    class Base {
    public:
        virtual std::string
        str() const noexcept {
            return "Base";
        }

    protected:
        ~Base() = default;

    }; // class Base

    class Derived final : public Base {
    public:
        virtual std::string
        str() const noexcept override {
            return "Derived";
        }

    }; // class Derived

    {
        ciel::shared_ptr<Derived> src{new Derived{}};

        ASSERT_TRUE(src);
        ASSERT_EQ(src->str(), "Derived");

        const ciel::shared_ptr<Base> dest{std::move(src)};

        ASSERT_FALSE(src);
        ASSERT_TRUE(dest);
        ASSERT_EQ(dest->str(), "Derived");
    }

    {
        // We shall not call the deleter on the base class pointer.
        const ciel::shared_ptr<Base> s1{new Derived{}};
        const ciel::shared_ptr<Base> s2 = ciel::make_shared<Derived>();
    }
}

TEST(shared_ptr_tests, make_shared) {
    const ciel::shared_ptr<int> p = ciel::make_shared<int>(42);

    ASSERT_EQ(*p, 42);
    ASSERT_EQ(p.use_count(), 1);
}

TEST(shared_ptr_tests, make_shared_non_trivial) {
    const ciel::shared_ptr<std::string> s = ciel::make_shared<std::string>(1000, 'b');

    ASSERT_EQ(*s, std::string(1000, 'b'));
    ASSERT_EQ(s.use_count(), 1);
}

TEST(shared_ptr_tests, custom_deleter) {
    int count = 0;

    {
        const ciel::shared_ptr<int> s{new int{123}, [&count](int*) {
                                          ++count;
                                      }};

        ASSERT_EQ(*s, 123);
        ASSERT_EQ(s.use_count(), 1);
    }

    ASSERT_EQ(count, 1);

    {
        const ciel::shared_ptr<int> s{nullptr, [&count](int*) {
                                          ++count;
                                      }};

        ASSERT_EQ(s.use_count(), 1);
    }

    ASSERT_EQ(count, 2);
}

TEST(shared_ptr_tests, concurrent_store_and_loads) {
    constexpr size_t threads_num    = 64;
    constexpr size_t operations_num = 10000;

    ciel::shared_ptr<size_t> s{new size_t{123}};
    SimpleLatch go{threads_num};

    std::vector<std::thread> consumers;
    consumers.reserve(threads_num);

    for (size_t i = 0; i < threads_num; ++i) {
        consumers.emplace_back([&s, &go] {
            go.arrive_and_wait();

            for (size_t j = 0; j < operations_num; ++j) {
                CIEL_UNUSED(ciel::shared_ptr<size_t>{s});
            }
        });
    }

    for (auto& t : consumers) {
        t.join();
    }

    ASSERT_EQ(s.use_count(), 1);
}

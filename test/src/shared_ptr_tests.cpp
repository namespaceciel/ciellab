#include <gtest/gtest.h>

#include <ciel/shared_ptr.hpp>

namespace {

class Base {
public:
    Base() noexcept = default;
    virtual ~Base() = default;

    virtual std::string str() const noexcept {
        return "Base";
    }

private:
    Base(const Base&);
    Base& operator=(const Base&);

};  // class Base

class Derived : public Base {
public:
    virtual std::string str() const noexcept {
        return "Derived";
    }

};  // class Derived

}   // namespace

TEST(shared_ptr_tests, default_constuctor) {
    ciel::shared_ptr<int> s;
}

TEST(shared_ptr_tests, move_constructor) {
    ciel::shared_ptr<int> src(new int(1729));

    ASSERT_TRUE(src);
    ASSERT_EQ(*src, 1729);

    ciel::shared_ptr<int> dest(std::move(src));
  
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
    ciel::shared_ptr<Derived> src(new Derived);

    ASSERT_TRUE(src);
    ASSERT_EQ(src->str(), "Derived");

    ciel::shared_ptr<Base> dest(std::move(src));

    ASSERT_FALSE(src);
    ASSERT_TRUE(dest);
    ASSERT_EQ(dest->str(), "Derived");
}

// TODO: make_shared is not implemented yet.
// TEST(shared_ptr_tests, make_shared) {
//     ciel::shared_ptr<int> p = ciel::make_shared<int>(42);
// }
//
// TEST(shared_ptr_tests, make_shared_non_trivial) {
//     ciel::shared_ptr<std::string> s = ciel::make_shared<std::string>(1000, 'b');
// }
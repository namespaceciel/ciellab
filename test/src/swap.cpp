#include <gtest/gtest.h>

#include <ciel/swap.hpp>

#include <tuple>
#include <utility>

using namespace ciel;

TEST(swap, issue_5) {
    /* Should not pick this version

    namespace std {
        template<class T>
            requires ciel::is_trivially_relocatable<T>::value
        void swap(T& a, T& b) noexcept {
            ciel::relocatable_swap(a, b);
        }
    } // namespace std
    */
    int x               = 1;
    int y               = 2;
    std::tuple<int&> tx = std::tie(x);
    std::tuple<int&> ty = std::tie(y);

    std::swap(tx, ty);

    ASSERT_EQ(x, 2);
    ASSERT_EQ(y, 1);
}

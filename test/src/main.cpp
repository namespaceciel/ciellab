#include <gtest/gtest.h>

#include <ciel/config.hpp>

#include <iostream>

int main(int argc, char** argv) {
    std::cout << "\n==================================================\n"
              << "CIEL_STD_VER: " << CIEL_STD_VER << '\n'
              << std::boolalpha << "CIEL_HAS_EXCEPTIONS: " <<
#ifdef CIEL_HAS_EXCEPTIONS
        true
#else
        false
#endif
              << '\n'
              << "CIEL_HAS_RTTI: " <<
#ifdef CIEL_HAS_RTTI
        true
#else
        false
#endif
              << '\n'
              << "==================================================\n\n";

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

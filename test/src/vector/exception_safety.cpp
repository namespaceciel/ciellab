#ifdef CIEL_HAS_EXCEPTIONS

#  include <gtest/gtest.h>

#  include <ciel/test/exception_generator.hpp>
#  include <ciel/vector.hpp>

#  include <cstddef>

using namespace ciel;

TEST(vector_exception_safety, push_back_in_capacity) {
    using EG    = ExceptionGenerator<1, DefaultConstructor | CopyConstructor, true>;
    EG::enabled = false;
    vector<EG> v;
    v.reserve(8);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }
    EG::enabled = true;

    try {
        EG::reset();

        ASSERT_TRUE(v.capacity() > v.size());
        v.emplace_back();

        ASSERT_TRUE(false);

    } catch (...) {
        ASSERT_EQ(v, std::initializer_list<size_t>({0, 1, 2, 3, 4}));
    }

    try {
        EG::reset();

        const EG eg;
        ASSERT_TRUE(v.capacity() > v.size());
        v.push_back(eg);

        ASSERT_TRUE(false);

    } catch (...) {
        ASSERT_EQ(v, std::initializer_list<size_t>({0, 1, 2, 3, 4}));
    }
}

TEST(vector_exception_safety, push_back_beyond_capacity_trivially_relocatable) {
    using EG = ExceptionGeneratorTriviallyRelocatable<1, DefaultConstructor | CopyConstructor, true>;
    static_assert(is_trivially_relocatable<EG>::value, "");
    EG::enabled = false;
    vector<EG> v;
    v.reserve(5);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }
    EG::enabled = true;

    try {
        EG::reset();

        ASSERT_TRUE(v.capacity() == v.size());
        v.emplace_back();

        ASSERT_TRUE(false);

    } catch (...) {
        ASSERT_EQ(v, std::initializer_list<size_t>({0, 1, 2, 3, 4}));
    }

    try {
        EG::reset();

        const EG eg;
        ASSERT_TRUE(v.capacity() == v.size());
        v.push_back(eg);

        ASSERT_TRUE(false);

    } catch (...) {
        ASSERT_EQ(v, std::initializer_list<size_t>({0, 1, 2, 3, 4}));
    }
}

TEST(vector_exception_safety, push_back_beyond_capacity_noexcept_move) {
    using EG = ExceptionGenerator<1, DefaultConstructor | CopyConstructor, true>;
    static_assert(not is_trivially_relocatable<EG>::value, "");
    EG::enabled = false;
    vector<EG> v;
    v.reserve(5);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }
    EG::enabled = true;

    try {
        EG::reset();

        ASSERT_TRUE(v.capacity() == v.size());
        v.emplace_back();

        ASSERT_TRUE(false);

    } catch (...) {
        ASSERT_EQ(v, std::initializer_list<size_t>({0, 1, 2, 3, 4}));
    }

    try {
        EG::reset();

        const EG eg;
        ASSERT_TRUE(v.capacity() == v.size());
        v.push_back(eg);

        ASSERT_TRUE(false);

    } catch (...) {
        ASSERT_EQ(v, std::initializer_list<size_t>({0, 1, 2, 3, 4}));
    }
}

TEST(vector_exception_safety, push_back_beyond_capacity_copy) {
    using EG = ExceptionGenerator<1, DefaultConstructor | CopyConstructor | MoveConstructor, false>;
    static_assert(not is_trivially_relocatable<EG>::value, "");
    EG::enabled = false;
    vector<EG> v;
    v.reserve(5);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }
    EG::enabled = true;

    try {
        EG::reset();

        ASSERT_TRUE(v.capacity() == v.size());
        v.emplace_back();

        ASSERT_TRUE(false);

    } catch (...) {
        ASSERT_EQ(v, std::initializer_list<size_t>({0, 1, 2, 3, 4}));
    }

    try {
        EG::reset();

        const EG eg;
        ASSERT_TRUE(v.capacity() == v.size());
        v.push_back(eg);

        ASSERT_TRUE(false);

    } catch (...) {
        ASSERT_EQ(v, std::initializer_list<size_t>({0, 1, 2, 3, 4}));
    }
}

TEST(vector_exception_safety, insert_in_capacity_N_gt_pos_end_dis) {
    using EG = ExceptionGenerator<3, CopyConstructor | CopyAssignment, true>;
    static_assert(not is_trivially_relocatable<EG>::value, "");
    EG::enabled = false;
    vector<EG> v;
    v.reserve(8);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }

    EG::enabled = true;

    try {
        const EG eg;
        EG::reset();

        ASSERT_TRUE(v.capacity() >= v.size() + 3);
        v.insert(v.end() - 1, 3, eg);

        ASSERT_TRUE(false);

    } catch (...) {}
}

TEST(vector_exception_safety, insert_in_capacity_N_lt_pos_end_dis) {
    using EG = ExceptionGenerator<3, CopyConstructor | CopyAssignment, true>;
    static_assert(not is_trivially_relocatable<EG>::value, "");
    EG::enabled = false;
    vector<EG> v;
    v.reserve(8);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }

    EG::enabled = true;

    try {
        const EG eg;
        EG::reset();

        ASSERT_TRUE(v.capacity() >= v.size() + 3);
        v.insert(v.begin() + 1, 3, eg);

        ASSERT_TRUE(false);

    } catch (...) {}
}

TEST(vector_exception_safety, insert_in_capacity_N_gt_pos_end_dis_trivially_relocatable) {
    using EG = ExceptionGeneratorTriviallyRelocatable<3, CopyConstructor | CopyAssignment, true>;
    static_assert(is_trivially_relocatable<EG>::value, "");
    EG::enabled = false;
    vector<EG> v;
    v.reserve(8);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }

    EG::enabled = true;

    try {
        const EG eg;
        EG::reset();

        ASSERT_TRUE(v.capacity() >= v.size() + 3);
        v.insert(v.end() - 1, 3, eg);

        ASSERT_TRUE(false);

    } catch (...) {}
}

TEST(vector_exception_safety, insert_in_capacity_N_lt_pos_end_dis_trivially_relocatable) {
    using EG = ExceptionGeneratorTriviallyRelocatable<3, CopyConstructor | CopyAssignment, true>;
    static_assert(is_trivially_relocatable<EG>::value, "");
    EG::enabled = false;
    vector<EG> v;
    v.reserve(8);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }

    EG::enabled = true;

    try {
        const EG eg;
        EG::reset();

        ASSERT_TRUE(v.capacity() >= v.size() + 3);
        v.insert(v.begin() + 1, 3, eg);

        ASSERT_TRUE(false);

    } catch (...) {}
}

TEST(vector_exception_safety, insert_in_capacity_one_at_end) {
    using EG    = ExceptionGenerator<1, CopyConstructor | CopyAssignment, true>;
    EG::enabled = false;
    vector<EG> v;
    v.reserve(6);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }

    EG::enabled = true;

    try {
        const EG eg;
        EG::reset();

        ASSERT_TRUE(v.capacity() >= v.size() + 1);
        v.insert(v.end(), eg);

        ASSERT_TRUE(false);

    } catch (...) {
        ASSERT_EQ(v, std::initializer_list<size_t>({0, 1, 2, 3, 4}));
    }
}

TEST(vector_exception_safety, insert_beyond_capacity_noexcept_move) {
    using EG = ExceptionGenerator<3, CopyConstructor | CopyAssignment, true>;
    static_assert(not is_trivially_relocatable<EG>::value, "");
    EG::enabled = false;
    vector<EG> v;
    v.reserve(5);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }

    EG::enabled = true;

    try {
        const EG eg;
        EG::reset();

        ASSERT_TRUE(v.capacity() < v.size() + 3);
        v.insert(v.begin() + 1, 3, eg);

        ASSERT_TRUE(false);

    } catch (...) {}
}

TEST(vector_exception_safety, insert_beyond_capacity_copy) {
    using EG = ExceptionGenerator<5, CopyConstructor | CopyAssignment, false>;
    static_assert(not is_trivially_relocatable<EG>::value, "");
    EG::enabled = false;
    vector<EG> v;
    v.reserve(5);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }

    EG::enabled = true;

    try {
        const EG eg;
        EG::reset();

        ASSERT_TRUE(v.capacity() < v.size() + 3);
        v.insert(v.begin() + 1, 3, eg);

        ASSERT_TRUE(false);

    } catch (...) {}
}

TEST(vector_exception_safety, insert_beyond_capacity_trivially_relocatable) {
    using EG = ExceptionGeneratorTriviallyRelocatable<3, CopyConstructor | CopyAssignment, true>;
    static_assert(is_trivially_relocatable<EG>::value, "");
    EG::enabled = false;
    vector<EG> v;
    v.reserve(5);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }

    EG::enabled = true;

    try {
        const EG eg;
        EG::reset();

        ASSERT_TRUE(v.capacity() < v.size() + 3);
        v.insert(v.end() - 1, 3, eg);

        ASSERT_TRUE(false);

    } catch (...) {}
}

TEST(vector_exception_safety, insert_beyond_capacity_one_at_end) {
    using EG    = ExceptionGenerator<1, CopyConstructor | CopyAssignment, true>;
    EG::enabled = false;
    vector<EG> v;
    v.reserve(5);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }

    EG::enabled = true;

    try {
        const EG eg;
        EG::reset();

        ASSERT_TRUE(v.capacity() < v.size() + 1);
        v.insert(v.end(), eg);

        ASSERT_TRUE(false);

    } catch (...) {
        ASSERT_EQ(v, std::initializer_list<size_t>({0, 1, 2, 3, 4}));
    }
}

TEST(vector_exception_safety, erase_N_gt_pos_end_dis) {
    using EG = ExceptionGenerator<3, MoveConstructor | MoveAssignment, false>;
    static_assert(not is_trivially_relocatable<EG>::value, "");
    EG::enabled = false;
    vector<EG> v;
    v.reserve(10);
    for (size_t i = 0; i < 10; ++i) {
        v.emplace_back(i);
    }

    EG::enabled = true;

    try {
        EG::reset();

        v.erase(v.end() - 7, v.end() - 3);

        ASSERT_TRUE(false);

    } catch (...) {}
}

TEST(vector_exception_safety, erase_N_lt_pos_end_dis) {
    using EG = ExceptionGenerator<3, MoveConstructor | MoveAssignment, false>;
    static_assert(not is_trivially_relocatable<EG>::value, "");
    EG::enabled = false;
    vector<EG> v;
    v.reserve(10);
    for (size_t i = 0; i < 10; ++i) {
        v.emplace_back(i);
    }

    EG::enabled = true;

    try {
        EG::reset();

        v.erase(v.end() - 4, v.end() - 3);

        ASSERT_TRUE(false);

    } catch (...) {}
}

TEST(vector_exception_safety, erase_trivially_relocatable) {
    using EG = ExceptionGeneratorTriviallyRelocatable<
        1, DefaultConstructor | CopyConstructor | CopyAssignment | MoveConstructor | MoveAssignment, false>;
    static_assert(is_trivially_relocatable<EG>::value, "");
    EG::enabled = false;
    vector<EG> v;
    v.reserve(10);
    for (size_t i = 0; i < 10; ++i) {
        v.emplace_back(i);
    }

    EG::enabled = true;

    try {
        EG::reset();

        v.erase(v.begin() + 1, v.begin() + 4);

    } catch (...) {
        ASSERT_TRUE(false);
    }
}

TEST(vector_exception_safety, assign_N_lt_size) {
    using EG    = ExceptionGenerator<3, CopyConstructor | CopyAssignment, true>;
    EG::enabled = false;
    vector<EG> v;
    v.reserve(5);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }

    EG::enabled = true;

    try {
        const EG eg;
        EG::reset();

        v.assign(3, eg);

        ASSERT_TRUE(false);

    } catch (...) {}
}

TEST(vector_exception_safety, assign_N_gt_size_lt_capacity) {
    using EG    = ExceptionGenerator<7, CopyConstructor | CopyAssignment, true>;
    EG::enabled = false;
    vector<EG> v;
    v.reserve(10);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }

    EG::enabled = true;

    try {
        const EG eg;
        EG::reset();

        v.assign(7, eg);

        ASSERT_TRUE(false);

    } catch (...) {}
}

TEST(vector_exception_safety, assign_N_gt_capacity) {
    using EG    = ExceptionGenerator<3, CopyConstructor | CopyAssignment, true>;
    EG::enabled = false;
    vector<EG> v;
    v.reserve(5);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }

    EG::enabled = true;

    try {
        const EG eg;
        EG::reset();

        v.assign(7, eg);

        ASSERT_TRUE(false);

    } catch (...) {}
}

TEST(vector_exception_safety, resize_N_lt_size) {
    using EG = ExceptionGeneratorTriviallyRelocatable<
        1, DefaultConstructor | CopyConstructor | CopyAssignment | MoveConstructor | MoveAssignment, false>;
    EG::enabled = false;
    vector<EG> v;
    v.reserve(5);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }

    const EG eg;
    EG::enabled = true;

    try {
        EG::reset();

        v.resize(3, eg);

    } catch (...) {
        ASSERT_TRUE(false);
    }
}

TEST(vector_exception_safety, resize_N_gt_size_lt_capacity) {
    using EG    = ExceptionGenerator<2, CopyConstructor | CopyAssignment, true>;
    EG::enabled = false;
    vector<EG> v;
    v.reserve(10);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }

    EG::enabled = true;

    try {
        const EG eg;
        EG::reset();

        v.resize(7, eg);

        ASSERT_TRUE(false);

    } catch (...) {}
}

TEST(vector_exception_safety, resize_N_gt_capacity) {
    using EG    = ExceptionGenerator<2, CopyConstructor | CopyAssignment, true>;
    EG::enabled = false;
    vector<EG> v;
    v.reserve(5);
    for (size_t i = 0; i < 5; ++i) {
        v.emplace_back(i);
    }

    EG::enabled = true;

    try {
        const EG eg;
        EG::reset();

        v.resize(7, eg);

        ASSERT_TRUE(false);

    } catch (...) {}
}

#endif

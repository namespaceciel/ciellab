#ifndef CIELLAB_TEST_TOOLS_H_
#define CIELLAB_TEST_TOOLS_H_

#include <condition_variable>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>

#include <ciel/config.hpp>
#include <ciel/move_proxy.hpp>
#include <ciel/type_traits.hpp>

struct ConstructAndAssignCounter {
    // To not be considered as trivially_relocatable.
    char padding_{};

    static size_t copy_;
    static size_t move_;

    static void
    reset() noexcept;
    static size_t
    copy() noexcept;
    static size_t
    move() noexcept;

    ConstructAndAssignCounter() noexcept = default;

    ConstructAndAssignCounter(const ConstructAndAssignCounter&) noexcept {
        ++copy_;
    }

    ConstructAndAssignCounter(ConstructAndAssignCounter&&) noexcept {
        ++move_;
    }

    ConstructAndAssignCounter&
    operator=(const ConstructAndAssignCounter&) noexcept {
        ++copy_;
        return *this;
    }

    ConstructAndAssignCounter&
    operator=(ConstructAndAssignCounter&&) noexcept {
        ++move_;
        return *this;
    }

}; // struct ConstructAndAssignCounter

static_assert(not ciel::is_trivially_relocatable<ConstructAndAssignCounter>::value, "");

struct MoveProxyTestClass {
    using value_type = ConstructAndAssignCounter;

    template<class InitializerList,
             typename std::enable_if<std::is_same<InitializerList, std::initializer_list<value_type>>::value, int>::type
             = 0>
    MoveProxyTestClass&
    operator=(InitializerList il) noexcept {
        for (auto&& t : il) {
            CIEL_UNUSED(value_type{std::forward<decltype(t)>(t)});
        }

        return *this;
    }

    template<class U = value_type, typename std::enable_if<ciel::worth_move<U>::value, int>::type = 0>
    MoveProxyTestClass&
    operator=(std::initializer_list<ciel::move_proxy<value_type>> il) noexcept {
        for (auto&& t : il) {
            CIEL_UNUSED(value_type{std::forward<decltype(t)>(t)});
        }

        return *this;
    }

    template<class U = value_type, typename std::enable_if<!ciel::worth_move<U>::value, int>::type = 0>
    MoveProxyTestClass&
    operator=(std::initializer_list<value_type> il) noexcept {
        for (auto&& t : il) {
            CIEL_UNUSED(value_type{std::forward<decltype(t)>(t)});
        }

        return *this;
    }

}; // struct MoveProxyTestClass

class SimpleLatch {
public:
    SimpleLatch(const size_t count_down) noexcept
        : count_down_(count_down) {}

    void
    arrive_and_wait() noexcept {
        std::unique_lock<std::mutex> lock(mutex_);

        if (--count_down_ == 0) {
            cv_.notify_all();

        } else {
            cv_.wait(lock);
        }
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    size_t count_down_;

}; // class SimpleLatch

template<class T, size_t Size, size_t Alignment>
struct AlignedAllocator {
    using value_type                             = T;
    using size_type                              = size_t;
    using difference_type                        = ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;

    static_assert(alignof(value_type) <= ciel::max_align, "");

    alignas(Alignment) unsigned char buf[Size]{};

    AlignedAllocator() noexcept = default;

    AlignedAllocator(const AlignedAllocator&) noexcept = default;

    AlignedAllocator(AlignedAllocator&& other) noexcept {
        other.buf[0] = 'x';
    }

    // clang-format off
    AlignedAllocator& operator=(const AlignedAllocator&) noexcept = default;
    // clang-format on

    AlignedAllocator&
    operator=(AlignedAllocator&& other) noexcept {
        other.buf[0] = 'x';
        return *this;
    }

    template<class U>
    struct rebind {
        using other = AlignedAllocator<U, Size, Alignment>;
    };

    value_type*
    allocate(const size_t n) {
        return static_cast<value_type*>(::operator new(n * sizeof(value_type)));
    }

    void
    deallocate(value_type* p, size_t) noexcept {
        ::operator delete(p);
    }

}; // struct SimpleAllocator

class HeapMemoryListNode {
private:
    HeapMemoryListNode* next{this};
    HeapMemoryListNode* prev{this};

public:
    size_t size{0};

    static HeapMemoryListNode dummy_head;
    static std::mutex mutex;

    void
    push() noexcept {
        CIEL_PRECONDITION(this != &dummy_head);
        CIEL_PRECONDITION(size != 0);

        std::lock_guard<std::mutex> lg(mutex);

        prev                  = &dummy_head;
        next                  = dummy_head.next;
        dummy_head.next->prev = this;
        dummy_head.next       = this;
    }

    void
    pop() noexcept {
        CIEL_PRECONDITION(this != &dummy_head);
        CIEL_PRECONDITION(size != 0);

        std::lock_guard<std::mutex> lg(mutex);

        next->prev = prev;
        prev->next = next;
    }

    ~HeapMemoryListNode() {
        CIEL_PRECONDITION(this == &dummy_head);
        CIEL_PRECONDITION(size == 0);

        HeapMemoryListNode* node = next;
        while (node != this) {
            std::cerr << "Error: " << node->size << " bytes leaked.\n";

            node = node->next;
        }
    }

}; // class HeapMemoryListNode

CIEL_NODISCARD void*
operator new(const size_t count);

CIEL_NODISCARD void*
operator new[](const size_t count);

void
operator delete(void* ptr) noexcept;

void
operator delete[](void* ptr) noexcept;

#if CIEL_STD_VER >= 14
void
operator delete(void* ptr, size_t sz) noexcept;

void
operator delete[](void* ptr, size_t sz) noexcept;
#endif

enum ExceptionValidOn {
    DefaultConstructor = 1,
    CopyConstructor    = 1 << 1,
    MoveConstructor    = 1 << 2,
    CopyAssignment     = 1 << 3,
    MoveAssignment     = 1 << 4
};

#ifdef CIEL_HAS_EXCEPTIONS
// ExceptionGenerator
#define ExceptionGeneratorDefinition(ExceptionGeneratorName)                                                     \
    template<size_t ThrowOn, size_t ValidOn, bool NoexceptMove>                                                  \
    class ExceptionGeneratorName {                                                                               \
        static_assert(ValidOn < (1 << 5), "");                                                                   \
                                                                                                                 \
        static constexpr bool ValidOnDefaultConstructor = (ValidOn & DefaultConstructor) != 0;                   \
        static constexpr bool ValidOnCopyConstructor    = (ValidOn & CopyConstructor) != 0;                      \
        static constexpr bool ValidOnMoveConstructor    = (ValidOn & MoveConstructor) != 0;                      \
        static constexpr bool ValidOnCopyAssignment     = (ValidOn & CopyAssignment) != 0;                       \
        static constexpr bool ValidOnMoveAssignment     = (ValidOn & MoveAssignment) != 0;                       \
                                                                                                                 \
        static_assert(!ValidOnMoveConstructor || !NoexceptMove, "");                                             \
        static_assert(!ValidOnMoveAssignment || !NoexceptMove, "");                                              \
                                                                                                                 \
        static size_t counter;                                                                                   \
                                                                                                                 \
        size_t* ptr{nullptr};                                                                                    \
                                                                                                                 \
    public:                                                                                                      \
        static bool enabled;                                                                                     \
                                                                                                                 \
        static void                                                                                              \
        reset() noexcept {                                                                                       \
            counter = 0;                                                                                         \
        }                                                                                                        \
                                                                                                                 \
        static void                                                                                              \
        throw_exception() {                                                                                      \
            reset();                                                                                             \
            throw 0;                                                                                             \
        }                                                                                                        \
                                                                                                                 \
        ExceptionGeneratorName(size_t i = 0) {                                                                   \
            if (ValidOnDefaultConstructor && enabled) {                                                          \
                if (++counter == ThrowOn) {                                                                      \
                    throw_exception();                                                                           \
                }                                                                                                \
            }                                                                                                    \
                                                                                                                 \
            ptr = new size_t{i};                                                                                 \
        }                                                                                                        \
                                                                                                                 \
        ExceptionGeneratorName(const ExceptionGeneratorName& other) {                                            \
            if (ValidOnCopyConstructor && enabled) {                                                             \
                if (++counter == ThrowOn) {                                                                      \
                    throw_exception();                                                                           \
                }                                                                                                \
            }                                                                                                    \
                                                                                                                 \
            ptr = new size_t{static_cast<size_t>(other)};                                                        \
        }                                                                                                        \
                                                                                                                 \
        ExceptionGeneratorName(ExceptionGeneratorName&& other) noexcept(NoexceptMove) {                          \
            if (ValidOnMoveConstructor && !NoexceptMove && enabled) {                                            \
                if (++counter == ThrowOn) {                                                                      \
                    throw_exception();                                                                           \
                }                                                                                                \
            }                                                                                                    \
                                                                                                                 \
            ptr = ciel::exchange(other.ptr, nullptr);                                                            \
        }                                                                                                        \
                                                                                                                 \
        ExceptionGeneratorName&                                                                                  \
        operator=(const ExceptionGeneratorName& other) {                                                         \
            if (ValidOnCopyAssignment && enabled) {                                                              \
                if (++counter == ThrowOn) {                                                                      \
                    throw_exception();                                                                           \
                }                                                                                                \
            }                                                                                                    \
                                                                                                                 \
            delete ptr;                                                                                          \
            ptr = new size_t{static_cast<size_t>(other)};                                                        \
            return *this;                                                                                        \
        }                                                                                                        \
                                                                                                                 \
        ExceptionGeneratorName&                                                                                  \
        operator=(ExceptionGeneratorName&& other) noexcept(NoexceptMove) {                                       \
            if (ValidOnMoveAssignment && !NoexceptMove && enabled) {                                             \
                if (++counter == ThrowOn) {                                                                      \
                    throw_exception();                                                                           \
                }                                                                                                \
            }                                                                                                    \
                                                                                                                 \
            delete ptr;                                                                                          \
            ptr = ciel::exchange(other.ptr, nullptr);                                                            \
            return *this;                                                                                        \
        }                                                                                                        \
                                                                                                                 \
        ~ExceptionGeneratorName() {                                                                              \
            if (ptr) {                                                                                           \
                *ptr = -1;                                                                                       \
                delete ptr;                                                                                      \
            }                                                                                                    \
        }                                                                                                        \
                                                                                                                 \
        CIEL_NODISCARD explicit                                                                                  \
        operator size_t() const noexcept {                                                                       \
            return ptr ? *ptr : 0;                                                                               \
        }                                                                                                        \
    };                                                                                                           \
                                                                                                                 \
    template<size_t ThrowOn, size_t ValidOn, bool NoexceptMove>                                                  \
    size_t ExceptionGeneratorName<ThrowOn, ValidOn, NoexceptMove>::counter = 0;                                  \
    template<size_t ThrowOn, size_t ValidOn, bool NoexceptMove>                                                  \
    bool ExceptionGeneratorName<ThrowOn, ValidOn, NoexceptMove>::enabled = false;                                \
                                                                                                                 \
    template<size_t ThrowOn, size_t ValidOn, bool NoexceptMove>                                                  \
    CIEL_NODISCARD bool operator==(const ExceptionGeneratorName<ThrowOn, ValidOn, NoexceptMove>& lhs,            \
                                   const ExceptionGeneratorName<ThrowOn, ValidOn, NoexceptMove>& rhs) noexcept { \
        return static_cast<size_t>(lhs) == static_cast<size_t>(rhs);                                             \
    }                                                                                                            \
                                                                                                                 \
    template<size_t ThrowOn, size_t ValidOn, bool NoexceptMove>                                                  \
    CIEL_NODISCARD bool operator==(const ExceptionGeneratorName<ThrowOn, ValidOn, NoexceptMove>& lhs,            \
                                   const size_t rhs) noexcept {                                                  \
        return static_cast<size_t>(lhs) == rhs;                                                                  \
    }

ExceptionGeneratorDefinition(ExceptionGenerator);
ExceptionGeneratorDefinition(ExceptionGeneratorTriviallyRelocatable);

template<size_t ThrowOn, size_t ValidOn, bool NoexceptMove>
struct ciel::is_trivially_relocatable<ExceptionGeneratorTriviallyRelocatable<ThrowOn, ValidOn, NoexceptMove>>
    : std::true_type {};

// InputIterator
// simulate input_iterator using int array base
class InputIterator : public ciel::input_iterator_base<InputIterator> {
public:
    using difference_type   = ptrdiff_t;
    using value_type        = int;
    using pointer           = int*;
    using reference         = int&;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept  = std::input_iterator_tag;

private:
    int* ptr;

public:
    InputIterator(int* p) noexcept
        : ptr(p) {
        CIEL_PRECONDITION(p != nullptr);
    }

    void
    go_next() noexcept {
        CIEL_PRECONDITION(*ptr != -1);
        *ptr = -1;
        ++ptr;
    }

    int&
    operator*() const noexcept {
        CIEL_PRECONDITION(*ptr != -1);
        return *ptr;
    }

    int*
    operator->() const noexcept {
        CIEL_PRECONDITION(*ptr != -1);
        return ptr;
    }

    int*
    base() const noexcept {
        return ptr;
    }

    CIEL_NODISCARD friend bool
    operator==(const InputIterator& lhs, const InputIterator& rhs) noexcept {
        return lhs.base() == rhs.base();
    }

}; // class InputIterator

#endif // CIEL_HAS_EXCEPTIONS

#endif // CIELLAB_TEST_TOOLS_H_

#ifndef CIELLAB_INCLUDE_CIEL_MEMORY_LEAK_CHECK_HPP_
#define CIELLAB_INCLUDE_CIEL_MEMORY_LEAK_CHECK_HPP_

#include <ciel/config.hpp>
#include <ciel/memory.hpp>

#include <cstddef>
#include <iostream>
#include <mutex>

NAMESPACE_CIEL_BEGIN

class HeapMemoryListNode {
private:
    HeapMemoryListNode* next{this};
    HeapMemoryListNode* prev{this};

public:
    size_t size_{0};

    CIEL_NODISCARD static HeapMemoryListNode&
    dummy_head() noexcept {
        static HeapMemoryListNode instance;
        return instance;
    }

    CIEL_NODISCARD static std::mutex&
    mutex() noexcept {
        static std::mutex instance;
        return instance;
    }

    void
    push() noexcept {
        CIEL_PRECONDITION(this != &dummy_head());
        CIEL_PRECONDITION(size_ != 0);

        const std::lock_guard<std::mutex> lg(mutex());

        prev                    = &dummy_head();
        next                    = dummy_head().next;
        dummy_head().next->prev = this;
        dummy_head().next       = this;
    }

    void
    pop() noexcept {
        CIEL_PRECONDITION(this != &dummy_head());
        CIEL_PRECONDITION(size_ != 0);

        const std::lock_guard<std::mutex> lg(mutex());

        next->prev = prev;
        prev->next = next;
    }

    ~HeapMemoryListNode() {
        CIEL_PRECONDITION(this == &dummy_head());
        CIEL_PRECONDITION(size_ == 0);

        HeapMemoryListNode* node = next;
        while (node != this) {
            std::cerr << "Error: " << node->size_ << " bytes leaked.\n";

            node = node->next;
        }
    }

}; // class HeapMemoryListNode

NAMESPACE_CIEL_END

#if !(defined(__clang__) && defined(__linux__)) // linux clang is unhappy about this.

CIEL_NODISCARD void*
operator new(const size_t);
CIEL_NODISCARD void*
operator new[](const size_t);
void
operator delete(void*) noexcept;
void
operator delete[](void*) noexcept;
#  if CIEL_STD_VER >= 14
void
operator delete(void*, size_t) noexcept;
void
operator delete[](void*, size_t) noexcept;
#  endif

#endif

#endif // CIELLAB_INCLUDE_CIEL_MEMORY_LEAK_CHECK_HPP_

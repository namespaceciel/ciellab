#ifndef CIELLAB_INCLUDE_CIEL_MEMORY_LEAK_CHECK_HPP_
#define CIELLAB_INCLUDE_CIEL_MEMORY_LEAK_CHECK_HPP_

#include <ciel/config.hpp>

#include <cstddef>
#include <memory>
#include <mutex>
#include <new>

NAMESPACE_CIEL_BEGIN

class HeapMemoryListNode {
private:
    HeapMemoryListNode* next{this};
    HeapMemoryListNode* prev{this};

public:
    size_t size_{0};

    CIEL_NODISCARD static HeapMemoryListNode&
    dummy_head() noexcept {
        HeapMemoryListNode instance;
        return instance;
    }

    CIEL_NODISCARD static std::mutex&
    mutex() noexcept {
        std::mutex instance;
        return instance;
    }

    void
    push() noexcept {
        CIEL_PRECONDITION(this != &dummy_head());
        CIEL_PRECONDITION(size_ != 0);

        std::lock_guard<std::mutex> lg(mutex());

        prev                    = &dummy_head();
        next                    = dummy_head().next;
        dummy_head().next->prev = this;
        dummy_head().next       = this;
    }

    void
    pop() noexcept {
        CIEL_PRECONDITION(this != &dummy_head());
        CIEL_PRECONDITION(size_ != 0);

        std::lock_guard<std::mutex> lg(mutex());

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

#ifndef __clang__ // clang is unhappy about this.
CIEL_NODISCARD inline void*
operator new(const size_t count) {
    const size_t extra = ciel::align_up(sizeof(HeapMemoryListNode), ciel::max_align);
    CIEL_POSTCONDITION(extra >= ciel::max_align);
    CIEL_POSTCONDITION(extra % ciel::max_align == 0);

    void* ptr = std::malloc(count + extra);
    if (ptr == nullptr) {
        ciel::throw_exception(std::bad_alloc{});
    }

    HeapMemoryListNode* node = static_cast<HeapMemoryListNode*>(ptr);
    node->size_              = count;
    node->push();

    ptr = (void*)((uintptr_t)ptr + extra);

    CIEL_POSTCONDITION(ciel::is_aligned(ptr, ciel::max_align));
    return ptr;
}

CIEL_NODISCARD inline void*
operator new[](const size_t count) {
    return operator new(count);
}

inline void
operator delete(void* ptr) noexcept {
    if (ptr == nullptr) {
        return;
    }

    const size_t extra = ciel::align_up(sizeof(HeapMemoryListNode), ciel::max_align);
    CIEL_POSTCONDITION(extra >= ciel::max_align);
    CIEL_POSTCONDITION(extra % ciel::max_align == 0);

    ptr = (void*)((uintptr_t)ptr - extra);

    HeapMemoryListNode* node = static_cast<HeapMemoryListNode*>(ptr);
    node->pop();

    CIEL_POSTCONDITION(ciel::is_aligned(ptr, ciel::max_align));
    std::free(ptr);
}

inline void
operator delete[](void* ptr) noexcept {
    return operator delete(ptr);
}

#if CIEL_STD_VER >= 14
inline void
operator delete(void* ptr, size_t sz) noexcept {
    return operator delete(ptr);
}

inline void
operator delete[](void* ptr, size_t sz) noexcept {
    return operator delete(ptr);
}
#endif // if CIEL_STD_VER >= 14

#endif // ifndef __clang__

NAMESPACE_CIEL_END

#endif // CIELLAB_INCLUDE_CIEL_MEMORY_LEAK_CHECK_HPP_

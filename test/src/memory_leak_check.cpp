#include <ciel/test/memory_leak_check.hpp>

#include <new>

#if !(defined(__clang__) && defined(__linux__)) // linux clang is unhappy about this.

CIEL_NODISCARD void*
operator new(const size_t count) {
    const size_t extra = ciel::align_up(sizeof(ciel::HeapMemoryListNode), ciel::max_align);
    CIEL_POSTCONDITION(extra >= ciel::max_align);
    CIEL_POSTCONDITION(extra % ciel::max_align == 0);

    void* ptr = std::malloc(count + extra); // NOLINT(cppcoreguidelines-no-malloc)
    if (ptr == nullptr) {
        CIEL_THROW_EXCEPTION(std::bad_alloc{});
    }

    ciel::HeapMemoryListNode* node = static_cast<ciel::HeapMemoryListNode*>(ptr);
    node->size_                    = count;
    node->push();

    ptr = (void*)((uintptr_t)ptr + extra);

    CIEL_POSTCONDITION(ciel::is_aligned(ptr, ciel::max_align));
    return ptr;
}

CIEL_NODISCARD void*
operator new[](const size_t count) {
    return operator new(count);
}

void
operator delete(void* ptr) noexcept {
    if (ptr == nullptr) {
        return;
    }

    const size_t extra = ciel::align_up(sizeof(ciel::HeapMemoryListNode), ciel::max_align);
    CIEL_POSTCONDITION(extra >= ciel::max_align);
    CIEL_POSTCONDITION(extra % ciel::max_align == 0);

    ptr = (void*)((uintptr_t)ptr - extra);

    ciel::HeapMemoryListNode* node = static_cast<ciel::HeapMemoryListNode*>(ptr);
    node->pop();

    CIEL_POSTCONDITION(ciel::is_aligned(ptr, ciel::max_align));
    std::free(ptr); // NOLINT(cppcoreguidelines-no-malloc)
}

void
operator delete[](void* ptr) noexcept {
    return operator delete(ptr);
}

#  if CIEL_STD_VER >= 14
void
operator delete(void* ptr, size_t) noexcept {
    return operator delete(ptr);
}

void
operator delete[](void* ptr, size_t) noexcept {
    return operator delete(ptr);
}
#  endif

#endif

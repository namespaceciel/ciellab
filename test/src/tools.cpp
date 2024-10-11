#include "tools.h"

size_t ConstructAndAssignCounter::copy_ = 0;
size_t ConstructAndAssignCounter::move_ = 0;

void
ConstructAndAssignCounter::reset() noexcept {
    copy_ = 0;
    move_ = 0;
}

size_t
ConstructAndAssignCounter::copy() noexcept {
    const size_t res = copy_;
    copy_            = 0;
    return res;
}

size_t
ConstructAndAssignCounter::move() noexcept {
    const size_t res = move_;
    move_            = 0;
    return res;
}

HeapMemoryListNode HeapMemoryListNode::dummy_head;
std::mutex HeapMemoryListNode::mutex;

CIEL_NODISCARD void*
operator new(const size_t count) {
    const size_t extra = ciel::align_up(sizeof(HeapMemoryListNode), ciel::max_align);
    CIEL_POSTCONDITION(extra >= ciel::max_align);
    CIEL_POSTCONDITION(extra % ciel::max_align == 0);

    void* ptr = std::malloc(count + extra);
    if (ptr == nullptr) {
        ciel::throw_exception(std::bad_alloc{});
    }

    HeapMemoryListNode* node = static_cast<HeapMemoryListNode*>(ptr);
    node->size               = count;
    node->push();

    ptr = (void*)((uintptr_t)ptr + extra);

    CIEL_POSTCONDITION(ciel::is_aligned(ptr, ciel::max_align));
    return ptr;
}

void
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

    std::free(node);
}

CIEL_NODISCARD void*
operator new[](const size_t count) {
    return operator new(count);
}

void
operator delete[](void* ptr) noexcept {
    return operator delete(ptr);
}

#if CIEL_STD_VER >= 14
void
operator delete(void* ptr, size_t) noexcept {
    return operator delete(ptr);
}

void
operator delete[](void* ptr, size_t) noexcept {
    return operator delete(ptr);
}
#endif

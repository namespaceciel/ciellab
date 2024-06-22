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

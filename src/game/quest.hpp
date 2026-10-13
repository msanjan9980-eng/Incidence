#pragma once

#include "core/math.hpp"

namespace inc::game {

enum class QuestId : u8 {
    ReachWarden   = 0,
    DefeatWarden  = 1,
    HalfArchive   = 2,
    FullArchive   = 3,
    Count         = 4,
};

struct Quest {
    const char* title;
    const char* hint;
};

const Quest& quest_at(QuestId id);

struct QuestState {
    u16 completed = 0;

    [[nodiscard]] bool is_completed(QuestId id) const noexcept {
        return (completed & (1u << int(id))) != 0;
    }
    void complete(QuestId id) noexcept {
        completed |= u16(1u << int(id));
    }
    [[nodiscard]] int completed_count() const noexcept {
        int c = 0;
        for (int i = 0; i < int(QuestId::Count); ++i) {
            if (completed & (1u << i)) ++c;
        }
        return c;
    }
};

} // namespace inc::game
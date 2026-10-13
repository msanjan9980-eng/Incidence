#include "game/quest.hpp"

namespace inc::game {

static const Quest QUESTS[int(QuestId::Count)] = {
    { "REACH THE WARDEN",  "clear all nine rooms"     },
    { "DEFEAT THE WARDEN", "survive the core"         },
    { "HALF THE ARCHIVE",  "recover six of her logs"  },
    { "THE FULL ARCHIVE",  "recover every log"        },
};

const Quest& quest_at(QuestId id) {
    const int i = int(id);
    if (i < 0 || i >= int(QuestId::Count)) return QUESTS[0];
    return QUESTS[i];
}

} // namespace inc::game
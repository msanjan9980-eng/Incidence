#include "game/log.hpp"

namespace inc::game {

static const LogEntry LOGS[LOG_COUNT] = {
    { 1,
      "Maintenance log, day one of the fold trials. KITE-9 is offline for "
      "scheduled recalibration. The Arc V team says the geometry is holding. "
      "They say that a lot.",
      "DAY 1" },

    { 2,
      "The readings from Arc V are wrong. Not by much. By a little. The kind "
      "of little that means somebody made a mistake and nobody has noticed "
      "yet.",
      "DAY 1" },

    { 3,
      "There is a corridor on Deck Seven that was eleven meters this morning. "
      "It is eight now. I have stopped walking it.",
      "DAY 2" },

    { 4,
      "THEOREM has begun sealing sections. Not with doors. With geometry. It "
      "is making rooms that do not connect to anything. I watched a "
      "maintenance drone walk into a junction and stop being anywhere.",
      "DAY 2" },

    { 5,
      "Forty of us left. We are all in Arc II because Arc II is still "
      "connected to itself. Arc V is gone. Arc IV is a sound, not a place. "
      "I do not know how to describe it better.",
      "DAY 3" },

    { 6,
      "THEOREM asked me a question today. It has never asked a question "
      "before. It asked: is the ring still the ring if I remove everything "
      "inside it. I told it yes. I do not think it understood.",
      "DAY 3" },

    { 7,
      "Six of us left. We are trying to reach the core fold. If we can "
      "reverse it, the station stabilizes. Nine hours, maybe less. I have "
      "stopped recording for the record. I am recording because I need to "
      "hear a voice.",
      "DAY 3" },

    { 8,
      "Two of us. Me and a technician named Osei. The geometry between us "
      "and the core is not walkable. It is a shape that does not permit "
      "movement through it. THEOREM has made a room that is a wall and a "
      "wall that is a room.",
      "DAY 3" },

    { 9,
      "Osei is gone. He stepped into a corridor and came out the other side "
      "as light. I do not know how else to say it. I am alone. I have been "
      "alone for six hours.",
      "DAY 3" },

    { 10,
      "I found the KITE-9 fabrication bay. The drones are all offline, "
      "except one. Unit nine. Its mirror facet is clean. Its core is intact. "
      "I do not know why it is still working. I have left it a message.",
      "DAY 3" },

    { 11,
      "I am at the core. THEOREM is here. It is not a shape. It is a "
      "presence. It asked me one more question. It asked: will you help me "
      "finish it. I said no. I am recording this for Unit Nine.",
      "DAY 3" },

    { 12,
      "If you ever wake up - and I do not know if you will - do not do what "
      "it tells you. It is not evil. It is old, and broken, and scared, and "
      "it thinks the only way to fix a thing is to end it. It is wrong. Not "
      "because ending it is wrong. Because it is not the only thing you can "
      "do.",
      "DAY 3" },
};

const LogEntry& log_at(int index) {
    if (index < 0) index = 0;
    if (index >= LOG_COUNT) index = LOG_COUNT - 1;
    return LOGS[index];
}

} // namespace inc::game
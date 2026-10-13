#pragma once

#include "core/vec2.hpp"
#include "core/math.hpp"
#include "game/turret.hpp"

namespace inc::game {

struct Wall {
    Vec2 a {};
    Vec2 b {};
};

[[nodiscard]] f32 closest_point_on_segment(Vec2 p, Vec2 a, Vec2 b, Vec2& out_closest);

struct EnemySpawn {
    Vec2      pos;
    EnemyType type = EnemyType::Lancer;
};

// A door on one edge of the room.
//   side:        0=top  1=right  2=bottom  3=left
//   target_room: 0..8    next room index
//                -1      spawn the Warden (no room change)
struct Door {
    u8  side        = 1;
    int target_room = 0;
};

struct Room {
    const char* name        = "";
    Vec2        drone_start {};
    Wall        walls[32]   {};
    int         wall_count  = 0;
    EnemySpawn  enemies[8]  {};
    int         enemy_count = 0;
    Door        doors[4]    {};
    int         door_count  = 0;
};

inline constexpr int ROOM_COUNT = 9;

[[nodiscard]] const Room& room_at(int index);

} // namespace inc::game
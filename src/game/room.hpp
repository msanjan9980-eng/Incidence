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

struct Room {
    const char* name       = "";
    Vec2        drone_start {};
    Wall        walls[32]  {};
    int         wall_count = 0;
    EnemySpawn  enemies[8] {};
    int         enemy_count = 0;
};

// The campaign is nine rooms. Increase this as we add more to ROOMS[] in
// room.cpp; the HUD auto-scales.
inline constexpr int ROOM_COUNT = 9;

[[nodiscard]] const Room& room_at(int index);

} // namespace inc::game
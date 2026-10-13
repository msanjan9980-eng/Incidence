#pragma once

#include "core/vec2.hpp"
#include "core/math.hpp"

namespace inc::game {

// ---------------------------------------------------------------------------
// Wall — a straight line segment that bullets ricochet off.
//
// Walls are defined by two endpoints and treated as zero-thickness. Bullets
// collide when their center comes within `bullet.radius` of the segment.
// The outward normal used for reflection is computed at collision time,
// based on which side the bullet came from.
//
// These walls are the entire reason the room geometry is interesting.
// Without them, every reflect is a straight line. With them, a bullet can
// chain four times and arrive at a turret the player never even faced.
// ---------------------------------------------------------------------------
struct Wall {
    Vec2 a {};
    Vec2 b {};
};

// Distance from `p` to the closest point on segment [a, b], plus the point.
[[nodiscard]] f32 closest_point_on_segment(Vec2 p, Vec2 a, Vec2 b, Vec2& out_closest);

// ---------------------------------------------------------------------------
// Room — one encounter. Defines walls, turret spawns, and drone start.
// ---------------------------------------------------------------------------
struct Room {
    const char* name       = "";
    Vec2        drone_start{};
    Wall        walls[32]  {};
    int         wall_count = 0;
    Vec2        turrets[8] {};
    int         turret_count = 0;
};

// Number of authored rooms. Increase when adding more to ROOMS[] in room.cpp.
inline constexpr int ROOM_COUNT = 3;

// Returns the room at `index`, wrapping with modulo. Never returns null.
[[nodiscard]] const Room& room_at(int index);

} // namespace inc::game
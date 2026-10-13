#include "game/room.hpp"

namespace inc::game {

f32 closest_point_on_segment(Vec2 p, Vec2 a, Vec2 b, Vec2& out_closest) {
    const Vec2 ab = b - a;
    const f32  len_sq = ab.length_sq();
    if (len_sq < EPS_SQ) {
        out_closest = a;
        return (p - a).length();
    }
    f32 t = (p - a).dot(ab) / len_sq;
    t = clamp(t, 0.0f, 1.0f);
    out_closest = a + ab * t;
    return (p - out_closest).length();
}

namespace {

const Room ROOMS[ROOM_COUNT] = {
    // -----------------------------------------------------------------------
    // ROOM 0 — ARENA
    // Open box, three turrets: two Lancers + one Banker at the bottom.
    // Teaches the basic reflection, then introduces the persistent Banker.
    // -----------------------------------------------------------------------
    {
        .name        = "ARENA",
        .drone_start = { VIRTUAL_W * 0.5f, VIRTUAL_H * 0.5f },
        .enemies = {
            { {  300.0f,             260.0f }, EnemyType::Lancer },
            { {  VIRTUAL_W - 300.0f, 260.0f }, EnemyType::Lancer },
            { {  VIRTUAL_W * 0.5f,   VIRTUAL_H - 220.0f }, EnemyType::Banker },
        },
        .enemy_count = 3,
    },

    // -----------------------------------------------------------------------
    // ROOM 1 — CROSS
    // Plus-shaped obstacle; four enemies at the corners: two Lancers,
    // one Banker, one Lead. The Lead makes standing still lethal.
    // -----------------------------------------------------------------------
    {
        .name        = "CROSS",
        .drone_start = { VIRTUAL_W * 0.5f, VIRTUAL_H - 180.0f },
        .walls = {
            { {  720.0f, 540.0f }, { 1200.0f, 540.0f } },
            { {  960.0f, 380.0f }, {  960.0f, 700.0f } },
        },
        .wall_count = 2,
        .enemies = {
            { {  300.0f,             220.0f }, EnemyType::Lancer },
            { {  VIRTUAL_W - 300.0f, 220.0f }, EnemyType::Lead   },
            { {  300.0f,             860.0f }, EnemyType::Banker },
            { {  VIRTUAL_W - 300.0f, 860.0f }, EnemyType::Lancer },
        },
        .enemy_count = 4,
    },

    // -----------------------------------------------------------------------
    // ROOM 2 — CORRIDOR
    // Corridor gap with five enemies: two Lancers, one Banker, two Leads.
    // The Leads fire through the gap; the Bankers survive long enough
    // to ricochet inside the corridor and come out the other side.
    // -----------------------------------------------------------------------
    {
        .name        = "CORRIDOR",
        .drone_start = { VIRTUAL_W * 0.5f, VIRTUAL_H * 0.5f },
        .walls = {
            { {  200.0f, 320.0f }, {  760.0f, 320.0f } },
            { { 1160.0f, 320.0f }, { 1720.0f, 320.0f } },
            { {  200.0f, 760.0f }, {  760.0f, 760.0f } },
            { { 1160.0f, 760.0f }, { 1720.0f, 760.0f } },
        },
        .wall_count = 4,
        .enemies = {
            { {  400.0f,             540.0f }, EnemyType::Lancer },
            { {  VIRTUAL_W - 400.0f, 540.0f }, EnemyType::Lancer },
            { {  960.0f,             420.0f }, EnemyType::Banker },
            { {  960.0f,             660.0f }, EnemyType::Lead   },
            { {  960.0f,             180.0f }, EnemyType::Lead   },
        },
        .enemy_count = 5,
    },
};

} // anonymous namespace

const Room& room_at(int index) {
    if (index < 0) index = 0;
    return ROOMS[index % ROOM_COUNT];
}

} // namespace inc::game
#include "game/room.hpp"

namespace inc::game {

// ---------------------------------------------------------------------------
// Geometry helper: closest point on a segment.
//
// For a segment A->B and a point P:
//   1. Project P onto the infinite line: t = dot(P-A, B-A) / |B-A|^2.
//   2. Clamp t to [0,1] so the result stays on the segment.
//   3. Closest point is A + (B-A) * t.
//
// Returns the Euclidean distance from P to the segment.
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Hardcoded rooms. Add new entries to extend the campaign.
//
// Each room declares:
//   - a name (shown in debug output),
//   - where the drone starts,
//   - wall segments (interior geometry),
//   - turret spawn positions (up to 8).
//
// Wall placement rule: no wall should pass within ~70 px of drone_start, or
// the drone's shield arc will intersect the wall geometry at spawn.
// ---------------------------------------------------------------------------
namespace {

const Room ROOMS[ROOM_COUNT] = {
    // -----------------------------------------------------------------------
    // ROOM 0 — ARENA
    // An open box. No interior walls. Three turrets in a triangle.
    // This is the "tutorial" room that teaches shield aiming.
    // -----------------------------------------------------------------------
    {
        .name        = "ARENA",
        .drone_start = { VIRTUAL_W * 0.5f, VIRTUAL_H * 0.5f },
        .turrets = {
            {  300.0f,            260.0f            },
            {  VIRTUAL_W - 300.0f, 260.0f           },
            {  VIRTUAL_W * 0.5f,  VIRTUAL_H - 220.0f },
        },
        .turret_count = 3,
    },

    // -----------------------------------------------------------------------
    // ROOM 1 — CROSS
    // A plus-shaped obstacle in the middle of the arena. Four turrets at the
    // corners. Bullets must thread between the arms of the cross, and the
    // arms themselves serve as extra ricochet surfaces.
    // -----------------------------------------------------------------------
    {
        .name        = "CROSS",
        .drone_start = { VIRTUAL_W * 0.5f, VIRTUAL_H - 180.0f },
        .walls = {
            { {  720.0f, 540.0f }, { 1200.0f, 540.0f } },   // horizontal arm
            { {  960.0f, 380.0f }, {  960.0f, 700.0f } },   // vertical arm
        },
        .wall_count = 2,
        .turrets = {
            {  300.0f,             220.0f },
            {  VIRTUAL_W - 300.0f, 220.0f },
            {  300.0f,             860.0f },
            {  VIRTUAL_W - 300.0f, 860.0f },
        },
        .turret_count = 4,
    },

    // -----------------------------------------------------------------------
    // ROOM 2 — CORRIDOR
    // Two horizontal walls with a central gap form a wide corridor.
    // Five turrets: three inside the corridor, two in the upper corners
    // firing down through the gap.
    // -----------------------------------------------------------------------
    {
        .name        = "CORRIDOR",
        .drone_start = { VIRTUAL_W * 0.5f, VIRTUAL_H * 0.5f },
        .walls = {
            { {  200.0f, 320.0f }, {  760.0f, 320.0f } },   // top-left
            { { 1160.0f, 320.0f }, { 1720.0f, 320.0f } },   // top-right
            { {  200.0f, 760.0f }, {  760.0f, 760.0f } },   // bottom-left
            { { 1160.0f, 760.0f }, { 1720.0f, 760.0f } },   // bottom-right
        },
        .wall_count = 4,
        .turrets = {
            {  400.0f,             540.0f },
            {  VIRTUAL_W - 400.0f, 540.0f },
            {  960.0f,             420.0f },
            {  960.0f,             660.0f },
            {  960.0f,             180.0f },
        },
        .turret_count = 5,
    },
};

} // anonymous namespace

const Room& room_at(int index) {
    if (index < 0) index = 0;
    return ROOMS[index % ROOM_COUNT];
}

} // namespace inc::game
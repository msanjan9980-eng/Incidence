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

// ---------------------------------------------------------------------------
// The campaign. Nine rooms, escalating from a bare arena to a walled maze.
//
// Each room teaches exactly one geometric idea and then composes it with the
// previous ones. Wall placements are kept at least ~70 px from drone_start so
// the shield never clips into geometry on spawn.
//
// Convention: VIRTUAL_W = 1920, VIRTUAL_H = 1080.
// ---------------------------------------------------------------------------
const Room ROOMS[ROOM_COUNT] = {

    // =======================================================================
    // ROOM 0 — ARENA
    // The tutorial. No walls. Two Lancers up top, one Banker below.
    // The player learns reflection against an empty box.
    // =======================================================================
    {
        .name        = "ARENA",
        .drone_start = { 960.0f, 540.0f },
        .enemies = {
            { {  300.0f, 260.0f }, EnemyType::Lancer },
            { { 1620.0f, 260.0f }, EnemyType::Lancer },
            { {  960.0f, 860.0f }, EnemyType::Banker },
        },
        .enemy_count = 3,
    },

    // =======================================================================
    // ROOM 1 — CROSS
    // Plus-shaped obstacle. Corners swap roles mid-fight.
    // =======================================================================
    {
        .name        = "CROSS",
        .drone_start = { 960.0f, 900.0f },
        .walls = {
            { {  720.0f, 540.0f }, { 1200.0f, 540.0f } },
            { {  960.0f, 380.0f }, {  960.0f, 700.0f } },
        },
        .wall_count = 2,
        .enemies = {
            { {  300.0f, 220.0f }, EnemyType::Lancer },
            { { 1620.0f, 220.0f }, EnemyType::Lead   },
            { {  300.0f, 860.0f }, EnemyType::Banker },
            { { 1620.0f, 860.0f }, EnemyType::Lancer },
        },
        .enemy_count = 4,
    },

    // =======================================================================
    // ROOM 2 — CORRIDOR
    // Two-wall corridor with a central gap. Leads fire through the gap.
    // =======================================================================
    {
        .name        = "CORRIDOR",
        .drone_start = { 960.0f, 540.0f },
        .walls = {
            { {  200.0f, 320.0f }, {  760.0f, 320.0f } },
            { { 1160.0f, 320.0f }, { 1720.0f, 320.0f } },
            { {  200.0f, 760.0f }, {  760.0f, 760.0f } },
            { { 1160.0f, 760.0f }, { 1720.0f, 760.0f } },
        },
        .wall_count = 4,
        .enemies = {
            { {  400.0f, 540.0f }, EnemyType::Lancer },
            { { 1520.0f, 540.0f }, EnemyType::Lancer },
            { {  960.0f, 420.0f }, EnemyType::Banker },
            { {  960.0f, 660.0f }, EnemyType::Lead   },
            { {  960.0f, 180.0f }, EnemyType::Lead   },
        },
        .enemy_count = 5,
    },

    // =======================================================================
    // ROOM 3 — HOURGLASS
    // Two diagonals form a pinched X in the center. Every shot must thread
    // the throat of the hourglass or bounce off an arm.
    // =======================================================================
    {
        .name        = "HOURGLASS",
        .drone_start = { 960.0f, 900.0f },
        .walls = {
            { {  620.0f, 220.0f }, { 1000.0f, 540.0f } },   // top-left diagonal
            { { 1300.0f, 220.0f }, {  920.0f, 540.0f } },   // top-right diagonal
            { {  620.0f, 860.0f }, { 1000.0f, 540.0f } },   // bottom-left diagonal
            { { 1300.0f, 860.0f }, {  920.0f, 540.0f } },   // bottom-right diagonal
        },
        .wall_count = 4,
        .enemies = {
            { {  300.0f, 180.0f }, EnemyType::Lancer },
            { { 1620.0f, 180.0f }, EnemyType::Lancer },
            { {  300.0f, 900.0f }, EnemyType::Banker },
            { { 1620.0f, 900.0f }, EnemyType::Banker },
            { {  960.0f,  80.0f }, EnemyType::Lead   },
        },
        .enemy_count = 5,
    },

    // =======================================================================
    // ROOM 4 — CHOKE
    // A full-height wall splits the room in two. A single gap in the middle
    // is the only line of sight. The player must choose which side to clear.
    // =======================================================================
    {
        .name        = "CHOKE",
        .drone_start = { 960.0f, 540.0f },
        .walls = {
            { {  960.0f,   40.0f }, {  960.0f, 420.0f } },   // upper wall
            { {  960.0f,  660.0f }, {  960.0f, 1040.0f } },  // lower wall
        },
        .wall_count = 2,
        .enemies = {
            { {  340.0f, 220.0f }, EnemyType::Lancer },
            { {  340.0f, 860.0f }, EnemyType::Lead   },
            { { 1580.0f, 220.0f }, EnemyType::Banker },
            { { 1580.0f, 860.0f }, EnemyType::Lancer },
            { {  960.0f, 120.0f }, EnemyType::Lead   },
        },
        .enemy_count = 5,
    },

    // =======================================================================
    // ROOM 5 — SPIRAL
    // Four wall segments spiral toward the center. Bullets curve inward
    // with every bounce, and kills require reading a curving path.
    // =======================================================================
    {
        .name        = "SPIRAL",
        .drone_start = { 960.0f, 540.0f },
        .walls = {
            { {  480.0f, 200.0f }, {  960.0f, 200.0f } },   // top
            { { 1440.0f, 480.0f }, { 1440.0f, 880.0f } },   // right
            { {  960.0f, 880.0f }, {  480.0f, 880.0f } },   // bottom
            { {  480.0f, 880.0f }, {  480.0f, 200.0f } },   // left
        },
        .wall_count = 4,
        .enemies = {
            { {  220.0f,  90.0f }, EnemyType::Lancer },
            { { 1700.0f,  90.0f }, EnemyType::Lead   },
            { {  220.0f, 990.0f }, EnemyType::Lead   },
            { { 1700.0f, 990.0f }, EnemyType::Lancer },
            { {  960.0f,  90.0f }, EnemyType::Banker },
            { {  960.0f, 990.0f }, EnemyType::Banker },
        },
        .enemy_count = 6,
    },

    // =======================================================================
    // ROOM 6 — PINCH
    // Two walls angled inward from the top and bottom narrow the playable
    // space into a waist. Movement is constrained; reflection is everything.
    // =======================================================================
    {
        .name        = "PINCH",
        .drone_start = { 960.0f, 540.0f },
        .walls = {
            { {  200.0f, 220.0f }, {  760.0f, 480.0f } },   // upper-left
            { { 1720.0f, 220.0f }, { 1160.0f, 480.0f } },   // upper-right
            { {  200.0f, 860.0f }, {  760.0f, 600.0f } },   // lower-left
            { { 1720.0f, 860.0f }, { 1160.0f, 600.0f } },   // lower-right
        },
        .wall_count = 4,
        .enemies = {
            { {  960.0f,  100.0f }, EnemyType::Lead   },
            { {  960.0f,  980.0f }, EnemyType::Lead   },
            { {  200.0f, 540.0f },  EnemyType::Lancer },
            { { 1720.0f, 540.0f },  EnemyType::Lancer },
            { {  960.0f, 540.0f - 320.0f }, EnemyType::Banker },
        },
        .enemy_count = 5,
    },

    // =======================================================================
    // ROOM 7 — CROSSFIRE
    // No walls. Six enemies arranged in a hexagon. Every direction is hot.
    // The player must rotate constantly; standing still is fatal.
    // =======================================================================
    {
        .name        = "CROSSFIRE",
        .drone_start = { 960.0f, 540.0f },
        .enemies = {
            { {  960.0f,  100.0f }, EnemyType::Lead   },
            { {  960.0f,  980.0f }, EnemyType::Lead   },
            { {  260.0f,  320.0f }, EnemyType::Lancer },
            { { 1660.0f,  320.0f }, EnemyType::Lancer },
            { {  260.0f,  760.0f }, EnemyType::Banker },
            { { 1660.0f,  760.0f }, EnemyType::Banker },
        },
        .enemy_count = 6,
    },

    // =======================================================================
    // ROOM 8 — THE BOX
    // A closed square in the dead center. Enemies surround it. Bullets can
    // only reach the drone by bouncing through the space around the box.
    // The final room before the Warden.
    // =======================================================================
    {
        .name        = "THE BOX",
        .drone_start = { 960.0f, 940.0f },
        .walls = {
            { {  760.0f, 400.0f }, { 1160.0f, 400.0f } },   // top
            { { 1160.0f, 400.0f }, { 1160.0f, 680.0f } },   // right
            { { 1160.0f, 680.0f }, {  760.0f, 680.0f } },   // bottom
            { {  760.0f, 680.0f }, {  760.0f, 400.0f } },   // left
        },
        .wall_count = 4,
        .enemies = {
            { {  180.0f, 180.0f }, EnemyType::Lead   },
            { { 1740.0f, 180.0f }, EnemyType::Lead   },
            { {  180.0f, 900.0f }, EnemyType::Banker },
            { { 1740.0f, 900.0f }, EnemyType::Banker },
            { {  960.0f, 140.0f }, EnemyType::Lancer },
            { {  400.0f, 540.0f }, EnemyType::Lancer },
            { { 1520.0f, 540.0f }, EnemyType::Lancer },
        },
        .enemy_count = 7,
    },
};

} // anonymous namespace

const Room& room_at(int index) {
    if (index < 0) index = 0;
    return ROOMS[index % ROOM_COUNT];
}

} // namespace inc::game
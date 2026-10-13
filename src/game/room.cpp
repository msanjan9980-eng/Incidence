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

    // 0: ARENA
    {
        .name        = "ARENA",
        .drone_start = { 960.0f, 540.0f },
        .enemies = {
            { {  300.0f, 260.0f }, EnemyType::Lancer },
            { { 1620.0f, 260.0f }, EnemyType::Lancer },
            { {  960.0f, 860.0f }, EnemyType::Banker },
        },
        .enemy_count = 3,
        .doors = { { 1, 1 } },
        .door_count = 1,
    },

    // 1: CROSS
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
        .doors = { { 3, 0 }, { 1, 2 } },
        .door_count = 2,
    },

    // 2: CORRIDOR
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
        .doors = { { 3, 1 }, { 1, 3 } },
        .door_count = 2,
    },

    // 3: HOURGLASS
    {
        .name        = "HOURGLASS",
        .drone_start = { 960.0f, 900.0f },
        .walls = {
            { {  620.0f, 220.0f }, { 1000.0f, 540.0f } },
            { { 1300.0f, 220.0f }, {  920.0f, 540.0f } },
            { {  620.0f, 860.0f }, { 1000.0f, 540.0f } },
            { { 1300.0f, 860.0f }, {  920.0f, 540.0f } },
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
        .doors = { { 3, 2 }, { 1, 4 } },
        .door_count = 2,
    },

    // 4: CHOKE
    {
        .name        = "CHOKE",
        .drone_start = { 960.0f, 540.0f },
        .walls = {
            { {  960.0f,   40.0f }, {  960.0f, 420.0f } },
            { {  960.0f,  660.0f }, {  960.0f, 1040.0f } },
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
        .doors = { { 3, 3 }, { 1, 5 } },
        .door_count = 2,
    },

    // 5: SPIRAL
    {
        .name        = "SPIRAL",
        .drone_start = { 960.0f, 540.0f },
        .walls = {
            { {  480.0f, 200.0f }, {  960.0f, 200.0f } },
            { { 1440.0f, 480.0f }, { 1440.0f, 880.0f } },
            { {  960.0f, 880.0f }, {  480.0f, 880.0f } },
            { {  480.0f, 880.0f }, {  480.0f, 200.0f } },
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
        .doors = { { 3, 4 }, { 1, 6 } },
        .door_count = 2,
    },

    // 6: PINCH
    {
        .name        = "PINCH",
        .drone_start = { 960.0f, 540.0f },
        .walls = {
            { {  200.0f, 220.0f }, {  760.0f, 480.0f } },
            { { 1720.0f, 220.0f }, { 1160.0f, 480.0f } },
            { {  200.0f, 860.0f }, {  760.0f, 600.0f } },
            { { 1720.0f, 860.0f }, { 1160.0f, 600.0f } },
        },
        .wall_count = 4,
        .enemies = {
            { {  960.0f,  100.0f }, EnemyType::Lead   },
            { {  960.0f,  980.0f }, EnemyType::Lead   },
            { {  200.0f, 540.0f },  EnemyType::Lancer },
            { { 1720.0f, 540.0f },  EnemyType::Lancer },
            { {  960.0f, 220.0f },  EnemyType::Banker },
        },
        .enemy_count = 5,
        .doors = { { 3, 5 }, { 1, 7 } },
        .door_count = 2,
    },

    // 7: CROSSFIRE
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
        .doors = { { 3, 6 }, { 1, 8 } },
        .door_count = 2,
    },

    // 8: THE BOX
    {
        .name        = "THE BOX",
        .drone_start = { 960.0f, 940.0f },
        .walls = {
            { {  760.0f, 400.0f }, { 1160.0f, 400.0f } },
            { { 1160.0f, 400.0f }, { 1160.0f, 680.0f } },
            { { 1160.0f, 680.0f }, {  760.0f, 680.0f } },
            { {  760.0f, 680.0f }, {  760.0f, 400.0f } },
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
        .doors = { { 3, 7 }, { 1, -1 } },
        .door_count = 2,
    },
};

} // anonymous namespace

const Room& room_at(int index) {
    if (index < 0) index = 0;
    if (index >= ROOM_COUNT) index = ROOM_COUNT - 1;
    return ROOMS[index];
}

} // namespace inc::game
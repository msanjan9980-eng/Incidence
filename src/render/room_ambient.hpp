#pragma once

// =============================================================================
// room_ambient.hpp - per-sector background fields.
//
// Called from World::draw once per frame. Each room index gets its own
// composition of fx:: primitives drawn beneath the playfield.
// =============================================================================

#include <cmath>

#include "core/color.hpp"
#include "core/math.hpp"
#include "core/vec2.hpp"
#include "render/renderer.hpp"
#include "render/fx.hpp"
#include "render/field.hpp"

namespace inc::room_ambient {

inline void draw_ambient(render::Renderer& r, int room_index, f32 t) {
    r.set_layer(render::Renderer::Layer::Alpha);

    const f32 cx = f32(VIRTUAL_W) * 0.5f;
    const f32 cy = f32(VIRTUAL_H) * 0.5f;

    switch (room_index) {
    case 0:  // ARENA - warm dawn
        fx::perspective_floor(r, t, cx, f32(VIRTUAL_H) * 0.72f,
                              COLOR_ENEMY_EDGE, 0.32f, 12);
        fx::orbiting_cloud(r, {cx, cy}, t, COLOR_ENEMY_CORE, 0.30f, 32, 280.0f, 520.0f);
        break;
    case 1:  // CROSS - cool blue
        fx::hex_grid(r, t * 0.35f, COLOR_CORE_EDGE, 0.28f, 110.0f, 10, 18);
        fx::ripple_curtains(r, t, COLOR_CORE_EDGE, 0.22f, 16, 22.0f, 0.7f);
        break;
    case 2:  // constellation + drift
        fx::constellation(r, t * 0.5f, COLOR_CORE_INNER, 0.40f, 44, 6.0f);
        fx::orbiting_cloud(r, {cx, cy * 0.7f}, t, COLOR_CORE_EDGE, 0.30f, 26, 200.0f, 380.0f);
        break;
    case 3:  // interference field
        fx::interference(r, t, {cx - 200.0f, cy}, {cx + 200.0f, cy},
                         COLOR_FACET_EDGE, 0.40f, 50, 0.030f, 70.0f);
        break;
    case 4:  // cyan floor + curtains
        fx::perspective_floor(r, t * 1.1f, cx, f32(VIRTUAL_H) * 0.68f,
                              COLOR_CORE_EDGE, 0.30f, 14);
        fx::ripple_curtains(r, t * 0.8f, COLOR_CORE_INNER, 0.20f, 12, 18.0f, 0.9f);
        break;
    case 5:  // pink hex + orbiting
        fx::hex_grid(r, t * 0.5f, COLOR_ENEMY_EDGE, 0.22f, 90.0f, 12, 22);
        fx::orbiting_cloud(r, {cx, cy}, t, COLOR_ENEMY_CORE, 0.28f, 28, 240.0f, 460.0f);
        break;
    case 6:  // red rings + constellation
        fx::radial_rings(r, {cx, cy}, t, COLOR_ENEMY_CORE, 0.42f, 3, 700.0f);
        fx::constellation(r, t * 0.4f, COLOR_ENEMY_EDGE, 0.34f, 36, 5.0f);
        break;
    case 7:  // yellow curtains + cloud
        fx::ripple_curtains(r, t * 1.2f, COLOR_FACET_EDGE, 0.24f, 20, 26.0f, 1.1f);
        fx::orbiting_cloud(r, {cx, cy}, t * 0.9f, COLOR_FACET_EDGE, 0.30f, 30, 220.0f, 420.0f);
        break;
    case 8:  // violet interference + hex
        fx::interference(r, t, {cx, cy - 150.0f}, {cx, cy + 150.0f},
                         COLOR_CORE_EDGE, 0.36f, 44, 0.028f, 80.0f);
        fx::hex_grid(r, t * 0.25f, COLOR_CORE_INNER, 0.18f, 130.0f, 8, 14);
        break;
    case 9:  // WARDEN - maximum intensity
        fx::radial_rings(r, {cx, cy}, t * 1.3f, COLOR_ENEMY_CORE, 0.55f, 4, 900.0f);
        fx::interference(r, t, {cx - 250.0f, cy}, {cx + 250.0f, cy},
                         COLOR_ENEMY_EDGE, 0.45f, 36, 0.024f, 120.0f);
        fx::constellation(r, t * 0.6f, COLOR_ENEMY_CORE, 0.40f, 48, 7.0f);
        break;
    case 2:  // constellation + drift
        fx::constellation(r, t * 0.5f, COLOR_CORE_INNER, 0.40f, 44, 6.0f);
        fx::orbiting_cloud(r, {cx, cy * 0.7f}, t, COLOR_CORE_EDGE, 0.30f, 26, 200.0f, 380.0f);
        break;
    case 6:  // red rings + constellation
        fx::radial_rings(r, {cx, cy}, t, COLOR_ENEMY_CORE, 0.42f, 3, 700.0f);
        fx::constellation(r, t * 0.4f, COLOR_ENEMY_EDGE, 0.34f, 36, 5.0f);
        break;
    case 9:  // WARDEN - maximum intensity
        fx::radial_rings(r, {cx, cy}, t * 1.3f, COLOR_ENEMY_CORE, 0.55f, 4, 900.0f);
        fx::interference(r, t, {cx - 250.0f, cy}, {cx + 250.0f, cy},
                         COLOR_ENEMY_EDGE, 0.45f, 36, 0.024f, 120.0f);
        fx::constellation(r, t * 0.6f, COLOR_ENEMY_CORE, 0.40f, 48, 7.0f);
        break;    default:
        break;
    }
}

} // namespace inc::room_ambient
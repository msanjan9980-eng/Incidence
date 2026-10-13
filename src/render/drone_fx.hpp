#pragma once

// =============================================================================
// drone_fx.hpp - reticle / reflect-arc / dash overlays drawn around the drone.
// =============================================================================

#include <cmath>

#include "core/color.hpp"
#include "core/math.hpp"
#include "core/vec2.hpp"
#include "game/drone.hpp"
#include "game/character.hpp"
#include "render/renderer.hpp"

namespace inc::drone_fx {

// ---------------------------------------------------------------------------
// Reflect arc preview: shield span, facet separators, normal ticks, aim dot.
// ---------------------------------------------------------------------------
inline void draw_reflect_arc(render::Renderer& r, const game::Drone& d,
                             f32 t, Color edge, Color inner) {
    const Vec2 c = d.position;
    const f32 R_OUT = game::Drone::SHIELD_R_OUTER;
    const f32 half  = d.shield_span * 0.5f;

    // Faint background arc spanning the shield
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_arc_ring(c, R_OUT - 2.0f, R_OUT + 2.0f,
                    d.heading - half, d.heading + half,
                    with_alpha(edge, 100), 24);
    r.set_layer(render::Renderer::Layer::Alpha);

    // Facet separator ticks
    const int F = game::Drone::SHIELD_FACETS;
    const f32 step = d.shield_span / f32(F);
    for (int i = 0; i <= F; ++i) {
        const f32 a = d.heading - half + f32(i) * step;
        const Vec2 p0 {c.x + std::cos(a) * (R_OUT - 4.0f),
                       c.y + std::sin(a) * (R_OUT - 4.0f)};
        const Vec2 p1 {c.x + std::cos(a) * (R_OUT + 6.0f),
                       c.y + std::sin(a) * (R_OUT + 6.0f)};
        r.draw_line(p0, p1, 1.5f, with_alpha(inner, 200));
    }

    // Per-facet normal tick (mid angle)
    for (int i = 0; i < F; ++i) {
        const f32 mid = d.heading - half + (f32(i) + 0.5f) * step;
        const Vec2 nn {std::cos(mid), std::sin(mid)};
        const f32 pulse = 0.5f + 0.5f * std::sin(t * 3.0f + f32(i) * 1.3f);
        const f32 tl = 8.0f + 6.0f * pulse;
        r.draw_line({c.x + nn.x * (R_OUT + 4.0f), c.y + nn.y * (R_OUT + 4.0f)},
                    {c.x + nn.x * (R_OUT + 4.0f + tl), c.y + nn.y * (R_OUT + 4.0f + tl)},
                    1.5f, with_alpha(inner, u8(160 + 80 * pulse)));
    }

    // Forward aim dot
    const Vec2 fwd {std::cos(d.heading), std::sin(d.heading)};
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_circle({c.x + fwd.x * (R_OUT + 40.0f),
                   c.y + fwd.y * (R_OUT + 40.0f)},
                  6.0f, with_alpha(inner, 120), 12);
    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_circle({c.x + fwd.x * (R_OUT + 40.0f),
                   c.y + fwd.y * (R_OUT + 40.0f)},
                  2.5f, with_alpha(inner, 240), 10);
}

// ---------------------------------------------------------------------------
// Dash cooldown radial around the drone.
// ---------------------------------------------------------------------------
inline void draw_dash_meter(render::Renderer& r, const game::Drone& d,
                            const game::Character& ch, f32 t) {
    const f32 cd_max = ch.dash_cooldown();
    if (cd_max <= 0.0f) return;
    const f32 cd_remaining = clamp(d.dash_cd, 0.0f, cd_max);
    const f32 ready = 1.0f - cd_remaining / cd_max;

    const f32 R = game::Drone::SHIELD_R_OUTER + 18.0f;
    const Vec2 c = d.position;

    r.draw_ring(c, R - 2.0f, R, with_alpha(COLOR_CORE_EDGE, 60), 32);

    if (ready > 0.001f) {
        r.draw_arc_ring(c, R - 3.0f, R + 1.0f, -HALF_PI,
                        -HALF_PI + TAU * ready,
                        with_alpha(COLOR_CORE_INNER, 200), 24);
    }

    if (ready >= 0.999f) {
        const f32 pulse = 0.5f + 0.5f * std::sin(t * 4.0f);
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_circle(c, R + 6.0f + pulse * 4.0f,
                      with_alpha(COLOR_CORE_INNER, u8(40 * pulse)), 20);
        r.set_layer(render::Renderer::Layer::Alpha);
    }
}

// ---------------------------------------------------------------------------
// Composite - one call from World::draw.
// ---------------------------------------------------------------------------
inline void draw_reticle(render::Renderer& r, const game::Drone& d,
                         const game::Character& ch, f32 t) {
    draw_reflect_arc(r, d, t, COLOR_CORE_EDGE, COLOR_CORE_INNER);
    draw_dash_meter(r, d, ch, t);
}

} // namespace inc::drone_fx

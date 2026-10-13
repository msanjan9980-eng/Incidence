#pragma once

// =============================================================================
// field.hpp - animated background fields.
//
// Larger procedural effects that fill regions of the screen: hex grids,
// perspective floors, drifting constellations, wave interference patterns,
// orbiting clouds. All deterministic from frame time.
// =============================================================================

#include <cmath>
#include <cstdio>
#include <cstring>

#include "core/color.hpp"
#include "core/math.hpp"
#include "core/vec2.hpp"
#include "render/renderer.hpp"

namespace inc::fx {

[[nodiscard]] inline f32 fhash(u32 n) {
    n = (n << 13u) ^ n;
    n = n * (n * n * 15731u + 789221u) + 1376312589u;
    return f32(n & 0x7FFFFFFFu) / f32(0x7FFFFFFF);
}

// ---------------------------------------------------------------------------
// Animated hex grid. Cell size in px, brightness pulses along x.
// ---------------------------------------------------------------------------
inline void hex_grid(render::Renderer& r, f32 t, Color color, f32 alpha = 1.0f,
                     f32 cell = 60.0f, int rows = 20, int cols = 34) {
    r.set_layer(render::Renderer::Layer::Additive);
    const f32 h = cell * 0.8660254f;
    for (int j = -1; j <= rows; ++j) {
        for (int i = -1; i <= cols; ++i) {
            const f32 cx = f32(i) * cell * 0.75f;
            const f32 cy = f32(j) * h + ((i & 1) ? h * 0.5f : 0.0f);
            const f32 pulse = 0.5f + 0.5f * std::sin(t * 0.7f + f32(i + j) * 0.35f);
            const u8 a = u8(alpha * (6.0f + 14.0f * pulse));
            const f32 rr = cell * 0.36f;
            Vec2 v[6];
            for (int k = 0; k < 6; ++k) {
                const f32 ang = TAU * f32(k) / 6.0f + PI / 6.0f;
                v[k] = {cx + std::cos(ang) * rr, cy + std::sin(ang) * rr};
            }
            r.draw_quad(v[0], v[1], v[2], v[3], with_alpha(color, u8(a / 3u)));
            r.draw_quad(v[2], v[3], v[4], v[5], with_alpha(color, u8(a / 3u)));
            r.draw_line(v[0], v[1], 1.0f, with_alpha(color, a));
            r.draw_line(v[2], v[3], 1.0f, with_alpha(color, a));
            r.draw_line(v[4], v[5], 1.0f, with_alpha(color, a));
        }
    }
    r.set_layer(render::Renderer::Layer::Alpha);
}

// ---------------------------------------------------------------------------
// Perspective floor. Horizon at horizon_y, rows converge to a vanishing point.
// ---------------------------------------------------------------------------
inline void perspective_floor(render::Renderer& r, f32 t, f32 cx, f32 horizon_y,
                              Color color, f32 alpha = 1.0f, int depth_rows = 14) {
    const f32 bottom = f32(VIRTUAL_H);
    const f32 span = bottom - horizon_y;
    for (int i = 0; i < depth_rows; ++i) {
        const f32 k = f32(i) / f32(depth_rows - 1);
        const f32 row = horizon_y + span * (k * k);
        const f32 p = 0.5f + 0.5f * std::sin(t * 1.6f - k * 4.0f);
        const u8 a = u8(alpha * (30.0f + 40.0f * p) * (0.4f + 0.6f * k));
        r.draw_line({0, row}, {f32(VIRTUAL_W), row}, 1.0f, with_alpha(color, a));
    }
    const int cols = 24;
    for (int i = -cols; i <= cols; ++i) {
        const f32 k = f32(i) / f32(cols);
        const f32 x0 = cx + k * 120.0f;
        const f32 x1 = cx + k * 2400.0f;
        const u8 a = u8(alpha * (24.0f + 20.0f * std::sin(t * 0.9f + f32(i) * 0.7f)));
        r.draw_line({x0, horizon_y}, {x1, bottom}, 1.0f, with_alpha(color, a));
    }
    // Horizon glow
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_quad({0, horizon_y - 2.0f}, {f32(VIRTUAL_W), horizon_y - 2.0f},
                {f32(VIRTUAL_W), horizon_y + 2.0f}, {0, horizon_y + 2.0f},
                with_alpha(color, u8(alpha * 60.0f)));
    r.set_layer(render::Renderer::Layer::Alpha);
}

// ---------------------------------------------------------------------------
// Drifting constellation. Points connect to their 2 nearest neighbors.
// ---------------------------------------------------------------------------
inline void constellation(render::Renderer& r, f32 t, Color color,
                          f32 alpha = 1.0f, int count = 70, f32 speed = 6.0f) {
    constexpr int MAX = 96;
    if (count > MAX) count = MAX;
    Vec2 pts[MAX];
    for (int i = 0; i < count; ++i) {
        const u32 s = u32(i) * 2654435761u;
        const f32 base_x = fhash(s) * f32(VIRTUAL_W);
        const f32 base_y = fhash(s + 1u) * f32(VIRTUAL_H);
        const f32 dx = std::sin(t * 0.15f + fhash(s + 2u) * 6.28f) * speed;
        const f32 dy = std::cos(t * 0.13f + fhash(s + 3u) * 6.28f) * speed;
        pts[i] = {base_x + dx, base_y + dy};
    }
    for (int i = 0; i < count; ++i) {
        for (int j = i + 1; j < count; ++j) {
            const f32 dx = pts[i].x - pts[j].x;
            const f32 dy = pts[i].y - pts[j].y;
            const f32 d2 = dx * dx + dy * dy;
            if (d2 < 220.0f * 220.0f) {
                const f32 k = 1.0f - std::sqrt(d2) / 220.0f;
                r.draw_line(pts[i], pts[j], 1.0f,
                            with_alpha(color, u8(alpha * 40.0f * k)));
            }
        }
    }
    for (int i = 0; i < count; ++i) {
        const f32 tw = 0.5f + 0.5f * std::sin(t * 2.2f + f32(i) * 0.7f);
        r.draw_circle(pts[i], 1.5f + tw * 1.0f,
                      with_alpha(color, u8(alpha * (120.0f + 80.0f * tw))), 8);
    }
}

// ---------------------------------------------------------------------------
// Interference wave field. Two radial wavefronts cancel where out of phase.
// ---------------------------------------------------------------------------
inline void interference(render::Renderer& r, f32 t, Vec2 c1, Vec2 c2,
                         Color color, f32 alpha = 1.0f,
                         int step = 40, f32 freq = 0.035f, f32 speed = 90.0f) {
    for (int y = 0; y < int(VIRTUAL_H); y += step) {
        for (int x = 0; x < int(VIRTUAL_W); x += step) {
            const f32 dx1 = f32(x) - c1.x;
            const f32 dy1 = f32(y) - c1.y;
            const f32 dx2 = f32(x) - c2.x;
            const f32 dy2 = f32(y) - c2.y;
            const f32 d1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
            const f32 d2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
            const f32 w = std::sin(d1 * freq - t * speed * freq)
                        + std::sin(d2 * freq - t * speed * freq);
            const f32 a = (w + 2.0f) * 0.25f;
            if (a > 0.55f) {
                const u8 al = u8(alpha * (a - 0.55f) * 180.0f);
                r.draw_circle({f32(x), f32(y)}, 1.6f, with_alpha(color, al), 6);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Orbiting dust cloud around a point. Uses cheap pseudo-random polar offsets.
// ---------------------------------------------------------------------------
inline void orbiting_cloud(render::Renderer& r, Vec2 center, f32 t,
                           Color color, f32 alpha = 1.0f,
                           int count = 60, f32 r_min = 220.0f, f32 r_max = 460.0f) {
    for (int i = 0; i < count; ++i) {
        const u32 s = u32(i) * 40503u;
        const f32 ang = fhash(s) * TAU;
        const f32 rr  = r_min + (r_max - r_min) * fhash(s + 1u);
        const f32 w   = (fhash(s + 2u) - 0.5f) * 0.55f;
        const f32 a   = ang + t * w;
        const Vec2 p {center.x + std::cos(a) * rr, center.y + std::sin(a) * rr * 0.45f};
        const f32 tw  = 0.5f + 0.5f * std::sin(t * 1.7f + fhash(s + 3u) * 6.28f);
        const u8 al = u8(alpha * (30.0f + 70.0f * tw));
        r.draw_circle(p, 1.2f + tw * 1.4f, with_alpha(color, al), 6);
    }
}

// ---------------------------------------------------------------------------
// Vertical ripple curtains. Parallel waving vertical lines.
// ---------------------------------------------------------------------------
inline void ripple_curtains(render::Renderer& r, f32 t, Color color,
                            f32 alpha = 1.0f, int count = 24,
                            f32 amplitude = 20.0f, f32 phase_speed = 0.9f) {
    for (int i = 0; i < count; ++i) {
        const f32 u = (f32(i) + 0.5f) / f32(count);
        const f32 x_base = u * f32(VIRTUAL_W);
        const f32 y0 = 0.0f;
        const f32 y1 = f32(VIRTUAL_H);
        const int segs = 18;
        for (int k = 0; k < segs; ++k) {
            const f32 k0 = f32(k) / f32(segs);
            const f32 k1 = f32(k + 1) / f32(segs);
            const f32 x0 = x_base + std::sin(t * phase_speed + k0 * 5.0f + u * 3.0f) * amplitude;
            const f32 x1 = x_base + std::sin(t * phase_speed + k1 * 5.0f + u * 3.0f) * amplitude;
            const f32 yA = y0 + (y1 - y0) * k0;
            const f32 yB = y0 + (y1 - y0) * k1;
            const u8 al = u8(alpha * 22.0f);
            r.draw_line({x0, yA}, {x1, yB}, 1.0f, with_alpha(color, al));
        }
    }
}

} // namespace inc::fx

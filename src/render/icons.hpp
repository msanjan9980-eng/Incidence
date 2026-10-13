#pragma once

#include <cmath>

#include "core/color.hpp"
#include "core/math.hpp"
#include "core/vec2.hpp"
#include "render/renderer.hpp"

namespace inc::render {

// ============================================================================
// Procedural icon library - no textures, all drawn from primitives.
// All icons fit inside a box of side `size` centered at `c`.
// ============================================================================

inline void icon_heart(Renderer& r, Vec2 c, f32 size, Color color) {
    const f32 rc = size * 0.30f;
    const f32 xo = size * 0.26f;
    const f32 yo = -size * 0.10f;
    r.draw_circle({c.x - xo, c.y + yo}, rc, color, 14);
    r.draw_circle({c.x + xo, c.y + yo}, rc, color, 14);
    r.draw_triangle({c.x - size * 0.53f, c.y + yo + rc * 0.45f},
                    {c.x + size * 0.53f, c.y + yo + rc * 0.45f},
                    {c.x, c.y + size * 0.62f}, color);
}

inline void icon_bullet(Renderer& r, Vec2 c, f32 size, Color color) {
    const f32 h = size * 0.5f;
    const f32 w = size * 0.30f;
    r.draw_triangle({c.x, c.y - h}, {c.x - w, c.y}, {c.x + w, c.y}, color);
    r.draw_triangle({c.x, c.y + h}, {c.x - w, c.y}, {c.x + w, c.y}, color);
}

inline void icon_chevron(Renderer& r, Vec2 c, f32 size, f32 rot, Color color) {
    const f32 th = size * 0.18f;
    Vec2 p1 = {c.x - size * 0.35f, c.y - size * 0.35f};
    Vec2 p2 = {c.x + size * 0.35f, c.y};
    Vec2 p3 = {c.x - size * 0.35f, c.y + size * 0.35f};
    if (rot != 0.0f) {
        p1 = (p1 - c).rotated(rot) + c;
        p2 = (p2 - c).rotated(rot) + c;
        p3 = (p3 - c).rotated(rot) + c;
    }
    r.draw_line(p1, p2, th, color);
    r.draw_line(p2, p3, th, color);
}

inline void icon_hex(Renderer& r, Vec2 c, f32 rad, Color fill, Color border) {
    Vec2 pts[6];
    for (int i = 0; i < 6; ++i) {
        const f32 a = TAU * f32(i) / 6.0f - HALF_PI;
        pts[i] = c + Vec2::from_angle(a) * rad;
    }
    if (color_a(fill) > 0) {
        for (int i = 1; i < 5; ++i) r.draw_triangle(pts[0], pts[i], pts[i + 1], fill);
    }
    if (color_a(border) > 0) {
        for (int i = 0; i < 6; ++i) r.draw_line(pts[i], pts[(i + 1) % 6], 2.0f, border);
    }
}

inline void icon_lock(Renderer& r, Vec2 c, f32 size, Color color) {
    r.draw_arc_ring({c.x, c.y - size * 0.22f}, size * 0.28f, size * 0.40f,
                    PI, TAU, color, 12);
    const f32 bw = size * 0.42f;
    r.draw_quad({c.x - bw, c.y}, {c.x + bw, c.y},
                {c.x + bw, c.y + bw}, {c.x - bw, c.y + bw}, color);
}

inline void icon_key(Renderer& r, Vec2 c, f32 size, Color color) {
    const f32 bx = c.x - size * 0.32f;
    r.draw_circle({bx, c.y}, size * 0.26f, color, 14);
    r.draw_circle({bx, c.y}, size * 0.12f, COLOR_VOID, 10);
    r.draw_line({bx + size * 0.22f, c.y}, {c.x + size * 0.52f, c.y}, size * 0.14f, color);
    r.draw_line({c.x + size * 0.28f, c.y}, {c.x + size * 0.28f, c.y + size * 0.20f}, size * 0.08f, color);
    r.draw_line({c.x + size * 0.48f, c.y}, {c.x + size * 0.48f, c.y + size * 0.24f}, size * 0.08f, color);
}

inline void icon_star(Renderer& r, Vec2 c, f32 size, Color color) {
    const f32 s = size * 0.5f;
    r.draw_triangle({c.x, c.y - s}, {c.x - s * 0.35f, c.y}, {c.x + s * 0.35f, c.y}, color);
    r.draw_triangle({c.x, c.y + s}, {c.x - s * 0.35f, c.y}, {c.x + s * 0.35f, c.y}, color);
    r.draw_triangle({c.x - s, c.y}, {c.x, c.y - s * 0.35f}, {c.x, c.y + s * 0.35f}, color);
    r.draw_triangle({c.x + s, c.y}, {c.x, c.y - s * 0.35f}, {c.x, c.y + s * 0.35f}, color);
}

// ============================================================================
// Animated menu background - parallax starfield + station arc.
// Deterministic (no global state), driven by `time` and `tint`.
// ============================================================================

inline void draw_menu_background(Renderer& r, f32 time, Color tint, u8 alpha) {
    r.set_layer(Renderer::Layer::Alpha);

    r.draw_quad({0, 0}, {VIRTUAL_W, 0}, {VIRTUAL_W, VIRTUAL_H}, {0, VIRTUAL_H},
                rgba(3, 8, 14, 255));
    r.draw_quad({0, 0}, {VIRTUAL_W, 0},
                {VIRTUAL_W, VIRTUAL_H * 0.45f}, {0, VIRTUAL_H * 0.45f},
                rgba(6, 18, 28, 255));
    r.draw_quad({0, VIRTUAL_H * 0.55f}, {VIRTUAL_W, VIRTUAL_H * 0.55f},
                {VIRTUAL_W, VIRTUAL_H}, {0, VIRTUAL_H},
                rgba(2, 6, 12, 255));

    // Slow star layer
    constexpr int N_SLOW = 90;
    for (int i = 0; i < N_SLOW; ++i) {
        const f32 h1 = f32((i * 2654435761u) & 0xFFFFu) / 65535.0f;
        const f32 h2 = f32((i * 40503u)      & 0xFFFFu) / 65535.0f;
        const f32 h3 = f32((i * 12345u)      & 0xFFFFu) / 65535.0f;
        const f32 speed = 3.0f + h3 * 8.0f;
        f32 x = std::fmod(h1 * VIRTUAL_W - time * speed, VIRTUAL_W);
        if (x < 0.0f) x += VIRTUAL_W;
        const f32 y = h2 * VIRTUAL_H;
        const f32 tw = 0.5f + 0.5f * std::sin(time * (1.2f + h3 * 2.0f) + h1 * 6.0f);
        const u8 a = u8(f32(alpha) * (0.25f + 0.55f * tw));
        r.draw_circle({x, y}, 0.6f + h3 * 1.4f, with_alpha(tint, a), 6);
    }

    // Fast star layer
    constexpr int N_FAST = 40;
    for (int i = 0; i < N_FAST; ++i) {
        const f32 h1 = f32((i * 73856093u) & 0xFFFFu) / 65535.0f;
        const f32 h2 = f32((i * 19349663u) & 0xFFFFu) / 65535.0f;
        const f32 h3 = f32((i * 83492791u) & 0xFFFFu) / 65535.0f;
        const f32 speed = 20.0f + h3 * 25.0f;
        f32 x = std::fmod(h1 * VIRTUAL_W - time * speed, VIRTUAL_W);
        if (x < 0.0f) x += VIRTUAL_W;
        const f32 y = h2 * VIRTUAL_H;
        const f32 tw = 0.5f + 0.5f * std::sin(time * (2.0f + h3 * 3.0f) + h2 * 4.0f);
        const u8 a = u8(f32(alpha) * (0.45f + 0.55f * tw));
        r.draw_circle({x, y}, 1.0f + h3 * 1.6f, with_alpha(tint, a), 8);
    }

    // Station arc
    r.set_layer(Renderer::Layer::Additive);
    const f32 arc_pulse = 0.5f + 0.5f * std::sin(time * 0.5f);
    r.draw_arc_ring({VIRTUAL_W * 0.5f, VIRTUAL_H * 1.55f},
                    VIRTUAL_W * 0.44f, VIRTUAL_W * 0.46f,
                    -PI * 1.15f, -PI * 0.85f,
                    with_alpha(COLOR_CORE_EDGE, u8(30 + 30 * arc_pulse)), 64);
    r.set_layer(Renderer::Layer::Alpha);
}

} // namespace inc::render
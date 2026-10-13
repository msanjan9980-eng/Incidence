#pragma once

// =============================================================================
// fx.hpp - screen-space overlay effects.
//
// Everything here is CPU-side; each function is a thin wrapper over the
// existing renderer primitives (quad / line / circle / ring / arc_ring / text).
// No shader work required. Order matters: call scanlines + grain + vignette
// LAST so they sit on top of the composition.
// =============================================================================

#include <cmath>
#include <cstdio>
#include <cstring>

#include "core/color.hpp"
#include "core/math.hpp"
#include "core/vec2.hpp"
#include "render/renderer.hpp"

namespace inc::fx {

// ---------------------------------------------------------------------------
// Deterministic hashes. Stable across frames for a given seed.
// ---------------------------------------------------------------------------
[[nodiscard]] inline f32 hash01(u32 n) {
    n = (n << 13u) ^ n;
    n = n * (n * n * 15731u + 789221u) + 1376312589u;
    return f32(n & 0x7FFFFFFFu) / f32(0x7FFFFFFF);
}

[[nodiscard]] inline f32 hash11(u32 n, f32 lo, f32 hi) {
    return lo + (hi - lo) * hash01(n);
}

// ---------------------------------------------------------------------------
// CRT scanlines. Darkens every other band. Place near the very end.
// ---------------------------------------------------------------------------
inline void scanlines(render::Renderer& r, f32 alpha = 1.0f,
                      f32 step = 4.0f, u8 darkness = 60) {
    r.set_layer(render::Renderer::Layer::Alpha);
    for (f32 y = 0.0f; y < f32(VIRTUAL_H); y += step) {
        r.draw_quad({0, y}, {f32(VIRTUAL_W), y},
                    {f32(VIRTUAL_W), y + 1.5f}, {0, y + 1.5f},
                    rgba(0, 0, 0, u8(alpha * f32(darkness))));
    }
}

// ---------------------------------------------------------------------------
// Vignette. Darkens edges with a stepped gradient. strength ~ 0..255.
// ---------------------------------------------------------------------------
inline void vignette(render::Renderer& r, f32 alpha = 1.0f,
                     f32 strength = 180.0f, int bands = 12) {
    r.set_layer(render::Renderer::Layer::Alpha);
    const f32 max_w = f32(VIRTUAL_W) * 0.32f;
    const f32 max_h = f32(VIRTUAL_H) * 0.32f;
    for (int i = 0; i < bands; ++i) {
        const f32 t = f32(i) / f32(bands);
        const f32 a = u8(strength * (1.0f - t) * alpha / f32(bands) * 3.0f);
        const f32 w = max_w * (1.0f - t);
        const f32 h = max_h * (1.0f - t);
        // left
        r.draw_quad({0, 0}, {w, 0}, {w, f32(VIRTUAL_H)}, {0, f32(VIRTUAL_H)},
                    rgba(0, 0, 0, u8(a)));
        // right
        r.draw_quad({f32(VIRTUAL_W) - w, 0}, {f32(VIRTUAL_W), 0},
                    {f32(VIRTUAL_W), f32(VIRTUAL_H)},
                    {f32(VIRTUAL_W) - w, f32(VIRTUAL_H)}, rgba(0, 0, 0, u8(a)));
        // top
        r.draw_quad({0, 0}, {f32(VIRTUAL_W), 0},
                    {f32(VIRTUAL_W), h}, {0, h}, rgba(0, 0, 0, u8(a)));
        // bottom
        r.draw_quad({0, f32(VIRTUAL_H) - h}, {f32(VIRTUAL_W), f32(VIRTUAL_H) - h},
                    {f32(VIRTUAL_W), f32(VIRTUAL_H)}, {0, f32(VIRTUAL_H)},
                    rgba(0, 0, 0, u8(a)));
    }
}

// ---------------------------------------------------------------------------
// Film grain. Deterministic per-frame dots. count ~ 300..1500.
// ---------------------------------------------------------------------------
inline void film_grain(render::Renderer& r, f32 t, f32 alpha = 1.0f,
                       int count = 500, f32 size = 1.2f) {
    r.set_layer(render::Renderer::Layer::Additive);
    const u32 base = u32(t * 60.0f);
    for (int i = 0; i < count; ++i) {
        const u32 s = base * 2654435761u + u32(i) * 40503u;
        const f32 x = hash01(s + 1u) * f32(VIRTUAL_W);
        const f32 y = hash01(s + 7u) * f32(VIRTUAL_H);
        const u8 a = u8(alpha * hash11(s + 13u, 15.0f, 55.0f));
        r.draw_circle({x, y}, size, rgba(220, 230, 240, a), 4);
    }
    r.set_layer(render::Renderer::Layer::Alpha);
}

// ---------------------------------------------------------------------------
// Chromatic text. Draws the text three times with R/G/B offsets.
// ---------------------------------------------------------------------------
inline void chromatic_text_centered(render::Renderer& r, Vec2 c, const char* s,
                                    f32 size, f32 t, f32 alpha = 1.0f) {
    const f32 jitter = 1.0f + 0.5f * std::sin(t * 7.0f);
    const f32 dx = 2.4f * jitter;
    const f32 dy = 1.4f * jitter;
    r.draw_text_centered({c.x + dx, c.y + dy}, s, size,
                         rgba(255, 60, 90, u8(alpha * 70.0f)));
    r.draw_text_centered({c.x - dx, c.y - dy}, s, size,
                         rgba(60, 200, 255, u8(alpha * 70.0f)));
    r.draw_text_centered(c, s, size, rgba(230, 245, 255, u8(alpha * 255.0f)));
}

inline void chromatic_text(render::Renderer& r, Vec2 p, const char* s,
                           f32 size, f32 t, f32 alpha = 1.0f) {
    const f32 jitter = 1.0f + 0.5f * std::sin(t * 7.0f);
    const f32 dx = 2.2f * jitter;
    r.draw_text({p.x + dx, p.y}, s, size, rgba(255, 60, 90, u8(alpha * 70.0f)));
    r.draw_text({p.x - dx, p.y}, s, size, rgba(60, 200, 255, u8(alpha * 70.0f)));
    r.draw_text(p, s, size, rgba(230, 245, 255, u8(alpha * 255.0f)));
}

// ---------------------------------------------------------------------------
// Glitch slice. Cuts a bright horizontal band at a hashed y and offsets it.
// ---------------------------------------------------------------------------
inline void glitch_slice(render::Renderer& r, f32 t, f32 alpha = 1.0f) {
    const u32 frame = u32(t * 24.0f);
    if ((frame % 7u) != 0u) return;
    const f32 y = hash01(frame * 9973u) * f32(VIRTUAL_H);
    const f32 h = hash11(frame * 31u, 2.0f, 14.0f);
    const f32 dx = hash11(frame * 17u, -30.0f, 30.0f);
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_quad({dx, y}, {f32(VIRTUAL_W) + dx, y},
                {f32(VIRTUAL_W) + dx, y + h}, {dx, y + h},
                rgba(120, 220, 255, u8(alpha * 26.0f)));
    r.draw_quad({-dx, y + h * 0.5f}, {f32(VIRTUAL_W) - dx, y + h * 0.5f},
                {f32(VIRTUAL_W) - dx, y + h * 0.5f + 1.5f}, {-dx, y + h * 0.5f + 1.5f},
                rgba(255, 80, 120, u8(alpha * 40.0f)));
    r.set_layer(render::Renderer::Layer::Alpha);
}

// ---------------------------------------------------------------------------
// TV static bars. Short vertical noise streaks. Cheap "signal loss" hint.
// ---------------------------------------------------------------------------
inline void noise_bars(render::Renderer& r, f32 t, f32 alpha = 1.0f,
                       int count = 20) {
    r.set_layer(render::Renderer::Layer::Additive);
    const u32 base = u32(t * 45.0f);
    for (int i = 0; i < count; ++i) {
        const u32 s = base * 2246822519u + u32(i) * 3266489917u;
        const f32 x = hash01(s) * f32(VIRTUAL_W);
        const f32 y = hash01(s + 3u) * f32(VIRTUAL_H);
        const f32 h = hash11(s + 5u, 4.0f, 26.0f);
        r.draw_quad({x, y}, {x + 1.0f, y}, {x + 1.0f, y + h}, {x, y + h},
                    rgba(200, 220, 255, u8(alpha * 40.0f)));
    }
    r.set_layer(render::Renderer::Layer::Alpha);
}

// ---------------------------------------------------------------------------
// Bright horizontal glow line. Use for scanline wipes on panels / banners.
// ---------------------------------------------------------------------------
inline void glow_line_h(render::Renderer& r, f32 x0, f32 x1, f32 y,
                        f32 thickness, Color c, f32 alpha = 1.0f) {
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_quad({x0, y - thickness * 2.0f}, {x1, y - thickness * 2.0f},
                {x1, y + thickness * 2.0f}, {x0, y + thickness * 2.0f},
                with_alpha(c, u8(alpha * 26.0f)));
    r.draw_quad({x0, y - thickness}, {x1, y - thickness},
                {x1, y + thickness}, {x0, y + thickness},
                with_alpha(c, u8(alpha * 90.0f)));
    r.draw_line({x0, y}, {x1, y}, 1.0f, with_alpha(c, u8(alpha * 255.0f)));
    r.set_layer(render::Renderer::Layer::Alpha);
}

// ---------------------------------------------------------------------------
// Pulsing corner brackets. Sub-brackets draw in on hover.
// ---------------------------------------------------------------------------
inline void corner_brackets(render::Renderer& r, f32 x0, f32 y0, f32 x1, f32 y1,
                            f32 t, Color c, f32 alpha = 1.0f,
                            f32 arm = 22.0f, f32 thickness = 2.0f) {
    const f32 pulse = 0.5f + 0.5f * std::sin(t * 5.0f);
    const f32 len = arm + pulse * 4.0f;
    const Color cc = with_alpha(c, u8(alpha * 220.0f));
    const Color ch = with_alpha(c, u8(alpha * (200.0f + 55.0f * pulse)));
    // TL
    r.draw_line({x0, y0}, {x0 + len, y0}, thickness, cc);
    r.draw_line({x0, y0}, {x0, y0 + len}, thickness, cc);
    // TR
    r.draw_line({x1, y0}, {x1 - len, y0}, thickness, cc);
    r.draw_line({x1, y0}, {x1, y0 + len}, thickness, cc);
    // BL
    r.draw_line({x0, y1}, {x0 + len, y1}, thickness, cc);
    r.draw_line({x0, y1}, {x0, y1 - len}, thickness, cc);
    // BR
    r.draw_line({x1, y1}, {x1 - len, y1}, thickness, cc);
    r.draw_line({x1, y1}, {x1, y1 - len}, thickness, cc);
    // corner dots
    r.draw_circle({x0, y0}, 3.0f, ch, 10);
    r.draw_circle({x1, y0}, 3.0f, ch, 10);
    r.draw_circle({x0, y1}, 3.0f, ch, 10);
    r.draw_circle({x1, y1}, 3.0f, ch, 10);
}

// ---------------------------------------------------------------------------
// Radial pulse rings around a point.
// ---------------------------------------------------------------------------
inline void radial_rings(render::Renderer& r, Vec2 c, f32 t, Color color,
                         f32 alpha = 1.0f, int count = 3, f32 max_r = 400.0f) {
    for (int i = 0; i < count; ++i) {
        const f32 phase = f32(i) / f32(count);
        const f32 k = std::fmod(t * 0.35f + phase, 1.0f);
        const f32 radius = 40.0f + k * max_r;
        const f32 a = (1.0f - k) * alpha * 90.0f;
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_ring(c, radius - 1.5f, radius + 1.5f,
                    with_alpha(color, u8(a)), 64);
        r.set_layer(render::Renderer::Layer::Alpha);
    }
}

// ---------------------------------------------------------------------------
// Light shafts radiating down from a horizon point.
// ---------------------------------------------------------------------------
inline void light_shafts(render::Renderer& r, f32 cx, f32 horizon_y, f32 t,
                         Color color, f32 alpha = 1.0f, int count = 9) {
    r.set_layer(render::Renderer::Layer::Additive);
    for (int i = 0; i < count; ++i) {
        const f32 u = (f32(i) + 0.5f) / f32(count);
        const f32 ang = (u - 0.5f) * 1.35f + std::sin(t * 0.4f + u * 6.0f) * 0.05f;
        const f32 dx = std::sin(ang);
        const f32 len = f32(VIRTUAL_H) - horizon_y + 40.0f;
        const f32 x0 = cx + dx * 30.0f;
        const f32 x1 = cx + dx * (30.0f + len * 1.6f);
        const f32 w  = 8.0f + std::sin(t * 1.7f + u * 12.0f) * 3.0f;
        const u8 a = u8(alpha * (14.0f + 8.0f * std::sin(t * 2.2f + u * 9.0f)));
        r.draw_quad({x0 - w, horizon_y}, {x0 + w, horizon_y},
                    {x1 + w * 2.2f, horizon_y + len}, {x1 - w * 2.2f, horizon_y + len},
                    with_alpha(color, a));
    }
    r.set_layer(render::Renderer::Layer::Alpha);
}

// ---------------------------------------------------------------------------
// Data stream: columns of falling glyphs at screen edges.
// ---------------------------------------------------------------------------
inline void data_stream(render::Renderer& r, f32 t, f32 x, f32 width,
                        int rows, Color color, f32 alpha = 1.0f) {
    r.set_layer(render::Renderer::Layer::Additive);
    const char glyphs[] = "01<>/\\+*=#";
    const int glen = int(sizeof(glyphs) - 1);
    for (int i = 0; i < rows; ++i) {
        const u32 s = u32(i) * 2654435761u;
        const f32 speed = hash11(s, 40.0f, 140.0f);
        const f32 y = std::fmod(hash01(s + 1u) * f32(VIRTUAL_H) + t * speed,
                                f32(VIRTUAL_H));
        const f32 col = hash01(s + 3u);
        const f32 px = x + col * width;
        const int gi = int(hash01(s + 5u) * f32(glen));
        char buf[2] = { glyphs[gi], 0 };
        const u8 a = u8(alpha * (60.0f + 120.0f * hash01(s + 7u)));
        r.draw_text({px, y}, buf, 1.6f, with_alpha(color, a));
    }
    r.set_layer(render::Renderer::Layer::Alpha);
}

// ---------------------------------------------------------------------------
// Scrolling ticker of status text along a horizontal line.
// ---------------------------------------------------------------------------
inline void ticker(render::Renderer& r, f32 t, const char* text, f32 y,
                   f32 speed, Color color, f32 alpha = 1.0f) {
    const f32 w = r.text_width(text, 1.8f);
    if (w <= 0.0f) return;
    const f32 span = w + f32(VIRTUAL_W);
    const f32 x0 = f32(VIRTUAL_W) - std::fmod(t * speed, span);
    r.draw_text({x0, y}, text, 1.8f, with_alpha(color, u8(alpha * 200.0f)));
}

} // namespace inc::fx

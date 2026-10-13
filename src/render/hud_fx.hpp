#pragma once

// =============================================================================
// hud_fx.hpp - gameplay HUD primitives.
//
// Stateless drawing helpers: caller passes the current value, the helpers
// render the correct visual state. Nothing here owns data.
// =============================================================================

#include <cmath>
#include <cstdio>

#include "core/color.hpp"
#include "core/math.hpp"
#include "core/vec2.hpp"
#include "render/renderer.hpp"

namespace inc::hud {

// ---------------------------------------------------------------------------
// Labeled horizontal bar with animated fill + ghost trail + ticks + bevel.
// ---------------------------------------------------------------------------
inline void hud_bar(render::Renderer& r, Vec2 c, f32 w, f32 h,
                    const char* label, f32 frac, f32 t,
                    Color fill, Color edge, bool warn = false) {
    frac = clamp(frac, 0.0f, 1.0f);
    const f32 x0 = c.x - w * 0.5f;
    const f32 x1 = c.x + w * 0.5f;
    const f32 y0 = c.y - h * 0.5f;
    const f32 y1 = c.y + h * 0.5f;

    // Track
    r.draw_quad({x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}, rgba(4, 12, 20, 220));

    // Ghost trail — pulses toward the real fill
    const f32 ghost = clamp(frac + 0.06f * std::sin(t * 2.2f), 0.0f, 1.0f);
    if (ghost > frac) {
        r.draw_quad({x0, y0}, {x0 + w * ghost, y0},
                    {x0 + w * ghost, y1}, {x0, y1},
                    with_alpha(fill, u8(80.0f)));
    }

    // Fill
    const f32 fx = x0 + w * frac;
    if (frac > 0.0f) {
        r.draw_quad({x0, y0}, {fx, y0}, {fx, y1}, {x0, y1}, with_alpha(fill, 230));
    }

    // Warning overlay when low
    if (warn) {
        const f32 wp = 0.5f + 0.5f * std::sin(t * 7.0f);
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_quad({x0, y0}, {x1, y0}, {x1, y1}, {x0, y1},
                    with_alpha(COLOR_ENEMY_CORE, u8(40.0f + 40.0f * wp)));
        r.set_layer(render::Renderer::Layer::Alpha);
    }

    // Tick marks
    const int ticks = 10;
    for (int i = 1; i < ticks; ++i) {
        const f32 tx = x0 + w * f32(i) / f32(ticks);
        r.draw_line({tx, y0 + 1.0f}, {tx, y1 - 1.0f}, 1.0f,
                    with_alpha(edge, u8(60.0f)));
    }

    // Border + bevel
    r.draw_line({x0, y0}, {x1, y0}, 1.5f, with_alpha(edge, 220));
    r.draw_line({x0, y1}, {x1, y1}, 1.5f, with_alpha(edge, 220));
    r.draw_line({x0, y0}, {x0, y1}, 1.5f, with_alpha(edge, 220));
    r.draw_line({x1, y0}, {x1, y1}, 1.5f, with_alpha(edge, 220));
    r.draw_line({x0 + 2.0f, y0 + 2.0f}, {x0 + 2.0f, y1 - 2.0f}, 1.0f,
                with_alpha(COLOR_CORE_INNER, 90));

    // Label
    if (label && label[0]) {
        r.draw_text({x0, y0 - 20.0f}, label, 1.6f, with_alpha(edge, 200));
    }

    // Numeric readout right-aligned
    char num[16];
    std::snprintf(num, sizeof(num), "%d%%", int(frac * 100.0f + 0.5f));
    const f32 nw = r.text_width(num, 1.6f);
    r.draw_text({x1 - nw, y0 - 20.0f}, num, 1.6f, with_alpha(fill, 220));
}

// ---------------------------------------------------------------------------
// Radial meter — cooldown, charge, ability ready.
// ---------------------------------------------------------------------------
inline void hud_radial(render::Renderer& r, Vec2 c, f32 radius,
                       f32 frac, f32 t, Color color, f32 a = 1.0f) {
    frac = clamp(frac, 0.0f, 1.0f);
    const f32 pulse = 0.5f + 0.5f * std::sin(t * 5.0f);

    // Backing ring
    r.draw_ring(c, radius - 2.0f, radius, with_alpha(color, u8(a * 50.0f)), 32);

    // Filled arc — approximated with N small segments
    const int N = 40;
    const int shown = int(frac * f32(N) + 0.5f);
    for (int i = 0; i < shown; ++i) {
        const f32 a0 = -HALF_PI + TAU * f32(i) / f32(N);
        const f32 a1 = -HALF_PI + TAU * f32(i + 1) / f32(N) + 0.02f;
        r.draw_arc_ring(c, radius - 3.0f, radius + 1.0f, a0, a1,
                        with_alpha(color, u8(a * 220.0f)), 6);
    }

    // Center dot
    r.draw_circle(c, 4.0f, with_alpha(COLOR_CORE_INNER, u8(a * 200.0f)), 12);

    // Ready flash
    if (frac >= 0.999f) {
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_circle(c, radius + 6.0f + pulse * 4.0f,
                      with_alpha(color, u8(a * 60.0f * pulse)), 24);
        r.set_layer(render::Renderer::Layer::Alpha);
    }
}

// ---------------------------------------------------------------------------
// Framed panel with corner brackets and a label tab.
// ---------------------------------------------------------------------------
inline void hud_frame(render::Renderer& r, f32 x0, f32 y0, f32 x1, f32 y1,
                      const char* label, Color edge, f32 a = 1.0f) {
    const f32 n = 18.0f;

    // Faint fill
    r.draw_quad({x0, y0}, {x1, y0}, {x1, y1}, {x0, y1},
                rgba(4, 12, 20, u8(a * 180.0f)));

    // Notched border
    const Color b = with_alpha(edge, u8(a * 200.0f));
    r.draw_line({x0 + n, y0}, {x1 - n, y0}, 1.5f, b);
    r.draw_line({x1, y0 + n}, {x1, y1 - n}, 1.5f, b);
    r.draw_line({x1 - n, y1}, {x0 + n, y1}, 1.5f, b);
    r.draw_line({x0, y1 - n}, {x0, y0 + n}, 1.5f, b);
    r.draw_line({x0, y0}, {x0 + n, y0}, 2.5f, with_alpha(edge, u8(a * 255.0f)));
    r.draw_line({x0, y0}, {x0, y0 + n}, 2.5f, with_alpha(edge, u8(a * 255.0f)));
    r.draw_line({x1, y0}, {x1 - n, y0}, 2.5f, with_alpha(edge, u8(a * 255.0f)));
    r.draw_line({x1, y0}, {x1, y0 + n}, 2.5f, with_alpha(edge, u8(a * 255.0f)));
    r.draw_line({x0, y1}, {x0 + n, y1}, 2.5f, with_alpha(edge, u8(a * 255.0f)));
    r.draw_line({x0, y1}, {x0, y1 - n}, 2.5f, with_alpha(edge, u8(a * 255.0f)));
    r.draw_line({x1, y1}, {x1 - n, y1}, 2.5f, with_alpha(edge, u8(a * 255.0f)));
    r.draw_line({x1, y1}, {x1, y1 - n}, 2.5f, with_alpha(edge, u8(a * 255.0f)));

    // Corner dots
    r.draw_circle({x0, y0}, 3.0f, with_alpha(edge, u8(a * 240.0f)), 10);
    r.draw_circle({x1, y0}, 3.0f, with_alpha(edge, u8(a * 240.0f)), 10);
    r.draw_circle({x0, y1}, 3.0f, with_alpha(edge, u8(a * 240.0f)), 10);
    r.draw_circle({x1, y1}, 3.0f, with_alpha(edge, u8(a * 240.0f)), 10);

    // Label tab on top-left
    if (label && label[0]) {
        const f32 tw = r.text_width(label, 1.6f);
        const f32 pad = 8.0f;
        r.draw_quad({x0 + 12.0f, y0 - 10.0f}, {x0 + 12.0f + tw + pad * 2.0f, y0 - 10.0f},
                    {x0 + 12.0f + tw + pad * 2.0f, y0 + 8.0f}, {x0 + 12.0f, y0 + 8.0f},
                    rgba(4, 12, 20, u8(a * 235.0f)));
        r.draw_line({x0 + 12.0f, y0 - 10.0f},
                    {x0 + 12.0f + tw + pad * 2.0f, y0 - 10.0f}, 1.5f,
                    with_alpha(edge, u8(a * 220.0f)));
        r.draw_text({x0 + 12.0f + pad, y0 - 6.0f}, label, 1.6f,
                    with_alpha(edge, u8(a * 240.0f)));
    }
}

// ---------------------------------------------------------------------------
// Ability icon with cooldown sweep.
// ---------------------------------------------------------------------------
inline void hud_ability_icon(render::Renderer& r, Vec2 c, f32 size,
                             char glyph, f32 cooldown, f32 t, Color edge) {
    cooldown = clamp(cooldown, 0.0f, 1.0f);
    const f32 half = size * 0.5f;
    const f32 x0 = c.x - half;
    const f32 x1 = c.x + half;
    const f32 y0 = c.y - half;
    const f32 y1 = c.y + half;

    const bool ready = (cooldown <= 0.001f);
    const Color eg = ready ? edge : with_alpha(edge, 110);

    r.draw_quad({x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}, rgba(6, 16, 24, 230));

    // Cooldown fill rises from bottom as it cools
    if (!ready) {
        const f32 fill_h = (1.0f - cooldown) * size;
        r.draw_quad({x0, y1 - fill_h}, {x1, y1 - fill_h},
                    {x1, y1}, {x0, y1}, with_alpha(edge, 60));
    }

    // Border
    r.draw_line({x0, y0}, {x1, y0}, 1.5f, eg);
    r.draw_line({x0, y1}, {x1, y1}, 1.5f, eg);
    r.draw_line({x0, y0}, {x0, y1}, 1.5f, eg);
    r.draw_line({x1, y0}, {x1, y1}, 1.5f, eg);

    // Glyph
    char buf[2] = { glyph, 0 };
    const f32 gw = r.text_width(buf, 2.4f);
    r.draw_text({c.x - gw * 0.5f, c.y - 12.0f}, buf, 2.4f,
                with_alpha(ready ? COLOR_CORE_INNER : edge, ready ? 255 : 140));

    // Ready pulse
    if (ready) {
        const f32 pulse = 0.5f + 0.5f * std::sin(t * 4.0f);
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_circle(c, size * 0.9f + pulse * 3.0f,
                      with_alpha(edge, u8(24.0f * pulse)), 16);
        r.set_layer(render::Renderer::Layer::Alpha);
    }
}

// ---------------------------------------------------------------------------
// Discrete ammo pips.
// ---------------------------------------------------------------------------
inline void hud_ammo_dots(render::Renderer& r, f32 x0, f32 y,
                          int count, int max_count, Color fill, Color empty) {
    const f32 gap = 14.0f;
    for (int i = 0; i < max_count; ++i) {
        const Vec2 p {x0 + f32(i) * gap, y};
        const bool on = (i < count);
        if (on) {
            r.draw_circle(p, 4.5f, with_alpha(fill, 240), 12);
        } else {
            r.draw_ring(p, 3.5f, 4.5f, with_alpha(empty, 160), 12);
        }
    }
}

// ---------------------------------------------------------------------------
// Centered warning banner with fade + pulse.
// ---------------------------------------------------------------------------
inline void hud_warning_banner(render::Renderer& r, f32 cx, f32 y,
                               const char* text, f32 alpha_in, f32 t,
                               Color color) {
    if (alpha_in <= 0.0f) return;
    const f32 pulse = 0.5f + 0.5f * std::sin(t * 8.0f);
    const f32 a = clamp(alpha_in, 0.0f, 1.0f);

    // Backing bar
    const f32 tw = r.text_width(text, 3.2f);
    const f32 bw = tw + 80.0f;
    r.draw_quad({cx - bw * 0.5f, y - 22.0f}, {cx + bw * 0.5f, y - 22.0f},
                {cx + bw * 0.5f, y + 30.0f}, {cx - bw * 0.5f, y + 30.0f},
                rgba(6, 12, 20, u8(a * 200.0f)));
    r.draw_line({cx - bw * 0.5f, y - 22.0f}, {cx + bw * 0.5f, y - 22.0f}, 2.0f,
                with_alpha(color, u8(a * 220.0f)));
    r.draw_line({cx - bw * 0.5f, y + 30.0f}, {cx + bw * 0.5f, y + 30.0f}, 2.0f,
                with_alpha(color, u8(a * 220.0f)));

    // Glow
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_circle({cx, y + 4.0f}, bw * 0.6f,
                  with_alpha(color, u8(a * (30.0f + 20.0f * pulse))), 32);
    r.set_layer(render::Renderer::Layer::Alpha);

    // Text
    r.draw_text_centered({cx, y}, text, 3.2f,
                         with_alpha(color, u8(a * (220.0f + 35.0f * pulse))));
}

// ---------------------------------------------------------------------------
// Small caps label with typewriter reveal.
// ---------------------------------------------------------------------------
inline void hud_mini_label(render::Renderer& r, Vec2 p,
                           const char* text, f32 size, f32 reveal, Color c) {
    reveal = clamp(reveal, 0.0f, 1.0f);
    int len = 0; while (text[len]) ++len;
    const int shown = int(f32(len) * reveal + 0.5f);
    for (int i = 0; i < shown; ++i) {
        char one[2] = { text[i], 0 };
        const f32 w = r.text_width(text[i] == 32 ? "X" : one, size);
        r.draw_text({p.x + 0.0f, p.y}, one, size, with_alpha(c, 220));
        p.x += w;
    }
    // Blinking cursor while revealing
    if (reveal < 0.999f) {
        const f32 blink = 0.5f + 0.5f * std::sin(reveal * 90.0f);
        if (blink > 0.4f) {
            r.draw_quad({p.x, p.y}, {p.x + 6.0f, p.y},
                        {p.x + 6.0f, p.y + size * 10.0f}, {p.x, p.y + size * 10.0f},
                        with_alpha(COLOR_CORE_INNER, 220));
        }
    }
}

// ---------------------------------------------------------------------------
// Simple segmented progress indicator (waveform-style).
// ---------------------------------------------------------------------------
inline void hud_segments(render::Renderer& r, f32 x0, f32 y,
                         int count, f32 filled_frac, f32 t,
                         f32 seg_w, f32 seg_h, Color on, Color off) {
    for (int i = 0; i < count; ++i) {
        const f32 sx = x0 + f32(i) * (seg_w + 3.0f);
        const f32 k = f32(i) / f32(count);
        const bool lit = (k <= filled_frac);
        const f32 wiggle = 0.5f + 0.5f * std::sin(t * 5.0f + f32(i) * 0.7f);
        const Color c = lit ? on : off;
        const f32 h = lit ? seg_h * (0.7f + 0.3f * wiggle) : seg_h;
        r.draw_quad({sx, y + (seg_h - h)}, {sx + seg_w, y + (seg_h - h)},
                    {sx + seg_w, y + seg_h}, {sx, y + seg_h},
                    with_alpha(c, lit ? 230 : 120));
    }
}

} // namespace inc::hud

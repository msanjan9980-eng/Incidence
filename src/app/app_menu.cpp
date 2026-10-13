#include "app/app.hpp"
#include "render/fx.hpp"
#include "render/particles.hpp"
#include "render/field.hpp"
#include "audio/audio.hpp"
#include "game/save.hpp"
#include "render/icons.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace inc {

// ---------------------------------------------------------------------------
// Parallax starfield - three depth layers, deterministic seeds.
// ---------------------------------------------------------------------------
static void menu_starfield(render::Renderer& r, f32 t, f32 a) {
    constexpr int LAYERS = 3;
    const int counts[LAYERS] = {60, 40, 24};
    const f32 speeds[LAYERS] = {8.0f, 22.0f, 55.0f};
    const f32 sizes[LAYERS]  = {0.7f, 1.1f, 1.8f};
    const f32 base_a[LAYERS] = {90.0f, 140.0f, 220.0f};
    for (int l = 0; l < LAYERS; ++l) {
        for (int i = 0; i < counts[l]; ++i) {
            const u32 seedA = u32(i * (7919u + u32(l) * 613u) + 1u);
            const u32 seedB = u32(i * (6247u + u32(l) * 419u) + 7u);
            const u32 seedC = u32(i * (3571u + u32(l) * 977u) + 13u);
            const f32 h1 = f32(seedA & 0xFFFFu) / 65535.0f;
            const f32 h2 = f32(seedB & 0xFFFFu) / 65535.0f;
            const f32 h3 = f32(seedC & 0xFFFFu) / 65535.0f;
            f32 x = std::fmod(h1 * f32(VIRTUAL_W) - t * speeds[l], f32(VIRTUAL_W));
            if (x < 0.0f) x += f32(VIRTUAL_W);
            const f32 y = h2 * f32(VIRTUAL_H);
            const f32 tw = 0.5f + 0.5f * std::sin(t * (1.0f + h3 * 2.5f) + h1 * 30.0f);
            const u8 al = u8(a * base_a[l] * (0.4f + 0.6f * tw) / 255.0f);
            r.draw_circle({x, y}, sizes[l] * (0.8f + h3 * 0.6f),
                          with_alpha(COLOR_CORE_INNER, al), 6);
        }
    }
}

// ---------------------------------------------------------------------------
// Perspective horizon grid at the bottom of the screen.
// ---------------------------------------------------------------------------
static void menu_horizon_grid(render::Renderer& r, f32 t, f32 a) {
    const f32 horizon_y = f32(VIRTUAL_H) * 0.78f;
    const f32 cx = f32(VIRTUAL_W) * 0.5f;
    for (int i = 0; i < 12; ++i) {
        const f32 k = f32(i) / 11.0f;
        const f32 y = horizon_y + (f32(VIRTUAL_H) - horizon_y) * (k * k);
        const u8 al = u8(40.0f * a * (1.0f - k * 0.7f));
        r.draw_line({0, y}, {f32(VIRTUAL_W), y}, 1.0f,
                    with_alpha(COLOR_CORE_EDGE, al));
    }
    for (int i = -12; i <= 12; ++i) {
        const f32 xf = cx + f32(i) * 90.0f;
        const f32 xb = cx + f32(i) * 400.0f;
        r.draw_line({xf, horizon_y}, {xb, f32(VIRTUAL_H)}, 1.0f,
                    with_alpha(COLOR_CORE_EDGE, u8(28.0f * a)));
    }
    const f32 sweep = std::fmod(t * 0.4f, 1.6f);
    const f32 sx = cx - 800.0f + sweep * 1600.0f;
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_quad({sx, horizon_y - 40.0f}, {sx + 200.0f, horizon_y - 40.0f},
                {sx + 200.0f, horizon_y + 4.0f}, {sx, horizon_y + 4.0f},
                with_alpha(COLOR_CORE_EDGE, u8(30.0f * a)));
    r.set_layer(render::Renderer::Layer::Alpha);
}

// ---------------------------------------------------------------------------
// Chromatic title "INCIDENCE" with glow halo.
// ---------------------------------------------------------------------------
// Orbit sparks around a selected element.
static void menu_orbit_sparks(render::Renderer& r, Vec2 c, f32 rx, f32 ry,
                              f32 t, Color col, f32 a, int n = 10) {
    for (int i = 0; i < n; ++i) {
        const f32 ang = t * 1.4f + TAU * f32(i) / f32(n);
        const Vec2 p { c.x + std::cos(ang) * rx, c.y + std::sin(ang) * ry };
        const f32 pulse = 0.5f + 0.5f * std::sin(t * 4.0f + f32(i) * 0.9f);
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_circle(p, 10.0f + pulse * 5.0f,
                      with_alpha(col, u8(a * (35.0f + 25.0f * pulse))), 12);
        r.set_layer(render::Renderer::Layer::Alpha);
        r.draw_circle(p, 2.0f + pulse * 1.2f, with_alpha(col, u8(a * 240.0f)), 8);
        const f32 trail = ang - 0.14f;
        const Vec2 q { c.x + std::cos(trail) * rx, c.y + std::sin(trail) * ry };
        r.draw_circle(q, 1.5f, with_alpha(col, u8(a * 130.0f)), 6);
    }
}
static void menu_title(render::Renderer& r, Vec2 c, f32 t, f32 a) {
    const char* title = "INCIDENCE";
    const int   len   = 9;
    const f32   size  = 15.0f;
    const f32   adv   = r.text_width("XX", size) - r.text_width("X", size);
    const f32   total = r.text_width(title, size);
    f32         px    = c.x - total * 0.5f;

    // Halo behind title
    const f32 pulse = 0.9f + 0.1f * std::sin(t * 1.8f);
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_circle(c, 500.0f * pulse, with_alpha(COLOR_CORE_EDGE, u8(a * 22.0f)), 48);
    r.set_layer(render::Renderer::Layer::Alpha);

    // Letters
    for (int i = 0; i < len; ++i) {
        const f32 enter = clamp((t - (0.30f + f32(i) * 0.055f)) / 0.40f, 0.0f, 1.0f);
        if (enter <= 0.0f) { px += adv; continue; }
        const f32 ease    = 1.0f - (1.0f - enter) * (1.0f - enter);
        const f32 slide_y = (1.0f - ease) * -200.0f;
        const f32 jitter  = std::sin(t * 3.0f + f32(i) * 1.3f) * 1.5f;
        char one[2] = { title[i], 0 };

        r.draw_text({px + 3.0f, c.y + 3.0f + slide_y + jitter}, one, size,
                    with_alpha(COLOR_ENEMY_EDGE, u8(a * enter * 40.0f)));
        r.draw_text({px - 3.0f, c.y - 3.0f + slide_y - jitter}, one, size,
                    with_alpha(COLOR_CORE_INNER, u8(a * enter * 50.0f)));

        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_circle({px + adv * 0.5f, c.y + slide_y + size * 3.5f},
                      60.0f * enter,
                      with_alpha(COLOR_CORE_INNER, u8(a * enter * 45.0f)), 24);
        r.set_layer(render::Renderer::Layer::Alpha);

        r.draw_text({px, c.y + slide_y + jitter}, one, size,
                    with_alpha(COLOR_CORE_INNER, u8(a * enter * 255.0f)));
        px += adv;
    }

    // Subtitle letter stagger
    const char* sub  = "REFLECT OR DIE";
    const int   slen = 14;
    const f32   ssz  = 2.6f;
    const f32   sadv = r.text_width("XX", ssz) - r.text_width("X", ssz);
    const f32   sw   = r.text_width(sub, ssz);
    f32         sx   = c.x - sw * 0.5f;
    for (int i = 0; i < slen; ++i) {
        const f32 enter = clamp((t - (1.20f + f32(i) * 0.028f)) / 0.30f, 0.0f, 1.0f);
        if (enter > 0.0f) {
            const f32 slide = (1.0f - enter) * 40.0f;
            char one[2] = { sub[i], 0 };
            r.draw_text({sx + slide, c.y + 140.0f}, one, ssz,
                        with_alpha(COLOR_FACET_EDGE, u8(a * enter * 210.0f)));
        }
        sx += sadv;
    }

    // Divider draw-out
    const f32 div_age = clamp((t - 1.80f) / 0.50f, 0.0f, 1.0f);
    if (div_age > 0.0f) {
        const f32 dhw = 320.0f * div_age;
        r.draw_line({c.x - dhw, c.y + 180.0f}, {c.x + dhw, c.y + 180.0f}, 1.0f,
                    with_alpha(COLOR_CORE_EDGE, u8(a * 100.0f * div_age)));
    }

    // Diamond pop
    const f32 dia_age = clamp((t - 2.05f) / 0.30f, 0.0f, 1.0f);
    if (dia_age > 0.0f) {
        const f32 ds     = 5.0f * dia_age;
        const f32 pulse2 = 0.5f + 0.5f * std::sin(t * 4.0f);
        r.draw_quad({c.x, c.y + 180.0f - ds}, {c.x + ds, c.y + 180.0f},
                    {c.x, c.y + 180.0f + ds}, {c.x - ds, c.y + 180.0f},
                    with_alpha(COLOR_CORE_EDGE,
                               u8(a * (180.0f + 60.0f * pulse2) * dia_age)));
    }
}

// ---------------------------------------------------------------------------
// One menu item with hover brackets and glow.
// ---------------------------------------------------------------------------
static void menu_item(render::Renderer& r, Vec2 c, const char* label,
                      bool selected, bool enabled, f32 t, f32 a, f32 appear_t) {
    const f32 pulse = 0.5f + 0.5f * std::sin(t * 5.0f);
    const Color col = enabled
        ? (selected ? COLOR_CORE_INNER : COLOR_CORE_EDGE)
        : COLOR_FACET_EDGE;
    const u8  al    = enabled ? u8(a * 255.0f) : u8(a * 110.0f);

    const f32 enter = clamp(appear_t / 0.45f, 0.0f, 1.0f);
    if (enter <= 0.0f) return;
    const f32 ease    = 1.0f - (1.0f - enter) * (1.0f - enter);
    const f32 slide_x = (1.0f - ease) * 60.0f;
    const u8  falpha  = u8(f32(al) * enter);

    if (selected && enabled) {
        menu_orbit_sparks(r, {c.x, c.y + 10.0f}, 240.0f, 60.0f, t,
                          COLOR_CORE_EDGE, a, 10);

        const f32 bw  = 260.0f;
        const f32 bh  = 30.0f;
        const f32 jit = std::sin(t * 6.0f) * 3.0f;
        const f32 ix  = c.x - bw - 40.0f + jit;
        const f32 rx  = c.x + bw + 40.0f - jit;
        const f32 arm = 20.0f + pulse * 4.0f;
        const Color bc = with_alpha(COLOR_CORE_INNER, u8(a * 240.0f));

        // Left bracket
        r.draw_line({ix, c.y - bh + 4.0f}, {ix, c.y + bh + 4.0f}, 3.0f, bc);
        r.draw_line({ix, c.y - bh + 4.0f}, {ix + arm, c.y - bh + 4.0f}, 3.0f, bc);
        r.draw_line({ix, c.y + bh + 4.0f}, {ix + arm, c.y + bh + 4.0f}, 3.0f, bc);
        for (int i = 0; i < 3; ++i) {
            const f32 ty = c.y - bh + 12.0f + f32(i) * 10.0f;
            const f32 ta = a * (140.0f + 60.0f * std::sin(t * 5.0f + f32(i)));
            r.draw_line({ix - 6.0f, ty}, {ix - 12.0f, ty}, 1.5f,
                        with_alpha(COLOR_CORE_EDGE, u8(ta)));
        }
        // Right bracket
        r.draw_line({rx, c.y - bh + 4.0f}, {rx, c.y + bh + 4.0f}, 3.0f, bc);
        r.draw_line({rx, c.y - bh + 4.0f}, {rx - arm, c.y - bh + 4.0f}, 3.0f, bc);
        r.draw_line({rx, c.y + bh + 4.0f}, {rx - arm, c.y + bh + 4.0f}, 3.0f, bc);
        for (int i = 0; i < 3; ++i) {
            const f32 ty = c.y - bh + 12.0f + f32(i) * 10.0f;
            const f32 ta = a * (140.0f + 60.0f * std::sin(t * 5.0f + f32(i) + 1.7f));
            r.draw_line({rx + 6.0f, ty}, {rx + 12.0f, ty}, 1.5f,
                        with_alpha(COLOR_CORE_EDGE, u8(ta)));
        }

        // Underline bar
        const f32 uw = 180.0f + pulse * 20.0f;
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_quad({c.x - uw, c.y + bh + 14.0f}, {c.x + uw, c.y + bh + 14.0f},
                    {c.x + uw, c.y + bh + 16.0f}, {c.x - uw, c.y + bh + 16.0f},
                    with_alpha(COLOR_CORE_EDGE, u8(a * (100.0f + 60.0f * pulse))));
        r.set_layer(render::Renderer::Layer::Alpha);

        // Glow behind text
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_circle({c.x, c.y + 10.0f}, 200.0f + pulse * 25.0f,
                      with_alpha(COLOR_CORE_EDGE, u8(a * 38.0f)), 40);
        r.set_layer(render::Renderer::Layer::Alpha);
    }

    const f32 sz = selected ? 4.0f : 3.2f;
    r.draw_text_centered({c.x + slide_x, c.y}, label, sz, with_alpha(col, falpha));
}

// ---------------------------------------------------------------------------
// Save-slot info panel.
// ---------------------------------------------------------------------------
static void menu_save_panel(render::Renderer& r, Vec2 c, f32 w, f32 h,
                            bool has_save, int room, int level, f32 a, f32 t) {
    const f32 reveal = clamp(t / 0.55f, 0.0f, 1.0f);
    if (reveal <= 0.0f) return;
    const f32 ease = 1.0f - (1.0f - reveal) * (1.0f - reveal);
    const f32 ww   = w * ease;
    const f32 x0   = c.x - ww * 0.5f;
    const f32 x1   = c.x + ww * 0.5f;
    const f32 y0   = c.y - h  * 0.5f;
    const f32 y1   = c.y + h  * 0.5f;

    r.draw_quad({x0 + 8.0f, y0 + 8.0f}, {x1 + 8.0f, y0 + 8.0f},
                {x1 + 8.0f, y1 + 8.0f}, {x0 + 8.0f, y1 + 8.0f},
                rgba(0, 0, 0, u8(a * 170.0f)));
    r.draw_quad({x0, y0}, {x1, y0}, {x1, y1}, {x0, y1},
                rgba(8, 22, 32, u8(a * 225.0f)));
    r.draw_line({x0, y0}, {x1, y0}, 1.5f, with_alpha(COLOR_CORE_EDGE, u8(a * 200.0f)));
    r.draw_line({x0, y0}, {x0, y1}, 1.5f, with_alpha(COLOR_CORE_EDGE, u8(a * 200.0f)));
    r.draw_line({x1, y1}, {x0, y1}, 1.5f, with_alpha(COLOR_CORE_EDGE, u8(a * 200.0f)));
    r.draw_line({x1, y0}, {x1, y1}, 1.5f, with_alpha(COLOR_CORE_EDGE, u8(a * 200.0f)));

    r.draw_text({x0 + 14.0f, y0 + 10.0f}, "SAVE DATA", 1.8f,
                with_alpha(COLOR_CORE_EDGE, u8(a * 200.0f)));
    r.draw_line({x0 + 14.0f, y0 + 40.0f}, {x1 - 14.0f, y0 + 40.0f}, 1.0f,
                with_alpha(COLOR_CORE_EDGE, u8(a * 90.0f)));

    if (has_save) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "LV %d", level);
        r.draw_text({x0 + 14.0f, y0 + 52.0f}, buf, 2.2f,
                    with_alpha(COLOR_CORE_INNER, u8(a * 240.0f)));
        std::snprintf(buf, sizeof(buf), "ROOM %02d", room);
        r.draw_text({x0 + 14.0f, y0 + 82.0f}, buf, 2.2f,
                    with_alpha(COLOR_FACET_EDGE, u8(a * 200.0f)));

        const f32 blink = 0.5f + 0.5f * std::sin(t * 3.0f);
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_circle({x1 - 24.0f, y0 + 24.0f}, 14.0f + blink * 6.0f,
                      with_alpha(COLOR_CORE_INNER, u8(a * (60.0f + 40.0f * blink))), 16);
        r.set_layer(render::Renderer::Layer::Alpha);
        r.draw_circle({x1 - 24.0f, y0 + 24.0f}, 6.0f,
                      with_alpha(COLOR_CORE_INNER, u8(a * (120.0f + 100.0f * blink))), 12);

        for (int i = 0; i < 8; ++i) {
            const f32 bx = x1 - 130.0f + f32(i) * 12.0f;
            const f32 bp = 0.5f + 0.5f * std::sin(t * 2.5f + f32(i) * 0.7f);
            const f32 bh2 = 6.0f + bp * 22.0f;
            r.draw_quad({bx, y1 - 24.0f - bh2}, {bx + 8.0f, y1 - 24.0f - bh2},
                        {bx + 8.0f, y1 - 24.0f}, {bx, y1 - 24.0f},
                        with_alpha(COLOR_CORE_EDGE, u8(a * (80.0f + 100.0f * bp))));
        }
    } else {
        r.draw_text({x0 + 14.0f, y0 + 56.0f}, "NO DATA", 2.2f,
                    with_alpha(COLOR_FACET_EDGE, u8(a * 130.0f)));
    }

    if (reveal < 1.0f) {
        const f32 sy = y0 + (y1 - y0) * reveal;
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_quad({x0, sy - 2.0f}, {x1, sy - 2.0f},
                    {x1, sy + 2.0f}, {x0, sy + 2.0f},
                    with_alpha(COLOR_CORE_INNER, u8((1.0f - reveal) * 180.0f)));
        r.set_layer(render::Renderer::Layer::Alpha);
    }
}

// ---------------------------------------------------------------------------
// Bottom bar with version, hints, station name.
// ---------------------------------------------------------------------------
static void menu_bottom_bar(render::Renderer& r, f32 a) {
    const f32 y = f32(VIRTUAL_H) - 44.0f;
    r.draw_line({40.0f, y - 20.0f}, {f32(VIRTUAL_W) - 40.0f, y - 20.0f}, 1.0f,
                with_alpha(COLOR_CORE_EDGE, u8(a * 80.0f)));
    r.draw_text({40.0f, y}, "INCIDENCE  v0.1.0", 1.8f,
                with_alpha(COLOR_FACET_EDGE, u8(a * 180.0f)));
    const char* mid = "UP / DOWN: SELECT     ENTER: CONFIRM     ESC: QUIT";
    const f32 mw = r.text_width(mid, 1.8f);
    r.draw_text({f32(VIRTUAL_W) * 0.5f - mw * 0.5f, y}, mid, 1.8f,
                with_alpha(COLOR_FACET_EDGE, u8(a * 200.0f)));
    r.draw_text({f32(VIRTUAL_W) - 300.0f, y}, "HALCYON STATION", 1.8f,
                with_alpha(COLOR_FACET_EDGE, u8(a * 140.0f)));
}

// ---------------------------------------------------------------------------
// Corner decorations.
// ---------------------------------------------------------------------------
static void menu_corners(render::Renderer& r, f32 a) {
    const f32 m = 30.0f;
    const f32 s = 60.0f;
    const Color c = with_alpha(COLOR_CORE_EDGE, u8(a * 180.0f));
    r.draw_line({m, m}, {m + s, m}, 2.0f, c);
    r.draw_line({m, m}, {m, m + s}, 2.0f, c);
    r.draw_line({f32(VIRTUAL_W) - m, m}, {f32(VIRTUAL_W) - m - s, m}, 2.0f, c);
    r.draw_line({f32(VIRTUAL_W) - m, m}, {f32(VIRTUAL_W) - m, m + s}, 2.0f, c);
    r.draw_line({m, f32(VIRTUAL_H) - m}, {m + s, f32(VIRTUAL_H) - m}, 2.0f, c);
    r.draw_line({m, f32(VIRTUAL_H) - m}, {m, f32(VIRTUAL_H) - m - s}, 2.0f, c);
    r.draw_line({f32(VIRTUAL_W) - m, f32(VIRTUAL_H) - m},
                {f32(VIRTUAL_W) - m - s, f32(VIRTUAL_H) - m}, 2.0f, c);
    r.draw_line({f32(VIRTUAL_W) - m, f32(VIRTUAL_H) - m},
                {f32(VIRTUAL_W) - m, f32(VIRTUAL_H) - m - s}, 2.0f, c);
    r.draw_circle({m, m}, 3.0f, with_alpha(COLOR_CORE_INNER, u8(a * 240.0f)), 10);
    r.draw_circle({f32(VIRTUAL_W) - m, m}, 3.0f, with_alpha(COLOR_CORE_INNER, u8(a * 240.0f)), 10);
    r.draw_circle({m, f32(VIRTUAL_H) - m}, 3.0f, with_alpha(COLOR_CORE_INNER, u8(a * 240.0f)), 10);
    r.draw_circle({f32(VIRTUAL_W) - m, f32(VIRTUAL_H) - m}, 3.0f, with_alpha(COLOR_CORE_INNER, u8(a * 240.0f)), 10);
}

// ---------------------------------------------------------------------------
// Small logo drone in the top-left corner.
// ---------------------------------------------------------------------------
static void menu_logo_drone(render::Renderer& r, Vec2 c, f32 t, f32 a) {
    const f32 heading = -HALF_PI + t * 0.3f;
    const f32 span = 2.44f;
    const f32 R_IN = 20.0f;
    const f32 R_OUT = 26.0f;
    for (int i = 0; i < 5; ++i) {
        const f32 step = span / 5.0f;
        const f32 a0 = heading - span * 0.5f + f32(i) * step;
        const f32 a1 = a0 + step;
        const Vec2 pin0 = c + Vec2::from_angle(a0) * R_IN;
        const Vec2 pin1 = c + Vec2::from_angle(a1) * R_IN;
        const Vec2 pout0 = c + Vec2::from_angle(a0) * R_OUT;
        const Vec2 pout1 = c + Vec2::from_angle(a1) * R_OUT;
        r.draw_line(pin0, pin1, 1.5f, with_alpha(COLOR_FACET_EDGE, u8(a * 200.0f)));
        r.draw_line(pout0, pout1, 2.0f, with_alpha(COLOR_FACET_EDGE, u8(a * 255.0f)));
        r.draw_line(pin0, pout0, 0.8f, with_alpha(COLOR_FACET_EDGE, u8(a * 140.0f)));
        r.draw_line(pin1, pout1, 0.8f, with_alpha(COLOR_FACET_EDGE, u8(a * 140.0f)));
    }
    r.draw_circle(c, 8.0f, with_alpha(COLOR_CORE_FILL, u8(a * 230.0f)), 16);
    r.draw_ring(c, 8.0f, 9.5f, with_alpha(COLOR_CORE_EDGE, u8(a * 240.0f)), 20);
    r.draw_circle(c, 2.5f, with_alpha(COLOR_CORE_INNER, u8(a * 255.0f)), 10);
}

// ---------------------------------------------------------------------------
// MAIN MENU - the first screen the player sees.
// ---------------------------------------------------------------------------
void App::draw_main_menu(render::Renderer& r) const {
    const f32 t = main_menu_time_;
    const f32 fade_in = clamp(t / 0.7f, 0.0f, 1.0f);
    const f32 a = fade_in;

    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({0, 0}, {f32(VIRTUAL_W), 0},
                {f32(VIRTUAL_W), f32(VIRTUAL_H)}, {0, f32(VIRTUAL_H)},
                rgba(3, 8, 14, 255));

    menu_starfield(r, t, a * 0.9f);
    menu_horizon_grid(r, t, a * 0.7f);
    menu_corners(r, a);
    menu_logo_drone(r, {80.0f, 60.0f}, t, a);

    // Title block
    menu_title(r, {f32(VIRTUAL_W) * 0.5f, 230.0f}, t, a);

    // Left-side decorative drone
    {
        const Vec2 dp { f32(VIRTUAL_W) * 0.15f, 700.0f };
        const f32 heading = -HALF_PI + std::sin(t * 0.8f) * 0.4f;
        const f32 span = 2.44f;
        const f32 R_IN = 46.0f;
        const f32 R_OUT = 58.0f;
        for (int i = 0; i < 5; ++i) {
            const f32 step = span / 5.0f;
            const f32 a0 = heading - span * 0.5f + f32(i) * step;
            const f32 a1 = a0 + step;
            const Vec2 pin0 = dp + Vec2::from_angle(a0) * R_IN;
            const Vec2 pin1 = dp + Vec2::from_angle(a1) * R_IN;
            const Vec2 pout0 = dp + Vec2::from_angle(a0) * R_OUT;
            const Vec2 pout1 = dp + Vec2::from_angle(a1) * R_OUT;
            r.draw_quad(pin0, pin1, pout1, pout0, with_alpha(COLOR_FACET_FILL, u8(a * 140.0f)));
            r.draw_line(pin0, pin1, 2.0f, with_alpha(COLOR_FACET_EDGE, u8(a * 220.0f)));
            r.draw_line(pout0, pout1, 3.0f, with_alpha(COLOR_FACET_EDGE, u8(a * 255.0f)));
            r.draw_line(pin0, pout0, 1.0f, with_alpha(COLOR_FACET_EDGE, u8(a * 160.0f)));
            r.draw_line(pin1, pout1, 1.0f, with_alpha(COLOR_FACET_EDGE, u8(a * 160.0f)));
        }
        r.draw_circle(dp, 14.0f, with_alpha(COLOR_CORE_FILL, u8(a * 230.0f)), 20);
        r.draw_ring(dp, 14.0f, 16.0f, with_alpha(COLOR_CORE_EDGE, u8(a * 255.0f)), 24);
        r.draw_circle(dp, 4.0f, with_alpha(COLOR_CORE_INNER, u8(a * 255.0f)), 12);
    }

    // Save slot existence
    FILE* sf = std::fopen("save.sav", "r");
    const bool has_save = (sf != nullptr);
    if (sf) std::fclose(sf);
    int sroom = 0, slevel = 1;
    if (has_save) {
        game::SaveData sd;
        if (game::load_game("save.sav", sd)) {
            sroom = sd.current_room;
            slevel = sd.character.level;
        }
    }

    // Menu items
    const char* items[3] = { "START NEW RUN", "CONTINUE", "QUIT" };
    const bool enabled[3] = { true, has_save, true };
    const f32 item_x = f32(VIRTUAL_W) * 0.5f;
    const f32 item_y0 = 640.0f;
    const f32 item_dy = 82.0f;
    for (int i = 0; i < 3; ++i) {
        const Vec2 c { item_x, item_y0 + f32(i) * item_dy };
        const f32 appear_t = t - (1.4f + f32(i) * 0.14f);
        menu_item(r, c, items[i], menu_cursor_ == i, enabled[i], t, a, appear_t);
    }

    // Panels
    menu_save_panel(r, {220.0f, 780.0f}, 340.0f, 130.0f,
                    has_save, sroom, slevel, a, t);
    menu_bottom_bar(r, a);

    {
        const char* quote = "\"You have no gun. You have a normal vector.\"";
        const int   qlen  = int(std::strlen(quote));
        const int   shown = int(clamp((t - 1.9f) * 28.0f, 0.0f, f32(qlen)));
        if (shown > 0) {
            char qbuf[96];
            for (int i = 0; i < shown; ++i) qbuf[i] = quote[i];
            qbuf[shown] = 0;
            const f32 qw = r.text_width(quote, 2.0f);
            const f32 qx = f32(VIRTUAL_W) - 60.0f - qw;
            const f32 qy = 780.0f;
            r.draw_text({qx, qy}, qbuf, 2.0f,
                        with_alpha(COLOR_FACET_EDGE, u8(a * 200.0f)));
            if (shown < qlen) {
                const f32 pw    = r.text_width(qbuf, 2.0f);
                const f32 blink = 0.5f + 0.5f * std::sin(t * 9.0f);
                if (blink > 0.35f) {
                    r.draw_quad({qx + pw + 1.0f, qy}, {qx + pw + 9.0f, qy},
                                {qx + pw + 9.0f, qy + 20.0f}, {qx + pw + 1.0f, qy + 20.0f},
                                with_alpha(COLOR_CORE_INNER, u8(a * 220.0f)));
                }
            }
        }
    }

    // Slow accent ring behind title
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_circle({f32(VIRTUAL_W) * 0.5f, 230.0f}, 380.0f + std::sin(t * 1.2f) * 20.0f,
                  with_alpha(COLOR_CORE_EDGE, u8(a * 10.0f)), 48);
    r.set_layer(render::Renderer::Layer::Alpha);

    // ---------- Batch 1 additions ----------
    // Light shafts fanning down from the horizon
    fx::light_shafts(r, f32(VIRTUAL_W) * 0.5f, f32(VIRTUAL_H) * 0.78f, t,
                     COLOR_CORE_EDGE, a * 0.8f, 9);

    // Drifting constellation across the top half
    fx::constellation(r, t * 0.6f, COLOR_CORE_INNER, a * 0.6f, 60, 8.0f);

    // Data streams hugging the left and right edges
    fx::data_stream(r, t, 40.0f, 120.0f, 40, COLOR_CORE_EDGE, a * 0.7f);
    fx::data_stream(r, t + 3.7f, f32(VIRTUAL_W) - 160.0f, 120.0f, 40,
                    COLOR_FACET_EDGE, a * 0.55f);

    // Orbiting dust cloud behind the title
    fx::orbiting_cloud(r, {f32(VIRTUAL_W) * 0.5f, 260.0f}, t,
                       COLOR_CORE_INNER, a * 0.7f, 54, 260.0f, 520.0f);

    // Radial pulse rings emanating from the title
    fx::radial_rings(r, {f32(VIRTUAL_W) * 0.5f, 260.0f}, t,
                     COLOR_CORE_EDGE, a * 0.7f, 3, 640.0f);

    // Ticker along the very bottom, under the bottom bar
    fx::ticker(r, t, "  >> SYSTEM NOMINAL  //  NO ANOMALIES DETECTED  //  WARDEN SIGNAL STABLE  ",
               f32(VIRTUAL_H) - 16.0f, 90.0f, COLOR_FACET_EDGE, a * 0.7f);

    // ---- Post-FX, place these LAST ----
    fx::glitch_slice(r, t, a * 0.9f);
    fx::noise_bars(r, t, a * 0.55f, 22);
    fx::film_grain(r, t, a * 0.9f, 420, 1.1f);
    fx::scanlines(r, a * 0.9f, 4.0f, 42);
    fx::vignette(r, a * 1.0f, 200.0f, 12);
}

} // namespace inc

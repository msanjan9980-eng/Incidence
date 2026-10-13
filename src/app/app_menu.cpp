#include "app/app.hpp"
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
static void menu_title(render::Renderer& r, Vec2 c, f32 t, f32 a) {
    const f32 pulse = 0.9f + 0.1f * std::sin(t * 1.8f);
    r.draw_text_centered({c.x + 3.0f, c.y + 3.0f}, "INCIDENCE", 15.0f,
                         with_alpha(COLOR_ENEMY_EDGE, u8(a * 40)));
    r.draw_text_centered({c.x - 3.0f, c.y - 3.0f}, "INCIDENCE", 15.0f,
                         with_alpha(COLOR_CORE_INNER, u8(a * 50)));
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_circle(c, 500.0f * pulse, with_alpha(COLOR_CORE_EDGE, u8(a * 22)), 48);
    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_text_centered(c, "INCIDENCE", 15.0f,
                         with_alpha(COLOR_CORE_INNER, u8(a * 255)));
    r.draw_text_centered({c.x, c.y + 140.0f}, "R E F L E C T   O R   D I E", 2.6f,
                         with_alpha(COLOR_FACET_EDGE, u8(a * 200)));
    r.draw_line({c.x - 320.0f, c.y + 180.0f}, {c.x + 320.0f, c.y + 180.0f}, 1.0f,
                with_alpha(COLOR_CORE_EDGE, u8(a * 100)));
    r.draw_quad({c.x, c.y + 176.0f}, {c.x + 5.0f, c.y + 180.0f},
                {c.x, c.y + 184.0f}, {c.x - 5.0f, c.y + 180.0f},
                with_alpha(COLOR_CORE_EDGE, u8(a * 220)));
}

// ---------------------------------------------------------------------------
// One menu item with hover brackets and glow.
// ---------------------------------------------------------------------------
static void menu_item(render::Renderer& r, Vec2 c, const char* label,
                      bool selected, bool enabled, f32 t, f32 a) {
    const f32 pulse = 0.5f + 0.5f * std::sin(t * 5.0f);
    const Color col = enabled
        ? (selected ? COLOR_CORE_INNER : COLOR_CORE_EDGE)
        : COLOR_FACET_EDGE;
    const u8 al = enabled ? u8(a * 255.0f) : u8(a * 110.0f);
    if (selected && enabled) {
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_circle({c.x, c.y + 10.0f}, 180.0f + pulse * 20.0f,
                      with_alpha(COLOR_CORE_EDGE, u8(a * 30.0f)), 36);
        r.set_layer(render::Renderer::Layer::Alpha);
        const f32 bw = 240.0f;
        const f32 bh = 26.0f;
        const f32 ix = c.x - bw - 40.0f + std::sin(t * 6.0f) * 3.0f;
        r.draw_line({ix, c.y - bh + 4.0f}, {ix, c.y + bh + 4.0f}, 3.0f,
                    with_alpha(COLOR_CORE_INNER, u8(a * 220.0f)));
        r.draw_line({ix, c.y - bh + 4.0f}, {ix + 18.0f, c.y - bh + 4.0f}, 3.0f,
                    with_alpha(COLOR_CORE_INNER, u8(a * 220.0f)));
        r.draw_line({ix, c.y + bh + 4.0f}, {ix + 18.0f, c.y + bh + 4.0f}, 3.0f,
                    with_alpha(COLOR_CORE_INNER, u8(a * 220.0f)));
        const f32 rx = c.x + bw + 40.0f - std::sin(t * 6.0f) * 3.0f;
        r.draw_line({rx, c.y - bh + 4.0f}, {rx, c.y + bh + 4.0f}, 3.0f,
                    with_alpha(COLOR_CORE_INNER, u8(a * 220.0f)));
        r.draw_line({rx, c.y - bh + 4.0f}, {rx - 18.0f, c.y - bh + 4.0f}, 3.0f,
                    with_alpha(COLOR_CORE_INNER, u8(a * 220.0f)));
        r.draw_line({rx, c.y + bh + 4.0f}, {rx - 18.0f, c.y + bh + 4.0f}, 3.0f,
                    with_alpha(COLOR_CORE_INNER, u8(a * 220.0f)));
    }
    const f32 sz = selected ? 4.0f : 3.2f;
    r.draw_text_centered(c, label, sz, with_alpha(col, al));
}

// ---------------------------------------------------------------------------
// Save-slot info panel.
// ---------------------------------------------------------------------------
static void menu_save_panel(render::Renderer& r, Vec2 c, f32 w, f32 h,
                            bool has_save, int room, int level, f32 a, f32 t) {
    r.draw_quad({c.x - w*0.5f, c.y - h*0.5f}, {c.x + w*0.5f, c.y - h*0.5f},
                {c.x + w*0.5f, c.y + h*0.5f}, {c.x - w*0.5f, c.y + h*0.5f},
                rgba(8, 22, 32, u8(a * 200.0f)));
    r.draw_line({c.x - w*0.5f, c.y - h*0.5f}, {c.x + w*0.5f, c.y - h*0.5f},
                1.5f, with_alpha(COLOR_CORE_EDGE, u8(a * 200.0f)));
    r.draw_line({c.x - w*0.5f, c.y - h*0.5f}, {c.x - w*0.5f, c.y + h*0.5f},
                1.5f, with_alpha(COLOR_CORE_EDGE, u8(a * 200.0f)));
    r.draw_text({c.x - w*0.5f + 14.0f, c.y - h*0.5f + 10.0f}, "SAVE DATA",
                1.8f, with_alpha(COLOR_CORE_EDGE, u8(a * 200.0f)));
    r.draw_line({c.x - w*0.5f + 14.0f, c.y - h*0.5f + 40.0f},
                {c.x + w*0.5f - 14.0f, c.y - h*0.5f + 40.0f}, 1.0f,
                with_alpha(COLOR_CORE_EDGE, u8(a * 90.0f)));
    if (has_save) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "LV %d", level);
        r.draw_text({c.x - w*0.5f + 14.0f, c.y - h*0.5f + 52.0f}, buf, 2.2f,
                    with_alpha(COLOR_CORE_INNER, u8(a * 240.0f)));
        std::snprintf(buf, sizeof(buf), "ROOM %02d", room);
        r.draw_text({c.x - w*0.5f + 14.0f, c.y - h*0.5f + 82.0f}, buf, 2.2f,
                    with_alpha(COLOR_FACET_EDGE, u8(a * 200.0f)));
        const f32 blink = 0.5f + 0.5f * std::sin(t * 3.0f);
        r.draw_circle({c.x + w*0.5f - 24.0f, c.y - h*0.5f + 24.0f}, 6.0f,
                      with_alpha(COLOR_CORE_INNER, u8(a * (120.0f + 100.0f * blink))), 12);
    } else {
        r.draw_text({c.x - w*0.5f + 14.0f, c.y - h*0.5f + 56.0f}, "NO DATA",
                    2.2f, with_alpha(COLOR_FACET_EDGE, u8(a * 130.0f)));
    }
}

// ---------------------------------------------------------------------------
// Bottom bar with version, hints, station name.
// ---------------------------------------------------------------------------
static void menu_bottom_bar(render::Renderer& r, f32 a) {
    const f32 y = f32(VIRTUAL_H) - 34.0f;
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
        menu_item(r, c, items[i], menu_cursor_ == i, enabled[i], t, a);
    }

    // Panels
    menu_save_panel(r, {220.0f, 780.0f}, 340.0f, 130.0f,
                    has_save, sroom, slevel, a, t);
    menu_bottom_bar(r, a);

    const char* quote = "\"You have no gun. You have a normal vector.\"";
    const f32 qw = r.text_width(quote, 2.0f);
    r.draw_text({f32(VIRTUAL_W) - 60.0f - qw, 780.0f}, quote, 2.0f,
                with_alpha(COLOR_FACET_EDGE, u8(a * 180.0f)));

    // Slow accent ring behind title
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_circle({f32(VIRTUAL_W) * 0.5f, 230.0f}, 380.0f + std::sin(t * 1.2f) * 20.0f,
                  with_alpha(COLOR_CORE_EDGE, u8(a * 10.0f)), 48);
    r.set_layer(render::Renderer::Layer::Alpha);
}

} // namespace inc

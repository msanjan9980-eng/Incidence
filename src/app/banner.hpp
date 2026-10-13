#pragma once

#include <cmath>
#include "core/color.hpp"
#include "core/math.hpp"
#include "core/vec2.hpp"
#include "render/renderer.hpp"
#include "render/icons.hpp"

namespace inc {

// Banner diagonal warning stripes - bright when alpha is high.
inline void banner_warning_stripes(render::Renderer& r,
                                   f32 x0, f32 y0, f32 x1, f32 y1,
                                   f32 alpha, Color a, Color b) {
    const f32 step = 26.0f;
    const f32 h = y1 - y0;
    const f32 w = x1 - x0;
    const int count = int((w + h) / step) + 2;
    for (int i = 0; i < count; ++i) {
        const f32 sx = x0 + f32(i) * step - h;
        const f32 sy = y0;
        const f32 ex = sx + h;
        const f32 ey = y1;
        const bool alt = (i & 1) == 0;
        const Color c = alt ? a : b;
        r.draw_quad({sx, sy}, {sx + step * 0.5f, sy},
                    {ex + step * 0.5f, ey}, {ex, ey},
                    with_alpha(c, u8(alpha * 60)));
    }
}

// Hex-grid pattern behind the card, fades top->bottom.
inline void banner_grid_pattern(render::Renderer& r,
                                f32 x0, f32 y0, f32 x1, f32 y1,
                                f32 alpha, Color color) {
    const f32 cell = 42.0f;
    const f32 gap  = 2.0f;
    for (f32 gy = y0; gy < y1; gy += cell) {
        for (f32 gx = x0; gx < x1; gx += cell) {
            const f32 t = (gy - y0) / (y1 - y0);
            const u8 a = u8(alpha * (1.0f - t) * 22.0f);
            r.draw_quad({gx, gy}, {gx + cell - gap, gy},
                        {gx + cell - gap, gy + cell - gap}, {gx, gy + cell - gap},
                        with_alpha(color, a));
        }
    }
}

// Drifting dust motes inside the card bounds.
inline void banner_motes(render::Renderer& r, f32 t,
                         f32 x0, f32 y0, f32 x1, f32 y1,
                         f32 alpha, Color color) {
    const int N = 26;
    const f32 w = x1 - x0;
    const f32 h = y1 - y0;
    for (int i = 0; i < N; ++i) {
        const f32 s1 = f32(i * 137) * 0.00091f;
        const f32 s2 = f32(i * 293) * 0.00107f;
        const f32 s3 = f32(i * 419) * 0.00123f;
        const f32 px = x0 + std::fmod(s1 * w + t * (10.0f + s3 * 20.0f), w);
        const f32 py = y0 + std::fmod(s2 * h + t * (6.0f + s1 * 12.0f), h);
        const f32 tw = 0.5f + 0.5f * std::sin(t * (1.5f + s3 * 2.5f) + s1 * 20.0f);
        const u8 a = u8(alpha * (0.20f + 0.55f * tw));
        r.draw_circle({px, py}, 1.0f + s3 * 1.6f, with_alpha(color, a), 6);
    }
}

// Pulsing rings at the four card corner brackets.
inline void banner_corner_pulses(render::Renderer& r, f32 t,
                                 f32 x0, f32 y0, f32 x1, f32 y1,
                                 f32 alpha, Color color) {
    const f32 cxs[4] = {x0, x1, x0, x1};
    const f32 cys[4] = {y0, y0, y1, y1};
    for (int i = 0; i < 4; ++i) {
        const f32 phase = f32(i) * 0.4f;
        const f32 p = 0.5f + 0.5f * std::sin(t * 3.2f + phase);
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_circle({cxs[i], cys[i]}, 22.0f + p * 10.0f,
                      with_alpha(color, u8(alpha * (26 + 20 * p))), 18);
        r.set_layer(render::Renderer::Layer::Alpha);
    }
}

// Diagonal light streak that sweeps across the card.
inline void banner_shine_sweep(render::Renderer& r, f32 t,
                               f32 x0, f32 y0, f32 x1, f32 y1,
                               f32 alpha, Color color) {
    const f32 w = x1 - x0;
    const f32 h = y1 - y0;
    const f32 cycle = 3.2f;
    const f32 k = std::fmod(t, cycle) / cycle;
    if (k > 0.45f) return;
    const f32 sk = k / 0.45f;
    const f32 band = 90.0f;
    const f32 sx = x0 - band + (w + band * 2.0f) * sk;
    const f32 intensity = std::sin(sk * PI);
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_quad({sx, y0}, {sx + band, y0},
                {sx + band - h, y1}, {sx - h, y1},
                with_alpha(color, u8(alpha * 30.0f * intensity)));
    r.set_layer(render::Renderer::Layer::Alpha);
}

// Tiny top-down room mini-map: walls and door edges only.
inline void banner_room_mini(render::Renderer& r,
                             f32 cx, f32 cy, f32 size,
                             int wall_count, int door_count,
                             f32 alpha, Color accent, Color dim) {
    const f32 half = size * 0.5f;
    const f32 x0 = cx - half;
    const f32 y0 = cy - half;
    const f32 x1 = cx + half;
    const f32 y1 = cy + half;
    r.draw_quad({x0, y0}, {x1, y0}, {x1, y1}, {x0, y1},
                rgba(8, 22, 32, u8(alpha * 200)));
    r.draw_line({x0, y0}, {x1, y0}, 1.5f, with_alpha(accent, u8(alpha * 200)));
    r.draw_line({x1, y0}, {x1, y1}, 1.5f, with_alpha(accent, u8(alpha * 200)));
    r.draw_line({x1, y1}, {x0, y1}, 1.5f, with_alpha(accent, u8(alpha * 200)));
    r.draw_line({x0, y1}, {x0, y0}, 1.5f, with_alpha(accent, u8(alpha * 200)));
    r.draw_line({x0, y0 + 4.0f}, {x0 + size * 0.25f, y0 + 4.0f}, 2.5f,
                with_alpha(dim, u8(alpha * 200)));
    r.draw_line({x1 - size * 0.30f, y1 - 4.0f}, {x1, y1 - 4.0f}, 2.5f,
                with_alpha(dim, u8(alpha * 200)));
    const int dots = wall_count;
    for (int i = 0; i < dots && i < 6; ++i) {
        const f32 wx = x0 + size * (0.35f + 0.12f * f32(i % 3));
        const f32 wy = y0 + size * (0.35f + 0.15f * f32(i / 3));
        r.draw_circle({wx, wy}, 2.5f, with_alpha(accent, u8(alpha * 200)), 8);
    }
    for (int i = 0; i < door_count && i < 4; ++i) {
        const f32 dx = (i == 0) ? x0 + half * 0.2f : (i == 2) ? x1 - half * 0.2f : cx;
        const f32 dy = (i == 1) ? y0 + half * 0.2f : (i == 3) ? y1 - half * 0.2f : cy;
        const f32 px = (i == 0 || i == 2) ? dx : (i == 1 ? x1 : x0);
        const f32 py = (i == 0 ? y0 : (i == 2 ? y1 : dy));
        r.draw_circle({px, py}, 3.0f, with_alpha(COLOR_CORE_INNER, u8(alpha * 220)), 10);
    }
}

// Row of enemy-type icons for the current room.
inline void banner_threat_row(render::Renderer& r,
                              f32 x, f32 y,
                              int lancers, int bankers, int leads,
                              f32 alpha, f32 t) {
    f32 cursor = x;
    const f32 gap = 42.0f;
    const f32 sz  = 22.0f;
    const f32 pulse = 0.5f + 0.5f * std::sin(t * 4.0f);
    for (int i = 0; i < lancers; ++i) {
        render::icon_lancer(r, {cursor, y}, sz,
                            with_alpha(COLOR_ENEMY_EDGE, u8(alpha * 220)),
                            with_alpha(COLOR_ENEMY_CORE, u8(alpha * 240)));
        cursor += gap;
    }
    for (int i = 0; i < bankers; ++i) {
        render::icon_banker(r, {cursor, y}, sz,
                            with_alpha(COLOR_BANKER_EDGE, u8(alpha * 220)),
                            with_alpha(COLOR_BANKER_CORE, u8(alpha * 240)));
        cursor += gap;
    }
    for (int i = 0; i < leads; ++i) {
        render::icon_lead(r, {cursor, y}, sz,
                          with_alpha(COLOR_LEAD_EDGE, u8(alpha * 220)),
                          with_alpha(COLOR_LEAD_CORE, u8(alpha * 240)));
        cursor += gap;
    }
    if (lancers + bankers + leads > 0) {
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_circle({x + gap * 0.5f - 10.0f, y}, 30.0f + pulse * 8.0f,
                      with_alpha(COLOR_ENEMY_CORE, u8(alpha * (14 + 12 * pulse))), 24);
        r.set_layer(render::Renderer::Layer::Alpha);
    }
}

// Bright horizontal line that wipes down the card on entry and up on exit.
inline void banner_scanline_wipe(render::Renderer& r,
                                 f32 elapsed, f32 total,
                                 f32 card_y0, f32 card_y1,
                                 f32 alpha, Color color) {
    const f32 in_dur  = 0.35f;
    const f32 out_dur = 0.45f;
    if (elapsed < in_dur) {
        const f32 k = elapsed / in_dur;
        const f32 y = card_y0 + (card_y1 - card_y0) * k;
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_quad({0, y - 3.0f}, {VIRTUAL_W, y - 3.0f},
                    {VIRTUAL_W, y + 3.0f}, {0, y + 3.0f},
                    with_alpha(color, u8(alpha * 180.0f * (1.0f - k))));
        r.set_layer(render::Renderer::Layer::Alpha);
    }
    const f32 exit_start = total - out_dur;
    if (elapsed > exit_start) {
        const f32 k = (elapsed - exit_start) / out_dur;
        const f32 y = card_y1 - (card_y1 - card_y0) * k;
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_quad({0, y - 3.0f}, {VIRTUAL_W, y - 3.0f},
                    {VIRTUAL_W, y + 3.0f}, {0, y + 3.0f},
                    with_alpha(color, u8(alpha * 180.0f * k)));
        r.set_layer(render::Renderer::Layer::Alpha);
    }
}

// Large curved room-number background glyph behind the card.
inline void banner_ghost_number(render::Renderer& r,
                                f32 cx, f32 cy, int idx,
                                f32 alpha, Color color) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%02d", idx);
    const f32 w = r.text_width(buf, 38.0f);
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_text({cx - w * 0.5f, cy - 130.0f}, buf, 38.0f,
                with_alpha(color, u8(alpha * 18.0f)));
    r.set_layer(render::Renderer::Layer::Alpha);
}

// Big diagonal "CLEARED" stamp for the room-cleared celebration variant.
inline void banner_cleared_stamp(render::Renderer& r,
                                 f32 cx, f32 cy, f32 alpha, f32 t) {
    const f32 pulse = 0.85f + 0.15f * std::sin(t * 4.5f);
    const f32 w = 520.0f;
    const f32 h = 96.0f;
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_quad({cx - w * 0.5f, cy - h * 0.5f}, {cx + w * 0.5f, cy - h * 0.5f},
                {cx + w * 0.5f, cy + h * 0.5f}, {cx - w * 0.5f, cy + h * 0.5f},
                rgba(6, 40, 22, u8(alpha * 200)));
    r.draw_quad({cx - w * 0.5f, cy - h * 0.5f}, {cx + w * 0.5f, cy - h * 0.5f},
                {cx + w * 0.5f, cy - h * 0.5f + 4.0f}, {cx - w * 0.5f, cy - h * 0.5f + 4.0f},
                with_alpha(COLOR_CORE_INNER, u8(alpha * 220 * pulse)));
    r.draw_quad({cx - w * 0.5f, cy + h * 0.5f - 4.0f}, {cx + w * 0.5f, cy + h * 0.5f - 4.0f},
                {cx + w * 0.5f, cy + h * 0.5f}, {cx - w * 0.5f, cy + h * 0.5f},
                with_alpha(COLOR_CORE_INNER, u8(alpha * 220 * pulse)));
    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_text_centered({cx, cy - 26.0f}, "SECTOR CLEARED", 7.0f,
                         with_alpha(COLOR_CORE_INNER, u8(alpha * 255 * pulse)));
    char buf[48];
    std::snprintf(buf, sizeof(buf), "NO HOSTILES REMAIN");
    r.draw_text_centered({cx, cy + 40.0f}, buf, 2.2f,
                         with_alpha(COLOR_CORE_EDGE, u8(alpha * 220)));
}

// Horizontal scanning bar that runs a hot line across the title baseline.
inline void banner_title_scan(render::Renderer& r, f32 t,
                              f32 x0, f32 x1, f32 y,
                              f32 alpha, Color color) {
    const f32 cycle = 2.6f;
    const f32 k = std::fmod(t, cycle) / cycle;
    const f32 band = 80.0f;
    const f32 sx = x0 - band + (x1 - x0 + band * 2.0f) * k;
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_quad({sx - band * 0.5f, y - 1.5f}, {sx + band * 0.5f, y - 1.5f},
                {sx + band * 0.5f, y + 1.5f}, {sx - band * 0.5f, y + 1.5f},
                with_alpha(color, u8(alpha * 120)));
    r.set_layer(render::Renderer::Layer::Alpha);
}

// Chapter / iteration number strip on the top-right of the card.
inline void banner_chapter_marker(render::Renderer& r,
                                  f32 x, f32 y,
                                  int room_idx,
                                  f32 alpha, Color color) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "CH %02d", room_idx + 1);
    const f32 w = r.text_width(buf, 2.0f);
    r.draw_line({x - w - 20, y + 8}, {x - 12, y + 8}, 1.0f,
                with_alpha(color, u8(alpha * 160)));
    r.draw_text({x - w - 14, y - 2}, buf, 2.0f,
                with_alpha(color, u8(alpha * 220)));
    r.draw_circle({x - 6, y + 8}, 3.0f, with_alpha(color, u8(alpha * 240)), 10);
}

// Diagonal chevrons running along the top edge of the card.
inline void banner_top_chevrons(render::Renderer& r, f32 t,
                                f32 x0, f32 y, f32 x1,
                                f32 alpha, Color color) {
    const f32 spacing = 46.0f;
    const f32 span = x1 - x0;
    const int count = int(span / spacing);
    for (int i = 0; i < count; ++i) {
        const f32 cx = x0 + spacing * 0.5f + f32(i) * spacing;
        const f32 ph = f32(i) * 0.5f;
        const f32 pulse = 0.5f + 0.5f * std::sin(t * 2.5f + ph);
        const u8 a = u8(alpha * (40 + 120 * pulse));
        render::icon_chevron(r, {cx, y}, 12.0f, 0.0f, with_alpha(color, a));
    }
}

// Bottom edge meter showing room clear status as small ticks.
inline void banner_status_ticks(render::Renderer& r,
                                f32 x0, f32 y, f32 x1,
                                int filled, int total,
                                f32 alpha, Color color) {
    const f32 span = x1 - x0;
    const f32 step = span / f32(total);
    for (int i = 0; i < total; ++i) {
        const f32 tx = x0 + step * (f32(i) + 0.5f);
        if (i < filled) {
            r.draw_quad({tx - 6.0f, y - 3.0f}, {tx + 6.0f, y - 3.0f},
                        {tx + 6.0f, y + 3.0f}, {tx - 6.0f, y + 3.0f},
                        with_alpha(color, u8(alpha * 220)));
        } else {
            r.draw_line({tx - 6.0f, y}, {tx + 6.0f, y}, 1.5f,
                        with_alpha(color, u8(alpha * 80)));
        }
    }
}

// Slow bobbing orbit ring around the hex badge.
inline void banner_badge_orbit(render::Renderer& r,
                               f32 cx, f32 cy, f32 radius, f32 t,
                               f32 alpha, Color color) {
    const f32 ang = t * 1.4f;
    for (int i = 0; i < 3; ++i) {
        const f32 a = ang + TAU * f32(i) / 3.0f;
        const Vec2 p = {cx + std::cos(a) * radius, cy + std::sin(a) * radius};
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_circle(p, 10.0f, with_alpha(color, u8(alpha * 80)), 12);
        r.set_layer(render::Renderer::Layer::Alpha);
        r.draw_circle(p, 3.0f, with_alpha(color, u8(alpha * 240)), 10);
    }
    r.draw_ring({cx, cy}, radius - 0.5f, radius + 0.5f,
                with_alpha(color, u8(alpha * 60)), 40);
}

// Word-wrap text to fit ~chars_per_line. Returns number of lines used.
inline int banner_wrap_text(const char* text, int chars_per_line,
                            char lines[][128], int max_lines) {
    int line = 0, col = 0, i = 0;
    lines[0][0] = 0;
    while (text[i] && line < max_lines) {
        int ws = i;
        while (text[i] && text[i] != ' ') ++i;
        const int wlen = i - ws;
        if (text[i] == ' ') ++i;
        if (col > 0 && col + 1 + wlen > chars_per_line) {
            if (++line >= max_lines) break;
            col = 0;
            lines[line][0] = 0;
        }
        if (col > 0 && col < 126) lines[line][col++] = ' ';
        for (int k = 0; k < wlen && col < 127; ++k) lines[line][col++] = text[ws + k];
        lines[line][col] = 0;
    }
    return line + 1;
}
} // namespace inc

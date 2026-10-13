#pragma once

// =============================================================================
// levelup_fx.hpp - the level-up celebration banner.
// =============================================================================

#include <cmath>
#include <cstdio>

#include "core/color.hpp"
#include "core/math.hpp"
#include "core/vec2.hpp"
#include "render/renderer.hpp"

namespace inc::levelup_fx {

inline void draw_banner(render::Renderer& r, int level,
                        f32 elapsed, f32 total) {
    const f32 cx = f32(VIRTUAL_W) * 0.5f;
    const f32 cy = f32(VIRTUAL_H) * 0.42f;

    f32 alpha = 1.0f;
    if (elapsed < 0.25f)               alpha = elapsed / 0.25f;
    else if (elapsed > total - 0.55f)  alpha = (total - elapsed) / 0.55f;
    alpha = clamp(alpha, 0.0f, 1.0f);
    if (alpha <= 0.0f) return;

    // Dim the world behind the banner
    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({0, 0}, {f32(VIRTUAL_W), 0},
                {f32(VIRTUAL_W), f32(VIRTUAL_H)}, {0, f32(VIRTUAL_H)},
                rgba(2, 4, 8, u8(alpha * 130.0f)));

    // Banner sweep from edges
    const f32 sweep_age = clamp(elapsed / 0.35f, 0.0f, 1.0f);
    const f32 sweep_ease = 1.0f - (1.0f - sweep_age) * (1.0f - sweep_age);
    const f32 half_w = sweep_ease * f32(VIRTUAL_W) * 0.45f;
    const f32 bh = 220.0f;
    const f32 top = cy - bh * 0.5f;
    const f32 bot = cy + bh * 0.5f;
    const f32 left = cx - half_w;
    const f32 right = cx + half_w;

    r.draw_quad({left, top}, {right, top}, {right, bot}, {left, bot},
                rgba(6, 14, 22, u8(alpha * 210.0f)));

    const Color edge_hi = with_alpha(COLOR_CORE_INNER, u8(alpha * 255.0f));
    r.draw_line({left, top}, {right, top}, 2.0f, edge_hi);
    r.draw_line({left, bot}, {right, bot}, 2.0f, edge_hi);

    // Edge notches
    const f32 notch = 22.0f;
    r.draw_line({left,  top}, {left,  top + notch}, 3.0f, edge_hi);
    r.draw_line({left,  bot}, {left,  bot - notch}, 3.0f, edge_hi);
    r.draw_line({right, top}, {right, top + notch}, 3.0f, edge_hi);
    r.draw_line({right, bot}, {right, bot - notch}, 3.0f, edge_hi);

    // ---- Title "LEVEL UP" - letter stagger ----
    {
        const char* title = "LEVEL UP";
        const int tlen = 8;
        const f32 tsize = 6.5f;
        const f32 tadv  = r.text_width("XX", tsize) - r.text_width("X", tsize);
        const f32 tw    = r.text_width(title, tsize);
        const f32 ty    = top + 38.0f;
        f32 px = cx - tw * 0.5f;
        for (int i = 0; i < tlen; ++i) {
            const f32 enter = clamp((elapsed - (0.15f + f32(i) * 0.035f)) / 0.30f, 0.0f, 1.0f);
            if (enter > 0.0f) {
                char one[2] = { title[i], 0 };
                const f32 slide = (1.0f - enter) * -18.0f;
                r.draw_text({px + 3.0f, ty + slide + 2.0f}, one, tsize,
                            with_alpha(COLOR_ENEMY_EDGE, u8(alpha * enter * 60.0f)));
                r.draw_text({px - 3.0f, ty + slide - 2.0f}, one, tsize,
                            with_alpha(COLOR_CORE_INNER, u8(alpha * enter * 60.0f)));
                r.draw_text({px, ty + slide}, one, tsize,
                            with_alpha(COLOR_CORE_INNER, u8(alpha * enter * 255.0f)));
            }
            px += tadv;
        }
    }

    // ---- Big numeral + rotating hex badge ----
    {
        const f32 num_age = clamp((elapsed - 0.40f) / 0.35f, 0.0f, 1.0f);
        if (num_age > 0.0f) {
            const f32 ease   = 1.0f - (1.0f - num_age) * (1.0f - num_age);
            const f32 bcx    = cx;
            const f32 bcy    = cy + 8.0f;
            const f32 badge_r = 68.0f * ease;
            const f32 rot    = elapsed * 0.55f;

            r.draw_ring({bcx, bcy}, badge_r - 4.0f, badge_r,
                        with_alpha(COLOR_CORE_EDGE, u8(alpha * 170.0f)), 32);

            for (int k = 0; k < 6; ++k) {
                const f32 a0 = rot + TAU * f32(k) / 6.0f;
                const f32 a1 = rot + TAU * f32(k + 1) / 6.0f;
                const Vec2 p0 {bcx + std::cos(a0) * badge_r, bcy + std::sin(a0) * badge_r};
                const Vec2 p1 {bcx + std::cos(a1) * badge_r, bcy + std::sin(a1) * badge_r};
                r.draw_line(p0, p1, 2.5f, with_alpha(COLOR_CORE_EDGE, u8(alpha * 220.0f)));
            }

            char nbuf[8];
            std::snprintf(nbuf, sizeof(nbuf), "%02d", level);
            const f32 nsize = 10.0f;
            const f32 nw = r.text_width(nbuf, nsize);
            r.set_layer(render::Renderer::Layer::Additive);
            r.draw_circle({bcx, bcy}, badge_r * 1.7f,
                          with_alpha(COLOR_CORE_EDGE, u8(alpha * 55.0f)), 32);
            r.set_layer(render::Renderer::Layer::Alpha);
            r.draw_text({bcx - nw * 0.5f, bcy - nsize * 3.5f}, nbuf, nsize,
                        with_alpha(COLOR_CORE_INNER, u8(alpha * 255.0f)));
        }
    }

    // ---- Chips: +1 STAT / +1 SKILL ----
    {
        const f32 chip_age = clamp((elapsed - 0.65f) / 0.30f, 0.0f, 1.0f);
        if (chip_age > 0.0f) {
            const f32 ease = 1.0f - (1.0f - chip_age) * (1.0f - chip_age);
            const f32 chip_y = bot - 46.0f;
            const f32 chip_h = 30.0f;
            const char* l1 = "+1 STAT POINT";
            const char* l2 = "+1 SKILL POINT";
            const f32 c1w = r.text_width(l1, 2.2f) + 40.0f;
            const f32 c2w = r.text_width(l2, 2.2f) + 40.0f;
            const f32 gap = 20.0f;
            const f32 total_w = c1w + c2w + gap;
            const f32 c1x = cx - total_w * 0.5f;
            const f32 c2x = c1x + c1w + gap;
            const f32 slide1 = (1.0f - ease) * -40.0f;
            const f32 slide2 = (1.0f - ease) *  40.0f;

            r.draw_quad({c1x + slide1, chip_y - chip_h * 0.5f},
                        {c1x + c1w + slide1, chip_y - chip_h * 0.5f},
                        {c1x + c1w + slide1, chip_y + chip_h * 0.5f},
                        {c1x + slide1, chip_y + chip_h * 0.5f},
                        with_alpha(COLOR_CORE_EDGE, u8(alpha * 60.0f)));
            r.draw_line({c1x + slide1, chip_y - chip_h * 0.5f},
                        {c1x + c1w + slide1, chip_y - chip_h * 0.5f}, 1.5f,
                        with_alpha(COLOR_CORE_INNER, u8(alpha * 200.0f)));
            r.draw_line({c1x + slide1, chip_y + chip_h * 0.5f},
                        {c1x + c1w + slide1, chip_y + chip_h * 0.5f}, 1.5f,
                        with_alpha(COLOR_CORE_INNER, u8(alpha * 200.0f)));
            r.draw_text({c1x + 20.0f + slide1, chip_y - 9.0f}, l1, 2.2f,
                        with_alpha(COLOR_CORE_INNER, u8(alpha * 240.0f)));

            r.draw_quad({c2x + slide2, chip_y - chip_h * 0.5f},
                        {c2x + c2w + slide2, chip_y - chip_h * 0.5f},
                        {c2x + c2w + slide2, chip_y + chip_h * 0.5f},
                        {c2x + slide2, chip_y + chip_h * 0.5f},
                        with_alpha(COLOR_CORE_EDGE, u8(alpha * 60.0f)));
            r.draw_line({c2x + slide2, chip_y - chip_h * 0.5f},
                        {c2x + c2w + slide2, chip_y - chip_h * 0.5f}, 1.5f,
                        with_alpha(COLOR_CORE_INNER, u8(alpha * 200.0f)));
            r.draw_line({c2x + slide2, chip_y + chip_h * 0.5f},
                        {c2x + c2w + slide2, chip_y + chip_h * 0.5f}, 1.5f,
                        with_alpha(COLOR_CORE_INNER, u8(alpha * 200.0f)));
            r.draw_text({c2x + 20.0f + slide2, chip_y - 9.0f}, l2, 2.2f,
                        with_alpha(COLOR_CORE_INNER, u8(alpha * 240.0f)));
        }
    }

    // ---- Radial pulse rings from center ----
    for (int i = 0; i < 3; ++i) {
        const f32 phase = 0.15f + f32(i) * 0.22f;
        const f32 k = clamp((elapsed - phase) / 1.0f, 0.0f, 1.0f);
        if (k > 0.0f && k < 1.0f) {
            const f32 radius = 80.0f + k * 620.0f;
            const f32 ring_a = (1.0f - k) * alpha * 110.0f;
            r.set_layer(render::Renderer::Layer::Additive);
            r.draw_ring({cx, cy}, radius - 2.0f, radius + 2.0f,
                        with_alpha(COLOR_CORE_EDGE, u8(ring_a)), 48);
            r.set_layer(render::Renderer::Layer::Alpha);
        }
    }

    // ---- Corner brackets ----
    {
        const f32 arm = 40.0f;
        const f32 off = 6.0f;
        const f32 lx = left + off;
        const f32 rx = right - off;
        const f32 ty = top + off;
        const f32 by = bot - off;
        const Color bc = with_alpha(COLOR_CORE_INNER, u8(alpha * 240.0f));
        r.draw_line({lx, ty}, {lx + arm, ty}, 2.5f, bc);
        r.draw_line({lx, ty}, {lx, ty + arm}, 2.5f, bc);
        r.draw_line({rx, ty}, {rx - arm, ty}, 2.5f, bc);
        r.draw_line({rx, ty}, {rx, ty + arm}, 2.5f, bc);
        r.draw_line({lx, by}, {lx + arm, by}, 2.5f, bc);
        r.draw_line({lx, by}, {lx, by - arm}, 2.5f, bc);
        r.draw_line({rx, by}, {rx - arm, by}, 2.5f, bc);
        r.draw_line({rx, by}, {rx, by - arm}, 2.5f, bc);
    }

    // ---- "PROMOTION" tab above the banner ----
    {
        const char* tab = "PROMOTION";
        const f32 tab_w = r.text_width(tab, 1.8f) + 24.0f;
        const f32 tab_h = 26.0f;
        r.draw_quad({cx - tab_w * 0.5f, top - tab_h + 2.0f},
                    {cx + tab_w * 0.5f, top - tab_h + 2.0f},
                    {cx + tab_w * 0.5f, top + 2.0f},
                    {cx - tab_w * 0.5f, top + 2.0f},
                    rgba(6, 14, 22, u8(alpha * 235.0f)));
        r.draw_line({cx - tab_w * 0.5f, top - tab_h + 2.0f},
                    {cx + tab_w * 0.5f, top - tab_h + 2.0f}, 1.5f,
                    with_alpha(COLOR_CORE_EDGE, u8(alpha * 220.0f)));
        const f32 txtw = r.text_width(tab, 1.8f);
        r.draw_text({cx - txtw * 0.5f, top - tab_h + 8.0f}, tab, 1.8f,
                    with_alpha(COLOR_CORE_EDGE, u8(alpha * 240.0f)));
    }
}

} // namespace inc::levelup_fx

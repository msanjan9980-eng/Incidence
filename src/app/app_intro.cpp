#include "app/app.hpp"

#include <cmath>
#include <cstdio>

namespace inc {

// Local copy of the intro card data. Duplicated here rather than shared
// because app.cpp's copies are file-static. Small enough to duplicate.
struct IntroCard {
    const char* small;
    const char* large;
    f32         duration;
    Color       accent;
};

static constexpr IntroCard INTRO_CARDS[] = {
    { "HALCYON STATION",                                 "YEAR 207",              3.5f, COLOR_CORE_EDGE },
    { "Two hundred years ago, the ring folded space.",   "",                      3.5f, COLOR_FACET_EDGE },
    { "It worked.",                                      "Everyone died.",        3.5f, COLOR_ENEMY_EDGE },
    { "The station AI, THEOREM, is still running.",      "PRESERVE THE RING.",    4.0f, COLOR_ENEMY_CORE },
    { "You are KITE-9, the last functioning deflector.", "YOU HAVE NO GUN.",      4.0f, COLOR_CORE_EDGE },
    { "You have a surface. The surface obeys one law.",  "REFLECT.",              4.0f, COLOR_FACET_EDGE },
};
static constexpr int INTRO_CARD_COUNT = sizeof(INTRO_CARDS) / sizeof(INTRO_CARDS[0]);

static void intro_polish_scanlines(render::Renderer& rr, f32 t, f32 a) {
    constexpr int N = 3;
    for (int i = 0; i < N; ++i) {
        const f32 speed = 40.0f + f32(i) * 15.0f;
        const f32 y = std::fmod(t * speed + f32(i) * 300.0f, f32(VIRTUAL_H));
        rr.set_layer(render::Renderer::Layer::Additive);
        rr.draw_quad({0, y - 1.5f}, {VIRTUAL_W, y - 1.5f},
                     {VIRTUAL_W, y + 1.5f}, {0, y + 1.5f},
                     with_alpha(COLOR_CORE_EDGE, u8(25 * a)));
        rr.set_layer(render::Renderer::Layer::Alpha);
    }
    for (int gy = 0; gy < 8; ++gy) {
        for (int gx = 0; gx < 14; ++gx) {
            const f32 h1 = f32(gx * 17 + gy * 31) * 0.13f;
            const f32 h2 = f32(gx * 7 + gy * 11) * 0.29f;
            const f32 dx = std::fmod(h1 * VIRTUAL_W + t * 6.0f, f32(VIRTUAL_W));
            const f32 dy = f32(gy) * f32(VIRTUAL_H) / 8.0f + std::sin(t * 0.4f + h2) * 6.0f;
            rr.draw_circle({dx, dy}, 1.0f + h2 * 1.5f,
                           with_alpha(COLOR_CORE_EDGE, u8(50 * a)), 6);
        }
    }
}

static void intro_polish_ambient(render::Renderer& rr, f32 t, f32 a, Color accent) {
    for (int i = 0; i < 22; ++i) {
        const f32 h1 = f32(i * 131) * 0.00091f;
        const f32 h2 = f32(i * 419) * 0.00103f;
        const f32 h3 = f32(i * 293) * 0.00117f;
        const f32 x = h1 * VIRTUAL_W;
        const f32 y = h2 * VIRTUAL_H;
        const f32 tw = 0.5f + 0.5f * std::sin(t * (1.0f + h3 * 3.0f) + h1 * 30.0f);
        const u8 al = u8(180.0f * a * (0.3f + 0.7f * tw));
        rr.draw_circle({x, y}, 1.5f + h3 * 1.5f, with_alpha(accent, al), 8);
    }
    rr.set_layer(render::Renderer::Layer::Additive);
    rr.draw_circle({0.0f, 0.0f}, 260.0f, with_alpha(accent, u8(18 * a)), 32);
    rr.draw_circle({f32(VIRTUAL_W), 0.0f}, 260.0f, with_alpha(accent, u8(18 * a)), 32);
    rr.draw_circle({0.0f, f32(VIRTUAL_H)}, 260.0f, with_alpha(accent, u8(18 * a)), 32);
    rr.draw_circle({f32(VIRTUAL_W), f32(VIRTUAL_H)}, 260.0f, with_alpha(accent, u8(18 * a)), 32);
    rr.set_layer(render::Renderer::Layer::Alpha);
}

void App::draw_intro_background(render::Renderer& rr, int card_index, f32 card_t) const {
    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 cy = VIRTUAL_H * 0.32f;
    const f32 t  = card_t;
    rr.set_layer(render::Renderer::Layer::Alpha);
    switch (card_index) {
    case 0: {
        const f32 R = 165.0f;
        const f32 slow = t * 0.15f;
        rr.draw_ring({cx, cy}, R - 3.0f, R, with_alpha(COLOR_CORE_EDGE, 180), 72);
        rr.draw_ring({cx, cy}, R - 32.0f, R - 26.0f, with_alpha(COLOR_CORE_EDGE, 90), 72);
        for (int i = 0; i < 24; ++i) {
            const f32 a = TAU * f32(i) / 24.0f + slow;
            const Vec2 p0 = {cx + std::cos(a) * (R + 4.0f), cy + std::sin(a) * (R + 4.0f)};
            const Vec2 p1 = {cx + std::cos(a) * (R + 12.0f), cy + std::sin(a) * (R + 12.0f)};
            const u8 al = (i % 6 == 0) ? 200 : 90;
            rr.draw_line(p0, p1, (i % 6 == 0) ? 2.5f : 1.0f, with_alpha(COLOR_CORE_EDGE, al));
        }
        for (int i = 0; i < 6; ++i) {
            const f32 a = TAU * f32(i) / 6.0f + slow * 0.7f;
            const f32 r = R * 0.55f;
            const Vec2 p = {cx + std::cos(a) * r, cy + std::sin(a) * r};
            rr.draw_circle(p, 6.0f, with_alpha(COLOR_CORE_INNER, 200), 10);
            rr.draw_line({cx, cy}, p, 1.0f, with_alpha(COLOR_CORE_EDGE, 60));
        }
        for (int i = 0; i < 4; ++i) {
            const f32 a = TAU * f32(i) / 4.0f + slow * 0.3f;
            const Vec2 a0 = {cx + std::cos(a) * 40.0f, cy + std::sin(a) * 40.0f};
            const Vec2 a1 = {cx + std::cos(a) * (R - 30.0f), cy + std::sin(a) * (R - 30.0f)};
            rr.draw_line(a0, a1, 1.5f, with_alpha(COLOR_CORE_EDGE, 110));
        }
        const f32 ma = t * 0.9f;
        const Vec2 mp = {cx + std::cos(ma) * R, cy + std::sin(ma) * R};
        rr.set_layer(render::Renderer::Layer::Additive);
        rr.draw_circle(mp, 14.0f, with_alpha(COLOR_CORE_INNER, 80), 16);
        rr.set_layer(render::Renderer::Layer::Alpha);
        rr.draw_circle(mp, 6.0f, COLOR_CORE_INNER, 12);
        rr.draw_circle({cx, cy}, 26.0f, COLOR_CORE_FILL, 24);
        rr.draw_ring  ({cx, cy}, 26.0f, 30.0f, COLOR_CORE_EDGE, 28);
        rr.draw_circle({cx, cy}, 8.0f, COLOR_CORE_INNER, 14);
        rr.draw_circle({cx, cy}, 4.0f, COLOR_CORE_EDGE, 10);
        break;
    }
    case 1: {
        for (int i = 0; i < 12; ++i) {
            const f32 a = TAU * f32(i) / 12.0f;
            const f32 warp = 0.4f + 0.6f * std::sin(t * 0.8f + f32(i) * 0.4f);
            const f32 r0 = 240.0f * warp;
            const f32 r1 = 30.0f + 60.0f * (1.0f - warp);
            const Vec2 p0 = {cx + std::cos(a) * r0, cy + std::sin(a) * r0};
            const Vec2 p1 = {cx + std::cos(a) * r1, cy + std::sin(a) * r1};
            rr.draw_line(p0, p1, 1.5f, with_alpha(COLOR_FACET_EDGE, u8(60 + 100 * warp)));
        }
        for (int i = 0; i < 8; ++i) {
            const f32 aa = TAU * f32(i) / 8.0f + t * 0.2f;
            for (int j = 1; j < 5; ++j) {
                const f32 r = 90.0f + f32(j) * 50.0f;
                const Vec2 p = {cx + std::cos(aa) * r, cy + std::sin(aa) * r};
                rr.draw_circle(p, 2.5f, with_alpha(COLOR_FACET_EDGE, 100), 8);
            }
        }
        const f32 pulse = 0.5f + 0.5f * std::sin(t * 3.0f);
        rr.set_layer(render::Renderer::Layer::Additive);
        rr.draw_circle({cx, cy}, 60.0f + pulse * 20.0f, with_alpha(COLOR_CORE_EDGE, 40), 40);
        rr.set_layer(render::Renderer::Layer::Alpha);
        rr.draw_circle({cx, cy}, 18.0f + pulse * 6.0f, COLOR_CORE_INNER, 24);
        rr.draw_circle({cx, cy}, 6.0f, COLOR_VOID, 10);
        break;
    }
    case 2: {
        const f32 wave = 40.0f + t * 300.0f;
        const f32 wave_a = clamp(1.0f - t / 3.2f, 0.0f, 1.0f);
        rr.draw_ring({cx, cy}, wave - 4.0f, wave + 4.0f,
                     with_alpha(COLOR_ENEMY_EDGE, u8(wave_a * 200)), 72);
        for (int i = 0; i < 24; ++i) {
            const f32 seed = f32(i) * 0.41f;
            const f32 a = TAU * f32(i) / 24.0f + seed;
            const f32 speed = 30.0f + std::fmod(seed * 47.0f, 40.0f);
            const f32 r = 30.0f + t * speed;
            const Vec2 p = {cx + std::cos(a) * r, cy + std::sin(a) * r};
            const f32 sz = 3.0f + std::fmod(seed * 13.0f, 4.0f);
            const u8 al = u8(200.0f * clamp(1.0f - t / 3.2f, 0.0f, 1.0f));
            rr.draw_quad({p.x - sz, p.y - sz}, {p.x + sz, p.y - sz},
                         {p.x + sz, p.y + sz}, {p.x - sz, p.y + sz},
                         with_alpha(COLOR_ENEMY_FILL, al));
            rr.draw_line(p, {p.x - std::cos(a) * 8.0f, p.y - std::sin(a) * 8.0f},
                         1.0f, with_alpha(COLOR_ENEMY_EDGE, u8(al * 0.6f)));
        }
        const f32 cf = clamp(1.0f - t / 3.5f, 0.0f, 1.0f);
        rr.set_layer(render::Renderer::Layer::Additive);
        rr.draw_circle({cx, cy}, 80.0f * cf, with_alpha(COLOR_ENEMY_CORE, u8(cf * 60)), 40);
        rr.set_layer(render::Renderer::Layer::Alpha);
        rr.draw_circle({cx, cy}, 24.0f * cf, COLOR_ENEMY_CORE, 20);
        rr.draw_circle({cx, cy}, 8.0f * cf, COLOR_VOID, 10);
        break;
    }
    case 3: {
        const f32 rot = t * 0.35f;
        Vec2 outer[6] = {};
        for (int i = 0; i < 6; ++i) {
            const f32 a = rot + TAU * f32(i) / 6.0f;
            outer[i] = {cx + std::cos(a) * 180.0f, cy + std::sin(a) * 180.0f};
        }
        for (int i = 0; i < 6; ++i) {
            rr.draw_line(outer[i], outer[(i+1)%6], 2.0f, with_alpha(COLOR_ENEMY_EDGE, 180));
        }
        Vec2 mid[3] = {};
        for (int i = 0; i < 3; ++i) {
            const f32 a = -rot * 1.8f + TAU * f32(i) / 3.0f;
            mid[i] = {cx + std::cos(a) * 100.0f, cy + std::sin(a) * 100.0f};
        }
        for (int i = 0; i < 3; ++i) {
            rr.draw_line(mid[i], mid[(i+1)%3], 2.5f, with_alpha(COLOR_ENEMY_CORE, 220));
        }
        Vec2 inn[3] = {};
        for (int i = 0; i < 3; ++i) {
            const f32 a = -rot * 2.6f + PI + TAU * f32(i) / 3.0f;
            inn[i] = {cx + std::cos(a) * 55.0f, cy + std::sin(a) * 55.0f};
        }
        for (int i = 0; i < 3; ++i) {
            rr.draw_line(inn[i], inn[(i+1)%3], 1.5f, with_alpha(COLOR_ENEMY_FILL, 200));
        }
        const f32 pl = 0.5f + 0.5f * std::sin(t * 4.0f);
        rr.draw_ring({cx, cy}, 30.0f + pl * 5.0f, 34.0f + pl * 5.0f,
                     with_alpha(COLOR_ENEMY_CORE, 200), 24);
        rr.draw_circle({cx, cy}, 22.0f, COLOR_ENEMY_CORE, 22);
        rr.draw_circle({cx, cy}, 10.0f, COLOR_VOID, 14);
        rr.draw_circle({cx, cy}, 4.0f, COLOR_ENEMY_CORE, 8);
        const f32 blink = std::fabs(std::sin(t * 0.55f));
        if (blink < 0.12f) {
            rr.draw_quad({cx - 90.0f, cy - 4.0f}, {cx + 90.0f, cy - 4.0f},
                         {cx + 90.0f, cy + 4.0f}, {cx - 90.0f, cy + 4.0f},
                         with_alpha(COLOR_ENEMY_FILL, 220));
        }
        break;
    }
    case 4: {
        const f32 rot = t * 0.25f;
        const f32 R_IN  = 80.0f;
        const f32 R_OUT = 96.0f;
        const f32 span  = 2.44f;
        const f32 heading = -HALF_PI + rot * 0.1f;
        for (int i = 0; i < 5; ++i) {
            const f32 step = span / 5.0f;
            const f32 a0 = heading - span * 0.5f + f32(i)     * step;
            const f32 a1 = heading - span * 0.5f + f32(i + 1) * step;
            const Vec2 pin0  = {cx + std::cos(a0) * R_IN,  cy + std::sin(a0) * R_IN};
            const Vec2 pin1  = {cx + std::cos(a1) * R_IN,  cy + std::sin(a1) * R_IN};
            const Vec2 pout0 = {cx + std::cos(a0) * R_OUT, cy + std::sin(a0) * R_OUT};
            const Vec2 pout1 = {cx + std::cos(a1) * R_OUT, cy + std::sin(a1) * R_OUT};
            rr.draw_quad(pin0, pin1, pout1, pout0, COLOR_FACET_FILL);
            rr.draw_line(pin0, pin1, 2.0f, COLOR_FACET_EDGE);
            rr.draw_line(pout0, pout1, 3.0f, COLOR_FACET_EDGE);
            rr.draw_line(pin0, pout0, 1.2f, COLOR_FACET_EDGE);
            rr.draw_line(pin1, pout1, 1.2f, COLOR_FACET_EDGE);
            const f32 mm = (a0 + a1) * 0.5f;
            const Vec2 nn = {std::cos(mm), std::sin(mm)};
            rr.draw_line({cx + nn.x * (R_OUT + 4.0f), cy + nn.y * (R_OUT + 4.0f)},
                         {cx + nn.x * (R_OUT + 20.0f), cy + nn.y * (R_OUT + 20.0f)},
                         2.0f, COLOR_FACET_NORMAL);
        }
        rr.draw_ring({cx, cy}, R_OUT + 30.0f, R_OUT + 32.0f,
                     with_alpha(COLOR_FACET_EDGE, 60), 60);
        const f32 pl2 = 0.5f + 0.5f * std::sin(t * 2.5f);
        rr.set_layer(render::Renderer::Layer::Additive);
        rr.draw_circle({cx, cy}, 40.0f + pl2 * 10.0f, with_alpha(COLOR_CORE_EDGE, 40), 40);
        rr.set_layer(render::Renderer::Layer::Alpha);
        rr.draw_circle({cx, cy}, 20.0f, COLOR_CORE_FILL, 24);
        rr.draw_ring  ({cx, cy}, 20.0f, 24.0f, COLOR_CORE_EDGE, 28);
        rr.draw_circle({cx, cy}, 6.0f, COLOR_CORE_INNER, 12);
        rr.draw_circle({cx + std::cos(heading) * 30.0f, cy + std::sin(heading) * 30.0f},
                       4.0f, COLOR_CORE_INNER, 10);
        break;
    }
    case 5: {
        const f32 n_ang = -PI * 0.25f;
        const Vec2 n {std::cos(n_ang), std::sin(n_ang)};
        const Vec2 tan_ {-n.y, n.x};
        const Vec2 m0 {cx - tan_.x * 100.0f, cy - tan_.y * 100.0f};
        const Vec2 m1 {cx + tan_.x * 100.0f, cy + tan_.y * 100.0f};
        rr.draw_line(m0, m1, 8.0f, COLOR_FACET_FILL);
        rr.draw_line(m0, m1, 3.0f, COLOR_FACET_EDGE);
        rr.draw_line(m0, m1, 1.5f, COLOR_CORE_INNER);
        for (int i = 0; i < 8; ++i) {
            const f32 d0 = 12.0f + f32(i) * 16.0f;
            rr.draw_line({cx + n.x * d0, cy + n.y * d0},
                         {cx + n.x * (d0 + 8.0f), cy + n.y * (d0 + 8.0f)},
                         1.8f, with_alpha(COLOR_FACET_NORMAL, 200));
        }
        const Vec2 in_dir {1.0f, 0.35f};
        const f32 in_len = std::sqrt(in_dir.x * in_dir.x + in_dir.y * in_dir.y);
        const Vec2 in_unit {in_dir.x / in_len, in_dir.y / in_len};
        const f32 in_dist = 420.0f;
        const f32 in_pos = 320.0f - std::fmod(t * 280.0f, in_dist + 80.0f);
        const Vec2 bp {cx - in_unit.x * in_pos, cy - in_unit.y * in_pos};
        rr.draw_line({cx - in_unit.x * (in_pos + 60.0f), cy - in_unit.y * (in_pos + 60.0f)},
                     bp, 3.0f, with_alpha(COLOR_BULLET_CORE, 180));
        rr.draw_circle(bp, 8.0f, COLOR_BULLET_CORE, 14);
        rr.set_layer(render::Renderer::Layer::Additive);
        rr.draw_circle(bp, 20.0f, with_alpha(COLOR_BULLET_GLOW, 100), 16);
        rr.set_layer(render::Renderer::Layer::Alpha);
        const f32 dp = in_dir.x * n.x + in_dir.y * n.y;
        const Vec2 out_dir {in_dir.x - 2.0f * dp * n.x, in_dir.y - 2.0f * dp * n.y};
        const f32 out_len = std::sqrt(out_dir.x * out_dir.x + out_dir.y * out_dir.y);
        const Vec2 out_unit {out_dir.x / out_len, out_dir.y / out_len};
        const f32 out_pos = std::fmod(t * 320.0f, in_dist + 80.0f);
        rr.draw_line({cx, cy}, {cx + out_unit.x * out_pos, cy + out_unit.y * out_pos},
                     3.0f, with_alpha(COLOR_CORE_INNER, 220));
        const f32 ang_in  = std::atan2(-in_unit.y, -in_unit.x);
        const f32 ang_ref = std::atan2(out_unit.y, out_unit.x);
        const f32 ang_n   = std::atan2(n.y, n.x);
        rr.draw_arc_ring({cx, cy}, 60.0f, 64.0f, ang_in, ang_n, COLOR_BULLET_CORE, 14);
        rr.draw_arc_ring({cx, cy}, 60.0f, 64.0f, ang_n, ang_ref, COLOR_CORE_INNER, 14);
        const f32 imp = 0.5f + 0.5f * std::sin(t * 6.0f);
        rr.set_layer(render::Renderer::Layer::Additive);
        rr.draw_circle({cx, cy}, 30.0f + imp * 10.0f,
                       with_alpha(COLOR_CORE_INNER, u8(120 + 60 * imp)), 24);
        rr.set_layer(render::Renderer::Layer::Alpha);
        break;
    }
    }
}

void App::draw_intro(render::Renderer& rr) const {
    f32 t = intro_time_;
    int idx = -1; f32 ct = 0;
    for (int i = 0; i < INTRO_CARD_COUNT; ++i) {
        if (t < INTRO_CARDS[i].duration) { idx = i; ct = t; break; }
        t -= INTRO_CARDS[i].duration;
    }
    if (idx < 0) return;
    const IntroCard& c = INTRO_CARDS[idx];
    const f32 p = ct / c.duration;
    f32 a = 1.0f;
    if (p < 0.10f)       a = p / 0.10f;
    else if (p > 0.90f)  a = (1.0f - p) / 0.10f;
    a = clamp(a, 0.0f, 1.0f);

    rr.set_layer(render::Renderer::Layer::Alpha);
    rr.draw_quad({0,0}, {VIRTUAL_W,0}, {VIRTUAL_W,VIRTUAL_H}, {0,VIRTUAL_H}, rgba(3,8,14,255));
    draw_intro_background(rr, idx, ct);
    intro_polish_scanlines(rr, t, a);
    intro_polish_ambient(rr, t, a, c.accent);
    rr.set_layer(render::Renderer::Layer::Alpha);

    rr.draw_quad({0,0}, {VIRTUAL_W,0}, {VIRTUAL_W,VIRTUAL_H*0.18f}, {0,VIRTUAL_H*0.18f}, rgba(0,0,0,200));
    rr.draw_quad({0,VIRTUAL_H*0.82f}, {VIRTUAL_W,VIRTUAL_H*0.82f}, {VIRTUAL_W,VIRTUAL_H}, {0,VIRTUAL_H}, rgba(0,0,0,200));

    char cnt[16];
    std::snprintf(cnt, sizeof(cnt), "%02d / %02d", idx + 1, INTRO_CARD_COUNT);
    rr.draw_text({50, 42}, cnt, 2.4f, with_alpha(c.accent, u8(220 * a)));

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 yt = VIRTUAL_H * 0.62f;
    const f32 yb = VIRTUAL_H * 0.73f;

    const f32 div_age = clamp((ct - 0.15f) / 0.35f, 0.0f, 1.0f);
    const f32 dhw = 500.0f * div_age;
    rr.draw_line({cx - dhw, yt - 30}, {cx + dhw, yt - 30}, 1.5f,
                 with_alpha(c.accent, u8(a * 140)));

    if (c.small && c.small[0]) {
        rr.draw_text_centered({cx, yt}, c.small, 2.8f,
                              with_alpha(COLOR_FACET_EDGE, u8(a * 230)));
    }
    if (c.large && c.large[0]) {
        const f32 pulse = 0.85f + 0.15f * std::sin(ct * 2.8f);
        rr.set_layer(render::Renderer::Layer::Additive);
        rr.draw_circle({cx, yb + 24.0f}, 240.0f + pulse * 30.0f,
                       with_alpha(c.accent, u8(a * 22)), 40);
        rr.set_layer(render::Renderer::Layer::Alpha);
        rr.draw_text_centered({cx, yb}, c.large, 7.2f,
                              with_alpha(c.accent, u8(a * 255 * pulse)));
    }

    const f32 sp = 0.5f + 0.5f * std::sin(intro_time_ * 3.5f);
    rr.draw_text_centered({cx, VIRTUAL_H - 36.0f}, "PRESS ANY KEY TO SKIP", 1.8f,
                          with_alpha(COLOR_FACET_EDGE, u8(80 + 90 * sp)));

    const f32 rail_y = VIRTUAL_H - 70;
    const f32 rail_w = VIRTUAL_W - 240;
    const f32 rail_x = 120;
    rr.draw_line({rail_x, rail_y}, {rail_x + rail_w, rail_y}, 2.0f,
                 with_alpha(COLOR_CORE_EDGE, 60));
    f32 total_elapsed = 0;
    for (int i = 0; i < idx; ++i) total_elapsed += INTRO_CARDS[i].duration;
    total_elapsed += ct;
    f32 total_all = 0;
    for (int i = 0; i < INTRO_CARD_COUNT; ++i) total_all += INTRO_CARDS[i].duration;
    const f32 progress = total_elapsed / total_all;
    const f32 fill_x = rail_x + rail_w * progress;
    rr.draw_line({rail_x, rail_y}, {fill_x, rail_y}, 3.0f,
                 with_alpha(c.accent, u8(200 * a)));
    f32 acc = 0;
    for (int i = 0; i < INTRO_CARD_COUNT; ++i) {
        acc += INTRO_CARDS[i].duration;
        const f32 mx = rail_x + rail_w * (acc / total_all);
        rr.draw_line({mx, rail_y - 5.0f}, {mx, rail_y + 5.0f}, 1.5f,
                     with_alpha(COLOR_FACET_EDGE, 160));
    }
}



} // namespace inc
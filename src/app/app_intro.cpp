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

void App::draw_intro_background(render::Renderer& rr, int card_index, f32 card_t) const {
    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 cy = VIRTUAL_H * 0.32f;
    const f32 t  = intro_time_;
    const f32 pulse = 0.5f + 0.5f * std::sin(t * 1.5f);

    rr.set_layer(render::Renderer::Layer::Alpha);

    switch (card_index) {
    case 0: {
        const f32 R = 150.0f;
        rr.draw_ring({ cx, cy }, R - 2.0f, R, COLOR_CORE_EDGE, 64);
        rr.draw_ring({ cx, cy }, R - 22.0f, R - 18.0f, with_alpha(COLOR_CORE_EDGE, 140), 64);
        for (int a = 0; a < 4; ++a) {
            const f32 a0 = TAU * f32(a) / 4.0f + t * 0.2f;
            rr.draw_arc_ring({ cx, cy }, R - 22.0f, R, a0, a0 + 0.6f, COLOR_CORE_INNER, 12);
        }
        const f32 marker_a = t * 0.6f;
        const Vec2 m = { cx + std::cos(marker_a) * R, cy + std::sin(marker_a) * R };
        rr.draw_circle(m, 5.0f, COLOR_CORE_INNER, 12);
        for (int i = 0; i < 6; ++i) {
            const f32 a = TAU * f32(i) / 6.0f;
            rr.draw_line({ cx + std::cos(a) * 20.0f, cy + std::sin(a) * 20.0f },
                         { cx + std::cos(a) * (R - 22.0f), cy + std::sin(a) * (R - 22.0f) },
                         1.0f, with_alpha(COLOR_CORE_EDGE, 90));
        }
        rr.draw_circle({ cx, cy }, 14.0f, with_alpha(COLOR_CORE_FILL, 220), 20);
        rr.draw_ring({ cx, cy }, 14.0f, 18.0f, COLOR_CORE_EDGE, 24);
        break;
    }
    case 1: {
        const f32 R = 120.0f;
        rr.draw_ring({ cx, cy }, R - 3.0f, R, COLOR_FACET_EDGE, 48);
        for (int i = 0; i < 16; ++i) {
            const f32 a = TAU * f32(i) / 16.0f + t * 0.15f;
            const Vec2 outer = { cx + std::cos(a) * (R * 2.4f), cy + std::sin(a) * (R * 2.4f) };
            const Vec2 inner = { cx + std::cos(a) * 40.0f, cy + std::sin(a) * 40.0f };
            rr.draw_line(outer, inner, 1.0f, with_alpha(COLOR_FACET_EDGE, 110));
        }
        rr.draw_circle({ cx, cy }, 24.0f + pulse * 6.0f, with_alpha(COLOR_CORE_INNER, 200), 24);
        rr.draw_circle({ cx, cy }, 40.0f + pulse * 10.0f, with_alpha(COLOR_CORE_EDGE, 80), 32);
        break;
    }
    case 2: {
        const f32 drift = card_t * 55.0f;
        for (int i = 0; i < 18; ++i) {
            const f32 a = TAU * f32(i) / 18.0f + f32(i) * 0.17f;
            const f32 rad = 90.0f + drift + f32(i % 4) * 12.0f;
            const Vec2 p0 = { cx + std::cos(a - 0.10f) * rad, cy + std::sin(a - 0.10f) * rad };
            const Vec2 p1 = { cx + std::cos(a + 0.10f) * rad, cy + std::sin(a + 0.10f) * rad };
            rr.draw_line(p0, p1, 4.0f, with_alpha(COLOR_CORE_EDGE, 160));
            rr.draw_circle(p0, 2.5f, with_alpha(COLOR_CORE_INNER, 200), 8);
            rr.draw_circle(p1, 2.5f, with_alpha(COLOR_CORE_INNER, 200), 8);
        }
        const f32 wave = 40.0f + card_t * 380.0f;
        const f32 wave_a = clamp(1.0f - card_t / 2.8f, 0.0f, 1.0f);
        rr.draw_ring({ cx, cy }, wave - 3.0f, wave + 3.0f,
                     with_alpha(COLOR_ENEMY_EDGE, u8(wave_a * 180.0f)), 64);
        const f32 cf = clamp(1.0f - card_t / 3.5f, 0.0f, 1.0f);
        rr.draw_circle({ cx, cy }, 30.0f * cf, with_alpha(COLOR_ENEMY_CORE, u8(cf * 200.0f)), 20);
        break;
    }
    case 3: {
        const f32 R = 160.0f;
        const f32 rot = t * 0.12f;
        {
            Vec2 prev{};
            for (int i = 0; i <= 6; ++i) {
                const f32 a = rot + TAU * f32(i) / 6.0f;
                const Vec2 p = { cx + std::cos(a) * R, cy + std::sin(a) * R };
                if (i > 0) rr.draw_line(prev, p, 2.0f, with_alpha(COLOR_ENEMY_EDGE, 160));
                prev = p;
            }
        }
        {
            Vec2 prev{};
            const f32 R2 = R * 0.62f;
            for (int i = 0; i <= 3; ++i) {
                const f32 a = -rot * 1.8f + TAU * f32(i) / 3.0f;
                const Vec2 p = { cx + std::cos(a) * R2, cy + std::sin(a) * R2 };
                if (i > 0) rr.draw_line(prev, p, 2.5f, with_alpha(COLOR_ENEMY_CORE, 200));
                prev = p;
            }
        }
        rr.draw_circle({ cx, cy }, 22.0f + pulse * 6.0f, with_alpha(COLOR_ENEMY_CORE, 220), 24);
        rr.draw_circle({ cx, cy }, 12.0f, COLOR_ENEMY_CORE, 18);
        rr.draw_circle({ cx, cy }, 4.0f, COLOR_VOID, 10);
        const f32 blink = std::fabs(std::sin(t * 0.4f));
        if (blink < 0.15f) rr.draw_line({ cx - 30.0f, cy }, { cx + 30.0f, cy }, 3.0f, COLOR_ENEMY_FILL);
        break;
    }
    case 4: {
        const Vec2 dp { cx, cy };
        const f32 R_CORE = 20.0f;
        const f32 R_IN = 78.0f;
        const f32 R_OUT = 92.0f;
        const f32 span = 2.44f;
        const f32 heading = -HALF_PI;
        for (int i = 0; i < 5; ++i) {
            const f32 step = span / 5.0f;
            const f32 a0 = heading - span * 0.5f + f32(i)     * step;
            const f32 a1 = heading - span * 0.5f + f32(i + 1) * step;
            const Vec2 pin0  = dp + Vec2::from_angle(a0) * R_IN;
            const Vec2 pin1  = dp + Vec2::from_angle(a1) * R_IN;
            const Vec2 pout0 = dp + Vec2::from_angle(a0) * R_OUT;
            const Vec2 pout1 = dp + Vec2::from_angle(a1) * R_OUT;
            rr.draw_quad(pin0, pin1, pout1, pout0, COLOR_FACET_FILL);
            rr.draw_line(pin0, pin1, 2.0f, COLOR_FACET_EDGE);
            rr.draw_line(pout0, pout1, 3.0f, COLOR_FACET_EDGE);
            rr.draw_line(pin0, pout0, 1.2f, COLOR_FACET_EDGE);
            rr.draw_line(pin1, pout1, 1.2f, COLOR_FACET_EDGE);
            const f32 mid = (a0 + a1) * 0.5f;
            const Vec2 n = Vec2::from_angle(mid);
            rr.draw_line(dp + n * (R_OUT + 4.0f), dp + n * (R_OUT + 18.0f), 2.0f, COLOR_FACET_NORMAL);
        }
        rr.draw_circle(dp, R_CORE + 8.0f, COLOR_CORE_GLOW, 24);
        rr.draw_circle(dp, R_CORE, COLOR_CORE_FILL, 24);
        rr.draw_ring(dp, R_CORE, R_CORE + 2.5f, COLOR_CORE_EDGE, 32);
        rr.draw_circle(dp, 5.0f, COLOR_CORE_INNER, 12);
        rr.draw_circle(dp, 200.0f + pulse * 20.0f, with_alpha(COLOR_CORE_EDGE, 18), 40);
        break;
    }
    case 5: {
        const Vec2 hit { cx, cy };
        const Vec2 n = Vec2::from_angle(-PI * 0.25f);
        const Vec2 tangent = n.perp();
        const Vec2 m0 = hit - tangent * 80.0f;
        const Vec2 m1 = hit + tangent * 80.0f;
        rr.draw_line(m0, m1, 6.0f, COLOR_FACET_EDGE);
        rr.draw_line(m0, m1, 2.0f, COLOR_CORE_INNER);
        for (int i = 0; i < 7; ++i) {
            const f32 d0 = 12.0f + f32(i) * 18.0f;
            rr.draw_line(hit + n * d0, hit + n * (d0 + 8.0f), 1.8f, with_alpha(COLOR_FACET_NORMAL, 200));
        }
        const Vec2 in_dir = { 1.0f, 0.35f };
        const Vec2 in_unit = in_dir.normalized();
        const Vec2 in_start = hit - in_unit * 380.0f;
        rr.draw_line(in_start, hit, 3.0f, COLOR_BULLET_CORE);
        rr.draw_circle(in_start, 6.0f, COLOR_BULLET_GLOW, 12);
        const Vec2 out_dir = in_dir - n * (2.0f * in_dir.dot(n));
        const Vec2 out_unit = out_dir.normalized();
        rr.draw_line(hit, hit + out_unit * 380.0f, 3.0f, COLOR_CORE_INNER);
        rr.draw_circle(hit, 12.0f + pulse * 4.0f, with_alpha(COLOR_CORE_INNER, 200), 20);
        rr.draw_circle(hit, 26.0f + pulse * 6.0f, with_alpha(COLOR_CORE_EDGE, 90), 24);
        const f32 ang_in  = std::atan2(-in_unit.y, -in_unit.x);
        const f32 ang_ref = std::atan2(out_unit.y, out_unit.x);
        const f32 ang_n   = std::atan2(n.y, n.x);
        rr.draw_arc_ring(hit, 50.0f, 54.0f, ang_in, ang_n, COLOR_BULLET_CORE, 12);
        rr.draw_arc_ring(hit, 50.0f, 54.0f, ang_n, ang_ref, COLOR_CORE_INNER, 12);
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
    f32 p = ct / c.duration;
    f32 a = 1.0f;
    if (p < 0.10f) a = p / 0.10f;
    else if (p > 0.88f) a = (1.0f - p) / 0.12f;
    a = clamp(a, 0.0f, 1.0f);
    f32 enter = clamp(ct / (c.duration * 0.30f), 0.0f, 1.0f);
    f32 ease = 1.0f - (1.0f - enter) * (1.0f - enter);
    f32 sl = (1.0f - ease) * -50.0f;
    f32 sr = (1.0f - ease) * 50.0f;

    rr.set_layer(render::Renderer::Layer::Alpha);
    rr.draw_quad({0,0}, {VIRTUAL_W,0}, {VIRTUAL_W,VIRTUAL_H}, {0,VIRTUAL_H}, rgba(3,8,14,255));
    draw_intro_background(rr, idx, ct);

    f32 cx = VIRTUAL_W * 0.5f;
    f32 yt = VIRTUAL_H * 0.62f;
    f32 yb = VIRTUAL_H * 0.74f;

    char cnt[16];
    std::snprintf(cnt, sizeof(cnt), "%02d / %02d", idx + 1, INTRO_CARD_COUNT);
    rr.draw_text({60, 44}, cnt, 2.2f, with_alpha(c.accent, u8(a * 180)));

    rr.draw_line({cx - 500, yt - 30}, {cx + 500, yt - 30}, 1.5f,
                 with_alpha(c.accent, u8(a * 120)));

    if (c.small && c.small[0]) {
        rr.draw_text_centered({cx + sl, yt}, c.small, 2.8f,
                              with_alpha(COLOR_FACET_EDGE, u8(a * 220)));
    }
    if (c.large && c.large[0]) {
        f32 pulse = 0.90f + 0.10f * std::sin(ct * 2.5f);
        rr.draw_text_centered({cx + sr, yb}, c.large, 7.0f,
                              with_alpha(c.accent, u8(a * 255 * pulse)));
    }

    rr.set_layer(render::Renderer::Layer::Additive);
    rr.draw_circle({cx, VIRTUAL_H * 0.32f}, 380.0f,
                   with_alpha(c.accent, u8(a * 16)), 40);
    rr.set_layer(render::Renderer::Layer::Alpha);

    f32 sp = 0.5f + 0.5f * std::sin(intro_time_ * 3.5f);
    rr.draw_text({VIRTUAL_W - 340, VIRTUAL_H - 46}, "PRESS ANY KEY TO SKIP", 1.8f,
                 with_alpha(COLOR_FACET_EDGE, u8(80 + 90 * sp)));

    f32 dy = VIRTUAL_H - 60;
    f32 ds = 26;
    f32 dx0 = cx - (f32(INTRO_CARD_COUNT - 1) * ds) * 0.5f;
    for (int i = 0; i < INTRO_CARD_COUNT; ++i) {
        Vec2 pc { dx0 + f32(i) * ds, dy };
        if (i < idx) rr.draw_circle(pc, 4.0f, with_alpha(COLOR_CORE_EDGE, 140), 12);
        else if (i == idx) {
            rr.draw_circle(pc, 5.5f, COLOR_CORE_INNER, 12);
            rr.draw_ring(pc, 8.0f, 10.0f, with_alpha(c.accent, 180), 16);
        } else rr.draw_ring(pc, 3.0f, 4.0f, with_alpha(COLOR_CORE_EDGE, 90), 12);
    }
}


} // namespace inc
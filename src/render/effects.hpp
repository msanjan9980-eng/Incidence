#pragma once

#include <cmath>
#include <cstdio>

#include "core/color.hpp"
#include "core/math.hpp"
#include "core/vec2.hpp"
#include "render/renderer.hpp"

namespace inc::render {

// ============================================================================
// Effects - particle pool, screen shake, hit-stop, floating labels, flash.
// World-space. Renderer view offset handles shake; effects draw in world coords.
// ============================================================================

struct Particle {
    Vec2  pos;
    Vec2  vel;
    f32   life     = 0.0f;
    f32   life_max = 1.0f;
    f32   size0    = 4.0f;
    f32   size1    = 0.0f;
    Color color    = 0;
    u8    kind     = 0; // 0=dot, 1=spark line, 2=ring
    bool  alive    = false;
};

struct FloatLabel {
    Vec2  pos;
    Vec2  vel;
    f32   life;
    char  text[24];
    Color color;
    bool  alive;
};

class Effects {
public:
    static constexpr int MAX_PARTICLES = 640;
    static constexpr int MAX_LABELS    = 48;

    Particle    particles[MAX_PARTICLES] = {};
    FloatLabel  labels[MAX_LABELS]       = {};

    f32  shake_t       = 0.0f;
    f32  shake_t_max   = 0.0f;
    f32  shake_mag_max = 0.0f;
    Vec2 shake_offset  { 0.0f, 0.0f };

    f32  hitstop      = 0.0f;
    f32  flash_t      = 0.0f;
    f32  flash_t_max  = 0.0f;
    Color flash_color = 0;

    void reset() {
        for (auto& p : particles) p.alive = false;
        for (auto& l : labels)    l.alive = false;
        shake_t = shake_t_max = 0.0f;
        shake_mag_max = 0.0f;
        shake_offset = {0,0};
        hitstop = 0.0f;
        flash_t = flash_t_max = 0.0f;
    }

    void update(f32 dt) {
        for (auto& p : particles) {
            if (!p.alive) continue;
            p.life += dt;
            if (p.life >= p.life_max) { p.alive = false; continue; }
            p.pos += p.vel * dt;
            const f32 k = std::pow(0.18f, dt);
            p.vel.x *= k; p.vel.y *= k;
        }
        for (auto& l : labels) {
            if (!l.alive) continue;
            l.life += dt;
            if (l.life >= 1.0f) { l.alive = false; continue; }
            l.pos += l.vel * dt;
            l.vel.y += 90.0f * dt;
            const f32 k = std::pow(0.90f, dt);
            l.vel.x *= k; l.vel.y *= k;
        }
        if (shake_t > 0.0f) {
            shake_t -= dt;
            if (shake_t <= 0.0f) {
                shake_t = 0.0f;
                shake_offset = {0,0};
            } else {
                const f32 k = shake_t / shake_t_max;
                const f32 m = shake_mag_max * k * k;
                const f32 tt = (shake_t_max - shake_t) * 42.0f;
                shake_offset.x = std::sin(tt * 1.7f) * m;
                shake_offset.y = std::cos(tt * 2.3f) * m;
            }
        }
        if (hitstop > 0.0f) {
            hitstop -= dt;
            if (hitstop < 0.0f) hitstop = 0.0f;
        }
        if (flash_t > 0.0f) {
            flash_t -= dt;
            if (flash_t < 0.0f) flash_t = 0.0f;
        }
    }

    void add_shake(f32 mag, f32 dur) {
        if (mag >= shake_mag_max || shake_t <= 0.0f) {
            shake_mag_max = mag;
            shake_t_max   = dur;
            shake_t       = dur;
        }
    }

    void add_hitstop(f32 s) { if (s > hitstop) hitstop = s; }
    void add_flash(Color c, f32 dur) {
        flash_color = c; flash_t = dur; flash_t_max = dur;
    }

    void spawn(Vec2 pos, Vec2 vel, f32 life, f32 s0, f32 s1,
               Color color, u8 kind) {
        for (auto& p : particles) {
            if (p.alive) continue;
            p.pos = pos; p.vel = vel;
            p.life = 0.0f; p.life_max = life;
            p.size0 = s0; p.size1 = s1;
            p.color = color; p.kind = kind;
            p.alive = true;
            return;
        }
    }

    void spawn_burst(Vec2 pos, int n, f32 speed, Color color) {
        for (int i = 0; i < n; ++i) {
            const f32 a = TAU * f32(i) / f32(n) + f32(i & 7) * 0.31f;
            const f32 s = speed * (0.55f + 0.45f * f32(i & 3) / 3.0f);
            spawn(pos, Vec2::from_angle(a) * s, 0.60f, 3.4f, 0.4f, color, 0);
        }
    }

    void spawn_reflect(Vec2 pos, Vec2 normal, Color color) {
        for (int i = 0; i < 7; ++i) {
            const f32 base = std::atan2(normal.y, normal.x);
            const f32 a = base + (f32(i) / 6.0f - 0.5f) * 1.1f;
            const f32 s = 240.0f + f32(i) * 34.0f;
            spawn(pos, Vec2::from_angle(a) * s, 0.34f, 4.4f, 0.6f, color, 1);
        }
    }

    void spawn_ring(Vec2 pos, f32 radius, Color color, f32 life = 0.55f) {
        spawn(pos, {0,0}, life, radius * 0.25f, radius, color, 2);
    }

    void add_label(Vec2 pos, const char* txt, Color color) {
        for (auto& l : labels) {
            if (l.alive) continue;
            std::snprintf(l.text, sizeof(l.text), "%s", txt);
            l.pos = pos;
            l.vel = { 0.0f, -95.0f };
            l.life = 0.0f;
            l.color = color;
            l.alive = true;
            return;
        }
    }

    void add_score_label(Vec2 pos, int amount, Color color) {
        char buf[24];
        std::snprintf(buf, sizeof(buf), "+%d", amount);
        add_label(pos, buf, color);
    }

    void draw(Renderer& r) const {
        r.set_layer(Renderer::Layer::Additive);
        for (const auto& p : particles) {
            if (!p.alive) continue;
            const f32 t = p.life / p.life_max;
            const f32 size = p.size0 * (1.0f - t) + p.size1 * t;
            const u8 a = u8((1.0f - t) * 255.0f);
            switch (p.kind) {
                case 0:
                    r.draw_circle(p.pos, size, with_alpha(p.color, a), 10);
                    break;
                case 1: {
                    Vec2 dir = p.vel.length_sq() > 1.0f
                             ? p.vel.normalized() : Vec2{1.0f, 0.0f};
                    r.draw_line(p.pos - dir * size * 2.6f,
                                p.pos + dir * size * 2.6f,
                                size * 0.9f, with_alpha(p.color, a));
                    break;
                }
                case 2: {
                    const f32 rad = p.size0 * (1.0f - t) + p.size1 * t;
                    r.draw_ring(p.pos, rad - 2.5f, rad + 2.5f,
                                with_alpha(p.color, a), 32);
                    break;
                }
            }
        }
        r.set_layer(Renderer::Layer::Alpha);
        for (const auto& l : labels) {
            if (!l.alive) continue;
            const f32 t = l.life;
            const u8 a = u8((1.0f - t * t) * 255.0f);
            r.draw_text_centered(l.pos, l.text, 2.6f,
                                 with_alpha(l.color, a));
        }
    }

    void draw_flash(Renderer& r) const {
        if (flash_t <= 0.0f) return;
        const f32 t = flash_t / flash_t_max;
        r.set_layer(Renderer::Layer::Additive);
        r.draw_quad({0,0}, {VIRTUAL_W,0}, {VIRTUAL_W,VIRTUAL_H}, {0,VIRTUAL_H},
                    with_alpha(flash_color, u8(t * 110.0f)));
        r.set_layer(Renderer::Layer::Alpha);
    }
};

} // namespace inc::render
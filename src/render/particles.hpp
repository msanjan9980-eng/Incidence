#pragma once

// =============================================================================
// particles.hpp - tiny CPU particle system.
//
// Fixed-size pool, no allocation. Two emitters: burst and stream. Particles
// are points + a short trail; the renderer batches them without issue.
// Call update(dt) in App::step, draw(r) in App::render.
// =============================================================================

#include <cmath>
#include <cstring>

#include "core/color.hpp"
#include "core/math.hpp"
#include "core/vec2.hpp"
#include "render/renderer.hpp"
#include "render/fx.hpp"

namespace inc::fx {

constexpr int MAX_PARTICLES = 2048;

enum class ParticleKind : u8 {
    Dot,      // small square, fades
    Spark,    // short line streak along velocity
    Ember,    // circle with additive glow
    Hex,      // tiny rotating quad
    Ring,     // expanding ring
};

struct Particle {
    Vec2  pos {};
    Vec2  vel {};
    f32   life      = 0.0f;
    f32   max_life  = 1.0f;
    f32   size0     = 2.0f;
    f32   size1     = 0.0f;
    f32   drag      = 1.0f;
    f32   gravity   = 0.0f;
    f32   spin      = 0.0f;
    f32   angle     = 0.0f;
    Color col0      = {};
    Color col1      = {};
    ParticleKind kind = ParticleKind::Dot;
    bool  alive     = false;
};

class ParticleSystem {
public:
    void clear() {
        for (int i = 0; i < MAX_PARTICLES; ++i) parts_[i].alive = false;
        cursor_ = 0;
    }

    void update(f32 dt) {
        for (int i = 0; i < MAX_PARTICLES; ++i) {
            Particle& p = parts_[i];
            if (!p.alive) continue;
            p.life -= dt;
            if (p.life <= 0.0f) { p.alive = false; continue; }
            p.vel.y += p.gravity * dt;
            p.vel   = p.vel * std::pow(p.drag, dt * 60.0f);
            p.pos   = p.pos + p.vel * dt;
            p.angle += p.spin * dt;
        }
    }

    void draw(render::Renderer& r) const {
        for (int i = 0; i < MAX_PARTICLES; ++i) {
            const Particle& p = parts_[i];
            if (!p.alive) continue;
            const f32 k = 1.0f - (p.life / p.max_life);
            const f32 kk = 1.0f - k;
            const f32 size = p.size0 + (p.size1 - p.size0) * k;
            // Color is a packed u32; alpha is the low byte.
            const Color c = (k < 0.5f) ? p.col0 : p.col1;
            const u8 a = u8(f32(c & 0xFFu) * kk);
            switch (p.kind) {
                case ParticleKind::Dot:
                    r.draw_quad({p.pos.x - size, p.pos.y - size},
                                {p.pos.x + size, p.pos.y - size},
                                {p.pos.x + size, p.pos.y + size},
                                {p.pos.x - size, p.pos.y + size},
                                with_alpha(c, a));
                    break;
                case ParticleKind::Spark: {
                    const Vec2 tail = p.pos - p.vel * 0.045f;
                    r.draw_line(tail, p.pos, size * 0.6f, with_alpha(c, a));
                    break;
                }
                case ParticleKind::Ember:
                    r.set_layer(render::Renderer::Layer::Additive);
                    r.draw_circle(p.pos, size * 3.5f, with_alpha(c, u8(a / 3u)), 12);
                    r.set_layer(render::Renderer::Layer::Alpha);
                    r.draw_circle(p.pos, size, with_alpha(c, a), 14);
                    break;
                case ParticleKind::Hex: {
                    const f32 s = size * 0.8f;
                    const f32 c0 = std::cos(p.angle) * s;
                    const f32 s0 = std::sin(p.angle) * s;
                    const Vec2 v0 {p.pos.x + c0, p.pos.y + s0};
                    const Vec2 v1 {p.pos.x - s0, p.pos.y + c0};
                    const Vec2 v2 {p.pos.x - c0, p.pos.y - s0};
                    const Vec2 v3 {p.pos.x + s0, p.pos.y - c0};
                    r.draw_quad(v0, v1, v2, v3, with_alpha(c, a));
                    break;
                }
                case ParticleKind::Ring: {
                    const f32 rr = size * (0.5f + 2.0f * k);
                    r.set_layer(render::Renderer::Layer::Additive);
                    r.draw_ring(p.pos, rr - 1.2f, rr + 1.2f, with_alpha(c, a), 32);
                    r.set_layer(render::Renderer::Layer::Alpha);
                    break;
                }
            }
        }
    }

    // Burst: emit n particles from a point in a cone or sphere.
    void burst(Vec2 origin, int n,
               f32 speed_min, f32 speed_max,
               f32 life_min, f32 life_max,
               f32 angle_center, f32 angle_spread,
               f32 size0, f32 size1,
               Color c0, Color c1,
               ParticleKind kind = ParticleKind::Spark,
               f32 gravity = 0.0f, f32 drag = 0.98f, f32 spin = 0.0f) {
        for (int i = 0; i < n; ++i) {
            Particle& p = alloc();
            const f32 u = f32(i) / f32(n <= 1 ? 1 : n - 1);
            const f32 ang = angle_center + (u - 0.5f) * angle_spread
                          + 0.06f * std::sin(u * 43.0f);
            const f32 spd = speed_min + (speed_max - speed_min) * hash01(u32(i) * 2654435761u);
            p.pos      = origin;
            p.vel      = {std::cos(ang) * spd, std::sin(ang) * spd};
            p.life     = life_min + (life_max - life_min) * hash01(u32(i) * 40503u + 7u);
            p.max_life = p.life;
            p.size0    = size0;
            p.size1    = size1;
            p.drag     = drag;
            p.gravity  = gravity;
            p.spin     = spin * (hash01(u32(i) * 3266489917u) * 2.0f - 1.0f);
            p.angle    = hash01(u32(i) * 9973u) * TAU;
            p.col0     = c0;
            p.col1     = c1;
            p.kind     = kind;
        }
    }

    // Stream: emit n particles per second continuously.
    void stream(Vec2 origin, f32 dt, f32 rate_per_sec,
                f32 speed_min, f32 speed_max,
                f32 life_min, f32 life_max,
                f32 angle_center, f32 angle_spread,
                f32 size0, f32 size1,
                Color c0, Color c1,
                ParticleKind kind = ParticleKind::Ember,
                f32 gravity = 0.0f, f32 drag = 0.97f, f32 spin = 0.0f) {
        stream_accum_ += rate_per_sec * dt;
        int to_emit = int(stream_accum_);
        stream_accum_ -= f32(to_emit);
        if (to_emit > 64) to_emit = 64;
        burst(origin, to_emit, speed_min, speed_max, life_min, life_max,
              angle_center, angle_spread, size0, size1, c0, c1,
              kind, gravity, drag, spin);
    }

    [[nodiscard]] int alive_count() const {
        int n = 0;
        for (int i = 0; i < MAX_PARTICLES; ++i) if (parts_[i].alive) ++n;
        return n;
    }

private:
    Particle& alloc() {
        // Linear scan for a free slot; wraps if full (recycles oldest).
        for (int i = 0; i < MAX_PARTICLES; ++i) {
            const int idx = (cursor_ + i) % MAX_PARTICLES;
            if (!parts_[idx].alive) { cursor_ = (idx + 1) % MAX_PARTICLES; return parts_[idx]; }
        }
        Particle& p = parts_[cursor_];
        cursor_ = (cursor_ + 1) % MAX_PARTICLES;
        return p;
    }

    Particle parts_[MAX_PARTICLES] {};
    int      cursor_        = 0;
    f32      stream_accum_  = 0.0f;
};

} // namespace inc::fx

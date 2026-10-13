#include "app/app.hpp"
#include "audio/audio.hpp"

#include "sokol_app.h"

#include <cstdio>

namespace inc {

static constexpr int MAX_SUBSTEPS_PER_FRAME = 8;

App::App()  = default;
App::~App() = default;

void App::init() {
    running_       = true;
    accumulator_   = 0.0;
    last_frame_ts_ = platform::time_seconds();
    frame_count_   = 0;
    screen_        = game::Screen::Title;
    title_time_    = 0.0f;

    std::fprintf(stderr, "[incidence] init - virtual %gx%g, fixed dt %g Hz\n",
                 static_cast<double>(VIRTUAL_W),
                 static_cast<double>(VIRTUAL_H),
                 static_cast<double>(FIXED_DT_INV));
    std::fflush(stderr);

    render::InitDesc rd{};
    rd.window_width  = platform::window_width();
    rd.window_height = platform::window_height();
    renderer_.init(rd);

    audio::init();
}

void App::frame() {
    const double now = platform::time_seconds();
    const double frame_dt = now - last_frame_ts_;
    last_frame_ts_ = now;

    advance_simulation(frame_dt);
    render();

    if (platform::quit_requested()) {
        running_ = false;
    }

    ++frame_count_;
}

void App::advance_simulation(double frame_dt) {
    if (frame_dt > 0.250) frame_dt = 0.250;
    accumulator_ += frame_dt;
    int substeps = 0;
    while (accumulator_ >= static_cast<double>(FIXED_DT)
           && substeps < MAX_SUBSTEPS_PER_FRAME) {
        step(FIXED_DT);
        accumulator_ -= static_cast<double>(FIXED_DT);
        ++substeps;
    }
    if (substeps == MAX_SUBSTEPS_PER_FRAME) accumulator_ = 0.0;
}

void App::step(f32 dt) {
    const auto& in = platform::platform_input();

    // Detect "any key" for the title screen.
    bool any_pressed = false;
    for (int i = 0; i < static_cast<int>(platform::Key::Count); ++i) {
        if (in.key_pressed[i]) { any_pressed = true; break; }
    }

    switch (screen_) {
        case game::Screen::Title: {
            title_time_ += dt;
            if (any_pressed && title_time_ > 0.4f) {
                world_.reset();
                screen_ = game::Screen::Playing;
            }
            break;
        }
        case game::Screen::Playing: {
            world_.step(dt, in);
            if (world_.player_dead()) {
                screen_ = game::Screen::GameOver;
            } else if (world_.warden_defeated()) {
                // Give the player a moment to see the explosion, then
                // transition to victory.
                if (world_.warden_active() == false) {
                    // The Warden was defeated via the boss path. Let the
                    // player watch for a bit before we show the screen.
                }
            }
            break;
        }
        case game::Screen::GameOver: {
            if (any_pressed) {
                world_.reset();
                screen_ = game::Screen::Playing;
            }
            break;
        }
        case game::Screen::Victory: {
            if (any_pressed) {
                world_.reset();
                screen_ = game::Screen::Title;
            }
            break;
        }
    }
}

void App::render() {
    renderer_.begin_frame(platform::window_width(),
                          platform::window_height());

    switch (screen_) {
        case game::Screen::Title:     draw_title(renderer_);     break;
        case game::Screen::Playing:   world_.draw(renderer_);      break;
        case game::Screen::GameOver:  draw_game_over(renderer_);   break;
        case game::Screen::Victory:   draw_victory(renderer_);     break;
    }

    renderer_.end_frame();
}

// ---------------------------------------------------------------------------
// Screens
// ---------------------------------------------------------------------------

void App::draw_title(render::Renderer& r) const {
    r.set_layer(render::Renderer::Layer::Alpha);

    // Dark backdrop with a slow pulse.
    const f32 pulse = 0.5f + 0.5f * std::sin(title_time_ * 1.6f);

    // Title text — rendered as rectangles for now. Real font rendering
    // comes after M10; this proves the screen flow.
    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 cy = VIRTUAL_H * 0.42f;

    // "INCIDENCE" as a stylized bar with a highlight sweep.
    const f32 bar_w = 620.0f;
    const f32 bar_h = 46.0f;
    r.draw_quad({ cx - bar_w * 0.5f, cy - bar_h * 0.5f },
                { cx + bar_w * 0.5f, cy - bar_h * 0.5f },
                { cx + bar_w * 0.5f, cy + bar_h * 0.5f },
                { cx - bar_w * 0.5f, cy + bar_h * 0.5f },
                with_alpha(COLOR_CORE_EDGE, u8(80 + 60 * pulse)));

    // Subtitle bar.
    const f32 sub_w = 380.0f;
    const f32 sub_h = 14.0f;
    const f32 sub_y = cy + 60.0f;
    r.draw_quad({ cx - sub_w * 0.5f, sub_y - sub_h * 0.5f },
                { cx + sub_w * 0.5f, sub_y - sub_h * 0.5f },
                { cx + sub_w * 0.5f, sub_y + sub_h * 0.5f },
                { cx - sub_w * 0.5f, sub_y + sub_h * 0.5f },
                with_alpha(COLOR_FACET_EDGE, 140));

    // "Press any key" indicator — pulsing ring.
    const f32 ring_y = VIRTUAL_H * 0.72f;
    const f32 ring_r = 30.0f + 6.0f * pulse;
    r.draw_ring({ cx, ring_y }, ring_r - 3.0f, ring_r + 3.0f,
                with_alpha(COLOR_CORE_INNER, 200), 32);

    // A small drone silhouette above the prompt, drawn with our own shapes.
    const Vec2 drone_pos { cx, VIRTUAL_H * 0.60f };
    r.draw_circle(drone_pos, 14.0f, COLOR_CORE_FILL, 20);
    r.draw_ring(drone_pos, 14.0f, 17.0f, COLOR_CORE_EDGE, 24);
    // Tiny shield arc
    const int facets = 5;
    const f32 span = 2.44f;
    const f32 heading = -HALF_PI;
    for (int i = 0; i < facets; ++i) {
        const f32 step = span / f32(facets);
        const f32 a0 = heading - span * 0.5f + f32(i)     * step;
        const f32 a1 = heading - span * 0.5f + f32(i + 1) * step;
        const Vec2 pin0 = drone_pos + Vec2::from_angle(a0) * 50.0f;
        const Vec2 pin1 = drone_pos + Vec2::from_angle(a1) * 50.0f;
        const Vec2 pout0= drone_pos + Vec2::from_angle(a0) * 60.0f;
        const Vec2 pout1= drone_pos + Vec2::from_angle(a1) * 60.0f;
        r.draw_quad(pin0, pin1, pout1, pout0, COLOR_FACET_FILL);
        r.draw_line(pout0, pout1, 2.0f, COLOR_FACET_EDGE);
    }

    // Additive glow on the ring and drone.
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_circle({ cx, ring_y }, 60.0f, with_alpha(COLOR_CORE_EDGE, u8(30 * pulse)), 24);
    r.draw_circle(drone_pos,      80.0f, with_alpha(COLOR_CORE_EDGE, 26), 24);
}

void App::draw_game_over(render::Renderer& r) const {
    r.set_layer(render::Renderer::Layer::Alpha);

    // Dark red overlay.
    r.draw_quad({ 0, 0 }, { VIRTUAL_W, 0 },
                { VIRTUAL_W, VIRTUAL_H }, { 0, VIRTUAL_H },
                with_alpha(COLOR_ENEMY_FILL, 40));

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 cy = VIRTUAL_H * 0.42f;

    // "GAME OVER" bar.
    const f32 bar_w = 520.0f;
    const f32 bar_h = 42.0f;
    r.draw_quad({ cx - bar_w * 0.5f, cy - bar_h * 0.5f },
                { cx + bar_w * 0.5f, cy - bar_h * 0.5f },
                { cx + bar_w * 0.5f, cy + bar_h * 0.5f },
                { cx - bar_w * 0.5f, cy + bar_h * 0.5f },
                with_alpha(COLOR_ENEMY_EDGE, 220));

    // "Press any key to retry" prompt.
    const f32 pulse = 0.5f + 0.5f * std::sin(platform::time_seconds() * 4.0);
    const f32 ring_y = VIRTUAL_H * 0.62f;
    const f32 ring_r = 24.0f + 5.0f * pulse;
    r.draw_ring({ cx, ring_y }, ring_r - 3.0f, ring_r + 3.0f,
                with_alpha(COLOR_ENEMY_CORE, u8(140 + 100 * pulse)), 32);

    // Additive glow.
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_circle({ cx, ring_y }, 50.0f,
                  with_alpha(COLOR_ENEMY_CORE, u8(30 * pulse)), 24);
}

void App::draw_victory(render::Renderer& r) const {
    r.set_layer(render::Renderer::Layer::Alpha);

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 cy = VIRTUAL_H * 0.42f;
    const f32 t  = platform::time_seconds();

    // Big cyan victory bar.
    const f32 bar_w = 620.0f;
    const f32 bar_h = 42.0f;
    r.draw_quad({ cx - bar_w * 0.5f, cy - bar_h * 0.5f },
                { cx + bar_w * 0.5f, cy - bar_h * 0.5f },
                { cx + bar_w * 0.5f, cy + bar_h * 0.5f },
                { cx - bar_w * 0.5f, cy + bar_h * 0.5f },
                with_alpha(COLOR_CORE_EDGE, 240));

    // Cascading room dots celebrating completion.
    const int dots = game::ROOM_COUNT + 1;
    const f32 dot_y = VIRTUAL_H * 0.58f;
    for (int i = 0; i < dots; ++i) {
        const f32 x = cx + (f32(i) - f32(dots - 1) * 0.5f) * 28.0f;
        const f32 offset = std::sin(t * 2.0f + f32(i) * 0.4f) * 6.0f;
        r.draw_circle({ x, dot_y + offset }, 8.0f, COLOR_CORE_INNER, 14);
    }

    // Restart prompt ring.
    const f32 pulse = 0.5f + 0.5f * std::sin(t * 4.0f);
    const f32 ring_y = VIRTUAL_H * 0.74f;
    const f32 ring_r = 24.0f + 5.0f * pulse;
    r.draw_ring({ cx, ring_y }, ring_r - 3.0f, ring_r + 3.0f,
                with_alpha(COLOR_CORE_EDGE, u8(140 + 100 * pulse)), 32);

    // Additive bloom across the victory bar.
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_circle({ cx, cy }, 400.0f, with_alpha(COLOR_CORE_EDGE, 20), 40);
    r.draw_circle({ cx, ring_y }, 60.0f, with_alpha(COLOR_CORE_INNER, u8(40 * pulse)), 24);
}

void App::shutdown() {
    audio::shutdown();
    renderer_.shutdown();
    std::fprintf(stderr, "[incidence] shutdown after %llu frames\n",
                 static_cast<unsigned long long>(frame_count_));
    std::fflush(stderr);
}

} // namespace inc
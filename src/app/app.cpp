#include "app/app.hpp"

#include <cstdio>

namespace inc {

// Design note: the background is not pure black. It is the deep-teal void
// from the art direction — the same color the shield's bloom will bleed into.
// Getting this right on day one makes every later screenshot feel on-brand.
static constexpr f32 BG_R = 0.020f;
static constexpr f32 BG_G = 0.055f;
static constexpr f32 BG_B = 0.070f;

// Guard against the accumulator spiral-of-death after a stall.
static constexpr int MAX_SUBSTEPS_PER_FRAME = 8;

App::App()  = default;
App::~App() = default;

void App::init() {
    running_       = true;
    accumulator_   = 0.0;
    last_frame_ts_ = platform::time_seconds();
    frame_count_   = 0;

    std::printf("[incidence] init - virtual %gx%g, fixed dt %g Hz\n",
                static_cast<double>(VIRTUAL_W),
                static_cast<double>(VIRTUAL_H),
                static_cast<double>(FIXED_DT_INV));
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
    // Clamp pathological frame deltas (debugger pause, window drag on Windows).
    // 250 ms is enough headroom for a single hitch without dropping sim ticks.
    if (frame_dt > 0.250) frame_dt = 0.250;

    accumulator_ += frame_dt;

    int substeps = 0;
    while (accumulator_ >= static_cast<double>(FIXED_DT)
           && substeps < MAX_SUBSTEPS_PER_FRAME) {
        step(FIXED_DT);
        accumulator_ -= static_cast<double>(FIXED_DT);
        ++substeps;
    }

    // If we hit the substep cap, we are behind. Drop the backlog rather than
    // accumulating it forever — a slightly-slow frame is better than a
    // permanently-lagging sim.
    if (substeps == MAX_SUBSTEPS_PER_FRAME) {
        accumulator_ = 0.0;
    }
}

void App::step(f32 /*dt*/) {
    // Milestone 1: no entities yet.
    // Later milestones fill this with:
    //   - shield rotation from input
    //   - projectile integration + collision (reflect)
    //   - enemy AI ticks
    //   - boss state machines
}

void App::render() {
    // Milestone 1: nothing to draw yet. Real rendering arrives in M3 with
    // the sokol_gfx pipeline, the virtual->screen letterbox, and the post pass.
    // The window will show the OS default clear color for now.
}

void App::shutdown() {
    std::printf("[incidence] shutdown after %llu frames\n",
                static_cast<unsigned long long>(frame_count_));
}

} // namespace inc
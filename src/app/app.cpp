#include "app/app.hpp"

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

    std::fprintf(stderr, "[incidence] init - virtual %gx%g, fixed dt %g Hz\n",
                 static_cast<double>(VIRTUAL_W),
                 static_cast<double>(VIRTUAL_H),
                 static_cast<double>(FIXED_DT_INV));
    std::fflush(stderr);

    render::InitDesc rd{};
    rd.window_width  = platform::window_width();
    rd.window_height = platform::window_height();
    renderer_.init(rd);
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

    if (substeps == MAX_SUBSTEPS_PER_FRAME) {
        accumulator_ = 0.0;
    }
}

void App::step(f32 dt) {
    world_.step(dt, platform::platform_input());
}

void App::render() {
    renderer_.begin_frame(platform::window_width(),
                          platform::window_height());
    world_.draw(renderer_);
    renderer_.end_frame();
}

void App::shutdown() {
    renderer_.shutdown();
    std::fprintf(stderr, "[incidence] shutdown after %llu frames\n",
                 static_cast<unsigned long long>(frame_count_));
    std::fflush(stderr);
}

} // namespace inc
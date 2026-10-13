#pragma once

#include "core/math.hpp"
#include "platform/platform.hpp"
#include "render/renderer.hpp"
#include "game/world.hpp"
#include "game/screen.hpp"

namespace inc {

class App {
public:
    App();
    ~App();

    App(const App&)            = delete;
    App& operator=(const App&) = delete;

    void init();
    void frame();
    void shutdown();

    [[nodiscard]] bool running() const noexcept { return running_; }

private:
    void advance_simulation(double frame_dt);
    void step(f32 dt);
    void render();

    void draw_title(render::Renderer& r) const;
    void draw_game_over(render::Renderer& r) const;
    void draw_victory(render::Renderer& r) const;

    bool   running_       = true;
    double accumulator_   = 0.0;
    double last_frame_ts_ = 0.0;
    u64    frame_count_   = 0;

    game::Screen  screen_ = game::Screen::Title;
    f32           title_time_ = 0.0f;

    render::Renderer  renderer_;
    game::World       world_;
};

} // namespace inc
#pragma once

#include "core/math.hpp"
#include "platform/platform.hpp"

namespace inc {

// ---------------------------------------------------------------------------
// App — the top-level game object.
//
// Owns the fixed-timestep loop and delegates simulation and rendering to
// subsystems added in later milestones. In Milestone 1 it does nothing but
// clear the screen; that is intentional. Getting the timing skeleton correct
// before any gameplay exists is what makes determinism possible later.
// ---------------------------------------------------------------------------
class App {
public:
    App();
    ~App();

    App(const App&)            = delete;
    App& operator=(const App&) = delete;

    // Called once from main() after the window is created.
    void init();

    // Called every frame by sokol_app.
    void frame();

    // Called once before shutdown.
    void shutdown();

    [[nodiscard]] bool running() const noexcept { return running_; }

private:
    // Fixed-timestep accumulator. Advance the simulation in FIXED_DT chunks
    // until the accumulator is drained, then render once. The maximum number
    // of substeps per frame is bounded to prevent a spiral-of-death after a
    // debugger pause or a long OS stall.
    void advance_simulation(double frame_dt);

    // One simulation tick. In M1 this is empty; later it steps projectiles,
    // the shield, enemies, and boss state machines.
    void step(f32 dt);

    // Draw the current world state. In M1 this clears to the background color.
    void render();

    bool   running_       = true;
    double accumulator_   = 0.0;
    double last_frame_ts_ = 0.0;
    u64    frame_count_   = 0;
};

} // namespace inc
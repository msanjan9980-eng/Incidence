#include "app/app.hpp"
#include "render/fx.hpp"
#include "render/hud_fx.hpp"

#include <cmath>
#include <cstdio>

namespace inc {

// =============================================================================
// Gameplay HUD overlay. World::draw owns LV / XP / hearts / kills; this file
// owns room/status, the sector scan frame, warden warning, and CRT post-FX.
// =============================================================================
void App::draw_gameplay_hud(render::Renderer& r) const {
    const f32 t = f32(platform::time_seconds());

    // -----------------------------------------------------------------
    // Top-right: room + status
    // -----------------------------------------------------------------
    {
        const int idx = world_.room_index();
        const bool boss = world_.warden_active();
        const bool cleared = world_.is_room_cleared(idx) && !boss;

        char rbuf[32];
        std::snprintf(rbuf, sizeof(rbuf), "ROOM %02d", idx);
        const f32 rw = r.text_width(rbuf, 2.0f);
        r.draw_text({f32(VIRTUAL_W) - 40.0f - rw, 40.0f}, rbuf, 2.0f,
                    with_alpha(COLOR_CORE_EDGE, 220));

        const char* status = boss ? "WARDEN"
                            : cleared ? "SECTOR CLEAR"
                                      : "HOSTILES ACTIVE";
        const Color sc = boss ? COLOR_ENEMY_CORE
                        : cleared ? COLOR_CORE_INNER
                                  : COLOR_FACET_EDGE;
        const f32 sw = r.text_width(status, 1.8f);
        r.draw_text({f32(VIRTUAL_W) - 40.0f - sw, 68.0f}, status, 1.8f,
                    with_alpha(sc, 220));
    }

    // -----------------------------------------------------------------
    // Bottom-right: sector scan frame (placeholder)
    // -----------------------------------------------------------------
    {
        const f32 size = 180.0f;
        const f32 x1 = f32(VIRTUAL_W) - 40.0f;
        const f32 x0 = x1 - size;
        const f32 y1 = f32(VIRTUAL_H) - 60.0f;
        const f32 y0 = y1 - size;
        hud::hud_frame(r, x0, y0, x1, y1, "SECTOR SCAN",
                       COLOR_FACET_EDGE, 0.55f);
    }

    // -----------------------------------------------------------------
    // Warden warning banner
    // -----------------------------------------------------------------
    if (world_.warden_active()) {
        const f32 pulse = 0.5f + 0.5f * std::sin(t * 3.5f);
        hud::hud_warning_banner(r, f32(VIRTUAL_W) * 0.5f, 130.0f,
                                "WARDEN ENGAGED",
                                0.75f + 0.25f * pulse, t,
                                COLOR_ENEMY_CORE);
    }

    // -----------------------------------------------------------------
    // Cohesive CRT overlay
    // -----------------------------------------------------------------
    fx::scanlines(r, 0.55f, 4.0f, 24);
    fx::film_grain(r, t, 0.4f, 160, 1.0f);
}

} // namespace inc

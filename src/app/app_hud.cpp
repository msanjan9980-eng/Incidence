#include "app/app.hpp"
#include "render/fx.hpp"
#include "render/hud_fx.hpp"

#include <cmath>
#include <cstdio>

namespace inc {

// =============================================================================
// Gameplay HUD. Called each frame from the Playing branch of App::render.
// Uses only state that already exists on World / Character.
// =============================================================================
void App::draw_gameplay_hud(render::Renderer& r) const {
    const f32 t = f32(platform::time_seconds());

    // -----------------------------------------------------------------
    // Top-left: level + XP bar + unspent point badges
    // -----------------------------------------------------------------
    {
        const game::Character& ch = world_.character();
        char buf[32];
        std::snprintf(buf, sizeof(buf), "LV %02d", ch.level);
        r.draw_text({40.0f, 40.0f}, buf, 2.4f, COLOR_CORE_INNER);

        hud::hud_bar(r, {360.0f, 50.0f}, 320.0f, 14.0f,
                     "XP", ch.xp_progress(), t,
                     COLOR_CORE_EDGE, COLOR_CORE_EDGE, false);

        f32 bx = 40.0f;
        if (ch.stat_points > 0) {
            std::snprintf(buf, sizeof(buf), "+%d STAT", ch.stat_points);
            r.draw_text({bx, 72.0f}, buf, 1.8f, COLOR_CORE_INNER);
            bx += 120.0f;
        }
        if (ch.skill_points > 0) {
            std::snprintf(buf, sizeof(buf), "+%d SKILL", ch.skill_points);
            r.draw_text({bx, 72.0f}, buf, 1.8f, COLOR_CORE_INNER);
        }
    }

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
    // Bottom-right: sector scan frame (placeholder for minimap)
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
    // Cohesive CRT overlay - subtler than the menu version
    // -----------------------------------------------------------------
    fx::scanlines(r, 0.55f, 4.0f, 24);
    fx::film_grain(r, t, 0.4f, 160, 1.0f);
}

} // namespace inc

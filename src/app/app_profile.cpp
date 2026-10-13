#include "app/app.hpp"
#include "render/fx.hpp"
#include "render/field.hpp"
#include "render/hud_fx.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace inc {

// =============================================================================
// Profile creation screen - text entry for a new profile name.
// =============================================================================
void App::draw_profile_create(render::Renderer& r) const {
    const f32 t = f32(platform::time_seconds());

    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({0, 0}, {f32(VIRTUAL_W), 0},
                {f32(VIRTUAL_W), f32(VIRTUAL_H)}, {0, f32(VIRTUAL_H)},
                rgba(3, 8, 14, 255));

    fx::hex_grid(r, t * 0.2f, COLOR_CORE_EDGE, 0.22f, 110.0f, 10, 18);
    fx::constellation(r, t * 0.5f, COLOR_CORE_INNER, 0.30f, 40, 6.0f);

    const f32 cx = f32(VIRTUAL_W) * 0.5f;
    const f32 cy = f32(VIRTUAL_H) * 0.42f;

    // Heading
    r.draw_text_centered({cx, cy - 160.0f}, "NEW PROFILE", 5.0f, COLOR_CORE_INNER);
    r.draw_text_centered({cx, cy - 100.0f}, "ENTER A NAME",
                         2.2f, with_alpha(COLOR_FACET_EDGE, 200));

    // Input panel
    const f32 pw = 720.0f;
    const f32 ph = 110.0f;
    const f32 px0 = cx - pw * 0.5f;
    const f32 py0 = cy - ph * 0.5f;
    hud::hud_frame(r, px0, py0, px0 + pw, py0 + ph, "PROFILE NAME",
                   COLOR_CORE_EDGE, 1.0f);

    // Typed text + caret
    const f32 text_x = px0 + 40.0f;
    const f32 text_y = cy - 14.0f;
    if (profile_input_len_ > 0) {
        r.draw_text({text_x, text_y}, profile_input_name_, 4.5f, COLOR_CORE_INNER);
    } else {
        r.draw_text({text_x, text_y}, "_", 4.5f,
                    with_alpha(COLOR_FACET_EDGE, 140));
    }
    const f32 cw = r.text_width(profile_input_name_, 4.5f);
    const f32 blink = 0.5f + 0.5f * std::sin(t * 9.0f);
    if (blink > 0.4f) {
        r.draw_quad({text_x + cw + 4.0f, text_y},
                    {text_x + cw + 16.0f, text_y},
                    {text_x + cw + 16.0f, text_y + 46.0f},
                    {text_x + cw + 4.0f, text_y + 46.0f},
                    with_alpha(COLOR_CORE_INNER, 220));
    }

    // Rules
    r.draw_text_centered({cx, cy + 110.0f},
        "1-16 characters  |  letters, digits, _ and - only",
        1.8f, with_alpha(COLOR_FACET_EDGE, 180));

    // Prompts
    const f32 pulse = 0.5f + 0.5f * std::sin(t * 3.0f);
    r.draw_text_centered({cx, cy + 200.0f},
        "ENTER: CONFIRM     ESC: BACK",
        2.2f, with_alpha(COLOR_CORE_INNER, u8(180 + 60 * pulse)));

    fx::scanlines(r, 0.6f, 4.0f, 28);
    fx::film_grain(r, t, 0.4f, 140, 1.0f);
    fx::vignette(r, 1.0f, 170.0f, 10);
}

// =============================================================================
// Profile selection screen - pick an existing profile to continue.
// =============================================================================
void App::draw_profile_select(render::Renderer& r) const {
    const f32 t = f32(platform::time_seconds());

    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({0, 0}, {f32(VIRTUAL_W), 0},
                {f32(VIRTUAL_W), f32(VIRTUAL_H)}, {0, f32(VIRTUAL_H)},
                rgba(3, 8, 14, 255));

    fx::hex_grid(r, t * 0.2f, COLOR_CORE_EDGE, 0.22f, 110.0f, 10, 18);
    fx::constellation(r, t * 0.5f, COLOR_CORE_INNER, 0.30f, 40, 6.0f);

    const f32 cx = f32(VIRTUAL_W) * 0.5f;
    const f32 cy = f32(VIRTUAL_H) * 0.42f;

    r.draw_text_centered({cx, cy - 200.0f}, "SELECT PROFILE", 5.0f, COLOR_CORE_INNER);

    if (profile_list_count_ == 0) {
        r.draw_text_centered({cx, cy}, "NO PROFILES YET",
                             3.0f, with_alpha(COLOR_FACET_EDGE, 220));
        r.draw_text_centered({cx, cy + 60.0f},
            "GO BACK AND CHOOSE START NEW RUN",
            2.0f, with_alpha(COLOR_FACET_EDGE, 180));
    } else {
        const f32 row_h = 66.0f;
        const f32 start_y = cy - (f32(profile_list_count_ - 1) * row_h) * 0.5f;
        const f32 panel_w = 760.0f;

        for (int i = 0; i < profile_list_count_; ++i) {
            const f32 row_y = start_y + f32(i) * row_h;
            const bool sel = (i == profile_cursor_);
            const auto& p = profile_list_[i];

            const f32 a = sel ? 1.0f : 0.55f;
            const Color edge = sel ? COLOR_CORE_INNER : COLOR_CORE_EDGE;

            hud::hud_frame(r, cx - panel_w * 0.5f, row_y - row_h * 0.5f + 4.0f,
                           cx + panel_w * 0.5f, row_y + row_h * 0.5f - 4.0f,
                           nullptr, edge, a);

            r.draw_text({cx - panel_w * 0.5f + 30.0f, row_y - 14.0f},
                        p.name, 3.0f, with_alpha(edge, u8(a * 255.0f)));

            if (p.has_save) {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "LV %02d   ROOM %02d", p.level, p.room);
                const f32 bw = r.text_width(buf, 2.0f);
                r.draw_text({cx + panel_w * 0.5f - 30.0f - bw, row_y - 8.0f},
                            buf, 2.0f, with_alpha(COLOR_FACET_EDGE, u8(a * 220.0f)));
            } else {
                r.draw_text({cx + panel_w * 0.5f - 200.0f, row_y - 8.0f},
                            "EMPTY", 2.0f,
                            with_alpha(COLOR_FACET_EDGE, u8(a * 160.0f)));
            }

            if (sel) {
                const f32 px = cx - panel_w * 0.5f - 24.0f;
                r.draw_quad({px, row_y - 12.0f}, {px + 10.0f, row_y - 12.0f},
                            {px + 10.0f, row_y + 12.0f}, {px, row_y + 12.0f},
                            COLOR_CORE_INNER);
            }
        }
    }

    const f32 pulse = 0.5f + 0.5f * std::sin(t * 3.0f);
    r.draw_text_centered({cx, cy + 260.0f},
        "UP / DOWN: SELECT     ENTER: CONTINUE     D: DELETE     ESC: BACK",
        2.0f, with_alpha(COLOR_CORE_INNER, u8(170 + 60 * pulse)));

    fx::scanlines(r, 0.6f, 4.0f, 28);
    fx::film_grain(r, t, 0.4f, 140, 1.0f);
    fx::vignette(r, 1.0f, 170.0f, 10);
}

} // namespace inc
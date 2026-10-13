#include "app/app.hpp"
#include "render/icons.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace inc {

void App::draw_character_sheet(render::Renderer& r) const {
    const game::Character& ch = world_.character();

    render::draw_menu_background(r, f32(platform::time_seconds()), COLOR_CORE_EDGE, 110);
    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({ 0, 0 }, { VIRTUAL_W, 0 }, { VIRTUAL_W, VIRTUAL_H }, { 0, VIRTUAL_H },
                with_alpha(COLOR_VOID, 140));

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 panel_top = VIRTUAL_H * 0.14f;
    const f32 panel_w = 900.0f;
    const f32 panel_h = 700.0f;
    const f32 panel_left = cx - panel_w * 0.5f;

    r.draw_quad({ panel_left, panel_top }, { panel_left + panel_w, panel_top },
                { panel_left + panel_w, panel_top + panel_h }, { panel_left, panel_top + panel_h },
                with_alpha(COLOR_CORE_FILL, 200));
    r.draw_line({ panel_left, panel_top }, { panel_left + panel_w, panel_top }, 2.0f, COLOR_CORE_EDGE);
    r.draw_line({ panel_left, panel_top + panel_h }, { panel_left + panel_w, panel_top + panel_h }, 2.0f, COLOR_CORE_EDGE);

    const f32 pad_x = panel_left + 60.0f;
    f32 y = panel_top + 40.0f;

    r.draw_text({ pad_x, y }, "UNIT KITE-9", 3.6f, COLOR_CORE_INNER);
    char lv_buf[32];
    std::snprintf(lv_buf, sizeof(lv_buf), "LEVEL %d", ch.level);
    const f32 lv_w = r.text_width(lv_buf, 3.6f);
    r.draw_text({ panel_left + panel_w - 60.0f - lv_w, y }, lv_buf, 3.6f, COLOR_CORE_EDGE);
    y += 60.0f;

    const int xp_need = ch.xp_to_next();
    char xp_buf[64];
    if (xp_need > 0) std::snprintf(xp_buf, sizeof(xp_buf), "XP   %d / %d", ch.xp, xp_need);
    else             std::snprintf(xp_buf, sizeof(xp_buf), "XP   MAX");
    r.draw_text({ pad_x, y }, xp_buf, 2.2f, COLOR_FACET_EDGE);

    const f32 xp_bar_x = pad_x + 260.0f;
    const f32 xp_bar_y = y + 4.0f;
    const f32 xp_bar_w = 520.0f;
    const f32 xp_bar_h = 14.0f;
    r.draw_quad({ xp_bar_x, xp_bar_y }, { xp_bar_x + xp_bar_w, xp_bar_y },
                { xp_bar_x + xp_bar_w, xp_bar_y + xp_bar_h }, { xp_bar_x, xp_bar_y + xp_bar_h },
                with_alpha(COLOR_CORE_EDGE, 50));
    const f32 xp_fill = xp_bar_w * ch.xp_progress();
    if (xp_fill > 0.5f) {
        r.draw_quad({ xp_bar_x, xp_bar_y }, { xp_bar_x + xp_fill, xp_bar_y },
                    { xp_bar_x + xp_fill, xp_bar_y + xp_bar_h }, { xp_bar_x, xp_bar_y + xp_bar_h },
                    COLOR_CORE_EDGE);
    }
    y += 70.0f;

    struct Row { const char* name; int value; f32 derived; const char* unit; };
    const Row rows[4] = {
        { "ARC",    ch.stats.arc,    rad_to_deg(ch.shield_span_rad()), "deg"   },
        { "FOCUS",  ch.stats.focus,  ch.spin_speed(),                   "rad/s" },
        { "CHARGE", ch.stats.charge, ch.reflect_multiplier(),           "x"     },
        { "DRIFT",  ch.stats.drift,  ch.dash_impulse(),                 "px/s"  },
    };

    for (int i = 0; i < 4; ++i) {
        const Row& row = rows[i];

        char key_buf[8];
        std::snprintf(key_buf, sizeof(key_buf), "[%d]", i + 1);
        r.draw_text({ pad_x, y }, key_buf, 2.0f, with_alpha(COLOR_FACET_EDGE, 200));
        r.draw_text({ pad_x + 50.0f, y }, row.name, 2.4f, COLOR_CORE_INNER);

        const f32 bar_x = pad_x + 220.0f;
        const f32 bar_w = 240.0f;
        const f32 bar_h = 14.0f;
        const f32 bar_y = y + 4.0f;
        const int range = game::STAT_MAX - game::STAT_MIN;
        const f32 frac = clamp(f32(row.value - game::STAT_MIN) / f32(range), 0.0f, 1.0f);

        r.draw_quad({ bar_x, bar_y }, { bar_x + bar_w, bar_y },
                    { bar_x + bar_w, bar_y + bar_h }, { bar_x, bar_y + bar_h },
                    with_alpha(COLOR_CORE_EDGE, 50));
        if (frac > 0.01f) {
            r.draw_quad({ bar_x, bar_y }, { bar_x + bar_w * frac, bar_y },
                        { bar_x + bar_w * frac, bar_y + bar_h }, { bar_x, bar_y + bar_h },
                        COLOR_CORE_EDGE);
        }

        char num_buf[16];
        std::snprintf(num_buf, sizeof(num_buf), "%d", row.value);
        r.draw_text({ bar_x + bar_w + 20.0f, y }, num_buf, 2.4f, COLOR_CORE_INNER);

        char derived_buf[48];
        std::snprintf(derived_buf, sizeof(derived_buf), "= %.1f %s",
                      static_cast<double>(row.derived), row.unit);
        r.draw_text({ bar_x + bar_w + 90.0f, y }, derived_buf, 2.0f,
                    with_alpha(COLOR_FACET_EDGE, 220));
        y += 46.0f;
    }

    y += 30.0f;

    char pts_buf[48];
    std::snprintf(pts_buf, sizeof(pts_buf), "POINTS TO SPEND: %d", ch.stat_points);
    const Color pts_color = (ch.stat_points > 0) ? COLOR_CORE_INNER : with_alpha(COLOR_FACET_EDGE, 140);
    r.draw_text({ pad_x, y }, pts_buf, 2.6f, pts_color);

    if (ch.stat_points > 0) {
        r.draw_text({ pad_x, y + 40.0f }, "PRESS 1 / 2 / 3 / 4 TO SPEND", 2.0f,
                    with_alpha(COLOR_FACET_EDGE, 200));
    }

    r.draw_text({ panel_left + panel_w - 240.0f, panel_top + panel_h - 40.0f },
                "[TAB] / [ESC] TO CLOSE", 2.0f, with_alpha(COLOR_FACET_EDGE, 200));
}

void App::draw_skill_tree(render::Renderer& r) const {
    const game::Character& ch = world_.character();

    render::draw_menu_background(r, f32(platform::time_seconds()), COLOR_CORE_EDGE, 110);
    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({ 0, 0 }, { VIRTUAL_W, 0 }, { VIRTUAL_W, VIRTUAL_H }, { 0, VIRTUAL_H },
                with_alpha(COLOR_VOID, 140));

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 panel_top = VIRTUAL_H * 0.10f;
    const f32 panel_w = 1300.0f;
    const f32 panel_h = 800.0f;
    const f32 panel_left = cx - panel_w * 0.5f;

    r.draw_quad({ panel_left, panel_top }, { panel_left + panel_w, panel_top },
                { panel_left + panel_w, panel_top + panel_h }, { panel_left, panel_top + panel_h },
                with_alpha(COLOR_CORE_FILL, 200));
    r.draw_line({ panel_left, panel_top }, { panel_left + panel_w, panel_top }, 2.0f, COLOR_CORE_EDGE);
    r.draw_line({ panel_left, panel_top + panel_h }, { panel_left + panel_w, panel_top + panel_h }, 2.0f, COLOR_CORE_EDGE);

    r.draw_text({ panel_left + 40.0f, panel_top + 30.0f }, "SKILL TREE", 3.6f, COLOR_CORE_INNER);

    char pts_buf[48];
    std::snprintf(pts_buf, sizeof(pts_buf), "SKILL POINTS: %d", ch.skill_points);
    const f32 pts_w = r.text_width(pts_buf, 2.6f);
    r.draw_text({ panel_left + panel_w - 40.0f - pts_w, panel_top + 40.0f },
                pts_buf, 2.6f,
                ch.skill_points > 0 ? COLOR_CORE_INNER : with_alpha(COLOR_FACET_EDGE, 140));

    const f32 branch_w = (panel_w - 80.0f) / 3.0f;
    const f32 col0_x = panel_left + 40.0f;
    const char* branch_names[3] = { "MIRROR", "FRAME", "WELL" };
    const Color branch_colors[3] = { COLOR_CORE_EDGE, COLOR_LEAD_EDGE, COLOR_BANKER_EDGE };

    const f32 header_y = panel_top + 110.0f;
    for (int b = 0; b < 3; ++b) {
        const f32 bx = col0_x + f32(b) * (branch_w + 20.0f);
        const f32 bw = r.text_width(branch_names[b], 3.0f);
        r.draw_text({ bx + (branch_w - bw) * 0.5f, header_y }, branch_names[b], 3.0f, branch_colors[b]);
        r.draw_line({ bx + 20.0f, header_y + 50.0f },
                    { bx + branch_w - 20.0f, header_y + 50.0f },
                    1.5f, with_alpha(branch_colors[b], 140));
    }

    const f32 row0_y = header_y + 80.0f;
    constexpr f32 ROW_H = 100.0f;
    constexpr f32 NODE_H = 80.0f;
    constexpr f32 NODE_PAD = 20.0f;

    for (int b = 0; b < 3; ++b) {
        const f32 bx = col0_x + f32(b) * (branch_w + 20.0f);
        const f32 bw = branch_w;

        for (int row = 0; row < 5; ++row) {
            const int sid = b * 5 + row;
            const game::SkillId skill = game::SkillId(sid);
            const game::SkillNode& node = game::skill_node(skill);

            const f32 ry = row0_y + f32(row) * ROW_H;
            const bool learned = ch.has_skill(skill);
            const bool can_learn = ch.can_learn(skill);
            const bool selected = (skill_cursor_branch_ == b && skill_cursor_row_ == row);

            const Color fill = learned ? with_alpha(branch_colors[b], 90)
                              : (can_learn ? with_alpha(branch_colors[b], 40)
                                           : with_alpha(COLOR_VOID, 160));
            r.draw_quad({ bx + NODE_PAD, ry }, { bx + bw - NODE_PAD, ry },
                        { bx + bw - NODE_PAD, ry + NODE_H }, { bx + NODE_PAD, ry + NODE_H }, fill);

            const Color border = learned ? branch_colors[b]
                                : (selected ? COLOR_CORE_INNER : with_alpha(branch_colors[b], 80));
            const f32 bw_line = selected ? 2.5f : 1.5f;
            r.draw_line({ bx + NODE_PAD, ry }, { bx + bw - NODE_PAD, ry }, bw_line, border);
            r.draw_line({ bx + bw - NODE_PAD, ry }, { bx + bw - NODE_PAD, ry + NODE_H }, bw_line, border);
            r.draw_line({ bx + bw - NODE_PAD, ry + NODE_H }, { bx + NODE_PAD, ry + NODE_H }, bw_line, border);
            r.draw_line({ bx + NODE_PAD, ry + NODE_H }, { bx + NODE_PAD, ry }, bw_line, border);

            char lv_req[16];
            std::snprintf(lv_req, sizeof(lv_req), "L%d", node.min_level);
            const f32 lv_w = r.text_width(lv_req, 1.6f);
            r.draw_text({ bx + bw - NODE_PAD - 15.0f - lv_w, ry + 8.0f }, lv_req, 1.6f,
                        with_alpha(branch_colors[b], learned ? 255 : 160));

            r.draw_text({ bx + NODE_PAD + 15.0f, ry + 12.0f }, node.name, 2.0f,
                        learned ? COLOR_CORE_INNER
                                : (can_learn ? branch_colors[b] : with_alpha(COLOR_FACET_EDGE, 160)));
            r.draw_text({ bx + NODE_PAD + 15.0f, ry + 40.0f }, node.desc, 1.6f,
                        with_alpha(COLOR_FACET_EDGE, 200));

            if (learned) {
                const Vec2 c { bx + bw - NODE_PAD - 18.0f, ry + NODE_H - 18.0f };
                r.draw_circle(c, 6.0f, branch_colors[b], 12);
                r.draw_circle(c, 3.0f, COLOR_VOID, 8);
            }
        }

        const f32 line_x = bx + bw * 0.5f;
        for (int row = 0; row < 4; ++row) {
            const f32 y0 = row0_y + f32(row) * ROW_H + NODE_H;
            const f32 y1 = row0_y + f32(row + 1) * ROW_H;
            const bool a_ok = ch.has_skill(game::SkillId(b * 5 + row));
            const bool b_ok = ch.has_skill(game::SkillId(b * 5 + row + 1));
            const Color lc = (a_ok && b_ok) ? branch_colors[b]
                            : (a_ok ? with_alpha(branch_colors[b], 180) : with_alpha(branch_colors[b], 60));
            r.draw_line({ line_x, y0 }, { line_x, y1 }, 2.0f, lc);
        }
    }

    r.draw_text({ panel_left + 40.0f, panel_top + panel_h - 60.0f },
                "ARROWS TO NAVIGATE    [ENTER] TO LEARN    [K] / [ESC] TO CLOSE",
                2.0f, with_alpha(COLOR_FACET_EDGE, 200));
}

void App::draw_equipment_panel(render::Renderer& r) const {
    const game::Character& ch = world_.character();

    render::draw_menu_background(r, f32(platform::time_seconds()), COLOR_CORE_EDGE, 110);
    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({ 0, 0 }, { VIRTUAL_W, 0 }, { VIRTUAL_W, VIRTUAL_H }, { 0, VIRTUAL_H },
                with_alpha(COLOR_VOID, 140));

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 panel_top = VIRTUAL_H * 0.16f;
    const f32 panel_w = 900.0f;
    const f32 panel_h = 680.0f;
    const f32 panel_left = cx - panel_w * 0.5f;

    r.draw_quad({ panel_left, panel_top }, { panel_left + panel_w, panel_top },
                { panel_left + panel_w, panel_top + panel_h }, { panel_left, panel_top + panel_h },
                with_alpha(COLOR_CORE_FILL, 200));
    r.draw_line({ panel_left, panel_top }, { panel_left + panel_w, panel_top }, 2.0f, COLOR_CORE_EDGE);
    r.draw_line({ panel_left, panel_top + panel_h }, { panel_left + panel_w, panel_top + panel_h }, 2.0f, COLOR_CORE_EDGE);

    const f32 pad_x = panel_left + 60.0f;
    f32 y = panel_top + 40.0f;

    r.draw_text({ pad_x, y }, "EQUIPMENT", 3.6f, COLOR_CORE_INNER);
    y += 80.0f;

    struct Row {
        const char* slot;
        const char* name;
        const char* desc;
        int         count_unlocked;
        bool        selected;
    };

    const Row rows[3] = {
        { "FACET", game::facet_name(ch.equipped.facet),
                   game::facet_desc(ch.equipped.facet),
                   1 + (ch.inventory.is_unlocked(game::FacetId::Wide) ? 1 : 0)
                     + (ch.inventory.is_unlocked(game::FacetId::Heavy) ? 1 : 0),
                   equipment_slot_ == 0 },
        { "CORE",  game::core_name(ch.equipped.core),
                   game::core_desc(ch.equipped.core),
                   1 + (ch.inventory.is_unlocked(game::CoreId::Coral) ? 1 : 0)
                     + (ch.inventory.is_unlocked(game::CoreId::Quartz) ? 1 : 0),
                   equipment_slot_ == 1 },
        { "HULL",  game::hull_name(ch.equipped.hull),
                   game::hull_desc(ch.equipped.hull),
                   1 + (ch.inventory.is_unlocked(game::HullId::Reactive) ? 1 : 0)
                     + (ch.inventory.is_unlocked(game::HullId::Silent) ? 1 : 0),
                   equipment_slot_ == 2 },
    };

    for (int i = 0; i < 3; ++i) {
        const Row& row = rows[i];
        const f32 row_h = 140.0f;

        if (row.selected) {
            r.draw_quad({ panel_left + 30.0f, y - 10.0f },
                        { panel_left + panel_w - 30.0f, y - 10.0f },
                        { panel_left + panel_w - 30.0f, y + row_h - 10.0f },
                        { panel_left + 30.0f, y + row_h - 10.0f },
                        with_alpha(COLOR_CORE_EDGE, 40));
            r.draw_line({ panel_left + 30.0f, y - 10.0f },
                        { panel_left + panel_w - 30.0f, y - 10.0f },
                        2.0f, COLOR_CORE_INNER);
            r.draw_line({ panel_left + 30.0f, y + row_h - 10.0f },
                        { panel_left + panel_w - 30.0f, y + row_h - 10.0f },
                        2.0f, COLOR_CORE_INNER);
        }

        r.draw_text({ pad_x, y + 20.0f }, row.slot, 2.6f,
                    row.selected ? COLOR_CORE_INNER : with_alpha(COLOR_FACET_EDGE, 200));
        r.draw_text({ pad_x + 180.0f, y + 20.0f }, row.name, 2.8f,
                    row.selected ? COLOR_CORE_INNER : COLOR_CORE_EDGE);
        r.draw_text({ pad_x + 180.0f, y + 60.0f }, row.desc, 2.0f,
                    with_alpha(COLOR_FACET_EDGE, 220));

        char count_buf[32];
        std::snprintf(count_buf, sizeof(count_buf), "%d / 3", row.count_unlocked);
        const f32 cw = r.text_width(count_buf, 2.0f);
        r.draw_text({ panel_left + panel_w - 60.0f - cw, y + 20.0f }, count_buf, 2.0f,
                    with_alpha(COLOR_FACET_EDGE, 180));

        y += row_h;
    }

    y += 20.0f;

    r.draw_text({ pad_x, y },
                "UP / DOWN: SELECT SLOT    LEFT / RIGHT: CHANGE ITEM",
                2.0f, with_alpha(COLOR_FACET_EDGE, 200));
    r.draw_text({ pad_x, y + 30.0f }, "[I] / [ESC] TO CLOSE",
                2.0f, with_alpha(COLOR_FACET_EDGE, 200));
}

void App::draw_log_overlay(render::Renderer& r) const {
    const auto& lg = logs_;
    if (!lg.playing()) return;

    const game::LogEntry& entry = game::log_at(lg.active_index);
    const int body_len = int(std::strlen(entry.body));

    // Panel entrance — driven by typing age, forced to 1 if typing already done
    const f32 open = lg.fully_shown ? 1.0f
                    : clamp(lg.char_timer / 0.30f, 0.0f, 1.0f);
    const f32 ease = 1.0f - (1.0f - open) * (1.0f - open);
    const f32 slide = (1.0f - ease) * 70.0f;

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 panel_top  = VIRTUAL_H * 0.28f + slide;
    const f32 panel_w    = 1100.0f;
    const f32 panel_h    = 480.0f;
    const f32 panel_left = cx - panel_w * 0.5f;
    const f32 panel_right  = panel_left + panel_w;
    const f32 panel_bottom = panel_top + panel_h;

    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({0,0}, {VIRTUAL_W,0}, {VIRTUAL_W,VIRTUAL_H}, {0,VIRTUAL_H},
                with_alpha(COLOR_VOID, u8(ease * 215.0f)));

    // Drop shadow
    r.draw_quad({panel_left + 14, panel_top + 14},
                {panel_right + 14, panel_top + 14},
                {panel_right + 14, panel_bottom + 14},
                {panel_left + 14, panel_bottom + 14},
                rgba(0,0,0,u8(ease * 160.0f)));

    // Panel body
    r.draw_quad({panel_left, panel_top}, {panel_right, panel_top},
                {panel_right, panel_bottom}, {panel_left, panel_bottom},
                with_alpha(COLOR_CORE_FILL, u8(ease * 235.0f)));

    // Border lines draw outward from center
    const f32 bl = panel_w * ease * 0.5f;
    r.draw_line({cx - bl, panel_top}, {cx + bl, panel_top}, 2.0f, COLOR_CORE_EDGE);
    r.draw_line({cx - bl, panel_bottom}, {cx + bl, panel_bottom}, 2.0f, COLOR_CORE_EDGE);

    // Left accent strip
    r.draw_quad({panel_left + 10, panel_top + 10},
                {panel_left + 16, panel_top + 10},
                {panel_left + 16, panel_bottom - 10},
                {panel_left + 10, panel_bottom - 10},
                with_alpha(COLOR_CORE_EDGE, u8(ease * 220.0f)));

    // Scanline sweep across the panel during first 0.5s
    if (lg.char_timer < 0.5f) {
        const f32 t = lg.char_timer / 0.5f;
        const f32 sy = panel_top + t * panel_h;
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_quad({panel_left, sy - 2.5f}, {panel_right, sy - 2.5f},
                    {panel_right, sy + 2.5f}, {panel_left, sy + 2.5f},
                    with_alpha(COLOR_CORE_INNER, u8((1.0f - t) * 200.0f)));
        r.set_layer(render::Renderer::Layer::Alpha);
    }

    // Header
    if (lg.char_timer > 0.10f) {
        const f32 ha = clamp((lg.char_timer - 0.10f) / 0.35f, 0.0f, 1.0f);
        r.draw_text({panel_left + 40, panel_top + 30},
                    "YARA OKONKWO - MAINTENANCE LOG", 2.6f,
                    with_alpha(COLOR_CORE_EDGE, u8(ha * 255.0f)));
    }

    // Day stamp slides in from the right
    if (lg.char_timer > 0.22f) {
        char day_buf[32];
        std::snprintf(day_buf, sizeof(day_buf), "DAY %d", entry.day);
        const f32 da = clamp((lg.char_timer - 0.22f) / 0.28f, 0.0f, 1.0f);
        const f32 dw = r.text_width(day_buf, 2.4f);
        r.draw_text({panel_right - 40 - dw + (1.0f - da) * 80.0f,
                     panel_top + 30},
                    day_buf, 2.4f,
                    with_alpha(COLOR_FACET_EDGE, u8(da * 220.0f)));
    }

    // Divider draws outward from center
    if (lg.char_timer > 0.30f) {
        const f32 d = clamp((lg.char_timer - 0.30f) / 0.30f, 0.0f, 1.0f);
        const f32 dhw = (panel_w - 80.0f) * 0.5f * d;
        r.draw_line({cx - dhw, panel_top + 80},
                    {cx + dhw, panel_top + 80},
                    1.0f, with_alpha(COLOR_CORE_EDGE, u8(d * 150.0f)));
    }

    // Body — word-wrapped with typing cursor
    const int shown = lg.chars_shown < body_len ? lg.chars_shown : body_len;
    const f32 text_x = panel_left + 40.0f;
    const f32 text_y0 = panel_top + 115.0f;
    constexpr f32 LINE_PX = 2.0f;
    constexpr int CHARS_PER_LINE = 72;
    constexpr f32 LINE_H = 34.0f;

    int line = 0;
    int col = 0;
    char buf[CHARS_PER_LINE + 4];
    for (int k = 0; k < shown; ++k) {
        if (col < CHARS_PER_LINE) buf[col++] = entry.body[k];
        if (col >= CHARS_PER_LINE) {
            buf[col] = '\0';
            r.draw_text({text_x, text_y0 + f32(line) * LINE_H}, buf,
                        LINE_PX, COLOR_CORE_INNER);
            ++line; col = 0;
        }
    }
    if (col > 0) {
        buf[col] = '\0';
        r.draw_text({text_x, text_y0 + f32(line) * LINE_H}, buf,
                    LINE_PX, COLOR_CORE_INNER);
    }

    // Typing cursor
    if (!lg.fully_shown && shown > 0) {
        const int cur_line = shown / CHARS_PER_LINE;
        const int cur_col  = shown % CHARS_PER_LINE;
        if (cur_line < 16) {
            char before[CHARS_PER_LINE + 1];
            const int lstart = cur_line * CHARS_PER_LINE;
            for (int k = 0; k < cur_col; ++k) before[k] = entry.body[lstart + k];
            before[cur_col] = '\0';
            const f32 curs_x = text_x + r.text_width(before, LINE_PX) + 2.0f;
            const f32 curs_y = text_y0 + f32(cur_line) * LINE_H;
            const f32 blink = 0.5f + 0.5f * std::sin(platform::time_seconds() * 14.0f);
            r.draw_quad({curs_x, curs_y}, {curs_x + 14.0f, curs_y},
                        {curs_x + 14.0f, curs_y + 16.0f}, {curs_x, curs_y + 16.0f},
                        with_alpha(COLOR_CORE_INNER, u8(140 + 100 * blink)));
        }
    }

    // Progress bar (chars shown / body)
    if (body_len > 0) {
        const f32 pb_w = panel_w - 80.0f;
        const f32 pb_x = panel_left + 40.0f;
        const f32 pb_y = panel_bottom - 34.0f;
        const f32 pb_h = 3.0f;
        r.draw_quad({pb_x, pb_y}, {pb_x + pb_w, pb_y},
                    {pb_x + pb_w, pb_y + pb_h}, {pb_x, pb_y + pb_h},
                    with_alpha(COLOR_CORE_EDGE, 40));
        const f32 f = f32(shown) / f32(body_len);
        if (f > 0.0f) {
            r.draw_quad({pb_x, pb_y}, {pb_x + pb_w * f, pb_y},
                        {pb_x + pb_w * f, pb_y + pb_h}, {pb_x, pb_y + pb_h},
                        COLOR_CORE_EDGE);
        }
    }

    // Continue prompt
    if (lg.fully_shown) {
        const f32 p = 0.5f + 0.5f * std::sin(platform::time_seconds() * 4.5f);
        r.draw_text_centered({cx, panel_bottom - 62.0f},
                             "[SPACE] CONTINUE", 2.2f + p * 0.25f,
                             with_alpha(COLOR_CORE_INNER, u8(170 + 85 * p)));
    } else {
        const f32 p = 0.5f + 0.5f * std::sin(platform::time_seconds() * 3.0f);
        r.draw_text_centered({cx, panel_bottom - 62.0f},
                             "[SPACE] SKIP TYPING", 1.8f,
                             with_alpha(COLOR_FACET_EDGE, u8(110 + 60 * p)));
    }

    // Additive glow under the panel
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_circle({cx, panel_top + panel_h * 0.5f}, 420.0f,
                  with_alpha(COLOR_CORE_EDGE, u8(ease * 14.0f)), 40);
    r.set_layer(render::Renderer::Layer::Alpha);
}

void App::draw_log_archive(render::Renderer& r) const {
    const auto& lg = logs_;

    render::draw_menu_background(r, f32(platform::time_seconds()), COLOR_CORE_EDGE, 110);
    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({ 0, 0 }, { VIRTUAL_W, 0 }, { VIRTUAL_W, VIRTUAL_H }, { 0, VIRTUAL_H },
                with_alpha(COLOR_VOID, 140));

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 panel_top = VIRTUAL_H * 0.10f;
    const f32 panel_w = 1100.0f;
    const f32 panel_h = 800.0f;
    const f32 panel_left = cx - panel_w * 0.5f;

    r.draw_quad({ panel_left, panel_top }, { panel_left + panel_w, panel_top },
                { panel_left + panel_w, panel_top + panel_h }, { panel_left, panel_top + panel_h },
                with_alpha(COLOR_CORE_FILL, 210));
    r.draw_line({ panel_left, panel_top }, { panel_left + panel_w, panel_top }, 2.0f, COLOR_CORE_EDGE);
    r.draw_line({ panel_left, panel_top + panel_h }, { panel_left + panel_w, panel_top + panel_h }, 2.0f, COLOR_CORE_EDGE);

    r.draw_text({ panel_left + 40.0f, panel_top + 30.0f }, "ARCHIVE", 3.6f, COLOR_CORE_INNER);

    char count[32];
    std::snprintf(count, sizeof(count), "%d / %d", lg.seen_count(), game::LOG_COUNT);
    const f32 cw = r.text_width(count, 2.6f);
    r.draw_text({ panel_left + panel_w - 40.0f - cw, panel_top + 40.0f },
                count, 2.6f, COLOR_CORE_EDGE);

    const f32 list_x = panel_left + 40.0f;
    f32 y = panel_top + 110.0f;
    for (int i = 0; i < game::LOG_COUNT; ++i) {
        const game::LogEntry& e = game::log_at(i);
        const bool seen = lg.has_seen(i);
        const bool selected = (archive_cursor_ == i);

        if (selected) {
            r.draw_quad({ panel_left + 30.0f, y - 4.0f },
                        { panel_left + panel_w - 30.0f, y - 4.0f },
                        { panel_left + panel_w - 30.0f, y + 42.0f },
                        { panel_left + 30.0f, y + 42.0f },
                        with_alpha(COLOR_CORE_EDGE, 40));
        }

        char num[16];
        std::snprintf(num, sizeof(num), "%02d", i + 1);
        r.draw_text({ list_x, y }, num, 2.2f, with_alpha(COLOR_FACET_EDGE, 200));

        if (seen) {
            r.draw_text({ list_x + 60.0f, y }, e.trigger, 2.2f,
                        selected ? COLOR_CORE_INNER : COLOR_CORE_EDGE);
        } else {
            r.draw_text({ list_x + 60.0f, y }, "[UNRECOVERED]", 2.2f,
                        with_alpha(COLOR_FACET_EDGE, 120));
        }

        y += 50.0f;
    }

    r.draw_text({ panel_left + 40.0f, panel_top + panel_h - 40.0f },
                "UP / DOWN: SCROLL    [L] / [ESC] TO CLOSE",
                2.0f, with_alpha(COLOR_FACET_EDGE, 200));
}

void App::draw_quest_log(render::Renderer& r) const {
    const auto& qs = quests_;

    render::draw_menu_background(r, f32(platform::time_seconds()), COLOR_CORE_EDGE, 110);
    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({ 0, 0 }, { VIRTUAL_W, 0 }, { VIRTUAL_W, VIRTUAL_H }, { 0, VIRTUAL_H },
                with_alpha(COLOR_VOID, 140));

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 panel_top = VIRTUAL_H * 0.20f;
    const f32 panel_w = 900.0f;
    const f32 panel_h = 600.0f;
    const f32 panel_left = cx - panel_w * 0.5f;

    r.draw_quad({ panel_left, panel_top }, { panel_left + panel_w, panel_top },
                { panel_left + panel_w, panel_top + panel_h }, { panel_left, panel_top + panel_h },
                with_alpha(COLOR_CORE_FILL, 210));
    r.draw_line({ panel_left, panel_top }, { panel_left + panel_w, panel_top }, 2.0f, COLOR_CORE_EDGE);
    r.draw_line({ panel_left, panel_top + panel_h }, { panel_left + panel_w, panel_top + panel_h }, 2.0f, COLOR_CORE_EDGE);

    r.draw_text({ panel_left + 40.0f, panel_top + 30.0f }, "OBJECTIVES", 3.6f, COLOR_CORE_INNER);

    char cnt[32];
    std::snprintf(cnt, sizeof(cnt), "%d / %d",
                  qs.completed_count(), int(game::QuestId::Count));
    const f32 cw = r.text_width(cnt, 2.6f);
    r.draw_text({ panel_left + panel_w - 40.0f - cw, panel_top + 40.0f },
                cnt, 2.6f, COLOR_CORE_EDGE);

    f32 y = panel_top + 130.0f;
    for (int i = 0; i < int(game::QuestId::Count); ++i) {
        const game::QuestId qid = game::QuestId(i);
        const game::Quest& q = game::quest_at(qid);
        const bool done = qs.is_completed(qid);

        const f32 bx = panel_left + 60.0f;
        const f32 by = y + 4.0f;
        const f32 box = 24.0f;

        r.draw_quad({ bx, by }, { bx + box, by },
                    { bx + box, by + box }, { bx, by + box },
                    done ? with_alpha(COLOR_CORE_EDGE, 200)
                         : with_alpha(COLOR_CORE_FILL, 200));

        const Color border = done ? COLOR_CORE_INNER
                                  : with_alpha(COLOR_FACET_EDGE, 160);
        r.draw_line({ bx, by }, { bx + box, by }, 2.0f, border);
        r.draw_line({ bx + box, by }, { bx + box, by + box }, 2.0f, border);
        r.draw_line({ bx + box, by + box }, { bx, by + box }, 2.0f, border);
        r.draw_line({ bx, by + box }, { bx, by }, 2.0f, border);

        if (done) {
            r.draw_line({ bx + 5.0f, by + 12.0f },
                        { bx + 10.0f, by + 18.0f }, 3.0f, COLOR_CORE_INNER);
            r.draw_line({ bx + 10.0f, by + 18.0f },
                        { bx + 20.0f, by + 5.0f }, 3.0f, COLOR_CORE_INNER);
        }

        r.draw_text({ panel_left + 110.0f, y }, q.title, 2.4f,
                    done ? COLOR_CORE_INNER : COLOR_CORE_EDGE);
        r.draw_text({ panel_left + 110.0f, y + 34.0f }, q.hint, 1.8f,
                    with_alpha(COLOR_FACET_EDGE, done ? 160 : 220));

        y += 100.0f;
    }

    r.draw_text({ panel_left + 40.0f, panel_top + panel_h - 40.0f },
                "[J] / [ESC] TO CLOSE",
                2.0f, with_alpha(COLOR_FACET_EDGE, 200));
}

} // namespace inc
// ============================================================================
// World map (M key): node graph of rooms + Warden node.
// ============================================================================
#include <cmath>

namespace inc {

void App::draw_world_map(render::Renderer& r) const {
    render::draw_menu_background(r, f32(platform::time_seconds()), COLOR_CORE_EDGE, 110);
    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({0.0f, 0.0f}, {VIRTUAL_W, 0.0f},
                {VIRTUAL_W, VIRTUAL_H}, {0.0f, VIRTUAL_H},
                rgba(5, 14, 18, 140));

    r.draw_text_centered({VIRTUAL_W * 0.5f, 60.0f},
                         "HALCYON STATION - RING MAP", 4.0f, COLOR_CORE_INNER);

    constexpr int NODES = 10;
    const f32 SPACING = 110.0f;
    const f32 R       = 26.0f;
    const f32 cx      = VIRTUAL_W * 0.5f;
    const f32 cy      = VIRTUAL_H * 0.5f;
    const f32 start_x = cx - f32(NODES - 1) * SPACING * 0.5f;

    static const char* NODE_NUM[10] = {
        "0","1","2","3","4","5","6","7","8","9"
    };
    static const char* NODE_NAME[10] = {
        "ARENA", "CROSS", "CORRIDOR", "HOURGLASS", "CHOKE",
        "SPIRAL", "PINCH", "CROSSFIRE", "THE BOX", "WARDEN"
    };

    for (int i = 0; i < NODES - 1; ++i) {
        const f32 x0 = start_x + f32(i) * SPACING;
        const f32 x1 = x0 + SPACING;
        const bool link_open = world_.is_room_cleared(i);
        const Color lc = link_open
            ? with_alpha(COLOR_CORE_EDGE, 140)
            : with_alpha(COLOR_CORE_EDGE,  40);
        r.draw_line({ x0 + R, cy }, { x1 - R, cy }, 3.0f, lc);
    }

    for (int i = 0; i < NODES; ++i) {
        const bool is_boss = (i == NODES - 1);
        const f32  x       = start_x + f32(i) * SPACING;

        bool cleared = false, current = false;
        if (is_boss) {
            cleared = world_.warden_defeated();
            current = world_.warden_active();
        } else {
            cleared = world_.is_room_cleared(i);
            current = (i == world_.room_index());
        }
        const bool selected = (i == map_cursor_);

        const Color base = is_boss ? COLOR_ENEMY_EDGE : COLOR_CORE_EDGE;
        r.draw_circle({ x, cy }, R, with_alpha(base, u8(cleared ? 180 : 40)), 20);

        Color ring = with_alpha(base, 140);
        if (selected || current) ring = COLOR_CORE_INNER;
        r.draw_ring({ x, cy }, R, R + 3.0f, ring, 24);

        if (selected) {
            const f32 pulse = 0.5f + 0.5f * std::sin(platform::time_seconds() * 5.0f);
            r.set_layer(render::Renderer::Layer::Additive);
            r.draw_ring({ x, cy }, R + 6.0f, R + 9.0f + pulse * 3.0f,
                        with_alpha(COLOR_CORE_INNER, u8(120 + 100 * pulse)), 24);
            r.set_layer(render::Renderer::Layer::Alpha);
        }

        r.draw_text_centered({ x, cy - 10.0f }, NODE_NUM[i], 3.0f,
                             cleared ? COLOR_VOID : COLOR_CORE_INNER);
        r.draw_text_centered({ x, cy + R + 16.0f }, NODE_NAME[i], 2.0f,
                             with_alpha(COLOR_CORE_INNER, 220));
    }

    r.draw_text_centered({ cx, VIRTUAL_H - 80.0f },
                         "LEFT/RIGHT: MOVE    ENTER: FAST-TRAVEL    ESC: CLOSE",
                         2.4f, with_alpha(COLOR_FACET_EDGE, 200));
}

} // namespace inc
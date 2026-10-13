#include "app/app.hpp"
#include "audio/audio.hpp"
#include "render/icons.hpp"
#include "game/save.hpp"
#include "game/room.hpp"

#include "sokol_app.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace inc {

static constexpr int MAX_SUBSTEPS_PER_FRAME = 8;

static const char* ROOM_BLURB[] = {
    "Central ring junction. The Lancers are still firing at micrometeorites that expired weeks ago.",
    "Deck Four intersection. The Bankers here have not stopped ricocheting since Day 3.",
    "Deck Six service corridor. The Leads know where you will be. They always have.",
    "Arc II rotating lab. Frame correction required.",
    "Bulkhead access. Line of sight: zero.",
    "Arc III gravitic well. Nothing travels straight here.",
    "Arc IV wave lab throat. Sit at the nodes or die at the antinodes.",
    "Observation deck. Every direction is inside.",
    "Arc V, core fold approach. The last room. The only room that matters.",
    "The last coherent fragment of THEOREM. It does not want to kill you. It wants to keep the fold closed forever.",
};

static const char* ROOM_NAME[] = {
    "ARENA", "CROSS", "CORRIDOR", "HOURGLASS", "CHOKE",
    "SPIRAL", "PINCH", "CROSSFIRE", "THE BOX", "WARDEN OF MIRROR",
};

static Color room_arc_color(int idx) {
    if (idx < 3) return COLOR_CORE_EDGE;
    if (idx < 5) return COLOR_LEAD_EDGE;
    if (idx < 6) return COLOR_BANKER_EDGE;
    if (idx < 8) return COLOR_ENEMY_EDGE;
    return COLOR_CORE_INNER;
}

static constexpr f32 ROOM_INTRO_FADE_IN  = 0.30f;
static constexpr f32 ROOM_INTRO_HOLD     = 3.20f;
static constexpr f32 ROOM_INTRO_FADE_OUT = 0.60f;
static constexpr f32 ROOM_INTRO_TOTAL    = ROOM_INTRO_FADE_IN + ROOM_INTRO_HOLD + ROOM_INTRO_FADE_OUT;

struct Star { f32 x, y, r; };
static constexpr Star STARS[] = {
    { 120.0f, 100.0f, 1.0f }, {  260.0f,  60.0f, 0.7f }, {  410.0f, 180.0f, 1.2f },
    {  560.0f,  90.0f, 0.6f }, {  700.0f, 220.0f, 1.0f }, {  850.0f, 140.0f, 0.8f },
    { 1020.0f, 200.0f, 1.1f }, { 1180.0f,  80.0f, 0.7f }, { 1320.0f, 240.0f, 1.0f },
    { 1470.0f, 130.0f, 0.9f }, { 1620.0f, 200.0f, 1.2f }, { 1780.0f, 100.0f, 0.6f },
    {  180.0f, 340.0f, 0.8f }, {  340.0f, 420.0f, 1.0f }, {  500.0f, 350.0f, 0.7f },
    { 1660.0f, 380.0f, 1.0f }, { 1820.0f, 460.0f, 0.7f }, {  140.0f, 560.0f, 0.9f },
    { 1780.0f, 600.0f, 1.0f }, {  200.0f, 720.0f, 0.7f }, { 1740.0f, 780.0f, 0.9f },
    {  120.0f, 880.0f, 1.0f }, {  320.0f, 960.0f, 0.6f }, { 1600.0f, 940.0f, 0.8f },
    { 1820.0f, 1000.0f, 1.1f }, {  540.0f, 1020.0f, 0.7f }, { 1400.0f, 1030.0f, 0.9f },
    {   80.0f, 200.0f, 0.7f }, {  960.0f,  40.0f, 0.9f }, { 1880.0f, 320.0f, 0.8f },
    {  900.0f, 900.0f, 0.7f }, {  440.0f, 620.0f, 0.6f }, { 1560.0f, 700.0f, 0.9f },
    {  660.0f, 480.0f, 0.8f }, { 1180.0f, 460.0f, 0.7f }, {  840.0f, 800.0f, 0.6f },
};
static constexpr int STAR_COUNT = sizeof(STARS) / sizeof(STARS[0]);

App::App()  = default;
App::~App() = default;

void App::init() {
    running_       = true;
    accumulator_   = 0.0;
    last_frame_ts_ = platform::time_seconds();
    frame_count_   = 0;
    screen_        = game::Screen::Intro;
    intro_time_    = 0.0f;
    title_time_    = 0.0f;
    victory_time_  = 0.0f;
    map_cursor_    = 0;

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
    if (platform::quit_requested()) running_ = false;
    ++frame_count_;
}

void App::shutdown() {
    audio::shutdown();
    renderer_.shutdown();
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
        platform::consume_pressed();
    }
    if (substeps == MAX_SUBSTEPS_PER_FRAME) accumulator_ = 0.0;
}

void App::check_quests() {
    if (world_.warden_active())   quests_.complete(game::QuestId::ReachWarden);
    if (world_.warden_defeated()) quests_.complete(game::QuestId::DefeatWarden);
    const int seen = logs_.seen_count();
    if (seen >= 6)  quests_.complete(game::QuestId::HalfArchive);
    if (seen >= 12) quests_.complete(game::QuestId::FullArchive);
}

void App::step(f32 dt) {
    const auto& in = platform::platform_input();

    bool any_pressed = false;
    for (int i = 0; i < static_cast<int>(platform::Key::Count); ++i) {
        if (in.key_pressed[i]) { any_pressed = true; break; }
    }

    switch (screen_) {
    case game::Screen::Intro: {
        intro_time_ += dt;
        if (any_pressed) { screen_ = game::Screen::Title; title_time_ = 0.0f; break; }
        f32 total = 0.0f;
        for (int i = 0; i < 6; ++i) total += 3.5f;
        if (intro_time_ >= total) { screen_ = game::Screen::Title; title_time_ = 0.0f; }
        break;
    }

    case game::Screen::Title: {
        title_time_ += dt;
        if (any_pressed && title_time_ > 0.4f) {
            game::SaveData sd;
            if (game::load_game("save.sav", sd)) {
                world_.character() = sd.character;
                world_.jump_to_room(sd.current_room);
                world_.set_rooms_visited(sd.rooms_visited);
            } else {
                world_.reset();
            }
            logs_   = game::LogState{};
            quests_ = game::QuestState{};
            map_cursor_ = world_.room_index();
            screen_ = game::Screen::Playing;
            last_room_shown_      = -1;
            last_warden_state_    = false;
            room_intro_timer_     = -1.0f;
            character_sheet_open_ = false;
            skill_tree_open_      = false;
            equipment_panel_open_ = false;
            log_archive_open_     = false;
            quest_log_open_       = false;
            world_map_open_       = false;
        }
        break;
    }

    case game::Screen::Playing: {
        if (action_toast_t_ > 0.0f) {
            action_toast_t_ -= dt;
            if (action_toast_t_ < 0.0f) action_toast_t_ = 0.0f;
        }

        if (paused_) {
            if (in.key_pressed[static_cast<int>(platform::Key::Escape)]) {
                paused_ = false;
                break;
            }
            if (in.key_pressed[static_cast<int>(platform::Key::Up)]) {
                if (pause_cursor_ > 0) --pause_cursor_;
            }
            if (in.key_pressed[static_cast<int>(platform::Key::Down)]) {
                if (pause_cursor_ < 2) ++pause_cursor_;
            }
            if (in.key_pressed[static_cast<int>(platform::Key::Enter)]) {
                if (pause_cursor_ == 0) {
                    paused_ = false;
                } else if (pause_cursor_ == 1) {
                    world_.reset();
                    logs_ = game::LogState{};
                    quests_ = game::QuestState{};
                    last_room_shown_   = -1;
                    last_warden_state_ = false;
                    room_intro_timer_  = -1.0f;
                    paused_ = false;
                } else {
                    running_ = false;
                }
            }
            break;
        }

        if (logs_.playing()) {
            const int idx = logs_.active_index;
            const int body_len = int(std::strlen(game::log_at(idx).body));

            if (in.key_pressed[static_cast<int>(platform::Key::Space)] ||
                in.key_pressed[static_cast<int>(platform::Key::Enter)]) {
                if (logs_.fully_shown) {
                    logs_.end();
                } else {
                    logs_.chars_shown = body_len;
                    logs_.fully_shown = true;
                }
            }

            if (!logs_.fully_shown) {
                logs_.char_timer += dt;
                logs_.chars_shown = int(logs_.char_timer * 45.0f);
                if (logs_.chars_shown >= body_len) {
                    logs_.chars_shown = body_len;
                    logs_.fully_shown = true;
                }
            }
            break;
        }

        if (in.key_pressed[static_cast<int>(platform::Key::Tab)]) {
            character_sheet_open_ = !character_sheet_open_;
            if (character_sheet_open_) {
                skill_tree_open_      = false;
                equipment_panel_open_ = false;
                log_archive_open_     = false;
                quest_log_open_       = false;
                world_map_open_       = false;
            }
        }

        if (in.key_pressed[static_cast<int>(platform::Key::K)]) {
            skill_tree_open_ = !skill_tree_open_;
            if (skill_tree_open_) {
                character_sheet_open_ = false;
                equipment_panel_open_ = false;
                log_archive_open_     = false;
                quest_log_open_       = false;
                world_map_open_       = false;
            }
        }

        if (in.key_pressed[static_cast<int>(platform::Key::I)]) {
            equipment_panel_open_ = !equipment_panel_open_;
            if (equipment_panel_open_) {
                character_sheet_open_ = false;
                skill_tree_open_      = false;
                log_archive_open_     = false;
                quest_log_open_       = false;
                world_map_open_       = false;
            }
        }

        if (in.key_pressed[static_cast<int>(platform::Key::L)]) {
            log_archive_open_ = !log_archive_open_;
            if (log_archive_open_) {
                character_sheet_open_ = false;
                skill_tree_open_      = false;
                equipment_panel_open_ = false;
                quest_log_open_       = false;
                world_map_open_       = false;
            }
        }

        if (in.key_pressed[static_cast<int>(platform::Key::J)]) {
            quest_log_open_ = !quest_log_open_;
            if (quest_log_open_) {
                character_sheet_open_ = false;
                skill_tree_open_      = false;
                equipment_panel_open_ = false;
                log_archive_open_     = false;
                world_map_open_       = false;
            }
        }

        if (in.key_pressed[static_cast<int>(platform::Key::M)]) {
            world_map_open_ = !world_map_open_;
            if (world_map_open_) {
                character_sheet_open_ = false;
                skill_tree_open_      = false;
                equipment_panel_open_ = false;
                log_archive_open_     = false;
                quest_log_open_       = false;
                map_cursor_           = world_.room_index();
            }
        }

        const bool any_panel =
            character_sheet_open_ || skill_tree_open_ ||
            equipment_panel_open_ || log_archive_open_ ||
            quest_log_open_       || world_map_open_;

        if (!any_panel && in.key_pressed[static_cast<int>(platform::Key::Escape)]) {
            paused_ = true;
            pause_cursor_ = 0;
            break;
        }

        if (any_panel) {
            if (in.key_pressed[static_cast<int>(platform::Key::Escape)]) {
                character_sheet_open_ = false;
                skill_tree_open_      = false;
                equipment_panel_open_ = false;
                log_archive_open_     = false;
                quest_log_open_       = false;
                world_map_open_       = false;
            }

            // --- Character sheet: 1/2/3/4 spend stat points ---
            if (character_sheet_open_) {
                int idx = -1;
                if (in.key_pressed[static_cast<int>(platform::Key::Num1)]) idx = 0;
                if (in.key_pressed[static_cast<int>(platform::Key::Num2)]) idx = 1;
                if (in.key_pressed[static_cast<int>(platform::Key::Num3)]) idx = 2;
                if (in.key_pressed[static_cast<int>(platform::Key::Num4)]) idx = 3;
                if (idx >= 0) {
                    static const char* STAT_NAMES[4] = {"ARC","FOCUS","CHARGE","DRIFT"};
                    if (world_.character().spend_point(idx)) {
                        std::snprintf(action_toast_text_, sizeof(action_toast_text_),
                                      "+1 %s", STAT_NAMES[idx]);
                        action_toast_color_ = COLOR_CORE_INNER;
                        action_toast_t_ = 0.9f;
                        game::save_game("save.sav", world_.character(),
                                        world_.room_index(),
                                        world_.rooms_visited());
                    } else {
                        std::snprintf(action_toast_text_, sizeof(action_toast_text_),
                                      "%s IS MAXED OR NO POINTS", STAT_NAMES[idx]);
                        action_toast_color_ = COLOR_ENEMY_EDGE;
                        action_toast_t_ = 0.9f;
                    }
                }
            }

            // --- Skill tree: arrows to navigate, Enter to learn ---
            if (skill_tree_open_) {
                if (in.key_pressed[static_cast<int>(platform::Key::Up)]) {
                    if (skill_cursor_row_ > 0) --skill_cursor_row_;
                }
                if (in.key_pressed[static_cast<int>(platform::Key::Down)]) {
                    if (skill_cursor_row_ < 4) ++skill_cursor_row_;
                }
                if (in.key_pressed[static_cast<int>(platform::Key::Left)]) {
                    if (skill_cursor_branch_ > 0) --skill_cursor_branch_;
                }
                if (in.key_pressed[static_cast<int>(platform::Key::Right)]) {
                    if (skill_cursor_branch_ < 2) ++skill_cursor_branch_;
                }
                if (in.key_pressed[static_cast<int>(platform::Key::Enter)]) {
                    const int sid = skill_cursor_branch_ * 5 + skill_cursor_row_;
                    const game::SkillNode& node = game::skill_node(game::SkillId(sid));
                    if (world_.character().learn_skill(game::SkillId(sid))) {
                        std::snprintf(action_toast_text_, sizeof(action_toast_text_),
                                      "LEARNED: %s", node.name);
                        action_toast_color_ = COLOR_CORE_INNER;
                        action_toast_t_ = 1.4f;
                        game::save_game("save.sav", world_.character(),
                                        world_.room_index(),
                                        world_.rooms_visited());
                    } else {
                        std::snprintf(action_toast_text_, sizeof(action_toast_text_),
                                      "CANNOT LEARN: %s", node.name);
                        action_toast_color_ = COLOR_ENEMY_EDGE;
                        action_toast_t_ = 1.0f;
                    }
                }
            }

            // --- Log archive: up/down ---
            if (log_archive_open_) {
                if (in.key_pressed[static_cast<int>(platform::Key::Up)]) {
                    if (archive_cursor_ > 0) --archive_cursor_;
                }
                if (in.key_pressed[static_cast<int>(platform::Key::Down)]) {
                    if (archive_cursor_ < game::LOG_COUNT - 1) ++archive_cursor_;
                }
            }

            if (world_map_open_) {
                if (in.key_pressed[static_cast<int>(platform::Key::Left)]) {
                    if (map_cursor_ > 0) --map_cursor_;
                }
                if (in.key_pressed[static_cast<int>(platform::Key::Right)]) {
                    if (map_cursor_ < game::ROOM_COUNT) ++map_cursor_;
                }
                if (in.key_pressed[static_cast<int>(platform::Key::Enter)]) {
                    if (map_cursor_ < game::ROOM_COUNT &&
                        world_.is_room_visited(map_cursor_)) {
                        world_.travel_to_room(map_cursor_);
                        world_map_open_ = false;
                    }
                }
            }

            break;
        }

        if (world_.pending_log_trigger() >= 0) {
            logs_.begin(world_.pending_log_trigger());
            world_.clear_pending_log_trigger();
        }

        world_.step(dt, in);
        check_quests();

        if (world_.player_dead()) {
            screen_ = game::Screen::GameOver;
            break;
        }
        if (world_.warden_defeated()) {
            screen_ = game::Screen::Victory;
            victory_time_ = 0.0f;
            break;
        }

        if (world_.room_index() != last_room_shown_ ||
            world_.warden_active() != last_warden_state_) {
            last_room_shown_   = world_.room_index();
            last_warden_state_ = world_.warden_active();
            room_intro_timer_  = ROOM_INTRO_TOTAL;
        }

        if (room_intro_timer_ > 0.0f) {
            room_intro_timer_ -= dt;
            if (room_intro_timer_ < 0.0f) room_intro_timer_ = 0.0f;
        }
        break;
    }

    case game::Screen::GameOver: {
        if (any_pressed) {
            screen_ = game::Screen::Title;
            title_time_ = 0.0f;
        }
        break;
    }

    case game::Screen::Victory: {
        victory_time_ += dt;
        if (any_pressed && victory_time_ > 1.0f) {
            screen_ = game::Screen::Title;
            title_time_ = 0.0f;
        }
        break;
    }

    default:
        break;
    }
}

void App::render() {
    renderer_.begin_frame(platform::window_width(), platform::window_height());

    switch (screen_) {
    case game::Screen::Intro:
        draw_intro(renderer_);
        break;
    case game::Screen::Title:
        draw_title(renderer_);
        break;
    case game::Screen::Playing:
        world_.draw(renderer_);
        if (character_sheet_open_) draw_character_sheet(renderer_);
        if (skill_tree_open_)      draw_skill_tree(renderer_);
        if (equipment_panel_open_) draw_equipment_panel(renderer_);
        if (log_archive_open_)     draw_log_archive(renderer_);
        if (quest_log_open_)       draw_quest_log(renderer_);
        if (world_map_open_)       draw_world_map(renderer_);
        if (logs_.playing())       draw_log_overlay(renderer_);
        if (paused_)               draw_pause_menu(renderer_);
        if (action_toast_t_ > 0.0f) draw_action_toast(renderer_);
        if (room_intro_timer_ > 0.0f) draw_room_intro(renderer_);
        break;
    case game::Screen::GameOver:
        draw_game_over(renderer_);
        break;
    case game::Screen::Victory:
        draw_victory(renderer_);
        break;
    }

    renderer_.end_frame();
}

void App::draw_title(render::Renderer& r) const {
    render::draw_menu_background(r, f32(platform::time_seconds()), COLOR_CORE_INNER, 220);

    const f32 pulse = 0.5f + 0.5f * std::sin(platform::time_seconds() * 2.0f);
    r.draw_text_centered({VIRTUAL_W*0.5f, VIRTUAL_H*0.32f},
                         "INCIDENCE", 12.0f, COLOR_CORE_INNER);
    r.draw_text_centered({VIRTUAL_W*0.5f, VIRTUAL_H*0.42f},
                         "REFLECT OR DIE", 3.5f,
                         with_alpha(COLOR_FACET_EDGE, u8(180 + 60 * pulse)));

    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_circle({VIRTUAL_W*0.5f, VIRTUAL_H*0.32f}, 400.0f,
                  with_alpha(COLOR_CORE_EDGE, 20), 48);
    r.set_layer(render::Renderer::Layer::Alpha);

    if (title_time_ > 0.4f) {
        r.draw_text_centered({VIRTUAL_W*0.5f, VIRTUAL_H*0.66f},
                             "PRESS ANY KEY TO BEGIN", 3.0f,
                             with_alpha(COLOR_CORE_EDGE, u8(pulse * 255.0f)));
    }

    r.draw_text_centered({VIRTUAL_W*0.5f, VIRTUAL_H - 50.0f},
                         "TAB: STATS   K: SKILLS   I: GEAR   L: LOGS   J: QUESTS   M: MAP",
                         2.2f, with_alpha(COLOR_FACET_EDGE, 180));
}

void App::draw_game_over(render::Renderer& r) const {
    render::draw_menu_background(r, f32(platform::time_seconds()), COLOR_ENEMY_EDGE, 140);
    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({0,0}, {VIRTUAL_W,0}, {VIRTUAL_W,VIRTUAL_H}, {0,VIRTUAL_H}, rgba(40,0,0,120));
    r.draw_text_centered({VIRTUAL_W*0.5f, VIRTUAL_H*0.42f},
                         "DEFLECTOR OFFLINE", 8.0f, COLOR_ENEMY_CORE);
    r.draw_text_centered({VIRTUAL_W*0.5f, VIRTUAL_H*0.55f},
                         "PRESS ANY KEY", 3.0f, COLOR_ENEMY_EDGE);
}

void App::draw_victory(render::Renderer& r) const {
    render::draw_menu_background(r, f32(platform::time_seconds()), COLOR_CORE_INNER, 180);
    r.set_layer(render::Renderer::Layer::Alpha);
    const f32 t = clamp(victory_time_ / 2.0f, 0.0f, 1.0f);
    r.draw_quad({0,0}, {VIRTUAL_W,0}, {VIRTUAL_W,VIRTUAL_H}, {0,VIRTUAL_H},
                rgba(10, 20, 10, u8(100 + 60 * t)));
    r.draw_text_centered({VIRTUAL_W*0.5f, VIRTUAL_H*0.38f},
                         "FOLD PRESERVED", 10.0f,
                         with_alpha(COLOR_CORE_INNER, u8(t * 255.0f)));
    r.draw_text_centered({VIRTUAL_W*0.5f, VIRTUAL_H*0.50f},
                         "KITE-9 ENDURES", 4.0f,
                         with_alpha(COLOR_FACET_EDGE, u8(t * 220.0f)));
    if (victory_time_ > 1.0f) {
        const f32 pulse = 0.5f + 0.5f * std::sin(platform::time_seconds() * 3.0f);
        r.draw_text_centered({VIRTUAL_W*0.5f, VIRTUAL_H*0.62f},
                             "PRESS ANY KEY", 3.0f,
                             with_alpha(COLOR_CORE_EDGE, u8(pulse * 255.0f)));
    }
}

void App::draw_room_intro(render::Renderer& r) const {
    const f32 elapsed = ROOM_INTRO_TOTAL - room_intro_timer_;

    // Three-phase alpha: fade in / hold / fade out
    f32 alpha = 1.0f;
    if (elapsed < ROOM_INTRO_FADE_IN) {
        alpha = elapsed / ROOM_INTRO_FADE_IN;
    } else if (elapsed > ROOM_INTRO_FADE_IN + ROOM_INTRO_HOLD) {
        const f32 out = elapsed - ROOM_INTRO_FADE_IN - ROOM_INTRO_HOLD;
        alpha = 1.0f - out / ROOM_INTRO_FADE_OUT;
    }
    alpha = clamp(alpha, 0.0f, 1.0f);

    const bool boss = world_.warden_active();
    const int idx   = boss ? 9 : world_.room_index();
    if (idx < 0 || idx > 9) return;

    const Color accent    = boss ? COLOR_ENEMY_CORE : room_arc_color(idx);
    const Color accent_lo = with_alpha(accent, u8(60 * alpha));
    const Color accent_hi = with_alpha(accent, u8(255 * alpha));

    // Entrance slide (top -> down) and slight scale via offset
    const f32 enter = clamp(elapsed / 0.35f, 0.0f, 1.0f);
    const f32 ease  = 1.0f - (1.0f - enter) * (1.0f - enter); // ease-out quad

    // Full-screen dim (fades faster than the card appears)
    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({0, 0}, {VIRTUAL_W, 0}, {VIRTUAL_W, VIRTUAL_H}, {0, VIRTUAL_H},
                rgba(2, 6, 12, u8(alpha * 160.0f)));

    // Card geometry
    const f32 card_w = 1100.0f;
    const f32 card_h = 300.0f;
    const f32 card_cx = VIRTUAL_W * 0.5f;
    const f32 card_cy = VIRTUAL_H * 0.5f + (1.0f - ease) * 60.0f;
    const f32 card_x0 = card_cx - card_w * 0.5f;
    const f32 card_y0 = card_cy - card_h * 0.5f;
    const f32 card_x1 = card_x0 + card_w;
    const f32 card_y1 = card_y0 + card_h;

    // Card fill
    r.draw_quad({card_x0, card_y0}, {card_x1, card_y0},
                {card_x1, card_y1}, {card_x0, card_y1},
                rgba(8, 22, 32, u8(alpha * 240.0f)));

    // Outer border (animated corner notches)
    const f32 n = 24.0f;
    const Color border = with_alpha(accent, u8(200 * alpha));
    r.draw_line({card_x0 + n, card_y0}, {card_x1 - n, card_y0}, 2.0f, border);
    r.draw_line({card_x1, card_y0 + n}, {card_x1, card_y1 - n}, 2.0f, border);
    r.draw_line({card_x1 - n, card_y1}, {card_x0 + n, card_y1}, 2.0f, border);
    r.draw_line({card_x0, card_y1 - n}, {card_x0, card_y0 + n}, 2.0f, border);
    // Corner notches
    r.draw_line({card_x0, card_y0}, {card_x0 + n, card_y0}, 3.0f, accent_hi);
    r.draw_line({card_x0, card_y0}, {card_x0, card_y0 + n}, 3.0f, accent_hi);
    r.draw_line({card_x1, card_y0}, {card_x1 - n, card_y0}, 3.0f, accent_hi);
    r.draw_line({card_x1, card_y0}, {card_x1, card_y0 + n}, 3.0f, accent_hi);
    r.draw_line({card_x0, card_y1}, {card_x0 + n, card_y1}, 3.0f, accent_hi);
    r.draw_line({card_x0, card_y1}, {card_x0, card_y1 - n}, 3.0f, accent_hi);
    r.draw_line({card_x1, card_y1}, {card_x1 - n, card_y1}, 3.0f, accent_hi);
    r.draw_line({card_x1, card_y1}, {card_x1, card_y1 - n}, 3.0f, accent_hi);

    // Left accent strip
    r.draw_quad({card_x0 + 20.0f, card_y0 + 20.0f},
                {card_x0 + 26.0f, card_y0 + 20.0f},
                {card_x0 + 26.0f, card_y1 - 20.0f},
                {card_x0 + 20.0f, card_y1 - 20.0f},
                with_alpha(accent, u8(200 * alpha)));

    // Top-left header row
    char buf[64];
    std::snprintf(buf, sizeof(buf), "ROOM %02d / %s", idx,
                  boss ? "WARDEN" : "SECTOR");
    r.draw_text({card_x0 + 60.0f, card_y0 + 34.0f}, buf, 2.2f, accent_lo);
    r.draw_text({card_x0 + 60.0f, card_y0 + 62.0f}, ROOM_NAME[idx], 5.0f,
                with_alpha(COLOR_CORE_INNER, u8(255 * alpha)));

    // Divider
    const f32 div_y = card_y0 + 138.0f;
    r.draw_line({card_x0 + 60.0f, div_y}, {card_x1 - 60.0f, div_y},
                1.0f, with_alpha(accent, u8(120 * alpha)));

    // Blurb
    r.draw_text({card_x0 + 60.0f, card_y0 + 156.0f}, ROOM_BLURB[idx], 2.0f,
                with_alpha(COLOR_FACET_EDGE, u8(220 * alpha)));

    // Bottom-right status row
    char tail[64];
    if (boss) {
        std::snprintf(tail, sizeof(tail), "BOSS ENCOUNTER");
    } else if (world_.is_room_cleared(idx)) {
        std::snprintf(tail, sizeof(tail), "SECTOR CLEAR");
    } else {
        std::snprintf(tail, sizeof(tail), "HOSTILES ACTIVE");
    }
    const f32 tail_w = r.text_width(tail, 1.8f);
    r.draw_text({card_x1 - 60.0f - tail_w, card_y1 - 36.0f}, tail, 1.8f, accent_lo);

    // Under-card accent bar with a pulsing sweep
    const f32 pulse = 0.5f + 0.5f * std::sin(elapsed * 6.0f);
    const f32 sweep = 180.0f + pulse * 120.0f;
    const f32 sweep_x = card_cx - sweep * 0.5f;
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_quad({sweep_x, card_y1 + 6.0f},
                {sweep_x + sweep, card_y1 + 6.0f},
                {sweep_x + sweep, card_y1 + 9.0f},
                {sweep_x, card_y1 + 9.0f},
                with_alpha(accent, u8(220 * alpha)));
    r.set_layer(render::Renderer::Layer::Alpha);
}

void App::draw_pause_menu(render::Renderer& r) const {
    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({0,0}, {VIRTUAL_W,0}, {VIRTUAL_W,VIRTUAL_H}, {0,VIRTUAL_H},
                rgba(0, 0, 0, 210));

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 cy = VIRTUAL_H * 0.5f;

    r.draw_text_centered({cx, cy - 220.0f}, "PAUSED", 9.0f, COLOR_CORE_INNER);

    static const char* OPTIONS[3] = { "RESUME", "RESTART", "QUIT TO DESKTOP" };
    for (int i = 0; i < 3; ++i) {
        const f32 y = cy - 40.0f + f32(i) * 70.0f;
        const bool sel = (i == pause_cursor_);
        if (sel) {
            const f32 pulse = 0.5f + 0.5f * std::sin(platform::time_seconds() * 5.0f);
            r.set_layer(render::Renderer::Layer::Additive);
            r.draw_circle({cx, y + 20.0f}, 220.0f,
                          with_alpha(COLOR_CORE_EDGE, u8(20 + 20 * pulse)), 40);
            r.set_layer(render::Renderer::Layer::Alpha);
        }
        const Color c = sel ? COLOR_CORE_INNER : with_alpha(COLOR_FACET_EDGE, 180);
        r.draw_text_centered({cx, y}, OPTIONS[i], 4.0f, c);
    }

    r.draw_text_centered({cx, VIRTUAL_H - 120.0f},
                         "UP/DOWN: SELECT   ENTER: CONFIRM   ESC: RESUME",
                         2.2f, with_alpha(COLOR_FACET_EDGE, 200));
}

void App::draw_action_toast(render::Renderer& r) const {
    if (action_toast_t_ <= 0.0f) return;

    constexpr f32 MAXT = 1.4f;
    const f32 t = clamp(action_toast_t_ / MAXT, 0.0f, 1.0f);
    f32 alpha = 1.0f;
    if (t > 0.6f) alpha = (1.0f - t) / 0.4f;
    alpha = clamp(alpha, 0.0f, 1.0f);

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 cy = VIRTUAL_H * 0.78f;
    const f32 w  = r.text_width(action_toast_text_, 3.0f);
    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({cx - w * 0.5f - 30.0f, cy - 20.0f},
                {cx + w * 0.5f + 30.0f, cy - 20.0f},
                {cx + w * 0.5f + 30.0f, cy + 55.0f},
                {cx - w * 0.5f - 30.0f, cy + 55.0f},
                rgba(0, 0, 0, u8(180 * alpha)));
    r.draw_text_centered({cx, cy}, action_toast_text_, 3.0f,
                         with_alpha(action_toast_color_, u8(255 * alpha)));
}

} // namespace inc
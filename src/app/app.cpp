#include "app/app.hpp"
#include "audio/audio.hpp"
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

        if (any_panel) {
            if (in.key_pressed[static_cast<int>(platform::Key::Escape)]) {
                character_sheet_open_ = false;
                skill_tree_open_      = false;
                equipment_panel_open_ = false;
                log_archive_open_     = false;
                quest_log_open_       = false;
                world_map_open_       = false;
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
                        world_.is_room_cleared(map_cursor_)) {
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
    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({0,0}, {VIRTUAL_W,0}, {VIRTUAL_W,VIRTUAL_H}, {0,VIRTUAL_H}, rgba(8,8,16,255));

    for (int i = 0; i < STAR_COUNT; ++i) {
        const f32 tw = 0.6f + 0.4f * std::sin(platform::time_seconds() * 1.5f + f32(i));
        r.draw_circle({STARS[i].x, STARS[i].y}, STARS[i].r,
                      with_alpha(COLOR_CORE_INNER, u8(120.0f * tw)), 8);
    }

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
    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({0,0}, {VIRTUAL_W,0}, {VIRTUAL_W,VIRTUAL_H}, {0,VIRTUAL_H}, rgba(40,0,0,200));
    r.draw_text_centered({VIRTUAL_W*0.5f, VIRTUAL_H*0.42f},
                         "DEFLECTOR OFFLINE", 8.0f, COLOR_ENEMY_CORE);
    r.draw_text_centered({VIRTUAL_W*0.5f, VIRTUAL_H*0.55f},
                         "PRESS ANY KEY", 3.0f, COLOR_ENEMY_EDGE);
}

void App::draw_victory(render::Renderer& r) const {
    r.set_layer(render::Renderer::Layer::Alpha);
    const f32 t = clamp(victory_time_ / 2.0f, 0.0f, 1.0f);
    r.draw_quad({0,0}, {VIRTUAL_W,0}, {VIRTUAL_W,VIRTUAL_H}, {0,VIRTUAL_H},
                rgba(10, 20, 10, u8(180 + 75 * t)));
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
    f32 alpha = 1.0f;
    if (elapsed < ROOM_INTRO_FADE_IN) {
        alpha = elapsed / ROOM_INTRO_FADE_IN;
    } else if (elapsed > ROOM_INTRO_FADE_IN + ROOM_INTRO_HOLD) {
        const f32 out = elapsed - ROOM_INTRO_FADE_IN - ROOM_INTRO_HOLD;
        alpha = 1.0f - out / ROOM_INTRO_FADE_OUT;
    }
    alpha = clamp(alpha, 0.0f, 1.0f);

    const bool boss = world_.warden_active();
    const int idx = boss ? 9 : world_.room_index();
    if (idx < 0 || idx > 9) return;

    const Color accent = boss ? COLOR_ENEMY_CORE : room_arc_color(idx);

    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({0, 0}, {VIRTUAL_W, 0}, {VIRTUAL_W, 180.0f}, {0, 180.0f},
                rgba(0, 0, 0, u8(alpha * 180.0f)));

    char buf[64];
    std::snprintf(buf, sizeof(buf), "ROOM %d", idx);
    r.draw_text({60.0f, 50.0f}, buf, 3.0f,
                with_alpha(accent, u8(alpha * 200.0f)));
    r.draw_text({60.0f, 95.0f}, ROOM_NAME[idx], 6.0f,
                with_alpha(COLOR_CORE_INNER, u8(alpha * 255.0f)));
    r.draw_text({60.0f, 145.0f}, ROOM_BLURB[idx], 2.0f,
                with_alpha(COLOR_FACET_EDGE, u8(alpha * 200.0f)));

    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_line({0.0f, 180.0f}, {VIRTUAL_W, 180.0f}, 2.0f,
                with_alpha(accent, u8(alpha * 160.0f)));
    r.set_layer(render::Renderer::Layer::Alpha);
}

} // namespace inc
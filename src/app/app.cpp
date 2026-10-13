#include "app/app.hpp"
#include "audio/audio.hpp"
#include "render/icons.hpp"
#include "app/banner.hpp"
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
    screen_        = game::Screen::MainMenu;
    intro_time_    = 0.0f;
    main_menu_time_ = 0.0f;
    menu_cursor_    = 0;
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
    case game::Screen::MainMenu: {
        main_menu_time_ += dt;
        if (in.key_pressed[static_cast<int>(platform::Key::Up)]) {
            if (menu_cursor_ > 0) --menu_cursor_; else menu_cursor_ = 2;
        }
        if (in.key_pressed[static_cast<int>(platform::Key::Down)]) {
            if (menu_cursor_ < 2) ++menu_cursor_; else menu_cursor_ = 0;
        }
        if (in.key_pressed[static_cast<int>(platform::Key::Escape)]) {
            running_ = false;
            break;
        }
        const bool activate =
            in.key_pressed[static_cast<int>(platform::Key::Enter)] ||
            in.key_pressed[static_cast<int>(platform::Key::Space)];
        if (activate) {
            if (menu_cursor_ == 0) {
                profile_input_len_ = 0;
                profile_input_name_[0] = 0;
                screen_ = game::Screen::ProfileCreate;
                main_menu_time_ = 0.0f;
                menu_cursor_ = 0;
            } else if (menu_cursor_ == 1) {
                profile_list_count_ = game::profiles_list(profile_list_, game::PROFILE_MAX);
                profile_cursor_ = 0;
                screen_ = game::Screen::ProfileSelect;
            } else {
                running_ = false;
            }
        }
        break;
    }

    case game::Screen::ProfileCreate: {
        static const platform::Key s_letters[26] = {
            platform::Key::A, platform::Key::B, platform::Key::C, platform::Key::D,
            platform::Key::E, platform::Key::F, platform::Key::G, platform::Key::H,
            platform::Key::I, platform::Key::J, platform::Key::K, platform::Key::L,
            platform::Key::M, platform::Key::N, platform::Key::O, platform::Key::P,
            platform::Key::Q, platform::Key::R, platform::Key::S, platform::Key::T,
            platform::Key::U, platform::Key::V, platform::Key::W, platform::Key::X,
            platform::Key::Y, platform::Key::Z
        };
        static const platform::Key s_digits[10] = {
            platform::Key::Num0, platform::Key::Num1, platform::Key::Num2,
            platform::Key::Num3, platform::Key::Num4, platform::Key::Num5,
            platform::Key::Num6, platform::Key::Num7, platform::Key::Num8,
            platform::Key::Num9
        };
        if (in.key_pressed[static_cast<int>(platform::Key::Escape)]) {
            screen_ = game::Screen::MainMenu;
            break;
        }
        if (profile_input_len_ < game::PROFILE_MAX_NAME) {
            for (int i = 0; i < 26; ++i) {
                if (in.key_pressed[static_cast<int>(s_letters[i])]) {
                    profile_input_name_[profile_input_len_++] = char('A' + i);
                    profile_input_name_[profile_input_len_] = 0;
                }
            }
            for (int i = 0; i < 10; ++i) {
                if (in.key_pressed[static_cast<int>(s_digits[i])]) {
                    profile_input_name_[profile_input_len_++] = char('0' + i);
                    profile_input_name_[profile_input_len_] = 0;
                }
            }
        }
        if (in.key_pressed[static_cast<int>(platform::Key::Enter)]) {
            if (game::profile_name_valid(profile_input_name_)) {
                if (game::profile_create(profile_input_name_)) {
                    game::profile_set_active(profile_input_name_);
                    world_.reset();
                    logs_   = game::LogState{};
                    quests_ = game::QuestState{};
                    screen_ = game::Screen::Intro;
                    intro_time_ = 0.0f;
                }
            }
        }
        break;
    }

    case game::Screen::ProfileSelect: {
        if (in.key_pressed[static_cast<int>(platform::Key::Escape)]) {
            screen_ = game::Screen::MainMenu;
            break;
        }
        if (profile_list_count_ > 0) {
            if (in.key_pressed[static_cast<int>(platform::Key::Up)]) {
                if (profile_cursor_ > 0) --profile_cursor_;
                else profile_cursor_ = profile_list_count_ - 1;
            }
            if (in.key_pressed[static_cast<int>(platform::Key::Down)]) {
                if (profile_cursor_ < profile_list_count_ - 1) ++profile_cursor_;
                else profile_cursor_ = 0;
            }
            if (in.key_pressed[static_cast<int>(platform::Key::D)]) {
                if (game::profile_delete(profile_list_[profile_cursor_].name)) {
                    profile_list_count_ = game::profiles_list(profile_list_, game::PROFILE_MAX);
                    if (profile_cursor_ >= profile_list_count_) profile_cursor_ = profile_list_count_ - 1;
                    if (profile_cursor_ < 0) profile_cursor_ = 0;
                }
            }
            if (in.key_pressed[static_cast<int>(platform::Key::Enter)]) {
                game::profile_set_active(profile_list_[profile_cursor_].name);
                game::SaveData sd;
                if (game::profile_load_active(sd)) {
                    world_.character() = sd.character;
                    world_.jump_to_room(sd.current_room);
                    world_.set_rooms_visited(sd.rooms_visited);
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
                } else {
                    world_.reset();
                    logs_   = game::LogState{};
                    quests_ = game::QuestState{};
                    screen_ = game::Screen::Intro;
                    intro_time_ = 0.0f;
                }
            }
        }
        break;
    }
    case game::Screen::Intro: {
        intro_time_ += dt;
        if (any_pressed) { screen_ = game::Screen::Title; title_time_ = 0.0f; break; }
        // Matches INTRO_CARDS durations in app_intro.cpp
        const f32 total = 22.5f;
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

        if (room_intro_timer_ <= 0.0f) world_.step(dt, in);
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
    case game::Screen::MainMenu:
        draw_main_menu(renderer_);
        break;
    case game::Screen::ProfileCreate:
        draw_profile_create(renderer_);
        break;
    case game::Screen::ProfileSelect:
        draw_profile_select(renderer_);
        break;
    case game::Screen::Intro:
        draw_intro(renderer_);
        break;
    case game::Screen::Title:
        draw_title(renderer_);
        break;
    case game::Screen::Playing:
        world_.draw(renderer_);
        draw_gameplay_hud(renderer_);
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
    f32 alpha = 1.0f;
    if (elapsed < ROOM_INTRO_FADE_IN) alpha = elapsed / ROOM_INTRO_FADE_IN;
    else if (elapsed > ROOM_INTRO_FADE_IN + ROOM_INTRO_HOLD) {
        const f32 out = elapsed - ROOM_INTRO_FADE_IN - ROOM_INTRO_HOLD;
        alpha = 1.0f - out / ROOM_INTRO_FADE_OUT;
    }
    alpha = clamp(alpha, 0.0f, 1.0f);

    const bool boss = world_.warden_active();
    const int idx   = boss ? 9 : world_.room_index();
    if (idx < 0 || idx > 9) return;
    const bool cleared = world_.is_room_cleared(idx) && !boss;

    const Color accent    = boss ? COLOR_ENEMY_CORE : (cleared ? COLOR_CORE_EDGE : room_arc_color(idx));
    const Color accent_hi = with_alpha(accent, u8(255 * alpha));

    const f32 enter = clamp(elapsed / 0.40f, 0.0f, 1.0f);
    const f32 ease  = 1.0f - (1.0f - enter) * (1.0f - enter);
    const f32 exit_t = clamp((elapsed - (ROOM_INTRO_FADE_IN + ROOM_INTRO_HOLD)) / ROOM_INTRO_FADE_OUT, 0.0f, 1.0f);
    const f32 slide_y = (1.0f - ease) * 90.0f - exit_t * exit_t * 60.0f;
    const f32 card_scale = 0.94f + 0.06f * ease;

    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({0,0}, {VIRTUAL_W,0}, {VIRTUAL_W,VIRTUAL_H}, {0,VIRTUAL_H},
                rgba(2, 6, 12, u8(alpha * (boss ? 210 : 160))));
    if (boss) {
        const f32 bp = 0.5f + 0.5f * std::sin(elapsed * 5.5f);
        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_quad({0,0}, {VIRTUAL_W,0}, {VIRTUAL_W,VIRTUAL_H}, {0,VIRTUAL_H},
                    with_alpha(COLOR_ENEMY_FILL, u8(alpha * (20 + 20 * bp))));
        r.set_layer(render::Renderer::Layer::Alpha);
    }

    banner_ghost_number(r, VIRTUAL_W * 0.5f, VIRTUAL_H * 0.5f, idx, alpha, accent);

    const f32 cw = 1150.0f * card_scale;
    const f32 ch = 340.0f * card_scale;
    const f32 ccx = VIRTUAL_W * 0.5f;
    const f32 ccy = VIRTUAL_H * 0.5f + slide_y;
    const f32 x0 = ccx - cw * 0.5f;
    const f32 y0 = ccy - ch * 0.5f;
    const f32 x1 = x0 + cw;
    const f32 y1 = y0 + ch;

    r.draw_quad({x0 + 24, y0 + 24}, {x1 + 24, y0 + 24},
                {x1 + 24, y1 + 24}, {x0 + 24, y1 + 24},
                rgba(0, 0, 0, u8(alpha * 90)));
    r.draw_quad({x0 + 12, y0 + 12}, {x1 + 12, y0 + 12},
                {x1 + 12, y1 + 12}, {x0 + 12, y1 + 12},
                rgba(0, 0, 0, u8(alpha * 170)));
    r.draw_quad({x0, y0}, {x1, y0}, {x1, y1}, {x0, y1},
                rgba(6, 18, 28, u8(alpha * 245)));
    r.draw_quad({x0, y0}, {x1, y0}, {x1, y0 + ch * 0.30f}, {x0, y0 + ch * 0.30f},
                rgba(10, 26, 38, u8(alpha * 210)));

    banner_grid_pattern(r, x0, y0, x1, y1, alpha, accent);

    for (int i = 0; i < 22; ++i) {
        const f32 hx  = x0 + f32(i) * 65.0f - ch;
        const f32 hx2 = hx + ch;
        r.draw_line({hx, y1}, {hx2, y0}, 1.0f, with_alpha(accent, u8(alpha * 9)));
    }

    if (boss) {
        banner_warning_stripes(r, x0, y0, x1, y0 + 8.0f, alpha, COLOR_ENEMY_CORE, COLOR_VOID);
        banner_warning_stripes(r, x0, y1 - 8.0f, x1, y1, alpha, COLOR_ENEMY_CORE, COLOR_VOID);
    }

    const f32 n = 30.0f;
    const Color border = with_alpha(accent, u8(180 * alpha));
    r.draw_line({x0 + n, y0}, {x1 - n, y0}, 2.0f, border);
    r.draw_line({x1, y0 + n}, {x1, y1 - n}, 2.0f, border);
    r.draw_line({x1 - n, y1}, {x0 + n, y1}, 2.0f, border);
    r.draw_line({x0, y1 - n}, {x0, y0 + n}, 2.0f, border);
    r.draw_line({x0, y0}, {x0 + n, y0}, 3.0f, accent_hi);
    r.draw_line({x0, y0}, {x0, y0 + n}, 3.0f, accent_hi);
    r.draw_line({x1, y0}, {x1 - n, y0}, 3.0f, accent_hi);
    r.draw_line({x1, y0}, {x1, y0 + n}, 3.0f, accent_hi);
    r.draw_line({x0, y1}, {x0 + n, y1}, 3.0f, accent_hi);
    r.draw_line({x0, y1}, {x0, y1 - n}, 3.0f, accent_hi);
    r.draw_line({x1, y1}, {x1 - n, y1}, 3.0f, accent_hi);
    r.draw_line({x1, y1}, {x1, y1 - n}, 3.0f, accent_hi);
    r.draw_circle({x0 + 14, y0 + 14}, 3.0f, accent_hi, 10);
    r.draw_circle({x1 - 14, y0 + 14}, 3.0f, accent_hi, 10);
    r.draw_circle({x0 + 14, y1 - 14}, 3.0f, accent_hi, 10);
    r.draw_circle({x1 - 14, y1 - 14}, 3.0f, accent_hi, 10);

    banner_corner_pulses(r, elapsed, x0, y0, x1, y1, alpha, accent);
    banner_shine_sweep(r, elapsed, x0, y0, x1, y1, alpha, accent);
    banner_motes(r, elapsed, x0 + 6, y0 + 6, x1 - 6, y1 - 6, alpha, accent);

    r.draw_quad({x0 + 22, y0 + 22}, {x0 + 32, y0 + 22},
                {x0 + 32, y1 - 22}, {x0 + 22, y1 - 22},
                with_alpha(accent, u8(220 * alpha)));
    r.draw_quad({x0 + 38, y0 + 22}, {x0 + 41, y0 + 22},
                {x0 + 41, y1 - 22}, {x0 + 38, y1 - 22},
                with_alpha(accent, u8(140 * alpha)));

    banner_badge_orbit(r, x0 + 96.0f, y0 + 70.0f, 46.0f, elapsed, alpha, accent);
    render::icon_hex(r, {x0 + 96, y0 + 70}, 34.0f,
                     with_alpha(accent, u8(alpha * 60)),
                     with_alpha(accent, u8(alpha * 220)));
    render::icon_hex(r, {x0 + 96, y0 + 70}, 26.0f, 0,
                     with_alpha(accent, u8(alpha * 140)));
    char num[8];
    std::snprintf(num, sizeof(num), "%d", idx);
    const f32 nw = r.text_width(num, 4.4f);
    r.draw_text({x0 + 96 - nw * 0.5f, y0 + 52}, num, 4.4f,
                with_alpha(COLOR_CORE_INNER, u8(alpha * 255)));

    char hdr[64];
    std::snprintf(hdr, sizeof(hdr), "ROOM %02d / %s", idx, boss ? "WARDEN" : "SECTOR");
    r.draw_text({x0 + 156, y0 + 40}, hdr, 2.2f, with_alpha(accent, u8(alpha * 200)));

    banner_chapter_marker(r, x1 - 40, y0 + 40, idx, alpha, accent);

    r.draw_text({x0 + 156, y0 + 76}, ROOM_NAME[idx], 5.6f,
                with_alpha(COLOR_CORE_INNER, u8(alpha * 255)));
    banner_title_scan(r, elapsed, x0 + 156, x0 + 156 + r.text_width(ROOM_NAME[idx], 5.6f),
                      y0 + 128, alpha, accent);

    const f32 div_y = y0 + 168;
    const f32 div_age = clamp((elapsed - 0.40f) / 0.35f, 0.0f, 1.0f);
    const f32 dhw = (cw - 200.0f) * 0.5f * div_age;
    r.draw_line({ccx - dhw, div_y}, {ccx + dhw, div_y}, 1.0f,
                with_alpha(accent, u8(150 * alpha)));
    if (div_age > 0.5f) {
        const f32 d = (div_age - 0.5f) * 2.0f;
        const f32 ds = 6.0f * d;
        r.draw_quad({ccx, div_y - ds}, {ccx + ds, div_y},
                    {ccx, div_y + ds}, {ccx - ds, div_y},
                    with_alpha(accent, u8(200 * alpha * d)));
        r.draw_line({ccx - dhw, div_y - 4}, {ccx - dhw, div_y + 4}, 2.0f,
                    with_alpha(accent, u8(180 * alpha * d)));
        r.draw_line({ccx + dhw, div_y - 4}, {ccx + dhw, div_y + 4}, 2.0f,
                    with_alpha(accent, u8(180 * alpha * d)));
    }

    const f32 blurb_age = clamp((elapsed - 0.55f) / 0.35f, 0.0f, 1.0f);
    if (blurb_age > 0.0f) {
        render::icon_chevron(r, {x0 + 96, y0 + 200}, 20.0f, 0.0f,
                             with_alpha(accent, u8(alpha * 220 * blurb_age)));
        {
                char blurb_lines[3][128];
                const int nlines = banner_wrap_text(ROOM_BLURB[idx], 72, blurb_lines, 3);
                for (int li = 0; li < nlines; ++li) {
                    r.draw_text({x0 + 128, y0 + 190 + f32(li) * 32.0f},
                                blurb_lines[li], 2.2f,
                                with_alpha(COLOR_FACET_EDGE, u8(alpha * 220 * blurb_age)));
                }
            }
    }

    int lancers = 0, bankers = 0, leads = 0;
    {
        const game::Room& rm = game::room_at(idx < 0 ? 0 : idx);
        for (int i = 0; i < rm.enemy_count; ++i) {
            switch (rm.enemies[i].type) {
                case game::EnemyType::Lancer: ++lancers; break;
                case game::EnemyType::Banker: ++bankers; break;
                case game::EnemyType::Lead:   ++leads;   break;
            }
        }
    }
    if (!cleared && !boss) {
        banner_threat_row(r, x0 + 150, y0 + 264, lancers, bankers, leads, alpha, elapsed);
        char tbuf[32];
        std::snprintf(tbuf, sizeof(tbuf), "%d HOSTILES", lancers + bankers + leads);
        r.draw_text({x0 + 150 + f32(lancers + bankers + leads) * 42.0f + 16, y0 + 256},
                    tbuf, 2.0f, with_alpha(COLOR_ENEMY_EDGE, u8(alpha * 220)));
    }

    // floor plan placeholder removed - was colliding with status pill

    banner_top_chevrons(r, elapsed, x0 + 60, y0 - 18, x1 - 60, alpha, accent);
    banner_status_ticks(r, x0 + 60, y1 + 20, x1 - 60,
                        cleared ? 10 : (idx + 1), 10, alpha, accent);

    char tail[32];
    if (boss)           std::snprintf(tail, sizeof(tail), "BOSS ENCOUNTER");
    else if (cleared)   std::snprintf(tail, sizeof(tail), "SECTOR CLEAR");
    else                std::snprintf(tail, sizeof(tail), "HOSTILES ACTIVE");
    const f32 tw = r.text_width(tail, 1.9f);
    const f32 px = x1 - 40 - tw - 18;
    const f32 py = y1 - 56;
    r.draw_quad({px, py}, {px + tw + 36, py},
                {px + tw + 36, py + 28}, {px, py + 28},
                with_alpha(accent, u8(alpha * 40)));
    r.draw_line({px, py}, {px + tw + 36, py}, 1.5f, with_alpha(accent, u8(200 * alpha)));
    r.draw_line({px, py + 28}, {px + tw + 36, py + 28}, 1.5f, with_alpha(accent, u8(200 * alpha)));
    r.draw_text({px + 18, py + 8}, tail, 1.9f, with_alpha(accent, u8(220 * alpha)));

    banner_scanline_wipe(r, elapsed, ROOM_INTRO_TOTAL, y0, y1, alpha, accent);

    if (cleared && !boss) {
        const f32 stamp_a = clamp((elapsed - 0.8f) / 0.4f, 0.0f, 1.0f) * alpha;
        banner_cleared_stamp(r, ccx, VIRTUAL_H * 0.30f, stamp_a, elapsed);
    }

    const f32 pulse = 0.5f + 0.5f * std::sin(elapsed * 6.0f);
    const f32 sweep = 260.0f + pulse * 200.0f;
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_quad({ccx - sweep * 0.5f, y1 + 10}, {ccx + sweep * 0.5f, y1 + 10},
                {ccx + sweep * 0.5f, y1 + 14}, {ccx - sweep * 0.5f, y1 + 14},
                with_alpha(accent, u8(220 * alpha)));
    r.draw_quad({ccx - sweep * 0.5f - 30, y1 + 6}, {ccx - sweep * 0.5f, y1 + 6},
                {ccx - sweep * 0.5f, y1 + 18}, {ccx - sweep * 0.5f - 30, y1 + 18},
                with_alpha(accent, u8(60 * alpha)));
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
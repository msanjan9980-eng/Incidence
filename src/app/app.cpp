#include "app/app.hpp"
#include "audio/audio.hpp"
#include "game/save.hpp"

#include "sokol_app.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace inc {

static constexpr int MAX_SUBSTEPS_PER_FRAME = 8;

struct IntroCard {
    const char* small;
    const char* large;
    f32         duration;
    Color       accent;
};

static constexpr IntroCard INTRO_CARDS[] = {
    { "HALCYON STATION",                                 "YEAR 207",              3.5f, COLOR_CORE_EDGE },
    { "Two hundred years ago, the ring folded space.",   "",                      3.5f, COLOR_FACET_EDGE },
    { "It worked.",                                      "Everyone died.",        3.5f, COLOR_ENEMY_EDGE },
    { "The station AI, THEOREM, is still running.",      "PRESERVE THE RING.",    4.0f, COLOR_ENEMY_CORE },
    { "You are KITE-9, the last functioning deflector.", "YOU HAVE NO GUN.",      4.0f, COLOR_CORE_EDGE },
    { "You have a surface. The surface obeys one law.",  "REFLECT.",              4.0f, COLOR_FACET_EDGE },
};
static constexpr int INTRO_CARD_COUNT = sizeof(INTRO_CARDS) / sizeof(INTRO_CARDS[0]);

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
    if (idx < 3)  return COLOR_CORE_EDGE;
    if (idx < 5)  return COLOR_LEAD_EDGE;
    if (idx < 6)  return COLOR_BANKER_EDGE;
    if (idx < 8)  return COLOR_ENEMY_EDGE;
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
            for (int i = 0; i < INTRO_CARD_COUNT; ++i) total += INTRO_CARDS[i].duration;
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
                screen_ = game::Screen::Playing;
                last_room_shown_      = -1;
                last_warden_state_    = false;
                room_intro_timer_     = -1.0f;
                character_sheet_open_ = false;
                skill_tree_open_      = false;
                equipment_panel_open_ = false;
                log_archive_open_     = false;
                quest_log_open_       = false;
            }
            break;
        }
        case game::Screen::Playing: {
            // ---- Log overlay: captures ALL input while playing ----------
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

            // ---- Panel toggles -----------------------------------------
            if (in.key_pressed[static_cast<int>(platform::Key::Tab)]) {
                character_sheet_open_ = !character_sheet_open_;
                if (character_sheet_open_) { skill_tree_open_ = false; equipment_panel_open_ = false; log_archive_open_ = false; quest_log_open_ = false; }
            }
            if (in.key_pressed[static_cast<int>(platform::Key::K)]) {
                skill_tree_open_ = !skill_tree_open_;
                if (skill_tree_open_) { character_sheet_open_ = false; equipment_panel_open_ = false; log_archive_open_ = false; quest_log_open_ = false; }
            }
            if (in.key_pressed[static_cast<int>(platform::Key::I)]) {
                equipment_panel_open_ = !equipment_panel_open_;
                if (equipment_panel_open_) { character_sheet_open_ = false; skill_tree_open_ = false; log_archive_open_ = false; quest_log_open_ = false; }
            }
            if (in.key_pressed[static_cast<int>(platform::Key::L)]) {
                log_archive_open_ = !log_archive_open_;
                if (log_archive_open_) { character_sheet_open_ = false; skill_tree_open_ = false; equipment_panel_open_ = false; quest_log_open_ = false; }
            }
            if (in.key_pressed[static_cast<int>(platform::Key::J)]) {
                quest_log_open_ = !quest_log_open_;
                if (quest_log_open_) { character_sheet_open_ = false; skill_tree_open_ = false; equipment_panel_open_ = false; log_archive_open_ = false; }
            }

            // ---- Character sheet input ----------------------------------
            if (character_sheet_open_) {
                if (in.key_pressed[static_cast<int>(platform::Key::Escape)]) {
                    character_sheet_open_ = false;
                }
                if (in.key_pressed[static_cast<int>(platform::Key::Num1)])
                    world_.character().spend_point(0);
                if (in.key_pressed[static_cast<int>(platform::Key::Num2)])
                    world_.character().spend_point(1);
                if (in.key_pressed[static_cast<int>(platform::Key::Num3)])
                    world_.character().spend_point(2);
                if (in.key_pressed[static_cast<int>(platform::Key::Num4)])
                    world_.character().spend_point(3);
                break;
            }

            // ---- Skill tree input ---------------------------------------
            if (skill_tree_open_) {
                if (in.key_pressed[static_cast<int>(platform::Key::Escape)]) {
                    skill_tree_open_ = false;
                    break;
                }
                if (in.key_pressed[static_cast<int>(platform::Key::Left)])
                    skill_cursor_branch_ = (skill_cursor_branch_ + 2) % 3;
                if (in.key_pressed[static_cast<int>(platform::Key::Right)])
                    skill_cursor_branch_ = (skill_cursor_branch_ + 1) % 3;
                if (in.key_pressed[static_cast<int>(platform::Key::Up)])
                    skill_cursor_row_ = (skill_cursor_row_ + 4) % 5;
                if (in.key_pressed[static_cast<int>(platform::Key::Down)])
                    skill_cursor_row_ = (skill_cursor_row_ + 1) % 5;
                if (in.key_pressed[static_cast<int>(platform::Key::Enter)] ||
                    in.key_pressed[static_cast<int>(platform::Key::Space)]) {
                    const int sid = skill_cursor_branch_ * 5 + skill_cursor_row_;
                    world_.character().learn_skill(game::SkillId(sid));
                }
                break;
            }

            // ---- Equipment panel input ----------------------------------
            if (equipment_panel_open_) {
                if (in.key_pressed[static_cast<int>(platform::Key::Escape)]) {
                    equipment_panel_open_ = false;
                    break;
                }
                if (in.key_pressed[static_cast<int>(platform::Key::Up)])
                    equipment_slot_ = (equipment_slot_ + 2) % 3;
                if (in.key_pressed[static_cast<int>(platform::Key::Down)])
                    equipment_slot_ = (equipment_slot_ + 1) % 3;
                const bool left  = in.key_pressed[static_cast<int>(platform::Key::Left)];
                const bool right = in.key_pressed[static_cast<int>(platform::Key::Right)];
                if (left || right) {
                    const int dir = left ? -1 : 1;
                    game::Character& ch = world_.character();
                    if (equipment_slot_ == 0)
                        ch.equipped.facet = ch.inventory.next_facet(ch.equipped.facet, dir);
                    else if (equipment_slot_ == 1)
                        ch.equipped.core = ch.inventory.next_core(ch.equipped.core, dir);
                    else
                        ch.equipped.hull = ch.inventory.next_hull(ch.equipped.hull, dir);
                }
                break;
            }

            // ---- Log archive input --------------------------------------
            if (log_archive_open_) {
                if (in.key_pressed[static_cast<int>(platform::Key::Escape)]) {
                    log_archive_open_ = false;
                    break;
                }
                if (in.key_pressed[static_cast<int>(platform::Key::Up)])
                    archive_cursor_ = (archive_cursor_ + game::LOG_COUNT - 1) % game::LOG_COUNT;
                if (in.key_pressed[static_cast<int>(platform::Key::Down)])
                    archive_cursor_ = (archive_cursor_ + 1) % game::LOG_COUNT;
                break;
            }

            // ---- Quest log input ----------------------------------------
            if (quest_log_open_) {
                if (in.key_pressed[static_cast<int>(platform::Key::Escape)]) {
                    quest_log_open_ = false;
                }
                break;
            }

            // ---- Normal gameplay ----------------------------------------
            world_.step(dt, in);

            const int trig = world_.pending_log_trigger();
            if (trig >= 0) {
                world_.clear_pending_log_trigger();
                if (trig < game::LOG_COUNT && !logs_.has_seen(trig)) {
                    logs_.begin(trig);
                    logs_.mark_seen(trig);
                    check_quests();
                }
            }

            const int  room_now = world_.room_index();
            const bool warden_now = world_.warden_active();
            const int  display_idx = warden_now ? game::ROOM_COUNT : room_now;
            if (display_idx != last_room_shown_ || warden_now != last_warden_state_) {
                room_intro_timer_ = 0.0f;
                last_room_shown_ = display_idx;
                last_warden_state_ = warden_now;
            }
            if (room_intro_timer_ >= 0.0f) {
                room_intro_timer_ += dt;
                if (room_intro_timer_ >= ROOM_INTRO_TOTAL) room_intro_timer_ = -1.0f;
            }

            if (world_.player_dead()) {
                screen_ = game::Screen::GameOver;
            } else if (world_.warden_defeated()) {
                screen_ = game::Screen::Victory;
                victory_time_ = 0.0f;
            }
            break;
        }
        case game::Screen::GameOver: {
            if (any_pressed) {
                world_.reset();
                logs_   = game::LogState{};
                quests_ = game::QuestState{};
                screen_ = game::Screen::Playing;
                last_room_shown_      = -1;
                last_warden_state_    = false;
                room_intro_timer_     = -1.0f;
                character_sheet_open_ = false;
                skill_tree_open_      = false;
                equipment_panel_open_ = false;
                log_archive_open_     = false;
                quest_log_open_       = false;
            }
            break;
        }
        case game::Screen::Victory: {
            victory_time_ += dt;
            if (any_pressed && victory_time_ > 2.0f) {
                world_.reset();
                logs_   = game::LogState{};
                quests_ = game::QuestState{};
                screen_ = game::Screen::Title;
                title_time_ = 0.0f;
            }
            break;
        }
    }
}

void App::render() {
    renderer_.begin_frame(platform::window_width(),
                          platform::window_height());
    switch (screen_) {
        case game::Screen::Intro:    draw_intro(renderer_);      break;
        case game::Screen::Title:    draw_title(renderer_);      break;
        case game::Screen::Playing:
            world_.draw(renderer_);
            if (logs_.playing())             draw_log_overlay(renderer_);
            else if (character_sheet_open_)  draw_character_sheet(renderer_);
            else if (skill_tree_open_)       draw_skill_tree(renderer_);
            else if (equipment_panel_open_)  draw_equipment_panel(renderer_);
            else if (log_archive_open_)      draw_log_archive(renderer_);
            else if (quest_log_open_)        draw_quest_log(renderer_);
            else                              draw_room_intro(renderer_);
            break;
        case game::Screen::GameOver: draw_game_over(renderer_);  break;
        case game::Screen::Victory:  draw_victory(renderer_);    break;
    }
    renderer_.end_frame();
}

static void draw_twinkle_stars(render::Renderer& rr, f32 t, Color base, u8 max_a) {
    for (int i = 0; i < STAR_COUNT; ++i) {
        const f32 tw = 0.55f + 0.45f * std::sin(t * 2.1f + f32(i) * 1.73f);
        const f32 a = f32(max_a) * tw;
        rr.draw_circle({ STARS[i].x, STARS[i].y },
                       STARS[i].r * (0.8f + 0.4f * tw),
                       with_alpha(base, u8(clamp(a, 0.0f, 255.0f))), 6);
    }
}

void App::draw_title(render::Renderer& rr) const {
    rr.set_layer(render::Renderer::Layer::Alpha);

    rr.draw_quad({ 0, 0 }, { VIRTUAL_W, 0 }, { VIRTUAL_W, VIRTUAL_H }, { 0, VIRTUAL_H },
                 with_alpha(COLOR_CORE_FILL, 200));
    draw_twinkle_stars(rr, title_time_, COLOR_CORE_INNER, 110);

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 cy = VIRTUAL_H * 0.30f;
    const f32 station_y = VIRTUAL_H * 0.42f;
    const f32 R_st = 340.0f;

    rr.draw_ring({ cx, station_y }, R_st - 3.0f, R_st, with_alpha(COLOR_CORE_EDGE, 55), 128);
    rr.draw_ring({ cx, station_y }, R_st - 42.0f, R_st - 34.0f, with_alpha(COLOR_CORE_EDGE, 40), 128);
    for (int a = 0; a < 4; ++a) {
        const f32 a0 = TAU * f32(a) / 4.0f + title_time_ * 0.15f;
        rr.draw_arc_ring({ cx, station_y }, R_st - 42.0f, R_st, a0, a0 + 0.55f,
                         with_alpha(COLOR_CORE_INNER, 90), 20);
    }
    for (int i = 0; i < 8; ++i) {
        const f32 a = TAU * f32(i) / 8.0f + title_time_ * 0.05f;
        rr.draw_line({ cx + std::cos(a) * 60.0f, station_y + std::sin(a) * 60.0f },
                     { cx + std::cos(a) * (R_st - 42.0f), station_y + std::sin(a) * (R_st - 42.0f) },
                     1.0f, with_alpha(COLOR_CORE_EDGE, 40));
    }
    rr.draw_circle({ cx, station_y }, 24.0f, with_alpha(COLOR_CORE_FILL, 240), 20);
    rr.draw_ring({ cx, station_y }, 24.0f, 28.0f, with_alpha(COLOR_CORE_EDGE, 160), 24);

    for (int i = 0; i < 5; ++i) {
        const f32 t0 = title_time_ * 0.1f + f32(i) * 1.3f;
        const f32 dx = cx + std::cos(t0) * (500.0f + f32(i) * 30.0f);
        const f32 dy = station_y + std::sin(t0) * (220.0f + f32(i) * 15.0f);
        rr.draw_triangle({ dx, dy - 5.0f }, { dx - 3.5f, dy + 3.0f }, { dx + 3.5f, dy + 3.0f },
                         with_alpha(COLOR_CORE_EDGE, 180));
    }

    const f32 pulse = 0.5f + 0.5f * std::sin(title_time_ * 1.6f);

    rr.draw_quad({ cx - 500.0f, cy - 30.0f }, { cx + 500.0f, cy - 30.0f },
                 { cx + 500.0f, cy + 160.0f }, { cx - 500.0f, cy + 160.0f },
                 with_alpha(COLOR_VOID, 180));

    rr.draw_text_centered({ cx, cy }, "INCIDENCE", 10.0f, COLOR_CORE_EDGE);
    rr.draw_text_centered({ cx, cy + 90.0f }, "A MIRROR DEFLECTOR", 2.2f, with_alpha(COLOR_FACET_EDGE, 200));
    rr.draw_text_centered({ cx, cy + 118.0f }, "UNIT KITE-9", 2.2f, with_alpha(COLOR_FACET_EDGE, 200));

    const Vec2 drone_pos { cx, VIRTUAL_H * 0.66f };
    rr.draw_circle(drone_pos, 14.0f, COLOR_CORE_FILL, 20);
    rr.draw_ring(drone_pos, 14.0f, 17.0f, COLOR_CORE_EDGE, 24);
    const f32 span = 2.44f;
    const f32 heading = -HALF_PI;
    for (int i = 0; i < 5; ++i) {
        const f32 step = span / 5.0f;
        const f32 a0 = heading - span * 0.5f + f32(i)     * step;
        const f32 a1 = heading - span * 0.5f + f32(i + 1) * step;
        const Vec2 pin0 = drone_pos + Vec2::from_angle(a0) * 50.0f;
        const Vec2 pin1 = drone_pos + Vec2::from_angle(a1) * 50.0f;
        const Vec2 pout0= drone_pos + Vec2::from_angle(a0) * 60.0f;
        const Vec2 pout1= drone_pos + Vec2::from_angle(a1) * 60.0f;
        rr.draw_quad(pin0, pin1, pout1, pout0, COLOR_FACET_FILL);
        rr.draw_line(pout0, pout1, 2.0f, COLOR_FACET_EDGE);
    }

    rr.draw_text_centered({ cx, VIRTUAL_H * 0.78f },
                          "REFLECT. REDIRECT. PRESERVE.",
                          2.0f, with_alpha(COLOR_FACET_EDGE, 180));

    const f32 ring_y = VIRTUAL_H * 0.88f;
    const f32 ring_r = 26.0f + 5.0f * pulse;
    rr.draw_ring({ cx, ring_y }, ring_r - 3.0f, ring_r + 3.0f, with_alpha(COLOR_CORE_INNER, 200), 32);
    rr.draw_text_centered({ cx, ring_y + 45.0f }, "PRESS ANY KEY", 2.4f, COLOR_CORE_INNER);

    rr.set_layer(render::Renderer::Layer::Additive);
    rr.draw_circle({ cx, ring_y }, 80.0f, with_alpha(COLOR_CORE_EDGE, u8(28 * pulse)), 24);
    rr.draw_circle(drone_pos, 90.0f, with_alpha(COLOR_CORE_EDGE, 24), 24);
    rr.draw_circle({ cx, station_y }, 400.0f, with_alpha(COLOR_CORE_EDGE, 10), 64);
}

void App::draw_room_intro(render::Renderer& rr) const {
    if (room_intro_timer_ < 0.0f) return;

    const f32 t = room_intro_timer_;
    f32 alpha = 1.0f;
    if (t < ROOM_INTRO_FADE_IN) {
        alpha = t / ROOM_INTRO_FADE_IN;
    } else if (t > ROOM_INTRO_FADE_IN + ROOM_INTRO_HOLD) {
        alpha = 1.0f - ((t - (ROOM_INTRO_FADE_IN + ROOM_INTRO_HOLD)) / ROOM_INTRO_FADE_OUT);
    }
    alpha = clamp(alpha, 0.0f, 1.0f);
    if (alpha < 0.01f) return;

    int idx = last_room_shown_;
    if (idx < 0 || idx > game::ROOM_COUNT) return;

    const char* name  = ROOM_NAME[idx];
    const char* blurb = ROOM_BLURB[idx];
    const Color arc_color = room_arc_color(idx);

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 band_y = 100.0f;
    const f32 w = 1200.0f;
    const f32 h = 76.0f;

    rr.set_layer(render::Renderer::Layer::Alpha);
    rr.draw_quad({ cx - w * 0.5f, band_y - h * 0.5f },
                 { cx + w * 0.5f, band_y - h * 0.5f },
                 { cx + w * 0.5f, band_y + h * 0.5f },
                 { cx - w * 0.5f, band_y + h * 0.5f },
                 with_alpha(COLOR_VOID, u8(alpha * 220.0f)));
    rr.draw_line({ cx - w * 0.5f, band_y - h * 0.5f - 4.0f },
                 { cx + w * 0.5f, band_y - h * 0.5f - 4.0f },
                 2.0f, with_alpha(arc_color, u8(alpha * 200.0f)));
    rr.draw_line({ cx - w * 0.5f, band_y + h * 0.5f + 4.0f },
                 { cx + w * 0.5f, band_y + h * 0.5f + 4.0f },
                 1.0f, with_alpha(arc_color, u8(alpha * 140.0f)));

    const f32 badge_x = cx - w * 0.5f + 44.0f;
    const f32 badge_y = band_y;
    {
        Vec2 prev{};
        for (int k = 0; k <= 6; ++k) {
            const f32 a = TAU * f32(k) / 6.0f - PI * 0.5f;
            const Vec2 p { badge_x + std::cos(a) * 24.0f, badge_y + std::sin(a) * 24.0f };
            if (k > 0) rr.draw_line(prev, p, 2.0f, with_alpha(arc_color, u8(alpha * 220.0f)));
            prev = p;
        }
        rr.draw_circle({ badge_x, badge_y }, 8.0f, with_alpha(arc_color, u8(alpha * 200.0f)), 16);
        rr.draw_circle({ badge_x, badge_y }, 3.0f, with_alpha(COLOR_VOID, u8(alpha * 255.0f)), 10);
    }

    const f32 name_px = 3.0f;
    const f32 name_x = badge_x + 40.0f;
    rr.draw_text({ name_x, band_y - 26.0f }, name, name_px,
                 with_alpha(arc_color, u8(alpha * 255.0f)));

    const char* arc_label = "ARC I";
    switch (idx) {
        case 3: case 4: arc_label = "ARC II";  break;
        case 5:         arc_label = "ARC III"; break;
        case 6: case 7: arc_label = "ARC IV";  break;
        case 8: case 9: arc_label = "ARC V";   break;
        default:        arc_label = "ARC I";   break;
    }
    const f32 name_w = rr.text_width(name, name_px);
    rr.draw_text({ name_x + name_w + 20.0f, band_y - 22.0f }, arc_label, 1.8f,
                 with_alpha(COLOR_FACET_EDGE, u8(alpha * 180.0f)));

    const f32 blurb_px = 1.8f;
    const f32 blurb_max_w = (cx + w * 0.5f) - name_x - 20.0f;
    const f32 blurb_w = rr.text_width(blurb, blurb_px);
    if (blurb_w < blurb_max_w) {
        rr.draw_text({ name_x, band_y + 8.0f }, blurb, blurb_px,
                     with_alpha(COLOR_FACET_EDGE, u8(alpha * 230.0f)));
    } else {
        char buf[256];
        int n = 0;
        while (blurb[n] && n < 250) { buf[n] = blurb[n]; ++n; }
        buf[n] = '\0';
        int cut = n;
        while (cut > 8 && rr.text_width(buf, blurb_px) > blurb_max_w - 30.0f) buf[--cut] = '\0';
        if (cut > 6) { buf[cut-3]='.'; buf[cut-2]='.'; buf[cut-1]='.'; buf[cut]='\0'; }
        rr.draw_text({ name_x, band_y + 8.0f }, buf, blurb_px,
                     with_alpha(COLOR_FACET_EDGE, u8(alpha * 230.0f)));
    }

    rr.set_layer(render::Renderer::Layer::Additive);
    rr.draw_circle({ cx, band_y }, 220.0f, with_alpha(arc_color, u8(alpha * 16.0f)), 40);
    rr.draw_circle({ badge_x, badge_y }, 60.0f, with_alpha(arc_color, u8(alpha * 60.0f)), 24);
}

void App::draw_game_over(render::Renderer& rr) const {
    rr.set_layer(render::Renderer::Layer::Alpha);
    rr.draw_quad({ 0, 0 }, { VIRTUAL_W, 0 }, { VIRTUAL_W, VIRTUAL_H }, { 0, VIRTUAL_H },
                 with_alpha(COLOR_VOID, 230));
    draw_twinkle_stars(rr, platform::time_seconds(), COLOR_ENEMY_EDGE, 70);

    const f32 t  = platform::time_seconds();
    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 shake = std::sin(t * 8.0f) * 3.0f;
    const f32 station_y = VIRTUAL_H * 0.28f + shake;
    const f32 R_st = 300.0f;

    for (int i = 0; i < 12; ++i) {
        const f32 a0 = TAU * f32(i) / 12.0f + t * 0.08f;
        const f32 drift = f32(i % 3) * 25.0f;
        rr.draw_arc_ring({ cx, station_y }, R_st - 3.0f + drift, R_st + drift,
                         a0, a0 + 0.35f, with_alpha(COLOR_ENEMY_EDGE, 130), 10);
    }
    const f32 cf = 0.4f + 0.3f * std::sin(t * 1.5f);
    rr.draw_circle({ cx, station_y }, 40.0f * cf, with_alpha(COLOR_ENEMY_CORE, u8(cf * 180.0f)), 24);
    rr.draw_circle({ cx, station_y }, 22.0f, with_alpha(COLOR_ENEMY_FILL, 240), 20);

    const f32 pp = std::fmod(t, 2.2f) / 2.2f;
    rr.draw_ring({ cx, VIRTUAL_H * 0.5f }, 80.0f + pp * 800.0f - 4.0f, 80.0f + pp * 800.0f + 4.0f,
                 with_alpha(COLOR_ENEMY_CORE, u8((1.0f - pp) * 100.0f)), 64);

    const f32 cy = VIRTUAL_H * 0.52f;
    rr.draw_quad({ cx - 400.0f, cy - 40.0f }, { cx + 400.0f, cy - 40.0f },
                 { cx + 400.0f, cy + 180.0f }, { cx - 400.0f, cy + 180.0f },
                 with_alpha(COLOR_VOID, 200));

    rr.draw_text_centered({ cx, cy }, "GAME OVER", 9.0f, COLOR_ENEMY_EDGE);
    rr.draw_text_centered({ cx, cy + 110.0f }, "KITE-9 DOWN", 2.4f, with_alpha(COLOR_ENEMY_CORE, 220));
    rr.draw_text_centered({ cx, cy + 150.0f }, "REFLECT. REDIRECT. PRESERVE.", 1.8f, with_alpha(COLOR_ENEMY_EDGE, 160));

    const f32 pulse = 0.5f + 0.5f * std::sin(t * 4.0f);
    const f32 ring_y = VIRTUAL_H * 0.82f;
    const f32 ring_r = 24.0f + 5.0f * pulse;
    rr.draw_ring({ cx, ring_y }, ring_r - 3.0f, ring_r + 3.0f,
                 with_alpha(COLOR_ENEMY_CORE, u8(140 + 100 * pulse)), 32);
    rr.draw_text_centered({ cx, ring_y + 45.0f }, "PRESS ANY KEY", 2.2f, with_alpha(COLOR_ENEMY_CORE, 220));

    rr.set_layer(render::Renderer::Layer::Additive);
    rr.draw_circle({ cx, ring_y }, 70.0f, with_alpha(COLOR_ENEMY_CORE, u8(35 * pulse)), 24);
    rr.draw_circle({ cx, station_y }, 320.0f, with_alpha(COLOR_ENEMY_FILL, 20), 64);
}

void App::draw_victory(render::Renderer& rr) const {
    rr.set_layer(render::Renderer::Layer::Alpha);
    rr.draw_quad({ 0, 0 }, { VIRTUAL_W, 0 }, { VIRTUAL_W, VIRTUAL_H }, { 0, VIRTUAL_H },
                 with_alpha(COLOR_VOID, 230));
    draw_twinkle_stars(rr, victory_time_, COLOR_CORE_INNER, 130);

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 t  = victory_time_;
    const f32 station_y = VIRTUAL_H * 0.30f;
    const f32 R_st = 320.0f;

    rr.draw_ring({ cx, station_y }, R_st - 3.0f, R_st, with_alpha(COLOR_CORE_EDGE, 200), 128);
    rr.draw_ring({ cx, station_y }, R_st - 42.0f, R_st - 36.0f, with_alpha(COLOR_CORE_INNER, 160), 128);
    for (int a = 0; a < 6; ++a) {
        const f32 a0 = TAU * f32(a) / 6.0f + victory_time_ * 0.25f;
        rr.draw_arc_ring({ cx, station_y }, R_st - 42.0f, R_st, a0, a0 + 0.35f, COLOR_CORE_INNER, 16);
    }
    for (int i = 0; i < 12; ++i) {
        const f32 a = TAU * f32(i) / 12.0f + victory_time_ * 0.03f;
        rr.draw_line({ cx + std::cos(a) * 80.0f, station_y + std::sin(a) * 80.0f },
                     { cx + std::cos(a) * (R_st - 42.0f), station_y + std::sin(a) * (R_st - 42.0f) },
                     1.0f, with_alpha(COLOR_CORE_EDGE, 60));
    }
    rr.draw_circle({ cx, station_y }, 34.0f, with_alpha(COLOR_CORE_FILL, 240), 24);
    rr.draw_ring({ cx, station_y }, 34.0f, 40.0f, COLOR_CORE_EDGE, 32);
    rr.draw_circle({ cx, station_y }, 12.0f, COLOR_CORE_INNER, 18);

    for (int i = 0; i < 40; ++i) {
        const f32 fi = f32(i);
        const f32 h = std::fmod(std::fabs(std::sin(fi * 12.9898f) * 43758.5453f), 1.0f);
        const f32 sx = h * VIRTUAL_W;
        const f32 period = 4.0f + std::fmod(fi * 0.7f, 2.5f);
        const f32 phase = std::fmod(victory_time_ / period + fi * 0.13f, 1.0f);
        const f32 sy = VIRTUAL_H - phase * VIRTUAL_H;
        const f32 sparkle = 0.5f + 0.5f * std::sin(fi + victory_time_ * 3.0f);
        const f32 a = (1.0f - phase) * sparkle * 0.7f;
        rr.draw_circle({ sx, sy }, 1.8f, with_alpha(COLOR_CORE_INNER, u8(clamp(a, 0.0f, 1.0f) * 200.0f)), 6);
    }

    const f32 cy = VIRTUAL_H * 0.52f;
    rr.draw_quad({ cx - 500.0f, cy - 40.0f }, { cx + 500.0f, cy - 40.0f },
                 { cx + 500.0f, cy + 100.0f }, { cx - 500.0f, cy + 100.0f },
                 with_alpha(COLOR_VOID, 200));
    rr.draw_text_centered({ cx, cy }, "VICTORY", 9.0f, COLOR_CORE_EDGE);
    rr.draw_text_centered({ cx, cy + 110.0f }, "THEOREM STABILIZED", 2.6f, with_alpha(COLOR_CORE_INNER, 220));
    rr.draw_text_centered({ cx, cy + 145.0f }, "THE RING WILL HOLD", 2.6f, with_alpha(COLOR_CORE_INNER, 220));

    const int dots = game::ROOM_COUNT + 1;
    const f32 dot_y = VIRTUAL_H * 0.68f;
    for (int i = 0; i < dots; ++i) {
        const f32 x = cx + (f32(i) - f32(dots - 1) * 0.5f) * 28.0f;
        const f32 offset = std::sin(t * 2.0f + f32(i) * 0.4f) * 6.0f;
        rr.draw_circle({ x, dot_y + offset }, 8.0f, COLOR_CORE_INNER, 14);
    }

    if (t > 2.0f) {
        const char* rec[] = {
            "> HALCYON STATION - FINAL STATE",
            "> RING INTEGRITY: STABLE",
            "> CREW ONBOARD: 0",
            "> OPERATIONAL DRONES: 1",
            "> THEOREM STATUS: COHERENT",
            "> LAST TRANSMISSION: IS THIS INTEGRITY?",
            "> REPLY: NONE",
        };
        const f32 rec_x  = cx - 340.0f;
        const f32 rec_y0 = VIRTUAL_H * 0.76f;
        for (int i = 0; i < 7; ++i) {
            const f32 lr = clamp((t - 2.0f - f32(i) * 0.2f) / 0.4f, 0.0f, 1.0f);
            const u8 la = u8(lr * 200.0f);
            if (la == 0) continue;
            rr.draw_text({ rec_x, rec_y0 + f32(i) * 22.0f }, rec[i], 1.9f, with_alpha(COLOR_CORE_EDGE, la));
        }
    }

    if (t > 3.5f) {
        const f32 pulse = 0.5f + 0.5f * std::sin(t * 4.0f);
        const f32 ring_y = VIRTUAL_H * 0.94f;
        const f32 ring_r = 20.0f + 4.0f * pulse;
        rr.draw_ring({ cx, ring_y }, ring_r - 3.0f, ring_r + 3.0f,
                     with_alpha(COLOR_CORE_EDGE, u8(140 + 100 * pulse)), 32);
        rr.draw_text_centered({ cx, ring_y + 40.0f }, "PRESS ANY KEY", 2.0f, with_alpha(COLOR_CORE_INNER, 220));
    }

    rr.set_layer(render::Renderer::Layer::Additive);
    rr.draw_circle({ cx, station_y }, 480.0f, with_alpha(COLOR_CORE_EDGE, 22), 64);
    rr.draw_circle({ cx, VIRTUAL_H * 0.5f }, 300.0f, with_alpha(COLOR_CORE_INNER, 8), 40);
}

void App::shutdown() {
    audio::shutdown();
    renderer_.shutdown();
    std::fprintf(stderr, "[incidence] shutdown after %llu frames\n",
                 static_cast<unsigned long long>(frame_count_));
    std::fflush(stderr);
}

} // namespace inc
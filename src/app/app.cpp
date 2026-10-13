#include "app/app.hpp"
#include "audio/audio.hpp"
#include "game/save.hpp"

#include "sokol_app.h"

#include <cmath>
#include <cstdio>

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
        // Consume the input edge after the FIRST substep so subsequent
        // substeps in the same frame do not fire the same key again.
        platform::consume_pressed();
    }
    if (substeps == MAX_SUBSTEPS_PER_FRAME) accumulator_ = 0.0;
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
                screen_ = game::Screen::Playing;
                last_room_shown_      = -1;
                last_warden_state_    = false;
                room_intro_timer_      = -1.0f;
                character_sheet_open_  = false;
                skill_tree_open_       = false;
                equipment_panel_open_  = false;
            }
            break;
        }
        case game::Screen::Playing: {
            if (in.key_pressed[static_cast<int>(platform::Key::Tab)]) {
                character_sheet_open_ = !character_sheet_open_;
                if (character_sheet_open_) { skill_tree_open_ = false; equipment_panel_open_ = false; }
            }
            if (in.key_pressed[static_cast<int>(platform::Key::K)]) {
                skill_tree_open_ = !skill_tree_open_;
                if (skill_tree_open_) { character_sheet_open_ = false; equipment_panel_open_ = false; }
            }
            if (in.key_pressed[static_cast<int>(platform::Key::E)]) {
                equipment_panel_open_ = !equipment_panel_open_;
                if (equipment_panel_open_) { character_sheet_open_ = false; skill_tree_open_ = false; }
            }

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

            if (equipment_panel_open_) {
                if (in.key_pressed[static_cast<int>(platform::Key::Escape)]) {
                    equipment_panel_open_ = false;
                    break;
                }
                if (in.key_pressed[static_cast<int>(platform::Key::Up)])
                    equipment_slot_ = (equipment_slot_ + 2) % 3;
                if (in.key_pressed[static_cast<int>(platform::Key::Down)])
                    equipment_slot_ = (equipment_slot_ + 1) % 3;
                if (in.key_pressed[static_cast<int>(platform::Key::Left)] ||
                    in.key_pressed[static_cast<int>(platform::Key::Right)]) {
                    const int dir = in.key_pressed[static_cast<int>(platform::Key::Left)] ? -1 : +1;
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

            world_.step(dt, in);

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
                screen_ = game::Screen::Playing;
                last_room_shown_      = -1;
                last_warden_state_    = false;
                room_intro_timer_      = -1.0f;
                character_sheet_open_  = false;
                skill_tree_open_       = false;
                equipment_panel_open_  = false;
            }
            break;
        }
        case game::Screen::Victory: {
            victory_time_ += dt;
            if (any_pressed && victory_time_ > 2.0f) {
                world_.reset();
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
            if (character_sheet_open_)     draw_character_sheet(renderer_);
            else if (skill_tree_open_)     draw_skill_tree(renderer_);
            else if (equipment_panel_open_) draw_equipment_panel(renderer_);
            else                           draw_room_intro(renderer_);
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

void App::draw_intro_background(render::Renderer& rr, int card_index, f32 card_t) const {
    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 cy = VIRTUAL_H * 0.32f;
    const f32 t  = intro_time_;
    const f32 pulse = 0.5f + 0.5f * std::sin(t * 1.5f);

    rr.set_layer(render::Renderer::Layer::Alpha);
    draw_twinkle_stars(rr, t, COLOR_CORE_INNER, 100);

    switch (card_index) {
    case 0: {
        const f32 R = 150.0f;
        rr.draw_ring({ cx, cy }, R - 2.0f, R, COLOR_CORE_EDGE, 64);
        rr.draw_ring({ cx, cy }, R - 22.0f, R - 18.0f, with_alpha(COLOR_CORE_EDGE, 140), 64);
        for (int a = 0; a < 4; ++a) {
            const f32 a0 = TAU * f32(a) / 4.0f + t * 0.2f;
            rr.draw_arc_ring({ cx, cy }, R - 22.0f, R, a0, a0 + 0.6f, COLOR_CORE_INNER, 12);
        }
        const f32 marker_a = t * 0.6f;
        const Vec2 m = { cx + std::cos(marker_a) * R, cy + std::sin(marker_a) * R };
        rr.draw_circle(m, 5.0f, COLOR_CORE_INNER, 12);
        for (int i = 0; i < 6; ++i) {
            const f32 a = TAU * f32(i) / 6.0f;
            rr.draw_line({ cx + std::cos(a) * 20.0f, cy + std::sin(a) * 20.0f },
                         { cx + std::cos(a) * (R - 22.0f), cy + std::sin(a) * (R - 22.0f) },
                         1.0f, with_alpha(COLOR_CORE_EDGE, 90));
        }
        rr.draw_circle({ cx, cy }, 14.0f, with_alpha(COLOR_CORE_FILL, 220), 20);
        rr.draw_ring({ cx, cy }, 14.0f, 18.0f, COLOR_CORE_EDGE, 24);
        break;
    }
    case 1: {
        const f32 R = 120.0f;
        rr.draw_ring({ cx, cy }, R - 3.0f, R, COLOR_FACET_EDGE, 48);
        for (int i = 0; i < 16; ++i) {
            const f32 a = TAU * f32(i) / 16.0f + t * 0.15f;
            const Vec2 outer = { cx + std::cos(a) * (R * 2.4f), cy + std::sin(a) * (R * 2.4f) };
            const Vec2 inner = { cx + std::cos(a) * 40.0f, cy + std::sin(a) * 40.0f };
            rr.draw_line(outer, inner, 1.0f, with_alpha(COLOR_FACET_EDGE, 110));
        }
        rr.draw_circle({ cx, cy }, 24.0f + pulse * 6.0f, with_alpha(COLOR_CORE_INNER, 200), 24);
        rr.draw_circle({ cx, cy }, 40.0f + pulse * 10.0f, with_alpha(COLOR_CORE_EDGE, 80), 32);
        break;
    }
    case 2: {
        const f32 drift = card_t * 55.0f;
        for (int i = 0; i < 18; ++i) {
            const f32 a = TAU * f32(i) / 18.0f + f32(i) * 0.17f;
            const f32 rad = 90.0f + drift + f32(i % 4) * 12.0f;
            const Vec2 p0 = { cx + std::cos(a - 0.10f) * rad, cy + std::sin(a - 0.10f) * rad };
            const Vec2 p1 = { cx + std::cos(a + 0.10f) * rad, cy + std::sin(a + 0.10f) * rad };
            rr.draw_line(p0, p1, 4.0f, with_alpha(COLOR_CORE_EDGE, 160));
            rr.draw_circle(p0, 2.5f, with_alpha(COLOR_CORE_INNER, 200), 8);
            rr.draw_circle(p1, 2.5f, with_alpha(COLOR_CORE_INNER, 200), 8);
        }
        const f32 wave = 40.0f + card_t * 380.0f;
        const f32 wave_a = clamp(1.0f - card_t / 2.8f, 0.0f, 1.0f);
        rr.draw_ring({ cx, cy }, wave - 3.0f, wave + 3.0f,
                     with_alpha(COLOR_ENEMY_EDGE, u8(wave_a * 180.0f)), 64);
        const f32 cf = clamp(1.0f - card_t / 3.5f, 0.0f, 1.0f);
        rr.draw_circle({ cx, cy }, 30.0f * cf, with_alpha(COLOR_ENEMY_CORE, u8(cf * 200.0f)), 20);
        break;
    }
    case 3: {
        const f32 R = 160.0f;
        const f32 rot = t * 0.12f;
        {
            Vec2 prev{};
            for (int i = 0; i <= 6; ++i) {
                const f32 a = rot + TAU * f32(i) / 6.0f;
                const Vec2 p = { cx + std::cos(a) * R, cy + std::sin(a) * R };
                if (i > 0) rr.draw_line(prev, p, 2.0f, with_alpha(COLOR_ENEMY_EDGE, 160));
                prev = p;
            }
        }
        {
            Vec2 prev{};
            const f32 R2 = R * 0.62f;
            for (int i = 0; i <= 3; ++i) {
                const f32 a = -rot * 1.8f + TAU * f32(i) / 3.0f;
                const Vec2 p = { cx + std::cos(a) * R2, cy + std::sin(a) * R2 };
                if (i > 0) rr.draw_line(prev, p, 2.5f, with_alpha(COLOR_ENEMY_CORE, 200));
                prev = p;
            }
        }
        rr.draw_circle({ cx, cy }, 22.0f + pulse * 6.0f, with_alpha(COLOR_ENEMY_CORE, 220), 24);
        rr.draw_circle({ cx, cy }, 12.0f, COLOR_ENEMY_CORE, 18);
        rr.draw_circle({ cx, cy }, 4.0f, COLOR_VOID, 10);
        const f32 blink = std::fabs(std::sin(t * 0.4f));
        if (blink < 0.15f) rr.draw_line({ cx - 30.0f, cy }, { cx + 30.0f, cy }, 3.0f, COLOR_ENEMY_FILL);
        break;
    }
    case 4: {
        const Vec2 dp { cx, cy };
        const f32 R_CORE = 20.0f;
        const f32 R_IN = 78.0f;
        const f32 R_OUT = 92.0f;
        const f32 span = 2.44f;
        const f32 heading = -HALF_PI;
        for (int i = 0; i < 5; ++i) {
            const f32 step = span / 5.0f;
            const f32 a0 = heading - span * 0.5f + f32(i)     * step;
            const f32 a1 = heading - span * 0.5f + f32(i + 1) * step;
            const Vec2 pin0  = dp + Vec2::from_angle(a0) * R_IN;
            const Vec2 pin1  = dp + Vec2::from_angle(a1) * R_IN;
            const Vec2 pout0 = dp + Vec2::from_angle(a0) * R_OUT;
            const Vec2 pout1 = dp + Vec2::from_angle(a1) * R_OUT;
            rr.draw_quad(pin0, pin1, pout1, pout0, COLOR_FACET_FILL);
            rr.draw_line(pin0, pin1, 2.0f, COLOR_FACET_EDGE);
            rr.draw_line(pout0, pout1, 3.0f, COLOR_FACET_EDGE);
            rr.draw_line(pin0, pout0, 1.2f, COLOR_FACET_EDGE);
            rr.draw_line(pin1, pout1, 1.2f, COLOR_FACET_EDGE);
            const f32 mid = (a0 + a1) * 0.5f;
            const Vec2 n = Vec2::from_angle(mid);
            rr.draw_line(dp + n * (R_OUT + 4.0f), dp + n * (R_OUT + 18.0f), 2.0f, COLOR_FACET_NORMAL);
        }
        rr.draw_circle(dp, R_CORE + 8.0f, COLOR_CORE_GLOW, 24);
        rr.draw_circle(dp, R_CORE, COLOR_CORE_FILL, 24);
        rr.draw_ring(dp, R_CORE, R_CORE + 2.5f, COLOR_CORE_EDGE, 32);
        rr.draw_circle(dp, 5.0f, COLOR_CORE_INNER, 12);
        rr.draw_circle(dp, 200.0f + pulse * 20.0f, with_alpha(COLOR_CORE_EDGE, 18), 40);
        break;
    }
    case 5: {
        const Vec2 hit { cx, cy };
        const Vec2 n = Vec2::from_angle(-PI * 0.25f);
        const Vec2 tangent = n.perp();
        const Vec2 m0 = hit - tangent * 80.0f;
        const Vec2 m1 = hit + tangent * 80.0f;
        rr.draw_line(m0, m1, 6.0f, COLOR_FACET_EDGE);
        rr.draw_line(m0, m1, 2.0f, COLOR_CORE_INNER);
        for (int i = 0; i < 7; ++i) {
            const f32 d0 = 12.0f + f32(i) * 18.0f;
            rr.draw_line(hit + n * d0, hit + n * (d0 + 8.0f), 1.8f, with_alpha(COLOR_FACET_NORMAL, 200));
        }
        const Vec2 in_dir = { 1.0f, 0.35f };
        const Vec2 in_unit = in_dir.normalized();
        const Vec2 in_start = hit - in_unit * 380.0f;
        rr.draw_line(in_start, hit, 3.0f, COLOR_BULLET_CORE);
        rr.draw_circle(in_start, 6.0f, COLOR_BULLET_GLOW, 12);
        const Vec2 out_dir = in_dir - n * (2.0f * in_dir.dot(n));
        const Vec2 out_unit = out_dir.normalized();
        rr.draw_line(hit, hit + out_unit * 380.0f, 3.0f, COLOR_CORE_INNER);
        rr.draw_circle(hit, 12.0f + pulse * 4.0f, with_alpha(COLOR_CORE_INNER, 200), 20);
        rr.draw_circle(hit, 26.0f + pulse * 6.0f, with_alpha(COLOR_CORE_EDGE, 90), 24);
        const f32 ang_in  = std::atan2(-in_unit.y, -in_unit.x);
        const f32 ang_ref = std::atan2(out_unit.y, out_unit.x);
        const f32 ang_n   = std::atan2(n.y, n.x);
        rr.draw_arc_ring(hit, 50.0f, 54.0f, ang_in, ang_n, COLOR_BULLET_CORE, 12);
        rr.draw_arc_ring(hit, 50.0f, 54.0f, ang_n, ang_ref, COLOR_CORE_INNER, 12);
        break;
    }
    }
}

void App::draw_intro(render::Renderer& rr) const {
    f32 t = intro_time_;
    int card_index = -1;
    f32 card_t = 0.0f;
    for (int i = 0; i < INTRO_CARD_COUNT; ++i) {
        if (t < INTRO_CARDS[i].duration) { card_index = i; card_t = t; break; }
        t -= INTRO_CARDS[i].duration;
    }
    if (card_index < 0) return;

    const IntroCard& card = INTRO_CARDS[card_index];
    const f32 p = card_t / card.duration;

    f32 alpha = 1.0f;
    if (p < 0.15f)      alpha = p / 0.15f;
    else if (p > 0.85f) alpha = (1.0f - p) / 0.15f;
    alpha = clamp(alpha, 0.0f, 1.0f);

    rr.set_layer(render::Renderer::Layer::Alpha);
    if (card_index == 2) {
        rr.draw_quad({ 0, 0 }, { VIRTUAL_W, 0 }, { VIRTUAL_W, VIRTUAL_H }, { 0, VIRTUAL_H },
                     with_alpha(COLOR_ENEMY_FILL, u8(alpha * 30.0f)));
    } else if (card_index == 3) {
        rr.draw_quad({ 0, 0 }, { VIRTUAL_W, 0 }, { VIRTUAL_W, VIRTUAL_H }, { 0, VIRTUAL_H },
                     with_alpha(COLOR_ENEMY_FILL, u8(alpha * 18.0f)));
    }

    draw_intro_background(rr, card_index, card_t);

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 text_y_top = VIRTUAL_H * 0.58f;
    const f32 text_y_bot = VIRTUAL_H * 0.68f;

    rr.draw_line({ cx - 500.0f, text_y_top - 20.0f }, { cx + 500.0f, text_y_top - 20.0f },
                 1.5f, with_alpha(card.accent, u8(alpha * 100.0f)));
    rr.draw_line({ cx - 500.0f, text_y_bot + 70.0f }, { cx + 500.0f, text_y_bot + 70.0f },
                 1.5f, with_alpha(card.accent, u8(alpha * 100.0f)));

    if (card.small && card.small[0] != '\0') {
        rr.draw_text_centered({ cx, text_y_top }, card.small, 2.6f,
                              with_alpha(COLOR_FACET_EDGE, u8(alpha * 200.0f)));
    }
    if (card.large && card.large[0] != '\0') {
        rr.draw_text_centered({ cx, text_y_bot }, card.large, 6.0f,
                              with_alpha(card.accent, u8(alpha * 255.0f)));
    }

    rr.set_layer(render::Renderer::Layer::Additive);
    rr.draw_circle({ cx, VIRTUAL_H * 0.32f }, 340.0f,
                   with_alpha(card.accent, u8(alpha * 14.0f)), 40);

    rr.set_layer(render::Renderer::Layer::Alpha);
    rr.draw_text({ VIRTUAL_W - 340.0f, VIRTUAL_H - 46.0f },
                 "PRESS ANY KEY TO SKIP", 1.8f,
                 with_alpha(COLOR_FACET_EDGE, 110));

    const f32 dot_y = VIRTUAL_H - 60.0f;
    const f32 dot_spacing = 26.0f;
    const f32 dots_x0 = cx - (f32(INTRO_CARD_COUNT - 1) * dot_spacing) * 0.5f;
    for (int i = 0; i < INTRO_CARD_COUNT; ++i) {
        const Vec2 c { dots_x0 + f32(i) * dot_spacing, dot_y };
        if (i < card_index) {
            rr.draw_circle(c, 4.0f, with_alpha(COLOR_CORE_EDGE, 140), 12);
        } else if (i == card_index) {
            rr.draw_circle(c, 5.5f, COLOR_CORE_INNER, 12);
            rr.draw_ring(c, 8.0f, 10.0f, with_alpha(card.accent, 180), 16);
        } else {
            rr.draw_ring(c, 3.0f, 4.0f, with_alpha(COLOR_CORE_EDGE, 90), 12);
        }
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

void App::draw_character_sheet(render::Renderer& r) const {
    const game::Character& ch = world_.character();

    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({ 0, 0 }, { VIRTUAL_W, 0 }, { VIRTUAL_W, VIRTUAL_H }, { 0, VIRTUAL_H },
                with_alpha(COLOR_VOID, 220));

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
        { "ARC",    ch.stats.arc,    rad_to_deg(ch.shield_span_rad()), "deg"  },
        { "FOCUS",  ch.stats.focus,  ch.spin_speed(),                   "rad/s" },
        { "CHARGE", ch.stats.charge, ch.reflect_multiplier(),           "x"    },
        { "DRIFT",  ch.stats.drift,  ch.dash_impulse(),                 "px/s" },
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

    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({ 0, 0 }, { VIRTUAL_W, 0 }, { VIRTUAL_W, VIRTUAL_H }, { 0, VIRTUAL_H },
                with_alpha(COLOR_VOID, 220));

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


// ---------------------------------------------------------------------------
// Equipment panel (E key).
// ---------------------------------------------------------------------------
void App::draw_equipment_panel(render::Renderer& r) const {
    const game::Character& ch = world_.character();

    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_quad({ 0, 0 }, { VIRTUAL_W, 0 },
                { VIRTUAL_W, VIRTUAL_H }, { 0, VIRTUAL_H },
                with_alpha(COLOR_VOID, 220));

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 panel_top = VIRTUAL_H * 0.16f;
    const f32 panel_w = 900.0f;
    const f32 panel_h = 680.0f;
    const f32 panel_left = cx - panel_w * 0.5f;

    r.draw_quad({ panel_left, panel_top }, { panel_left + panel_w, panel_top },
                { panel_left + panel_w, panel_top + panel_h },
                { panel_left, panel_top + panel_h },
                with_alpha(COLOR_CORE_FILL, 200));
    r.draw_line({ panel_left, panel_top }, { panel_left + panel_w, panel_top },
                2.0f, COLOR_CORE_EDGE);
    r.draw_line({ panel_left, panel_top + panel_h },
                { panel_left + panel_w, panel_top + panel_h },
                2.0f, COLOR_CORE_EDGE);

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

        // Selection highlight
        if (row.selected) {
            r.draw_quad({ panel_left + 30.0f,             y - 10.0f },
                        { panel_left + panel_w - 30.0f,   y - 10.0f },
                        { panel_left + panel_w - 30.0f,   y + row_h - 10.0f },
                        { panel_left + 30.0f,             y + row_h - 10.0f },
                        with_alpha(COLOR_CORE_EDGE, 40));
            r.draw_line({ panel_left + 30.0f, y - 10.0f },
                        { panel_left + panel_w - 30.0f, y - 10.0f },
                        2.0f, COLOR_CORE_INNER);
            r.draw_line({ panel_left + 30.0f, y + row_h - 10.0f },
                        { panel_left + panel_w - 30.0f, y + row_h - 10.0f },
                        2.0f, COLOR_CORE_INNER);
        }

        // Slot label
        r.draw_text({ pad_x, y + 20.0f }, row.slot, 2.6f,
                    row.selected ? COLOR_CORE_INNER : with_alpha(COLOR_FACET_EDGE, 200));

        // Item name
        r.draw_text({ pad_x + 180.0f, y + 20.0f }, row.name, 2.8f,
                    row.selected ? COLOR_CORE_INNER : COLOR_CORE_EDGE);

        // Effect description
        r.draw_text({ pad_x + 180.0f, y + 60.0f }, row.desc, 2.0f,
                    with_alpha(COLOR_FACET_EDGE, 220));

        // Count of unlocked items in this slot
        char count_buf[32];
        std::snprintf(count_buf, sizeof(count_buf), "%d / 3", row.count_unlocked);
        const f32 cw = r.text_width(count_buf, 2.0f);
        r.draw_text({ panel_left + panel_w - 60.0f - cw, y + 20.0f }, count_buf, 2.0f,
                    with_alpha(COLOR_FACET_EDGE, 180));

        y += row_h;
    }

    y += 20.0f;

    // Footer hints
    r.draw_text({ pad_x, y },
                "UP / DOWN: SELECT SLOT    LEFT / RIGHT: CHANGE ITEM",
                2.0f, with_alpha(COLOR_FACET_EDGE, 200));
    r.draw_text({ pad_x, y + 30.0f },
                "[E] / [ESC] TO CLOSE",
                2.0f, with_alpha(COLOR_FACET_EDGE, 200));
}
void App::shutdown() {
    audio::shutdown();
    renderer_.shutdown();
    std::fprintf(stderr, "[incidence] shutdown after %llu frames\n",
                 static_cast<unsigned long long>(frame_count_));
    std::fflush(stderr);
}

} // namespace inc
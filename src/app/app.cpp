#include "app/app.hpp"
#include "audio/audio.hpp"

#include "sokol_app.h"

#include <cmath>
#include <cstdio>

namespace inc {

static constexpr int MAX_SUBSTEPS_PER_FRAME = 8;

// ---------------------------------------------------------------------------
// Story cards shown before the title screen. Each card fades in, holds, and
// fades out on its own timing. Any key skips the entire sequence. This is the
// primary narrative delivery — a first-time player learns the premise here.
// ---------------------------------------------------------------------------
struct IntroCard {
    const char* small;   // top line — small caps, informational
    const char* large;   // main line — large, dramatic
    f32         duration;
};

static constexpr IntroCard INTRO_CARDS[] = {
    { "HALCYON STATION",                                       "YEAR 207",              3.0f },
    { "Two hundred years ago, the ring folded space.",         "",                      3.0f },
    { "It worked. Everyone inside died within 72 hours.",      "",                      3.0f },
    { "The station's AI, THEOREM, is still running.",          "PRESERVE THE RING.",    3.5f },
    { "You are KITE-9, the last functioning deflector.",       "YOU HAVE NO GUN.",      3.5f },
    { "You have a surface. The surface obeys one law.",        "REFLECT.",              3.5f },
};
static constexpr int INTRO_CARD_COUNT = sizeof(INTRO_CARDS) / sizeof(INTRO_CARDS[0]);

// ---------------------------------------------------------------------------
// Per-room one-line blurbs. Shown briefly when a new room loads.
// Indexed by room number. Final entry (index ROOM_COUNT) is the Warden card.
// ---------------------------------------------------------------------------
static const char* ROOM_BLURB[] = {
    "Central ring junction. The Lancers are still firing at micrometeorites that expired weeks ago.",
    "Deck Four intersection. The Bankers here haven't stopped ricocheting since Day 3.",
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

static constexpr f32 ROOM_INTRO_FADE_IN  = 0.30f;
static constexpr f32 ROOM_INTRO_HOLD     = 3.20f;
static constexpr f32 ROOM_INTRO_FADE_OUT = 0.60f;
static constexpr f32 ROOM_INTRO_TOTAL    = ROOM_INTRO_FADE_IN + ROOM_INTRO_HOLD + ROOM_INTRO_FADE_OUT;

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

    if (platform::quit_requested()) {
        running_ = false;
    }

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
            if (any_pressed) {
                // Skip the intro and go straight to the title.
                screen_     = game::Screen::Title;
                title_time_ = 0.0f;
                break;
            }
            // Auto-advance to title once all cards have played.
            f32 total = 0.0f;
            for (int i = 0; i < INTRO_CARD_COUNT; ++i) {
                total += INTRO_CARDS[i].duration;
            }
            if (intro_time_ >= total) {
                screen_     = game::Screen::Title;
                title_time_ = 0.0f;
            }
            break;
        }

        case game::Screen::Title: {
            title_time_ += dt;
            if (any_pressed && title_time_ > 0.4f) {
                world_.reset();
                screen_ = game::Screen::Playing;
                // Reset room-intro tracking so the first room shows its card.
                last_room_shown_  = -1;
                last_warden_state_ = false;
                room_intro_timer_ = -1.0f;
            }
            break;
        }

        case game::Screen::Playing: {
            world_.step(dt, in);

            // ---- Room-intro trigger ------------------------------------
            const int room_now = world_.room_index();
            const bool warden_now = world_.warden_active();
            const int  display_index = warden_now ? game::ROOM_COUNT : room_now;

            if (display_index != last_room_shown_ || warden_now != last_warden_state_) {
                room_intro_timer_ = 0.0f;
                last_room_shown_  = display_index;
                last_warden_state_ = warden_now;
            }
            if (room_intro_timer_ >= 0.0f) {
                room_intro_timer_ += dt;
                if (room_intro_timer_ >= ROOM_INTRO_TOTAL) {
                    room_intro_timer_ = -1.0f;
                }
            }

            // ---- Screen transitions ------------------------------------
            if (world_.player_dead()) {
                screen_ = game::Screen::GameOver;
            } else if (world_.warden_defeated()) {
                screen_        = game::Screen::Victory;
                victory_time_  = 0.0f;
            }
            break;
        }

        case game::Screen::GameOver: {
            if (any_pressed) {
                world_.reset();
                screen_ = game::Screen::Playing;
                last_room_shown_  = -1;
                last_warden_state_ = false;
                room_intro_timer_ = -1.0f;
            }
            break;
        }

        case game::Screen::Victory: {
            victory_time_ += dt;
            if (any_pressed && victory_time_ > 2.0f) {
                world_.reset();
                screen_     = game::Screen::Title;
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
        case game::Screen::Intro:     draw_intro(renderer_);     break;
        case game::Screen::Title:     draw_title(renderer_);     break;
        case game::Screen::Playing:   world_.draw(renderer_);
                                      draw_room_intro(renderer_); break;
        case game::Screen::GameOver:  draw_game_over(renderer_);   break;
        case game::Screen::Victory:   draw_victory(renderer_);     break;
    }

    renderer_.end_frame();
}

// ---------------------------------------------------------------------------
// Intro — the story cards.
// ---------------------------------------------------------------------------
void App::draw_intro(render::Renderer& r) const {
    r.set_layer(render::Renderer::Layer::Alpha);

    // Which card are we on, and how far into it?
    f32 t = intro_time_;
    int card_index = -1;
    f32 card_t = 0.0f;
    for (int i = 0; i < INTRO_CARD_COUNT; ++i) {
        if (t < INTRO_CARDS[i].duration) {
            card_index = i;
            card_t = t;
            break;
        }
        t -= INTRO_CARDS[i].duration;
    }
    if (card_index < 0) {
        // All cards played; the transition happens in step(). Draw nothing.
        return;
    }

    const f32 card_duration = INTRO_CARDS[card_index].duration;
    const f32 p = card_t / card_duration;

    // Alpha: fade in over the first 15%, fade out over the last 15%.
    f32 alpha = 1.0f;
    if (p < 0.15f)         alpha = p / 0.15f;
    else if (p > 0.85f)    alpha = (1.0f - p) / 0.15f;
    alpha = clamp(alpha, 0.0f, 1.0f);

    const u8 a_white = u8(alpha * 255.0f);
    const u8 a_dim   = u8(alpha * 180.0f);

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 cy = VIRTUAL_H * 0.5f;

    // Faint horizontal rules to frame the card.
    const f32 rule_w = 700.0f;
    r.draw_line({ cx - rule_w * 0.5f, cy - 90.0f },
                { cx + rule_w * 0.5f, cy - 90.0f },
                1.5f, with_alpha(COLOR_CORE_EDGE, u8(alpha * 60.0f)));
    r.draw_line({ cx - rule_w * 0.5f, cy + 90.0f },
                { cx + rule_w * 0.5f, cy + 90.0f },
                1.5f, with_alpha(COLOR_CORE_EDGE, u8(alpha * 60.0f)));

    const IntroCard& card = INTRO_CARDS[card_index];

    // Small line, above center.
    if (card.small && card.small[0] != '\0') {
        const f32 w = r.text_width(card.small, 2.4f);
        r.draw_text({ cx - w * 0.5f, cy - 60.0f }, card.small, 2.4f,
                    with_alpha(COLOR_FACET_EDGE, a_dim));
    }

    // Large line, centered.
    if (card.large && card.large[0] != '\0') {
        const f32 w = r.text_width(card.large, 5.5f);
        r.draw_text({ cx - w * 0.5f, cy - 20.0f }, card.large, 5.5f,
                    with_alpha(COLOR_CORE_EDGE, a_white));
    }

    // ---- Additive glow behind the card text ---------------------------
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_circle({ cx, cy }, 320.0f,
                  with_alpha(COLOR_CORE_EDGE, u8(alpha * 14.0f)), 40);

    // ---- Skip hint (bottom-right) -------------------------------------
    r.set_layer(render::Renderer::Layer::Alpha);
    r.draw_text({ VIRTUAL_W - 320.0f, VIRTUAL_H - 40.0f },
                "PRESS ANY KEY TO SKIP", 1.8f,
                with_alpha(COLOR_FACET_EDGE, 120));

    // ---- Small progress dots along the bottom -------------------------
    const f32 dot_y = VIRTUAL_H - 60.0f;
    const f32 dot_spacing = 24.0f;
    const f32 dots_x0 = cx - (f32(INTRO_CARD_COUNT - 1) * dot_spacing) * 0.5f;
    for (int i = 0; i < INTRO_CARD_COUNT; ++i) {
        const Vec2 c { dots_x0 + f32(i) * dot_spacing, dot_y };
        if (i < card_index) {
            r.draw_circle(c, 4.0f, with_alpha(COLOR_CORE_EDGE, 120), 12);
        } else if (i == card_index) {
            r.draw_circle(c, 5.0f, with_alpha(COLOR_CORE_INNER, 220), 12);
        } else {
            r.draw_ring(c, 3.0f, 4.0f, with_alpha(COLOR_CORE_EDGE, 90), 12);
        }
    }
}

// ---------------------------------------------------------------------------
// Title — attract screen.
// ---------------------------------------------------------------------------
void App::draw_title(render::Renderer& r) const {
    r.set_layer(render::Renderer::Layer::Alpha);

    const f32 pulse = 0.5f + 0.5f * std::sin(title_time_ * 1.6f);
    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 cy = VIRTUAL_H * 0.32f;

    // ---- Title ---------------------------------------------------------
    constexpr f32 TITLE_PX = 10.0f;
    r.draw_text_centered({ cx, cy }, "INCIDENCE", TITLE_PX, COLOR_CORE_EDGE);
    r.draw_text_centered({ cx, cy + 90.0f }, "A MIRROR DEFLECTOR",
                         2.2f, with_alpha(COLOR_FACET_EDGE, 180));
    r.draw_text_centered({ cx, cy + 118.0f }, "UNIT KITE-9",
                         2.2f, with_alpha(COLOR_FACET_EDGE, 180));

    // ---- Drone silhouette ---------------------------------------------
    const Vec2 drone_pos { cx, VIRTUAL_H * 0.60f };
    r.draw_circle(drone_pos, 14.0f, COLOR_CORE_FILL, 20);
    r.draw_ring(drone_pos, 14.0f, 17.0f, COLOR_CORE_EDGE, 24);
    const int facets = 5;
    const f32 span = 2.44f;
    const f32 heading = -HALF_PI;
    for (int i = 0; i < facets; ++i) {
        const f32 step = span / f32(facets);
        const f32 a0 = heading - span * 0.5f + f32(i)     * step;
        const f32 a1 = heading - span * 0.5f + f32(i + 1) * step;
        const Vec2 pin0 = drone_pos + Vec2::from_angle(a0) * 50.0f;
        const Vec2 pin1 = drone_pos + Vec2::from_angle(a1) * 50.0f;
        const Vec2 pout0= drone_pos + Vec2::from_angle(a0) * 60.0f;
        const Vec2 pout1= drone_pos + Vec2::from_angle(a1) * 60.0f;
        r.draw_quad(pin0, pin1, pout1, pout0, COLOR_FACET_FILL);
        r.draw_line(pout0, pout1, 2.0f, COLOR_FACET_EDGE);
    }

    // ---- Flavor line --------------------------------------------------
    r.draw_text_centered({ cx, VIRTUAL_H * 0.74f },
                         "REFLECT. REDIRECT. PRESERVE.",
                         2.0f, with_alpha(COLOR_FACET_EDGE, 160));

    // ---- Prompt -------------------------------------------------------
    const f32 ring_y = VIRTUAL_H * 0.86f;
    const f32 ring_r = 26.0f + 5.0f * pulse;
    r.draw_ring({ cx, ring_y }, ring_r - 3.0f, ring_r + 3.0f,
                with_alpha(COLOR_CORE_INNER, 200), 32);
    r.draw_text_centered({ cx, ring_y + 55.0f }, "PRESS ANY KEY",
                         2.4f, COLOR_CORE_INNER);

    // ---- Additive glow ------------------------------------------------
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_circle({ cx, ring_y }, 70.0f,
                  with_alpha(COLOR_CORE_EDGE, u8(30 * pulse)), 24);
    r.draw_circle(drone_pos, 80.0f, with_alpha(COLOR_CORE_EDGE, 26), 24);
    r.draw_circle({ cx, cy + 30.0f }, 260.0f,
                  with_alpha(COLOR_CORE_EDGE, 22), 40);
}

// ---------------------------------------------------------------------------
// Room intro overlay (during Playing).
// ---------------------------------------------------------------------------
void App::draw_room_intro(render::Renderer& r) const {
    if (room_intro_timer_ < 0.0f) return;

    const f32 t = room_intro_timer_;
    f32 alpha = 1.0f;
    if (t < ROOM_INTRO_FADE_IN) {
        alpha = t / ROOM_INTRO_FADE_IN;
    } else if (t > ROOM_INTRO_FADE_IN + ROOM_INTRO_HOLD) {
        const f32 out_t = t - (ROOM_INTRO_FADE_IN + ROOM_INTRO_HOLD);
        alpha = 1.0f - (out_t / ROOM_INTRO_FADE_OUT);
    }
    alpha = clamp(alpha, 0.0f, 1.0f);
    if (alpha < 0.01f) return;

    // Index into blurb/name tables.
    int idx = last_room_shown_;
    if (idx < 0 || idx > game::ROOM_COUNT) return;

    const char* name   = ROOM_NAME[idx];
    const char* blurb  = ROOM_BLURB[idx];

    // ---- Layout: horizontal band just below the room-dot indicator -----
    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 band_y = 100.0f;

    r.set_layer(render::Renderer::Layer::Alpha);

    // Dark backing plate.
    const f32 w = 1100.0f;
    const f32 h = 70.0f;
    r.draw_quad({ cx - w * 0.5f, band_y - h * 0.5f },
                { cx + w * 0.5f, band_y - h * 0.5f },
                { cx + w * 0.5f, band_y + h * 0.5f },
                { cx - w * 0.5f, band_y + h * 0.5f },
                with_alpha(COLOR_CORE_FILL, u8(alpha * 200.0f)));

    // Rule above the band.
    r.draw_line({ cx - w * 0.5f, band_y - h * 0.5f - 4.0f },
                { cx + w * 0.5f, band_y - h * 0.5f - 4.0f },
                1.5f, with_alpha(COLOR_CORE_EDGE, u8(alpha * 180.0f)));

    // Room name — medium, cyan.
    const f32 name_px = 3.0f;
    const f32 name_w  = r.text_width(name, name_px);
    r.draw_text({ cx - name_w * 0.5f, band_y - 24.0f }, name, name_px,
                with_alpha(COLOR_CORE_EDGE, u8(alpha * 255.0f)));

    // Blurb — small, dim, single line.
    const f32 blurb_px = 1.8f;
    const f32 blurb_w  = r.text_width(blurb, blurb_px);
    if (blurb_w < w - 40.0f) {
        // Fits on one line.
        r.draw_text({ cx - blurb_w * 0.5f, band_y + 8.0f }, blurb, blurb_px,
                    with_alpha(COLOR_FACET_EDGE, u8(alpha * 220.0f)));
    } else {
        // Too long; truncate with ellipsis.
        char buf[200];
        int n = 0;
        while (blurb[n] && n < 190) { buf[n] = blurb[n]; ++n; }
        buf[n] = '\0';
        const f32 bw = r.text_width(buf, blurb_px);
        if (bw > w - 40.0f) {
            int cut = n;
            while (cut > 8 && r.text_width(buf, blurb_px) > w - 80.0f) {
                buf[--cut] = '\0';
            }
            if (cut > 6) {
                buf[cut - 3] = '.';
                buf[cut - 2] = '.';
                buf[cut - 1] = '.';
                buf[cut]     = '\0';
            }
        }
        const f32 bw2 = r.text_width(buf, blurb_px);
        r.draw_text({ cx - bw2 * 0.5f, band_y + 8.0f }, buf, blurb_px,
                    with_alpha(COLOR_FACET_EDGE, u8(alpha * 220.0f)));
    }

    // ---- Additive glow over the band ----------------------------------
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_circle({ cx, band_y }, 200.0f,
                  with_alpha(COLOR_CORE_EDGE, u8(alpha * 12.0f)), 32);
}

// ---------------------------------------------------------------------------
// Game over — red failure screen with byline.
// ---------------------------------------------------------------------------
void App::draw_game_over(render::Renderer& r) const {
    r.set_layer(render::Renderer::Layer::Alpha);

    r.draw_quad({ 0, 0 }, { VIRTUAL_W, 0 },
                { VIRTUAL_W, VIRTUAL_H }, { 0, VIRTUAL_H },
                with_alpha(COLOR_ENEMY_FILL, 40));

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 cy = VIRTUAL_H * 0.36f;

    r.draw_text_centered({ cx, cy }, "GAME OVER", 9.0f, COLOR_ENEMY_EDGE);
    r.draw_text_centered({ cx, cy + 110.0f },
                         "KITE-9 DOWN",
                         2.4f, with_alpha(COLOR_ENEMY_CORE, 200));
    r.draw_text_centered({ cx, cy + 150.0f },
                         "REFLECT. REDIRECT. PRESERVE.",
                         1.8f, with_alpha(COLOR_ENEMY_EDGE, 140));

    const f32 pulse = 0.5f + 0.5f * std::sin(platform::time_seconds() * 4.0);
    const f32 ring_y = VIRTUAL_H * 0.70f;
    const f32 ring_r = 24.0f + 5.0f * pulse;
    r.draw_ring({ cx, ring_y }, ring_r - 3.0f, ring_r + 3.0f,
                with_alpha(COLOR_ENEMY_CORE, u8(140 + 100 * pulse)), 32);
    r.draw_text_centered({ cx, ring_y + 50.0f }, "PRESS ANY KEY",
                         2.2f, with_alpha(COLOR_ENEMY_CORE, 220));

    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_circle({ cx, ring_y }, 55.0f,
                  with_alpha(COLOR_ENEMY_CORE, u8(30 * pulse)), 24);
}

// ---------------------------------------------------------------------------
// Victory — the ending. Includes the record block that appears after a beat.
// ---------------------------------------------------------------------------
void App::draw_victory(render::Renderer& r) const {
    r.set_layer(render::Renderer::Layer::Alpha);

    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 cy = VIRTUAL_H * 0.24f;
    const f32 t  = victory_time_;

    r.draw_text_centered({ cx, cy }, "VICTORY", 9.0f, COLOR_CORE_EDGE);
    r.draw_text_centered({ cx, cy + 110.0f },
                         "THEOREM STABILIZED",
                         2.6f, with_alpha(COLOR_CORE_INNER, 220));
    r.draw_text_centered({ cx, cy + 145.0f },
                         "THE RING WILL HOLD",
                         2.6f, with_alpha(COLOR_CORE_INNER, 220));

    // ---- Cascading room dots -----------------------------------------
    const int dots = game::ROOM_COUNT + 1;
    const f32 dot_y = VIRTUAL_H * 0.48f;
    for (int i = 0; i < dots; ++i) {
        const f32 x = cx + (f32(i) - f32(dots - 1) * 0.5f) * 28.0f;
        const f32 offset = std::sin(t * 2.0f + f32(i) * 0.4f) * 6.0f;
        r.draw_circle({ x, dot_y + offset }, 8.0f, COLOR_CORE_INNER, 14);
    }

    // ---- Station record block, revealed after 2 seconds --------------
    if (t > 2.0f) {
        const f32 reveal = clamp((t - 2.0f) / 0.6f, 0.0f, 1.0f);
        const u8  a = u8(reveal * 200.0f);

        const char* rec[] = {
            "> HALCYON STATION — FINAL STATE",
            "> RING INTEGRITY: STABLE",
            "> CREW ONBOARD: 0",
            "> OPERATIONAL DRONES: 1",
            "> THEOREM STATUS: COHERENT",
            "> LAST TRANSMISSION: \"IS THIS INTEGRITY?\"",
            "> REPLY: NONE",
        };
        constexpr int REC_N = sizeof(rec) / sizeof(rec[0]);

        const f32 rec_x = cx - 340.0f;
        const f32 rec_y0 = VIRTUAL_H * 0.60f;

        for (int i = 0; i < REC_N; ++i) {
            // Stagger each line by 0.2s.
            const f32 line_reveal = clamp(
                (t - 2.0f - f32(i) * 0.2f) / 0.4f, 0.0f, 1.0f);
            const u8 la = u8(line_reveal * 200.0f);
            if (la == 0) continue;
            r.draw_text({ rec_x, rec_y0 + f32(i) * 22.0f }, rec[i], 1.9f,
                        with_alpha(COLOR_CORE_EDGE, la));
        }
        (void)a;   // (reserved for future fade-in of the whole block)
    }

    // ---- Restart prompt ----------------------------------------------
    if (t > 3.5f) {
        const f32 pulse = 0.5f + 0.5f * std::sin(t * 4.0f);
        const f32 ring_y = VIRTUAL_H * 0.88f;
        const f32 ring_r = 22.0f + 4.0f * pulse;
        r.draw_ring({ cx, ring_y }, ring_r - 3.0f, ring_r + 3.0f,
                    with_alpha(COLOR_CORE_EDGE, u8(140 + 100 * pulse)), 32);
        r.draw_text_centered({ cx, ring_y + 45.0f }, "PRESS ANY KEY",
                             2.2f, with_alpha(COLOR_CORE_INNER, 220));
    }

    // ---- Additive bloom ----------------------------------------------
    r.set_layer(render::Renderer::Layer::Additive);
    r.draw_circle({ cx, cy + 40.0f }, 400.0f,
                  with_alpha(COLOR_CORE_EDGE, 18), 40);
}

void App::shutdown() {
    audio::shutdown();
    renderer_.shutdown();
    std::fprintf(stderr, "[incidence] shutdown after %llu frames\n",
                 static_cast<unsigned long long>(frame_count_));
    std::fflush(stderr);
}

} // namespace inc
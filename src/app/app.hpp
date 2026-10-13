#pragma once

#include "core/math.hpp"
#include "platform/platform.hpp"
#include "render/renderer.hpp"
#include "game/world.hpp"
#include "game/screen.hpp"
#include "game/log.hpp"
#include "game/quest.hpp"

namespace inc {

class App {
public:
    App();
    ~App();

    App(const App&)            = delete;
    App& operator=(const App&) = delete;

    void init();
    void frame();
    void shutdown();

    [[nodiscard]] bool running() const noexcept { return running_; }

private:
    void advance_simulation(double frame_dt);
    void step(f32 dt);
    void render();

    void draw_title(render::Renderer& r) const;
    void draw_game_over(render::Renderer& r) const;
    void draw_victory(render::Renderer& r) const;
    void draw_room_intro(render::Renderer& r) const;

    void draw_intro(render::Renderer& r) const;
    void draw_intro_background(render::Renderer& r,
                               int card_index, f32 card_t) const;

    void draw_character_sheet(render::Renderer& r) const;
    void draw_skill_tree(render::Renderer& r) const;
    void draw_equipment_panel(render::Renderer& r) const;
    void draw_log_overlay(render::Renderer& r) const;
    void draw_log_archive(render::Renderer& r) const;
    void draw_quest_log(render::Renderer& r) const;
    void draw_world_map(render::Renderer& r) const;

    void check_quests();

    bool   running_       = true;
    double accumulator_   = 0.0;
    double last_frame_ts_ = 0.0;
    u64    frame_count_   = 0;

    game::Screen  screen_       = game::Screen::Intro;
    f32           intro_time_   = 0.0f;
    f32           title_time_   = 0.0f;
    f32           victory_time_ = 0.0f;

    f32           room_intro_timer_ = -1.0f;
    int           last_room_shown_  = -1;
    bool          last_warden_state_ = false;

    bool          character_sheet_open_ = false;
    bool          skill_tree_open_      = false;
    bool          equipment_panel_open_ = false;
    bool          log_archive_open_     = false;
    bool          quest_log_open_       = false;
    bool          world_map_open_       = false;

    int           skill_cursor_branch_  = 0;
    int           skill_cursor_row_     = 0;
    int           equipment_slot_       = 0;
    int           archive_cursor_       = 0;
    int           map_cursor_           = 0;

    game::LogState    logs_;
    game::QuestState  quests_;

    render::Renderer  renderer_;
    game::World       world_;
};

} // namespace inc
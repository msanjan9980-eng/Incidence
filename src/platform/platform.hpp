#pragma once

#include <cstdint>
#include <string>

namespace inc::platform {

// ---------------------------------------------------------------------------
// A thin layer over sokol_app. Keeping this isolated means the rest of the
// codebase never includes a sokol header directly; if we ever swap to SDL or
// a console SDK, only this file changes.
// ---------------------------------------------------------------------------

enum class Key : std::uint16_t {
    Unknown = 0,
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Left, Right, Up, Down,
    Space, Enter, Escape, Tab,
    ShiftL, ShiftR, CtrlL, CtrlR, AltL, AltR,
    MouseLeft, MouseRight, MouseMiddle,
    GamepadA, GamepadB, GamepadX, GamepadY,
    GamepadLB, GamepadRB,
    GamepadDpadUp, GamepadDpadDown, GamepadDpadLeft, GamepadDpadRight,
    GamepadStart, GamepadSelect,
    Count
};

struct InputState {
    bool  key_down    [static_cast<int>(Key::Count)] = {};
    bool  key_pressed [static_cast<int>(Key::Count)] = {};  // this frame only
    bool  key_released[static_cast<int>(Key::Count)] = {};  // this frame only
    float mouse_x  = 0.0f;
    float mouse_y  = 0.0f;
    float mouse_dx = 0.0f;
    float mouse_dy = 0.0f;
    float gamepad_lx = 0.0f;   // -1..1
    float gamepad_ly = 0.0f;
    float gamepad_rx = 0.0f;
    float gamepad_ry = 0.0f;
};

struct WindowDesc {
    std::string title      = "INCIDENCE";
    int         width      = 1920;
    int         height     = 1080;
    bool        fullscreen = false;
    bool        vsync      = true;
    bool        resizable  = true;
};

// Window dimensions in physical pixels (what the OS sees).
[[nodiscard]] int   window_width();
[[nodiscard]] int   window_height();

// Frame scale factor for high-DPI displays (e.g. 2.0 on a Retina Mac).
[[nodiscard]] float dpi_scale();

// True if the app has been asked to close (window X, Escape on web, etc).
[[nodiscard]] bool  quit_requested();

// Milliseconds wall clock since process start. Monotonic.
[[nodiscard]] double time_seconds();

} // namespace inc::platform
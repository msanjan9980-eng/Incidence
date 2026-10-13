#pragma once

#include <cstdint>
#include <string>

namespace inc::platform {

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
    bool  key_pressed [static_cast<int>(Key::Count)] = {};
    bool  key_released[static_cast<int>(Key::Count)] = {};
    float mouse_x  = 0.0f;
    float mouse_y  = 0.0f;
    float mouse_dx = 0.0f;
    float mouse_dy = 0.0f;
    float gamepad_lx = 0.0f;
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

[[nodiscard]] int   window_width();
[[nodiscard]] int   window_height();
[[nodiscard]] float dpi_scale();
[[nodiscard]] bool  quit_requested();
[[nodiscard]] double time_seconds();

// NEW: read-only access to the current frame's input state.
[[nodiscard]] const InputState& platform_input();

// Clears the per-frame "key_pressed" and "key_released" edges. Must be
// called after the first simulation substep of a frame so subsequent
// substeps within the same frame do not see the same edge twice.
void consume_pressed();

} // namespace inc::platform
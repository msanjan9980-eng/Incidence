#include "platform/platform.hpp"

#include "sokol_app.h"
#include "sokol_time.h"

#include <cstring>

// ---------------------------------------------------------------------------
// This file owns input state and translates sokol_app events into our Key
// enum. It exposes three C-linkage bridge functions that main.cpp calls
// from sokol_app's callbacks.
// ---------------------------------------------------------------------------

namespace inc::platform {

InputState g_input{};
double     g_start_time = 0.0;
bool       g_quit_requested = false;

namespace {

Key translate_key(sapp_keycode k) {
    switch (k) {
        case SAPP_KEYCODE_A: return Key::A;
        case SAPP_KEYCODE_B: return Key::B;
        case SAPP_KEYCODE_C: return Key::C;
        case SAPP_KEYCODE_D: return Key::D;
        case SAPP_KEYCODE_E: return Key::E;
        case SAPP_KEYCODE_F: return Key::F;
        case SAPP_KEYCODE_G: return Key::G;
        case SAPP_KEYCODE_H: return Key::H;
        case SAPP_KEYCODE_I: return Key::I;
        case SAPP_KEYCODE_J: return Key::J;
        case SAPP_KEYCODE_K: return Key::K;
        case SAPP_KEYCODE_L: return Key::L;
        case SAPP_KEYCODE_M: return Key::M;
        case SAPP_KEYCODE_N: return Key::N;
        case SAPP_KEYCODE_O: return Key::O;
        case SAPP_KEYCODE_P: return Key::P;
        case SAPP_KEYCODE_Q: return Key::Q;
        case SAPP_KEYCODE_R: return Key::R;
        case SAPP_KEYCODE_S: return Key::S;
        case SAPP_KEYCODE_T: return Key::T;
        case SAPP_KEYCODE_U: return Key::U;
        case SAPP_KEYCODE_V: return Key::V;
        case SAPP_KEYCODE_W: return Key::W;
        case SAPP_KEYCODE_X: return Key::X;
        case SAPP_KEYCODE_Y: return Key::Y;
        case SAPP_KEYCODE_Z: return Key::Z;

        case SAPP_KEYCODE_0: return Key::Num0;
        case SAPP_KEYCODE_1: return Key::Num1;
        case SAPP_KEYCODE_2: return Key::Num2;
        case SAPP_KEYCODE_3: return Key::Num3;
        case SAPP_KEYCODE_4: return Key::Num4;
        case SAPP_KEYCODE_5: return Key::Num5;
        case SAPP_KEYCODE_6: return Key::Num6;
        case SAPP_KEYCODE_7: return Key::Num7;
        case SAPP_KEYCODE_8: return Key::Num8;
        case SAPP_KEYCODE_9: return Key::Num9;

        case SAPP_KEYCODE_LEFT:  return Key::Left;
        case SAPP_KEYCODE_RIGHT: return Key::Right;
        case SAPP_KEYCODE_UP:    return Key::Up;
        case SAPP_KEYCODE_DOWN:  return Key::Down;

        case SAPP_KEYCODE_SPACE:  return Key::Space;
        case SAPP_KEYCODE_ENTER:  return Key::Enter;
        case SAPP_KEYCODE_ESCAPE: return Key::Escape;
        case SAPP_KEYCODE_TAB:    return Key::Tab;

        case SAPP_KEYCODE_LEFT_SHIFT:    return Key::ShiftL;
        case SAPP_KEYCODE_RIGHT_SHIFT:   return Key::ShiftR;
        case SAPP_KEYCODE_LEFT_CONTROL:  return Key::CtrlL;
        case SAPP_KEYCODE_RIGHT_CONTROL: return Key::CtrlR;
        case SAPP_KEYCODE_LEFT_ALT:      return Key::AltL;
        case SAPP_KEYCODE_RIGHT_ALT:     return Key::AltR;

        default: return Key::Unknown;
    }
}

} // anonymous namespace

const InputState& platform_input() { return g_input; }

int   window_width()   { return sapp_width();  }
int   window_height()  { return sapp_height(); }
float dpi_scale()      { return sapp_dpi_scale(); }
bool  quit_requested() { return g_quit_requested; }

double time_seconds() {
    return stm_sec(stm_now()) - g_start_time;
}

void consume_pressed() {
    std::memset(g_input.key_pressed,  0, sizeof(g_input.key_pressed));
    std::memset(g_input.key_released, 0, sizeof(g_input.key_released));
}

void handle_event_internal(const sapp_event* ev) {
    switch (ev->type) {
        case SAPP_EVENTTYPE_KEY_DOWN: {
            if (ev->key_repeat) break;
            const Key k = translate_key(ev->key_code);
            if (k != Key::Unknown) {
                const int idx = static_cast<int>(k);
                if (!g_input.key_down[idx]) {
                    g_input.key_pressed[idx] = true;
                }
                g_input.key_down[idx] = true;
            }
            break;
        }
        case SAPP_EVENTTYPE_KEY_UP: {
            const Key k = translate_key(ev->key_code);
            if (k != Key::Unknown) {
                const int idx = static_cast<int>(k);
                g_input.key_down[idx]     = false;
                g_input.key_released[idx] = true;
            }
            break;
        }
        case SAPP_EVENTTYPE_MOUSE_MOVE: {
            g_input.mouse_dx = ev->mouse_x - g_input.mouse_x;
            g_input.mouse_dy = ev->mouse_y - g_input.mouse_y;
            g_input.mouse_x  = ev->mouse_x;
            g_input.mouse_y  = ev->mouse_y;
            break;
        }
        case SAPP_EVENTTYPE_QUIT_REQUESTED: {
            g_quit_requested = true;
            break;
        }
        default:
            break;
    }
}

void end_frame_internal() {
    std::memset(g_input.key_pressed,  0, sizeof(g_input.key_pressed));
    std::memset(g_input.key_released, 0, sizeof(g_input.key_released));
    g_input.mouse_dx = 0.0f;
    g_input.mouse_dy = 0.0f;
}

void init_internal() {
    g_start_time = stm_sec(stm_now());
    g_quit_requested = false;
}

} // namespace inc::platform

// ---------------------------------------------------------------------------
// C-linkage bridge functions called by main.cpp from sokol_app callbacks.
// ---------------------------------------------------------------------------
extern "C" {

void platform_init_cb() {
    inc::platform::init_internal();
}

void platform_handle_event_cb(const sapp_event* ev) {
    inc::platform::handle_event_internal(ev);
}

void platform_end_frame_cb() {
    inc::platform::end_frame_internal();
}

} // extern "C"
#include "app/app.hpp"
#include "platform/platform.hpp"

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#endif
#include "sokol_app.h"
#include "sokol_time.h"
#include "sokol_log.h"

#include <cstdio>

namespace {

inc::App g_app;

} // namespace

extern "C" {
void platform_init_cb();
void platform_handle_event_cb(const sapp_event* ev);
void platform_end_frame_cb();
}

// ---------------------------------------------------------------------------
// Sokol log callback. Every error/warning sokol_gfx or sokol_app emits will
// come through here. This is how we find out about shader compilation
// failures, missing uniforms, and API misuse.
// ---------------------------------------------------------------------------
static void on_sokol_log(const char* tag,
                         uint32_t log_level,
                         uint32_t log_item_id,
                         const char* message_or_null,
                         uint32_t line_nr,
                         const char* filename_or_null,
                         void* user_data)
{
    (void)log_level;
    (void)log_item_id;
    (void)user_data;

    const char* file = filename_or_null ? filename_or_null : "(null)";
    const char* msg  = message_or_null  ? message_or_null  : "(null)";
    const char* lvl  = "?";

    switch (log_level) {
        case 0: lvl = "FATAL";   break;
        case 1: lvl = "ERROR";   break;
        case 2: lvl = "WARN";    break;
        case 3: lvl = "INFO";    break;
        default: break;
    }

    std::fprintf(stderr, "[sokol:%s:%s] %s (%s:%u)\n",
                 tag, lvl, msg, file, line_nr);
    std::fflush(stderr);
}

#if defined(_WIN32)
static void apply_borderless_fullscreen() {
    HWND hwnd = FindWindowA(nullptr, "INCIDENCE");
    if (!hwnd) {
        std::fprintf(stderr, "[incidence] fullscreen: window not found\n");
        return;
    }
    // Strip decorations
    LONG style = GetWindowLongA(hwnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX
             | WS_MAXIMIZEBOX | WS_SYSMENU);
    SetWindowLongA(hwnd, GWL_STYLE, style);

    // Cover the primary monitor
    HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoA(hmon, &mi);
    const int mx = mi.rcMonitor.left;
    const int my = mi.rcMonitor.top;
    const int mw = mi.rcMonitor.right - mi.rcMonitor.left;
    const int mh = mi.rcMonitor.bottom - mi.rcMonitor.top;

    // Borderless fullscreen, no permanent topmost.
    SetWindowPos(hwnd, HWND_TOP, mx, my, mw, mh,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    // One-shot focus steal so the window appears over other apps at launch.
    SetForegroundWindow(hwnd);
    BringWindowToTop(hwnd);
    std::fprintf(stderr, "[incidence] fullscreen: %dx%d at (%d,%d)\n",
                 mw, mh, mx, my);
}
#endif

static void on_init() {
    std::fprintf(stderr, "[incidence] on_init called\n"); std::fflush(stderr);
    stm_setup();
    platform_init_cb();
    g_app.init();
    std::fprintf(stderr, "[incidence] on_init done\n"); std::fflush(stderr);
}

static void on_frame() {
#if defined(_WIN32)
    static bool s_fullscreen_done = false;
    if (!s_fullscreen_done) {
        s_fullscreen_done = true;
        apply_borderless_fullscreen();
    }
#endif
    g_app.frame();
    platform_end_frame_cb();
    if (!g_app.running()) sapp_quit();
}

static void on_cleanup() {
    std::fprintf(stderr, "[incidence] on_cleanup called\n"); std::fflush(stderr);
    g_app.shutdown();
}

static void on_event(const sapp_event* ev) { platform_handle_event_cb(ev); }

sapp_desc sokol_main(int /*argc*/, char* /*argv*/[]) {
    sapp_desc desc{};
    desc.init_cb      = on_init;
    desc.frame_cb     = on_frame;
    desc.cleanup_cb   = on_cleanup;
    desc.event_cb     = on_event;
    desc.width        = 1920;
    desc.height       = 1080;
    desc.window_title = "INCIDENCE";
    desc.high_dpi     = true;
    desc.logger.func  = on_sokol_log;   // <-- route all sokol messages to us
    return desc;
}
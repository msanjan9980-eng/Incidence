#include "app/app.hpp"
#include "platform/platform.hpp"

#include "sokol_app.h"
#include "sokol_time.h"

#include <cstdio>

// ---------------------------------------------------------------------------
// sokol_app callbacks. Sokol owns main(); we provide sokol_main() and the
// callbacks, and sokol drives everything. No sapp_run() call is needed.
// ---------------------------------------------------------------------------

namespace {

inc::App g_app;

} // namespace

extern "C" {
void platform_init_cb();
void platform_handle_event_cb(const sapp_event* ev);
void platform_end_frame_cb();
}

static void on_init() {
    std::fprintf(stderr, "[incidence] on_init called\n"); std::fflush(stderr);
    stm_setup();
    platform_init_cb();
    g_app.init();
    std::fprintf(stderr, "[incidence] on_init done\n"); std::fflush(stderr);
}

static void on_frame() {
    g_app.frame();
    platform_end_frame_cb();
    if (!g_app.running()) sapp_quit();
}

static void on_cleanup() {
    std::fprintf(stderr, "[incidence] on_cleanup called\n"); std::fflush(stderr);
    g_app.shutdown();
}

static void on_event(const sapp_event* ev) { platform_handle_event_cb(ev); }

// sokol_main is called by sokol's own entry point. Do NOT write your own
// main() — sokol provides it, and duplicates cause a linker collision.
sapp_desc sokol_main(int /*argc*/, char* /*argv*/[]) {
    sapp_desc desc{};
    desc.init_cb      = on_init;
    desc.frame_cb     = on_frame;
    desc.cleanup_cb   = on_cleanup;
    desc.event_cb     = on_event;
    desc.width        = 1280;
    desc.height       = 720;
    desc.window_title = "INCIDENCE";
    desc.high_dpi     = true;
    return desc;
}
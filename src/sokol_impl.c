/* ---------------------------------------------------------------------------
 * sokol_impl.c
 *
 * The single translation unit where sokol's implementation lives.
 * Every other file includes the sokol headers WITHOUT SOKOL_IMPL.
 *
 * SOKOL_GLCORE: use desktop OpenGL 4.x on Windows. This lets us write GLSL
 * shaders directly (D3D11 would require HLSL or sokol-shdc cross-compilation).
 *
 * SOKOL_GLCORE works everywhere the D3D11 backend does. We can switch to
 * D3D11 + sokol-shdc later when we need to squeeze the last bit of
 * performance out of the Windows build.
 * ------------------------------------------------------------------------- */

#if defined(_WIN32)
  #define SOKOL_GLCORE
#endif

/* --- sokol_app: window, input, lifecycle --------------------------------- */
#define SOKOL_APP_IMPL
#include "sokol_app.h"

/* --- sokol_gfx: cross-platform GPU --------------------------------------- */
#define SOKOL_GFX_IMPL
#include "sokol_gfx.h"

/* --- sokol_glue: connects sokol_app context to sokol_gfx ---------------- */
#define SOKOL_GLUE_IMPL
#include "sokol_glue.h"

/* --- sokol_time: monotonic clock ---------------------------------------- */
#define SOKOL_TIME_IMPL
#include "sokol_time.h"
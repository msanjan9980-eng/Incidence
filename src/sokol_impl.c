/* ---------------------------------------------------------------------------
 * sokol_impl.c
 *
 * The single translation unit where sokol's implementation lives.
 * Every other file includes the sokol headers WITHOUT SOKOL_IMPL.
 *
 * Backend selection: on Windows, use the D3D11 graphics API.
 * ------------------------------------------------------------------------- */

#if defined(_WIN32)
  #define SOKOL_D3D11
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
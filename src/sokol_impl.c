/* ---------------------------------------------------------------------------
 * sokol_impl.c
 *
 * The single translation unit where sokol's implementation lives.
 * ------------------------------------------------------------------------- */

#if defined(_WIN32)
  #define SOKOL_GLCORE
#endif

/* --- sokol_app ----------------------------------------------------------- */
#define SOKOL_APP_IMPL
#include "sokol_app.h"

/* --- sokol_gfx ----------------------------------------------------------- */
#define SOKOL_GFX_IMPL
#include "sokol_gfx.h"

/* --- sokol_glue ---------------------------------------------------------- */
#define SOKOL_GLUE_IMPL
#include "sokol_glue.h"

/* --- sokol_time ---------------------------------------------------------- */
#define SOKOL_TIME_IMPL
#include "sokol_time.h"

/* --- sokol_audio --------------------------------------------------------- */
#define SOKOL_AUDIO_IMPL
#include "sokol_audio.h"
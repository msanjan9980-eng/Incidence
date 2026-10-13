#pragma once

#include "core/math.hpp"

namespace inc::audio {

// ---------------------------------------------------------------------------
// Audio — a tiny procedural synth.
//
// No sample assets. Every sound is generated from sine waves and simple
// envelopes, mixed on the fly by sokol_audio's stereo callback. This keeps
// the binary at a few hundred kilobytes and lets us pitch-shift freely.
//
// The most important sound is the shield impact: its pitch is derived from
// the bullet's incidence angle. Glancing blows ring high, head-on blows
// thud low. Players internalize this mapping and start *hearing* the
// geometry before they see it.
// ---------------------------------------------------------------------------

// Currently playing voices. Fixed pool; oldest voice is stolen on overflow.
struct Voice {
    bool  active      = false;
    f32   phase       = 0.0f;   // current sample phase [0, 1)
    f32   freq        = 440.0f; // Hz
    f32   amp         = 0.0f;   // current amplitude
    f32   decay_rate  = 3.0f;   // amplitude multiplier per second
    f32   pan         = 0.0f;   // -1 left, +1 right
    int   wave        = 0;      // 0 = sine, 1 = triangle, 2 = noise
};

static constexpr int MAX_VOICES = 32;

void init();
void shutdown();

// --- Firing events -------------------------------------------------------

// A turret fired. Frequency is fixed per enemy type (see callers).
void play_enemy_fire();

// A bullet bounced off the shield. `incidence_deg` is the angle between
// the bullet's incoming direction and the shield facet normal, in [0, 90].
// Higher angle -> higher pitch.
void play_shield_reflect(f32 incidence_deg);

// A bullet hit the drone core. Low thud.
void play_player_hit();

// A turret died. Rising chime.
void play_enemy_death();

// Warden phase transition. Deep impact.
void play_warden_phase();

// Warden defeated. Fanfare.
void play_victory();

// --- Per-frame -----------------------------------------------------------

// Advance all voices by `dt` seconds. Called from App::step.
void tick(f32 dt);

} // namespace inc::audio
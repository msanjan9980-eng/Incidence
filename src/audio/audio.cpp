#include "audio/audio.hpp"

#include "sokol_audio.h"

#include <cmath>

namespace inc::audio {

namespace {

constexpr f32 SAMPLE_RATE = 44100.0f;

Voice   g_voices[MAX_VOICES];
int     g_next_voice = 0;

// ---------------------------------------------------------------------------
// Voice allocation
// ---------------------------------------------------------------------------
Voice& alloc_voice() {
    // Prefer an inactive slot; otherwise steal the next one round-robin.
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (!g_voices[i].active) return g_voices[i];
    }
    Voice& v = g_voices[g_next_voice];
    g_next_voice = (g_next_voice + 1) % MAX_VOICES;
    return v;
}

void emit(f32 freq, f32 amp, f32 decay, f32 pan, int wave) {
    Voice& v = alloc_voice();
    v.active     = true;
    v.phase      = 0.0f;
    v.freq       = freq;
    v.amp        = amp;
    v.decay_rate = decay;
    v.pan        = pan;
    v.wave       = wave;
}

// ---------------------------------------------------------------------------
// Waveform generation
// ---------------------------------------------------------------------------
f32 sample_wave(int wave, f32 phase) {
    switch (wave) {
        case 0: // sine
            return std::sin(phase * TAU);
        case 1: // triangle: cheap, brighter
            return 4.0f * std::fabs(phase - 0.5f) - 1.0f;
        case 2: // white-ish noise (hash)
            {
                const u32 h = u32(phase * 999983.0f);
                return (f32(h & 0xFFFFu) / 32768.0f) - 1.0f;
            }
        default: return 0.0f;
    }
}

// ---------------------------------------------------------------------------
// sokol_audio callback — fills `num_frames` stereo sample pairs.
// ---------------------------------------------------------------------------
void audio_cb(f32* buffer, int num_frames, int /*num_channels*/) {
    const f32 dt = 1.0f / SAMPLE_RATE;

    for (int i = 0; i < num_frames; ++i) {
        f32 l = 0.0f, r = 0.0f;

        for (auto& v : g_voices) {
            if (!v.active) continue;

            const f32 s = sample_wave(v.wave, v.phase) * v.amp;

            const f32 pan_l = (1.0f - v.pan) * 0.5f;
            const f32 pan_r = (1.0f + v.pan) * 0.5f;
            l += s * pan_l;
            r += s * pan_r;

            // Advance phase.
            v.phase += v.freq * dt;
            if (v.phase >= 1.0f) v.phase -= 1.0f;

            // Decay amplitude.
            v.amp *= std::exp(-v.decay_rate * dt);
            if (v.amp < 0.0005f) {
                v.active = false;
            }
        }

        // Soft clip to prevent harsh digital distortion when many voices sum.
        auto soft = [](f32 x) {
            if (x >  1.0f) return  1.0f - 1.0f / (x + 1.0f);
            if (x < -1.0f) return -1.0f + 1.0f / (-x + 1.0f);
            return x;
        };

        buffer[i * 2 + 0] = soft(l * 0.5f);
        buffer[i * 2 + 1] = soft(r * 0.5f);
    }
}

} // anonymous namespace

void init() {
    for (auto& v : g_voices) { v = Voice{}; }

    saudio_desc desc{};
    desc.sample_rate    = int(SAMPLE_RATE);
    desc.num_channels   = 2;
    desc.stream_cb      = audio_cb;
    desc.buffer_frames  = 512;      // ~11 ms latency
    saudio_setup(&desc);
}

void shutdown() {
    saudio_shutdown();
}

// ---------------------------------------------------------------------------
// Sound primitives
// ---------------------------------------------------------------------------

void play_enemy_fire() {
    // Short click, low pitch. Two-tone: fundamental + harmonic.
    emit(220.0f, 0.16f,  18.0f, -0.3f, 0);
    emit(440.0f, 0.08f,  22.0f,  0.3f, 1);
}

void play_shield_reflect(f32 incidence_deg) {
    // Map incidence angle [0, 90] to frequency [300, 2400] Hz, exponentially.
    //   f = 300 * (8 ^ (angle / 90))
    const f32 a = clamp(incidence_deg, 0.0f, 90.0f);
    const f32 f = 300.0f * std::pow(8.0f, a / 90.0f);

    emit(f,          0.22f, 12.0f, 0.0f, 0);
    emit(f * 1.5f,   0.10f, 16.0f, 0.0f, 1);
}

void play_player_hit() {
    emit(110.0f, 0.30f, 4.5f, 0.0f, 2);
    emit( 55.0f, 0.35f, 3.2f, 0.0f, 0);
}

void play_enemy_death() {
    // Rising chime: three notes stacked.
    emit(440.0f, 0.18f, 6.0f, 0.0f, 0);
    emit(660.0f, 0.14f, 6.0f, 0.0f, 0);
    emit(880.0f, 0.10f, 5.0f, 0.0f, 0);
}

void play_warden_phase() {
    emit( 80.0f, 0.40f, 1.8f, 0.0f, 0);
    emit(120.0f, 0.30f, 2.5f, 0.0f, 1);
    emit(240.0f, 0.18f, 4.0f, 0.0f, 0);
}

void play_victory() {
    // Four-note ascending arpeggio.
    emit(523.0f, 0.20f, 2.5f, 0.0f, 0);
    emit(659.0f, 0.20f, 2.5f, 0.0f, 0);
    emit(784.0f, 0.20f, 2.5f, 0.0f, 0);
    emit(1047.0f, 0.25f, 1.8f, 0.0f, 0);
}

void tick(f32 /*dt*/) {
    // Voices advance inside the audio callback (which has its own fixed dt
    // tied to the audio sample rate). This function exists so App can hook
    // the audio system into its tick if it ever needs to; currently a no-op.
}

} // namespace inc::audio
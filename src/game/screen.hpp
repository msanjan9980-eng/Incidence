#pragma once

namespace inc::game {

// ---------------------------------------------------------------------------
// Screen — the top-level UI state.
//
//   Intro     : title-card sequence that establishes the story (skippable).
//   Title     : attract screen, "press any key to begin".
//   Playing   : normal gameplay (rooms + warden).
//   Victory   : after Warden defeated, offers restart.
//   GameOver  : after 3 HP lost, offers restart.
// ---------------------------------------------------------------------------
enum class Screen {
    Intro,
    Title,
    Playing,
    Victory,
    GameOver,
};

} // namespace inc::game
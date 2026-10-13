#pragma once

#include "game/character.hpp"
#include <string>

namespace inc::game {

// ---------------------------------------------------------------------------
// SaveData — everything that gets serialized to disk.
// ---------------------------------------------------------------------------
struct SaveData {
    Character character;
    int       current_room = 0;
    bool      has_save     = false;
};

// Save `ch` and `current_room` to a text file. Returns true on success.
// Format is a flat JSON-ish object — human readable, easy to hand-edit.
bool save_game(const std::string& path, const Character& ch, int current_room);

// Load a save file. Returns true if a valid save was found and parsed.
// On failure, `out.has_save` is false and the character is left untouched.
bool load_game(const std::string& path, SaveData& out);

} // namespace inc::game
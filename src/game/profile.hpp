#pragma once

#include <string>
#include "core/math.hpp"
#include "game/character.hpp"
#include "game/save.hpp"

namespace inc::game {

struct ProfileInfo {
    char name[32];
    bool has_save;
    int  level;
    int  room;
};

inline constexpr int PROFILE_MAX_NAME = 16;
inline constexpr int PROFILE_MAX      = 16;

void profiles_init();
[[nodiscard]] bool profile_name_valid(const char* name);
int  profiles_list(ProfileInfo* out, int max);
bool profile_create(const char* name);
bool profile_delete(const char* name);
bool profile_set_active(const char* name);
bool profile_get_active(char* out, int out_size);
bool profile_save_active(const Character& ch, int room, u16 visited);
bool profile_load_active(SaveData& out);

} // namespace inc::game
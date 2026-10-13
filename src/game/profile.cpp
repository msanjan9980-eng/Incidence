#include "game/profile.hpp"
#include <cstdio>
#include <cstring>

namespace inc::game {

namespace {

constexpr const char* CFG_PATH    = "profiles.cfg";
constexpr const char* LEGACY_PATH = "save.sav";
constexpr const char* LEGACY_NAME = "LEGACY";

void safe_copy(char* dst, const char* src, int cap) {
    if (cap <= 0) return;
    int i = 0;
    while (src[i] && i < cap - 1) { dst[i] = src[i]; ++i; }
    dst[i] = 0;
}

std::string save_path_for(const char* name) {
    return std::string(name) + ".sav";
}

bool file_exists(const std::string& path) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::fclose(f);
    return true;
}

bool read_line(std::FILE* f, char* out, int cap) {
    int c = 0; int i = 0; bool any = false;
    while ((c = std::fgetc(f)) != EOF && c != 10) {
        any = true;
        if (i < cap - 1) out[i++] = (char)c;
    }
    out[i] = 0;
    if (i > 0 && out[i-1] == 13) out[i-1] = 0;
    return any || (c != EOF);
}

int read_all_names(char names[][PROFILE_MAX_NAME + 4], int max) {
    std::FILE* f = std::fopen(CFG_PATH, "r");
    if (!f) return 0;
    char line[128];
    int n = 0;
    bool first = true;
    while (read_line(f, line, sizeof(line))) {
        if (first) { first = false; continue; }
        if (!line[0]) continue;
        if (n >= max) break;
        safe_copy(names[n], line, PROFILE_MAX_NAME + 4);
        ++n;
    }
    std::fclose(f);
    return n;
}

bool read_active(char* out, int out_size) {
    std::FILE* f = std::fopen(CFG_PATH, "r");
    if (!f) { if (out_size > 0) out[0] = 0; return false; }
    char line[128];
    bool ok = false;
    if (read_line(f, line, sizeof(line)) && std::strncmp(line, "active=", 7) == 0) {
        safe_copy(out, line + 7, out_size);
        ok = (out[0] != 0);
    } else {
        if (out_size > 0) out[0] = 0;
    }
    std::fclose(f);
    return ok;
}

} // anon

void profiles_init() {
    if (file_exists(CFG_PATH)) return;
    std::FILE* cfg = std::fopen(CFG_PATH, "w");
    if (!cfg) return;
    if (file_exists(LEGACY_PATH)) {
        std::rename(LEGACY_PATH, "LEGACY.sav");
        std::fprintf(cfg, "active=%s\n", LEGACY_NAME);
        std::fprintf(cfg, "%s\n", LEGACY_NAME);
    } else {
        std::fprintf(cfg, "active=\n");
    }
    std::fclose(cfg);
}

bool profile_name_valid(const char* name) {
    if (!name || !name[0]) return false;
    int n = 0;
    for (; name[n]; ++n) {
        if (n >= PROFILE_MAX_NAME) return false;
        const unsigned char c = (unsigned char)name[n];
        if (c < 33 || c > 126) return false;
        if (c == '/' || c == '\\' || c == ':' || c == '*' ||
            c == '?' || c == '"' || c == '<' || c == '>' ||
            c == '|' || c == '.') return false;
    }
    if (std::strcmp(name, "active") == 0) return false;
    return n > 0;
}

int profiles_list(ProfileInfo* out, int max) {
    profiles_init();
    char names[PROFILE_MAX][PROFILE_MAX_NAME + 4];
    const int n = read_all_names(names, PROFILE_MAX);
    const int lim = (n < max) ? n : max;
    for (int i = 0; i < lim; ++i) {
        safe_copy(out[i].name, names[i], sizeof(out[i].name));
        out[i].has_save = false;
        out[i].level = 1;
        out[i].room  = 0;
        const std::string path = save_path_for(names[i]);
        if (file_exists(path)) {
            SaveData sd;
            if (load_game(path, sd)) {
                out[i].has_save = true;
                out[i].level = sd.character.level;
                out[i].room  = sd.current_room;
            }
        }
    }
    return lim;
}

bool profile_create(const char* name) {
    if (!profile_name_valid(name)) return false;
    profiles_init();
    char names[PROFILE_MAX][PROFILE_MAX_NAME + 4];
    const int n = read_all_names(names, PROFILE_MAX);
    for (int i = 0; i < n; ++i) {
        if (std::strcmp(names[i], name) == 0) return false;
    }
    std::FILE* f = std::fopen(CFG_PATH, "a");
    if (!f) return false;
    std::fprintf(f, "%s\n", name);
    std::fclose(f);
    std::remove(save_path_for(name).c_str());
    return true;
}

bool profile_delete(const char* name) {
    profiles_init();
    char names[PROFILE_MAX][PROFILE_MAX_NAME + 4];
    const int n = read_all_names(names, PROFILE_MAX);
    if (n == 0) return false;

    char active[PROFILE_MAX_NAME + 4] = {0};
    read_active(active, sizeof(active));
    const bool was_active = (std::strcmp(active, name) == 0);

    std::FILE* f = std::fopen(CFG_PATH, "w");
    if (!f) return false;
    if (was_active) std::fprintf(f, "active=\n");
    else            std::fprintf(f, "active=%s\n", active);
    for (int i = 0; i < n; ++i) {
        if (std::strcmp(names[i], name) == 0) continue;
        std::fprintf(f, "%s\n", names[i]);
    }
    std::fclose(f);
    std::remove(save_path_for(name).c_str());
    return true;
}

bool profile_set_active(const char* name) {
    profiles_init();
    char names[PROFILE_MAX][PROFILE_MAX_NAME + 4];
    const int n = read_all_names(names, PROFILE_MAX);
    bool found = false;
    for (int i = 0; i < n; ++i) {
        if (std::strcmp(names[i], name) == 0) { found = true; break; }
    }
    if (!found) return false;
    std::FILE* f = std::fopen(CFG_PATH, "w");
    if (!f) return false;
    std::fprintf(f, "active=%s\n", name);
    for (int i = 0; i < n; ++i) std::fprintf(f, "%s\n", names[i]);
    std::fclose(f);
    return true;
}

bool profile_get_active(char* out, int out_size) {
    profiles_init();
    return read_active(out, out_size);
}

bool profile_save_active(const Character& ch, int room, u16 visited) {
    char name[PROFILE_MAX_NAME + 4] = {0};
    if (!profile_get_active(name, sizeof(name))) return false;
    return save_game(save_path_for(name), ch, room, visited);
}

bool profile_load_active(SaveData& out) {
    char name[PROFILE_MAX_NAME + 4] = {0};
    if (!profile_get_active(name, sizeof(name))) return false;
    return load_game(save_path_for(name), out);
}

} // namespace inc::game
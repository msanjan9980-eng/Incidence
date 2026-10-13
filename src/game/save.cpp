#include "game/save.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace inc::game {

bool save_game(const std::string& path, const Character& ch, int current_room, u16 rooms_visited) {
    FILE* f = std::fopen(path.c_str(), "w");
    if (!f) return false;

    std::fprintf(f,
        "{\n"
        "  \"v\": 1,\n"
        "  \"lv\": %d,\n"
        "  \"xp\": %d,\n"
        "  \"pts\": %d,\n"
        "  \"skp\": %d,\n"
        "  \"arc\": %d,\n"
        "  \"foc\": %d,\n"
        "  \"chg\": %d,\n"
        "  \"drf\": %d,\n"
        "  \"skills\": %d,\n"
        "  \"eqf\": %d,\n"
        "  \"eqc\": %d,\n"
        "  \"eqh\": %d,\n"
        "  \"inv\": %d,\n"
        "  \"room\": %d,\n"
        "  \"rvis\": %d\n"
        "}\n",
        ch.level,
        ch.xp,
        ch.stat_points,
        ch.skill_points,
        ch.stats.arc,
        ch.stats.focus,
        ch.stats.charge,
        ch.stats.drift,
        int(ch.learned_skills),
        int(ch.equipped.facet),
        int(ch.equipped.core),
        int(ch.equipped.hull),
        int(ch.inventory.unlocked),
        current_room,
        unsigned(rooms_visited));

    std::fclose(f);
    return true;
}

namespace {

bool find_int(const char* text, const char* key, int& out) {
    const size_t klen = std::strlen(key);
    const char* p = text;
    while ((p = std::strstr(p, key)) != nullptr) {
        p += klen;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p != ':') continue;
        ++p;
        while (*p == ' ' || *p == '\t') ++p;

        char* end = nullptr;
        const long v = std::strtol(p, &end, 10);
        if (end == p) { continue; }
        out = int(v);
        return true;
    }
    return false;
}

} // anonymous namespace

bool load_game(const std::string& path, SaveData& out) {
    out.has_save = false;

    FILE* f = std::fopen(path.c_str(), "r");
    if (!f) return false;

    char buf[4096];
    const size_t n = std::fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    std::fclose(f);

    int v = 0;
    if (!find_int(buf, "\"v\"", v))  return false;
    if (v != 1)                       return false;

    int lv = 1, xp = 0, pts = 0, skp = 0;
    int arc = 3, foc = 3, chg = 3, drf = 3, room = 0;
    int skills = 0;
    int eqf = 0, eqc = 0, eqh = 0, inv = 73;

    find_int(buf, "\"lv\"",  lv);
    find_int(buf, "\"xp\"",  xp);
    find_int(buf, "\"pts\"", pts);
    find_int(buf, "\"skp\"", skp);
    find_int(buf, "\"arc\"", arc);
    find_int(buf, "\"foc\"", foc);
    find_int(buf, "\"chg\"", chg);
    find_int(buf, "\"drf\"", drf);
    find_int(buf, "\"skills\"", skills);
    find_int(buf, "\"eqf\"", eqf);
    find_int(buf, "\"eqc\"", eqc);
    find_int(buf, "\"eqh\"", eqh);
    find_int(buf, "\"inv\"", inv);
    find_int(buf, "\"room\"", room);

    if (lv  < LEVEL_MIN) lv  = LEVEL_MIN;
    if (lv  > LEVEL_MAX) lv  = LEVEL_MAX;
    if (arc < STAT_MIN) arc = STAT_MIN;
    if (arc > STAT_MAX) arc = STAT_MAX;
    if (foc < STAT_MIN) foc = STAT_MIN;
    if (foc > STAT_MAX) foc = STAT_MAX;
    if (chg < STAT_MIN) chg = STAT_MIN;
    if (chg > STAT_MAX) chg = STAT_MAX;
    if (drf < STAT_MIN) drf = STAT_MIN;
    if (drf > STAT_MAX) drf = STAT_MAX;

    if (eqf < 0 || eqf >= int(FacetId::Count)) eqf = 0;
    if (eqc < 0 || eqc >= int(CoreId::Count))  eqc = 0;
    if (eqh < 0 || eqh >= int(HullId::Count))  eqh = 0;
    if (inv < 0) inv = 73;
    inv &= 0x01FF;

    {
        int rvis = 0;
        find_int(buf, "\"rvis\"", rvis);
        out.rooms_visited = u16(rvis & 0xFFFF);
    }

    out.character.level            = lv;
    out.character.xp               = xp;
    out.character.stat_points      = pts;
    out.character.skill_points     = skp;
    out.character.stats.arc        = arc;
    out.character.stats.focus      = foc;
    out.character.stats.charge     = chg;
    out.character.stats.drift      = drf;
    out.character.learned_skills   = u16(skills & 0x7FFF);
    out.character.equipped.facet   = FacetId(eqf);
    out.character.equipped.core    = CoreId(eqc);
    out.character.equipped.hull    = HullId(eqh);
    out.character.inventory.unlocked = u16(inv);
    out.current_room               = room;
    out.has_save                   = true;
    return true;
}

} // namespace inc::game
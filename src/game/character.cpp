#include "game/character.hpp"

namespace inc::game {

f32 Character::shield_span_rad() const noexcept {
    const f32 deg = 100.0f + f32(stats.arc) * 7.5f;
    return deg_to_rad(deg);
}

f32 Character::spin_speed() const noexcept {
    return 2.5f + f32(stats.focus) * 0.2f;
}

f32 Character::reflect_multiplier() const noexcept {
    return 1.0f + f32(stats.charge) * 0.1f;
}

f32 Character::dash_impulse() const noexcept {
    return 400.0f + f32(stats.drift) * 40.0f;
}

f32 Character::dash_cooldown() const noexcept {
    return 1.0f - f32(stats.drift) * 0.03f;
}

int Character::xp_to_next(int lv) noexcept {
    if (lv < LEVEL_MIN) lv = LEVEL_MIN;
    if (lv >= LEVEL_MAX) return 0;
    const int n = lv - 1;
    return 100 + 50 * n + 25 * n * n;
}

f32 Character::xp_progress() const noexcept {
    const int need = xp_to_next();
    if (need <= 0) return 1.0f;
    return clamp(f32(xp) / f32(need), 0.0f, 1.0f);
}

int Character::add_xp(int amount) {
    if (amount <= 0) return 0;
    if (level >= LEVEL_MAX) return 0;

    xp += amount;
    int gained = 0;
    while (level < LEVEL_MAX) {
        const int need = xp_to_next();
        if (need <= 0) break;
        if (xp < need) break;
        xp -= need;
        ++level;
        ++stat_points;
        ++skill_points;
        ++gained;
    }
    if (level >= LEVEL_MAX) xp = 0;
    return gained;
}

bool Character::spend_point(int index) {
    if (stat_points <= 0) return false;

    int* target = nullptr;
    switch (index) {
        case 0: target = &stats.arc;    break;
        case 1: target = &stats.focus;  break;
        case 2: target = &stats.charge; break;
        case 3: target = &stats.drift;  break;
        default: return false;
    }
    if (*target >= STAT_MAX) return false;
    ++(*target);
    --stat_points;
    return true;
}

} // namespace inc::game
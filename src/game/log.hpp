#pragma once

#include "core/math.hpp"

namespace inc::game {

struct LogEntry {
    int         day;
    const char* body;
    const char* trigger;
};

inline constexpr int LOG_COUNT = 12;

const LogEntry& log_at(int index);

struct LogState {
    u16  seen         = 0;
    int  active_index = -1;
    f32  char_timer   = 0.0f;
    int  chars_shown  = 0;
    bool fully_shown  = false;

    [[nodiscard]] bool has_seen(int i) const noexcept {
        return (seen & (1u << i)) != 0;
    }
    void mark_seen(int i) noexcept {
        if (i >= 0 && i < LOG_COUNT) seen |= u16(1u << i);
    }
    [[nodiscard]] int seen_count() const noexcept {
        int c = 0;
        for (int i = 0; i < LOG_COUNT; ++i) if (has_seen(i)) ++c;
        return c;
    }
    void begin(int i) noexcept {
        active_index = i;
        char_timer   = 0.0f;
        chars_shown  = 0;
        fully_shown  = false;
    }
    void end() noexcept { active_index = -1; }
    [[nodiscard]] bool playing() const noexcept { return active_index >= 0; }
};

} // namespace inc::game
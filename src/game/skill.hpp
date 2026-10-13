#pragma once

#include "core/math.hpp"

namespace inc::game {

// ---------------------------------------------------------------------------
// SkillId - one for each node in the skill tree.
//
// Nodes are grouped in three branches of five. Within a branch, each node
// requires the previous one plus a minimum level. The player earns one skill
// point per level, so by the level cap (20) they can max one full branch
// (5 nodes) and most of a second.
// ---------------------------------------------------------------------------
enum class SkillId : u8 {
    // ---- MIRROR: reflection purity ------------------------------------
    PerfectReflection = 0,
    PrismFacet        = 1,
    AbsorberFacet     = 2,
    ChainMirror       = 3,
    VoidFacet         = 4,

    // ---- FRAME: geometry ---------------------------------------------
    RotateInstinct    = 5,
    BlinkDash         = 6,
    CurvedFacet       = 7,
    FrameShift        = 8,
    KleinLock         = 9,

    // ---- WELL: force -------------------------------------------------
    HeavyShot         = 10,
    GravityWell       = 11,
    AnchorFacet       = 12,
    OrbitalRecall     = 13,
    Singularity       = 14,

    Count             = 15,
    None              = 255,
};

// ---------------------------------------------------------------------------
// SkillNode - one row in the skill tree.
// ---------------------------------------------------------------------------
struct SkillNode {
    SkillId     id;
    const char* name;        // display name, uppercase
    const char* desc;        // short description, lowercase
    int         min_level;   // minimum character level to unlock
    SkillId     prereq;      // SkillId::None for root skills
    u8          branch;      // 0=mirror 1=frame 2=well
    u8          row;         // 0..4 within the branch
};

// The full skill table.
[[nodiscard]] const SkillNode& skill_node(SkillId id);

// Precomputed counts for UI.
inline constexpr int SKILL_BRANCH_COUNT = 3;
inline constexpr int SKILL_ROWS_PER_BRANCH = 5;

} // namespace inc::game
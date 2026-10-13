#include "game/skill.hpp"

namespace inc::game {

inline constexpr int SKILL_COUNT = int(SkillId::Count);

static constexpr SkillNode SKILLS[SKILL_COUNT] = {
    // MIRROR
    { SkillId::PerfectReflection, "PERFECT REFLECTION", "+15% damage per bounce",           2,  SkillId::None,              0, 0 },
    { SkillId::PrismFacet,        "PRISM FACET",        "reflect splits into 3",            5,  SkillId::PerfectReflection, 0, 1 },
    { SkillId::AbsorberFacet,     "ABSORBER FACET",     "one facet absorbs bullets",        8,  SkillId::PrismFacet,        0, 2 },
    { SkillId::ChainMirror,       "CHAIN MIRROR",       "reflected bullets survive longer", 12, SkillId::AbsorberFacet,     0, 3 },
    { SkillId::VoidFacet,         "VOID FACET",         "press V: delete nearest bullet",   15, SkillId::ChainMirror,       0, 4 },
    // FRAME
    { SkillId::RotateInstinct,    "ROTATE INSTINCT",    "+25% rotation speed",              3,  SkillId::None,              1, 0 },
    { SkillId::BlinkDash,         "BLINK DASH",         "dash teleports instead of moving", 6,  SkillId::RotateInstinct,    1, 1 },
    { SkillId::CurvedFacet,       "CURVED FACET",       "+10% reflect speed",               9,  SkillId::BlinkDash,         1, 2 },
    { SkillId::FrameShift,        "FRAME SHIFT",        "press F: swap shield 180 deg",     13, SkillId::CurvedFacet,       1, 3 },
    { SkillId::KleinLock,         "KLEIN LOCK",         "+50% damage on banked shots",      16, SkillId::FrameShift,        1, 4 },
    // WELL
    { SkillId::HeavyShot,         "HEAVY SHOT",         "+60% damage, -25% reflect speed",  4,  SkillId::None,              2, 0 },
    { SkillId::GravityWell,       "GRAVITY WELL",       "reflected bullets curve",          7,  SkillId::HeavyShot,         2, 1 },
    { SkillId::AnchorFacet,       "ANCHOR FACET",       "killing shots leave a flash",      10, SkillId::GravityWell,       2, 2 },
    { SkillId::OrbitalRecall,     "ORBITAL RECALL",     "dash twice for a recall",          14, SkillId::AnchorFacet,       2, 3 },
    { SkillId::Singularity,       "SINGULARITY",        "bullets gently pull toward you",   17, SkillId::OrbitalRecall,     2, 4 },
};

const SkillNode& skill_node(SkillId id) {
    const int idx = int(id);
    if (idx < 0 || idx >= SKILL_COUNT) return SKILLS[0];
    return SKILLS[idx];
}

} // namespace inc::game
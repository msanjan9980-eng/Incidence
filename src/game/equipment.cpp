#include "game/equipment.hpp"

namespace inc::game {

const char* facet_name(FacetId id) {
    switch (id) {
        case FacetId::Mirror: return "MIRROR";
        case FacetId::Wide:   return "WIDE MIRROR";
        case FacetId::Heavy:  return "HEAVY MIRROR";
        default:              return "";
    }
}

const char* core_name(CoreId id) {
    switch (id) {
        case CoreId::Glass:  return "GLASS";
        case CoreId::Coral:  return "CORAL";
        case CoreId::Quartz: return "QUARTZ";
        default:             return "";
    }
}

const char* hull_name(HullId id) {
    switch (id) {
        case HullId::Light:    return "LIGHT";
        case HullId::Reactive: return "REACTIVE";
        case HullId::Silent:   return "SILENT";
        default:               return "";
    }
}

const char* facet_desc(FacetId id) {
    switch (id) {
        case FacetId::Mirror: return "baseline";
        case FacetId::Wide:   return "+15% shield width";
        case FacetId::Heavy:  return "+20% reflect speed";
        default:              return "";
    }
}

const char* core_desc(CoreId id) {
    switch (id) {
        case CoreId::Glass:  return "3 HP";
        case CoreId::Coral:  return "4 HP";
        case CoreId::Quartz: return "+25% reflect speed";
        default:             return "";
    }
}

const char* hull_desc(HullId id) {
    switch (id) {
        case HullId::Light:    return "baseline";
        case HullId::Reactive: return "-25% dash cooldown";
        case HullId::Silent:   return "+50% dash impulse";
        default:               return "";
    }
}

f32 equip_shield_span_mult(const Equipment& eq) {
    return (eq.facet == FacetId::Wide) ? 1.15f : 1.0f;
}

f32 equip_reflect_speed_mult(const Equipment& eq) {
    f32 m = 1.0f;
    if (eq.facet == FacetId::Heavy)  m *= 1.20f;
    if (eq.core  == CoreId::Quartz)  m *= 1.25f;
    return m;
}

int equip_bonus_hp(const Equipment& eq) {
    return (eq.core == CoreId::Coral) ? 1 : 0;
}

f32 equip_dash_cooldown_mult(const Equipment& eq) {
    return (eq.hull == HullId::Reactive) ? 0.75f : 1.0f;
}

f32 equip_dash_impulse_mult(const Equipment& eq) {
    return (eq.hull == HullId::Silent) ? 1.50f : 1.0f;
}

} // namespace inc::game
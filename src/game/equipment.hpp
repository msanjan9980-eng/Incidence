#pragma once

#include "core/math.hpp"

namespace inc::game {

enum class FacetId : u8 { Mirror = 0, Wide = 1, Heavy = 2, Count = 3 };
enum class CoreId  : u8 { Glass  = 0, Coral = 1, Quartz = 2, Count = 3 };
enum class HullId  : u8 { Light  = 0, Reactive = 1, Silent = 2, Count = 3 };

struct Equipment {
    FacetId facet = FacetId::Mirror;
    CoreId  core  = CoreId::Glass;
    HullId  hull  = HullId::Light;
};

const char* facet_name(FacetId id);
const char* core_name(CoreId id);
const char* hull_name(HullId id);

const char* facet_desc(FacetId id);
const char* core_desc(CoreId id);
const char* hull_desc(HullId id);

f32 equip_shield_span_mult(const Equipment& eq);
f32 equip_reflect_speed_mult(const Equipment& eq);
int equip_bonus_hp(const Equipment& eq);
f32 equip_dash_cooldown_mult(const Equipment& eq);
f32 equip_dash_impulse_mult(const Equipment& eq);

} // namespace inc::game
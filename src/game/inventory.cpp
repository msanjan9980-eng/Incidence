#include "game/inventory.hpp"

namespace inc::game {

bool Inventory::is_unlocked(FacetId id) const noexcept { return (unlocked & (1u << int(id)))       != 0; }
bool Inventory::is_unlocked(CoreId  id) const noexcept { return (unlocked & (1u << (3 + int(id)))) != 0; }
bool Inventory::is_unlocked(HullId  id) const noexcept { return (unlocked & (1u << (6 + int(id)))) != 0; }

void Inventory::unlock(FacetId id) noexcept { unlocked |= u16(1u << int(id)); }
void Inventory::unlock(CoreId  id) noexcept { unlocked |= u16(1u << (3 + int(id))); }
void Inventory::unlock(HullId  id) noexcept { unlocked |= u16(1u << (6 + int(id))); }

FacetId Inventory::next_facet(FacetId from, int dir) const noexcept {
    const int n = int(FacetId::Count);
    int i = int(from);
    for (int k = 0; k < n; ++k) {
        i = (i + dir + n) % n;
        if (is_unlocked(FacetId(i))) return FacetId(i);
    }
    return from;
}

CoreId Inventory::next_core(CoreId from, int dir) const noexcept {
    const int n = int(CoreId::Count);
    int i = int(from);
    for (int k = 0; k < n; ++k) {
        i = (i + dir + n) % n;
        if (is_unlocked(CoreId(i))) return CoreId(i);
    }
    return from;
}

HullId Inventory::next_hull(HullId from, int dir) const noexcept {
    const int n = int(HullId::Count);
    int i = int(from);
    for (int k = 0; k < n; ++k) {
        i = (i + dir + n) % n;
        if (is_unlocked(HullId(i))) return HullId(i);
    }
    return from;
}

} // namespace inc::game
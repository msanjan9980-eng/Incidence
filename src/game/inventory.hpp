#pragma once

#include "core/math.hpp"
#include "game/equipment.hpp"

namespace inc::game {

// Unlocked bitmask: bits 0-2 = facets, 3-5 = cores, 6-8 = hulls.
// Default: Mirror (bit 0), Glass (bit 3), Light (bit 6) => 73.
struct Inventory {
    u16 unlocked = (1u << 0) | (1u << 3) | (1u << 6);

    [[nodiscard]] bool is_unlocked(FacetId id) const noexcept;
    [[nodiscard]] bool is_unlocked(CoreId  id) const noexcept;
    [[nodiscard]] bool is_unlocked(HullId  id) const noexcept;

    void unlock(FacetId id) noexcept;
    void unlock(CoreId  id) noexcept;
    void unlock(HullId  id) noexcept;

    [[nodiscard]] FacetId next_facet(FacetId from, int dir) const noexcept;
    [[nodiscard]] CoreId  next_core (CoreId  from, int dir) const noexcept;
    [[nodiscard]] HullId  next_hull (HullId  from, int dir) const noexcept;
};

} // namespace inc::game
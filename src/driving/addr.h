#pragma once
// A(address in DRIVING.XBE): that address in this build. PAL's own build leaves it alone; src/driving_usa is these sources with
// their functions renamed and this table (tools/map_bond.py writes all of it). Unmapped does not compile.
#include <cstdint>

#if defined(AUF_LANG_USA)
#include "driving/addr_usa.h"
#endif

#ifdef AUF_ADDR_TABLE
uint32_t NotMappedByToolsMapBond();  // not constexpr: reaching it does not compile
#endif

consteval uint32_t A(uint32_t pal) {
#ifdef AUF_ADDR_TABLE
    switch (pal) {
        AUF_ADDR_TABLE
        default: return NotMappedByToolsMapBond();
    }
#else
    return pal;
#endif
}

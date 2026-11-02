#pragma once
// A(english address): that address in this build. bond_eng.xbe's own build leaves it alone; src/bond_fre and src/bond_ger are the same
// sources with their functions renamed and this table (tools/map_bond.py writes all of it). Unmapped does not compile. Comments keep the English addresses throughout.
#include <cstdint>

#if defined(AUF_LANG_FRE)
#include "bond/addr_fre.h"
#elif defined(AUF_LANG_GER)
#include "bond/addr_ger.h"
#elif defined(AUF_LANG_USA)
#include "bond/addr_usa.h"
#endif

#ifdef AUF_ADDR_TABLE
uint32_t NotMappedByToolsMapBond();  // not constexpr: reaching it does not compile
#endif

consteval uint32_t A(uint32_t english) {
#ifdef AUF_ADDR_TABLE
    switch (english) {
        AUF_ADDR_TABLE
        default: return NotMappedByToolsMapBond();
    }
#else
    return english;
#endif
}

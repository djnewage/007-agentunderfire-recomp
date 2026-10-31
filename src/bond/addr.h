#pragma once
// A(english address): that address in this build. bond_eng.xbe's own build leaves it alone; src/bond_fre and src/bond_ger are the same
// sources with their functions renamed and this table (tools/map_bond.py writes all of it). Unmapped does not compile. Comments keep the English addresses throughout.
#include <cstdint>

#if defined(AUF_LANG_FRE)
#include "bond/addr_fre.h"
#elif defined(AUF_LANG_GER)
#include "bond/addr_ger.h"
#endif

constexpr uint32_t A(uint32_t english) {
#ifdef AUF_ADDR_TABLE
    switch (english) {
        AUF_ADDR_TABLE
        default: throw "tools/map_bond.py has not mapped this address";
    }
#else
    return english;
#endif
}

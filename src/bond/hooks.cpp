// Hooks for bond_eng.xbe, the game.
#include <cstring>

#include "common/auf.h"
#include "kernel/kernel.h"

namespace auf {
extern const int kImage = kBond;

// a level's pause (0x265C0 sets it; 0x26D70 then skips the frame's input), or the front end, by the game state's name
// (a pointer at [0x3B2414] + 0x10, compared at 0x266B6)
bool MenuUp() {
    if (kernel::R32(0x3B2428) || kernel::R32(0x3B2418)) return true;
    uint32_t state = kernel::R32(0x3B2414);
    if (!state) return true;  // starting
    const uint32_t name = kernel::R32(state + 0x10);
    return name && !std::strncmp(reinterpret_cast<const char*>(&kernel::G<uint8_t>(name)), "frontend", 8);
}
bool OnGun() { return false; }
}  // namespace auf

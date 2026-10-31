// Hooks for bond_eng.xbe, the game.
#include "bond/addr.h"
#include <cstring>

#include "common/auf.h"
#include "kernel/kernel.h"

namespace auf {
extern const int kImage = kBond;
extern const uint32_t kAddrMission = A(0x2A9F98), kAddrRunMedal = A(0x3B2D08), kAddrBestMedals = A(0x2A9C24);

// a level's pause (0x265C0 sets it; 0x26D70 then skips the frame's input), or the front end, by the game state's name
// (a pointer at [0x3B2414] + 0x10, compared at 0x266B6)
bool MenuUp() {
    if (kernel::R32(A(0x3B2428)) || kernel::R32(A(0x3B2418))) return true;
    uint32_t state = kernel::R32(A(0x3B2414));
    if (!state) return true;  // starting
    const uint32_t name = kernel::R32(state + 0x10);
    return name && !std::strncmp(reinterpret_cast<const char*>(&kernel::G<uint8_t>(name)), "frontend", 8);
}
bool OnGun() { return false; }
}  // namespace auf

// A text's typing-out at 0x5BE11 ends its 256-byte copy at the count of letters shown, which a fast text's speed takes
// past the buffer (1071 on the Multiplayer page). On the console that zero lands in other memory; here past the stack's
// top, a crash. By then the whole text shows, so the copy's own last byte does the same.
void ClampTextEnd(X86Context& ctx, uint8_t*) {
    if (ctx.eax > 0xFF) ctx.eax = 0xFF;
}

// Hooks for DRIVING.XBE, the driving levels (EA's engine, an earlier NightFire Driving.xbe).
#include <cstring>

#include "common/auf.h"
#include "kernel/kernel.h"

namespace auf {
extern const int kImage = kDriving;
// the name of the control file in force, its first letter cleared once read: "data/Control/povcfg0.def" on a mounted gun,
// ".../drivecfg.def" in the car (0x17FED0, FUN_00050530's singleton; found comparing mis02a's memory with mis02b's)
bool OnGun() {
    return !std::strncmp(reinterpret_cast<const char*>(&kernel::G<uint8_t>(0x17FED0 + 13)), "povcfg", 6);
}
// a pause in play (0x1801D8: the only byte set across one, found comparing memory before, during and after), or the level
// not in play yet (0x1E1818, set from its start to the end of the intro)
bool MenuUp() { return kernel::G<uint8_t>(0x1801D8) != 0 || kernel::R32(0x1E1818) != 0; }
}  // namespace auf

// Hooks for DRIVING.XBE, the driving levels (EA's engine, an earlier NightFire Driving.xbe).
#include <cstring>

#include "common/auf.h"
#include "kernel/kernel.h"

namespace auf {
extern const int kImage = kDriving;
extern const uint32_t kAddrMission = 0, kAddrRunMedal = 0, kAddrBestMedals = 0;
// the name of the control file in force, its first letter cleared once read: "data/Control/povcfg0.def" on a mounted gun,
// ".../drivecfg.def" in the car (0x17FED0, FUN_00050530's singleton; found comparing mis02a's memory with mis02b's)
bool OnGun() {
    return !std::strncmp(reinterpret_cast<const char*>(&kernel::G<uint8_t>(0x17FED0 + 13)), "povcfg", 6);
}
// the level's state 3: paused, or a movie or the mission's end (0x77A20 and 0x503C0 stop the camera on it; NightFire's later
// version of this engine has the same at its 0x234E24), or the level not in play yet (0x1E1818, set from its start to the end of
// the intro)
bool MenuUp() { return kernel::R32(0x1BEB1C) == 3 || kernel::R32(0x1E1818) != 0; }
bool PlayerFree() { return true; }
bool Zoomed() { return false; }
void ScopeZoom(int) {}
}  // namespace auf

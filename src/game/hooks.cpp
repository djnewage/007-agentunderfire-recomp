// Hooks for default.xbe: the logos and movies before the game.
#include <cstring>

#include "common/auf.h"
#include "kernel/kernel.h"
#include "xdk/hle.h"

namespace auf {
extern const int kImage = kDefault;
extern const uint32_t kAddrMission = 0, kAddrRunMedal = 0, kAddrBestMedals = 0;
bool MenuUp() { return false; }
bool OnGun() { return false; }
bool PlayerFree() { return true; }
bool Zoomed() { return false; }
int MenuClick() { return kClickGo; }
bool PointerWanted() { return false; }
void ScopeZoom(int) {}
}  // namespace auf

// the language: 0x24052 is XGetLanguage (1 English, 3 German, 4 French), which 0x222B0 turns into the game it launches
X86_FUNC(sub_00024052) {
    const uint32_t codes[3] = {1, 4, 3};
    ctx.eax = codes[auf::language % 3];
    ctx.esp += 4;
}

// skip_intro: 0x20BD0 (cdecl: the movie's name) plays one and returns at its end or a button. The boot sequence 0x21130
// plays EAG and MGM, then ATT when it started with nothing shown yet; what is left is its relaunch into the game.
// HILITE.MPC, the attract loop default.xbe comes back for from an idle title, stays.
X86_EXTERN_FUNC(__imp__sub_00020BD0);
X86_FUNC(sub_00020BD0) {
    const char* name = reinterpret_cast<const char*>(&kernel::G<uint8_t>(kernel::R32(ctx.esp + 4)));
    if (auf::skip_intro && !std::strstr(name, "HILITE")) {
        ctx.esp += 4;  // cdecl: the return address only
        return;
    }
    __imp__sub_00020BD0(ctx, base);
}

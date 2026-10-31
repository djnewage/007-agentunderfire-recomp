// Sixty frames a second and more on foot.
//
// The game asks for every second vertical blank: the device's parameters are set to interval 2 at 0x139015, and 0x138940 (cdecl:
// a mode, 0 immediate, 1 to 4 one to eight blanks, else default) sets them for a Reset. Everything runs off the clock (0x25CA0:
// the frame's seconds at 0x3B2420 from the time counter, the server at its own sv_fps), so nothing else depends on it. With the
// default interval the runtime paces frames by the frame rate cap alone (Settings::fps_cap, 60 unless changed).
// AUF_30FPS=1 keeps the console's.
#include "bond/addr.h"
#include <cstdlib>

#include "common/auf.h"
#include "core/log.h"
#include "kernel/kernel.h"
#include "xdk/hle.h"

namespace {
constexpr uint32_t kParamsInterval = A(0x346788);  // the present parameters' interval (0x346758 + 0x30)
constexpr uint32_t kModeDefault = 5;

bool Unlocked() {
    static const bool on = [] {
        bool v = auf::uncapped && !std::getenv("AUF_30FPS");
        LOGR("auf: on foot %s", v ? "paced by the frame rate cap" : "at the console's 30 frames a second");
        return v;
    }();
    return on;
}
}  // namespace

// after 0x139015 wrote the first parameters
void DefaultInterval(X86Context&, uint8_t*) {
    if (Unlocked()) kernel::W32(kParamsInterval, 0);
}

X86_EXTERN_FUNC(__imp__sub_00138940);
X86_FUNC(sub_00138940) {
    if (Unlocked()) kernel::W32(ctx.esp + 4, kModeDefault);
    __imp__sub_00138940(ctx, base);
}

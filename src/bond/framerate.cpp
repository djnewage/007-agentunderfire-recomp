// Sixty frames a second and more on foot.
//
// The game asks for every second vertical blank: the device's parameters are set to interval 2 at 0x139015, and 0x138940 (cdecl:
// a mode, 0 immediate, 1 to 4 one to eight blanks, else default) sets them for a Reset. Everything runs off the frame's seconds at 0x3B2420 (0x25CA0, the server at its own sv_fps), so with
// those measured (below) nothing else depends on the rate: walking 4 seconds ends in the same place at 60 and 144. With the
// default interval the runtime paces frames by the frame rate cap alone (Settings::fps_cap, 60 unless changed, 120 to 240 and
// Unlimited as chosen: app::g_pace_hz lifts the console's 60 from a present interval).
// AUF_30FPS=1 keeps the console's.
#include "bond/addr.h"
#include <cstdlib>

#include "app/app.h"
#include "common/auf.h"
#include "core/log.h"
#include "kernel/kernel.h"
#include "xdk/hle.h"

namespace {
constexpr uint32_t kParamsInterval = A(0x346788);  // the present parameters' interval (0x346758 + 0x30)
constexpr uint32_t kModeDefault = 5;
constexpr uint32_t kFrameSeconds = A(0x3B2420), kBlankSeconds = A(0x346DB0);
constexpr uint32_t kPaceHz = 1000;  // above any cap; Unlimited stops here too

bool Unlocked() {
    static const bool on = [] {
        bool v = auf::uncapped && !std::getenv("AUF_30FPS");
        LOGR("auf: on foot %s", v ? "paced by the frame rate cap" : "at the console's 30 frames a second");
        if (v) app::g_pace_hz = kPaceHz;
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
// The frame's seconds: 0x25CA0 measures them from the time counter (0x25D07), then takes 0x138760's instead, the device's count of
// blanks a frame: 1/60 however fast it draws, so above 60 the game ran that much faster. The measured time is what it gets, past a
// stop of a tenth of a second the game's.
X86_EXTERN_FUNC(__imp__sub_00138760);
X86_FUNC(sub_00138760) {
    if (Unlocked()) {
        const float measured = kernel::G<float>(kFrameSeconds);
        if (measured > 0 && measured < 0.1f) kernel::G<float>(kBlankSeconds) = measured;
    }
    __imp__sub_00138760(ctx, base);
}

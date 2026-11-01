// Hooks for DRIVING.XBE, the driving levels (EA's engine, an earlier NightFire Driving.xbe).
#include <chrono>
#include <cstdlib>
#include <cstring>

#include "common/auf.h"
#include "core/log.h"
#include "kernel/kernel.h"
#include "xdk/hle.h"

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

// Sixty frames a second. The engine runs one step of the game per tick of its timer and draws one frame a step; this PAL build
// asks 0xE0540 for 50 ticks a second, NightFire's later build of the engine for 60. Everything counts in steps, so the whole level
// plays a fifth faster, as a step-counting game does at 60. The timer's period is whole milliseconds and 1000 / 60 would be 16,
// 62.5 ticks against a cap of 60: it runs each millisecond and the tick 0xE0500 (stdcall, 5 arguments) goes through only when due.
// Slow motion steps less often (+0x20 of the scheduler 0x180364) and still shows only the frames it steps.
namespace {
constexpr uint32_t kTicksPerSecond = 60, kTimerRate = 0x1E59E8;
bool Sixty() {
    static const bool on = [] {
        bool v = auf::uncapped && !std::getenv("AUF_30FPS");
        LOGR("auf: driving at %u steps a second", v ? kTicksPerSecond : 50);
        return v;
    }();
    return on;
}
}  // namespace

X86_EXTERN_FUNC(__imp__sub_000E0540);
X86_FUNC(sub_000E0540) {
    const bool started = kernel::R32(0x1E59B8) != 0;
    if (Sixty() && !started) kernel::G<uint32_t>(ctx.esp + 4) = 1000;
    __imp__sub_000E0540(ctx, base);
    if (Sixty() && !started) ctx.eax = kernel::G<uint32_t>(kTimerRate) = kTicksPerSecond;
}

X86_EXTERN_FUNC(__imp__sub_000E0500);
X86_FUNC(sub_000E0500) {
    if (Sixty()) {
        using Clock = std::chrono::steady_clock;
        static Clock::time_point next = Clock::now();
        const auto now = Clock::now();
        if (now < next) {
            ctx.esp += 4 + 0x14;
            return;
        }
        next += std::chrono::nanoseconds(1000000000 / kTicksPerSecond);
        if (now > next + std::chrono::milliseconds(100)) next = now;  // a long stop: no burst of steps after it
    }
    __imp__sub_000E0500(ctx, base);
}

// Slow motion: the scene's event 0x466D0 (thiscall, the scale at +4) sets the scheduler's scale, and each step draws one frame, so a
// quarter of the speed is a quarter of the frames. Held to half: 30 frames a second, the effect kept and the scene over sooner.
X86_EXTERN_FUNC(__imp__sub_000466D0);
X86_FUNC(sub_000466D0) {
    const uint32_t event = ctx.ecx;
    const float asked = kernel::G<float>(event + 4);
    if (asked > 0 && asked < 0.5f) kernel::G<float>(event + 4) = 0.5f;
    LOG_FIRST(16, "auf: slow motion, the scene asks for %.3f", asked);
    __imp__sub_000466D0(ctx, base);
}

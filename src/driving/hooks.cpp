// Hooks for DRIVING.XBE, the driving levels (EA's engine, an earlier NightFire Driving.xbe).
#include <chrono>
#include <cstdlib>
#include <cstring>

#include "common/auf.h"
#include "driving/addr.h"
#include "core/log.h"
#include "kernel/kernel.h"
#include "xdk/hle.h"

namespace auf {
extern const int kImage = kDriving;
#ifdef AUF_LANG_USA
extern const bool kUsa = true;
#else
extern const bool kUsa = false;
#endif
extern const uint32_t kAddrMission = 0, kAddrRunMedal = 0, kAddrBestMedals = 0;
// the name of the control file in force, its first letter cleared once read: "data/Control/povcfg0.def" on a mounted gun,
// ".../drivecfg.def" in the car (0x17FED0, FUN_00050530's singleton; found comparing mis02a's memory with mis02b's)
bool LevelPaused() { return false; }
bool OnGun() {
    return !std::strncmp(reinterpret_cast<const char*>(&kernel::G<uint8_t>(A(0x17FED0) + 13)), "povcfg", 6);
}
// the level's state 3: paused, or a movie or the mission's end (0x77A20 and 0x503C0 stop the camera on it; NightFire's later
// version of this engine has the same at its 0x234E24), or the level not in play yet (0x1E1818, set from its start to the end of
// the intro)
bool MenuUp() { return kernel::R32(A(0x1BEB1C)) == 3 || kernel::R32(A(0x1E1818)) != 0; }
bool PlayerFree() { return true; }
bool Zoomed() { return false; }
void ScopeZoom(int) {}
}  // namespace auf

// Sixty frames a second. The engine runs one step of the game per tick of its timer and draws one frame a step; this PAL build
// asks 0xE0540 for 50 ticks a second, NightFire's later build of the engine for 60. Everything counts in steps, so without the numbers
// below the whole level would play a fifth faster. The timer's period is whole milliseconds and 1000 / 60 would be 16,
// 62.5 ticks against a cap of 60: it runs each millisecond and the tick 0xE0500 (stdcall, 5 arguments) goes through only when due.
// Slow motion steps less often (+0x20 of the scheduler 0x180364) and still shows only the frames it steps.
//
// A step's length is written into the code as numbers, and the USA build (Driving.xbe, 60 ticks) has them for 60 where this has
// them for 50, the level data's tuning being the same: compared by sdk/tools/symbols/xbe_port.py, each below is 1/50 or 50 here and
// 1/60 or 60 there. Without them 60 ticks play a fifth too fast. The 576 and 288 that also differ are the screen's, not time.
namespace {
constexpr uint32_t kTicksPerSecond = 60, kTimerRate = A(0x1E59E8);
#ifndef AUF_LANG_USA  // its numbers are for 60 already
const uint32_t kSeconds[] = {
    0x133CF8, 0x1340A0, 0x134184, 0x134294, 0x134330, 0x13447C, 0x13451C, 0x134624, 0x1346E4, 0x13479C,
    0x13483C, 0x1348E0, 0x134A28, 0x134AEC, 0x134B80, 0x13573C, 0x1357D0, 0x135974, 0x135984, 0x1359A8,
    0x1359BC, 0x1359F0, 0x1372F0, 0x137478, 0x1376C0, 0x137968, 0x1385D0, 0x1385D8, 0x138608, 0x1386F4,
    0x138744, 0x138790, 0x138848, 0x138870, 0x13888C, 0x1388F0, 0x138938, 0x138AB8, 0x138B68, 0x1394D8,
    0x139CA0, 0x13A060, 0x13A088, 0x13A110, 0x13A164, 0x13A250, 0x13A288, 0x13A400, 0x13A414, 0x13A4F0,
    0x13A590, 0x13A5CC, 0x13A670, 0x13A6EC, 0x13A744, 0x13A80C, 0x13A870, 0x13A910, 0x13A98C, 0x13AA48,
    0x13AAD0, 0x13AAE8, 0x13AAEC, 0x13AAF4, 0x13ABCC, 0x13ABD0, 0x13ACA0, 0x13ACD0, 0x13AD84, 0x13AF94,
    0x13AFE4, 0x13AFF8, 0x13B124, 0x13B130, 0x13B17C, 0x13B1F4, 0x13B220,
};
const uint32_t kRates[] = {0x135738, 0x1385D4, 0x1386F0, 0x13AAF0, 0x13AD80};
#endif
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
    const bool started = kernel::R32(A(0x1E59B8)) != 0;
    if (Sixty() && !started) {
        kernel::G<uint32_t>(ctx.esp + 4) = 1000;
#ifndef AUF_LANG_USA
        for (uint32_t va : kSeconds) kernel::G<float>(va) = 1.0f / kTicksPerSecond;
        for (uint32_t va : kRates) kernel::G<float>(va) = float(kTicksPerSecond);
#endif
    }
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

// the same two numbers as arguments (cdecl 0x24ED0 from 0x5822F, thiscall 0x98CA0 from 0x110E70)
X86_EXTERN_FUNC(__imp__sub_00024ED0);
X86_FUNC(sub_00024ED0) {
    if (Sixty()) kernel::G<float>(ctx.esp + 4) = 1.0f / kTicksPerSecond;
    __imp__sub_00024ED0(ctx, base);
}
X86_EXTERN_FUNC(__imp__sub_00098CA0);
X86_FUNC(sub_00098CA0) {
    if (Sixty() && kernel::R32(ctx.esp + 4) == 50) kernel::W32(ctx.esp + 4, kTicksPerSecond);
    __imp__sub_00098CA0(ctx, base);
}

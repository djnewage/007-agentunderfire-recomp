// The mouse aims the mounted guns as in a PC shooter, not as a stick with lag.
//
// The camera's update 0x77A20 (fastcall: the camera) eases its rates +0x250 (up and down) and +0x254 (left and right) to their
// targets +0x260 and +0x264, set by the right stick, and turns by them each frame, scaled by the view's half angle (+0x60), so a
// zoom slows it. Held full right the stick turned the camera 1.55 radians (+0x1BC) in 390 frames: 0.004 a frame. While the
// keyboard and mouse play on a gun the counts go straight into rates and targets, so there is no easing and the turn follows the
// mouse: the on-foot rate, 0.0008 radians a count. Headless (no mouse): AUF_MOUSE_TEST=dx,dy,from,to, counts a call for calls from..to.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "common/auf.h"
#include "core/input.h"
#include "core/log.h"
#include "kernel/kernel.h"
#include "xdk/hle.h"

namespace {
constexpr uint32_t kUpRate = 0x250, kSideRate = 0x254, kUpTarget = 0x260, kSideTarget = 0x264;
constexpr float kRatePerCount = 0.0008f / 0.004f;

void SetF(uint32_t va, float f) { std::memcpy(&kernel::G<uint32_t>(va), &f, 4); }

bool Counts(float& x, float& y) {
    static const struct Test {
        float dx = 0, dy = 0;
        uint32_t from = 0, to = 0;
    } test = [] {
        Test t;
        if (const char* e = std::getenv("AUF_MOUSE_TEST")) std::sscanf(e, "%f,%f,%u,%u", &t.dx, &t.dy, &t.from, &t.to);
        return t;
    }();
    static uint32_t call = 0;
    call++;
    if (test.to) {
        bool on = call >= test.from && call <= test.to;
        x = on ? test.dx : 0, y = on ? test.dy : 0;
        return true;
    }
    static auto last = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    float dt = std::clamp(std::chrono::duration<float>(now - last).count(), 0.001f, 0.1f);
    last = now;
    if (!input::UsingKeyboard() || !input::TakeMouse(x, y)) return false;
    x *= 2400 * dt, y *= 2400 * dt;  // TakeMouse's rate back to counts
    return true;
}
}  // namespace

X86_EXTERN_FUNC(__imp__sub_00077A20);
X86_FUNC(sub_00077A20) {
    const uint32_t cam = ctx.ecx;
    float x, y;
    if (cam && auf::OnGun() && !auf::MenuUp() && Counts(x, y)) {
        SetF(cam + kSideRate, x * kRatePerCount);
        SetF(cam + kSideTarget, x * kRatePerCount);
        SetF(cam + kUpRate, y * kRatePerCount);
        SetF(cam + kUpTarget, y * kRatePerCount);
        static bool said = false;
        if (!said && (x || y)) {
            said = true;
            LOGR("auf: the mouse aims the mounted gun directly");
        }
    }
    __imp__sub_00077A20(ctx, base);
}

// Widescreen and the field of view in the driving levels.
//
// Every view is built by 0x102990 (thiscall: horizontal field of view in degrees, aspect, near, far), as NightFire's 0xE46D0,
// called from 0x846B0 (the game's views: twice the camera's half angle, aspect 4:3) and 0x7C530 (an auxiliary view, aspect 1,
// left alone). A wider frame widens the horizontal angle in tangent and the aspect with it: the vertical angle is the game's,
// and the cull data the view keeps follows from the same arguments. The field of view scales the angle first.
#include <algorithm>
#include <cmath>
#include <cstring>

#include "common/auf.h"
#include "kernel/kernel.h"
#include "xdk/hle.h"

namespace {
constexpr float kPi = 3.14159265f;
float GetF(uint32_t va) {
    float f;
    std::memcpy(&f, &kernel::G<uint32_t>(va), 4);
    return f;
}
void SetF(uint32_t va, float f) { std::memcpy(&kernel::G<uint32_t>(va), &f, 4); }
}  // namespace

X86_EXTERN_FUNC(__imp__sub_00102990);
X86_FUNC(sub_00102990) {
    float aspect = GetF(ctx.esp + 8);
    if (aspect > 1.2f) {  // the game's views, not the auxiliary one
        float k = auf::FrameWidth(), fov = GetF(ctx.esp + 4) * auf::FovScale();
        if (k > 1.0f + 1e-3f) {
            fov = 2 * std::atan(k * std::tan(fov * kPi / 360)) * 180 / kPi;
            SetF(ctx.esp + 8, aspect * k);
        }
        SetF(ctx.esp + 4, std::min(fov, 170.0f));
    }
    __imp__sub_00102990(ctx, base);
}

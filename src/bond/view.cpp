// Widescreen and the field of view on foot.
//
// The projection's ratio is (target width / height) * [0x3467B8] / [0x3467BC], the two set to 1 and 1.1 at 0x138F94 and
// read only by 0x1387E0, which the four projection helpers call: 0x13A350 (the draw), 0x13A3D0 and 0x13A440 (name tags),
// 0x13A4B0 (the culling planes, HUD scales). So the 4:3 console picture is 1.1 narrower than true. On a wider frame the
// numerator becomes that width over 4:3 times 1.1: true proportions, the game's vertical 60 degrees, and drawing and
// culling agree. On a 4:3 frame it stays the console's 1.
// The field of view scales the angle those helpers are given; the scope divides the "fov" setting (0xBD78x), so it
// zooms in proportion. The first person weapon, drawn through 0x13A350 from 0x14B654, keeps its own.
#include "bond/addr.h"
#include <algorithm>
#include <cstring>

#include "common/auf.h"
#include "kernel/kernel.h"
#include "vr/vr.h"
#include "xdk/hle.h"

namespace {
float GetF(uint32_t va) {
    float f;
    std::memcpy(&f, &kernel::G<uint32_t>(va), 4);
    return f;
}
void SetF(uint32_t va, float f) { std::memcpy(&kernel::G<uint32_t>(va), &f, 4); }

void ScaleFov(uint32_t va) { SetF(va, std::min(GetF(va) * auf::FovScale(), 2.9f)); }  // short of 180 degrees


// the culling frustum (0x15CD00, from the ratio through 0x13A4B0) reaches past the frame's edges: on it, what stands just off
// the screen and lights what is on it popped as the view turned (a TV's overscan hid that). Harry confirmed 2.2 times the
// tangent in EoN's driving levels, the same complaint
constexpr float kCullMargin = 2.2f;
float g_margin = 1.0f;
}  // namespace

X86_EXTERN_FUNC(__imp__sub_001387E0);
X86_FUNC(sub_001387E0) {
    float k = auf::FrameWidth();
    SetF(A(0x3467B8), (k > 1.0f + 1e-3f ? k * 1.1f : 1.0f) * g_margin);
    __imp__sub_001387E0(ctx, base);
}

X86_EXTERN_FUNC(__imp__sub_0015CD00);
X86_FUNC(sub_0015CD00) {
    // (view matrix, near, far, field of view, the frustum): five planes of 0x20 bytes there, their count at +0xC0
    const uint32_t frustum = kernel::R32(ctx.esp + 0x14);
    g_margin = kCullMargin;
    __imp__sub_0015CD00(ctx, base);
    // In a headset the title's camera is the aiming hand and the head looks elsewhere: no plane around the hand's view holds
    // all the head sees (a gun held down, or to one side). No planes: nothing is left out for where it is. What the rooms
    // hide from the camera's place is still left out.
    if (vr::Enabled() && frustum) kernel::W32(frustum + 0xC0, 0);
    g_margin = 1.0f;
}

X86_EXTERN_FUNC(__imp__sub_0013A350);
X86_FUNC(sub_0013A350) {
    if (vr::Enabled() || kernel::R32(ctx.esp) != A(0x14B654)) ScaleFov(ctx.esp + 8);  // in a headset the weapon is in the same view
    __imp__sub_0013A350(ctx, base);
}
X86_EXTERN_FUNC(__imp__sub_0013A3D0);
X86_FUNC(sub_0013A3D0) {
    ScaleFov(ctx.esp + 8);
    __imp__sub_0013A3D0(ctx, base);
}
X86_EXTERN_FUNC(__imp__sub_0013A440);
X86_FUNC(sub_0013A440) {
    ScaleFov(ctx.esp + 8);
    __imp__sub_0013A440(ctx, base);
}
X86_EXTERN_FUNC(__imp__sub_0013A4B0);
X86_FUNC(sub_0013A4B0) {
    ScaleFov(ctx.esp + 0xC);
    __imp__sub_0013A4B0(ctx, base);
}

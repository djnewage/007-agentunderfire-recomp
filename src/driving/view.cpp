// Widescreen and the field of view in the driving levels.
//
// Every view is built by 0x102990 (thiscall: horizontal field of view in degrees, aspect, near, far), as NightFire's 0xE46D0,
// called from 0x846B0 (the game's views: twice the camera's half angle, aspect 4:3) and 0x7C530 (an auxiliary view, aspect 1,
// left alone). A wider frame widens the horizontal angle in tangent and the aspect with it: the vertical angle is the game's,
// and the cull data the view keeps follows from the same arguments. The field of view scales the angle first.
#include <vector>
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

// What the world draws is chosen apart from the view above: the list builder 0x7DC10 tests each thing's sphere against the frustum
// at 0x190640, whose planes 0x7B7F0 builds through 0x7E3C0 (thiscall: position, camera, half angle in degrees, far) from the camera's
// half angle alone, 33 degrees whatever the view's width (draw counts did not change at all from 4:3 to four times that). Given the
// wider angle here; it also reaches past the frame's edges by EoN's margin (2.2 times the tangent, Harry confirmed it there):
// on the edge, what stands just off the screen and lights what is on it popped as the car turned.
X86_EXTERN_FUNC(__imp__sub_0007E3C0);
X86_FUNC(sub_0007E3C0) {
    constexpr float kCullMargin = 2.2f, kMost = 80.0f;
    float half = GetF(ctx.esp + 0xC) * auf::FovScale();
    float wide = std::atan(std::max(auf::FrameWidth(), 1.0f) * kCullMargin * std::tan(half * kPi / 180)) * 180 / kPi;
    SetF(ctx.esp + 0xC, std::min(wide, kMost));
    __imp__sub_0007E3C0(ctx, base);
}

// The loading picture: 0xBDEC0 copies it into the back buffer by CPU, so the 2D squeeze never sees it
// and the frame's widening stretches it. Each row is shrunk into the frame's 4:3 middle, black either side, before the unlock
// (0xBDF75: [esp+0x1C] the rows, the locked rectangle's pitch [esp+0x20] and bits [esp+0x24]; a row is the pitch's width).
void LoadingPictureCentred(X86Context& ctx, uint8_t*) {
    const float k = auf::FrameWidth();
    if (k <= 1.001f) return;
    const uint32_t rows = kernel::R32(ctx.esp + 0x1C), pitch = kernel::R32(ctx.esp + 0x20), bits = kernel::R32(ctx.esp + 0x24);
    const uint32_t w = pitch / 4, n = uint32_t(float(w) / k), left = (w - n) / 2;
    if (!bits || !w || !n) return;
    std::vector<uint32_t> row(w);
    for (uint32_t y = 0; y < rows; y++) {
        auto* p = reinterpret_cast<uint32_t*>(&kernel::G<uint8_t>(bits + y * pitch));
        std::memcpy(row.data(), p, w * 4);
        for (uint32_t x = 0; x < w; x++) p[x] = x >= left && x < left + n ? row[size_t(x - left) * w / n] : 0xFF000000u;
    }
}

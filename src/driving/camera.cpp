// The driving levels' camera, read where the game keeps it, for a headset (vr/vr.h).
//
// (USA addresses throughout; the PAL disc's are not mapped yet, and there every function here says it knows nothing, which
// leaves the headset with the renderer's own reading of the camera, a frame or two late.)
//
// The view manager [0x1F6ED8] holds each player's cameras: +4 player 1's RPlayerCamera. Its matrix at +0x10 is the camera
// in the world, a row an axis (the third, +0x30, the way it looks; the fourth, +0x40, where it is), y up. +0x130 is the
// camera mode in use, an index into the table [0x1F6D88] (0x18 bytes an entry, [0x1F6DB4] of them, built from
// data\render\camera.ini by 0x6D370), whose word at +8 says what kind it is: 0x01 Bumper, 0x02 Heli (the chase camera),
// 0x04 Spline, 0x08 Ellipse (the pause's orbit), 0x10 Fixed, 0x20 Tumble, 0x40 an animated camera, 0x80 Collision, 0x100
// AutoDrive (a mounted gun's: its update is 0x778B0, hooked in mouse.cpp). The byte 0x1CE91B is the cut scenes' Heli camera.
// A change of camera is asked for as the game's own camera button does (0x751F0): 0x74F60 (thiscall: the mode, the word at
// 0x1CE424, five zeros). The button passes over the Bumper camera; a headset asks for it once a level, since sitting at
// the car's nose is the view to have there (auf::car_bumper).
#include <algorithm>
#include <cmath>
#include <cstring>

#include "common/auf.h"
#include "core/log.h"
#include "kernel/kernel.h"
#include "vr/vr.h"
#include "xdk/hle.h"

#ifdef AUF_LANG_USA
X86_EXTERN_FUNC(sub_00074F60);
#endif

namespace auf {
namespace {
constexpr float kPi = 3.14159265f;
#ifdef AUF_LANG_USA
constexpr uint32_t kViews = 0x1F6ED8, kModes = 0x1F6D88, kModeCount = 0x1F6DB4, kCinematic = 0x1CE91B, kSwitchWord = 0x1CE424;
constexpr uint32_t kMatrix = 0x10, kMode = 0x130, kEntry = 0x18, kEntryKind = 8;

float GetF(uint32_t va) {
    float f;
    std::memcpy(&f, &kernel::G<uint32_t>(va), 4);
    return f;
}
uint32_t Camera() {
    const uint32_t views = kernel::R32(kViews);
    return views ? kernel::R32(views + 4) : 0;
}
// which way each of the matrix's rows points beside the renderer's reading of the same camera (its right, its up, the way
// it looks): settled once, from frames where the two agree closely
float g_sign[3] = {1, 1, 1};
bool g_settled = false;
bool Axes(float right[3], float up[3], float ahead[3]) {
    const uint32_t cam = Camera();
    if (!cam) return false;
    float* rows[3] = {right, up, ahead};
    for (int r = 0; r < 3; r++) {
        float l = 0;
        for (int c = 0; c < 3; c++) rows[r][c] = GetF(cam + kMatrix + uint32_t(r * 0x10 + c * 4)), l += rows[r][c] * rows[r][c];
        l = std::sqrt(l);
        if (!(l > 0.5f && l < 2.0f)) return false;  // not a camera's matrix yet
        for (int c = 0; c < 3; c++) rows[r][c] *= g_sign[r] / l;
    }
    return true;
}
void Settle() {
    float seen[9], right[3], up[3], ahead[3];
    if (g_settled || !vr::SeenAxes(seen) || !Axes(right, up, ahead)) return;
    const float* mine[3] = {right, up, ahead};
    float dots[3];
    for (int r = 0; r < 3; r++) dots[r] = mine[r][0] * seen[r * 3] + mine[r][1] * seen[r * 3 + 1] + mine[r][2] * seen[r * 3 + 2];
    static int agreed = 0;
    if (std::fabs(dots[0]) < 0.97f || std::fabs(dots[1]) < 0.97f || std::fabs(dots[2]) < 0.97f) {
        agreed = 0;
        return;
    }
    if (++agreed < 20) return;  // the renderer's is a frame or two old: a camera at rest for a third of a second
    for (int r = 0; r < 3; r++) g_sign[r] = dots[r] < 0 ? -1.0f : 1.0f;
    g_settled = true;
    LOGR("auf: the camera's matrix read: its rows are %s, %s, %s", g_sign[0] > 0 ? "right" : "left", g_sign[1] > 0 ? "up" : "down",
         g_sign[2] > 0 ? "ahead" : "behind");
}
uint32_t Kind(uint32_t cam, int32_t* index = nullptr) {
    const uint32_t table = kernel::R32(kModes), count = kernel::R32(kModeCount);
    const int32_t mode = int32_t(kernel::R32(cam + kMode));
    if (index) *index = mode;
    if (!table || mode < 0 || uint32_t(mode) >= count) return 0;
    return kernel::G<uint16_t>(table + uint32_t(mode) * kEntry + kEntryKind);
}
// the game's own request for a camera (0x751F0's call)
void Ask(X86Context& ctx, uint8_t* base, uint32_t cam, uint32_t mode) {
    const X86Context kept = ctx;
    for (int i = 0; i < 5; i++) X86_PUSH32(0);
    X86_PUSH32(uint32_t(kernel::G<uint16_t>(kSwitchWord)));
    X86_PUSH32(mode);
    X86_PUSH32(0);  // where it returns to: not used
    ctx.ecx = cam;
    sub_00074F60(ctx, base);
    ctx = kept;
}
#endif
bool g_gun = false;  // the hand has a mounted gun: its heading and the camera's differ by g_gun_offset
float g_gun_offset = 0;
}  // namespace

void GunOffset(bool on, float offset) { g_gun = on, g_gun_offset = offset; }

bool CameraAngles(float& heading, float& pitch) {
#ifdef AUF_LANG_USA
    float right[3], up[3], ahead[3];
    if (!g_settled || !Axes(right, up, ahead)) return false;
    // the heading: what lies ahead in the level plane (z and x, y being up), counted so that a turn to the left raises it,
    // carried on over full turns
    const float a1 = ahead[2], a2 = ahead[0], t1 = -right[2], t2 = -right[0];
    const float now = (a2 * t1 - a1 * t2 >= 0 ? 1.0f : -1.0f) * std::atan2(a1, a2) * 180 / kPi;
    static float last = now, carried = now;
    carried += std::remainder(now - last, 360.0f);
    last = now;
    heading = carried;
    pitch = std::asin(std::clamp(ahead[1], -1.0f, 1.0f)) * 180 / kPi;
    return true;
#else
    (void)heading, (void)pitch;
    return false;
#endif
}

void FrameCamera(X86Context& ctx, uint8_t* base) {
    if (!vr::Running()) return;
#ifdef AUF_LANG_USA
    const uint32_t cam = Camera();
    float right[3], up[3], ahead[3];
    Settle();
    if (cam && g_settled && Axes(right, up, ahead)) {
        int32_t mode = 0;
        const uint32_t kind = Kind(cam, &mode);
        static uint32_t said = ~0u;
        if (kind != said) LOGR("auf: the camera is of kind %X (mode %d)", kind, mode), said = kind;
        const bool cut = MenuUp() || (kind & (0x04 | 0x08 | 0x10 | 0x40)) || mode == int32_t(kernel::G<int8_t>(kCinematic)) || !kind;
        if (cut) return vr::Flat();  // a menu, the intro, a scripted camera: on the screen
        // level axes of the same heading, seen from the camera's (x right, y up, z towards the viewer): y the world's up, z
        // what is left of the camera's own z, x across them
        float y[3] = {right[1], up[1], -ahead[1]}, z[3] = {0, 0, 1};
        const auto unit = [](float* v) {
            const float l = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
            for (int i = 0; i < 3; i++) v[i] = l > 0 ? v[i] / l : 0;
            return l;
        };
        unit(y);
        const float d = z[0] * y[0] + z[1] * y[1] + z[2] * y[2];
        for (int i = 0; i < 3; i++) z[i] -= d * y[i];
        if (unit(z) > 0.2f) {
            const float x[3] = {y[1] * z[2] - y[2] * z[1], y[2] * z[0] - y[0] * z[2], y[0] * z[1] - y[1] * z[0]};
            float level[9];
            for (int i = 0; i < 3; i++) level[i * 3] = x[i], level[i * 3 + 1] = y[i], level[i * 3 + 2] = z[i];
            float heading, pitch;
            if ((kind & 0x100) && g_gun && CameraAngles(heading, pitch)) vr::View(level, true, heading - g_gun_offset);
            else vr::View(level, false, 0);
        }
        // the car: the view from its nose, asked for once a level (a camera object is a level's)
        static uint32_t asked_of = 0;
        static int asked = 0;
        if (cam != asked_of) asked_of = cam, asked = 0;
        if (car_bumper && kind == 0x02 && asked < 3) {
            const uint32_t table = kernel::R32(kModes), count = kernel::R32(kModeCount);
            for (uint32_t i = 0; i < count; i++)
                if (kernel::G<uint16_t>(table + i * kEntry + kEntryKind) == 0x01) {
                    static uint64_t next = 0;
                    const uint64_t now = uint64_t(vr::Ticks());
                    if (now < next) break;
                    next = now + 1500;  // again if it did not take, three times at most
                    asked++;
                    LOGR("auf: the car's bumper camera asked for (mode %u)", i);
                    Ask(ctx, base, cam, i);
                    break;
                }
        }
        return;
    }
#else
    (void)ctx, (void)base;
#endif
    // not known (the PAL disc, or not settled yet): as the renderer reads it
    if (MenuUp()) vr::Flat();
    else if (!(OnGun() && vr::HandAims())) vr::Ahead();
}
}  // namespace auf

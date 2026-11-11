// A headset: the player's shots leave from the gun in the hand, and its flash and tracers start there.
// USA bond.xbe only so far: these addresses are not in tools/map_bond.py's table, and the other discs' builds shoot from
// the view's middle as the game does.
#define AUF_LANG_USA
//
// A weapon's shot asks 0x9D110 (Weapon's vtable +0x214; thiscall: where from, forward, left, up, each a pointer or null,
// ret 0x10) for its start and its axes. For a weapon whose owner (+0x2CC) is the Player (vtable 0x1E1370) the start is
// the view's origin (0xAED60) and the axes those of the aim (player +0x758), which in a headset is the hand's already
// (mouse.cpp), so only the start is wrong: it becomes the hand's place. Bullets (0x83680), projectiles (0x87920) and the
// SSR4000 (0xA4340) all start from what it gives; an Actor's shots take another branch and are not touched.
//
// A tracer's start and the muzzle flash are a tag's place (tag_barrel) from 0x63E70 (cdecl: a context, the entity, the
// tag, where it is, its axes), which for the weapon drawn in first person (the entity's +0x138 has bit 1) works back from
// where the view model is drawn on screen: not where a headset draws it. There too the hand's place is given, a little
// ahead along the aim, where the barrel ends.
#include <cstring>

#include "common/auf.h"
#include "core/log.h"
#include "kernel/kernel.h"
#include "vr/vr.h"
#include "xdk/hle.h"

namespace {
constexpr uint32_t kOwner = 0x2CC, kPlayerVtable = 0x1E1370, kFirstPerson = 0x138;
constexpr float kBarrel = 0.25f;  // metres from the hand to the barrel's end

void SetF(uint32_t va, float f) { std::memcpy(&kernel::G<uint32_t>(va), &f, 4); }

// the hand in the world: from the camera's place along its axes (right, down, towards the viewer), the hand being given
// to its right, above it and towards the viewer; ahead metres further along the aim
// (from: the place the game itself gave, the view's origin; the camera matrix's own place is not used, the frustum hook
// seeing more views than the player's)
bool Hand(const float from[3], float out[3], float ahead) {
    float rows[12], at[3];
    if (!auf::ViewCamera(rows) || !vr::TitleHand(at)) return false;
    // the camera kept is not the player's just now (a cut scene's, another view's): its axes are not to be used either
    float off = 0;
    for (int c = 0; c < 3; c++) off += (rows[9 + c] - from[c]) * (rows[9 + c] - from[c]);
    if (off > 64.0f * 64.0f) return false;
    const float back = at[2] - ahead * vr::Units();
    for (int c = 0; c < 3; c++) out[c] = from[c] + rows[c] * at[0] - rows[3 + c] * at[1] + rows[6 + c] * back;
    return true;
}
float GetF(uint32_t va) {
    float f;
    std::memcpy(&f, &kernel::G<uint32_t>(va), 4);
    return f;
}
// the view's origin as the player's last shot had it, and when: the flash and the tracers of that shot start from it
float g_eye[3] = {};
uint64_t g_eye_at = 0;
}  // namespace

X86_EXTERN_FUNC(__imp__sub_0009D110);
X86_FUNC(sub_0009D110) {
    const uint32_t weapon = ctx.ecx, pos = kernel::R32(ctx.esp + 4);
    __imp__sub_0009D110(ctx, base);
    const uint32_t owner = weapon ? kernel::R32(weapon + kOwner) : 0;
    if (!pos || !owner || kernel::R32(owner) != kPlayerVtable) return;
    const float from[3] = {GetF(pos), GetF(pos + 4), GetF(pos + 8)};
    for (int c = 0; c < 3; c++) g_eye[c] = from[c];
    g_eye_at = vr::Ticks();
    float hand[3];
    if (!Hand(from, hand, 0)) return;
    for (int c = 0; c < 3; c++) SetF(pos + uint32_t(c * 4), hand[c]);
    LOG_FIRST(1, "auf: the player's shots leave from the hand");
}

// (the place is counted from the view's origin of the shot just fired, not from the camera the frustum hook kept, which
// is at times another view's: with that, shots and tracers went astray)
X86_EXTERN_FUNC(__imp__sub_00063E70);
X86_FUNC(sub_00063E70) {
    const uint32_t entity = kernel::R32(ctx.esp + 8), pos = kernel::R32(ctx.esp + 0x10);
    __imp__sub_00063E70(ctx, base);
    float hand[3];
    if (!entity || !pos || !(kernel::R32(entity + kFirstPerson) & 2)) return;
    if (!g_eye_at || vr::Ticks() - g_eye_at > 400 || !Hand(g_eye, hand, kBarrel)) return;
    for (int c = 0; c < 3; c++) SetF(pos + uint32_t(c * 4), hand[c]);
    LOG_FIRST(1, "auf: the held weapon's flash and tracers start at the hand");
}

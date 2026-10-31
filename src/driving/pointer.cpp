// DRIVING.XBE's pause menu: the pointer, and a fourth row, PC Settings, that opens the settings window.
//
// The pause object is [0x1DFB08]: +0x2E0 the pause is up, +0x420 the tab, +0x424 the row (0 Continue, 1 Restart, 2 Quit), +0x428
// the prompt's choice (0 none, 1 Yes, 2 No), +0x42C the row the prompt replaces (1 Restart, 2 Quit). Plain fields: the draw 0xBAEE0
// reads them each frame. 0xB5B00 (thiscall, the event) takes presses: 1 A, 7 left, 9 right, 0xB up, 0xD down. It has the tabs' own
// set up, so tabs go through it. A row is a widget of x +0, y +4 and a string number +0x34, which 0xBC8C0 looks up. The pointer's
// rectangles are the widgets' places in 640x480, their widths from captures of the English text.
#include <atomic>
#include <cstdint>
#include <cstring>

#include "app/app.h"
#include "common/auf.h"
#include "core/log.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xdk/hle.h"

X86_EXTERN_FUNC(sub_000B20D0);
X86_EXTERN_FUNC(sub_000B4560);
X86_EXTERN_FUNC(sub_000B10B0);
X86_EXTERN_FUNC(sub_000B2BE0);

namespace {
constexpr uint32_t kObject = 0x1DFB08, kUp = 0x2E0, kTab = 0x420, kRow = 0x424, kPrompt = 0x428, kPromptRow = 0x42C;
constexpr uint32_t kColour = 0x16B910, kBright = 0xFFC4E6FF, kDim = 0xFF65839F;
constexpr uint32_t kQuitWidget = 0x158, kChevron3 = 0x1B8, kStringId = 0x34, kOurString = 0x7FF0, kRowPitch = 26;
constexpr int kOurRow = 3;
constexpr uint32_t kEventA = 1, kEventLeft = 7, kEventRight = 9, kEventUp = 0xB, kEventDown = 0xD;
std::atomic<int> g_click{auf::kClickGo};
std::atomic<bool> g_up{false};
uint32_t g_widget = 0, g_chevron = 0, g_string = 0;

struct Rect {
    float x0, y0, x1, y1;
    bool Has(float x, float y) const { return x >= x0 && x < x1 && y >= y0 && y < y1; }
};
const Rect kRows[4] = {{113, 176, 269, 198}, {113, 202, 265, 224}, {113, 228, 240, 250}, {113, 254, 260, 276}};
const Rect kTabs[4] = {{100, 112, 184, 134}, {244, 112, 355, 134}, {413, 112, 500, 134}, {540, 112, 610, 134}};

float GetF(uint32_t va) { return kernel::G<float>(va); }
void SetF(uint32_t va, float f) { std::memcpy(&kernel::G<uint32_t>(va), &f, 4); }

void Call1(X86Context& ctx, uint8_t* base, void (*f)(X86Context&, uint8_t*), uint32_t arg) {
    X86_PUSH32(arg);
    X86_PUSH32(0xBB284u);
    f(ctx, base);
    ctx.esp += 4;
}

// copies of Quit's widget and of its chevron a row lower, once the pause exists
void Make(uint32_t object) {
    if (g_widget) return;
    constexpr uint32_t kBytes = 0xA0;
    const uint32_t quit = kernel::R32(object + kQuitWidget), chevron = kernel::R32(object + kChevron3);
    g_widget = mem::AllocHostContiguous(kBytes, 16);
    g_chevron = mem::AllocHostContiguous(kBytes, 16);
    std::memcpy(&kernel::G<uint8_t>(g_widget), &kernel::G<uint8_t>(quit), kBytes);
    std::memcpy(&kernel::G<uint8_t>(g_chevron), &kernel::G<uint8_t>(chevron), kBytes);
    SetF(g_widget + 4, GetF(quit + 4) + kRowPitch);
    SetF(g_chevron + 4, GetF(chevron + 4) + kRowPitch);
    kernel::W32(g_widget + kStringId, kOurString);
    static const char kText[] = "PC Settings";
    g_string = mem::AllocHostContiguous(sizeof kText, 16);
    std::memcpy(&kernel::G<uint8_t>(g_string), kText, sizeof kText);
    LOGR("auf: PC Settings is a row of the driving pause");
}
}  // namespace

X86_EXTERN_FUNC(__imp__sub_000B5B00);

namespace {
void Event(X86Context& ctx, uint8_t* base, uint32_t object, uint32_t event) {
    const uint32_t ecx = ctx.ecx;
    X86_PUSH32(event);
    X86_PUSH32(0xBC6B0u);
    ctx.ecx = object;
    __imp__sub_000B5B00(ctx, base);  // its ret 4 takes both
    ctx.ecx = ecx;
}

// the game's own step down makes the sound; then the row we want
void SelectRow(X86Context& ctx, uint8_t* base, uint32_t object, int row) {
    kernel::W32(object + kRow, 0);
    Event(ctx, base, object, kEventDown);
    kernel::W32(object + kRow, uint32_t(row));
}
}  // namespace

// in the draw, after the three rows
void DrawPcRow(X86Context& ctx, uint8_t* base) {
    const uint32_t object = ctx.esi;
    Make(object);
    const uint32_t row = kernel::R32(object + kRow);
    Call1(ctx, base, sub_000B20D0, g_widget);
    kernel::W32(kColour, row == kOurRow ? kBright : kDim);
    Call1(ctx, base, sub_000B4560, g_widget + 0x30);
    if (row == kOurRow && !kernel::R32(object + kPrompt)) {
        Call1(ctx, base, sub_000B10B0, g_chevron);
        Call1(ctx, base, sub_000B2BE0, g_chevron);
    }
}

namespace auf {
int MenuClick() { return g_click.load(std::memory_order_relaxed); }
bool PointerWanted() { return g_up.load(std::memory_order_relaxed); }
}  // namespace auf

// the string number of ours
X86_EXTERN_FUNC(__imp__sub_000BC8C0);
X86_FUNC(sub_000BC8C0) {
    if (kernel::R32(ctx.esp + 4) == kOurString && g_string) {
        ctx.eax = g_string;
        ctx.esp += 4;
        return;
    }
    __imp__sub_000BC8C0(ctx, base);
}

// the presses: the fourth row below Quit
X86_FUNC(sub_000B5B00) {
    const uint32_t object = ctx.ecx, event = kernel::R32(ctx.esp + 4);
    const bool rows = !kernel::R32(object + kTab) && !kernel::R32(object + kPrompt);
    const int row = int(kernel::R32(object + kRow));
    if (rows && ((event == kEventDown && row == 2) || row == kOurRow)) {
        if (row == kOurRow && event == kEventA) {
            app::ToggleOverlay();
            ctx.esp += 8;
            return;
        }
        if (event == kEventDown || event == kEventUp) {
            int to = event == kEventUp ? 2 : kOurRow;
            ctx.esp += 8;
            if (to != row) SelectRow(ctx, base, object, to);
            return;
        }
    }
    __imp__sub_000B5B00(ctx, base);
}

// each frame: the pointer's item
X86_EXTERN_FUNC(__imp__sub_000BC6B0);
X86_FUNC(sub_000BC6B0) {
    using namespace auf;
    const uint32_t object = ctx.ecx;
    const bool up = object && kernel::R32(object + kUp) && kernel::R32(0x1BEB1C) == 3;
    g_up.store(up, std::memory_order_relaxed);
    int click = kClickGo;
    float x, y;
    if (up && PointerAt(x, y)) {
        static float last_x = -1, last_y = -1;
        static bool last_press = false;
        const bool press = (PointerButtons() & 1) != 0, moved = x != last_x || y != last_y, edge = press && !last_press;
        last_x = x, last_y = y, last_press = press;
        const int tab = int(kernel::R32(object + kTab));
        const uint32_t prompt = kernel::R32(object + kPrompt);
        click = kClickNone;
        for (int t = 0; t < 4; t++)
            if (kTabs[t].Has(x, y) && edge && t != tab && !prompt) {
                for (int i = 0; i < 4 && int(kernel::R32(object + kTab)) != t; i++) {
                    const uint32_t ecx = ctx.ecx;
                    Event(ctx, base, object, t < int(kernel::R32(object + kTab)) ? kEventLeft : kEventRight);
                    ctx.ecx = ecx;
                }
            }
        if (prompt) {
            const float top = kernel::R32(object + kPromptRow) == 1 ? 202 : 228;
            const Rect yes{293, top, 328, top + 22}, no{353, top, 379, top + 22};
            const int hit = yes.Has(x, y) ? 1 : no.Has(x, y) ? 2 : 0;
            if (hit && (moved || edge)) kernel::W32(object + kPrompt, uint32_t(hit));
            if (hit) click = kClickGo;
        } else if (!tab) {
            for (int r = 0; r < 4; r++)
                if (kRows[r].Has(x, y)) {
                    if ((moved || edge) && int(kernel::R32(object + kRow)) != r) {
                        const uint32_t ecx = ctx.ecx;
                        SelectRow(ctx, base, object, r);
                        ctx.ecx = ecx;
                    }
                    click = kClickGo;
                }
        }
    }
    g_click.store(click, std::memory_order_relaxed);
    __imp__sub_000BC6B0(ctx, base);
}

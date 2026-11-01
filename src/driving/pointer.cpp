// DRIVING.XBE's pause menu: the pointer, and a tab, PC Settings, with its page.
//
// The pause object is [0x1DFB08]: +0x2E0 the pause is up, +0x420 the tab, +0x424 the row (0 Continue, 1 Restart, 2 Quit), +0x428
// the prompt's choice (0 none, 1 Yes, 2 No), +0x42C the row the prompt replaces (1 Restart, 2 Quit). Plain fields: the draw 0xBAEE0
// reads them each frame. 0xB5B00 (thiscall, the event) takes presses: 1 A, 7 left, 9 right, 0xB up, 0xD down. It has the tabs' own
// set up, so tabs go through it. A label is a widget of x +0, y +4 and a string number +0x34, which 0xBC8C0 looks up. The pointer's
// rectangles are the widgets' places in 640x480, their widths from captures of the English text.
// The page is the pause on its first tab, with +0x420 set to 4 from 0xBB1B1, where the draw has drawn the tabs (which index by it) and
// turns to the panel, which has nothing for 4, then 3 for the strip (0xBBC5F); back to 0 after the draw (0xBC730); then rows drawn after the draw (a hook at 0xBC730), as the game draws its own. 0xB5B00 gets 0x10 for B and 6 for START.
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

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
X86_EXTERN_FUNC(sub_000B3740);  // the width of a string in the font set
X86_EXTERN_FUNC(sub_000BC8C0);  // a string by its number

namespace {
constexpr uint32_t kObject = 0x1DFB08, kUp = 0x2E0, kTab = 0x420, kRow = 0x424, kPrompt = 0x428, kPromptRow = 0x42C;
constexpr uint32_t kColour = 0x16B910, kBright = 0xFFC4E6FF, kDim = 0xFF65839F;
constexpr uint32_t kQuitWidget = 0x158, kBriefingTab = 0x10C, kFirstTab = 0x104, kMapTab = 0x110, kChevron3 = 0x1B8, kStringId = 0x34, kOurString = 0x7FF0;
constexpr uint32_t kEventA = 1, kEventLeft = 7, kEventRight = 9, kEventUp = 0xB, kEventDown = 0xD, kEventB = 0x10, kEventStart = 6;
constexpr uint32_t kPageTab = 4, kStringsFirst = kOurString + 1;
constexpr float kPageTop = 150, kPagePitch = 20, kPageX = 110, kPageValueX = 340;
std::atomic<int> g_click{auf::kClickGo};
std::atomic<bool> g_up{false};
uint32_t g_widget = 0, g_chevron = 0, g_string = 0;
// the page: a widget and a string for each row's label and value, and the cursor
uint32_t g_page_widgets[auf::kPcRows * 2] = {}, g_page_strings[auf::kPcRows * 2] = {}, g_page_chevron = 0;
int g_cursor = 0;
bool g_page = false;
constexpr uint32_t kStringBytes = 64;

struct Rect {
    float x0, y0, x1, y1;
    bool Has(float x, float y) const { return x >= x0 && x < x1 && y >= y0 && y < y1; }
};
// wide enough for the French and German rows
const Rect kRows[3] = {{113, 176, 340, 198}, {113, 202, 340, 224}, {113, 228, 340, 250}};
// the labels' places, measured once in the draw (LayOutTabs, TabPlaces); these until then
Rect g_tabs[5] = {{100, 112, 184, 134}, {244, 112, 355, 134}, {413, 112, 500, 134}, {540, 112, 610, 134}, {510, 112, 640, 134}};
uint32_t g_map_widget = 0;  // the Map tab's, while +0x110 holds ours for the lit box
bool g_laid_out = false;
float g_tab_x[5] = {};  // the labels' x, PC Settings' last

float GetF(uint32_t va) { return kernel::G<float>(va); }
void SetF(uint32_t va, float f) { std::memcpy(&kernel::G<uint32_t>(va), &f, 4); }

void Call1(X86Context& ctx, uint8_t* base, void (*f)(X86Context&, uint8_t*), uint32_t arg) {
    X86_PUSH32(arg);
    X86_PUSH32(0xBB284u);
    f(ctx, base);
    ctx.esp += 4;
}

// a Map tab beside Briefing (0xB6043: +0x3DC 1 or 2)
bool HasMap(uint32_t object) {
    const uint32_t m = kernel::R32(object + 0x3DC);
    return m == 1 || m == 2;
}
int LastTab(uint32_t object) { return HasMap(object) ? 3 : 2; }

// a copy of Briefing's label for the tab, and the page's widgets, once the pause exists
void Make(uint32_t object) {
    if (g_widget) return;
    constexpr uint32_t kBytes = 0xA0;
    const uint32_t quit = kernel::R32(object + kQuitWidget), chevron = kernel::R32(object + kChevron3);
    g_widget = mem::AllocHostContiguous(kBytes, 16);
    g_chevron = mem::AllocHostContiguous(kBytes, 16);
    std::memcpy(&kernel::G<uint8_t>(g_widget), &kernel::G<uint8_t>(quit), kBytes);
    std::memcpy(&kernel::G<uint8_t>(g_chevron), &kernel::G<uint8_t>(chevron), kBytes);
    std::memcpy(&kernel::G<uint8_t>(g_widget), &kernel::G<uint8_t>(kernel::R32(object + kBriefingTab)), kBytes);
    kernel::W32(g_widget + kStringId, kOurString);
    const char* text = auf::PcTabName();
    g_string = mem::AllocHostContiguous(uint32_t(std::strlen(text) + 1), 16);
    std::memcpy(&kernel::G<uint8_t>(g_string), text, std::strlen(text) + 1);
    g_map_widget = kernel::R32(object + kMapTab);
    for (int i = 0; i < auf::kPcRows * 2; i++) {
        const uint32_t w = g_page_widgets[i] = mem::AllocHostContiguous(kBytes, 16);
        std::memcpy(&kernel::G<uint8_t>(w), &kernel::G<uint8_t>(quit), kBytes);
        SetF(w, i % 2 ? kPageValueX : kPageX);
        SetF(w + 4, kPageTop + float(i / 2) * kPagePitch);
        kernel::W32(w + kStringId, kStringsFirst + uint32_t(i));
        g_page_strings[i] = mem::AllocHostContiguous(kStringBytes, 16);
    }
    g_page_chevron = mem::AllocHostContiguous(kBytes, 16);
    std::memcpy(&kernel::G<uint8_t>(g_page_chevron), &kernel::G<uint8_t>(chevron), kBytes);
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

namespace {
void OpenPage(uint32_t object) {
    Make(object);
    kernel::W32(object + kTab, 0);
    kernel::W32(object + kPrompt, 0);
    g_page = true;
    g_cursor = -1;  // on the strip, as the other tabs open: left and right go between tabs, down to the rows
    LOGR("auf: PC Settings opened in the driving pause");
}
void ClosePage(uint32_t object) {
    kernel::W32(object + kTab, 0);
    kernel::W32(object + kRow, 0);
    g_page = false;
    LOGR("auf: PC Settings closed in the driving pause");
}
// the game's step's sound, for a row of ours
void Sound(X86Context& ctx, uint8_t* base, uint32_t object) {
    g_page = false;  // the step goes to the game
    kernel::W32(object + kRow, 0);
    Event(ctx, base, object, kEventDown);
    kernel::W32(object + kRow, 0);
    g_page = true;
}
}  // namespace

// in the draw, the tabs done: nothing of the panel's
void PageHidesPanel(X86Context& ctx, uint8_t*) {
    if (g_page) kernel::W32(ctx.esi + kTab, kPageTab);
}

// the panel's paths meet at 0xBBC5F; the strip's boxes and labels after it index by the tab: the Map's, whose widgets always exist, so
// none of the others is bright
void PagePanelDone(X86Context& ctx, uint8_t*) {
    if (g_page) {
        kernel::W32(ctx.esi + kTab, 3);
        kernel::W32(ctx.esi + kMapTab, g_widget);  // the strip's lit box goes by the current tab's label: ours
    }
}

namespace {
// the tabs with PC Settings' beside them, measured in the game's font: as they are if there is room, or closed up (a Map tab, a long
// language)
void LayOutTabs(X86Context& ctx, uint8_t* base, uint32_t object) {
    if (g_laid_out) return;
    g_laid_out = true;
    uint32_t widgets[5];
    float width[5];
    int n = 0;
    for (int i = 0; i < 3 + (HasMap(object) ? 1 : 0); i++) widgets[n++] = i == 3 ? g_map_widget : kernel::R32(object + kFirstTab + uint32_t(i) * 4);
    widgets[n++] = g_widget;
    float total = 0;
    for (int i = 0; i < n; i++) {
        Call1(ctx, base, sub_000B20D0, widgets[i]);
        Call1(ctx, base, sub_000BC8C0, kernel::R32(widgets[i] + kStringId));
        Call1(ctx, base, sub_000B3740, ctx.eax);
        width[i] = float(int32_t(ctx.eax));
        total += width[i];
    }
    constexpr float kLeft = 110, kRight = 632, kWidest = 40;
    const float gap = std::min(kWidest, (kRight - kLeft - total) / float(n - 1));
    float x = kLeft;
    for (int i = 0; i < n; i++) {
        SetF(widgets[i], x);
        const int slot = i == n - 1 ? 4 : i;
        g_tab_x[slot] = x;
        g_tabs[slot] = {x - gap / 2, 112, x + width[i] + gap / 2, 134};
        x += width[i] + gap;
    }
    LOGR("auf: the driving pause's tabs laid out, %d of them, the gap %.0f", n, gap);
}
}  // namespace

// the draw spaces its labels itself (0xBBF50), leaving their x at [esp+0x64 + 8 tab] for the strip below: ours instead; on the page the
// fourth is PC Settings', whose label +0x110 holds
void TabPlaces(X86Context& ctx, uint8_t*) {
    if (!g_laid_out) return;
    for (uint32_t i = 0; i < 4; i++) {
        const float x = i == 3 && g_page ? g_tab_x[4] : g_tab_x[i];
        if (x > 0) SetF(ctx.esp + 0x64 + 8 * i, x);
    }
}

// after the pause's draw: the page's rows
void DrawPcPage(X86Context& ctx, uint8_t* base) {
    const uint32_t object = ctx.esi;
    if (g_page) {
        kernel::W32(object + kTab, 0);
        kernel::W32(object + kMapTab, g_map_widget);
    }
    if (!kernel::R32(object + kUp) || kernel::R32(0x1BEB1C) != 3) return;
    Make(object);
    LayOutTabs(ctx, base, object);
    Call1(ctx, base, sub_000B20D0, g_widget);
    kernel::W32(kColour, g_page ? kBright : kDim);
    Call1(ctx, base, sub_000B4560, g_widget + 0x30);
    if (g_page && HasMap(object)) {  // the draw drew ours in its place
        Call1(ctx, base, sub_000B20D0, g_map_widget);
        kernel::W32(kColour, kDim);
        Call1(ctx, base, sub_000B4560, g_map_widget + 0x30);
    }
    if (!g_page) return;
    const uint32_t chevron = kernel::R32(object + kChevron3);
    std::memcpy(&kernel::G<uint8_t>(g_page_chevron), &kernel::G<uint8_t>(chevron), 0xA0);
    SetF(g_page_chevron + 4, GetF(chevron + 4) - GetF(kernel::R32(object + kQuitWidget) + 4) + kPageTop + float(g_cursor) * kPagePitch);
    for (int i = 0; i < auf::PcRows() * 2; i++) {
        const std::string text = i % 2 ? auf::PcValue(i / 2, auf::Language()) : auf::PcLabel(i / 2, auf::Language());
        std::snprintf(reinterpret_cast<char*>(&kernel::G<uint8_t>(g_page_strings[i])), kStringBytes, "%s", text.c_str());
        Call1(ctx, base, sub_000B20D0, g_page_widgets[i]);
        kernel::W32(kColour, i / 2 == g_cursor ? kBright : kDim);
        Call1(ctx, base, sub_000B4560, g_page_widgets[i] + 0x30);
    }
    if (g_cursor >= 0) {
        Call1(ctx, base, sub_000B10B0, g_page_chevron);
        Call1(ctx, base, sub_000B2BE0, g_page_chevron);
    }
}

namespace auf {
int MenuClick() { return g_click.load(std::memory_order_relaxed); }
bool PointerWanted() { return g_up.load(std::memory_order_relaxed); }
}  // namespace auf

// the string number of ours
X86_EXTERN_FUNC(__imp__sub_000BC8C0);
X86_FUNC(sub_000BC8C0) {
    const uint32_t id = kernel::R32(ctx.esp + 4);
    if (id == kOurString && g_string) {
        ctx.eax = g_string;
        ctx.esp += 4;
        return;
    }
    if (id >= kStringsFirst && id < kStringsFirst + auf::kPcRows * 2 && g_widget) {
        ctx.eax = g_page_strings[id - kStringsFirst];
        ctx.esp += 4;
        return;
    }
    __imp__sub_000BC8C0(ctx, base);
}

// the presses: the fourth row below Quit
X86_FUNC(sub_000B5B00) {
    const uint32_t object = ctx.ecx, event = kernel::R32(ctx.esp + 4);
    if (g_page) {
        ctx.esp += 8;
        // back to the tab before, by the game's step, which sets up what it shows
        const auto back = [&] {
            ClosePage(object);
            kernel::W32(object + kTab, uint32_t(LastTab(object) - 1));
            Event(ctx, base, object, kEventRight);
        };
        if (event == kEventB) back();
        else if (event == kEventStart) {
            ClosePage(object);
            Event(ctx, base, object, kEventStart);
        } else if (g_cursor < 0) {
            if (event == kEventLeft) back();
            if (event == kEventDown) {
                g_cursor = 0;
                Sound(ctx, base, object);
            }
        } else if (event == kEventUp && g_cursor == 0) {
            g_cursor = -1;
            Sound(ctx, base, object);
        } else if (event == kEventUp || event == kEventDown) {
            g_cursor = (g_cursor + auf::PcRows() + (event == kEventUp ? -1 : 1)) % auf::PcRows();
            Sound(ctx, base, object);
        } else if (event == kEventLeft || event == kEventRight) auf::PcStep(g_cursor, event == kEventLeft ? -1 : 1);
        else if (event == kEventA) auf::PcStep(g_cursor, auf::PcClickDir(kPageValueX));
        return;
    }
    // right from the last tab: PC Settings
    if (event == kEventRight && int(kernel::R32(object + kTab)) == LastTab(object) && !kernel::R32(object + kPrompt)) {
        OpenPage(object);
        ctx.esp += 8;
        return;
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
    if (!up) g_page = false;
    if (up && g_page && PointerAt(x, y)) {
        static float page_x = -1, page_y = -1;
        const bool moved = x != page_x || y != page_y;
        page_x = x, page_y = y;
        click = kClickNone;
        // a click on a tab leaves the page for it
        static bool page_press = false;
        const bool press = (PointerButtons() & 1) != 0, page_edge = press && !page_press;
        page_press = press;
        if (page_edge)
            for (int t = 0; t < 4; t++)
                if (g_tabs[t].Has(x, y) && (t < 3 || HasMap(object))) {
                    ClosePage(object);
                    for (int i = 0; i < t; i++) Event(ctx, base, object, kEventRight);  // from the first
                    ctx.ecx = object;
                    break;
                }
        const int row = int((y - kPageTop + 6) / kPagePitch);
        if (x >= kPageX && x < 560 && y >= kPageTop - 6 && row >= 0 && row < auf::PcRows()) {
            if (moved && row != g_cursor) {
                const uint32_t ecx = ctx.ecx;
                g_cursor = row;
                Sound(ctx, base, object);
                ctx.ecx = ecx;
            }
            click = kClickGo;
        }
    } else if (up && PointerAt(x, y)) {
        static float last_x = -1, last_y = -1;
        static bool last_press = false;
        const bool press = (PointerButtons() & 1) != 0, moved = x != last_x || y != last_y, edge = press && !last_press;
        last_x = x, last_y = y, last_press = press;
        const int tab = int(kernel::R32(object + kTab));
        const uint32_t prompt = kernel::R32(object + kPrompt);
        click = kClickNone;
        if (g_tabs[4].Has(x, y) && edge && !prompt) OpenPage(object);
        for (int t = 0; t < 4; t++)
            if (g_tabs[t].Has(x, y) && edge && t != tab && !prompt && (t < 3 || HasMap(object))) {
                for (int i = 0; i < 4 && int(kernel::R32(object + kTab)) != t; i++) {
                    const uint32_t ecx = ctx.ecx;
                    Event(ctx, base, object, t < int(kernel::R32(object + kTab)) ? kEventLeft : kEventRight);
                    ctx.ecx = ecx;
                }
            }
        if (prompt) {
            const float top = kernel::R32(object + kPromptRow) == 1 ? 202 : 228;
            // the row's right part in two, not the words' width: the words are small targets
            const Rect yes{262, top - 4, 340, top + 26}, no{340, top - 4, 440, top + 26};
            const int hit = yes.Has(x, y) ? 1 : no.Has(x, y) ? 2 : 0;
            if (hit && (moved || edge)) kernel::W32(object + kPrompt, uint32_t(hit));
            if (hit) click = kClickGo;
        } else if (!tab) {
            for (int r = 0; r < 3; r++)
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

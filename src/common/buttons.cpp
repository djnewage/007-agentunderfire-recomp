// Keyboard prompts: while the keyboard and mouse play, the button pictures of bond_eng.xbe's menus and hints show the keys.
//
// Its front end and pause draw the prompts from a 256x256 A8R8G8B8 atlas (A, B, X, Y and the d-pads among other art); "Press A"
// hints from a 32x64 DXT3 picture of an A. When one is bound (GameInfo::texture_bound) and as the disc has it (its bytes' FNV-1a), it
// gets a replacement four times larger with those cells drawn as keys: the first binding of each control, A as Enter and B as
// Backspace where those select and go back (a menu, or Bond not the player's to move), the d-pads as arrow keys. With a controller
// the replacement goes and the disc's pictures are drawn. The guest's bytes are left alone. Shapes: src/common/strokes.h.
// DRIVING.XBE's icons are other textures, not done. Headless: AUF_BUTTON_DUMP=1 writes each drawing to buttons_<data>_<w>.rgba.
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "common/auf.h"
#include "common/strokes.h"
#include "core/input.h"
#include "core/log.h"
#include "gpu/texture.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"

namespace auf {
namespace {
constexpr int kSharp = 4;
constexpr uint16_t kVkLeft = 0x25, kVkUp = 0x26, kVkRight = 0x27, kVkDown = 0x28, kEnter = 0x0D, kBackspace = 0x08;
constexpr Colour kWhite{240, 240, 240, 255}, kDark{24, 26, 30, 255}, kEdge{200, 205, 215, 255};

struct Cell {
    Area area;
    int control;  // input::k*, or -1 with arrows
    uint16_t arrow0, arrow1;
};
const std::vector<Cell> kAtlasCells = {
    {{1, 90, 23, 112}, input::kY, 0, 0},       {{24, 90, 47, 111}, input::kA, 0, 0},
    {{64, 93, 80, 108}, -1, kVkRight, 0},      {{87, 93, 103, 108}, -1, kVkUp, 0},
    {{129, 93, 145, 108}, -1, kVkLeft, kVkRight}, {{229, 93, 245, 108}, -1, kVkUp, kVkDown},
    {{195, 118, 211, 133}, -1, kVkUp, kVkDown},
    {{6, 150, 24, 168}, input::kB, 0, 0},      {{27, 150, 45, 168}, input::kY, 0, 0},
    {{48, 150, 66, 168}, input::kA, 0, 0},     {{69, 150, 87, 168}, input::kX, 0, 0},
};
const std::vector<Cell> kPressACells = {{{0, 0, 32, 32}, input::kA, 0, 0}};  // drawn square: the hint's quad is

struct Known {
    uint32_t format, w, h, bytes;
    uint64_t disc;
    const std::vector<Cell>* cells;
    uint32_t canvas;  // texels across and down of the drawing
};
const Known kKnown[] = {{0x06, 256, 256, 256 * 256 * 4, 0xC3A73051EF13D69Full, &kAtlasCells, 256},
                        {0x0E, 32, 64, 32 * 64, 0xA17873E88E43A396ull, &kPressACells, 32}};

struct Look {
    bool keys = false, select = false;
    std::array<uint16_t, 4> bound{};
    bool operator==(const Look&) const = default;
};
struct Picture {
    const Known* known = nullptr;
    gpu::TextureDesc desc;
    Look drawn;
};
std::mutex g_lock;
std::map<uint32_t, Picture> g_pictures;
std::map<uint32_t, uint64_t> g_not;  // what else was looked at, by its first bytes

uint64_t Fnv(const uint8_t* p, size_t n) {
    uint64_t h = 0xCBF29CE484222325ull;
    for (size_t i = 0; i < n; i++) h = (h ^ p[i]) * 0x100000001B3ull;
    return h;
}

Look Current() {
    Look l;
    l.keys = input::UsingKeyboard();
    l.select = MenuUp() || !PlayerFree();
    const int controls[4] = {input::kA, input::kB, input::kX, input::kY};
    for (int i = 0; i < 4; i++) l.bound[size_t(i)] = input::ControlKeys(controls[i])[0];
    return l;
}

std::string KeyLabel(uint16_t vk) {
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) return std::string(1, char(vk));
    switch (vk) {
        case 0x01: return "LMB";
        case 0x02: return "RMB";
        case 0x04: return "MMB";
        case 0x08: return "BKSP";
        case 0x09: return "TAB";
        case 0x0D: return "ENTER";
        case 0x1B: return "ESC";
        case 0x20: return "SPACE";
        case 0xA0: case 0xA1: case 0x10: return "SHIFT";
        case 0xA2: case 0xA3: case 0x11: return "CTRL";
        default: return "";
    }
}

void KeyCap(Canvas& c, Area cap, uint16_t vk) {
    float w = cap.x1 - cap.x0, h = cap.y1 - cap.y0, r = std::min(w, h) * 0.2f, edge = std::max(0.5f, std::min(w, h) * 0.06f);
    Fill(c, cap, kEdge, [&](float x, float y) { return BoxDistance(x, y, cap, r, r); });
    Fill(c, cap, kDark, [&](float x, float y) { return BoxDistance(x, y, cap, r, r) + edge; });
    float cx = (cap.x0 + cap.x1) / 2, cy = (cap.y0 + cap.y1) / 2;
    if (vk >= kVkLeft && vk <= kVkDown) {
        float k = std::min(w, h) * 0.25f;
        std::vector<Pt> head;
        if (vk == kVkUp) head = {{cx, cy - k}, {cx + k, cy + k * 0.7f}, {cx - k, cy + k * 0.7f}};
        if (vk == kVkDown) head = {{cx, cy + k}, {cx - k, cy - k * 0.7f}, {cx + k, cy - k * 0.7f}};
        if (vk == kVkLeft) head = {{cx - k, cy}, {cx + k * 0.7f, cy - k}, {cx + k * 0.7f, cy + k}};
        if (vk == kVkRight) head = {{cx + k, cy}, {cx - k * 0.7f, cy + k}, {cx - k * 0.7f, cy - k}};
        Fill(c, cap, kWhite, [&](float x, float y) { return PolygonDistance(x, y, head); });
        return;
    }
    std::string label = KeyLabel(vk);
    if (label.empty()) return;
    float height = std::min(h * 0.5f, (w - 2 * edge - std::max(1.0f, w * 0.12f)) / TextWidth(label.c_str(), 1));
    Text(c, label.c_str(), cx, cy, height, std::max(0.45f, height * 0.2f), kWhite);
}

uint16_t KeyOf(int control, const Look& l) {
    if (control == input::kA) return l.select ? kEnter : l.bound[0];
    if (control == input::kB) return l.select ? kBackspace : l.bound[1];
    return control == input::kX ? l.bound[2] : l.bound[3];
}

void Apply(uint32_t data, Picture& pic, const Look& look) {
    pic.drawn = look;
    if (!look.keys) {
        gpu::ClearReplacement(data);
        LOGR("auf: the button pictures (%08X) as the disc has them", data);
        return;
    }
    const Known& k = *pic.known;
    std::vector<uint32_t> disc;
    const auto* guest = static_cast<const uint8_t*>(kernel::HostPtr(data));
    if (!gpu::DecodeBgra(pic.desc, 0, guest, disc)) return;
    const uint32_t W = k.canvas * kSharp;
    std::vector<uint32_t> out = k.canvas == k.w && k.canvas == k.h ? gpu::UpscalePicture(disc, k.w, k.h, kSharp)
                                                                   : std::vector<uint32_t>(size_t(W) * W, 0);
    Canvas c;
    c.scale = kSharp;
    c.size = int(W);
    c.px.assign(size_t(W) * W, {0, 0, 0, 0});
    for (const Cell& cell : *k.cells) {
        const Area& a = cell.area;
        for (int y = int(a.y0) * kSharp; y < int(a.y1) * kSharp; y++)
            for (int x = int(a.x0) * kSharp; x < int(a.x1) * kSharp; x++) out[size_t(y) * W + x] = 0;
        if (cell.control >= 0) {
            KeyCap(c, a, KeyOf(cell.control, look));
        } else if (!cell.arrow1) {
            KeyCap(c, a, cell.arrow0);
        } else {  // two arrows side by side
            float mid = (a.x0 + a.x1) / 2;
            KeyCap(c, {a.x0, a.y0 + 3, mid, a.y1 - 3}, cell.arrow0);
            KeyCap(c, {mid, a.y0 + 3, a.x1, a.y1 - 3}, cell.arrow1);
        }
    }
    for (uint32_t i = 0; i < W * W; i++) {
        const auto& p = c.px[i];
        if (p[3] <= 0) continue;
        auto ch = [&](int n) { return uint32_t(std::clamp(p[size_t(n)] / p[3], 0.0f, 1.0f) * 255 + 0.5f); };
        out[i] = ch(2) | ch(1) << 8 | ch(0) << 16 | uint32_t(std::clamp(p[3], 0.0f, 1.0f) * 255 + 0.5f) << 24;
    }
    if (std::getenv("AUF_BUTTON_DUMP")) {
        char name[64];
        std::snprintf(name, sizeof name, "buttons_%08X_%u.rgba", data, W);
        if (FILE* f = std::fopen(name, "wb")) {
            std::fwrite(out.data(), 4, out.size(), f);
            std::fclose(f);
        }
    }
    gpu::SetReplacement(data, k.bytes, W, W, std::move(out));
    LOGR("auf: the button pictures (%08X) drawn as keys%s", data, look.select ? ", Enter and Backspace" : "");
}
}  // namespace

// GameInfo::texture_bound
bool ButtonPictures(uint32_t header, uint32_t) {
    gpu::TextureDesc d;
    if (!gpu::Describe(header, d)) return false;
    const Known* k = nullptr;
    for (const Known& c : kKnown)
        if (c.format == d.format && c.w == d.width && c.h == d.height) k = &c;
    if (!k || !mem::IsCommitted(d.data) || !mem::IsCommitted(d.data + k->bytes - 1)) return false;
    std::lock_guard<std::mutex> hold(g_lock);
    const auto* bytes = static_cast<const uint8_t*>(kernel::HostPtr(d.data));
    auto it = g_pictures.find(d.data);
    if (it == g_pictures.end()) {
        // the whole hash once per address and contents: many textures share this shape, bound many times a frame
        uint64_t head = Fnv(bytes, 256);
        auto no = g_not.find(d.data);
        if (no != g_not.end() && no->second == head) return false;
        if (Fnv(bytes, k->bytes) != k->disc) {
            g_not[d.data] = head;
            return false;
        }
        it = g_pictures.emplace(d.data, Picture{k, d, {}}).first;
    } else if (it->second.known != k) {  // the memory holds something else now
        g_pictures.erase(it);
        return false;
    }
    Look look = Current();
    if (look == it->second.drawn) return false;
    Apply(d.data, it->second, look);
    return true;
}

// each poll of controller 1: drawn again when what they should show changes
void RefreshButtons() {
    std::lock_guard<std::mutex> hold(g_lock);
    if (g_pictures.empty()) return;
    Look look = Current();
    for (auto& [data, pic] : g_pictures)
        if (!(look == pic.drawn) && mem::IsCommitted(data) && mem::IsCommitted(data + pic.known->bytes - 1) &&
            Fnv(static_cast<const uint8_t*>(kernel::HostPtr(data)), pic.known->bytes) == pic.known->disc)
            Apply(data, pic, look);
}
}  // namespace auf
